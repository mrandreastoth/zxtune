/*
Abstract:
  Playlist data caching provider

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#ifndef ZXTUNE_QT_PLAYITEMS_PROVIDER_H_DEFINED
#define ZXTUNE_QT_PLAYITEMS_PROVIDER_H_DEFINED

//common includes
#include <iterator.h>
//library includes
#include <core/module_detect.h>
//qt includes
#include <QtCore/QMetaType>
//boost includes
#include <boost/function.hpp>

//runtime attributes which are frequently requested
class PlayitemAttributes
{
public:
  typedef boost::shared_ptr<const PlayitemAttributes> Ptr;

  virtual ~PlayitemAttributes() {}

  virtual String GetType() const = 0;
  virtual String GetTitle() const = 0;
  virtual uint_t GetDuration() const = 0;
};

class Playitem
{
public:
  typedef boost::shared_ptr<const Playitem> Ptr;
  typedef ObjectIterator<Playitem::Ptr> Iterator;

  virtual ~Playitem() {}

  virtual const PlayitemAttributes& GetAttributes() const = 0;
  virtual ZXTune::Module::Holder::Ptr GetModule() const = 0;
  virtual Parameters::Container::Ptr GetAdjustedParameters() const = 0;
};

Q_DECLARE_METATYPE(Playitem::Ptr);

class PlayitemDetectParameters
{
public:
  virtual ~PlayitemDetectParameters() {}

  virtual bool ProcessPlayitem(Playitem::Ptr item) = 0;
  virtual void ShowProgress(const Log::MessageData& msg) = 0;
};

class PlayitemsProvider
{
public:
  typedef boost::shared_ptr<PlayitemsProvider> Ptr;

  virtual ~PlayitemsProvider() {}

  virtual Error DetectModules(const String& path, PlayitemDetectParameters& detectParams) const = 0;

  virtual Error OpenModule(const String& path, PlayitemDetectParameters& detectParams) const = 0;

  static Ptr Create(Parameters::Accessor::Ptr ioParams, Parameters::Accessor::Ptr coreParams);
};

#endif //ZXTUNE_QT_PLAYITEMS_PROVIDER_H_DEFINED
