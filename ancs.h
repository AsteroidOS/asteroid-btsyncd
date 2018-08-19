#ifndef ANCS_H
#define ANCS_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QVariant>

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
    bool isMatchingCharacteristic(QString uuid, QMap<QString, QVariantMap> dbusObject);
    void appendByte(QByteArray &arr, unsigned int val);
    void append2Bytes(QByteArray &arr, unsigned int val);
    unsigned int decodeNumber(const QByteArray &arr, int offset, int length);
    int decodeStringAttribute(QByteArray bytes, int offset, int maxLenght, QString &result);
    void prepareQuery(QByteArray &result, const QByteArray &msgid);
};

#endif // ANCS_H
