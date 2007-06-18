// -*- Mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 8; -*-
/* This file is part of the KDE project
   Copyright (C) Andrew Stanley-Jones

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <stdio.h>
#include <stdlib.h>

#include <klocale.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <kuniqueapplication.h>

#include "tray.h"
#include "version.h"

extern "C" int KDE_EXPORT kdemain(int argc, char *argv[])
{
  Klipper::createAboutData();
  KCmdLineArgs::init( argc, argv, Klipper::aboutData());
  KUniqueApplication::addCmdLineOptions();

  if (!KUniqueApplication::start()) {
       fprintf(stderr, "Klipper is already running!\n");
       exit(0);
  }
  KUniqueApplication app;
  app.disableSessionManagement();

  KlipperTray *toplevel = new KlipperTray();

#ifdef __GNUC__
#warning Port to KSystemTrayIcon
#endif
  toplevel->setGeometry(-100, -100, 42, 42 );
  toplevel->show();

  int ret = app.exec();
  delete toplevel;
  Klipper::destroyAboutData();
  return ret;
}
