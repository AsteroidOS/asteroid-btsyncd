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

#include "agent.h"

Agent::Agent(QDBusConnection bus, QObject *parent) : QObject(parent)
{
    mPath = AGENT_PATH_BASE;
    bus.registerObject(mPath, this, QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllProperties);
}

QDBusObjectPath Agent::getPath()
{
    return QDBusObjectPath(mPath);
}

/* Exposed slots */
void Agent::Release()
{
}

QString Agent::RequestPinCode(QDBusObjectPath object)
{
}

void Agent::DisplayPinCode(QDBusObjectPath object, QString pinCode)
{
}

quint32 Agent::RequestPasskey(QDBusObjectPath object)
{
}

void Agent::DisplayPasskey(QDBusObjectPath object, quint32 passkey, quint16 entered)
{
}

void Agent::RequestConfirmation(QDBusObjectPath object, quint32 passKey)
{
}

void Agent::RequestAuthorization(QDBusObjectPath object)
{
}

void Agent::AuthorizeService(QDBusObjectPath object, QString uuid)
{
}

void Agent::Cancel()
{
}
