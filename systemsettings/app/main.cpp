/**
 * Copyright (C) 2009 Ben Cooksley <ben@eclipse.endoftheinternet.org>
 *
 * This file was sourced from the System Settings package
 * Copyright (C) 2005 Benjamin C Meyer
 *                    <ben+systempreferences at meyerhome dot net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <KLocale>
#include <KAboutData>
#include <KCmdLineArgs>
#include <KUniqueApplication>

#include "SystemSettingsApp.h"
#include "SettingsBase.h"

int main( int argc, char *argv[] )
{
    // About data
    KAboutData aboutData("systemsettings", 0, ki18n("System Settings"), "1.0", ki18n("Central configuration center for KDE."),
                         KAboutData::License_GPL, ki18n("Copyright © 2009 System Settings authors"));
    aboutData.addAuthor(ki18n("Ben Cooksley"), ki18n("Maintainer"), "ben@eclipse.endoftheinternet.org");
    aboutData.addAuthor(ki18n("Mathias Soeken"), ki18n("Developer"), "msoeken@informatik.uni-bremen.de");
    aboutData.addAuthor(ki18n("Will Stephenson"), ki18n("Internal module representation, internal module model"), "wstephenson@kde.org");

    aboutData.setProgramIconName("preferences-system");
    KCmdLineArgs::init(argc, argv, &aboutData);

    SystemSettingsApp application;

    SettingsBase *mainWindow = new SettingsBase();
    mainWindow->show();
    application.setMainWindow(mainWindow);
    return application.exec();
}
