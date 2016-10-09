TEMPLATE = app
QT -= gui
QT += dbus
CONFIG += link_pkgconfig c++11
PKGCONFIG += giomm-2.4 mpris-qt5

HEADERS += \
    notificationservice.h \
    weatherservice.h \
    mediaservice.h \
    service.h \
    characteristic.h \
    descriptor.h \
    common.h \
    advertisement.h \
    bluezmanager.h
SOURCES += main.cpp \
    notificationservice.cpp \
    weatherservice.cpp \
    mediaservice.cpp \
    service.cpp \
    characteristic.cpp \
    descriptor.cpp \
    advertisement.cpp \
    bluezmanager.cpp

TARGET = asteroid-btsyncd
target.path = /usr/bin/

schemas = org.asteroidos.weather.gschema.xml
schemas.path = /usr/share/glib-2.0/schemas/

INSTALLS += target schemas
