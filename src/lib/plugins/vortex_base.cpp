#include "vortex_base.h"
#include "vortex_io.h"

#include <convert_parameters.h>

#include <error.h>

#include <boost/algorithm/string.hpp>

#define FILE_TAG 23C2245

namespace
{
  const std::size_t LIMITER(~std::size_t(0));

  //TODO
  const String::value_type TEXT_EMPTY[] = {0};

  //Table #0 of Pro Tracker 3.3x - 3.4r
  const uint16_t FreqTable_PT_33_34r[96] = {
    0x0c21, 0x0b73, 0x0ace, 0x0a33, 0x09a0, 0x0916, 0x0893, 0x0818, 0x07a4, 0x0736, 0x06ce, 0x066d,
      0x0610, 0x05b9, 0x0567, 0x0519, 0x04d0, 0x048b, 0x0449, 0x040c, 0x03d2, 0x039b, 0x0367, 0x0336,
      0x0308, 0x02dc, 0x02b3, 0x028c, 0x0268, 0x0245, 0x0224, 0x0206, 0x01e9, 0x01cd, 0x01b3, 0x019b,
      0x0184, 0x016e, 0x0159, 0x0146, 0x0134, 0x0122, 0x0112, 0x0103, 0x00f4, 0x00e6, 0x00d9, 0x00cd,
      0x00c2, 0x00b7, 0x00ac, 0x00a3, 0x009a, 0x0091, 0x0089, 0x0081, 0x007a, 0x0073, 0x006c, 0x0066,
      0x0061, 0x005b, 0x0056, 0x0051, 0x004d, 0x0048, 0x0044, 0x0040, 0x003d, 0x0039, 0x0036, 0x0033,
      0x0030, 0x002d, 0x002b, 0x0028, 0x0026, 0x0024, 0x0022, 0x0020, 0x001e, 0x001c, 0x001b, 0x0019,
      0x0018, 0x0016, 0x0015, 0x0014, 0x0013, 0x0012, 0x0011, 0x0010, 0x000f, 0x000e, 0x000d, 0x000c
  };

  //Table #0 of Pro Tracker 3.4x - 3.5x
  const uint16_t FreqTable_PT_34_35[96] = {
    0x0c22, 0x0b73, 0x0acf, 0x0a33, 0x09a1, 0x0917, 0x0894, 0x0819, 0x07a4, 0x0737, 0x06cf, 0x066d,
      0x0611, 0x05ba, 0x0567, 0x051a, 0x04d0, 0x048b, 0x044a, 0x040c, 0x03d2, 0x039b, 0x0367, 0x0337,
      0x0308, 0x02dd, 0x02b4, 0x028d, 0x0268, 0x0246, 0x0225, 0x0206, 0x01e9, 0x01ce, 0x01b4, 0x019b,
      0x0184, 0x016e, 0x015a, 0x0146, 0x0134, 0x0123, 0x0112, 0x0103, 0x00f5, 0x00e7, 0x00da, 0x00ce,
      0x00c2, 0x00b7, 0x00ad, 0x00a3, 0x009a, 0x0091, 0x0089, 0x0082, 0x007a, 0x0073, 0x006d, 0x0067,
      0x0061, 0x005c, 0x0056, 0x0052, 0x004d, 0x0049, 0x0045, 0x0041, 0x003d, 0x003a, 0x0036, 0x0033,
      0x0031, 0x002e, 0x002b, 0x0029, 0x0027, 0x0024, 0x0022, 0x0020, 0x001f, 0x001d, 0x001b, 0x001a,
      0x0018, 0x0017, 0x0016, 0x0014, 0x0013, 0x0012, 0x0011, 0x0010, 0x000f, 0x000e, 0x000d, 0x000c
  };

