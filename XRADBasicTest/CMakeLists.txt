cmake_minimum_required(VERSION 3.7)

set(Project "XRADBasicTest")
project(${Project} LANGUAGES CXX)

set(XRAD_Root $ENV{XRADRoot} CACHE PATH "XRAD library sources directory.")
if("${XRAD_Root}" STREQUAL "")
	message(FATAL_ERROR "Please, specify XRAD library sources directory (XRAD_Root).")
endif()

include(${XRAD_Root}/XRADBasic/CMake/Package.cmake)

set(Project_IncludeDirs ${Project_IncludeDirs}
	${XRAD_IncludeDir}
	)

set(Project_Sources_All
	XRADBasicTest.cpp
	pre.h
	)

add_executable(${Project} ${Project_Sources_All})
target_include_directories(${Project} PRIVATE ${Project_IncludeDirs})
target_link_libraries(${Project}
	${XRADBasic_Libraries}
	)
xrad_add_dependencies(${Project} "XRADBasic")
