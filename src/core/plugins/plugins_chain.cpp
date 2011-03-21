/*
Abstract:
  Plugins chain implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "plugins_chain.h"
//common includes
#include <string_helpers.h>
//std includes
#include <list>
//boost includes
#include <boost/make_shared.hpp>
//text includes
#include <core/text/core.h>

namespace
{
  using namespace ZXTune;

  class NestedPluginsChain : public PluginsChain
  {
  public:
    NestedPluginsChain(PluginsChain::Ptr parent, Plugin::Ptr newOne)
      : Parent(parent)
      , NewPlug(newOne)
    {
    }

    virtual void Add(Plugin::Ptr /*plugin*/)
    {
      assert(!"Should not be called");
    }

    virtual Plugin::Ptr GetLast() const
    {
      return NewPlug;
    }

    virtual uint_t Count() const
    {
      return Parent->Count() + 1;
    }

    virtual String AsString() const
    {
      const String thisId = NewPlug->Id();
      if (Parent->Count())
      {
        return Parent->AsString() + Text::MODULE_CONTAINERS_DELIMITER + thisId;
      }
      else
      {
        return thisId;
      }
    }
  private:
    const PluginsChain::Ptr Parent;
    const Plugin::Ptr NewPlug;
  };
}

namespace ZXTune
{
  PluginsChain::Ptr PluginsChain::CreateMerged(PluginsChain::Ptr parent, Plugin::Ptr newOne)
  {
    return boost::make_shared<NestedPluginsChain>(parent, newOne);
  }

}
