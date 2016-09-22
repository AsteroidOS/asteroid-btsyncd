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

#include "characteristic.h"
#include "common.h"
#include "service.h"

Characteristic::Characteristic(QDBusConnection bus, unsigned int index, QString uuid, QStringList flags, Service *service, QObject *parent) : QObject(parent), mBus(QDBusConnection::systemBus())
{
    mPath = service->getPath().path() + "/char" + QString::number(index);

    mBus = bus;
    mUuid = uuid;
    mFlags = flags;
    mService = service;
    bus.registerObject(mPath, this, QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllProperties);
}

QDBusObjectPath Characteristic::getPath()
{
    return QDBusObjectPath(mPath);
}

void Characteristic::addDescriptor(Descriptor *descriptor)
{
    mDescriptors.append(descriptor);
}

QList<QDBusObjectPath> Characteristic::getDescriptorPaths()
{
    QList<QDBusObjectPath> result;
    foreach(Descriptor *desc, mDescriptors)
        result.append(desc->getPath());
    return result;
}

QList<Descriptor *> Characteristic::getDescriptors()
{
    return mDescriptors;
}

QDBusObjectPath Characteristic::getService()
{
    return mService->getPath();
}

QString Characteristic::getUuid()
{
    return mUuid;
}

QStringList Characteristic::getFlags()
{
    return mFlags;
}

/* Exposed slots */

QByteArray Characteristic::ReadValue()
{
    fprintf(stderr, "Default ReadValue called\n");
    return QByteArray();
}

void Characteristic::WriteValue(QByteArray value)
{
    fprintf(stderr, "Default WriteValue called\n");
}

void Characteristic::StartNotify()
{
    fprintf(stderr, "Default StartNotify called\n");
}

void Characteristic::StopNotify()
{
    fprintf(stderr, "Default StopNotify called\n");
}
