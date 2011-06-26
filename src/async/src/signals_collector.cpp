/*
Abstract:
  Signals working implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//common includes
#include <logging.h>
//library includes
#include <async/signals_collector.h>
//std includes
#include <algorithm>
#include <list>
//boost includes
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>

namespace
{
  const std::string THIS_MODULE("Signals");

  using namespace Async::Signals;

  class CollectorImpl : public Collector
  {
  public:
    typedef boost::shared_ptr<CollectorImpl> Ptr;
    typedef boost::weak_ptr<CollectorImpl> WeakPtr;

    explicit CollectorImpl(uint_t mask)
      : Mask(mask)
      , Signals(0)
    {
    }

    virtual bool WaitForSignals(uint_t& sigmask, uint_t timeoutMs)
    {
      boost::mutex::scoped_lock locker(Mutex);
      if (Event.timed_wait(locker, boost::posix_time::milliseconds(timeoutMs)))
      {
        assert(Signals);
        sigmask = Signals;
        Signals = 0;
        return true;
      }
      return false;
    }

    void Notify(uint_t sigmask)
    {
      if (0 != (sigmask & Mask))
      {
        const boost::mutex::scoped_lock locker(Mutex);
        Signals |= sigmask;
        Event.notify_one();
      }
    }
  private:
    const uint_t Mask;
    boost::mutex Mutex;
    boost::condition_variable Event;
    uint_t Signals;
  };

  class DispatcherImpl : public Dispatcher
  {
  public:
    DispatcherImpl()
    {
    }

    virtual Collector::Ptr CreateCollector(uint_t mask) const
    {
      const boost::mutex::scoped_lock lock(Lock);
      const CollectorImpl::Ptr collector = boost::make_shared<CollectorImpl>(mask);
      Collectors.push_back(CollectorEntry(collector));
      return collector;
    }

    virtual void Notify(uint_t sigmask)
    {
      const boost::mutex::scoped_lock lock(Lock);
      CollectorEntries::iterator newEnd = std::remove_if(Collectors.begin(), Collectors.end(),
        boost::bind(&CollectorEntry::Expired, _1));
      std::for_each(Collectors.begin(), newEnd, boost::bind(&CollectorEntry::DoSignal, _1, sigmask));
      //just for log yet
      std::for_each(newEnd, Collectors.end(), boost::mem_fn(&CollectorEntry::Release));
      Collectors.erase(newEnd, Collectors.end());
    }
  private:
    struct CollectorEntry
    {
      CollectorEntry()
        : Id(0)
      {
      }

      explicit CollectorEntry(CollectorImpl::Ptr ptr)
        : Delegate(ptr)
        , Id(ptr.get())
      {
        Log::Debug(THIS_MODULE, "Created collector %1$#x", Id);
      }

      bool Expired() const
      {
        return Delegate.expired();
      }

      void Release()
      {
        if (Id)
        {
          Log::Debug(THIS_MODULE, "Destroyed collector %1$#x", Id);
        }
        Delegate.reset();
        Id = 0;
      }

      void DoSignal(uint_t signal)
      {
        if (const CollectorImpl::Ptr ptr = Delegate.lock())
        {
          ptr->Notify(signal);
        }
      }
    private:
      CollectorImpl::WeakPtr Delegate;
      const void* Id;
    };
    typedef std::list<CollectorEntry> CollectorEntries;
    mutable boost::mutex Lock;
    mutable CollectorEntries Collectors;
  };
}

namespace Async
{
  namespace Signals
  {
    Dispatcher::Ptr Dispatcher::Create()
    {
      return Dispatcher::Ptr(new DispatcherImpl());
    }
  }
}