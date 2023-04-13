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

# VARS
option(CCLI_BUILD_TESTS "Build Tests" ON)

# PROJECT
#set(CMAKE_GENERATOR_TOOLSET "ClangCL")
project(ccli)
set(CCLI ccli)

# COMPILER
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
if (MSVC)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:CONSOLE")
elseif(APPLE)
elseif(UNIX)
endif()

# SOURCES
file(GLOB SOURCES "src/ccli.cpp")
file(GLOB HEADERS "include/ccli/ccli.h")

# GROUPING
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "Source Files" FILES ${SOURCES})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/include" PREFIX "Header Files" FILES ${HEADERS})

# TARGET
add_library(${CCLI} STATIC ${SOURCES} ${HEADERS})
add_library(${CCLI}::${CCLI} ALIAS ${CCLI})
target_include_directories(${CCLI} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

if(CCLI_BUILD_TESTS)
	# exe
	set(TEST_NAME "basic_test")
	add_executable(${TEST_NAME} WIN32 "test/basic_test.cpp")
	set_target_properties(${TEST_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")   
	set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${TEST_NAME}) 

	target_link_libraries(${TEST_NAME} PRIVATE ccli::ccli)
	add_dependencies(${TEST_NAME} ccli::ccli)
endif()

