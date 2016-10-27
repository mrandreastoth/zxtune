/**
*
* @file
*
* @brief  Binary format expression interface and factory
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//common includes
#include <iterator.h>
#include <types.h>
//std includes
#include <memory>
#include <string>

namespace Binary
{
  namespace FormatDSL
  {
    class Predicate
    {
    public:
      typedef std::shared_ptr<const Predicate> Ptr;
      virtual ~Predicate() = default;

      virtual bool Match(uint_t val) const = 0;
    };

    typedef std::vector<Predicate::Ptr> Pattern;

    class Expression
    {
    public:
      typedef std::unique_ptr<const Expression> Ptr;
      virtual ~Expression() = default;

      virtual std::size_t StartOffset() const = 0;
      virtual ObjectIterator<Predicate::Ptr>::Ptr Predicates() const = 0;

      static Ptr Parse(const std::string& notation);
    };
  }
}
