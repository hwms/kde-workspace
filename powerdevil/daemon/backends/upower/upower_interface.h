/*
 * This file was generated by qdbusxml2cpp version 0.7
 * Command line was: qdbusxml2cpp -N -m -p upower_interface /home/ltinkl/svn/KDE/kdebase/workspace/powerdevil/daemon/backends/upower/dbus/org.freedesktop.UPower.xml
 *
 * qdbusxml2cpp is Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 *
 * This is an auto-generated file.
 * Do not edit! All changes made to it will be lost.
 */

#ifndef UPOWER_INTERFACE_H_1287851466
#define UPOWER_INTERFACE_H_1287851466

#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtDBus/QtDBus>

/*
 * Proxy class for interface org.freedesktop.UPower
 */
class OrgFreedesktopUPowerInterface: public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName()
    { return "org.freedesktop.UPower"; }

public:
    OrgFreedesktopUPowerInterface(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = 0);

    ~OrgFreedesktopUPowerInterface();

    Q_PROPERTY(bool CanHibernate READ canHibernate)
    inline bool canHibernate() const
    { return qvariant_cast< bool >(property("CanHibernate")); }

    Q_PROPERTY(bool CanSuspend READ canSuspend)
    inline bool canSuspend() const
    { return qvariant_cast< bool >(property("CanSuspend")); }

    Q_PROPERTY(QString DaemonVersion READ daemonVersion)
    inline QString daemonVersion() const
    { return qvariant_cast< QString >(property("DaemonVersion")); }

    Q_PROPERTY(bool LidIsClosed READ lidIsClosed)
    inline bool lidIsClosed() const
    { return qvariant_cast< bool >(property("LidIsClosed")); }

    Q_PROPERTY(bool LidIsPresent READ lidIsPresent)
    inline bool lidIsPresent() const
    { return qvariant_cast< bool >(property("LidIsPresent")); }

    Q_PROPERTY(bool OnBattery READ onBattery)
    inline bool onBattery() const
    { return qvariant_cast< bool >(property("OnBattery")); }

    Q_PROPERTY(bool OnLowBattery READ onLowBattery)
    inline bool onLowBattery() const
    { return qvariant_cast< bool >(property("OnLowBattery")); }

public Q_SLOTS: // METHODS
    inline QDBusPendingReply<> AboutToSleep()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QLatin1String("AboutToSleep"), argumentList);
    }

    inline QDBusPendingReply<QList<QDBusObjectPath> > EnumerateDevices()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QLatin1String("EnumerateDevices"), argumentList);
    }

    inline QDBusPendingReply<> Hibernate()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QLatin1String("Hibernate"), argumentList);
    }

    inline QDBusPendingReply<bool> HibernateAllowed()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QLatin1String("HibernateAllowed"), argumentList);
    }

    inline QDBusPendingReply<> Suspend()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QLatin1String("Suspend"), argumentList);
    }

    inline QDBusPendingReply<bool> SuspendAllowed()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QLatin1String("SuspendAllowed"), argumentList);
    }

Q_SIGNALS: // SIGNALS
    void Changed();
    void DeviceAdded(const QString &in0);
    void DeviceChanged(const QString &in0);
    void DeviceRemoved(const QString &in0);
    void Resuming();
    void Sleeping();
};

#endif
