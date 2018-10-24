#ifndef ANCS_H
#define ANCS_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QVariant>
#include <QCache>

#include "ancs_notification.h"

class ANCS: public QObject
{
    Q_OBJECT
public:
    ANCS();
    void searchForAncsCharacteristics();
    void clearNotifications();
private slots:
    void NotificationCharacteristicPropertiesChanged( QString interfaceName,
                                                      QMap<QString, QVariant> changedProperties, QStringList invalidatedProperties);
    void DataCharacteristicPropertiesChanged( QString interfaceName,
                                              QMap<QString, QVariant> changedProperties, QStringList invalidatedProperties);
private:
    QString controlCharacteristic;
    QCache<unsigned int, ANCSNotification> notificationCache;
    bool isMatchingCharacteristic(QString uuid, QMap<QString, QVariantMap> dbusObject);
    void appendByte(QByteArray &arr, unsigned int val);
    void append2Bytes(QByteArray &arr, unsigned int val);
    unsigned int decodeNumber(const QByteArray &arr, int offset, int length);
    int decodeStringAttribute(QByteArray bytes, int offset, int maxLenght, QString &result);
    void prepareQuery(QByteArray &result, const QByteArray &msgid);
    bool validateGetNotificationAttributesResponse(const QByteArray &bytes);
    void handleGetNotificationAttributesResponse(const QByteArray &bytes);

};

#endif // ANCS_H
