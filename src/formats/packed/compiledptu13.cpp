/*
Abstract:
  ProTrackerUtility 1.3 compiled modules support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//library includes
#include "container.h"
#include <formats/chiptune_decoders.h>
#include <formats/chiptune/metainfo.h>
#include <formats/chiptune/protracker3_detail.h>
//common includes
#include <byteorder.h>
#include <logging.h>
#include <tools.h>
//library includes
#include <binary/typed_container.h>
//boost includes
#include <boost/array.hpp>
//text includes
#include <formats/text/chiptune.h>
#include <formats/text/packed.h>

namespace
{
  const std::string THIS_MODULE("Formats::Packed::CompiledPTU13");
}

namespace CompiledPTU13
{
  const std::size_t MAX_MODULE_SIZE = 0xb900;
  const std::size_t PLAYER_SIZE = 0x900;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct Player
  {
    uint8_t Padding1[0x1b];
    uint16_t PositionsAddr;
  } PACK_POST;

  PACK_PRE struct RawHeader
  {
    uint8_t Id[13];        //'ProTracker 3.'
    uint8_t Subversion;
    uint8_t Optional1[16]; //' compilation of '
    uint8_t Metainfo[68];
    uint8_t Mode;
    uint8_t FreqTableNum;
    uint8_t Tempo;
    uint8_t Length;
    uint8_t Loop;
    uint16_t PatternsOffset;
    boost::array<uint16_t, Formats::Chiptune::ProTracker3::MAX_SAMPLES_COUNT> SamplesOffsets;
    boost::array<uint16_t, Formats::Chiptune::ProTracker3::MAX_ORNAMENTS_COUNT> OrnamentsOffsets;
    uint8_t Positions[1];//finished by marker
  } PACK_POST;

  const uint8_t POS_END_MARKER = 0xff;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(RawHeader) == 202);

  const String DESCRIPTION = String(Text::PROTRACKERUTILITY13_DECODER_DESCRIPTION) + Text::PLAYER_SUFFIX;

  const std::string FORMAT(
    "21??"  //ld hl,xxxx +0x665
    "35"    //dec (hl)
    "c2??"  //jp nz,xxxx
    "23"    //inc hl
    "35"    //dec (hl)
    "2045"  //jr nz,xx
    "11??"  //ld de,xxxx +0x710
    "1a"    //ld a,(de)
    "b7"    //or a
    "2029"  //jr nz,xx
    "32??"  //ld (xxxx),a
    "57"    //ld d,a
    "ed73??"//ld (xxxx),sp
    "21??"  //ld hl,xxxx //positions offset
    "7e"    //ld a,(hl)
    "3c"    //inc a
    "2003"  //jr nz,xxxx
    "21??"  //ld hl,xxxx
    "5e"    //ld e,(hl)
    "23"    //inc hl
    "22??"  //ld (xxxx),hl
    "21??"  //ld hl,xxxx
    "19"    //add hl,de
    "19"    //add hl,de
    "f9"    //ld sp,hl
    "d1"    //pop de
    "e1"    //pop hl
    "22??"  //ld (xxxx),hl
  );

  uint_t GetPatternsCount(const RawHeader& hdr, std::size_t maxSize)
  {
    const uint8_t* const dataBegin = &hdr.Tempo;
    const uint8_t* const dataEnd = dataBegin + maxSize;
    const uint8_t* const lastPosition = std::find(hdr.Positions, dataEnd, POS_END_MARKER);
    if (lastPosition != dataEnd && 
        lastPosition == std::find_if(hdr.Positions, lastPosition, std::bind2nd(std::modulus<std::size_t>(), 3)))
    {
      return 1 + *std::max_element(hdr.Positions, lastPosition) / 3;
    }
    return 0;
  }

}//CompiledPTU13

namespace Formats
{
  namespace Packed
  {
    class CompiledPTU13Decoder : public Decoder
    {
    public:
      CompiledPTU13Decoder()
        : Player(Binary::Format::Create(CompiledPTU13::FORMAT))
        , Decoder(Formats::Chiptune::CreateProTracker3Decoder())
      {
      }

      virtual String GetDescription() const
      {
        return CompiledPTU13::DESCRIPTION;
      }

      virtual Binary::Format::Ptr GetFormat() const
      {
        return Player;
      }

      virtual Container::Ptr Decode(const Binary::Container& rawData) const
      {
        const uint8_t* const data = safe_ptr_cast<const uint8_t*>(rawData.Data());
        const std::size_t availSize = rawData.Size();
        const std::size_t playerSize = CompiledPTU13::PLAYER_SIZE;
        if (!Player->Match(data, availSize) || availSize < CompiledPTU13::PLAYER_SIZE + sizeof(CompiledPTU13::RawHeader))
        {
          return Container::Ptr();
        }
        const CompiledPTU13::Player& rawPlayer = *safe_ptr_cast<const CompiledPTU13::Player*>(data);
        const uint_t positionsAddr = fromLE(rawPlayer.PositionsAddr);
        if (positionsAddr < playerSize + offsetof(CompiledPTU13::RawHeader, Positions))
        {
          Log::Debug(THIS_MODULE, "Invalid compile addr");
          return Container::Ptr();
        }
        const uint_t dataAddr = positionsAddr - offsetof(CompiledPTU13::RawHeader, Positions);
        const CompiledPTU13::RawHeader& rawHeader = *safe_ptr_cast<const CompiledPTU13::RawHeader*>(data + playerSize);
        const uint_t patternsCount = CompiledPTU13::GetPatternsCount(rawHeader, availSize - playerSize);
        if (!patternsCount)
        {
          Log::Debug(THIS_MODULE, "Invalid patterns count");
          return Container::Ptr();
        }
        const uint_t compileAddr = dataAddr - playerSize;
        Log::Debug(THIS_MODULE, "Detected player compiled at %1% (#%1$04x) with %2% patterns", compileAddr, patternsCount);
        const std::size_t modDataSize = std::min(CompiledPTU13::MAX_MODULE_SIZE, availSize - playerSize);
        const Binary::Container::Ptr modData = rawData.GetSubcontainer(playerSize, modDataSize);
        const Formats::Chiptune::PatchedDataBuilder::Ptr builder = Formats::Chiptune::PatchedDataBuilder::Create(*modData);
        //fix patterns/samples/ornaments offsets
        for (uint_t idx = offsetof(CompiledPTU13::RawHeader, PatternsOffset); idx != offsetof(CompiledPTU13::RawHeader, Positions); idx += 2)
        {
          builder->FixLEWord(idx, -dataAddr);
        }
        //fix patterns offsets
        for (uint_t idx = fromLE(rawHeader.PatternsOffset) - dataAddr, lim = idx + 6 * patternsCount; idx != lim; idx += 2)
        {
          builder->FixLEWord(idx, -dataAddr);
        }
        const Binary::Container::Ptr fixedModule = builder->GetResult();
        if (Formats::Chiptune::Container::Ptr fixedParsed = Decoder->Decode(*fixedModule))
        {
          return CreatePackedContainer(fixedParsed, playerSize + fixedParsed->Size());
        }
        Log::Debug(THIS_MODULE, "Failed to parse fixed module");
        return Container::Ptr();
      }
    private:
      const Binary::Format::Ptr Player;
      const Formats::Chiptune::Decoder::Ptr Decoder;
    };

    Decoder::Ptr CreateCompiledPTU13Decoder()
    {
      return boost::make_shared<CompiledPTU13Decoder>();
    }
  }//namespace Packed
}//namespace Formats