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

#include <contextproperty.h>

#include <QTimer>
#include <QDBusMessage>
#include <QDebug>

#include "batteryservice.h"
#include "characteristic.h"
#include "common.h"

BatteryLvlChrc::BatteryLvlChrc(QDBusConnection bus, int index, Service *service) : Characteristic(bus, index, BATTERY_LVL_UUID, {"read", "notify"}, service)
{
    m_battery = new ContextProperty("Battery.ChargePercentage", this);
    connect(m_battery, SIGNAL(valueChanged()), this, SLOT(onBatteryPercentageChanged()));
    connect(this, SIGNAL(valueChanged()), this, SLOT(emitPropertiesChanged()));
    m_value = QByteArray::number(100);
    QTimer::singleShot(0, this, SLOT(onBatteryPercentageChanged()));
}

void BatteryLvlChrc::onBatteryPercentageChanged()
{
    m_value = QByteArray::number(m_battery->value().toUInt());

    emit valueChanged();
}

void BatteryLvlChrc::emitPropertiesChanged()
{
    QDBusConnection connection = QDBusConnection::systemBus();
    QDBusMessage message = QDBusMessage::createSignal(getPath().path(),
                                                      "org.freedesktop.DBus.Properties",
                                                      "PropertiesChanged");

    QVariantMap changedProperties;
    changedProperties[QStringLiteral("Value")] = QVariant(m_value);

    QList<QVariant> arguments;
    arguments << QVariant(GATT_CHRC_IFACE) << QVariant(changedProperties) << QVariant(QStringList());
    message.setArguments(arguments);

    if (!connection.send(message))
        qDebug() << "Failed to send DBus property notification signal";
}

BatteryService::BatteryService(int index, QDBusConnection bus, QObject *parent) : Service(bus, index, BATTERY_UUID, parent)
{
    addCharacteristic(new BatteryLvlChrc(bus, 0, this));
}
