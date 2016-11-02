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

#ifndef DESCRIPTOR_H
#define DESCRIPTOR_H

#include <QObject>
#include <QDBusConnection>
#include <QDBusObjectPath>
#include <QString>
#include <QByteArray>

#include "common.h"

class Characteristic;

class Descriptor : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", GATT_DESC_IFACE)
    Q_PROPERTY(QDBusObjectPath Characteristic READ getCharacteristic())
    Q_PROPERTY(QString UUID READ getUuid())
    Q_PROPERTY(QStringList Flags READ getFlags())

public:
    explicit Descriptor(QDBusConnection bus, unsigned int index, QStringList flags, Characteristic *characteristic, QString uuid, QObject *parent);
    QDBusObjectPath getPath();

    QDBusObjectPath getCharacteristic();
    QString getUuid();
    QStringList getFlags();

private:
    QDBusConnection mBus;
    QString mUuid;
    QStringList mFlags;
    QString mPath;
    Characteristic *mChrc;

public slots:
    QByteArray ReadValue(QVariantMap options);
    void WriteValue(QByteArray value, QVariantMap options);
};

#endif // DESCRIPTOR_H
