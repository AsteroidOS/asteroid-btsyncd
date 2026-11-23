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

#ifndef SCREENSHOTSERVICE_H
#define SCREENSHOTSERVICE_H

#include <QObject>

#include "service.h"

class ScreenshotReqChrc : public Characteristic
{
    Q_OBJECT
public:
    ScreenshotReqChrc(QDBusConnection bus, int index, Service *service)
        : Characteristic(bus, index, SCREENSH_REQ_UUID, {"encrypt-authenticated-write"}, service) {}

public slots:
    void WriteValue(QByteArray, QVariantMap);

signals:
    void screenshotTaken(QString);
};


class ScreenshotContentChrc : public Characteristic
{
    Q_OBJECT
public:
    ScreenshotContentChrc(QDBusConnection bus, int index, Service *service) : Characteristic(bus, index, SCREENSH_CON_UUID, {"encrypt-authenticated-read", "encrypt-authenticated-notify"}, service)
    {}

public slots:
    QByteArray ReadValue(QVariantMap)
    {
        return getValue();
    }

private slots:
    void onScreenshotTaken(QString);
};

class ScreenshotService : public Service
{
    Q_OBJECT
public:
    explicit ScreenshotService(int index, QDBusConnection bus, QObject *parent = 0);
};

#endif // SCREENSHOTSERVICE_H
