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

#ifndef ADVERTISEMENT_H
#define ADVERTISEMENT_H

#include <QObject>
#include <QDBusAbstractAdaptor>
#include <QDBusObjectPath>
#include <QDBusConnection>

#include "common.h"

class Advertisement : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", LE_ADVERTISEMENT_IFACE)
    Q_PROPERTY(QString Type READ getType())
    Q_PROPERTY(QStringList ServiceUUIDs READ getServiceUUIDs())
    Q_PROPERTY(QStringList SolicitUUIDs READ getSolicitUUIDs())
    Q_PROPERTY(QMap<unsigned int, QByteArray> ManufacturerData READ getManufacturerData())
    Q_PROPERTY(QMap<QString, QByteArray> ServiceData READ getServiceData())
    Q_PROPERTY(bool IncludeTxPower READ getIncludeTxPower())
    Q_PROPERTY(bool Discoverable READ getDiscoverable())

public:
    explicit Advertisement(QDBusConnection bus = QDBusConnection::systemBus(), QObject *parent = 0);
    QDBusObjectPath getPath();

private:
    void addServiceUuid(QString uuid);
    void addSolicitUuid(QString uuid);
    void addServiceData(QString uuid, QByteArray data);
    void addManufacturerData(unsigned int manufCode, QByteArray data);

    QString getType();
    QStringList getServiceUUIDs();
    QStringList getSolicitUUIDs();
    QMap<unsigned int, QByteArray> getManufacturerData();
    QMap<QString, QByteArray> getServiceData();
    bool getIncludeTxPower();
    bool getDiscoverable();

    QDBusConnection mBus;
    QString mPath, mAdType;
    QStringList mServiceUuids, mSolicitUuids;
    QMap<unsigned int, QByteArray> mManufacturerData;
    QMap<QString, QByteArray> mServiceData;
    bool mIncludeTxPower;
    bool mDiscoverable;

public slots:
    void Release();
};

#endif // ADVERTISEMENT_H