  //Table #1 of Pro Tracker 3.3x - 3.5x
  const uint16_t FreqTable_ST[96] = {
    0x0ef8, 0x0e10, 0x0d60, 0x0c80, 0x0bd8, 0x0b28, 0x0a88, 0x09f0, 0x0960, 0x08e0, 0x0858, 0x07e0,
      0x077c, 0x0708, 0x06b0, 0x0640, 0x05ec, 0x0594, 0x0544, 0x04f8, 0x04b0, 0x0470, 0x042c, 0x03fd,
      0x03be, 0x0384, 0x0358, 0x0320, 0x02f6, 0x02ca, 0x02a2, 0x027c, 0x0258, 0x0238, 0x0216, 0x01f8,
      0x01df, 0x01c2, 0x01ac, 0x0190, 0x017b, 0x0165, 0x0151, 0x013e, 0x012c, 0x011c, 0x010a, 0x00fc,
      0x00ef, 0x00e1, 0x00d6, 0x00c8, 0x00bd, 0x00b2, 0x00a8, 0x009f, 0x0096, 0x008e, 0x0085, 0x007e,
      0x0077, 0x0070, 0x006b, 0x0064, 0x005e, 0x0059, 0x0054, 0x004f, 0x004b, 0x0047, 0x0042, 0x003f,
      0x003b, 0x0038, 0x0035, 0x0032, 0x002f, 0x002c, 0x002a, 0x0027, 0x0025, 0x0023, 0x0021, 0x001f,
      0x001d, 0x001c, 0x001a, 0x0019, 0x0017, 0x0016, 0x0015, 0x0013, 0x0012, 0x0011, 0x0010, 0x000f
  };

  //Table #2 of Pro Tracker 3.4r
  const uint16_t FreqTable_ASM_34r[96] = {
    0x0d3e, 0x0c80, 0x0bcc, 0x0b22, 0x0a82, 0x09ec, 0x095c, 0x08d6, 0x0858, 0x07e0, 0x076e, 0x0704,
      0x069f, 0x0640, 0x05e6, 0x0591, 0x0541, 0x04f6, 0x04ae, 0x046b, 0x042c, 0x03f0, 0x03b7, 0x0382,
      0x034f, 0x0320, 0x02f3, 0x02c8, 0x02a1, 0x027b, 0x0257, 0x0236, 0x0216, 0x01f8, 0x01dc, 0x01c1,
      0x01a8, 0x0190, 0x0179, 0x0164, 0x0150, 0x013d, 0x012c, 0x011b, 0x010b, 0x00fc, 0x00ee, 0x00e0,
      0x00d4, 0x00c8, 0x00bd, 0x00b2, 0x00a8, 0x009f, 0x0096, 0x008d, 0x0085, 0x007e, 0x0077, 0x0070,
      0x006a, 0x0064, 0x005e, 0x0059, 0x0054, 0x0050, 0x004b, 0x0047, 0x0043, 0x003f, 0x003c, 0x0038,
      0x0035, 0x0032, 0x002f, 0x002d, 0x002a, 0x0028, 0x0026, 0x0024, 0x0022, 0x0020, 0x001e, 0x001d,
      0x001b, 0x001a, 0x0019, 0x0018, 0x0015, 0x0014, 0x0013, 0x0012, 0x0011, 0x0010, 0x000f, 0x000e
  };

  //Table #2 of Pro Tracker 3.4x - 3.5x
  const uint16_t FreqTable_ASM_34_35[96] = {
    0x0d10, 0x0c55, 0x0ba4, 0x0afc, 0x0a5f, 0x09ca, 0x093d, 0x08b8, 0x083b, 0x07c5, 0x0755, 0x06ec,
      0x0688, 0x062a, 0x05d2, 0x057e, 0x052f, 0x04e5, 0x049e, 0x045c, 0x041d, 0x03e2, 0x03ab, 0x0376,
      0x0344, 0x0315, 0x02e9, 0x02bf, 0x0298, 0x0272, 0x024f, 0x022e, 0x020f, 0x01f1, 0x01d5, 0x01bb,
      0x01a2, 0x018b, 0x0174, 0x0160, 0x014c, 0x0139, 0x0128, 0x0117, 0x0107, 0x00f9, 0x00eb, 0x00dd,
      0x00d1, 0x00c5, 0x00ba, 0x00b0, 0x00a6, 0x009d, 0x0094, 0x008c, 0x0084, 0x007c, 0x0075, 0x006f,
      0x0069, 0x0063, 0x005d, 0x0058, 0x0053, 0x004e, 0x004a, 0x0046, 0x0042, 0x003e, 0x003b, 0x0037,
      0x0034, 0x0031, 0x002f, 0x002c, 0x0029, 0x0027, 0x0025, 0x0023, 0x0021, 0x001f, 0x001d, 0x001c,
      0x001a, 0x0019, 0x0017, 0x0016, 0x0015, 0x0014, 0x0012, 0x0011, 0x0010, 0x000f, 0x000e, 0x000d
  };

