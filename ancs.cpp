#include "ancs.h"

#include <QDebug>
#include <QDBusInterface>
#include <QDBusArgument>

#include "common.h"
#include "ancs_protocol_constants.h"

bool ANCS::isMatchingCharacteristic(QString uuid, QMap<QString, QVariantMap> dbusObject)
{
    if (!dbusObject.contains(GATT_CHRC_IFACE)) {
        return false;
    }
    QString charUuid = dbusObject.value(GATT_CHRC_IFACE).value("UUID").toString();
    return charUuid.toLower() == uuid.toLower();
}

void ANCS::SearchForAncsCharacteristics()
{
    QDBusConnection bus = QDBusConnection::systemBus();
    QDBusInterface remoteOm(BLUEZ_SERVICE_NAME, "/", DBUS_OM_IFACE, bus);
    QDBusMessage result = remoteOm.call("GetManagedObjects");
    QString notificationChar;
    QString controlChar;
    QString dataChar;
    const QDBusArgument argument = result.arguments().at(0).value<QDBusArgument>();
    if (argument.currentType() == QDBusArgument::MapType) {
        argument.beginMap();
        while (!argument.atEnd()) {
            QString key;
            QMap<QString, QVariantMap> value;

            argument.beginMapEntry();
            argument >> key >> value;
            argument.endMapEntry();
            if (isMatchingCharacteristic(ANCS_NOTIFICATION_SOURCE_CHARACTERISTIC_UUID, value)) {
                qDebug() << "Found ANCS notification source characteristic:" << key;
                notificationChar = key;
            }
            if (isMatchingCharacteristic(ANCS_CONTROL_POINT_CHARACTERISTIC_UUID, value)) {
                qDebug() << "Found ANCS control point characteristic:" << key;
                controlChar = key;
            }
            if (isMatchingCharacteristic(ANCS_DATA_SOURCE_CHARACTERISTIC_UUID, value)) {
                qDebug() << "Found ANCS data source characteristic:" << key;
                dataChar = key;
            }
        }
        argument.endMap();
    }
    if (!notificationChar.isEmpty() && !controlChar.isEmpty() && !dataChar.isEmpty()) {
        qDebug() << "All ANCS characteristics found";
    }
}
