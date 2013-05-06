/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright 2013  Martin Bříza <mbriza@redhat.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef BASICSMBACKEND_H
#define BASICSMBACKEND_H

#include "../kworkspace.h"
#include "../kdisplaymanager.h"

#include <QString>
#include <QList>
#include <QtDBus>

class KDMBackendPrivate;

class NullSMBackend {
public:
    NullSMBackend() { }
    virtual ~NullSMBackend() { }
    virtual bool canShutdown() { return false; }
    virtual void shutdown(KWorkSpace::ShutdownType, KWorkSpace::ShutdownMode, const QString&) { }
    virtual void setLock(bool) { }
    virtual bool isSwitchable() { return false; }
    virtual bool localSessions(SessList&) { return false; }
    virtual bool switchVT(int) { return false; }
};

/**
 * Basic Session Manager Backend
 * 
 * Implements methods for user session management
 */
class BasicSMBackend : public NullSMBackend {
private:
    KDMBackendPrivate *d;
    enum { Dunno, NoDM, KDM, GDM, LightDM } DMType;
public:
    BasicSMBackend(KDMBackendPrivate *p);
    virtual ~BasicSMBackend();
    virtual bool canShutdown();
    virtual void shutdown(KWorkSpace::ShutdownType shutdownType, KWorkSpace::ShutdownMode shutdownMode, const QString &bootOption = QString());
    virtual void setLock(bool on);
    virtual bool isSwitchable();
    virtual bool localSessions(SessList &list);
    virtual bool switchVT(int vt);
};

class Login1SMBackend : public BasicSMBackend {
private:
    bool getCurrentSeat(QDBusObjectPath *currentSession, QDBusObjectPath *currentSeat);
    QList<QDBusObjectPath> getSessionsForSeat(const QDBusObjectPath &path);
public:
    Login1SMBackend();
    virtual ~Login1SMBackend();
    virtual bool canShutdown();
    virtual void shutdown(KWorkSpace::ShutdownType shutdownType, KWorkSpace::ShutdownMode shutdownMode, const QString &bootOption = QString());
    virtual void setLock(bool on);
    virtual bool isSwitchable();
    virtual bool localSessions(SessList &list);
    virtual bool switchVT(int vt);
};

class ConsoleKitSMBackend : public BasicSMBackend {
private:
    bool getCurrentSeat(QDBusObjectPath *currentSession, QDBusObjectPath *currentSeat);
    QList<QDBusObjectPath> getSessionsForSeat(const QDBusObjectPath &path);
public:
    ConsoleKitSMBackend();
    virtual ~ConsoleKitSMBackend();
    virtual bool canShutdown();
    virtual void shutdown(KWorkSpace::ShutdownType shutdownType, KWorkSpace::ShutdownMode shutdownMode, const QString &bootOption = QString());
    virtual void setLock(bool on);
    virtual bool isSwitchable();
    virtual bool localSessions(SessList &list);
    virtual bool switchVT(int vt);
};

#endif // BASICSMBACKEND_H
