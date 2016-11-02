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

#ifndef BATTERYSERVICE_H
#define BATTERYSERVICE_H

#include <QObject>

#include "service.h"

class ContextProperty;

class BatteryLvlChrc : public Characteristic
{
    Q_OBJECT
    Q_PROPERTY(QByteArray Value READ getValue NOTIFY valueChanged)
public:
    BatteryLvlChrc(QDBusConnection bus, int index, Service *service);

public slots:
    QByteArray ReadValue(QVariantMap)
    {
        return m_value;
    }

    void StartNotify() {}
    void StopNotify() {}

private slots:
    void emitPropertiesChanged();
    void onBatteryPercentageChanged();

signals:
    void valueChanged();

private:
    ContextProperty *m_battery;
    QByteArray m_value;

    QByteArray getValue()
    {
        return m_value;
    }
};

class BatteryService : public Service
{
    Q_OBJECT
public:
    explicit BatteryService(int index, QDBusConnection bus, QObject *parent = 0);
};

#endif // BATTERYSERVICE_H