  //Table #3 of Pro Tracker 3.4r
  const uint16_t FreqTable_REAL_34r[96] = {
    0x0cda, 0x0c22, 0x0b73, 0x0acf, 0x0a33, 0x09a1, 0x0917, 0x0894, 0x0819, 0x07a4, 0x0737, 0x06cf,
      0x066d, 0x0611, 0x05ba, 0x0567, 0x051a, 0x04d0, 0x048b, 0x044a, 0x040c, 0x03d2, 0x039b, 0x0367,
      0x0337, 0x0308, 0x02dd, 0x02b4, 0x028d, 0x0268, 0x0246, 0x0225, 0x0206, 0x01e9, 0x01ce, 0x01b4,
      0x019b, 0x0184, 0x016e, 0x015a, 0x0146, 0x0134, 0x0123, 0x0113, 0x0103, 0x00f5, 0x00e7, 0x00da,
      0x00ce, 0x00c2, 0x00b7, 0x00ad, 0x00a3, 0x009a, 0x0091, 0x0089, 0x0082, 0x007a, 0x0073, 0x006d,
      0x0067, 0x0061, 0x005c, 0x0056, 0x0052, 0x004d, 0x0049, 0x0045, 0x0041, 0x003d, 0x003a, 0x0036,
      0x0033, 0x0031, 0x002e, 0x002b, 0x0029, 0x0027, 0x0024, 0x0022, 0x0020, 0x001f, 0x001d, 0x001b,
      0x001a, 0x0018, 0x0017, 0x0016, 0x0014, 0x0013, 0x0012, 0x0011, 0x0010, 0x000f, 0x000e, 0x000d
  };

  //Table #3 of Pro Tracker 3.4x - 3.5x
  const uint16_t FreqTable_REAL_34_35[96] = {
    0x0cda, 0x0c22, 0x0b73, 0x0acf, 0x0a33, 0x09a1, 0x0917, 0x0894, 0x0819, 0x07a4, 0x0737, 0x06cf,
      0x066d, 0x0611, 0x05ba, 0x0567, 0x051a, 0x04d0, 0x048b, 0x044a, 0x040c, 0x03d2, 0x039b, 0x0367,
      0x0337, 0x0308, 0x02dd, 0x02b4, 0x028d, 0x0268, 0x0246, 0x0225, 0x0206, 0x01e9, 0x01ce, 0x01b4,
      0x019b, 0x0184, 0x016e, 0x015a, 0x0146, 0x0134, 0x0123, 0x0112, 0x0103, 0x00f5, 0x00e7, 0x00da,
      0x00ce, 0x00c2, 0x00b7, 0x00ad, 0x00a3, 0x009a, 0x0091, 0x0089, 0x0082, 0x007a, 0x0073, 0x006d,
      0x0067, 0x0061, 0x005c, 0x0056, 0x0052, 0x004d, 0x0049, 0x0045, 0x0041, 0x003d, 0x003a, 0x0036,
      0x0033, 0x0031, 0x002e, 0x002b, 0x0029, 0x0027, 0x0024, 0x0022, 0x0020, 0x001f, 0x001d, 0x001b,
      0x001a, 0x0018, 0x0017, 0x0016, 0x0014, 0x0013, 0x0012, 0x0011, 0x0010, 0x000f, 0x000e, 0x000d
  };

