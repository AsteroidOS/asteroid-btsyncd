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

#include <giomm.h>

#include "weatherservice.h"
#include "characteristic.h"
#include "common.h"
#include <time.h>

int getQByteArrayInt(QByteArray arr, int index) {
    return (((unsigned char) arr[index * 2]) << 8) | ((unsigned char) arr[index * 2 + 1]);
}

class WeatherCityChrc : public Characteristic
{
public:
    WeatherCityChrc(QDBusConnection bus, int index, Service *service) : Characteristic(bus, index, WEAT_CITY_UUID, {"encrypt-authenticated-write","notify"}, service) {}

public slots:
    void WriteValue(QByteArray value, QVariantMap)
    {
        const Glib::RefPtr<Gio::Settings> settings = Gio::Settings::create("org.asteroidos.weather");
        settings->set_string("city-name", value.data());
    }
};

class WeatherIdsChrc : public Characteristic
{
public:
    WeatherIdsChrc(QDBusConnection bus, int index, Service *service) : Characteristic(bus, index, WEAT_IDS_UUID, {"encrypt-authenticated-write"}, service) {}

public slots:
    void WriteValue(QByteArray value, QVariantMap)
    {
        for(int i = 0; i < 5; i++) {
            const Glib::RefPtr<Gio::Settings> settings = Gio::Settings::create("org.asteroidos.weather.day" + std::to_string(i));
            settings->set_int("id", getQByteArrayInt(value, i));
        }

        const Glib::RefPtr<Gio::Settings> settings = Gio::Settings::create("org.asteroidos.weather");
        settings->set_int("timestamp-day0", (int)time(NULL));
    }
};

class WeatherMinTempsChrc : public Characteristic
{
public:
    WeatherMinTempsChrc(QDBusConnection bus, int index, Service *service) : Characteristic(bus, index, WEAT_MINT_UUID, {"encrypt-authenticated-write"}, service) {}

public slots:
    void WriteValue(QByteArray value, QVariantMap)
    {
        for(int i = 0; i < 5; i++) {
            const Glib::RefPtr<Gio::Settings> settings = Gio::Settings::create("org.asteroidos.weather.day" + std::to_string(i));
            settings->set_int("min-temp", getQByteArrayInt(value, i));
        }
    }
};

class WeatherMaxTempsChrc : public Characteristic
{
public:
    WeatherMaxTempsChrc(QDBusConnection bus, int index, Service *service) : Characteristic(bus, index, WEAT_MAXT_UUID, {"encrypt-authenticated-write"}, service) {}

public slots:
    void WriteValue(QByteArray value, QVariantMap)
    {
        for(int i = 0; i < 5; i++) {
            const Glib::RefPtr<Gio::Settings> settings = Gio::Settings::create("org.asteroidos.weather.day" + std::to_string(i));
            settings->set_int("max-temp", getQByteArrayInt(value, i));
        }
    }
};

WeatherService::WeatherService(int index, QDBusConnection bus, QObject *parent) : Service(bus, index, WEATHER_UUID, parent)
{
    addCharacteristic(new WeatherCityChrc(bus, 0, this));
    addCharacteristic(new WeatherIdsChrc(bus, 1, this));
    addCharacteristic(new WeatherMaxTempsChrc(bus, 2, this));
    addCharacteristic(new WeatherMinTempsChrc(bus, 3, this));
}
