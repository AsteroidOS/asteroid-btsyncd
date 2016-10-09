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

#ifndef MEDIASERVICE_H
#define MEDIASERVICE_H

#include <QObject>

#include <MprisPlayer>

#include "service.h"

#define MEDIA_COMMAND_PREVIOUS 0x0
#define MEDIA_COMMAND_NEXT     0x1
#define MEDIA_COMMAND_PLAY     0x2
#define MEDIA_COMMAND_PAUSE    0x3


class MediaCommandsChrc : public Characteristic
{
    Q_OBJECT
    Q_PROPERTY(QByteArray value READ getValue NOTIFY valueChanged)

public:
    MediaCommandsChrc(MprisPlayer *player, QDBusConnection bus, int index, Service *service)
        : Characteristic(bus, index, MEDIA_COMM_UUID, {"notify"}, service), m_player(player)
    {
        m_value.resize(1);
    }

signals:
    void valueChanged();

private:
    MprisPlayer *m_player;
    QByteArray m_value;

    QByteArray getValue()
    {
        return m_value;
    }

public slots:
    void pauseRequested();
    void playRequested();
    void playPauseRequested();
    void stopRequested();
    void nextRequested();
    void previousRequested();
};

class MediaService : public Service
{
    Q_OBJECT
public:
    explicit MediaService(int index, QDBusConnection bus = QDBusConnection::systemBus(), QObject *parent = 0);

private:
    MprisPlayer *m_mprisPlayer;
    MediaCommandsChrc *m_commandsChrc;
};

#endif // MEDIASERVICE_H
