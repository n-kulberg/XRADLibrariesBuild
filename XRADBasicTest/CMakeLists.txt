cmake_minimum_required(VERSION 3.7)

project("XRADBasicTest")

#if(NOT DEFINED ENV{XRADRoot})
#	message(FATAL_ERROR "The XRADRoot environment variable is not set.")
#endif()
set(XRADRoot $ENV{XRADRoot} CACHE PATH "XRAD library root directory.")
if("${XRADRoot}" STREQUAL "")
	message(FATAL_ERROR "The XRAD root directory must be set.")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	SET(XRADBasic_IncludeDir "${XRADRoot}")
        SET(XRADBasic_LibrariesDir "${XRADRoot}/XRADBasic/out/build/x64-Debug" CACHE PATH "Path to XRADBasic build directory.")
	SET(XRADBasic_Libraries "${XRADBasic_LibrariesDir}/XRADBasicD.lib")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	SET(XRADBasic_IncludeDir "${XRADRoot}")
        SET(XRADBasic_BuildDir "${XRADRoot}/build" CACHE PATH "Path to XRAD build root directory.")
	SET(XRADBasic_LibrariesDir "${XRADBasic_BuildDir}/XRADBasic")
	SET(XRADBasic_Libraries "${XRADBasic_LibrariesDir}/libXRADBasic.a")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -fopenmp")
	#SET(SystemLibraries "libomp.a" "libgcc.a" "libstdc++.a")
else()
	message(FATAL_ERROR "Unsupported CMAKE_CXX_COMPILER_ID: \"${CMAKE_CXX_COMPILER_ID}\".")
endif()

include_directories(${XRADBasic_IncludeDir})

set(XRADBasicTest_Sources_cpp
	XRADBasicTest.cpp
)

set(XRADBasicTest_Sources_h
	pre.h
)

set(XRADBasicTest_Sources_All
	${XRADBasicTest_Sources_cpp}
	${XRADBasicTest_Sources_h}
)

add_executable(XRADBasicTest ${XRADBasicTest_Sources_All})
target_link_libraries(XRADBasicTest ${XRADBasic_Libraries} ${SystemLibraries})
