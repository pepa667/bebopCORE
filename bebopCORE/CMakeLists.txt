cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(bebopCORE)

if(DEFINED IDF_VERSION_MAJOR)
	if(NOT ((IDF_VERSION_MAJOR EQUAL 5) AND (IDF_VERSION_MINOR EQUAL 5)))
		message(FATAL_ERROR "bebopCORE requires ESP-IDF 5.5.x")
	endif()
else()
	message(WARNING "Unable to detect ESP-IDF version; expected 5.5.x")
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter)
if(NOT ((Python3_VERSION_MAJOR EQUAL 3) AND (Python3_VERSION_MINOR EQUAL 11)))
	message(FATAL_ERROR "bebopCORE requires Python 3.11.x")
endif()

# Custom partition table
set(PARTITION_CSV_FILE ${CMAKE_SOURCE_DIR}/partitions.csv)
