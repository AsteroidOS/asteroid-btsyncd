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

#include "descriptor.h"
#include "common.h"
#include "characteristic.h"

Descriptor::Descriptor(QDBusConnection bus, unsigned int index, QStringList flags, Characteristic *characteristic, QString uuid, QObject *parent) : QObject(parent), mBus(QDBusConnection::systemBus())
{
    mPath = characteristic->getPath().path() + "/desc" + QString::number(index);
    mBus = bus;
    mUuid = uuid;
    mFlags = flags;
    mChrc = characteristic;
    bus.registerObject(mPath, this, QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllProperties);
}

QDBusObjectPath Descriptor::getCharacteristic()
{
    return mChrc->getPath();
}

QString Descriptor::getUuid()
{
    return mUuid;
}

QDBusObjectPath Descriptor::getPath()
{
    return QDBusObjectPath(mPath);
}

QStringList Descriptor::getFlags()
{
    return mFlags;
}

/* Exposed slots */

QByteArray Descriptor::ReadValue(QVariantMap)
{
    fprintf(stderr, "Default ReadValue called, returning error\n");
    return QByteArray();
}

void Descriptor::WriteValue(QByteArray, QVariantMap)
{
    fprintf(stderr, "Default WriteValue called, returning error\n");
}
