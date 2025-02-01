#ifndef CTS_H
#define CTS_H
#include <QObject>
#include <QString>
#include <QMap>
#include <QVariant>
#include <QCache>
#include <QTimer>
#include <QDateTime>

#define CTS_CHARACTERISTIC_UUID "00002a2b-0000-1000-8000-00805f9b34fb"

class CTS: public QObject
{
    Q_OBJECT
    public:
        CTS();
        void searchForTimeCharacteristics();
        void disconnect() {};
    private slots:
        void TimeCharacteristicPropertiesChanged( QString interfaceName,
                                                QMap<QString, QVariant> changedProperties, QStringList invalidatedProperties);
    private:
        bool isMatchingCharacteristic(QString uuid, QMap<QString, QVariantMap> dbusObject);

};
#endif // CTS_H