  //Volume table of Pro Tracker 3.3x - 3.4x
  const uint8_t VolumeTable_33_34[256] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02,
      0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03,
      0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04,
      0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x05, 0x05,
      0x00, 0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x03, 0x04, 0x04, 0x05, 0x05, 0x06, 0x06,
      0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x04, 0x04, 0x05, 0x05, 0x06, 0x06, 0x07, 0x07,
      0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x04, 0x05, 0x05, 0x06, 0x06, 0x07, 0x07, 0x08,
      0x00, 0x00, 0x01, 0x01, 0x02, 0x03, 0x03, 0x04, 0x05, 0x05, 0x06, 0x06, 0x07, 0x08, 0x08, 0x09,
      0x00, 0x00, 0x01, 0x02, 0x02, 0x03, 0x04, 0x04, 0x05, 0x06, 0x06, 0x07, 0x08, 0x08, 0x09, 0x0a,
      0x00, 0x00, 0x01, 0x02, 0x03, 0x03, 0x04, 0x05, 0x06, 0x06, 0x07, 0x08, 0x09, 0x09, 0x0a, 0x0b,
      0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x04, 0x05, 0x06, 0x07, 0x08, 0x08, 0x09, 0x0a, 0x0b, 0x0c,
      0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d,
      0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
  };

  //Volume table of Pro Tracker 3.5x
  const uint8_t VolumeTable_35[256] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
      0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02,
      0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03,
      0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x04, 0x04,
      0x00, 0x00, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x05, 0x05,
      0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x02, 0x03, 0x03, 0x04, 0x04, 0x04, 0x05, 0x05, 0x06, 0x06,
      0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x04, 0x04, 0x05, 0x05, 0x06, 0x06, 0x07, 0x07,
      0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x04, 0x04, 0x05, 0x05, 0x06, 0x06, 0x07, 0x07, 0x08,
      0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x04, 0x04, 0x05, 0x05, 0x06, 0x07, 0x07, 0x08, 0x08, 0x09,
      0x00, 0x01, 0x01, 0x02, 0x03, 0x03, 0x04, 0x05, 0x05, 0x06, 0x07, 0x07, 0x08, 0x09, 0x09, 0x0a,
      0x00, 0x01, 0x01, 0x02, 0x03, 0x04, 0x04, 0x05, 0x06, 0x07, 0x07, 0x08, 0x09, 0x0a, 0x0a, 0x0b,
      0x00, 0x01, 0x02, 0x02, 0x03, 0x04, 0x05, 0x06, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0a, 0x0b, 0x0c,
      0x00, 0x01, 0x02, 0x03, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0a, 0x0b, 0x0c, 0x0d,
      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
  };
}

namespace ZXTune
{
  namespace Tracking
  {
    VortexPlayer::ChannelState::ChannelState()
      : Enabled(false), Envelope(false)
      , Note(), SampleNum(0), PosInSample(0)
      , OrnamentNum(0), PosInOrnament(0)
      , Volume(15), VolSlide(0)
      , ToneSlider(), SlidingTargetNote(LIMITER), ToneAccumulator(0)
      , EnvSliding(), NoiseSliding()
      , VibrateCounter(0), VibrateOn(), VibrateOff()
    {
    }

    VortexPlayer::VortexPlayer(const String& filename)
      : Parent(filename)
      , Device(AYM::CreateChip()), Version(), FreqTable(), VolumeTable()
    {
    }

    void VortexPlayer::Initialize(std::size_t version, NoteTable table)
    {
      Version = version;
      Notetable = table;
      VolumeTable = Version <= 4 ? VolumeTable_33_34 : VolumeTable_35;
      switch (table)
      {
      case FREQ_TABLE_PT:
        FreqTable = Version <= 3 ? FreqTable_PT_33_34r : FreqTable_PT_34_35;
        break;
      case FREQ_TABLE_ST:
        FreqTable = FreqTable_ST;
        break;
      case FREQ_TABLE_ASM:
        FreqTable = Version <= 3 ? FreqTable_ASM_34r : FreqTable_ASM_34_35;
        break;
      default:
        FreqTable = Version <= 3 ? FreqTable_REAL_34r : FreqTable_REAL_34_35;
      }

      assert(VolumeTable && FreqTable);
      InitTime();
    }

    /// Retrieving current state of sound
    VortexPlayer::State VortexPlayer::GetSoundState(Sound::Analyze::ChannelsState& state) const
    {
      assert(Device.get());
      Device->GetState(state);
      return PlaybackState;
    }

