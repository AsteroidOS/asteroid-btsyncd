#include "ancs.h"

#include <QDebug>
#include <QDBusInterface>
#include <QDBusArgument>

#include "common.h"
#include "ancs_protocol_constants.h"

#define TITLE_MAX_LENGTH 50
#define MESSAGE_MAX_LENGTH 100
#define MAX_ANCS_NOTIFICATIONS 10

ANCS::ANCS() : notificationCache(MAX_ANCS_NOTIFICATIONS) {}

bool ANCS::isMatchingCharacteristic(QString uuid, QMap<QString, QVariantMap> dbusObject)
{
    if (!dbusObject.contains(GATT_CHRC_IFACE)) {
        return false;
    }
    QString charUuid = dbusObject.value(GATT_CHRC_IFACE).value("UUID").toString();
    return charUuid.toLower() == uuid.toLower();
}

void ANCS::searchForAncsCharacteristics()
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
        controlCharacteristic = controlChar;
        bus.connect(BLUEZ_SERVICE_NAME, notificationChar, DBUS_PROPERTIES_IFACE, "PropertiesChanged", this,
                    SLOT(NotificationCharacteristicPropertiesChanged(QString, QMap<QString, QVariant>, QStringList)));
        bus.connect(BLUEZ_SERVICE_NAME, dataChar, DBUS_PROPERTIES_IFACE, "PropertiesChanged", this,
                    SLOT(DataCharacteristicPropertiesChanged(QString, QMap<QString, QVariant>, QStringList)));
        QDBusInterface dataCharacteristicIface("org.bluez", dataChar, GATT_CHRC_IFACE,
                                               QDBusConnection::systemBus());
        QDBusInterface notificationCharacteristicIface("org.bluez", notificationChar, GATT_CHRC_IFACE,
                                                       QDBusConnection::systemBus());
        dataCharacteristicIface.call("StartNotify");
        notificationCharacteristicIface.call("StartNotify");
        qDebug() << "ANCS notifications enabled";
    }
}

void ANCS::clearNotifications()
{
    notificationCache.clear();
}

void ANCS::appendByte(QByteArray &arr, unsigned int val)
{
    arr.append(static_cast<char>(val));
}

void ANCS::append2Bytes(QByteArray &arr, unsigned int val)
{
    appendByte(arr, val);
    appendByte(arr, val >> 8);
}

unsigned int ANCS::decodeNumber(const QByteArray &arr, int offset, int length)
{
    unsigned int result = 0;
    for (int i = offset + length - 1; i >= offset; i--) {
        result *= 256;
        result += static_cast<unsigned char>(arr[i]);
    }
    return result;
}



void ANCS::prepareQuery(QByteArray &result, const QByteArray &msgId)
{
    appendByte(result, ANCS_COMMAND_ID_GET_NOTIFICATION_ATTRIBUTES);
    result.append(msgId);
    appendByte(result, ANCS_NOTIFICATION_ATTRIBUTE_ID_TITLE);
    append2Bytes(result, TITLE_MAX_LENGTH);
    appendByte(result, ANCS_NOTIFICATION_ATTRIBUTE_ID_MESSAGE);
    append2Bytes(result, MESSAGE_MAX_LENGTH);
}

void ANCS::NotificationCharacteristicPropertiesChanged(QString interfaceName,
                                                       QMap<QString, QVariant> changedProperties, QStringList invalidatedProperties)
{
    if (changedProperties.contains("Value")) {
        QVariant value = changedProperties["Value"];
        if (value.type() == QVariant::ByteArray) {
            QByteArray bytes = value.toByteArray();
            if (bytes.length() == 8) {
                unsigned int eventId = decodeNumber(bytes, 0, 1);
                if (eventId == ANCS_EVENT_ID_NOTIFICATION_ADDED || eventId == ANCS_EVENT_ID_NOTIFICATION_MODIFIED) {
                    unsigned int eventFlags = decodeNumber(bytes, 1, 1);
                    unsigned int categoryId = decodeNumber(bytes, 2, 1);
                    unsigned int categoryCount = decodeNumber(bytes, 3, 1);
                    QByteArray msgId = bytes.mid(4);
                    unsigned int msgKey = decodeNumber(msgId, 0, 4);
                    ANCSNotification *entry = notificationCache.object(msgKey);
                    if (!entry)
                        entry = new ANCSNotification;
                    if (!entry)
                        return;
                    entry->eventFlags = eventFlags;
                    entry->categoryId = categoryId;
                    notificationCache.insert(msgKey, entry);

                    QDBusInterface controlCharacteristicIface("org.bluez", controlCharacteristic, GATT_CHRC_IFACE,
                                                              QDBusConnection::systemBus());
                    QByteArray query;
                    prepareQuery(query, msgId);
                    QMap<QString, QVariant> empty;
                    controlCharacteristicIface.call("WriteValue", query, empty);
                } else if (eventId == ANCS_EVENT_ID_NOTIFICATION_REMOVED) {
                    QByteArray msgId = bytes.mid(4);
                    unsigned int msgKey = decodeNumber(msgId, 0, 4);
                    notificationCache.remove(msgKey);
                }
            }
        }
    }
}

