cmake_minimum_required(VERSION 3.6.0)

project(
	asteroid-btsyncd
	VERSION 2.0
	DESCRIPTION "Bluetooth Low Energy synchronization daemon for AsteroidOS"
)

find_package(ECM REQUIRED NO_MODULE)
find_package(AsteroidApp6 REQUIRED)

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ASTEROID_MODULE_PATH})

include(FeatureSummary)
include(GNUInstallDirs)
include(ECMFindQmlModule)
include(Asteroid6CMakeSettings)
include(Asteroid6Translations)

find_package(Qt6 REQUIRED COMPONENTS Core DBus)

find_package(PkgConfig REQUIRED)
pkg_check_modules(AmberMpris ambermpris6 REQUIRED IMPORTED_TARGET)
pkg_check_modules(MLITE mlite6 REQUIRED IMPORTED_TARGET)

add_subdirectory(src)

build_translations(i18n)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
