cmake_minimum_required(VERSION 3.7)

set(Project "XRADImmediateTest")
project(${Project} LANGUAGES CXX)

set(XRAD_Root $ENV{XRADRoot} CACHE PATH "XRAD library sources directory.")
if("${XRAD_Root}" STREQUAL "")
	message(FATAL_ERROR "Please, specify XRAD library sources directory (XRAD_Root).")
endif()

include(${XRAD_Root}/XRADBasic/CMake/CommonOptions.cmake)
include(${XRAD_Root}/XRADBasic/CMake/Package.cmake)
include(${XRAD_Root}/XRADSystem/CMake/Package.cmake)
include(${XRAD_Root}/XRADGUI/CMake/Package.cmake)

set(Project_IncludeDirs ${Project_IncludeDirs}
	.
	${XRAD_IncludeDir}
	)

set(Project_Sources_cpp
    ${XRAD_Root}/XRADBasic/Tests/StringEncodeTest.cpp
    SharpeningTest.cpp
    TestArrays.cpp
    TestDynamicDialog.cpp
    TestFFT.cpp
    TestFileOperations.cpp
    TestFunctions.cpp
    TestFunctionsArchive.cpp
    TestGUIFunctions.cpp
    TestJson.cpp
    TestMath.cpp
    TestMisc.cpp
    TestPerformanceCounter.cpp
    TestProgress.cpp
    TestThreads.cpp
    TrackedObject.cpp
	)

set(Project_Sources_h
    pre.h
    ${XRAD_Root}/XRADBasic/Tests/StringEncodeTest.h
    ${XRAD_Root}/XRADBasic/Tests/TestHelpers.h
    SharpeningTest.h
    TestArrays.h
    TestDynamicDialog.h
    TestFFT.h
    TestFileOperations.h
    TestFunctions.h
    TestFunctionsArchive.h
    TestGUIFunctions.h
    TestJson.h
    TestMath.h
    TestMisc.h
    TestPerformanceCounter.h
    TestProgress.h
    TestThreads.h
    TrackedObject.h
	)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	set(Project_Sources_PlatformSpecific_cpp
	    ${XRAD_Root}/XRADBasic/Sources/PlatformSpecific/MSVC/XRADNatvisTest.cpp
	    ${XRAD_Root}/XRADBasic/Tests/StringConverters_MS_Test.cpp
		)
	set(Project_Sources_PlatformSpecific_h
	    ${XRAD_Root}/XRADBasic/Sources/PlatformSpecific/MSVC/XRADNatvisTest.h
	    ${XRAD_Root}/XRADBasic/Tests/StringConverters_MS_Test.h
		)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	set(Project_Sources_PlatformSpecific_cpp)
	set(Project_Sources_PlatformSpecific_h)
else()
	message(FATAL_ERROR "Unsupported CMAKE_CXX_COMPILER_ID: \"${CMAKE_CXX_COMPILER_ID}\".")
endif()

set(Project_Sources_All
	${Project_Sources_cpp}
	${Project_Sources_h}
	${Project_Sources_PlatformSpecific_cpp}
	${Project_Sources_PlatformSpecific_h}
	)

add_executable(${Project} ${Project_Sources_All})
target_include_directories(${Project} PRIVATE ${Project_IncludeDirs})
target_link_libraries(${Project}
	${XRADGUI_Libraries}
	${XRADGUITestsLib_Libraries}
	${XRADSystem_Libraries}
	${XRADBasic_Libraries}
	)
