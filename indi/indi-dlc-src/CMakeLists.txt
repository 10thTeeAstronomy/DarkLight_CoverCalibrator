cmake_minimum_required(VERSION 3.10)
project(darklight_covercalibrator)

include(GNUInstallDirs)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")

find_package(INDI REQUIRED)

set(CDRIVER_VERSION_MAJOR 0)
set(CDRIVER_VERSION_MINOR 2)

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake
	${CMAKE_CURRENT_BINARY_DIR}/config.h
	)

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/indi_darklight_covercalibrator.xml.cmake
	${CMAKE_CURRENT_BINARY_DIR}/indi_darklight_covercalibrator.xml
	)

include_directories( ${CMAKE_CURRENT_BINARY_DIR})
include_directories( ${CMAKE_CURRENT_SOURCE_DIR})
include_directories( ${INDI_INCLUDE_DIR})

include(CMakeCommon)

add_executable(
	indi_darklight_covercalibrator 
	darklight_covercalibrator.cpp
	)

target_link_libraries(
	indi_darklight_covercalibrator
	${INDI_LIBRARIES}
	)

install(TARGETS indi_darklight_covercalibrator RUNTIME DESTINATION bin)

install(
	FILES
	${CMAKE_CURRENT_BINARY_DIR}/indi_darklight_covercalibrator.xml
	DESTINATION ${INDI_DATA_DIR}
	)
