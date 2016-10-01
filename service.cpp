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

#include "service.h"
#include "common.h"

#include <QDBusMetaType>

Service::Service(QDBusConnection bus, unsigned int index, QString uuid, QObject *parent) : QObject(parent), mBus(QDBusConnection::systemBus())
{
    mPath = SERVICE_PATH_BASE + QString::number(index);
    mBus = bus;
    mUuid = uuid;

    mOmIface = new ServiceOmIface(this);

    qDBusRegisterMetaType<InterfaceList>();
    qDBusRegisterMetaType<ManagedObjectList>();

    bus.registerObject(mPath, this, QDBusConnection::ExportAdaptors | QDBusConnection::ExportAllProperties);
}

QDBusObjectPath Service::getPath()
{
    return QDBusObjectPath(mPath);
}

void Service::addCharacteristic(Characteristic *charac)
{
    mCharacteristics.append(charac);
}

QList<QDBusObjectPath> Service::getCharacteristicPaths()
{
    QList<QDBusObjectPath> result;
    foreach(Characteristic *carac, mCharacteristics)
        result.append(carac->getPath());
    return result;
}

QList<Characteristic *> Service::getCharacteristics()
{
    return mCharacteristics;
}

QString Service::getUuid()
{
    return mUuid;
}

bool Service::getPrimary()
{
    return true;
}

/* Exposed slots */
ManagedObjectList ServiceOmIface::GetManagedObjects()
{
    ManagedObjectList response;

    InterfaceList serviceInterfaces;
    QVariantMap serviceProperties;
    serviceProperties.insert("UUID", mService->getUuid());
    serviceProperties.insert("Primary", mService->getPrimary());
    serviceProperties.insert("Characteristics", qVariantFromValue(mService->getCharacteristicPaths()));
    serviceInterfaces.insert(GATT_SERVICE_IFACE, serviceProperties);
    response.insert(mService->getPath(), serviceInterfaces);

    QList<Characteristic *> chrcs = mService->getCharacteristics();
    foreach(Characteristic *chrc, chrcs) {
        InterfaceList chrcInterfaces;
        QVariantMap chrcProperties;
        chrcProperties.insert("Service", qVariantFromValue(chrc->getService()));
        chrcProperties.insert("UUID", chrc->getUuid());
        chrcProperties.insert("Flags", chrc->getFlags());
        chrcProperties.insert("Descriptors", qVariantFromValue(chrc->getDescriptorPaths()));
        chrcInterfaces.insert(GATT_CHRC_IFACE, chrcProperties);
        response.insert(chrc->getPath(), chrcInterfaces);

        QList<Descriptor *> descs = chrc->getDescriptors();
        foreach(Descriptor *desc, descs) {
            InterfaceList descInterfaces;
            QVariantMap descProperties;
            descProperties.insert("UUID", desc->getUuid());
            descProperties.insert("Characteristic", qVariantFromValue(desc->getCharacteristic()));
            descProperties.insert("Flags", desc->getFlags());
            descInterfaces.insert(GATT_DESC_IFACE, descProperties);
            response.insert(desc->getPath(), descInterfaces);
        }
    }

    return response;
}
