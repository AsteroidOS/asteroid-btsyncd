/*
 * Copyright (C) 2026 - Ed Beroset <beroset@ieee.org>
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

#include <QDateTime>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDebug>
#include "settime.h"

bool setSystemTime(const QDateTime& dateTime) {
    QDBusInterface interface("org.freedesktop.timedate1",
                             "/org/freedesktop/timedate1",
                             "org.freedesktop.timedate1",
                             QDBusConnection::systemBus());

    const QDateTime utcDateTime = dateTime.toUTC();

    const qint64 usec_utc{utcDateTime.toSecsSinceEpoch() * 1'000'000};

    constexpr bool isAdmin{false};
    constexpr bool adjustNTP{false};

    QDBusMessage reply = interface.call("SetTime", usec_utc, isAdmin, adjustNTP);

    if (reply.type() == QDBusMessage::ErrorMessage) {
        qWarning("Failed to set time: %s", reply.errorMessage().toLocal8Bit().constData());
        return false;
    }

    return true;
}

