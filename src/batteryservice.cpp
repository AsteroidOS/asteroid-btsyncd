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

#include "batteryservice.h"

#include <QDebug>

#include "batterystatus.h"
#include "characteristic.h"
#include "common.h"

BatteryLvlChrc::BatteryLvlChrc(QDBusConnection bus, int index, Service *service) : Characteristic(bus, index, BATTERY_LVL_UUID, {"encrypt-authenticated-read", "encrypt-authenticated-notify"}, service)
{
    m_battery = new BatteryStatus(this);
    connect(m_battery, &BatteryStatus::chargePercentageChanged,
            this, &BatteryLvlChrc::onBatteryPercentageChanged);

    updateValue(QByteArray(1, 100));
}

void BatteryLvlChrc::onBatteryPercentageChanged(int percentage)
{
    if (percentage >= 0)
        updateValue(QByteArray(1, percentage));
}

BatteryService::BatteryService(int index, QDBusConnection bus, QObject *parent) : Service(bus, index, BATTERY_UUID, parent)
{
    addCharacteristic(new BatteryLvlChrc(bus, 0, this));
}
