cmake_minimum_required(VERSION 2.8)

project(kat)

if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug)
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z")
	if (${CMAKE_SYSTEM_NAME} MATCHES Darwin)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
	endif()
endif()

set(HEADERS
    kvalue.h
    kvm.h
    kgc.h)
set(SOURCES
    kat.cpp
    kvalue.cpp
    kvm.cpp
    kgc.cpp)

include_directories(${Boost_INCLUDE_DIRS})
add_executable(${PROJECT_NAME} ${HEADERS} ${SOURCES})

