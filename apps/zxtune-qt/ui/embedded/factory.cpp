/*
Abstract:
  UI widgets factory implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "ui/factory.h"
#include "mainwindow.h"

namespace
{
  class EmbeddedWidgetsFactory : public WidgetsFactory
  {
  public:
    virtual QPointer<QMainWindow> CreateMainWindow(Parameters::Container::Ptr options, const StringArray& cmdline) const
    {
      return QPointer<QMainWindow>(EmbeddedMainWindow::Create(options, cmdline));
    }
  };
}

WidgetsFactory& WidgetsFactory::Instance()
{
  static EmbeddedWidgetsFactory instance;
  return instance;
}