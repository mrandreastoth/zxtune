/*
Abstract:
  AY/YM sound rendering chips interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef DEVICES_AYM_CHIP_H_DEFINED
#define DEVICES_AYM_CHIP_H_DEFINED

//library includes
#include <devices/aym.h>
#include <devices/state.h>
#include <sound/mixer.h>
#include <sound/receiver.h>

//supporting for AY/YM-based modules
namespace Devices
{
  namespace AYM
  {
    // Describes real device
    class Chip : public Device, public StateSource
    {
    public:
      typedef boost::shared_ptr<Chip> Ptr;
    };

    enum ChannelMasks
    {
      CHANNEL_MASK_A = 1,
      CHANNEL_MASK_B = 2,
      CHANNEL_MASK_C = 4,
      CHANNEL_MASK_N = 8,
      CHANNEL_MASK_E = 16,
    };

    enum LayoutType
    {
      LAYOUT_ABC = 0,
      LAYOUT_ACB = 1,
      LAYOUT_BAC = 2,
      LAYOUT_BCA = 3,
      LAYOUT_CAB = 4,
      LAYOUT_CBA = 5,
      LAYOUT_MONO = 6,

      LAYOUT_LAST
    };

    enum InterpolationType
    {
      INTERPOLATION_NONE = 0,
      INTERPOLATION_LQ = 1,
      INTERPOLATION_HQ = 2
    };

    enum ChipType
    {
      TYPE_AY38910 = 0,
      TYPE_YM2149F = 1
    };

    class ChipParameters
    {
    public:
      typedef boost::shared_ptr<const ChipParameters> Ptr;

      virtual ~ChipParameters() {}

      virtual uint_t Version() const = 0;

      virtual uint64_t ClockFreq() const = 0;
      virtual uint_t SoundFreq() const = 0;
      virtual ChipType Type() const = 0;
      virtual InterpolationType Interpolation() const = 0;
      virtual uint_t DutyCycleValue() const = 0;
      virtual uint_t DutyCycleMask() const = 0;
      virtual LayoutType Layout() const = 0;
    };

    /// Virtual constructors
    Chip::Ptr CreateChip(ChipParameters::Ptr params, Sound::ThreeChannelsMixer::Ptr mixer, Sound::Receiver::Ptr target);
  }
}

#endif //DEVICES_AYM_CHIP_H_DEFINED
