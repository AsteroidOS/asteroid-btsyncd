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

#include "application.h"
#include "notificationservice.h"
#include "weatherservice.h"
#include "mediaservice.h"
#include "batteryservice.h"
#include "screenshotservice.h"
#include "timeservice.h"

#include <QDBusMetaType>

Application::Application(QDBusConnection bus, QObject *parent) : QObject(parent)
{
    mPath = "/";

    addService(new NotificationService(0, bus));
    addService(new WeatherService(1, bus));
    addService(new MediaService(2, bus));
    addService(new BatteryService(3, bus));
    addService(new ScreenshotService(4, bus));
    addService(new TimeService(5, bus));

    qDBusRegisterMetaType<InterfaceList>();
    qDBusRegisterMetaType<ManagedObjectList>();

    bus.registerObject(mPath, this, QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllProperties);
}

QDBusObjectPath Application::getPath()
{
    return QDBusObjectPath(mPath);
}

void Application::addService(Service *service)
{
    mServices.append(service);
}

/* Exposed slots */
ManagedObjectList Application::GetManagedObjects()
{
    ManagedObjectList response;

    foreach(Service *serv, mServices) {
        InterfaceList serviceInterfaces;
        QVariantMap serviceProperties;
        serviceProperties.insert("UUID", serv->getUuid());
        serviceProperties.insert("Primary", serv->getPrimary());
        serviceProperties.insert("Characteristics", qVariantFromValue(serv->getCharacteristicPaths()));
        serviceInterfaces.insert(GATT_SERVICE_IFACE, serviceProperties);
        response.insert(serv->getPath(), serviceInterfaces);

        QList<Characteristic *> chrcs = serv->getCharacteristics();
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
    }

    return response;
}
