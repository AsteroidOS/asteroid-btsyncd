set(
	SRC
	main.cpp
	notificationservice.cpp
	weatherservice.cpp
	mediaservice.cpp
	batteryservice.cpp
	batterystatus.cpp
	screenshotservice.cpp
	settime.cpp
	timeservice.cpp
	service.cpp
	characteristic.cpp
	descriptor.cpp
	application.cpp
	advertisement.cpp
	bluezmanager.cpp
	ancs.cpp
	ancs_notification.cpp
	cts.cpp
)
set(
	HEADERS
	notificationservice.h
	weatherservice.h
	mediaservice.h
	batteryservice.h
	batterystatus.h
	screenshotservice.h
	settime.h
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
	ancs_notification.h
	cts.h
)

add_executable(asteroid-btsyncd ${SRC} ${HEADERS})

target_link_libraries(
	asteroid-btsyncd
	PRIVATE
		Qt6::Core
		Qt6::DBus
		PkgConfig::AmberMpris
		PkgConfig::GIOMM
)

install(
	TARGETS asteroid-btsyncd
	DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(
	FILES org.asteroidos.weather.gschema.xml
	DESTINATION ${CMAKE_INSTALL_DATADIR}/glib-2.0/schemas
)
