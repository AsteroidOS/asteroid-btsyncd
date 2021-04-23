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

public:
    explicit Characteristic(QDBusConnection bus, unsigned int index, QString uuid, QStringList flags, Service *service, QObject *parent = 0);

    QDBusObjectPath getPath();

    void addDescriptor(Descriptor *descriptor);
    QList<QDBusObjectPath> getDescriptorPaths();
    QList<Descriptor *> getDescriptors();

    QDBusObjectPath getService();
    QString getUuid();
    QStringList getFlags();

private:
    QString mPath;
    QDBusConnection mBus;
    QString mUuid;
    QStringList mFlags;
    QList<Descriptor *> mDescriptors;
    Service *mService;


public slots:
    virtual QByteArray ReadValue(QVariantMap options);
    virtual void WriteValue(QByteArray value, QVariantMap options);
    virtual void StartNotify();
    virtual void StopNotify();
};

#endif // CHARACTERISTIC_H
