/*
 * Copyright (C) 2026 AsteroidOS
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "findphoneservice.h"
#include <QDebug>

FindPhoneRingChrc::FindPhoneRingChrc(QDBusConnection bus, int index, Service *service)
    : Characteristic(bus, index, FINDPHONE_RING_UUID,
                     {"read", "notify", "encrypt-authenticated-read", "encrypt-authenticated-notify"},
                     service)
{
    m_value.append((char)0);
}

void FindPhoneRingChrc::setRinging(bool ring)
{
    char val = ring ? 1 : 0;
    if (m_value.isEmpty() || m_value.at(0) != val) {
        m_value.clear();
        m_value.append(val);
        emit valueChanged();
        emitPropertiesChanged();
        qDebug() << "FindPhone: Ringing state set to" << (ring ? "ON" : "OFF");
    }
}

void FindPhoneRingChrc::emitPropertiesChanged()
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

    if (!connection.send(message)) {
        qDebug() << "FindPhone: Failed to send DBus property notification signal";
    }
}

FindPhoneService::FindPhoneService(int index, QDBusConnection bus, QObject *parent)
    : Service(bus, index, FINDPHONE_UUID, parent)
{
    m_ringChrc = new FindPhoneRingChrc(bus, 0, this);
    addCharacteristic(m_ringChrc);

    const QString path = QStringLiteral("/org/asteroidos/btsyncd/findphone");
    const auto options = QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllProperties;
    auto sessionBus = QDBusConnection::sessionBus();
    if (!sessionBus.registerObject(path, this, options) || !sessionBus.registerService(SERVICE_NAME)) {
        qWarning() << "Cannot publish Find Phone on the session bus:" << sessionBus.lastError().message();
    }

    bus.registerObject(path, this, options);
}

void FindPhoneService::ringPhone(bool ring)
{
    if (m_ringChrc) {
        m_ringChrc->setRinging(ring);
    }
}

void FindPhoneService::toggleRing()
{
    if (m_ringChrc) {
        m_ringChrc->setRinging(!m_ringChrc->isRinging());
    }
}

bool FindPhoneService::isRinging()
{
    return m_ringChrc ? m_ringChrc->isRinging() : false;
}
