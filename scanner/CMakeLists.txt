cmake_minimum_required( VERSION 3.5 )
set (objectName dab_scanner)
#set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11 -flto")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11 -g -fsanitize=address")
#set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")
#set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -g")
set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -g -fsanitize=address")
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set (CMAKE_INSTALL_PREFIX "/usr/local/bin" CACHE PATH "default install path" FORCE )
endif()
#set (CMAKE_INSTALL_PREFIX /usr/local/bin)

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

OPTION(RTLSDR   "Input: RTLSDR"   OFF)
OPTION(SDRPLAY  "Input: SDRPLAY"  OFF)
OPTION(AIRSPY   "Input: AirSpy"   OFF)
OPTION(HACKRF	"Input: HACKRF"	  OFF)
OPTION(RTL_TCP	"Input: RTL_TCP"  OFF)
OPTION(LIMESDR	"Input: LIMESDR"  OFF)
OPTION(SDRPLAY_V3  "Input: SDRPlay_V3"  OFF)

#OPTION(X64_DEFINED "optimize for x64/SSE"  OFF)

if ( (NOT RTLSDR) AND (NOT SDRPLAY) AND (NOT AirSpy) AND (NOT HACKRF) AND (NOT LIMESDR) AND (NOT SDRPLAY_V3) AND (NOT RTL_TCP) )
   message("None of the Input Options selected. Using default SDRPlay")
   set(SDRPlay ON)
endif ()

set (objectName "")

if(RTLSDR)
   if (objectName STREQUAL "")
      set(RTLSDR ON)
      set(objectName dab-scanner-rtlsdr)
   else ()
      message ("Ignoring second option")
   endif ()
endif ()

if(SDRPLAY)
   if (objectName STREQUAL "")
      set(SDRPLAY ON)
      set(objectName dab-scanner-sdrplay-v2)
   else ()
      message ("Ignoring second option")
   endif ()
endif ()

if(AIRSPY)
   if (objectName STREQUAL "")
      set(AIRSPY ON)
      set(objectName dab-scanner-airspy)
   else ()
      message ("Ignoring second option")
   endif ()
endif ()

if(RTL_TCP)
   if (objectName STREQUAL "")
      set(RTL_TCP ON)
      set(objectName dab-scanner-rtl_tcp)
   else ()
      message ("Ignoring second option")
   endif ()
endif ()

if(HACKRF)
   if (objectName STREQUAL "")
      set(HACKRF ON)
      set(objectName dab-scanner-hackrf)
   else ()
      message ("Ignoring second option")
   endif ()
endif ()

if(LIMESDR)
   if (objectName STREQUAL "")
      set(LIMESDR ON)
      set(objectName dab-scanner-lime)
   else ()
      message ("Ignoring second option")
   endif ()
endif ()

if(SDRPLAY_V3)
   if (objectName STREQUAL "")
      set(SDRPLAY_V3 ON)
      set(objectName dab-scanner-sdrplay-v3)
   else ()
      message ("Ignoring second option")
   endif ()
endif ()


#########################################################################
	find_package (PkgConfig)

        find_package(FFTW3f)
        if (NOT FFTW3F_FOUND)
            message(FATAL_ERROR "please install FFTW3")
        endif ()

        find_package(zlib)
	if (NOT ZLIB_FOUND)
            message(FATAL_ERROR "please install libz")
        endif ()
	list(APPEND extraLibs ${ZLIB_LIBRARY})

	find_library (PTHREADS pthread)
	if (NOT(PTHREADS))
	   message (FATAL_ERROR "please install libpthread")
	else (NOT(PTHREADS))
	   set (extraLibs ${extraLibs} ${PTHREADS})
	endif (NOT(PTHREADS))


#########################################################################
        find_package (PkgConfig)

