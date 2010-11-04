/*
Abstract:
  Playlist view

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#ifndef ZXTUNE_QT_PLAYLIST_VIEW_H_DEFINED
#define ZXTUNE_QT_PLAYLIST_VIEW_H_DEFINED

//qt includes
#include <QtGui/QWidget>

class PlaylistView : public QWidget
{
  Q_OBJECT
public:
  static PlaylistView* Create(QWidget* parent, const class PlaylistSupport& playlist);

  virtual const class PlaylistSupport& GetPlaylist() const = 0;
  virtual void Update() = 0;
};

#endif //ZXTUNE_QT_PLAYLIST_VIEW_H_DEFINED
