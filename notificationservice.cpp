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

#include "notificationservice.h"
#include "characteristic.h"
#include "common.h"

#include <QDBusInterface>

class NotificationsUpdateChrc : public Characteristic
{
public:
    NotificationsUpdateChrc(QDBusConnection bus, unsigned int index, Service *service) : Characteristic(bus, index, NOTI_UPD_UUID, {"write"}, service, service) {}

public slots:
    void WriteValue(QByteArray value)
    {
        QVariantMap hints;
        hints.insert("x-nemo-preview-body", QString(value));
        hints.insert("x-nemo-preview-summary", "Phone");
        hints.insert("urgency", 3);

        QList<QVariant> argumentList;
        argumentList << "asteroid-btsyncd"; // app name
        argumentList << (uint)0;            // replace id
        argumentList << "";                 // app icon
        argumentList << "Phone";            // summary
        argumentList << QString(value);     // body
        argumentList << QStringList();      // actions
        argumentList << hints;              // hints
        argumentList << (int)3000;          // timeout in ms

        static QDBusInterface notifyApp("org.freedesktop.Notifications", "/org/freedesktop/Notifications", "org.freedesktop.Notifications");
        QDBusMessage reply = notifyApp.callWithArgumentList(QDBus::AutoDetect, "Notify", argumentList);
        if(reply.type() == QDBusMessage::ErrorMessage) {
            fprintf(stderr, "NotificationsUpdateChrc::writeValue: D-Bus Error: %s\n", reply.errorMessage().toStdString().c_str());
        }
    }
};

class NotificationsFeedbackChrc : public Characteristic
{
public:
    NotificationsFeedbackChrc(QDBusConnection bus, int index, Service *service) : Characteristic(bus, index, NOTI_FDB_UUID, {"notify"}, service) {}

    // TODO: Feedback
};

NotificationService::NotificationService(QDBusConnection bus, int index, QObject *parent) : Service(bus, index, NOTIF_UUID, parent)
{
    addCharacteristic(new NotificationsUpdateChrc(bus, 0, this));
    addCharacteristic(new NotificationsFeedbackChrc(bus, 1, this));
}