// return number of bytes processed or -1 on failure
int ANCS::decodeStringAttribute(QByteArray bytes, int offset, int maxLenght, QString &result)
{
    if (offset + 2 > bytes.length()) {
        qDebug() << "Premature end of message, ignoring";
        qDebug() << "Message was:" << bytes.toHex();
        return -1;
    }
    int length = decodeNumber(bytes, offset, 2);
    offset += 2;
    if (length > maxLenght) {
        qDebug() << "Unexpected length" << length << "ignoring message";
        qDebug() << "Message was:" << bytes.toHex();
        return -1;
    }
    if (offset + length > bytes.length()) {
        qDebug() << "Attribute spans beyond message length, ignoring";
        qDebug() << "Message was:" << bytes.toHex();
        return -1;
    }
    result = QString::fromUtf8(bytes.mid(offset, length));
    return length + 2;
}

void ANCS::DataCharacteristicPropertiesChanged(QString interfaceName,
                                               QMap<QString, QVariant> changedProperties, QStringList invalidatedProperties)
{
    if (changedProperties.contains("Value")) {
        QVariant value = changedProperties["Value"];
        if (value.type() == QVariant::ByteArray) {
            QByteArray bytes = value.toByteArray();
            if (bytes.length() < 1) {
                qDebug() << "Malformed message, length < 0, ignoring";
                return;
            }
            unsigned int commandId = decodeNumber(bytes, 0, 1);
            if (commandId == ANCS_COMMAND_ID_GET_NOTIFICATION_ATTRIBUTES) {
                handleGetNotificationAttributesResponse(bytes);
            }
        }
    }
}

bool ANCS::validateGetNotificationAttributesResponse(const QByteArray &bytes)
{
    if (bytes.length() < 5) {
        qDebug() << "Malformed GetNotificationAttributes response, ignoring";
        qDebug() << "Message was:" << bytes.toHex();
        return false;
    }
    unsigned int commandId = decodeNumber(bytes, 0, 1);
    if (commandId != ANCS_COMMAND_ID_GET_NOTIFICATION_ATTRIBUTES) {
        qDebug() << "Expected ANCS_COMMAND_ID_GET_NOTIFICATION_ATTRIBUTES, ignoring";
        qDebug() << "Message was:" << bytes.toHex();
        return false;
    }
    return true;
}

void ANCS::handleGetNotificationAttributesResponse(const QByteArray &bytes)
{
    if (!validateGetNotificationAttributesResponse(bytes))
        return;
    QByteArray msgId = bytes.mid(1, 4);
    // skip event id and message id
    int offset = 5;
    QString title;
    QString message;
    while (offset < bytes.length()) {
        unsigned int attributeId = decodeNumber(bytes, offset, 1);
        offset += 1;
        if (attributeId == ANCS_NOTIFICATION_ATTRIBUTE_ID_TITLE) {
            int length = decodeStringAttribute(bytes, offset, TITLE_MAX_LENGTH, title);
            if (length == -1)
                return;
            offset += length;

        } else if (attributeId == ANCS_NOTIFICATION_ATTRIBUTE_ID_MESSAGE) {
            int length = decodeStringAttribute(bytes, offset, MESSAGE_MAX_LENGTH, message);
            if (length == -1)
                return;
            offset += length;
        } else {
            qDebug() << "Unknown attribute id, ignoring whole message";
            qDebug() << "Message was:" << bytes.toHex();
            return;
        }
    }
    if (offset != bytes.length()) {
        qDebug() << "Message not fully processed, ignoring";
        qDebug() << "Message was:" << bytes.toHex();
        return;
    }
    unsigned int cacheKey = decodeNumber(msgId, 0, 4);
    ANCSNotification *notification = notificationCache.object(cacheKey);
    if (!notification) {
        qWarning() << "ANCS notification not found in the cache, skipping, key:" << cacheKey;
        return;
    }
    notification->title = title;
    notification->message = message;
    notification->refresh();
}
