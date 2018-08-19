#ifndef ANCS_H
#define ANCS_H

#include <QObject>

class ANCS: public QObject
{
    Q_OBJECT
public slots:
    void SearchForAncsCharacteristics();
private:
    bool isMatchingCharacteristic(QString uuid, QMap<QString, QVariantMap> dbusObject);
};

#endif // ANCS_H
