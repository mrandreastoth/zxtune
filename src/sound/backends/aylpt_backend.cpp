/**
*
* @file
*
* @brief  AYLPT backend implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "backend_impl.h"
#include "storage.h"
//common includes
#include <byteorder.h>
#include <error_tools.h>
#include <make_ptr.h>
//library includes
#include <module/conversion/api.h>
#include <module/conversion/types.h>
#include <debug/log.h>
#include <devices/aym.h>
#include <l10n/api.h>
#include <platform/shared_library.h>
#include <sound/backend_attrs.h>
#include <sound/backends_parameters.h>
#include <sound/render_params.h>
//std includes
#include <algorithm>
#include <cstring>
#include <thread>
//text includes
#include "text/backends.h"

#define FILE_TAG F1936398

namespace
{
  const Debug::Stream Dbg("Sound::Backend::Aylpt");
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("sound_backends");
}

namespace Sound
{
namespace AyLpt
{
  const String ID = Text::AYLPT_BACKEND_ID;
  const char* const DESCRIPTION = L10n::translate("Real AY via LPT backend");
  const uint_t CAPABILITIES = CAP_TYPE_HARDWARE;

  //http://logix4u.net/component/content/article/14-parallel-port/15-a-tutorial-on-parallel-port-interfacing
  //http://bulba.untergrund.net/LPT-YM.7z
  enum
  {
    DATA_PORT = 0x378,
    CONTROL_PORT = DATA_PORT + 2,

    //pin14 (Control-1) -> ~BDIR
    PIN_NOWRITE = 0x2,
    //pin16 (Control-2) -> BC1
    PIN_ADDR = 0x4,
    //pin17 (Control-3) -> ~RESET
    PIN_NORESET = 0x8,
    //other unused pins
    PIN_UNUSED = 0xf1,

    CMD_SELECT_ADDR = PIN_ADDR | PIN_NORESET | PIN_UNUSED,
    CMD_SELECT_DATA = PIN_NORESET | PIN_UNUSED,
    CMD_WRITE_COMMIT = PIN_NOWRITE | PIN_NORESET | PIN_UNUSED,
    CMD_RESET_START = PIN_ADDR | PIN_UNUSED,
    CMD_RESET_STOP = PIN_NORESET | PIN_ADDR | PIN_UNUSED,
  };

  static_assert(CMD_SELECT_ADDR == 0xfd, "Invariant");
  static_assert(CMD_SELECT_DATA == 0xf9, "Invariant");
  static_assert(CMD_WRITE_COMMIT == 0xfb, "Invariant");
  static_assert(CMD_RESET_START == 0xf5, "Invariant");
  static_assert(CMD_RESET_STOP == 0xfd, "Invariant");

  class LptPort
  {
  public:
    typedef std::shared_ptr<LptPort> Ptr;
    virtual ~LptPort() {}

    virtual void Control(uint_t val) = 0;
    virtual void Data(uint_t val) = 0;
  };

  class BackendWorker : public Sound::BackendWorker
  {
  public:
    BackendWorker(Parameters::Accessor::Ptr params, Binary::Data::Ptr data, LptPort::Ptr port)
      : Params(Sound::RenderParameters::Create(params))
      , Data(data)
      , Port(port)
    {
    }

    virtual ~BackendWorker()
    {
    }

    virtual VolumeControl::Ptr GetVolumeControl() const
    {
      return VolumeControl::Ptr();
    }

    virtual void Startup()
    {
      Reset();
      Dbg("Successfull start");
      NextFrameTime = std::chrono::steady_clock::now();
      FrameDuration = std::chrono::microseconds(Params->FrameDuration().Get());
    }

    virtual void Shutdown()
    {
      Reset();
      Dbg("Successfull shut down");
    }

    virtual void Pause()
    {
      Reset();
    }

    virtual void Resume()
    {
    }

    virtual void FrameStart(const Module::TrackState& state)
    {
      WaitForNextFrame();
      const uint8_t* regs = static_cast<const uint8_t*>(Data->Start()) + state.Frame() * Devices::AYM::Registers::TOTAL;
      WriteRegisters(regs);
    }

    virtual void FrameFinish(Chunk /*buffer*/)
    {
      NextFrameTime += FrameDuration;
    }
  private:
    void Reset()
    {
      Port->Control(CMD_RESET_STOP);
      Port->Control(CMD_RESET_START);
      Delay();
      Port->Control(CMD_RESET_STOP);
    }

    void Write(uint_t reg, uint_t val)
    {
      Port->Control(CMD_WRITE_COMMIT);
      Port->Data(reg);
      Port->Control(CMD_SELECT_ADDR);
      Port->Data(reg);

      Port->Control(CMD_WRITE_COMMIT);
      Port->Data(val);
      Port->Control(CMD_SELECT_DATA);
      Port->Data(val);
      Port->Control(CMD_WRITE_COMMIT);
    }

    void WaitForNextFrame()
    {
      std::this_thread::sleep_until(NextFrameTime);
    }

    static void Delay()
    {
      //according to datasheets, maximal timing is reset pulse width 5uS
      std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
   
    void WriteRegisters(const uint8_t* data)
    {
      for (uint_t idx = 0; idx <= Devices::AYM::Registers::ENV; ++idx)
      {
        const uint_t reg = Devices::AYM::Registers::ENV - idx;
        const uint_t val = data[reg];
        if (reg != Devices::AYM::Registers::ENV || val != 0xff)
        {
          Write(reg, val);
        }
      }
    }
  private:
    const Sound::RenderParameters::Ptr Params;
    const Binary::Data::Ptr Data;
    const LptPort::Ptr Port;
    std::chrono::steady_clock::time_point NextFrameTime;
    std::chrono::microseconds FrameDuration;
  };

  class DlPortIO : public LptPort
  {
  public:
    explicit DlPortIO(Platform::SharedLibrary::Ptr lib)
      : Lib(lib)
      , WriteByte(reinterpret_cast<WriteFunctionType>(Lib->GetSymbol("DlPortWritePortUchar")))
    {
    }

    virtual void Control(uint_t val)
    {
      WriteByte(CONTROL_PORT, val);
    }

    virtual void Data(uint_t val)
    {
      WriteByte(DATA_PORT, val);
    }
  private:
    const Platform::SharedLibrary::Ptr Lib;
    typedef void (__stdcall *WriteFunctionType)(unsigned short port, unsigned char val);
    const WriteFunctionType WriteByte;
  };

  class DllName : public Platform::SharedLibrary::Name
  {
  public:
    virtual std::string Base() const
    {
      return "dlportio";
    }
    
    virtual std::vector<std::string> PosixAlternatives() const
    {
      return std::vector<std::string>();
    }
    
    virtual std::vector<std::string> WindowsAlternatives() const
    {
      static const std::string ALTERNATIVES[] =
      {
        "inpout32.dll",
        "inpoutx64.dll"
      };
      return std::vector<std::string>(ALTERNATIVES, std::end(ALTERNATIVES));
    }
  };

  LptPort::Ptr LoadLptLibrary()
  {
    static const DllName NAME;
    const Platform::SharedLibrary::Ptr lib = Platform::SharedLibrary::Load(NAME);
    return MakePtr<DlPortIO>(lib);
  }

  class BackendWorkerFactory : public Sound::BackendWorkerFactory
  {
  public:
    explicit BackendWorkerFactory(LptPort::Ptr port)
      : Port(port)
    {
    }

    virtual BackendWorker::Ptr CreateWorker(Parameters::Accessor::Ptr params, Module::Holder::Ptr holder) const
    {
      static const Module::Conversion::AYDumpConvertParam spec;
      if (const Binary::Data::Ptr data = Module::Convert(*holder, spec, params))
      {
        return MakePtr<BackendWorker>(params, data, Port);
      }
      throw Error(THIS_LINE, translate("Real AY via LPT is not supported for this module."));
    }
  private:
    const LptPort::Ptr Port;
  };
}//AyLpt
}//Sound

namespace Sound
{
  void RegisterAyLptBackend(BackendsStorage& storage)
  {
    try
    {
      const AyLpt::LptPort::Ptr port = AyLpt::LoadLptLibrary();
      const BackendWorkerFactory::Ptr factory = MakePtr<AyLpt::BackendWorkerFactory>(port);
      storage.Register(AyLpt::ID, AyLpt::DESCRIPTION, AyLpt::CAPABILITIES, factory);
    }
    catch (const Error& e)
    {
      storage.Register(AyLpt::ID, AyLpt::DESCRIPTION, AyLpt::CAPABILITIES, e);
    }
  }
}
