/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2009 Marco Martin <notmart@gmail.com>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/


#ifndef CURRENTAPPCONTROL_HEADER
#define CURRENTAPPCONTROL_HEADER

#include <Plasma/Applet>

namespace Plasma
{
    class IconWidget;
}

class CurrentAppControl : public Plasma::Applet
{
    Q_OBJECT
public:

    CurrentAppControl(QObject *parent, const QVariantList &args);
    ~CurrentAppControl();

    void init();
    void constraintsEvent(Plasma::Constraints constraints);

protected Q_SLOTS:
    void activeWindowChanged(WId id);
    void windowChanged(WId id);
    void syncActiveWindow();
    void closeWindow();
    void listWindows();

private:
    Plasma::IconWidget *m_currentTask;
    Plasma::IconWidget *m_closeTask;
    WId m_activeWindow;
    WId m_pendingActiveWindow;
};

K_EXPORT_PLASMA_APPLET(currentappcontrol, CurrentAppControl)
#endif
