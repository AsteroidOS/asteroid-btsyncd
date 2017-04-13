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

#ifndef BLUEZMANAGER_H
#define BLUEZMANAGER_H

#include <QObject>
#include <QDBusObjectPath>
#include <QDBusServiceWatcher>
#include <QDBusConnection>

typedef QMap<QString, QMap<QString, QVariant>> InterfaceList;

class BlueZManager : public QObject
{
    Q_OBJECT
public:
    explicit BlueZManager(QDBusObjectPath appPath, QDBusObjectPath advertPath, QObject *parent = 0);
    void updateConnected();

private:
    bool mConnected;
    QDBusObjectPath mAppPath, mAdvertPath;
    QString mAdapter, mConnectedDevice;
    QDBusServiceWatcher *mWatcher;
    QDBusConnection mBus;

    void updateAdapter();

signals:
    void connectedChanged();
    void adapterChanged();

public slots:
    void serviceRegistered(const QString& name);
    void serviceUnregistered(const QString& name);
    void InterfacesAdded(QDBusObjectPath, InterfaceList);
    void InterfacesRemoved(QDBusObjectPath, QStringList);
    void PropertiesChanged(QString, QMap<QString, QVariant>, QStringList);

    void onConnectedChanged();
    void onAdapterChanged();
};

#endif // BLUEZMANAGER_H
