/*
 * Copyright (C) 2024 AsteroidOS
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

#ifndef DEVICE_INFO_SERVICE__H
#define DEVICE_INFO_SERVICE__H

#include <QObject>
#include <QDebug>

#include <systemsettings/aboutsettings.h>

#include "service.h"


//////////////////////////////////////////////////////////////////////

class DeviceInfoModelNoChrc : public Characteristic
{
    Q_OBJECT
    Q_PROPERTY(QByteArray Value READ getValue NOTIFY valueChanged)

public:
    DeviceInfoModelNoChrc(AboutSettings* about, QDBusConnection bus, int index, Service *service);

signals:
    void valueChanged();

public slots:
    QByteArray ReadValue(QVariantMap)
    {
        qDebug() << Q_FUNC_INFO << m_value;
        return m_value;
    }

private slots:
    void emitPropertiesChanged();

private:
    QByteArray m_value;

    QByteArray getValue()
    {
        qDebug() << Q_FUNC_INFO << m_value;
        return m_value;
    }

};

//////////////////////////////////////////////////////////////////////

class DeviceInfoSerialNoChrc : public Characteristic
{
    Q_OBJECT
    Q_PROPERTY(QByteArray Value READ getValue NOTIFY valueChanged)

public:
    DeviceInfoSerialNoChrc(AboutSettings* about, QDBusConnection bus, int index, Service *service);

signals:
    void valueChanged();

public slots:
    QByteArray ReadValue(QVariantMap)
    {
        qDebug() << Q_FUNC_INFO << m_value;
        return m_value;
    }

private slots:
    void emitPropertiesChanged();

private:
    QByteArray m_value;

    QByteArray getValue()
    {
        qDebug() << Q_FUNC_INFO << m_value;
        return m_value;
    }

};

//////////////////////////////////////////////////////////////////////

class DeviceInfoFwRevisionChrc : public Characteristic
{
    Q_OBJECT
    Q_PROPERTY(QByteArray Value READ getValue NOTIFY valueChanged)

public:
    DeviceInfoFwRevisionChrc(AboutSettings* about, QDBusConnection bus, int index, Service *service);

signals:
    void valueChanged();

public slots:
    QByteArray ReadValue(QVariantMap)
    {
        qDebug() << Q_FUNC_INFO << m_value;
        return m_value;
    }

private slots:
    void emitPropertiesChanged();

private:
    QByteArray m_value;

    QByteArray getValue()
    {
        qDebug() << Q_FUNC_INFO << m_value;
        return m_value;
    }

};
//////////////////////////////////////////////////////////////////////

class DeviceInfoHardwareRevChrc : public Characteristic
{
    Q_OBJECT
    Q_PROPERTY(QByteArray Value READ getValue NOTIFY valueChanged)

public:
    DeviceInfoHardwareRevChrc(AboutSettings* about, QDBusConnection bus, int index, Service *service);

signals:
    void valueChanged();

public slots:
    QByteArray ReadValue(QVariantMap)
    {
        qDebug() << Q_FUNC_INFO << m_value;
        return m_value;
    }

private slots:
    void emitPropertiesChanged();

private:
    QByteArray m_value;

    QByteArray getValue()
    {
        qDebug() << Q_FUNC_INFO << m_value;
        return m_value;
    }

};
//////////////////////////////////////////////////////////////////////

class DeviceInfoSoftwareRevChrc : public Characteristic
{
    Q_OBJECT
    Q_PROPERTY(QByteArray Value READ getValue NOTIFY valueChanged)

public:
    DeviceInfoSoftwareRevChrc(AboutSettings* about, QDBusConnection bus, int index, Service *service);

signals:
    void valueChanged();

public slots:
    QByteArray ReadValue(QVariantMap)
    {
        qDebug() << Q_FUNC_INFO << m_value;
        return m_value;
    }

private slots:
    void emitPropertiesChanged();

private:
    QByteArray m_value;

    QByteArray getValue()
    {
        qDebug() << Q_FUNC_INFO << m_value;
        return m_value;
    }

};
//////////////////////////////////////////////////////////////////////

class DeviceInfoManufacturerChrc : public Characteristic
{
    Q_OBJECT
    Q_PROPERTY(QByteArray Value READ getValue NOTIFY valueChanged)

public:
    DeviceInfoManufacturerChrc(AboutSettings* about, QDBusConnection bus, int index, Service *service);

signals:
    void valueChanged();

public slots:
    QByteArray ReadValue(QVariantMap)
    {
        qDebug() << Q_FUNC_INFO << m_value;
        return m_value;
    }

private slots:
    void emitPropertiesChanged();

private:
    QByteArray m_value;

    QByteArray getValue()
    {
        qDebug() << Q_FUNC_INFO << m_value;
        return m_value;
    }

};
//////////////////////////////////////////////////////////////////////

class DeviceInfoSystemIdChrc : public Characteristic
{
    Q_OBJECT
    Q_PROPERTY(QByteArray Value READ getValue NOTIFY valueChanged)

public:
    DeviceInfoSystemIdChrc(AboutSettings* about, QDBusConnection bus, int index, Service *service);

signals:
    void valueChanged();

public slots:
    QByteArray ReadValue(QVariantMap)
    {
        qDebug() << Q_FUNC_INFO << m_value;
        return m_value;
    }

private slots:
    void emitPropertiesChanged();

private:
    QByteArray m_value;

    QByteArray getValue()
    {
        qDebug() << Q_FUNC_INFO << m_value;
        return m_value;
    }

};

//////////////////////////////////////////////////////////////////////

class DeviceInfoService : public Service
{
    Q_OBJECT
public:
    explicit DeviceInfoService(int index, QDBusConnection bus, QObject *parent = 0);

private:
    AboutSettings *m_about;
};

#endif // DEVICE_INFO_SERVICE__H

