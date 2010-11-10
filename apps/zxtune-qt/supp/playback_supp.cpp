/*
Abstract:
  Playback support implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "playback_supp.h"
#include "supp/playitems_provider.h"
#include "ui/utils.h"
//common includes
#include <error.h>
#include <tools.h>
//library includes
#include <sound/backend.h>
//qt includes
#include <QtCore/QMutex>

namespace
{
  class PlaybackSupportImpl : public PlaybackSupport
  {
  public:
    explicit PlaybackSupportImpl(QObject& parent)
      : PlaybackSupport(parent)
    {
    }

    virtual ~PlaybackSupportImpl()
    {
      this->wait();
    }

    virtual void SetItem(const Playitem& item)
    {
      OpenBackend();
      Backend->SetModule(item.GetModule());
      this->wait();
      Player = Backend->GetPlayer();
      Backend->Play();
      this->start();
    }

    virtual void Play()
    {
      //play only if any module selected or set
      if (0 != Player.use_count())
      {
        Backend->Play();
        this->start();
      }
    }

    virtual void Stop()
    {
      if (Backend.get())
      {
        Backend->Stop();
        this->wait();
      }
    }

    virtual void Pause()
    {
      const ZXTune::Sound::Backend::State curState = Backend.get() ? Backend->GetCurrentState() : ZXTune::Sound::Backend::NOTOPENED;
      if (ZXTune::Sound::Backend::STARTED == curState)
      {
        Backend->Pause();
      }
      else if (ZXTune::Sound::Backend::PAUSED == curState)
      {
        Backend->Play();
      }
    }

    virtual void Seek(int frame)
    {
      if (Backend.get())
      {
        Backend->SetPosition(frame);
      }
    }

    virtual void run()
    {
      using namespace ZXTune;

      //notify about start
      OnStartModule(Player);

      SignalsCollector::Ptr signaller = Backend->CreateSignalsCollector(
        Sound::Backend::MODULE_RESUME | Sound::Backend::MODULE_PAUSE |
        Sound::Backend::MODULE_STOP | Sound::Backend::MODULE_FINISH);
      for (;;)
      {
        uint_t sigmask = 0;
        if (signaller->WaitForSignals(sigmask, 100/*10fps*/))
        {
          if (sigmask & (Sound::Backend::MODULE_STOP | Sound::Backend::MODULE_FINISH))
          {
            if (sigmask & Sound::Backend::MODULE_FINISH)
            {
              OnFinishModule();
            }
            break;
          }
          else if (sigmask & Sound::Backend::MODULE_RESUME)
          {
            OnResumeModule();
          }
          else if (sigmask & Sound::Backend::MODULE_PAUSE)
          {
            OnPauseModule();
          }
          else
          {
            assert(!"Invalid signal");
          }
        }
        const Sound::Backend::State curState = Backend->GetCurrentState();
        if (Sound::Backend::STARTED != curState)
        {
          continue;
        }
        OnUpdateState();
      }
      //notify about stop
      OnStopModule();
    }
  private:
    void OpenBackend()
    {
      if (Backend.get())
      {
        return;
      }
      using namespace ZXTune;
      //create backend
      {
        const Parameters::Accessor::Ptr soundParams = Parameters::Container::Create();
        for (Sound::BackendCreator::Iterator::Ptr backends = Sound::EnumerateBackends();
          backends->IsValid(); backends->Next())
        {
          if (!backends->Get()->CreateBackend(soundParams, Backend))
          {
            break;
          }
        }
        assert(Backend.get());
        static const Sound::MultiGain MIXER3[] =
        {
          { {1.0, 0.0} },
          { {0.5, 0.5} },
          { {0.0, 1.0} }
        };
        static const Sound::MultiGain MIXER4[] =
        {
          { {1.0, 0.0} },
          { {0.7, 0.3} },
          { {0.3, 0.7} },
          { {0.0, 1.0} }
        };
        Backend->SetMixer(std::vector<Sound::MultiGain>(MIXER3, ArrayEnd(MIXER3)));
        Backend->SetMixer(std::vector<Sound::MultiGain>(MIXER4, ArrayEnd(MIXER4)));
      }
      OnSetBackend(*Backend);
    }
  private:
    ZXTune::Sound::Backend::Ptr Backend;
    ZXTune::Module::Player::ConstPtr Player;
  };
}

PlaybackSupport::PlaybackSupport(QObject& parent) : QThread(&parent)
{
}

PlaybackSupport* PlaybackSupport::Create(QObject& parent)
{
  REGISTER_METATYPE(ZXTune::Module::Player::ConstPtr);
  return new PlaybackSupportImpl(parent);
}
