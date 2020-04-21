cmake_minimum_required(VERSION 3.7)

set(Project "XRADDicomTest")
project(${Project} LANGUAGES CXX)

set(XRAD_Root $ENV{XRADRoot} CACHE PATH "XRAD library sources directory.")
if("${XRAD_Root}" STREQUAL "")
	message(FATAL_ERROR "Please, specify XRAD library sources directory (XRAD_Root).")
endif()

include(${XRAD_Root}/XRADGUI/CMake/Package.cmake)
include(${XRAD_Root}/XRADDicomGUI/CMake/Package.cmake)

set(Project_IncludeDirs ${Project_IncludeDirs}
	.
	${XRAD_IncludeDir}
	)

set(Project_Sources_All
	pre.h
	GenerateFigures.cpp
	GenerateFigures.h
	Tests.cpp
	Tests.h
	XRADDicomTest.cpp
	)

add_executable(${Project} ${XRADGUI_ExecutableOptions} ${Project_Sources_All})
target_include_directories(${Project} PRIVATE ${Project_IncludeDirs})
target_link_libraries(${Project}
	${XRADDicomGUI_Libraries}
	${XRADGUI_Libraries}
	)
xrad_add_dependencies(${Project} "XRADDicomGUI;XRADGUI")
