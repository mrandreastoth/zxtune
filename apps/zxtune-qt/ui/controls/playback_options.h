/*
Abstract:
  Playback options widget declaration

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_PLAYBACK_OPTIONS_H_DEFINED
#define ZXTUNE_QT_PLAYBACK_OPTIONS_H_DEFINED

//library includes
#include <sound/backend.h>
//qt includes
#include <QtGui/QWidget>

class PlaybackSupport;

class PlaybackOptions : public QWidget
{
  Q_OBJECT
protected:
  explicit PlaybackOptions(QWidget& parent);
public:
  //creator
  static PlaybackOptions* Create(QWidget& parent, PlaybackSupport& supp, Parameters::Container& params);
public slots:
  virtual void InitState(ZXTune::Sound::Backend::Ptr) = 0;
  virtual void CloseState() = 0;
};

#endif //ZXTUNE_QT_PLAYBACK_OPTIONS_H_DEFINED