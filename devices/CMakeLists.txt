cmake_minimum_required( VERSION 2.8.11 )
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11 -flto")
set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set (CMAKE_INSTALL_PREFIX "/usr/local/" CACHE PATH "default install path" FORCE )
endif()

if(MINGW)
    add_definitions ( -municode)
endif()

########################################################################
# select the release build type by default to get optimization flags
########################################################################
if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE "Release")
   message(STATUS "Build type not specified: defaulting to release.")
endif(NOT CMAKE_BUILD_TYPE)
set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "")

### make sure our local CMake Modules path comes first
list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_SOURCE_DIR}/cmake/Modules)

######################################################################
#
########################################################################
if(DEFINED AIRSPY)
   set(AIRSPY true)
   set(objectName dab-device-airspy_lib)
endif ()

if(DEFINED SDRPLAY)
   set(SDRPLAY true)
   set(objectName dab-device-sdrplay_lib)
endif ()

if(DEFINED RTLSDR)
   set(RTLSDR true)
   set(objectName dab-device-rtlsdr_lib)
endif ()

if(DEFINED HACKRF)
   set(HACKRF true)
   set(objectName dab-device-hackrf_lib)
endif ()

######################################################################
#
########################################################################
find_package (PkgConfig)

##########################################################################
#The devices
#

if (SDRPLAY)
   find_path (SDRPLAYLIB_INCLUDE_DIR
                  NAMES mirsdrapi-rsp.h
                  PATHS
                  /usr/local/include/
    )
    include_directories (${SDRPLAYLIB_INCLUDE_DIR})

    find_library (SDRPLAYLIB mirsdrapi-rsp)
    if(NOT(SDRPLAYLIB))
       message(FATAL_ERROR "please install -lmirsdrapi-rsp")
    else(NOT(SDRPLAYLIB))
       list (APPEND extraLibs ${SDRPLAYLIB})
    endif(NOT(SDRPLAYLIB))

    include_directories (
        ../devices/sdrplay-handler
    )

    set ($(objectName)_HDRS
        ${${objectName}_HDRS}
        ../devices/sdrplay-handler/sdrplay-handler.h
    )

   set (${objectName}_SRCS
        ${${objectName}_SRCS}
        ../devices/sdrplay-handler/sdrplay-handler.cpp
   )

    add_definitions (-DHAVE_SDRPLAY)
endif (SDRPLAY)

if (HACKRF)
   find_path (HACKLIB_INCLUDE_DIR
                  NAMES mirsdrapi-rsp.h
                  PATHS
              ../devices/hackrf-handler/libhackrf/
                   /usr/local/include/
    )
    include_directories (${HACKLIB_INCLUDE_DIR})

    find_library (HACKLIB hackrf)
    if(NOT(HACKLIB))
       message(FATAL_ERROR "please install -lhackrf")
    else(NOT(HACKLIB))
       list (APPEND extraLibs ${HACKLIB})
    endif(NOT(HACKLIB))

    include_directories (
        ../devices/hackrf-handler
    )

    set ($(objectName)_HDRS
         ${${objectName}_HDRS}
         ../devices/hackrf-handler/hackrf-handler.h
    )

    set (${objectName}_SRCS
         ${${objectName}_SRCS}
         ../devices/hackrf-handler/hackrf-handler.cpp
    )

    add_definitions (-DHAVE_HACKRF)
endif (HACKRF)

if (AIRSPY)
    find_package(LibAIRSPY)
    if (NOT LIBAIRSPY_FOUND)
        message(FATAL_ERROR "please install airspy library")
    endif ()

   include_directories (
     ../devices/airspy-handler
   )

    set ($(objectName)_HDRS
        ${${objectName}_HDRS}
        ../devices/airspy-handler/airspy-handler.h
    )

    set (${objectName}_SRCS
        ${${objectName}_SRCS}
        ../devices/airspy-handler/airspy-handler.cpp
    )

    add_definitions (-DHAVE_AIRSPY)
endif (AIRSPY)

if (RTLSDR)
    find_package(LibRTLSDR)
       if (NOT LIBRTLSDR_FOUND)
           message(FATAL_ERROR "please install librtlsdr")
       endif ()

    include_directories (
        ../devices/rtlsdr-handler/
    )

    set (${objectName}_HDRS
        ${${objectName}_HDRS}
        ../devices/rtlsdr-handler/rtlsdr-handler.h
    )

    set (${objectName}_SRCS
        ${${objectName}_SRCS}
        ../devices/rtlsdr-handler/rtlsdr-handler.cpp
    )

    add_definitions (-DHAVE_RTLSDR)
endif()

#######################################################################
#
#    Here we really start

    include_directories (
        ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
        ../
        /usr/include/
    )

    set (${objectName}_HDRS
        ${${objectName}_HDRS}
        ../dab-api.h
        ../device-handler.h
        ../ringbuffer.h
    )

    set (${objectName}_SRCS
        ${${objectName}_SRCS}
    )

#####################################################################

    add_library (${objectName} SHARED
                    ${${objectName}_SRCS}
    )

    target_link_libraries (${objectName}
                           ${FFTW3F_LIBRARIES}
                           ${extraLibs}
                           ${FAAD_LIBRARIES}
                           ${CMAKE_DL_LIBS}
    )

    INSTALL (TARGETS ${objectName} 
                LIBRARY DESTINATION ./lib)

    INSTALL (FILES ${${objectName}_HDRS} DESTINATION include/${objectName} COMPONENT devel)

########################################################################
# Create uninstall target
########################################################################

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
