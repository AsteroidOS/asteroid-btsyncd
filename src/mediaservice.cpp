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

#include "mediaservice.h"
#include "characteristic.h"
#include "common.h"

#include <QDebug>

class MediaTitleChrc : public Characteristic
{
public:
    MediaTitleChrc(MprisPlayer *player, QDBusConnection bus, int index, Service *service)
        : Characteristic(bus, index, MEDIA_TITLE_UUID, {"encrypt-authenticated-write"}, service), m_player(player) {}

private:
    MprisPlayer *m_player;

public slots:
    void WriteValue(QByteArray value, QVariantMap)
    {
        QVariantMap metadata = m_player->metadata();
        metadata[Mpris::metadataToString(Mpris::Title)] = QString(value);
        m_player->setMetadata(metadata);
    }
};

class MediaAlbumChrc : public Characteristic
{
public:
    MediaAlbumChrc(MprisPlayer *player, QDBusConnection bus, int index, Service *service)
        : Characteristic(bus, index, MEDIA_ALBUM_UUID, {"encrypt-authenticated-write"}, service), m_player(player) {}

private:
    MprisPlayer *m_player;

public slots:
    void WriteValue(QByteArray value, QVariantMap)
    {
        QVariantMap metadata = m_player->metadata();
        metadata[Mpris::metadataToString(Mpris::Album)] = QString(value);
        m_player->setMetadata(metadata);
    }
};

class MediaArtistChrc : public Characteristic
{
public:
    MediaArtistChrc(MprisPlayer *player, QDBusConnection bus, int index, Service *service)
        : Characteristic(bus, index, MEDIA_ARTIST_UUID, {"encrypt-authenticated-write"}, service), m_player(player) {}

private:
    MprisPlayer *m_player;

public slots:
    void WriteValue(QByteArray value, QVariantMap)
    {
        QVariantMap metadata = m_player->metadata();
        metadata[Mpris::metadataToString(Mpris::Artist)] = QString(value);
        m_player->setMetadata(metadata);
    }
};

class MediaPlayingChrc : public Characteristic
{
public:
    MediaPlayingChrc(MprisPlayer *player, QDBusConnection bus, int index, Service *service)
        : Characteristic(bus, index, MEDIA_PLAY_UUID, {"encrypt-authenticated-write"}, service), m_player(player) {}

private:
    MprisPlayer *m_player;

public slots:
    void WriteValue(QByteArray value, QVariantMap)
    {
        m_player->setPlaybackStatus(value[0] ? Mpris::Playing : Mpris::Paused);
    }
};

void MediaCommandsChrc::pauseRequested()
{
    m_value[0] = MEDIA_COMMAND_PAUSE;
    emit valueChanged();
}

void MediaCommandsChrc::playRequested()
{
    m_value[0] = MEDIA_COMMAND_PLAY;
    emit valueChanged();
}

void MediaCommandsChrc::playPauseRequested()
{
    if(m_player->playbackStatus() == Mpris::Playing)
        m_value[0] = MEDIA_COMMAND_PAUSE;
    else
        m_value[0] = MEDIA_COMMAND_PLAY;

    emit valueChanged();
}

void MediaCommandsChrc::stopRequested()
{
    m_value[0] = MEDIA_COMMAND_PAUSE;
    emit valueChanged();
}

void MediaCommandsChrc::nextRequested()
{
    m_value[0] = MEDIA_COMMAND_NEXT;
    emit valueChanged();
}

void MediaCommandsChrc::previousRequested()
{
    m_value[0] = MEDIA_COMMAND_PREVIOUS;
    emit valueChanged();
}

void MediaCommandsChrc::volumeRequested(double volume)
{
    m_value[0] = MEDIA_COMMAND_VOLUME;
    m_value[1] = volume*100;
    emit valueChanged();
}

void MediaCommandsChrc::emitPropertiesChanged()
{
    QDBusConnection connection = QDBusConnection::systemBus();
    QDBusMessage message = QDBusMessage::createSignal(getPath().path(),
                                                      "org.freedesktop.DBus.Properties",
                                                      "PropertiesChanged");

    QVariantMap changedProperties;
    changedProperties[QStringLiteral("Value")] = QVariant(m_value);

    QList<QVariant> arguments;
    arguments << QVariant(GATT_CHRC_IFACE) << QVariant(changedProperties) << QVariant(QStringList());
    message.setArguments(arguments);

    if (!connection.send(message))
        qDebug() << "Failed to send DBus property notification signal";
}

class MediaVolumeChrc : public Characteristic
{
public:
    MediaVolumeChrc(MprisPlayer *player, QDBusConnection bus, int index, Service *service)
        : Characteristic(bus, index, MEDIA_VOL_UUID, {"encrypt-authenticated-write"}, service), m_player(player) {}

private:
    MprisPlayer *m_player;

public slots:
    void WriteValue(QByteArray value, QVariantMap)
    {
        m_player->setVolume(int( (unsigned char) value[0])/100.0);
    }
};

MediaService::MediaService(int index, QDBusConnection bus, QObject *parent) : Service(bus, index, MEDIA_UUID, parent)
{
    m_mprisPlayer = new MprisPlayer(this);
    m_mprisPlayer->setServiceName("asteroid-btsyncd");
    m_mprisPlayer->setIdentity("Asteroid BLE Sync Daemon");

    m_mprisPlayer->setCanControl(true);
    m_mprisPlayer->setCanGoNext(true);
    m_mprisPlayer->setCanGoPrevious(true);
    m_mprisPlayer->setCanPause(true);
    m_mprisPlayer->setCanPlay(true);

    m_mprisPlayer->setCanSeek(false);
    m_mprisPlayer->setCanQuit(false);
    m_mprisPlayer->setCanRaise(false);
    m_mprisPlayer->setCanSetFullscreen(false);

    m_mprisPlayer->setPlaybackStatus(Mpris::Stopped);
    m_mprisPlayer->setLoopStatus(Mpris::None);
    m_mprisPlayer->setShuffle(false);

    addCharacteristic(new MediaTitleChrc(m_mprisPlayer, bus, 0, this));
    addCharacteristic(new MediaAlbumChrc(m_mprisPlayer, bus, 1, this));
    addCharacteristic(new MediaArtistChrc(m_mprisPlayer, bus, 2, this));
    addCharacteristic(new MediaPlayingChrc(m_mprisPlayer, bus, 3, this));
    m_commandsChrc = new MediaCommandsChrc(m_mprisPlayer, bus, 4, this);
    addCharacteristic(m_commandsChrc);
    addCharacteristic(new MediaVolumeChrc(m_mprisPlayer, bus, 5, this));

    connect(m_mprisPlayer, SIGNAL(pauseRequested()), m_commandsChrc, SLOT(pauseRequested()));
    connect(m_mprisPlayer, SIGNAL(playRequested()), m_commandsChrc, SLOT(playRequested()));
    connect(m_mprisPlayer, SIGNAL(playPauseRequested()), m_commandsChrc, SLOT(playPauseRequested()));
    connect(m_mprisPlayer, SIGNAL(stopRequested()), m_commandsChrc, SLOT(stopRequested()));
    connect(m_mprisPlayer, SIGNAL(nextRequested()), m_commandsChrc, SLOT(nextRequested()));
    connect(m_mprisPlayer, SIGNAL(previousRequested()), m_commandsChrc, SLOT(previousRequested()));
    connect(m_mprisPlayer, SIGNAL(volumeRequested(double)), m_commandsChrc, SLOT(volumeRequested(double)));
 }
