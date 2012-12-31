project(MK_DataLogger CXX C)
cmake_minimum_required(VERSION 2.8)

# Set default built type
set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
	"Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel.")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

# Target platfrom
option(FULL_OPTIMIZATION "Enable all optimization flags" OFF)

# This enables Eclipse to discover the include dirs automatically
SET(CMAKE_VERBOSE_MAKEFILE ON)

# The CDT Error Parser cannot handle error messages that span more than one line, which is the
# default gcc behavior. This forces gcc to generate single line error messages with no line
# wrapping.
IF(CMAKE_COMPILER_IS_GNUCC)
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fmessage-length=0")
ENDIF(CMAKE_COMPILER_IS_GNUCC)
IF(CMAKE_COMPILER_IS_GNUCXX)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fmessage-length=0")
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

# Settings
if(CMAKE_COMPILER_IS_GNUCXX)
	# Add support for C++11/0x
	add_definitions(-std=gnu++0x)
	
	# Optimizations etc
	if(FULL_OPTIMIZATION)
		message(STATUS "Building with a performance profile optimized for the HexaMac")
		add_definitions(-g -pthread -O6 -Wall -march=corei7 -mtune=corei7 -msse -msse2 -msse3 -msse4 -msse4.2)
	else()
		message(STATUS "Building with a general performance profile.")
		add_definitions(-g -pthread -O3 -Wall -march=native -mtune=native)
	endif()
	
	#set(CMAKE_EXE_LINKER_FLAGS -fopenmp) 
endif()

if(UNIX)
	add_definitions(-D_LINUX)
endif()

set(MKDL_SRC
	MK_Connection.cc
	MK_DataLogger.cc
	MK_Protocol.cc
	rs232.c
)

add_executable(MK_DataLogger ${MKDL_SRC})