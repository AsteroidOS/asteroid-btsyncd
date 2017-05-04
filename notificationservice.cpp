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
#include <QXmlStreamReader>

class NotificationsUpdateChrc : public Characteristic
{
public:
    NotificationsUpdateChrc(QDBusConnection bus, unsigned int index, QHash<int, uint> *knownAndroidNotifs, Service *service)
      : Characteristic(bus, index, NOTI_UPD_UUID, {"encrypt-authenticated-write"}, service, service), mKnownAndroidNotifs(knownAndroidNotifs) {}

public slots:
    void WriteValue(QByteArray value, QVariantMap)
    {
        int id;
        uint replacesId;
        QString packageName, appName, appIcon, summary, body, vibrate = nullptr;

        mReader.addData(value);

        if(value.endsWith("</insert>")) {
            if(mReader.readNextStartElement() && mReader.name() == "insert") {
                while(mReader.readNextStartElement()) {
                    if(mReader.name() == "pn") packageName = mReader.readElementText();
                    else if(mReader.name() == "id") id = mReader.readElementText().toInt();
                    else if(mReader.name() == "an") appName = mReader.readElementText();
                    else if(mReader.name() == "ai") appIcon = mReader.readElementText();
                    else if(mReader.name() == "su") summary = mReader.readElementText();
                    else if(mReader.name() == "bo") body = mReader.readElementText();
                    else if(mReader.name() == "vb") vibrate = mReader.readElementText();
                    else mReader.skipCurrentElement();
                }
                mReader.clear();

                replacesId = mKnownAndroidNotifs->value(id, 0);

                QVariantMap hints;
                hints.insert("x-nemo-preview-body", body);
                hints.insert("x-nemo-preview-summary", summary);

                if (vibrate == nullptr) // for backwards compatibility
                  hints.insert("x-nemo-feedback", "information_strong");
                else if (vibrate.compare("none") == 0)
                  hints.insert("x-nemo-feedback", "notifier");
                else if (vibrate.compare("normal") == 0)
                  hints.insert("x-nemo-feedback", "chat");
                else if (vibrate.compare("strong") == 0)
                  hints.insert("x-nemo-feedback", "information_strong");

                hints.insert("urgency", 3);

                QList<QVariant> argumentList;
                argumentList << appName;
                argumentList << replacesId;
                argumentList << appIcon;
                argumentList << summary;
                argumentList << body;
                argumentList << QStringList(); // actions
                argumentList << hints;
                argumentList << (int) 0;    // timeout

                static QDBusInterface notifyApp(NOTIFICATIONS_SERVICE_NAME, NOTIFICATIONS_PATH_BASE, NOTIFICATIONS_MAIN_IFACE);
                QDBusMessage reply = notifyApp.callWithArgumentList(QDBus::AutoDetect, "Notify", argumentList);
                if(reply.type() == QDBusMessage::ErrorMessage) {
                    fprintf(stderr, "NotificationsUpdateChrc::writeValue: D-Bus Error: %s\n", reply.errorMessage().toStdString().c_str());
                }

                if(!replacesId) {
                    if(reply.arguments().size() > 0)
                        mKnownAndroidNotifs->insert(id, reply.arguments()[0].toUInt());
                }
            } else
                mReader.clear();
        }
    }

private:
    QHash<int, uint> *mKnownAndroidNotifs;
    QXmlStreamReader mReader;
};

class NotificationsFeedbackChrc : public Characteristic
{
public:
    NotificationsFeedbackChrc(QDBusConnection bus, int index, QHash<int, uint> *knownAndroidNotifs, Service *service)
         : Characteristic(bus, index, NOTI_FDB_UUID, {"notify"}, service), mKnownAndroidNotifs(knownAndroidNotifs)
    {}
    // TODO: Feedback
private:
    QHash<int, uint> *mKnownAndroidNotifs;
};

NotificationService::NotificationService(int index, QDBusConnection bus, QObject *parent) : Service(bus, index, NOTIF_UUID, parent)
{
    addCharacteristic(new NotificationsUpdateChrc(bus, 0, &mKnownAndroidNotifs, this));
    addCharacteristic(new NotificationsFeedbackChrc(bus, 1, &mKnownAndroidNotifs, this));
    QDBusConnection::sessionBus().connect(NOTIFICATIONS_SERVICE_NAME, NOTIFICATIONS_PATH_BASE,
        NOTIFICATIONS_MAIN_IFACE, "NotificationClosed", this, SLOT(NotificationClosed(uint, uint)));
}

void NotificationService::NotificationClosed(uint replacesId, uint) {
    int id = mKnownAndroidNotifs.key(replacesId, 0);
    if(id)
        mKnownAndroidNotifs.remove(id);
}
