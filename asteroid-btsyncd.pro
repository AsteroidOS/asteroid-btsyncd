TEMPLATE = app
QT -= gui
QT += dbus
CONFIG += link_pkgconfig c++11
PKGCONFIG += giomm-2.4 mpris-qt5 contextkit-statefs timed-qt5 libkcalcoren-qt5 libmkcal-qt5

HEADERS += \
    notificationservice.h \
    weatherservice.h \
    mediaservice.h \
    batteryservice.h \
    screenshotservice.h \
    timeservice.h \
    calendarservice.h \
    service.h \
    characteristic.h \
    descriptor.h \
    common.h \
    application.h \
    advertisement.h \
    bluezmanager.h \
    ancs_protocol_constants.h \
    ancs.h \
    ancs_notification.h
SOURCES += main.cpp \
    notificationservice.cpp \
    weatherservice.cpp \
    mediaservice.cpp \
    batteryservice.cpp \
    screenshotservice.cpp \
    timeservice.cpp \
    calendarservice.cpp \
    service.cpp \
    characteristic.cpp \
    descriptor.cpp \
    application.cpp \
    advertisement.cpp \
    bluezmanager.cpp \
    ancs.cpp \
    ancs_notification.cpp

TRANSLATIONS = $$files(i18n/$$TARGET.*.ts)

TARGET = asteroid-btsyncd
target.path = /usr/bin/

schemas.files = org.asteroidos.weather.gschema.xml
schemas.path = /usr/share/glib-2.0/schemas/

INSTALLS += target schemas
