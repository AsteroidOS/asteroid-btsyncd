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

#include "advertisement.h"

Advertisement::Advertisement(QDBusConnection bus, QObject *parent) : QObject(parent), mBus(QDBusConnection::systemBus())
{
    mPath = ADVERTISEMENT_PATH_BASE;
    mBus = bus;
    mAdType = "peripheral";
    mIncludeTxPower = false;
    addServiceUuid(ADVERT_UUID);
    mDiscoverable = true;

    bus.registerObject(mPath, this, QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllProperties);
}

QDBusObjectPath Advertisement::getPath()
{
    return QDBusObjectPath(mPath);
}

void Advertisement::addServiceUuid(QString uuid)
{
    mServiceUuids.append(uuid);
}

void Advertisement::addManufacturerData(unsigned int manufCode, QByteArray data)
{
    mManufacturerData.insert(manufCode, data);
}

void Advertisement::addServiceData(QString uuid, QByteArray data)
{
    mServiceData.insert(uuid, data);
}

QString Advertisement::getType()
{
    return mAdType;
}

QStringList Advertisement::getServiceUUIDs()
{
    return mServiceUuids;
}

QStringList Advertisement::getSolicitUUIDs()
{
    return mSolicitUuids;
}

QMap<unsigned int, QByteArray> Advertisement::getManufacturerData()
{
    return mManufacturerData;
}

QMap<QString, QByteArray> Advertisement::getServiceData()
{
    return mServiceData;
}

bool Advertisement::getIncludeTxPower()
{
    return mIncludeTxPower;
}

bool Advertisement::getDiscoverable()
{
    return mDiscoverable;
}

/* Exposed slots */
void Advertisement::Release()
{
    fprintf(stderr, "%s: Released!", mPath.toStdString().c_str());
}
