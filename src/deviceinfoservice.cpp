/*
 * Copyright (C) 2024 - AsteroidOS
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

#include <QDBusInterface>
#include <QDBusMessage>
#include <QDebug>
#include <QFile>

#include <systemsettings/aboutsettings.h>

#include "deviceinfoservice.h"
#include "characteristic.h"
#include "common.h"

////////////////////////////////////////////////////////////////////////////////

DeviceInfoModelNoChrc::DeviceInfoModelNoChrc(AboutSettings* about, QDBusConnection bus, int index, Service *service) : Characteristic(bus, index, INFO_MODEL_NO_UUID, {"encrypt-authenticated-read"}, service) {

    m_value = about->baseOperatingSystemName().toLocal8Bit(); // for example "bass"

    connect(this, SIGNAL(valueChanged()), this, SLOT(emitPropertiesChanged()));

}

void DeviceInfoModelNoChrc::emitPropertiesChanged()
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

////////////////////////////////////////////////////////////////////////////////

DeviceInfoSerialNoChrc::DeviceInfoSerialNoChrc(AboutSettings* about, QDBusConnection bus, int index, Service *service) : Characteristic(bus, index, INFO_SERIAL_NO_UUID, {"encrypt-authenticated-read"}, service) {

    m_value = about->serial().toLocal8Bit(); // for example "504KPED0068620"

    connect(this, SIGNAL(valueChanged()), this, SLOT(emitPropertiesChanged()));
    emit valueChanged();

}

void DeviceInfoSerialNoChrc::emitPropertiesChanged()
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
////////////////////////////////////////////////////////////////////////////////

DeviceInfoFwRevisionChrc::DeviceInfoFwRevisionChrc(AboutSettings* about, QDBusConnection bus, int index, Service *service) : Characteristic(bus, index, INFO_FW_REVISION_UUID, {"encrypt-authenticated-read"}, service) {

    m_value = about->adaptationVersion().toLocal8Bit();

    connect(this, SIGNAL(valueChanged()), this, SLOT(emitPropertiesChanged()));

    emit valueChanged();

}

void DeviceInfoFwRevisionChrc::emitPropertiesChanged()
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

////////////////////////////////////////////////////////////////////////////////

DeviceInfoHardwareRevChrc::DeviceInfoHardwareRevChrc(AboutSettings* about, QDBusConnection bus, int index, Service *service) : Characteristic(bus, index, INFO_HARDWARE_REV_UUID, {"encrypt-authenticated-read"}, service) {

    m_value = about->vendorVersion().toLocal8Bit(); // for example "LG Watch Urbane"

    connect(this, SIGNAL(valueChanged()), this, SLOT(emitPropertiesChanged()));

}

void DeviceInfoHardwareRevChrc::emitPropertiesChanged()
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
////////////////////////////////////////////////////////////////////////////////

DeviceInfoManufacturerChrc::DeviceInfoManufacturerChrc(AboutSettings* about, QDBusConnection bus, int index, Service *service) : Characteristic(bus, index, INFO_MANUFACTURER_UUID, {"encrypt-authenticated-read"}, service) {

    m_value = about->vendorName().toLocal8Bit();

    connect(this, SIGNAL(valueChanged()), this, SLOT(emitPropertiesChanged()));
}

void DeviceInfoManufacturerChrc::emitPropertiesChanged()
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
////////////////////////////////////////////////////////////////////////////////

DeviceInfoSoftwareRevChrc::DeviceInfoSoftwareRevChrc(AboutSettings* about, QDBusConnection bus, int index, Service *service) : Characteristic(bus, index, INFO_SOFTWARE_REV_UUID, {"encrypt-authenticated-read"}, service) {

    m_value = about->softwareVersion().toLocal8Bit();

    connect(this, SIGNAL(valueChanged()), this, SLOT(emitPropertiesChanged()));
}

void DeviceInfoSoftwareRevChrc::emitPropertiesChanged()
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
////////////////////////////////////////////////////////////////////////////////

DeviceInfoSystemIdChrc::DeviceInfoSystemIdChrc(AboutSettings* about, QDBusConnection bus, int index, Service *service) : Characteristic(bus, index, INFO_SYSTEM_ID_UUID, {"encrypt-authenticated-read"}, service) {

    m_value = about->operatingSystemName().toLocal8Bit(); // it will be "AsteroidOS"

    connect(this, SIGNAL(valueChanged()), this, SLOT(emitPropertiesChanged()));

}

void DeviceInfoSystemIdChrc::emitPropertiesChanged()
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


////////////////////////////////////////////////////////////////////////////////

DeviceInfoService::DeviceInfoService(int index, QDBusConnection bus, QObject *parent) : Service(bus, index, INFO_UUID, parent)
{

    m_about = new AboutSettings(this);

    addCharacteristic(new DeviceInfoModelNoChrc(m_about, bus, 0, this));
    addCharacteristic(new DeviceInfoSerialNoChrc(m_about, bus, 1, this));
    addCharacteristic(new DeviceInfoFwRevisionChrc(m_about, bus, 2, this));
    addCharacteristic(new DeviceInfoHardwareRevChrc(m_about, bus, 3, this));
    addCharacteristic(new DeviceInfoSoftwareRevChrc(m_about, bus, 4, this));
    addCharacteristic(new DeviceInfoManufacturerChrc(m_about, bus, 5, this));
    addCharacteristic(new DeviceInfoSystemIdChrc(m_about, bus, 6, this));

}