    /// Rendering frame
    VortexPlayer::State VortexPlayer::RenderFrame(const Sound::Parameters& params,
      Sound::Receiver& receiver)
    {
      AYM::DataChunk chunk;
      chunk.Tick = (CurrentState.Tick += params.ClocksPerFrame());
      RenderData(chunk);

      Device->RenderData(params, chunk, receiver);

      return Parent::RenderFrame(params, receiver);
    }

    VortexPlayer::State VortexPlayer::Reset()
    {
      Device->Reset();
      return Parent::Reset();
    }

    VortexPlayer::State VortexPlayer::SetPosition(std::size_t /*frame*/)
    {
      return PlaybackState;
    }

    void VortexPlayer::Convert(const Conversion::Parameter& param, Dump& dst) const
    {
      using namespace Conversion;
      if (const VortexTextParam* const p = parameter_cast<VortexTextParam>(&param))
      {
        VortexDescr descr;
        const String::value_type DELIMITER[] = {'\n', 0};
        StringMap::const_iterator it(Information.Properties.find(Module::ATTR_TITLE));
        if (Information.Properties.end() != it)
        {
          descr.Title = it->second;
        }
        it = Information.Properties.find(Module::ATTR_AUTHOR);
        if (Information.Properties.end() != it)
        {
          descr.Author = it->second;
        }
        descr.Version = 30 + Version;
        descr.Notetable = Notetable;
        descr.Tempo = Information.Statistic.Tempo;
        descr.Loop = Information.Loop;
        descr.Order = Data.Positions;
        StringArray asArray;
        std::back_insert_iterator<StringArray> iter(asArray);
        *iter = "[Module]"; ++iter;
        iter = DescriptionToStrings(descr, iter);
        *iter = TEXT_EMPTY;//free
        for (std::size_t idx = 1; idx != Data.Ornaments.size(); ++idx)
        {
          if (Data.Ornaments[idx].Data.size())
          {
            *iter = (Formatter("[Ornament%1%]") % idx).str();
            *iter = OrnamentToString(Data.Ornaments[idx]);
            *iter = TEXT_EMPTY;//free
          }
        }
        for (std::size_t idx = 1; idx != Data.Samples.size(); ++idx)
        {
          if (Data.Samples[idx].Data.size())
          {
            *iter = (Formatter("[Sample%1%]") % idx).str();
            iter = SampleToStrings(Data.Samples[idx], ++iter);
            *iter = TEXT_EMPTY;
          }
        }
        for (std::size_t idx = 0; idx != Data.Patterns.size(); ++idx)
        {
          if (Data.Patterns[idx].size())
          {
            *iter = (Formatter("[Pattern%1%]") % idx).str();
            iter = PatternToStrings(Data.Patterns[idx], ++iter);
            *iter = TEXT_EMPTY;
          }
        }
        const String& result(boost::algorithm::join(asArray, DELIMITER));
        dst.resize(result.size() * sizeof(String::value_type));
        std::memcpy(&dst[0], &result[0], dst.size());
      }
      else
      {
        Parent::Convert(param, dst);
      }
    }

