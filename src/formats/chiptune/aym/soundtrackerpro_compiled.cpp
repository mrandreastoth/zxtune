/**
* 
* @file
*
* @brief  SoundTrackerPro compiled modules support implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "soundtrackerpro_detail.h"
#include "formats/chiptune/container.h"
#include "formats/chiptune/metainfo.h"
//common includes
#include <byteorder.h>
#include <contract.h>
#include <iterator.h>
#include <make_ptr.h>
#include <range_checker.h>
//library includes
#include <binary/format_factories.h>
#include <binary/typed_container.h>
#include <debug/log.h>
#include <math/numeric.h>
#include <strings/optimize.h>
//std includes
#include <array>
#include <cstring>
//text includes
#include <formats/text/chiptune.h>

namespace Formats
{
namespace Chiptune
{
  namespace SoundTrackerProCompiled
  {
    const Debug::Stream Dbg("Formats::Chiptune::SoundTrackerProCompiled");

    using namespace SoundTrackerPro;

    //size and offsets are taken from ~3400 modules analyzing
    const std::size_t MIN_SIZE = 200;
    const std::size_t MAX_SIZE = 0x2800;

    /*
      Typical module structure. Order is not changed according to different tests

      Header
      Id (optional)
      PatternsData
      Ornaments
      Samples
      Positions
      <possible gap due to invalid length>
      PatternsOffsets
      OrnamentsOffsets
      SamplesOffsets (may be trunkated)
    */

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
    PACK_PRE struct RawHeader
    {
      uint8_t Tempo;
      uint16_t PositionsOffset;
      uint16_t PatternsOffset;
      uint16_t OrnamentsOffset;
      uint16_t SamplesOffset;
      uint8_t FixesCount;
    } PACK_POST;

    const uint8_t ID[] =
    {
      'K', 'S', 'A', ' ', 'S', 'O', 'F', 'T', 'W', 'A', 'R', 'E', ' ',
      'C', 'O', 'M', 'P', 'I', 'L', 'A', 'T', 'I', 'O', 'N', ' ', 'O', 'F', ' '
    };

    PACK_PRE struct RawId
    {
      uint8_t Id[sizeof(ID)];
      std::array<char, 25> Title;

      bool Check() const
      {
        return 0 == std::memcmp(Id, ID, sizeof(Id));
      }
    } PACK_POST;

    PACK_PRE struct RawPositions
    {
      uint8_t Length;
      uint8_t Loop;
      PACK_PRE struct PosEntry
      {
        uint8_t PatternOffset;//*6
        int8_t Transposition;
      } PACK_POST;
      PosEntry Data[1];
    } PACK_POST;

    PACK_PRE struct RawPattern
    {
      std::array<uint16_t, 3> Offsets;
    } PACK_POST;

    PACK_PRE struct RawObject
    {
      int8_t Loop;
      int8_t Size;

      uint_t GetSize() const
      {
        return Size < 0
          ? 0
          : Size;
      }
      
      uint_t GetLoop() const
      {
        return Loop < 0
          ? GetSize()
          : Loop;
      }
    } PACK_POST;

    PACK_PRE struct RawOrnament : RawObject
    {
      typedef int8_t Line;

      Line GetLine(uint_t idx) const
      {
        const int8_t* const src = safe_ptr_cast<const int8_t*>(this + 1);
        uint8_t offset = static_cast<uint8_t>(idx * sizeof(Line));
        return src[offset];
      }

      std::size_t GetUsedSize() const
      {
        return sizeof(RawObject) + std::min<std::size_t>(GetSize() * sizeof(Line), 256);
      }
    } PACK_POST;

    PACK_PRE struct RawOrnaments
    {
      std::array<uint16_t, MAX_ORNAMENTS_COUNT> Offsets;
    } PACK_POST;

    PACK_PRE struct RawSample : RawObject
    {
      PACK_PRE struct Line
      {
        // NxxTaaaa
        // xxnnnnnE
        // vvvvvvvv
        // vvvvvvvv

        // a - level
        // T - tone mask
        // N - noise mask
        // E - env mask
        // n - noise
        // v - signed vibrato
        uint8_t LevelAndFlags;
        uint8_t NoiseAndFlag;
        int16_t Vibrato;

        uint_t GetLevel() const
        {
          return LevelAndFlags & 15;
        }

        bool GetToneMask() const
        {
          return 0 != (LevelAndFlags & 16);
        }

        bool GetNoiseMask() const
        {
          return 0 != (LevelAndFlags & 128);
        }

        bool GetEnvelopeMask() const
        {
          return 0 != (NoiseAndFlag & 1);
        }

        uint_t GetNoise() const
        {
          return (NoiseAndFlag & 62) >> 1;
        }

        int_t GetVibrato() const
        {
          return fromLE(Vibrato);
        }
      } PACK_POST;

      Line GetLine(uint_t idx) const
      {
        static_assert(0 == (sizeof(Line) & (sizeof(Line) - 1)), "Invalid layout");
        const uint_t maxLines = 256 / sizeof(Line);
        const Line* const src = safe_ptr_cast<const Line*>(this + 1);
        return src[idx % maxLines];
      }

      std::size_t GetUsedSize() const
      {
        return sizeof(RawObject) + std::min<std::size_t>(GetSize() * sizeof(Line), 256);
      }
    } PACK_POST;

    PACK_PRE struct RawSamples
    {
      std::array<uint16_t, MAX_SAMPLES_COUNT> Offsets;
    } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

    static_assert(sizeof(RawHeader) == 10, "Invalid layout");
    static_assert(sizeof(RawId) == 53, "Invalid layout");
    static_assert(sizeof(RawPositions) == 4, "Invalid layout");
    static_assert(sizeof(RawPattern) == 6, "Invalid layout");
    static_assert(sizeof(RawOrnament) == 2, "Invalid layout");
    static_assert(sizeof(RawOrnaments) == 32, "Invalid layout");
    static_assert(sizeof(RawSample) == 2, "Invalid layout");
    static_assert(sizeof(RawSamples) == 30, "Invalid layout");

    class StubBuilder : public Builder
    {
    public:
      MetaBuilder& GetMetaBuilder() override
      {
        return GetStubMetaBuilder();
      }
      void SetInitialTempo(uint_t /*tempo*/) override {}
      void SetSample(uint_t /*index*/, Sample /*sample*/) override {}
      void SetOrnament(uint_t /*index*/, Ornament /*ornament*/) override {}
      void SetPositions(Positions /*positions*/) override {}
      PatternBuilder& StartPattern(uint_t /*index*/) override
      {
        return GetStubPatternBuilder();
      }
      void StartChannel(uint_t /*index*/) override {}
      void SetRest() override {}
      void SetNote(uint_t /*note*/) override {}
      void SetSample(uint_t /*sample*/) override {}
      void SetOrnament(uint_t /*ornament*/) override {}
      void SetEnvelope(uint_t /*type*/, uint_t /*value*/) override {}
      void SetNoEnvelope() override {}
      void SetGliss(uint_t /*target*/) override {}
      void SetVolume(uint_t /*vol*/) override {}
    };

    uint_t GetUnfixDelta(const RawHeader& hdr, const RawId& id, const RawPattern& firstPattern)
    {
      //first pattern is always placed after the header (and optional id);
      const std::size_t hdrSize = sizeof(hdr) + (id.Check() ? sizeof(id) : 0);
      const std::size_t firstData = fromLE(firstPattern.Offsets[0]);
      if (0 != hdr.FixesCount)
      {
        Require(firstData == hdrSize);
      }
      else
      {
        Require(firstData >= hdrSize);
      }
      return static_cast<uint_t>(firstData - hdrSize);
    }

    std::size_t GetMaxSize(const RawHeader& hdr)
    {
      return fromLE(hdr.SamplesOffset) + sizeof(RawSamples);
    }

    class RangesMap
    {
    public:
      explicit RangesMap(std::size_t limit)
        : ServiceRanges(RangeChecker::CreateShared(limit))
        , TotalRanges(RangeChecker::CreateSimple(limit))
        , FixedRanges(RangeChecker::CreateSimple(limit))
      {
      }

      void AddService(std::size_t offset, std::size_t size) const
      {
        Require(ServiceRanges->AddRange(offset, size));
        Add(offset, size);
      }

      void AddFixed(std::size_t offset, std::size_t size) const
      {
        Require(FixedRanges->AddRange(offset, size));
        Add(offset, size);
      }

      void Add(std::size_t offset, std::size_t size) const
      {
        Dbg(" Affected range %1%..%2%", offset, offset + size);
        Require(TotalRanges->AddRange(offset, size));
      }

      std::size_t GetSize() const
      {
        return TotalRanges->GetAffectedRange().second;
      }

      RangeChecker::Range GetFixedArea() const
      {
        return FixedRanges->GetAffectedRange();
      }
    private:
      const RangeChecker::Ptr ServiceRanges;
      const RangeChecker::Ptr TotalRanges;
      const RangeChecker::Ptr FixedRanges;
    };

    class Format
    {
    public:
      explicit Format(Binary::TypedContainer data)
        : Delegate(std::move(data))
        , Ranges(Delegate.GetSize())
        , Source(GetServiceObject<RawHeader>(0, 0))
        , Id(GetObject<RawId>(sizeof(Source)))
        , UnfixDelta(GetUnfixDelta(Source, Id, GetPattern(0)))
      {
        if (Id.Check())
        {
          Ranges.AddService(sizeof(Source), sizeof(Id));
        }
        if (UnfixDelta)
        {
          Dbg("Unfix delta is %1%", UnfixDelta);
        }
      }

      void ParseCommonProperties(Builder& builder) const
      {
        builder.SetInitialTempo(Source.Tempo);
        MetaBuilder& meta = builder.GetMetaBuilder();
        meta.SetProgram(Text::SOUNDTRACKERPRO_DECODER_DESCRIPTION);
        if (Id.Check())
        {
          meta.SetTitle(Strings::OptimizeAscii(Id.Title));
        }
      }

      void ParsePositions(Builder& builder) const
      {
        Positions positions;
        for (RangeIterator<const RawPositions::PosEntry*> iter = GetPositions(); iter; ++iter)
        {
          const RawPositions::PosEntry& src = *iter;
          Require(0 == src.PatternOffset % 6);
          const uint_t patNum = src.PatternOffset / 6;
          PositionEntry dst;
          dst.PatternIndex = patNum;
          dst.Transposition = src.Transposition;
          positions.Lines.push_back(dst);
        }
        positions.Loop = PeekByte(fromLE(Source.PositionsOffset) + offsetof(RawPositions, Loop));
        Dbg("Positions: %1% entries, loop to %2%", positions.GetSize(), positions.GetLoop());
        builder.SetPositions(std::move(positions));
      }

      void ParsePatterns(const Indices& pats, Builder& builder) const
      {
        Dbg("Patterns: %1% to parse", pats.Count());
        bool hasValidPatterns = false;
        const uint_t minPatternsOffset = sizeof(Source) + (Id.Check() ? sizeof(Id) : 0);
        for (Indices::Iterator it = pats.Items(); it; ++it)
        {
          const uint_t patIndex = *it;
          Dbg("Parse pattern %1%", patIndex);
          if (ParsePattern(patIndex, minPatternsOffset, builder))
          {
            hasValidPatterns = true;
          }
        }
        Require(hasValidPatterns);
      }

      void ParseSamples(const Indices& samples, Builder& builder) const
      {
        Dbg("Samples: %1% to parse", samples.Count());
        for (Indices::Iterator it = samples.Items(); it; ++it)
        {
          const uint_t samIdx = *it;
          Dbg("Parse sample %1%", samIdx);
          const RawSample& src = GetSample(samIdx);
          builder.SetSample(samIdx, ParseSample(src));
        }
        //mark possible samples offsets as used
        const std::size_t samplesOffsets = fromLE(Source.SamplesOffset);
        Ranges.Add(samplesOffsets, std::min(sizeof(RawSamples), Delegate.GetSize() - samplesOffsets));
      }

      void ParseOrnaments(const Indices& ornaments, Builder& builder) const
      {
        Dbg("Ornaments: %1% to parse", ornaments.Count());
        for (Indices::Iterator it = ornaments.Items(); it; ++it)
        {
          const uint_t ornIdx = *it;
          Dbg("Parse ornament %1%", ornIdx);
          const RawOrnament& src = GetOrnament(ornIdx);
          builder.SetOrnament(ornIdx, ParseOrnament(src));
        }
      }

      std::size_t GetSize() const
      {
        const std::size_t result = Ranges.GetSize();
        Require(result + UnfixDelta <= 0x10000);
        return result;
      }

      RangeChecker::Range GetFixedArea() const
      {
        return Ranges.GetFixedArea();
      }
    private:
      RangeIterator<const RawPositions::PosEntry*> GetPositions() const
      {
        const std::size_t offset = fromLE(Source.PositionsOffset);
        const RawPositions* const positions = Delegate.GetField<RawPositions>(offset);
        Require(positions != nullptr);
        const uint_t length = positions->Length;
        Require(length != 0);
        Ranges.AddService(offset, sizeof(*positions) + (length - 1) * sizeof(RawPositions::PosEntry));
        const RawPositions::PosEntry* const firstEntry = positions->Data;
        const RawPositions::PosEntry* const lastEntry = firstEntry + length;
        return RangeIterator<const RawPositions::PosEntry*>(firstEntry, lastEntry);
      }

      const RawPattern& GetPattern(uint_t index) const
      {
        return GetServiceObject<RawPattern>(index, fromLE(Source.PatternsOffset));
      }

      const RawSample& GetSample(uint_t index) const
      {
        const uint16_t offset = fromLE(GetServiceObject<uint16_t>(index, fromLE(Source.SamplesOffset))) - UnfixDelta;
        const RawObject* const obj = Delegate.GetField<RawObject>(offset);
        Require(obj != nullptr);
        const RawSample* const res = safe_ptr_cast<const RawSample*>(obj);
        Ranges.Add(offset, res->GetUsedSize());
        return *res;
      }

      const RawOrnament& GetOrnament(uint_t index) const
      {
        const uint16_t offset = fromLE(GetServiceObject<uint16_t>(index, fromLE(Source.OrnamentsOffset))) - UnfixDelta;
        const RawObject* const obj = Delegate.GetField<RawObject>(offset);
        Require(obj != nullptr);
        const RawOrnament* const res = safe_ptr_cast<const RawOrnament*>(obj);
        Ranges.Add(offset, res->GetUsedSize());
        return *res;
      }

      template<class T>
      const T& GetObject(uint_t offset) const
      {
        const T* const src = Delegate.GetField<T>(offset);
        Require(src != nullptr);
        Ranges.Add(offset, sizeof(T));
        return *src;
      }

      template<class T>
      const T& GetServiceObject(std::size_t index, std::size_t baseOffset) const
      {
        const std::size_t offset = baseOffset + index * sizeof(T);
        const T* const src = Delegate.GetField<T>(offset);
        Require(src != nullptr);
        Ranges.AddService(offset, sizeof(T));
        return *src;
      }

      uint8_t PeekByte(std::size_t offset) const
      {
        const uint8_t* const data = Delegate.GetField<uint8_t>(offset);
        Require(data != nullptr);
        return *data;
      }

      struct DataCursors : public std::array<std::size_t, 3>
      {
        DataCursors(const RawPattern& src, uint_t minOffset, uint_t unfixDelta)
        {
          for (std::size_t idx = 0; idx != size(); ++idx)
          {
            const std::size_t offset = fromLE(src.Offsets[idx]);
            Require(offset >= minOffset + unfixDelta);
            at(idx) = offset - unfixDelta;
          }
        }
      };

      struct ParserState
      {
        struct ChannelState
        {
          std::size_t Offset;
          uint_t Period;
          uint_t Counter;

          ChannelState()
            : Offset()
            , Period()
            , Counter()
          {
          }

          void Skip(uint_t toSkip)
          {
            Counter -= toSkip;
          }


          static bool CompareByCounter(const ChannelState& lh, const ChannelState& rh)
          {
            return lh.Counter < rh.Counter;
          }
        };

        std::array<ChannelState, 3> Channels;

        explicit ParserState(const DataCursors& src)
          : Channels()
        {
          for (std::size_t idx = 0; idx != src.size(); ++idx)
          {
            Channels[idx].Offset = src[idx];
          }
        }

        uint_t GetMinCounter() const
        {
          return std::min_element(Channels.begin(), Channels.end(), &ChannelState::CompareByCounter)->Counter;
        }

        void SkipLines(uint_t toSkip)
        {
          std::for_each(Channels.begin(), Channels.end(), std::bind2nd(std::mem_fun_ref(&ChannelState::Skip), toSkip));
        }
      };

      bool ParsePattern(uint_t patIndex, uint_t minOffset, Builder& builder) const
      {
        const RawPattern& src = GetPattern(patIndex);
        const DataCursors rangesStarts(src, minOffset, UnfixDelta);
        ParserState state(rangesStarts);
        PatternBuilder& patBuilder = builder.StartPattern(patIndex);
        uint_t lineIdx = 0;
        for (; lineIdx < MAX_PATTERN_SIZE; ++lineIdx)
        {
          //skip lines if required
          if (const uint_t linesToSkip = state.GetMinCounter())
          {
            state.SkipLines(linesToSkip);
            lineIdx += linesToSkip;
          }
          if (!HasLine(state))
          {
            patBuilder.Finish(std::max<uint_t>(lineIdx, MIN_PATTERN_SIZE));
            break;
          }
          patBuilder.StartLine(lineIdx);
          ParseLine(state, builder);
        }
        for (uint_t chanNum = 0; chanNum != rangesStarts.size(); ++chanNum)
        {
          const std::size_t start = rangesStarts[chanNum];
          if (start >= Delegate.GetSize())
          {
            Dbg("Invalid offset (%1%)", start);
          }
          else
          {
            const std::size_t stop = std::min(Delegate.GetSize(), state.Channels[chanNum].Offset + 1);
            Ranges.AddFixed(start, stop - start);
          }
        }
        return lineIdx >= MIN_PATTERN_SIZE;
      }

      bool HasLine(const ParserState& src) const
      {
        for (uint_t chan = 0; chan < 3; ++chan)
        {
          const ParserState::ChannelState& state = src.Channels[chan];
          if (state.Counter)
          {
            continue;
          }
          if (state.Offset >= Delegate.GetSize())
          {
            return false;
          }
          else if (0 == chan && 0x00 == PeekByte(state.Offset))
          {
            return false;
          }
        }
        return true;
      }

      void ParseLine(ParserState& src, Builder& builder) const
      {
        for (uint_t chan = 0; chan < 3; ++chan)
        {
          ParserState::ChannelState& state = src.Channels[chan];
          if (state.Counter)
          {
            --state.Counter;
            continue;
          }
          builder.StartChannel(chan);
          ParseChannel(state, builder);
          state.Counter = state.Period;
        }
      }

      void ParseChannel(ParserState::ChannelState& state, Builder& builder) const
      {
        while (state.Offset < Delegate.GetSize())
        {
          const uint_t cmd = PeekByte(state.Offset++);
          if (cmd == 0)
          {
            continue;
          }
          else if (cmd <= 0x60)//note
          {
            builder.SetNote(cmd - 1);
            break;
          }
          else if (cmd <= 0x6f)//sample
          {
            builder.SetSample(cmd - 0x61);
          }
          else if (cmd <= 0x7f)//ornament
          {
            builder.SetOrnament(cmd - 0x70);
            builder.SetNoEnvelope();
            builder.SetGliss(0);
          }
          else if (cmd <= 0xbf) //skip
          {
            state.Period = cmd - 0x80;
          }
          else if (cmd <= 0xcf) //envelope
          {
            if (cmd != 0xc0)
            {
              builder.SetEnvelope(cmd - 0xc0, PeekByte(state.Offset++));
            }
            else
            {
              builder.SetEnvelope(0, 0);
            }
            builder.SetOrnament(0);
            builder.SetGliss(0);
          }
          else if (cmd <= 0xdf) //reset
          {
            builder.SetRest();
            break;
          }
          else if (cmd <= 0xef)//empty
          {
            break;
          }
          else if (cmd == 0xf0)//glissade
          {
            builder.SetGliss(static_cast<int8_t>(PeekByte(state.Offset++)));
          }
          else //volume
          {
            builder.SetVolume(cmd - 0xf1);
          }
        }
      }

      static Sample ParseSample(const RawSample& src)
      {
        Sample dst;
        const uint_t size = src.GetSize();
        dst.Lines.resize(size);
        for (uint_t idx = 0; idx < size; ++idx)
        {
          const RawSample::Line& line = src.GetLine(idx);
          Sample::Line& res = dst.Lines[idx];
          res.Level = line.GetLevel();
          res.Noise = line.GetNoise();
          res.ToneMask = line.GetToneMask();
          res.NoiseMask = line.GetNoiseMask();
          res.EnvelopeMask = line.GetEnvelopeMask();
          res.Vibrato = line.GetVibrato();
        }
        dst.Loop = std::min(src.GetLoop(), size);
        return dst;
      }

      static Ornament ParseOrnament(const RawOrnament& src)
      {
        Ornament dst;
        const uint_t size = src.GetSize();
        dst.Lines.resize(size);
        for (uint_t idx = 0; idx < size; ++idx)
        {
          dst.Lines[idx] = src.GetLine(idx);
        }
        dst.Loop = std::min(src.GetLoop(), size);
        return dst;
      }
    private:
      const Binary::TypedContainer Delegate;
      RangesMap Ranges;
      const RawHeader& Source;
      const RawId& Id;
      const uint_t UnfixDelta;
    };

    enum AreaTypes
    {
      HEADER,
      IDENTIFIER,
      POSITIONS,
      PATTERNS,
      ORNAMENTS,
      SAMPLES,
      END
    };

    struct Areas : public AreaController
    {
      Areas(const RawHeader& header, std::size_t size)
      {
        AddArea(HEADER, 0);
        const std::size_t idOffset = sizeof(header);
        if (idOffset + sizeof(RawId) <= size)
        {
          const RawId* const id = safe_ptr_cast<const RawId*>(&header + 1);
          if (id->Check())
          {
            AddArea(IDENTIFIER, sizeof(header));
          }
        }
        AddArea(POSITIONS, fromLE(header.PositionsOffset));
        AddArea(PATTERNS, fromLE(header.PatternsOffset));
        AddArea(ORNAMENTS, fromLE(header.OrnamentsOffset));
        AddArea(SAMPLES, fromLE(header.SamplesOffset));
        AddArea(END, size);
      }

      bool CheckHeader() const
      {
        return sizeof(RawHeader) <= GetAreaSize(HEADER) && Undefined == GetAreaSize(END);
      }

      bool CheckPositions(uint_t positions) const
      {
        if (!positions)
        {
          return false;
        }
        const std::size_t size = GetAreaSize(POSITIONS);
        if (Undefined == size)
        {
          return false;
        }
        const std::size_t requiredSize = sizeof(RawPositions) + (positions - 1) * sizeof(RawPositions::PosEntry);
        if (requiredSize > size)
        {
          //no place
          return false;
        }
        //check for possible invalid length in header- real positions should fit place
        return 0 == (size - requiredSize) % sizeof(RawPositions::PosEntry);
      }

      bool CheckSamples() const
      {
        const std::size_t size = GetAreaSize(SAMPLES);
        if (Undefined == size)
        {
          return false;
        }
        //samples offsets should be the last region
        return IsLast(SAMPLES);
      }

      bool CheckOrnaments() const
      {
        const std::size_t size = GetAreaSize(ORNAMENTS);
        if (Undefined == size)
        {
          return false;
        }
        const std::size_t requiredSize = sizeof(RawOrnaments);
        return requiredSize == size;
      }
    private:
      bool IsLast(AreaTypes area) const
      {
        const std::size_t addr = GetAreaAddress(area);
        const std::size_t size = GetAreaSize(area);
        const std::size_t limit = GetAreaAddress(END);
        return addr + size == limit;
      }
    };

    Binary::TypedContainer CreateContainer(const Binary::Container& rawData)
    {
      const std::size_t size = std::min(rawData.Size(), MAX_SIZE);
      return Binary::TypedContainer(rawData, size);
    }
    
    bool CheckHeader(const RawHeader& hdr)
    {
      return Math::InRange<uint_t>(hdr.Tempo, 3, 15)
          && Math::InRange<uint_t>(fromLE(hdr.PositionsOffset), sizeof(hdr), 0x2600)
          && Math::InRange<uint_t>(fromLE(hdr.PatternsOffset), sizeof(hdr), 0x2700)
          && Math::InRange<uint_t>(fromLE(hdr.OrnamentsOffset), sizeof(hdr), 0x2700)
          && Math::InRange<uint_t>(fromLE(hdr.SamplesOffset), sizeof(hdr), 0x2700)
      ;
    }

    bool Check(const Binary::TypedContainer& data)
    {
      const RawHeader* const hdr = data.GetField<RawHeader>(0);
      if (nullptr == hdr)
      {
        return false;
      }
      if (!CheckHeader(*hdr))
      {
        return false;
      }
      const Areas areas(*hdr, std::min(data.GetSize(), GetMaxSize(*hdr)));
      if (!areas.CheckHeader())
      {
        return false;
      }
      if (!areas.CheckSamples())
      {
        return false;
      }
      if (!areas.CheckOrnaments())
      {
        return false;
      }
      if (const RawPositions* positions = data.GetField<RawPositions>(areas.GetAreaAddress(POSITIONS)))
      {
        if (!areas.CheckPositions(positions->Length))
        {
          return false;
        }
      }
      else
      {
        return false;
      }
      return true;
    }

    bool Check(const Binary::Container& rawData)
    {
      const Binary::TypedContainer data(CreateContainer(rawData));
      return Check(data);
    }

    const std::string FORMAT(
      "03-0f"  // uint8_t Tempo; 3..15
      "?00-26" // uint16_t PositionsOffset; 0..MAX_MODULE_SIZE
      "?00-27" // uint16_t PatternsOffset; 0..MAX_MODULE_SIZE
      "?00-27" // uint16_t OrnamentsOffset; 0..MAX_MODULE_SIZE
      "?00-27" // uint16_t SamplesOffset; 0..MAX_MODULE_SIZE
    );

    class Decoder : public Formats::Chiptune::SoundTrackerPro::Decoder
    {
    public:
      Decoder()
        : Header(Binary::CreateFormat(FORMAT, MIN_SIZE))
      {
      }

      String GetDescription() const override
      {
        return Text::SOUNDTRACKERPROCOMPILED_DECODER_DESCRIPTION;
      }

      Binary::Format::Ptr GetFormat() const override
      {
        return Header;
      }

      bool Check(const Binary::Container& rawData) const override
      {
        return Header->Match(rawData) && SoundTrackerProCompiled::Check(rawData);
      }

      Formats::Chiptune::Container::Ptr Decode(const Binary::Container& rawData) const override
      {
        Builder& stub = GetStubBuilder();
        return ParseCompiled(rawData, stub);
      }

      Formats::Chiptune::Container::Ptr Parse(const Binary::Container& rawData, Builder& target) const override
      {
        return ParseCompiled(rawData, target);
      }
    private:
      const Binary::Format::Ptr Header;
    };
  }//SoundTrackerProCompiled

  namespace SoundTrackerPro
  {
    Builder& GetStubBuilder()
    {
      static SoundTrackerProCompiled::StubBuilder stub;
      return stub;
    }

    Decoder::Ptr CreateCompiledModulesDecoder()
    {
      return MakePtr<SoundTrackerProCompiled::Decoder>();
    }

    Formats::Chiptune::Container::Ptr ParseCompiled(const Binary::Container& rawData, Builder& target)
    {
      using namespace SoundTrackerProCompiled;
      if (!Check(rawData))
      {
        return Formats::Chiptune::Container::Ptr();
      }
      try
      {
        const Binary::TypedContainer totalData(rawData);
        const std::size_t maxSize = GetMaxSize(*totalData.GetField<RawHeader>(0));
        const Binary::TypedContainer data(rawData, std::min(rawData.Size(), maxSize));
        const Format format(data);

        format.ParseCommonProperties(target);

        StatisticCollectingBuilder statistic(target);
        format.ParsePositions(statistic);
        const Indices& usedPatterns = statistic.GetUsedPatterns();
        format.ParsePatterns(usedPatterns, statistic);
        const Indices& usedSamples = statistic.GetUsedSamples();
        format.ParseSamples(usedSamples, target);
        const Indices& usedOrnaments = statistic.GetUsedOrnaments();
        format.ParseOrnaments(usedOrnaments, target);

        Require(format.GetSize() >= MIN_SIZE);
        const Binary::Container::Ptr subData = rawData.GetSubcontainer(0, format.GetSize());
        const RangeChecker::Range fixedRange = format.GetFixedArea();
        return CreateCalculatingCrcContainer(subData, fixedRange.first, fixedRange.second - fixedRange.first);
      }
      catch (const std::exception&)
      {
        Dbg("Failed to create");
        return Formats::Chiptune::Container::Ptr();
      }
    }

    Binary::Container::Ptr InsertMetaInformation(const Binary::Container& rawData, const Dump& info)
    {
      using namespace SoundTrackerProCompiled;
      StatisticCollectingBuilder statistic(GetStubBuilder());
      if (Binary::Container::Ptr parsed = ParseCompiled(rawData, statistic))
      {
        const Binary::TypedContainer typedHelper(CreateContainer(*parsed));
        const RawHeader& header = *typedHelper.GetField<RawHeader>(0);
        const std::size_t headerSize = sizeof(header);
        const std::size_t infoSize = info.size();
        const PatchedDataBuilder::Ptr patch = PatchedDataBuilder::Create(*parsed);
        const RawId* const id = typedHelper.GetField<RawId>(headerSize);
        if (id && id->Check())
        {
          patch->OverwriteData(headerSize, info);
        }
        else
        {
          patch->InsertData(headerSize, info);
          const int_t delta = static_cast<int_t>(infoSize);
          patch->FixLEWord(offsetof(RawHeader, PositionsOffset), delta);
          patch->FixLEWord(offsetof(RawHeader, PatternsOffset), delta);
          patch->FixLEWord(offsetof(RawHeader, OrnamentsOffset), delta);
          patch->FixLEWord(offsetof(RawHeader, SamplesOffset), delta);
          const std::size_t patternsStart = fromLE(header.PatternsOffset);
          Indices usedPatterns = statistic.GetUsedPatterns();
          //first pattern is used to detect fixdelta
          usedPatterns.Insert(0);
          for (Indices::Iterator it = usedPatterns.Items(); it; ++it)
          {
            const std::size_t patOffsets = patternsStart + *it * sizeof(RawPattern);
            patch->FixLEWord(patOffsets + 0, delta);
            patch->FixLEWord(patOffsets + 2, delta);
            patch->FixLEWord(patOffsets + 4, delta);
          }
          const std::size_t ornamentsStart = fromLE(header.OrnamentsOffset);
          const Indices& usedOrnaments = statistic.GetUsedOrnaments();
          for (Indices::Iterator it = usedOrnaments.Items(); it; ++it)
          {
            const std::size_t ornOffset = ornamentsStart + *it * sizeof(uint16_t);
            patch->FixLEWord(ornOffset, delta);
          }
          const std::size_t samplesStart = fromLE(header.SamplesOffset);
          const Indices& usedSamples = statistic.GetUsedSamples();
          for (Indices::Iterator it = usedSamples.Items(); it; ++it)
          {
            const std::size_t samOffset = samplesStart + *it * sizeof(uint16_t);
            patch->FixLEWord(samOffset, delta);
          }
        }
        return patch->GetResult();
      }
      return Binary::Container::Ptr();
    }
  }// namespace SoundTrackerPro

  Formats::Chiptune::Decoder::Ptr CreateSoundTrackerProCompiledDecoder()
  {
    return SoundTrackerPro::CreateCompiledModulesDecoder();
  }
}// namespace Chiptune
}// namespace Formats
