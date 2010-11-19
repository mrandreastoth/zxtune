/*
Abstract:
  Playlist container internal interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#ifndef ZXTUNE_QT_PLAYLIST_CONTAINER_IMPL_H_DEFINED
#define ZXTUNE_QT_PLAYLIST_CONTAINER_IMPL_H_DEFINED

//local includes
#include "container.h"
//common includes
#include <parameters.h>

namespace Playlist
{
  namespace IO
  {
    struct ContainerItem
    {
      String Path;
      Parameters::Accessor::Ptr AdjustedParameters;
    };

    typedef std::vector<ContainerItem> ContainerItems;
    typedef boost::shared_ptr<const ContainerItems> ContainerItemsPtr;

    Container::Ptr CreateContainer(PlayitemsProvider::Ptr provider,
      Parameters::Accessor::Ptr properties,
      ContainerItemsPtr items);
  }
}

#endif //ZXTUNE_QT_PLAYLIST_CONTAINER_IMPL_H_DEFINED
