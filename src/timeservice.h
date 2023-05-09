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

#ifndef TIMESERVICE_H
#define TIMESERVICE_H

#include <QObject>

#include "service.h"

class TimeSetChrc : public Characteristic
{
    Q_OBJECT
public:
    TimeSetChrc(QDBusConnection bus, int index, Service *service)
        : Characteristic(bus, index, TIME_SET_UUID, {"encrypt-authenticated-write", "notify"}, service) {}

public slots:
    void WriteValue(QByteArray, QVariantMap);

signals:
    void screenshotTaken(QString);
};

class TimeService : public Service
{
    Q_OBJECT
public:
    explicit TimeService(int index, QDBusConnection bus, QObject *parent = 0);
};

#endif // TIMESERVICE_H
