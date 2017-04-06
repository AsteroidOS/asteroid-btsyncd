/*
 * Copyright (C) 2017 - Florent Revest <revestflo@gmail.com>
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

#ifndef AGENT_H
#define AGENT_H

#include <QObject>
#include <QDBusConnection>
#include <QDBusObjectPath>

#include "common.h"

class Agent : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", DBUS_OM_IFACE)

public:
    Agent(QDBusConnection bus = QDBusConnection::systemBus(), QObject *parent = 0);
    QDBusObjectPath getPath();

private:
    QString mPath;

public slots:
    void Release();
    QString RequestPinCode(QDBusObjectPath object);
    void DisplayPinCode(QDBusObjectPath object, QString pinCode);
    quint32 RequestPasskey(QDBusObjectPath object);
    void DisplayPasskey(QDBusObjectPath object, quint32 passkey, quint16 entered);
    void RequestConfirmation(QDBusObjectPath object, quint32 passKey);
    void RequestAuthorization(QDBusObjectPath object);
    void AuthorizeService(QDBusObjectPath object, QString uuid);
    void Cancel();
};

#endif // AGENT_H
