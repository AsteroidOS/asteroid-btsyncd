/*
 * Copyright (C) 2026 AsteroidOS
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef FINDPHONESERVICE_H
#define FINDPHONESERVICE_H

#include <QObject>
#include <QDBusConnection>
#include <QDBusMessage>
#include "service.h"
#include "characteristic.h"
#include "common.h"

class FindPhoneRingChrc : public Characteristic
{
    Q_OBJECT
    Q_PROPERTY(QByteArray Value READ getValue NOTIFY valueChanged)
public:
    FindPhoneRingChrc(QDBusConnection bus, int index, Service *service);

public slots:
    QByteArray ReadValue(QVariantMap) { return m_value; }
    void StartNotify() {}
    void StopNotify() {}
    void setRinging(bool ring);
    bool isRinging() const { return !m_value.isEmpty() && m_value.at(0) == 1; }

signals:
    void valueChanged();

private:
    QByteArray m_value;
    QByteArray getValue() { return m_value; }
    void emitPropertiesChanged();
};

class FindPhoneService : public Service
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.asteroidos.btsyncd.FindPhone")
public:
    explicit FindPhoneService(int index, QDBusConnection bus, QObject *parent = nullptr);

public slots:
    void ringPhone(bool ring);
    void toggleRing();
    bool isRinging();

private:
    FindPhoneRingChrc *m_ringChrc;
};

#endif // FINDPHONESERVICE_H
