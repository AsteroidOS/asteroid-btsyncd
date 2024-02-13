set(SRC
	main.cpp
	notificationservice.cpp
	weatherservice.cpp
	mediaservice.cpp
	batteryservice.cpp
	screenshotservice.cpp
	deviceinfoservice.cpp
	timeservice.cpp
	service.cpp
	characteristic.cpp
	descriptor.cpp
	application.cpp
	advertisement.cpp
	bluezmanager.cpp
	ancs.cpp
	ancs_notification.cpp)
set(HEADERS
	notificationservice.h
	weatherservice.h
	mediaservice.h
	batteryservice.h
	screenshotservice.h
	deviceinfoservice.h
	timeservice.h
	service.h
	characteristic.h
	descriptor.h
	common.h
	application.h
	advertisement.h
	bluezmanager.h
	ancs_protocol_constants.h
	ancs.h
	ancs_notification.h)

add_executable(asteroid-btsyncd ${SRC} ${HEADERS})

target_link_libraries(asteroid-btsyncd PRIVATE
	Qt5::DBus
	Timed::Timed
	QtMpris::QtMpris
	SystemSettings::SystemSettings
	PkgConfig::GIOMM)
	

install(TARGETS asteroid-btsyncd
	DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES org.asteroidos.weather.gschema.xml
	DESTINATION ${CMAKE_INSTALL_DATADIR}/glib-2.0/schemas)
