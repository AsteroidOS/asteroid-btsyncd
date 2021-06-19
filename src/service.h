/*
 * Copyright (C) 2016 - Florent Revest <revestflo@gmail.com>
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

#ifndef SERVICE_H
#define SERVICE_H

#include <QObject>
#include <QDBusConnection>
#include <QDBusObjectPath>
#include <QDBusAbstractAdaptor>
#include <QList>
#include <QString>

#include "characteristic.h"
#include "common.h"

class Service : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", GATT_SERVICE_IFACE)
    Q_PROPERTY(QString UUID READ getUuid())
    Q_PROPERTY(bool Primary READ getPrimary())
    Q_PROPERTY(QList<QDBusObjectPath>  Characteristics READ getCharacteristicPaths())

public:
    explicit Service(QDBusConnection bus, unsigned int index, QString uuid, QObject *parent = 0);
    QDBusObjectPath getPath();
    void addCharacteristic(Characteristic *charac);
    QList<QDBusObjectPath> getCharacteristicPaths();
    QList<Characteristic *> getCharacteristics();

    QString getUuid();
    bool getPrimary();

private:
    QDBusConnection mBus;
    QString mPath, mUuid;
    QList<Characteristic *> mCharacteristics;
};

#endif // SERVICE_H
