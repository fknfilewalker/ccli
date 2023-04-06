###########################################################################
# Disable insource build

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
	message(FATAL_ERROR
		"CMake generation for this project is not allowed within the source directory!"
		"\n Remove the CMakeCache.txt file and try again from another folder, e.g.:"
		"\n "
		"\n rm CMakeCache.txt"
		"\n cd .."
		"\n mkdir cmake-make"
		"\n cd cmake-make"
		"\n cmake ../project"
	)
endif()

cmake_minimum_required(VERSION 3.14.0)
# SET
set(APP_NAME example)

# COMPILER
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
if(WIN32)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHc /MP /Qpar")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:CONSOLE")
elseif(APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wwrite-strings -x objective-c++")
elseif(UNIX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wwrite-strings")
endif()

# PROJECT
project(ccli)
# .h
file(GLOB HEADERS "*.h" "*.hpp")
# .c/.cpp
file(GLOB SOURCES "*.cpp")

# exe
add_executable(${APP_NAME} WIN32 ${SOURCES} ${HEADERS})
set_target_properties(${APP_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")   
set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${APP_NAME}) 

