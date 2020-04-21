cmake_minimum_required(VERSION 3.7)

set(Project "XRADImmediateTest")
project(${Project} LANGUAGES CXX)

set(XRAD_Root $ENV{XRADRoot} CACHE PATH "XRAD library sources directory.")
if("${XRAD_Root}" STREQUAL "")
	message(FATAL_ERROR "Please, specify XRAD library sources directory (XRAD_Root).")
endif()

include(${XRAD_Root}/XRADGUI/CMake/Package.cmake)

set(Project_IncludeDirs ${Project_IncludeDirs}
	.
	${XRAD_IncludeDir}
	)

set(Project_Sources
	pre.h
	${XRAD_Root}/XRADBasic/Tests/StringEncodeTest.cpp
	${XRAD_Root}/XRADBasic/Tests/StringEncodeTest.h
	${XRAD_Root}/XRADBasic/Tests/TestHelpers.h
	SharpeningTest.cpp
	SharpeningTest.h
	TestArrays.cpp
	TestArrays.h
	TestDynamicDialog.cpp
	TestDynamicDialog.h
	TestFFT.cpp
	TestFFT.h
	TestFileOperations.cpp
	TestFileOperations.h
	TestFunctions.cpp
	TestFunctions.h
	TestFunctionsArchive.cpp
	TestFunctionsArchive.h
	TestGUIFunctions.cpp
	TestGUIFunctions.h
	TestJson.cpp
	TestJson.h
	TestMath.cpp
	TestMath.h
	TestMisc.cpp
	TestMisc.h
	TestPerformanceCounter.cpp
	TestPerformanceCounter.h
	TestProgress.cpp
	TestProgress.h
	TestThreads.cpp
	TestThreads.h
	TrackedObject.cpp
	TrackedObject.h
	)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	# TODO: TestFFT_asm.asm
	set(Project_Sources_PlatformSpecific
		${XRAD_Root}/XRADBasic/Sources/PlatformSpecific/MSVC/XRADNatvisTest.cpp
		${XRAD_Root}/XRADBasic/Sources/PlatformSpecific/MSVC/XRADNatvisTest.h
		${XRAD_Root}/XRADBasic/Tests/StringConverters_MS_Test.cpp
		${XRAD_Root}/XRADBasic/Tests/StringConverters_MS_Test.h
		)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	set(Project_Sources_PlatformSpecific)
else()
	message(FATAL_ERROR "Unsupported CMAKE_CXX_COMPILER_ID: \"${CMAKE_CXX_COMPILER_ID}\".")
endif()

set(Project_Sources_All
	${Project_Sources}
	${Project_Sources_PlatformSpecific}
	)

add_executable(${Project} ${XRADGUI_ExecutableOptions} ${Project_Sources_All})
target_include_directories(${Project} PRIVATE ${Project_IncludeDirs})
target_link_libraries(${Project}
	${XRADGUI_Libraries}
	${XRADGUITestsLib_Libraries}
	)
xrad_add_dependencies(${Project} "XRADGUI;XRADGUITestsLib")
