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

#include <stdio.h>
#include <stdlib.h>

#include <QtCore/QCoreApplication>
#include <QtDBus/QtDBus>

#include "common.h"
#include "notificationservice.h"
#include "weatherservice.h"
#include "mediaservice.h"

QString findAdapter(QDBusConnection bus)
{
    QDBusInterface remoteOm(BLUEZ_SERVICE_NAME, "/", DBUS_OM_IFACE, bus);

    QDBusMessage result = remoteOm.call("GetManagedObjects");

    const QDBusArgument argument = result.arguments().at(0).value<QDBusArgument>();
    if (argument.currentType() == QDBusArgument::MapType) {
        argument.beginMap();
        while (!argument.atEnd()) {
                QString key;
                QList<QVariant> value;

                argument.beginMapEntry();
                argument >> key >> value;
                argument.endMapEntry();

                if (value.contains(QVariant(QString(GATT_MANAGER_IFACE)))) {
                    return key;
                }
        }
        argument.endMap();
    }

    return "";
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QDBusConnection bus = QDBusConnection::systemBus();

    if (!bus.isConnected()) {
        fprintf(stderr, "Cannot connect to the D-Bus system bus.\n");
        return 2;
    }
    if (!QDBusConnection::sessionBus().isConnected()) {
        fprintf(stderr, "Cannot connect to the D-Bus session bus.\n");
        return 2;
    }

    QString adapter = findAdapter(bus);
    if(adapter.isEmpty()) {
        fprintf(stderr, "GattManager1 interface not found.\n");
        return 1;
    }
    /* if (!bus.registerService(SERVICE_NAME)) {
        fprintf(stderr, "%s\n",
                qPrintable(QDBusConnection::systemBus().lastError().message()));
        exit(1);
    } */

    QDBusInterface serviceManager(BLUEZ_SERVICE_NAME, adapter, GATT_MANAGER_IFACE, bus);

    NotificationService notifServ(bus, 0);
    WeatherService weatherServ(bus, 1);
    MediaService mediaServ(bus, 2);

    serviceManager.asyncCall("RegisterService", qVariantFromValue(notifServ.getPath()), QVariantMap());
    serviceManager.asyncCall("RegisterService", qVariantFromValue(weatherServ.getPath()), QVariantMap());
    serviceManager.asyncCall("RegisterService", qVariantFromValue(mediaServ.getPath()), QVariantMap());

    app.exec();
    return 0;
}
