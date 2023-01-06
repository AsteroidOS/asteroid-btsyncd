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
#include <QDateTime>

#include <timed-qt5/wallclock>
#include <timed-qt5/interface>

#include "timeservice.h"
#include "characteristic.h"
#include "common.h"

void TimeSetChrc::WriteValue(QByteArray value, QVariantMap)
{
    int year = 1900 + (unsigned char) value[0];
    int month = 1+ (unsigned char) value[1];
    int day = (unsigned char) value[2];
    int hour = (unsigned char) value[3];
    int minute = (unsigned char) value[4];
    int second = (unsigned char) value[5];

    Maemo::Timed::WallClock::Settings s;
    QDateTime newTime(QDate(year, month, day), QTime(hour, minute, second));
    s.setTimeManual(newTime.toTime_t());

    Maemo::Timed::Interface timed;
    timed.wall_clock_settings_async(s);
}

TimeService::TimeService(int index, QDBusConnection bus, QObject *parent) : Service(bus, index, TIME_UUID, parent)
{
    addCharacteristic(new TimeSetChrc(bus, 0, this));
}
