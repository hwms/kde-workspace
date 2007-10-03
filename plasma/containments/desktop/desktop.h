/*
*   Copyright 2007 by Aaron Seigo <aseigo@kde.org>
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU Library General Public License version 2,
*   or (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details
*
*   You should have received a copy of the GNU Library General Public
*   License along with this program; if not, write to the
*   Free Software Foundation, Inc.,
*   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef PLASMA_DESKTOP_H
#define PLASMA_DESKTOP_H

#include <QList>

#include <plasma/containment.h>

class QAction;
namespace Plasma
{
    class AppletBrowser;
}

class DefaultDesktop : public Plasma::Containment
{
    Q_OBJECT

public:
    DefaultDesktop(QObject *parent, const QVariantList &args);
    ~DefaultDesktop();

    QList<QAction*> contextActions();

protected Q_SLOTS:
    void launchExplorer();
    void launchAppletBrowser();
    void runCommand();
    void lockScreen();
    void logout();

private:
    QAction *m_engineExplorerAction;
    QAction *m_appletBrowserAction;
    QAction *m_runCommandAction;
    QAction *m_lockAction;
    QAction *m_logoutAction;
    Plasma::AppletBrowser *m_appletBrowser;
};

#endif // PLASMA_PANEL_H
