cmake_minimum_required(VERSION 3.7)

project("XRADConsoleTest")

#if(NOT DEFINED ENV{XRADRoot})
#	message(FATAL_ERROR "The XRADRoot environment variable is not set.")
#endif()
set(XRADRoot $ENV{XRADRoot} CACHE PATH "XRAD library root directory.")
if("${XRADRoot}" STREQUAL "")
	message(FATAL_ERROR "The XRAD root directory must be set.")
endif()

if ((CACHE{XRADBasic_Libraries} OR (DEFINED XRADBasic_Libraries)) AND ("${XRADBasic_Libraries}" STREQUAL ""))
	unset(XRADBasic_Libraries CACHE)
endif ()
if ((CACHE{XRADSystem_Libraries} OR (DEFINED XRADSystem_Libraries)) AND ("${XRADSystem_Libraries}" STREQUAL ""))
	unset(XRADSystem_Libraries CACHE)
endif ()
if ((CACHE{XRADConsoleUI_Libraries} OR (DEFINED XRADConsoleUI_Libraries)) AND ("${XRADConsoleUI_Libraries}" STREQUAL ""))
	unset(XRADConsoleUI_Libraries CACHE)
endif ()
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	set(XRAD_IncludeDir "${XRADRoot}")
	set(XRAD_BuildDir "${XRADRoot}" CACHE PATH "Path to XRAD build directory.")
	set(XRADBasic_Libraries "${XRAD_BuildDir}/XRADBasic/out/build/x64-Debug/XRADBasicD.lib" CACHE FILEPATH "The XRADBasicD.lib file location.")
	set(XRADSystem_Libraries "${XRAD_BuildDir}/XRADSystem/out/build/x64-Debug/XRADSystemD.lib" CACHE FILEPATH "The XRADSystemD.lib file location.")
	set(XRADConsoleUI_Libraries "${XRAD_BuildDir}/XRADConsoleUI/out/build/x64-Debug/XRADConsoleUID.lib" CACHE FILEPATH "The XRADConsoleUID.lib file location.")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	set(XRAD_IncludeDir "${XRADRoot}")
	set(XRAD_BuildDir "${XRADRoot}/build" CACHE PATH "Path to XRAD build root directory.")
	set(XRADBasic_Libraries "${XRAD_BuildDir}/XRADBasic/libXRADBasic.a" CACHE FILEPATH "The libXRADBasic.a file location.")
	set(XRADSystem_Libraries "${XRAD_BuildDir}/XRADSystem/libXRADSystem.a" CACHE FILEPATH "The libXRADSystem.a file location.")
	set(XRADConsoleUI_Libraries "${XRAD_BuildDir}/XRADConsoleUI/libXRADConsoleUI.a" CACHE FILEPATH "The libXRADConsoleUI.a file location.")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -fopenmp")
else()
	message(FATAL_ERROR "Unsupported CMAKE_CXX_COMPILER_ID: \"${CMAKE_CXX_COMPILER_ID}\".")
endif()

include_directories(${XRAD_IncludeDir})

set(XRADConsoleTest_Sources_cpp
	XRADConsoleTest.cpp
)

set(XRADConsoleTest_Sources_h
	pre.h
)

set(XRADConsoleTest_Sources_All
	${XRADConsoleTest_Sources_cpp}
	${XRADConsoleTest_Sources_h}
)

add_executable(XRADConsoleTest ${XRADConsoleTest_Sources_All})
target_link_libraries(XRADConsoleTest
	${XRADConsoleUI_Libraries}
	${XRADSystem_Libraries}
	${XRADBasic_Libraries}
)
