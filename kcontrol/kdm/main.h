/*
 * main.h
 *
 * Copyright (c) 1999 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
 *
 * Requires the Qt widget libraries, available at no cost at
 * http://www.troll.no/
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __kdm_main_h
#define __kdm_main_h

#include <qtabwidget.h>


#include <kcmodule.h>


class KDMAppearanceWidget;
class KBackground;
class KDMFontWidget;
class KDMLiloWidget;
class KDMSessionsWidget;
class KDMUsersWidget;


class KDModule : public KCModule
{
  Q_OBJECT

public:

  KDModule(QWidget *parent, const char *name);

  void load();
  void save();
  void defaults();


protected:

  void resizeEvent(QResizeEvent *e);


protected slots:

  void moduleChanged(bool state);


private:

  QTabWidget          *tab;

  KDMAppearanceWidget *appearance;
  KBackground         *background;
  KDMFontWidget       *font;
  KDMLiloWidget       *lilo;
  KDMSessionsWidget   *sessions;
  KDMUsersWidget      *users;

};

#endif

