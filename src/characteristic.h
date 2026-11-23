/*
 * Copyright (C) 2016 - Florent Revest <revestflo@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CHARACTERISTIC_H
#define CHARACTERISTIC_H

#include <QObject>
#include <QDBusConnection>
#include <QList>
#include <QString>

#include "descriptor.h"
#include "common.h"

class Service;

class Characteristic : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", GATT_CHRC_IFACE)
    Q_PROPERTY(QDBusObjectPath Service READ getService())
    Q_PROPERTY(QString UUID READ getUuid())
    Q_PROPERTY(QStringList Flags READ getFlags())
    Q_PROPERTY(QList<QDBusObjectPath> Descriptors READ getDescriptorPaths())
    Q_PROPERTY(QByteArray Value READ getValue NOTIFY valueChanged)
    Q_PROPERTY(bool Notifying READ getNotifying NOTIFY notifyingChanged)

public:
    explicit Characteristic(QDBusConnection bus, unsigned int index, QString uuid, QStringList flags, Service *service, QObject *parent = 0);

    QDBusObjectPath getPath();

    void addDescriptor(Descriptor *descriptor);
    QList<QDBusObjectPath> getDescriptorPaths();
    QList<Descriptor *> getDescriptors();

    QDBusObjectPath getService();
    QString getUuid();
    QStringList getFlags();
    QByteArray getValue() const { return mValue; }
    bool getNotifying();

protected:
    // Server-side notification support.
    //
    // updateValue() stores the new characteristic value and emits
    // org.freedesktop.DBus.Properties.PropertiesChanged for
    // org.bluez.GattCharacteristic1. BlueZ turns that signal into ATT
    // notifications for centrals that subscribed through StartNotify.
    // Subclasses that only need "notify on change" never touch D-Bus.
    //
    // emitNotifyingChange() announces the Notifying flag; it is called
    // automatically by StartNotify()/StopNotify().
    void updateValue(const QByteArray &value);
    void emitNotifyingChange();

    // Returns false (and logs) when a remote GATT write is shorter than the
    // fixed layout a WriteValue override is about to index into. Call this at
    // the top of every override that reads value at a fixed offset, so a short
    // payload from a paired phone can't drive an out-of-bounds read.
    bool hasMinLength(const QByteArray &value, int minBytes) const;

private:
    QString mPath;
    QDBusConnection mBus;
    QString mUuid;
    QStringList mFlags;
    QList<Descriptor *> mDescriptors;
    Service *mService;
    QByteArray mValue;
    bool mNotifying;

private slots:
    void emitPropertiesChanged(const QByteArray &value);

signals:
    // Emitted every time updateValue() changed the value, whether or not the
    // bytes differ from the previous chunk (identical consecutive chunks must
    // still notify).
    void valueChanged();
    void notifyingChanged();

    // Producer hooks: a service can connect to these to start/stop whatever
    // generates the values (sensors, D-Bus watches, ...). They fire exactly
    // once per state change, so repeated StartNotify() calls are harmless.
    void notifyEnabled();
    void notifyDisabled();

public slots:
    virtual QByteArray ReadValue(QVariantMap options);
    virtual void WriteValue(QByteArray value, QVariantMap options);

    // Idempotent notification state. BlueZ may call StartNotify several
    // times without StopNotify in between (and does so on re-subscribe);
    // state changes only on actual transitions. Subclasses that need the
    // old "do something on start" behavior should connect to the
    // notifyEnabled()/notifyDisabled() signals instead of overriding.
    virtual void StartNotify();
    virtual void StopNotify();
};

#endif // CHARACTERISTIC_H