##########################################################################
#	The devices
#

	if (RTL_TCP)
	   include_directories (
	     ./devices/rtl_tcp
	   )
	   set ($(objectName)_HDRS
	        ${${objectName}_HDRS}
	        ./devices/rtl_tcp/rtl_tcp-client.h
           )

	   set (${objectName}_SRCS
	        ${${objectName}_SRCS}
	        ./devices/rtl_tcp/rtl_tcp-client.cpp
	   )

	add_definitions (-DHAVE_RTL_TCP)
	endif (RTL_TCP)
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
	     ./devices/sdrplay-handler
	   )

	   set ($(objectName)_HDRS
	        ${${objectName}_HDRS}
	        ./devices/sdrplay-handler/sdrplay-handler.h
           )

	   set (${objectName}_SRCS
	        ${${objectName}_SRCS}
	        ./devices/sdrplay-handler/sdrplay-handler.cpp
	   )

	 add_definitions (-DHAVE_SDRPLAY)
	endif (SDRPLAY)

	if (SDRPLAY_V3)
	   find_path (SDRPLAYLIB_INCLUDE_DIR
                      NAMES sdrplay_api.h
                      PATHS
                      /usr/local/include/
	   )
           include_directories (${SDRPLAYLIB_INCLUDE_DIR})

           find_library (SDRPLAYLIB sdrplay_api)
           if(NOT(SDRPLAYLIB))
              message(FATAL_ERROR "please install -lsdrplay_api")
           else(NOT(SDRPLAYLIB))
              list (APPEND extraLibs ${SDRPLAYLIB})
           endif(NOT(SDRPLAYLIB))

	   include_directories (
	     ./devices/sdrplay-handler-v3
	   )

	   set ($(objectName)_HDRS
	        ${${objectName}_HDRS}
	        ./devices/sdrplay-handler-v3/sdrplay-handler-v3.h
           )

	   set (${objectName}_SRCS
	        ${${objectName}_SRCS}
	        ./devices/sdrplay-handler-v3/sdrplay-handler-v3.cpp
	   )

	   add_definitions (-DHAVE_SDRPLAY_V3)
	endif (SDRPLAY_V3)

	if (HACKRF)
	   find_path (HACKLIB_INCLUDE_DIR
                      NAMES mirsdrapi-rsp.h
                      PATHS
	              ./devices/hackrf-handler/libhackrf/
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
	     ./devices/hackrf-handler
	   )

	   set ($(objectName)_HDRS
	        ${${objectName}_HDRS}
	        ./devices/hackrf-handler/hackrf-handler.h
           )

	   set (${objectName}_SRCS
	        ${${objectName}_SRCS}
	        ./devices/hackrf-handler/hackrf-handler.cpp
	   )

	 add_definitions (-DHAVE_HACKRF)
	endif (HACKRF)

	if (LIMESDR)
           find_library (LIMELIB LimeSuite)
           if(NOT(LIMELIB))
              message(FATAL_ERROR "please install LimeSuite")
           else(NOT(LIMELIB))
              list (APPEND extraLibs ${LIMELIB})
           endif(NOT(LIMELIB))

	   include_directories (
	     ./devices/lime-handler
	   )

	   set ($(objectName)_HDRS
	        ${${objectName}_HDRS}
	        ./devices/lime-handler/LMS7002M_parameters.h
	        ./devices/lime-handler/LimeSuite.h
	        ./devices/lime-handler/lime-handler.h
           )

	   set (${objectName}_SRCS
	        ${${objectName}_SRCS}
	        ./devices/lime-handler/lime-handler.cpp
	   )

	 add_definitions (-DHAVE_LIME)
	endif (LIMESDR)

	if (AIRSPY)
           find_package(LibAIRSPY)
           if (NOT LIBAIRSPY_FOUND)
               message(FATAL_ERROR "please install airspy library")
           endif ()
	   ### include_directories (${AIRSPYLIB_INCLUDE_DIR})

	   include_directories (
	     ./devices/airspy-handler
	   )

	   set ($(objectName)_HDRS
	        ${${objectName}_HDRS}
	        ./devices/airspy-handler/airspy-handler.h
	   )

	   set (${objectName}_SRCS
	        ${${objectName}_SRCS}
	        ./devices/airspy-handler/airspy-handler.cpp
	   )

	   add_definitions (-DHAVE_AIRSPY)
	endif (AIRSPY)


	if (RTLSDR)
           find_package(LibRTLSDR)
           if (NOT LIBRTLSDR_FOUND)
               message(FATAL_ERROR "please install librtlsdr")
           endif ()
	   ###include_directories (${RTLSDR_INCLUDE_DIR})

	   include_directories (
	        ./devices/rtlsdr-handler/
	   )

	   set (${objectName}_HDRS
	        ${${objectName}_HDRS}
	        ./devices/rtlsdr-handler/rtlsdr-handler.h
	   )

	   set (${objectName}_SRCS
	        ${${objectName}_SRCS}
	        ./devices/rtlsdr-handler/rtlsdr-handler.cpp
	   )

	   add_definitions (-DHAVE_RTLSDR)
	endif()


