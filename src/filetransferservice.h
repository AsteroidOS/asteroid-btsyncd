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

#ifndef FILETRANSFERSERVICE_H
#define FILETRANSFERSERVICE_H

#include <QByteArray>
#include <QHash>
#include <QQueue>
#include <QString>
#include <QTimer>

#include <memory>

#include "fts_engine.h"
#include "service.h"

class FileTransferService;

/* Version: read-only u32 little endian (FtsEngine::Version). */
class FtsVersionChrc : public Characteristic
{
    Q_OBJECT
public:
    FtsVersionChrc(QDBusConnection bus, int index, FileTransferService *service);

public slots:
    QByteArray ReadValue(QVariantMap options) override;
};

/* Transfer: requests arrive as Write Commands, replies leave as notifications. */
class FtsTransferChrc : public Characteristic
{
    Q_OBJECT
public:
    FtsTransferChrc(QDBusConnection bus, int index, FileTransferService *service);
    void notify(const QByteArray &chunk) { updateValue(chunk); }

public slots:
    void WriteValue(QByteArray value, QVariantMap options) override;
    void StartNotify() override;
    void StopNotify() override;

private:
    FileTransferService *m_service;
};

/*
 * GATT glue around FtsEngine (fts_engine.h holds the protocol).
 *
 * Access: a BlueZ peer that is paired and trusted, over an encrypted and
 * authenticated link; no local arming or confirmation on the watch. Replies
 * need a subscriber, so writes are ignored until notifications are enabled.
 * One peer at a time: input from another device restarts the parser, and
 * unsubscribing (which BlueZ also does on disconnect) or 30 s of silence in
 * the middle of a command drops it.
 */
class FileTransferService : public Service
{
    Q_OBJECT
public:
    explicit FileTransferService(int index, QDBusConnection bus, QObject *parent = 0);

    void handleWrite(const QByteArray &bytes, const QString &devicePath, int mtu);
    void setNotifying(bool on);

private:
    bool peerAllowed(const QString &devicePath);
    void queueMessage(const std::string &message);
    void pump();
    void drop(const char *reason);

    FtsTransferChrc *m_transfer;
    std::unique_ptr<FtsEngine> m_engine;   // null when the daemon user has no usable home
    QString m_device;
    QHash<QString, bool> m_peerVerdicts;
    QTimer m_stallTimer;
    bool m_notifying = false;
    int m_notifySize = 20;                 // ATT MTU 23 until a write reports the negotiated one
    QQueue<QByteArray> m_out;
    bool m_pumpScheduled = false;
};

#endif // FILETRANSFERSERVICE_H
