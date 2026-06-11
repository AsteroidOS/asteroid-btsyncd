/*
 * Copyright (C) 2026 - Florent Revest <revestflo@gmail.com>
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

#ifndef BATTERYSTATUS_H
#define BATTERYSTATUS_H

#include <QObject>

class BatteryStatus : public QObject
{
    Q_OBJECT
public:
    explicit BatteryStatus(QObject *parent = nullptr);

    int chargePercentage() const { return m_chargePercentage; }

signals:
    void chargePercentageChanged(int percentage);

private slots:
    void onBatteryLevelInd(int percentage);

private:
    int m_chargePercentage = -1;
};

#endif // BATTERYSTATUS_H
