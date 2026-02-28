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

