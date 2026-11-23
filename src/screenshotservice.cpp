/*
 * Copyright (C) 2017 - Florent Revest <revestflo@gmail.com>
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

#include <QDBusInterface>
#include <QDBusMessage>
#include <QDebug>
#include <QFile>

#include "screenshotservice.h"
#include "characteristic.h"
#include "common.h"

void ScreenshotReqChrc::WriteValue(QByteArray, QVariantMap)
{
    QList<QVariant> argumentList;
    argumentList << "/tmp/btsyncd-screenshot.jpg";
    static QDBusInterface notifyApp(SCREENSHOT_SERVICE_NAME, SCREENSHOT_PATH_BASE, SCREENSHOT_MAIN_IFACE, QDBusConnection::systemBus());
    QDBusMessage reply = notifyApp.callWithArgumentList(QDBus::AutoDetect, "saveScreenshot", argumentList);
    if(reply.type() == QDBusMessage::ErrorMessage)
        fprintf(stderr, "ScreenshotReqChrc::WriteValue: D-Bus Error: %s\n", reply.errorMessage().toStdString().c_str());
    emit screenshotTaken("/tmp/btsyncd-screenshot.jpg");
}

void ScreenshotContentChrc::onScreenshotTaken(QString path)
{
    QFile f(path);
    if(!f.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open" << path;
        updateValue(QByteArray::number(0x0));
        return;
    }

    qint64 totalSize = f.bytesAvailable();
    QByteArray size;
    size.append((totalSize >> 0) & 0xFF);
    size.append((totalSize >> 8) & 0xFF);
    size.append((totalSize >> 16) & 0xFF);
    size.append((totalSize >> 24) & 0xFF);
    updateValue(size);

    while (!f.atEnd())
        updateValue(f.read(20));
    f.close();
}


ScreenshotService::ScreenshotService(int index, QDBusConnection bus, QObject *parent) : Service(bus, index, SCREENSH_UUID, parent)
{
    ScreenshotReqChrc *reqChrc = new ScreenshotReqChrc(bus, 0, this);
    ScreenshotContentChrc *contChrc = new ScreenshotContentChrc(bus, 1, this);

    connect(reqChrc, SIGNAL(screenshotTaken(QString)), contChrc, SLOT(onScreenshotTaken(QString)));

    addCharacteristic(reqChrc);
    addCharacteristic(contChrc);
}
