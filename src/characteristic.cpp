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

#include "characteristic.h"
#include "common.h"
#include "service.h"

#include <QDebug>
#include <QDBusMessage>

Characteristic::Characteristic(QDBusConnection bus, unsigned int index, QString uuid, QStringList flags, Service *service, QObject *parent) : QObject(parent), mBus(QDBusConnection::systemBus())
{
    mPath = service->getPath().path() + "/char" + QString::number(index);

    mBus = bus;
    mUuid = uuid;
    mFlags = flags;
    mService = service;
    mNotifying = false;

    bus.registerObject(mPath, this, QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllProperties);

    connect(this, &Characteristic::valueChanged, this, [this] { emitPropertiesChanged(mValue); });
}

QDBusObjectPath Characteristic::getPath()
{
    return QDBusObjectPath(mPath);
}

void Characteristic::addDescriptor(Descriptor *descriptor)
{
    mDescriptors.append(descriptor);
}

QList<QDBusObjectPath> Characteristic::getDescriptorPaths()
{
    QList<QDBusObjectPath> result;
    foreach(Descriptor *desc, mDescriptors)
        result.append(desc->getPath());
    return result;
}

QList<Descriptor *> Characteristic::getDescriptors()
{
    return mDescriptors;
}

QDBusObjectPath Characteristic::getService()
{
    return mService->getPath();
}

QString Characteristic::getUuid()
{
    return mUuid;
}

QStringList Characteristic::getFlags()
{
    return mFlags;
}

bool Characteristic::getNotifying()
{
    return mNotifying;
}

void Characteristic::updateValue(const QByteArray &value)
{
    mValue = value;
    emit valueChanged();
}

void Characteristic::emitNotifyingChange()
{
    QDBusMessage message = QDBusMessage::createSignal(getPath().path(),
                                                      "org.freedesktop.DBus.Properties",
                                                      "PropertiesChanged");

    QVariantMap changedProperties;
    changedProperties.insert(QStringLiteral("Notifying"), QVariant(mNotifying));

    QList<QVariant> arguments;
    arguments << QVariant(GATT_CHRC_IFACE) << QVariant(changedProperties) << QVariant(QStringList());
    message.setArguments(arguments);

    if (!mBus.send(message))
        qWarning() << "Failed to send Notifying property change for" << mUuid;
}

bool Characteristic::hasMinLength(const QByteArray &value, int minBytes) const
{
    if (value.size() < minBytes) {
        qWarning() << "Rejecting GATT write to" << mUuid << ": got" << value.size()
                   << "bytes, expected at least" << minBytes;
        return false;
    }
    return true;
}

void Characteristic::emitPropertiesChanged(const QByteArray &value)
{
    QDBusMessage message = QDBusMessage::createSignal(getPath().path(),
                                                      "org.freedesktop.DBus.Properties",
                                                      "PropertiesChanged");

    QVariantMap changedProperties;
    changedProperties.insert(QStringLiteral("Value"), QVariant(value));

    QList<QVariant> arguments;
    arguments << QVariant(GATT_CHRC_IFACE) << QVariant(changedProperties) << QVariant(QStringList());
    message.setArguments(arguments);

    if (!mBus.send(message))
        qWarning() << "Failed to send DBus property notification signal for" << mUuid;
}

/* Exposed slots */

QByteArray Characteristic::ReadValue(QVariantMap)
{
    qWarning() << "Default ReadValue called on" << mUuid;
    return mValue;
}

void Characteristic::WriteValue(QByteArray, QVariantMap)
{
    qWarning() << "Default WriteValue called on" << mUuid;
}

void Characteristic::StartNotify()
{
    if (mNotifying)
        return;
    mNotifying = true;
    emit notifyingChanged();
    emit notifyEnabled();
    emitNotifyingChange();
}

void Characteristic::StopNotify()
{
    if (!mNotifying)
        return;
    mNotifying = false;
    emit notifyingChanged();
    emit notifyDisabled();
    emitNotifyingChange();
}
