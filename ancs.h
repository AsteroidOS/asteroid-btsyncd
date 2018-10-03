#ifndef ANCS_H
#define ANCS_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QVariant>
#include <QCache>

struct ANCSMessageCacheEntry {
    unsigned int eventId;
    unsigned int eventFlags;
    unsigned int categoryId;
    unsigned int categoryCount;

};

class ANCS: public QObject
{
    Q_OBJECT
public slots:
    void SearchForAncsCharacteristics();
private slots:
    void NotificationCharacteristicPropertiesChanged( QString interfaceName,
                                                      QMap<QString, QVariant> changedProperties, QStringList invalidatedProperties);
    void DataCharacteristicPropertiesChanged( QString interfaceName,
                                              QMap<QString, QVariant> changedProperties, QStringList invalidatedProperties);
private:
    QString controlCharacteristic;
    QCache<unsigned int, ANCSMessageCacheEntry> messageCache;
    bool isMatchingCharacteristic(QString uuid, QMap<QString, QVariantMap> dbusObject);
    void appendByte(QByteArray &arr, unsigned int val);
    void append2Bytes(QByteArray &arr, unsigned int val);
    unsigned int decodeNumber(const QByteArray &arr, int offset, int length);
    int decodeStringAttribute(QByteArray bytes, int offset, int maxLenght, QString &result);
    void prepareQuery(QByteArray &result, const QByteArray &msgid);
    void sendNotification(QString title, QString message, QString icon);
    void decodeIcon(unsigned int categoryId, QString &result);
    bool validateGetNotificationAttributesResponse(const QByteArray &bytes);
    void handleGetNotificationAttributesResponse(const QByteArray &bytes);

};

#endif // ANCS_H