    void VortexPlayer::RenderData(AYM::DataChunk& chunk)
    {
      const Line& line(Data.Patterns[CurrentState.Position.Pattern][CurrentState.Position.Note]);
      if (0 == CurrentState.Position.Frame)//begin note
      {
        if (0 == CurrentState.Position.Note)//pattern begin
        {
          Commons.NoiseBase = 0;
        }
        for (std::size_t chan = 0; chan != line.Channels.size(); ++chan)
        {
          const Line::Chan& src(line.Channels[chan]);
          ChannelState& dst(Channels[chan]);
          if (src.Enabled)
          {
            dst.PosInSample = dst.PosInOrnament = 0;
            dst.VolSlide = dst.EnvSliding = dst.NoiseSliding = 0;
            dst.ToneSlider.Reset();
            dst.ToneAccumulator = 0;
            dst.VibrateCounter = 0;
            dst.Enabled = *src.Enabled;
          }
          if (src.Note)
          {
            dst.Note = *src.Note;
          }
          if (src.SampleNum)
          {
            dst.SampleNum = *src.SampleNum;
          }
          if (src.OrnamentNum)
          {
            dst.OrnamentNum = *src.OrnamentNum;
            dst.PosInOrnament = 0;
          }
          if (src.Volume)
          {
            dst.Volume = *src.Volume;
          }
          for (CommandsArray::const_iterator it = src.Commands.begin(), lim = src.Commands.end(); it != lim; ++it)
          {
            switch (it->Type)
            {
            case GLISS:
              dst.ToneSlider.Period = dst.ToneSlider.Counter = it->Param1;
              dst.ToneSlider.Delta = it->Param2;
              dst.SlidingTargetNote = LIMITER;
              dst.VibrateCounter = 0;
              if (0 == dst.ToneSlider.Counter && Version >= 7)
              {
                ++dst.ToneSlider.Counter;
              }
              break;
            case GLISS_NOTE:
              dst.ToneSlider.Period = dst.ToneSlider.Counter = it->Param1;
              dst.ToneSlider.Delta = it->Param2;
              dst.SlidingTargetNote = it->Param3;
              dst.VibrateCounter = 0;
              //tone up                                     freq up
              if (bool(dst.Note < dst.SlidingTargetNote) != bool(dst.ToneSlider.Delta < 0))
              {
                dst.ToneSlider.Delta = -dst.ToneSlider.Delta;
              }
              break;
            case SAMPLEOFFSET:
              dst.PosInSample = it->Param1;
              break;
            case ORNAMENTOFFSET:
              dst.PosInOrnament = it->Param1;
              break;
            case VIBRATE:
              dst.VibrateCounter = dst.VibrateOn = it->Param1;
              dst.VibrateOff = it->Param2;
              dst.ToneSlider.Value = 0;
              dst.ToneSlider.Counter = 0;
              break;
            case SLIDEENV:
              Commons.EnvSlider.Period = Commons.EnvSlider.Counter = it->Param1;
              Commons.EnvSlider.Delta = it->Param2;
              break;
            case ENVELOPE:
              chunk.Data[AYM::DataChunk::REG_ENV] = uint8_t(it->Param1);
              Commons.EnvBase = it->Param2;
              chunk.Mask |= (1 << AYM::DataChunk::REG_ENV);
              dst.Envelope = true;
              Commons.EnvSlider.Reset();
              break;
            case NOENVELOPE:
              dst.Envelope = false;
              break;
            case NOISEBASE:
              Commons.NoiseBase = it->Param1;
              break;
            case TEMPO:
              //ignore
              break;
            default:
              assert(!"Invalid command");
            }
          }
        }
      }
      //permanent registers
      chunk.Data[AYM::DataChunk::REG_MIXER] = 0;
      chunk.Mask |= (1 << AYM::DataChunk::REG_MIXER) |
        (1 << AYM::DataChunk::REG_VOLA) | (1 << AYM::DataChunk::REG_VOLB) | (1 << AYM::DataChunk::REG_VOLC);
      std::size_t toneReg = AYM::DataChunk::REG_TONEA_L;
      std::size_t volReg = AYM::DataChunk::REG_VOLA;
      uint8_t toneMsk = AYM::DataChunk::MASK_TONEA;
      uint8_t noiseMsk = AYM::DataChunk::MASK_NOISEA;
      signed envelopeAddon(0);
      for (ChannelState* dst = Channels; dst != ArrayEnd(Channels);
        ++dst, toneReg += 2, ++volReg, toneMsk <<= 1, noiseMsk <<= 1)
      {
        if (dst->Enabled)
        {
          const Sample& curSample(Data.Samples[dst->SampleNum]);
          const Sample::Line& curSampleLine(curSample.Data[dst->PosInSample]);
          const Ornament& curOrnament(Data.Ornaments[dst->OrnamentNum]);

          assert(!curOrnament.Data.empty());
          //calculate tone
          const signed toneAddon(curSampleLine.ToneOffset + dst->ToneAccumulator);
          if (curSampleLine.KeepToneOffset)
          {
            dst->ToneAccumulator = toneAddon;
          }
          const std::size_t halfTone(clamp<std::size_t>(dst->Note + curOrnament.Data[dst->PosInOrnament], 0, 95));
          const uint16_t tone((FreqTable[halfTone] + dst->ToneSlider.Value + toneAddon) & 0xfff);
          if (dst->ToneSlider.Update() && LIMITER != dst->SlidingTargetNote)
          {
            const uint16_t targetTone(FreqTable[dst->SlidingTargetNote]);
            if ((dst->ToneSlider.Delta > 0 && tone + dst->ToneSlider.Delta > targetTone) ||
              (dst->ToneSlider.Delta < 0 && tone + dst->ToneSlider.Delta < targetTone))
            {
              //slided to target note
              dst->Note = dst->SlidingTargetNote;
              dst->SlidingTargetNote = LIMITER;
              dst->ToneSlider.Value = 0;
              dst->ToneSlider.Counter = 0;
            }
          }
          chunk.Data[toneReg] = uint8_t(tone & 0xff);
          chunk.Data[toneReg + 1] = uint8_t(tone >> 8);
          chunk.Mask |= 3 << toneReg;
          dst->VolSlide = clamp(dst->VolSlide + curSampleLine.VolSlideAddon, -15, 15);
          //calculate level
          chunk.Data[volReg] = GetVolume(dst->Volume, clamp<signed>(dst->VolSlide + curSampleLine.Level, 0, 15))
            | uint8_t(dst->Envelope && !curSampleLine.EnvMask ? AYM::DataChunk::MASK_ENV : 0);
          //mixer
          if (curSampleLine.ToneMask)
          {
            chunk.Data[AYM::DataChunk::REG_MIXER] |= toneMsk;
          }
          if (curSampleLine.NoiseMask)
          {
            chunk.Data[AYM::DataChunk::REG_MIXER] |= noiseMsk;
            if (curSampleLine.NEOffset)
            {
              dst->EnvSliding = curSampleLine.NEOffset;
            }
            envelopeAddon += curSampleLine.NEOffset;
          }
          else
          {
            Commons.NoiseAddon = curSampleLine.NEOffset + dst->NoiseSliding;
            if (curSampleLine.KeepNEOffset)
            {
              dst->NoiseSliding = Commons.NoiseAddon;
            }
          }

          if (++dst->PosInSample >= curSample.Data.size())
          {
            dst->PosInSample = curSample.Loop;
          }
          if (++dst->PosInOrnament >= curOrnament.Data.size())
          {
            dst->PosInOrnament = curOrnament.Loop;
          }
        }
        else
        {
          chunk.Data[volReg] = 0;
          //????
          chunk.Data[AYM::DataChunk::REG_MIXER] |= toneMsk | noiseMsk;
        }
        if (dst->VibrateCounter > 0 && !--dst->VibrateCounter)
        {
          dst->Enabled = !dst->Enabled;
          dst->VibrateCounter = dst->Enabled ? dst->VibrateOn : dst->VibrateOff;
        }
      }
      const signed envPeriod(envelopeAddon + Commons.EnvSlider.Value + signed(Commons.EnvBase));
      chunk.Data[AYM::DataChunk::REG_TONEN] = uint8_t(Commons.NoiseBase + Commons.NoiseAddon) & 0x1f;
      chunk.Data[AYM::DataChunk::REG_TONEE_L] = uint8_t(envPeriod & 0xff);
      chunk.Data[AYM::DataChunk::REG_TONEE_H] = uint8_t(envPeriod >> 8);
      chunk.Mask |= (1 << AYM::DataChunk::REG_TONEN) |
        (1 << AYM::DataChunk::REG_TONEE_L) | (1 << AYM::DataChunk::REG_TONEE_H);
      Commons.EnvSlider.Update();
      CurrentState.Position.Channels = std::count_if(Channels, ArrayEnd(Channels),
        boost::mem_fn(&ChannelState::Enabled));
    }

    uint8_t VortexPlayer::GetVolume(std::size_t volume, std::size_t level)
    {
      assert(volume <= 15);
      assert(level <= 15);
      return VolumeTable[volume * 16 + level];
    }
  }
}
