/*
 * Adafruit BLE File Transfer service (0xFEBB, protocol v4) rooted at the
 * daemon user's home directory.
 *
 * Copyright (C) 2026 - Martin Monperrus <martin.monperrus@gnieh.org>
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

#include "filetransferservice.h"

#include <pwd.h>
#include <unistd.h>

#include <QDBusInterface>
#include <QDBusObjectPath>
#include <QDebug>
#include <QtEndian>

#include "common.h"

/* The GATT attribute value limit; BlueZ refuses longer notifications. */
static const int MAX_NOTIFY_SIZE = 512;
/* Notifications sent per event-loop turn, then yield to the other services. */
static const int NOTIFY_BURST = 8;
/* A command left half-received this long is dropped. */
static const int STALL_MS = 30000;

FtsVersionChrc::FtsVersionChrc(QDBusConnection bus, int index, FileTransferService *service)
    : Characteristic(bus, index, FTS_VERSION_UUID, {"encrypt-authenticated-read"}, service)
{
}

QByteArray FtsVersionChrc::ReadValue(QVariantMap)
{
    QByteArray value(4, '\0');
    qToLittleEndian<quint32>(FtsEngine::Version, value.data());
    return value;
}

FtsTransferChrc::FtsTransferChrc(QDBusConnection bus, int index, FileTransferService *service)
    : Characteristic(bus, index, FTS_TRANSFER_UUID,
                     {"encrypt-authenticated-write", "write-without-response", "encrypt-authenticated-notify"},
                     service),
      m_service(service)
{
}

void FtsTransferChrc::WriteValue(QByteArray value, QVariantMap options)
{
    // A byte stream, like NUS: prepared/offset writes have no meaning here.
    if (options.value("offset", 0).toInt() != 0 || options.value("prepare-authorize", false).toBool()) {
        qWarning() << "File transfer: rejecting offset/prepared write";
        return;
    }
    const QVariant deviceOption = options.value("device");
    QString device = qvariant_cast<QDBusObjectPath>(deviceOption).path();
    if (device.isEmpty())
        device = deviceOption.toString();
    m_service->handleWrite(value, device, options.value("mtu", 0).toInt());
}

void FtsTransferChrc::StartNotify()
{
    Characteristic::StartNotify();
    m_service->setNotifying(true);
}

void FtsTransferChrc::StopNotify()
{
    Characteristic::StopNotify();
    m_service->setNotifying(false);
}

FileTransferService::FileTransferService(int index, QDBusConnection bus, QObject *parent)
    : Service(bus, index, FTS_UUID, parent)
{
    addCharacteristic(new FtsVersionChrc(bus, 0, this));
    m_transfer = new FtsTransferChrc(bus, 1, this);
    addCharacteristic(m_transfer);

    // Fail closed: serve only a non-root daemon user's home directory.
    const struct passwd *pw = getpwuid(getuid());
    if (getuid() == 0 || !pw || !pw->pw_dir || access(pw->pw_dir, R_OK | X_OK) != 0) {
        qWarning() << "File transfer: disabled, no usable non-root home directory";
    } else {
        m_engine.reset(new FtsEngine(pw->pw_dir));
        m_engine->onMessage = [this](const std::string &message) { queueMessage(message); };
    }

    m_stallTimer.setSingleShot(true);
    m_stallTimer.setInterval(STALL_MS);
    connect(&m_stallTimer, &QTimer::timeout, this, [this] {
        if (m_engine && m_engine->midCommand())
            drop("command stalled");
    });
}

void FileTransferService::drop(const char *reason)
{
    if (m_engine && m_engine->midCommand())
        qInfo() << "File transfer: dropping the current command (" << reason << ")";
    if (m_engine)
        m_engine->reset();
    m_out.clear();
    m_stallTimer.stop();
}

void FileTransferService::setNotifying(bool on)
{
    m_notifying = on;
    if (!on) {
        drop("unsubscribed");
        m_device.clear();
        m_peerVerdicts.clear();  // trust is re-read on the next connection
    }
}

bool FileTransferService::peerAllowed(const QString &devicePath)
{
    if (devicePath.isEmpty())
        return false;
    auto cached = m_peerVerdicts.constFind(devicePath);
    if (cached != m_peerVerdicts.constEnd())
        return cached.value();

    QDBusInterface device(BLUEZ_SERVICE_NAME, devicePath, DEVICE_MANAGER_IFACE, QDBusConnection::systemBus());
    const bool paired = device.isValid() && device.property("Paired").toBool();
    const bool trusted = device.isValid() && device.property("Trusted").toBool();
    if (!paired || !trusted)
        qWarning() << "File transfer: rejecting peer" << devicePath << "paired=" << paired << "trusted=" << trusted;
    m_peerVerdicts.insert(devicePath, paired && trusted);
    return paired && trusted;
}

void FileTransferService::handleWrite(const QByteArray &bytes, const QString &devicePath, int mtu)
{
    if (!m_engine || !m_notifying || !peerAllowed(devicePath))
        return;
    if (devicePath != m_device) {
        drop("new peer");
        m_device = devicePath;
    }
    if (mtu > 3)
        m_notifySize = qBound(20, mtu - 3, MAX_NOTIFY_SIZE);
    m_engine->feed(reinterpret_cast<const uint8_t *>(bytes.constData()), size_t(bytes.size()));
    if (m_engine->midCommand())
        m_stallTimer.start();
    else
        m_stallTimer.stop();
}

void FileTransferService::queueMessage(const std::string &message)
{
    // Each message starts a new notification: Adafruit's client expects every
    // reply header at the start of one.
    for (size_t at = 0; at < message.size(); at += size_t(m_notifySize))
        m_out.enqueue(QByteArray(message.data() + at, int(std::min(message.size() - at, size_t(m_notifySize)))));
    if (!m_pumpScheduled) {
        m_pumpScheduled = true;
        QTimer::singleShot(0, this, [this] { pump(); });
    }
}

void FileTransferService::pump()
{
    m_pumpScheduled = false;
    for (int i = 0; i < NOTIFY_BURST && !m_out.isEmpty() && m_notifying; i++)
        m_transfer->notify(m_out.dequeue());
    if (!m_out.isEmpty() && m_notifying) {
        m_pumpScheduled = true;
        QTimer::singleShot(0, this, [this] { pump(); });
    }
}
