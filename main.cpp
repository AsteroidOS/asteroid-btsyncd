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
#include "advertisement.h"
#include "bluezmanager.h"

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    if (!QDBusConnection::systemBus().isConnected()) {
        fprintf(stderr, "Cannot connect to the D-Bus system bus.\n");
        return 3;
    }
    if (!QDBusConnection::sessionBus().isConnected()) {
        fprintf(stderr, "Cannot connect to the D-Bus session bus.\n");
        return 2;
    }
    /* if (!bus.registerService(SERVICE_NAME)) {
        fprintf(stderr, "%s\n",
                qPrintable(QDBusConnection::systemBus().lastError().message()));
        exit(1);
    } */

    NotificationService notifServ(0);
    WeatherService weatherServ(1);
    MediaService mediaServ(2);
    QList<QDBusObjectPath> servicesPathList;
    servicesPathList << notifServ.getPath() << weatherServ.getPath() << mediaServ.getPath();

    Advertisement advert;

    BlueZManager bm(servicesPathList, advert.getPath());

    app.exec();
    return 0;
}
