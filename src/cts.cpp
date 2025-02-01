// Implements the Current Time Service as per: https://www.bluetooth.com/specifications/specs/cts-1-1/
#include "cts.h"

#include <QDebug>
#include <QDBusInterface>
#include <QDBusArgument>
#include <QDateTime>
#include <QTimeZone>

#include <timed-qt5/wallclock>
#include <timed-qt5/interface>

#include "common.h"

CTS::CTS() {

}

void CTS::searchForTimeCharacteristics() {
    QDBusConnection bus = QDBusConnection::systemBus();
    QDBusInterface remoteOm(BLUEZ_SERVICE_NAME, "/", DBUS_OM_IFACE, bus);
    QDBusMessage result = remoteOm.call("GetManagedObjects");
    QString timeChar;
    qDebug() << "CTS searching for characteristic";
    const QDBusArgument argument = result.arguments().at(0).value<QDBusArgument>();
    if (argument.currentType() == QDBusArgument::MapType) {
        argument.beginMap();
        while (!argument.atEnd()) {
            QString key;
            QMap<QString, QVariantMap> value;

            argument.beginMapEntry();
            argument >> key >> value;
            argument.endMapEntry();
            if (isMatchingCharacteristic(CTS_CHARACTERISTIC_UUID, value)) {
                qDebug() << "Found Current Time Characteristic:" << key;
                timeChar = key;
            }

        }
        argument.endMap();
    }

    if (!timeChar.isEmpty()) {
        qDebug() << "Current Time Characteristic found";
        bus.connect(BLUEZ_SERVICE_NAME, timeChar, DBUS_PROPERTIES_IFACE, "PropertiesChanged", this,
                    SLOT(TimeCharacteristicPropertiesChanged(QString, QMap<QString, QVariant>, QStringList)));
        QDBusInterface timeCharacteristicIface("org.bluez", timeChar, GATT_CHRC_IFACE,
                                                QDBusConnection::systemBus());
        timeCharacteristicIface.call("StartNotify");
    }
}

void CTS::TimeCharacteristicPropertiesChanged(QString /* interfaceName */,
                                                       QMap<QString, QVariant> changedProperties, QStringList /* invalidatedProperties */)
{
    if (changedProperties.contains("Value")) {
        QVariant value = changedProperties["Value"];
        if (value.type() == QVariant::ByteArray) {

            QByteArray bytes = value.toByteArray();

            if(bytes.size() != 10) {
                qWarning() << "Current time value is not 10 bytes long";
                return;
            }
            ushort year = (bytes[1] << 8)  + bytes[0];
            uint8_t month = bytes[2];
            uint8_t day = bytes[3];
            uint8_t hour = bytes[4];
            uint8_t minute = bytes[5];
            uint8_t second = bytes[6];
            uint8_t day_of_week = bytes[7];
            uint8_t exact_time_256 = bytes[8];
            uint8_t adjust_reason = bytes[9];

            Maemo::Timed::WallClock::Settings s;
            QDateTime newTime(QDate(year, month, day), QTime(hour, minute, second));
            newTime.setTimeZone(QTimeZone::systemTimeZone());
            s.setTimeManual(newTime.toTime_t());

            Maemo::Timed::Interface timed;
            timed.wall_clock_settings_async(s);
        }
    }
}

bool CTS::isMatchingCharacteristic(QString uuid, QMap<QString, QVariantMap> dbusObject)
{
    if (!dbusObject.contains(GATT_CHRC_IFACE)) {
        return false;
    }
    QString charUuid = dbusObject.value(GATT_CHRC_IFACE).value("UUID").toString();
    return charUuid.toLower() == uuid.toLower();
}