#######################################################################
#
#	Here we really start

	include_directories (
	           ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
	           .
	           ./
	           ./main
	           ./devices
	           ./includes
	           ./front-end
	           ./front-end/viterbi-spiral
	           ./support
	           ./tii-handling
	           ./printing
	           /usr/include/
	)

	set (${objectName}_HDRS
	     ${${objectName}_HDRS}
	     ./main/dab-processor.h
	     ./devices/device-handler.h
	     ./devices/device-exceptions.h
	     ./includes/dab-api.h
	     ./includes/dab-constants.h
	     ./includes/bit-extractors.h
             ./includes/time-converter.h
	     ./front-end/phasetable.h
	     ./front-end/freq-interleaver.h
	     ./front-end/timesyncer.h
	     ./front-end/correlator.h
	     ./front-end/freqsyncer.h
	     ./front-end/fic-handler.h
	     ./front-end/fib-table.h
             ./front-end/ensemble.h
             ./front-end/fib-dbtypes.h
             ./front-end/fib-config.h
             ./front-end/fib-decoder.h
	     ./front-end/sample-reader.h
	     ./front-end/tii-detector.h
	     ./front-end/viterbi-spiral/viterbi-spiral.h
	     ./front-end/viterbi-spiral/viterbi-no-sse.h
	     ./support/ringbuffer.h
	     ./support/band-handler.h 
	     ./support/charsets.h
	     ./support/protTables.h
	     ./support/fft-handler.h
	     ./support/dab-params.h	
	     ./support/time-converter.h
	     ./support/dab-tables.h
	     ./support/crc-handlers.h
	     ./tii-handling/tii-handler.h
             ./tii-handling/tiiQueue.h
             ./tii-handling/tii-reader.h
             ./tii-handling/cacheElement.h
	     ./printing/scanner_printer.h
	     ./printing/csv_printer.h
	     ./printing/json_printer.h
	)

	set (${objectName}_SRCS
	     ${${objectName}_SRCS}
	     ./main/main.cpp
	     ./main/dab-processor.cpp
	     ./devices/device-handler.cpp
	     ./front-end/ofdm-decoder.cpp
	     ./front-end/phasetable.cpp
	     ./front-end/freq-interleaver.cpp
	     ./front-end/timesyncer.cpp
	     ./front-end/correlator.cpp
	     ./front-end/freqsyncer.cpp
	     ./front-end/sample-reader.cpp
	     ./front-end/fic-handler.cpp
	     ./front-end/ensemble.cpp
             ./front-end/fib-config.cpp
             ./front-end/fib-decoder.cpp
	     ./front-end/tii-detector.cpp
	     ./front-end/viterbi-spiral/viterbi-spiral.cpp
	     ./front-end/viterbi-spiral/spiral-no-sse.c
	     ./support/band-handler.cpp
	     ./support/charsets.cpp
	     ./support/protTables.cpp
	     ./support/fft-handler.cpp
	     ./support/dab-params.cpp
	     ./support/time-converter.cpp
	     ./support/dab-tables.cpp
	     ./support/crc-handlers.cpp
	     ./tii-handling/tii-handler.cpp
             ./tii-handling/tiiQueue.cpp
             ./tii-handling/tii-reader.cpp
	     ./printing/scanner-printer.cpp
	     ./printing/csv-printer.cpp
	     ./printing/json-printer.cpp
	)

	include_directories (
	          ${FFTW_INCLUDE_DIRS}
	)

#####################################################################

	add_executable (${objectName} 
	                ${${objectName}_SRCS}
	)

	target_link_libraries (${objectName}
	                       ${FFTW3F_LIBRARIES}
	                       ${extraLibs}
	                       ${CMAKE_DL_LIBS}
	)

	INSTALL (TARGETS ${objectName} DESTINATION .)

########################################################################
# Create uninstall target
########################################################################

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
