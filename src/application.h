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

#ifndef APPLICATION_H
#define APPLICATION_H

#include <QObject>
#include <QDBusConnection>
#include <QList>
#include <QString>

#include "service.h"
#include "common.h"

typedef QMap<QString, QVariantMap> InterfaceList;
typedef QMap<QDBusObjectPath, InterfaceList> ManagedObjectList;
Q_DECLARE_METATYPE(InterfaceList)
Q_DECLARE_METATYPE(ManagedObjectList)

class Application : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", DBUS_OM_IFACE)

public:
    Application(QDBusConnection bus = QDBusConnection::systemBus(), QObject *parent = 0);
    QDBusObjectPath getPath();
    void addService(Service *service);

private:
    QString mPath;
    QList<Service *> mServices;

public slots:
    ManagedObjectList GetManagedObjects();
};

#endif // APPLICATION_H
