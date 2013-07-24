/*
Abstract:
  TFM chip adapter

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef DEVICES_TFM_DEFINED
#define DEVICES_TFM_DEFINED

//library includes
#include <devices/fm.h>

namespace Devices
{
  namespace TFM
  {
    const uint_t CHIPS = 2;
    const uint_t VOICES = Devices::FM::VOICES * CHIPS;

    using Devices::FM::Stamp;

    struct DataChunk
    {
      DataChunk() : TimeStamp()
      {
      }

      Stamp TimeStamp;
      boost::array<Devices::FM::DataChunk::Registers, CHIPS> Data;
    };

    class Device
    {
    public:
      typedef boost::shared_ptr<Device> Ptr;
      virtual ~Device() {}

      virtual void RenderData(const DataChunk& src) = 0;
      virtual void Flush() = 0;
      virtual void Reset() = 0;
    };

    class Chip : public Device, public StateSource
    {
    public:
      typedef boost::shared_ptr<Chip> Ptr;
    };

    using Devices::FM::ChipParameters;
    Chip::Ptr CreateChip(ChipParameters::Ptr params, Sound::Receiver::Ptr target);
  }
}

#endif //DEVICES_TFM_DEFINED
