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

class MediaTitleChrc : public Characteristic
{
public:
    MediaTitleChrc(QDBusConnection bus, int index, Service *service) : Characteristic(bus, index, MEDIA_TITLE_UUID, {"write"}, service) {}

public slots:
    void writeValue(QByteArray value)
    {
        // TODO: Title
    }
};

class MediaAlbumChrc : public Characteristic
{
public:
    MediaAlbumChrc(QDBusConnection bus, int index, Service *service) : Characteristic(bus, index, MEDIA_ALBUM_UUID, {"write"}, service) {}

public slots:
    void writeValue(QByteArray value)
    {
        // TODO: Album
    }
};

class MediaArtistChrc : public Characteristic
{
public:
    MediaArtistChrc(QDBusConnection bus, int index, Service *service) : Characteristic(bus, index, MEDIA_ARTIST_UUID, {"write"}, service) {}

public slots:
    void writeValue(QByteArray value)
    {
        // TODO: Artist
    }
};

class MediaPlayingChrc : public Characteristic
{
public:
    MediaPlayingChrc(QDBusConnection bus, int index, Service *service) : Characteristic(bus, index, MEDIA_PLAY_UUID, {"write"}, service) {}

public slots:
    void writeValue(QByteArray value)
    {
        // TODO: Playing
    }
};

class MediaCommandsChrc : public Characteristic
{
public:
    MediaCommandsChrc(QDBusConnection bus, int index, Service *service) : Characteristic(bus, index, MEDIA_COMM_UUID, {"notify"}, service) {}

    // TODO: Commands
};

MediaService::MediaService(QDBusConnection bus, int index, QObject *parent) : Service(bus, index, MEDIA_UUID, parent)
{
    addCharacteristic(new MediaTitleChrc(bus, 0, this));
    addCharacteristic(new MediaAlbumChrc(bus, 1, this));
    addCharacteristic(new MediaArtistChrc(bus, 2, this));
    addCharacteristic(new MediaPlayingChrc(bus, 3, this));
    addCharacteristic(new MediaCommandsChrc(bus, 4, this));
}
