/**
* 
* @file
*
* @brief  FSB subformats support
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "fmod.h"
//std includes
#include <map>

namespace Formats
{
namespace Archived
{
  namespace FSB
  {
    typedef std::map<String, Binary::Container::Ptr> NamedDataMap;

    class FormatBuilder : public Fmod::Builder
    {
    public:
      using Ptr = std::shared_ptr<FormatBuilder>;
      
      virtual NamedDataMap CaptureResult() = 0;
    };

    FormatBuilder::Ptr CreateMpegBuilder();
    FormatBuilder::Ptr CreatePcmBuilder();
  }
}
}
