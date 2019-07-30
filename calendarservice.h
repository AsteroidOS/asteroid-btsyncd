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

#ifndef CALENDARSERVICE_H
#define CALENDARSERVICE_H

#include <QObject>

#include "service.h"

class CalendarWriteChrc : public Characteristic
{
    Q_OBJECT
public:
    CalendarWriteChrc(QDBusConnection bus, int index, Service *service)
        : Characteristic(bus, index, CALENDAR_WRT_UUID, {"encrypt-authenticated-write"}, service) {}

public slots:
    void WriteValue(QByteArray, QVariantMap);
};

class CalendarService : public Service
{
    Q_OBJECT
public:
    explicit CalendarService(int index, QDBusConnection bus, QObject *parent = 0);
};

#endif // CALENDARSERVICE_H
