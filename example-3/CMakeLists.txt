cmake_minimum_required( VERSION 2.8.11 )
set (objectName dab_cmdline-3)
#set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11 -flto")
#set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11 -fsanitize=address")
set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
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
OPTION(SDRPLAY  "Input: SDRPlay"  OFF)
OPTION(AIRSPY   "Input: AirSpy"   OFF)
OPTION(HACKRF	"Input: HACKRF"	  OFF)
OPTION(LIMESDR	"Input: LIMESDR"  OFF)
OPTION(RTL_TCP  "Input: RTL_TCP"  OFF)
OPTION(SDRPLAY_V3  "Input: SDRPlay_V3"  OFF)
OPTION(WAVFILES "Input: WAVFILES" OFF)
OPTION(RAWFILES "Input: RAWFILES" OFF)
OPTION(SERVER	"Create TDC server"	  OFF)
OPTION(X64_DEFINED "optimize for x64/SSE"  OFF)
OPTION(RPI_DEFINED "optimize for ARM/NEON" OFF)

if ( (NOT RTLSDR) AND (NOT SDRPLAY) AND (NOT AirSpy) AND (NOT HACKRF) AND (NOT LIMESDR) AND (NOT RTL_TCP) AND (NOT SDRPLAY_V3) AND (NOT WAVFILES) AND (NOT RAWFILES) )
   message("None of the Input Options selected. Using default SDRPlay")
   set(SDRPlay ON)
endif ()

set (objectName "")

if(RTLSDR)
   if (objectName STREQUAL "")
      set(RTLSDR ON)
      set(objectName dab-rtlsdr-3)
   else ()
      message ("Ignoring second option")
   endif ()
endif ()

if(SDRPLAY)
   if (objectName STREQUAL "")
      set(SDRPLAY ON)
      set(objectName dab-sdrplay-3)
   else ()
      message ("Ignoring second option")
   endif ()
endif ()

if(AIRSPY)
   if (objectName STREQUAL "")
      set(AIRSPY ON)
      set(objectName dab-airspy-3)
   else ()
      message ("Ignoring second option")
   endif ()
endif ()

if(HACKRF)
   if (objectName STREQUAL "")
      set(HACKRF ON)
      set(objectName dab-hackrf-3)
   else ()
      message ("Ignoring second option")
   endif ()
endif ()

if(LIMESDR)
   if (objectName STREQUAL "")
      set(LIMESDR ON)
      set(objectName dab-lime-3)
   else ()
      message ("Ignoring second option")
   endif ()
endif ()

if(RTL_TCP)
   if (objectName STREQUAL "")
      set(RTL_TCP ON)
      set(objectName dab-rtl_tcp-3)
   else ()
      message ("Ignoring second option")
   endif ()
endif ()

if(SDRPLAY_V3)
   if (objectName STREQUAL "")
      set(SDRPLAY_V3 ON)
      set(objectName dab-sdrplay-v3)
   else ()
      message ("Ignoring second option")
   endif ()
endif ()

if(WAVFILES)
   if (objectName STREQUAL "")
      set(WAVFILES ON)
      set(objectName dab-files-3)
   else ()
      message ("Ignoring second option")
   endif ()
endif ()

if(RAWFILES)
   if (objectName STREQUAL "")
      set(RAWFILES ON)
      set(objectName dab-raw-3)
   else ()
      message ("Ignoring second option")
   endif ()
endif ()

if (DEFINED_SERVER)
    add_definitions(-DHAVE_SERVER)
endif ()

add_definitions (-DSTREAMER_OUTPUT)

#########################################################################
	find_package (PkgConfig)

        find_package(FFTW3f)
        if (NOT FFTW3F_FOUND)
            message(FATAL_ERROR "please install FFTW3")
        endif ()

        find_package(Faad)
        if (NOT FAAD_FOUND )
            message(FATAL_ERROR "please install libfaad")
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
	     ../devices/rtl_tcp
	   )
	   set ($(objectName)_HDRS
	        ${${objectName}_HDRS}
	        ../devices/rtl_tcp/rtl_tcp-client.h
           )

	   set (${objectName}_SRCS
	        ${${objectName}_SRCS}
	        ../devices/rtl_tcp/rtl_tcp-client.cpp
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
	     ../devices/sdrplay-handler-v3
	   )

	   set ($(objectName)_HDRS
	        ${${objectName}_HDRS}
	        ../devices/sdrplay-handler-v3/sdrplay-handler-v3.h
           )

	   set (${objectName}_SRCS
	        ${${objectName}_SRCS}
	        ../devices/sdrplay-handler-v3/sdrplay-handler-v3.cpp
	   )

	   add_definitions (-DHAVE_SDRPLAY_V3)
	endif (SDRPLAY_V3)

	if (LIMESDR)
           find_library (LIMELIB LimeSuite)
           if(NOT(LIMELIB))
              message(FATAL_ERROR "please install LimeSuite")
           else(NOT(LIMELIB))
              list (APPEND extraLibs ${LIMELIB})
           endif(NOT(LIMELIB))

           include_directories (
             ../devices/lime-handler
           )

 set ($(objectName)_HDRS
                ${${objectName}_HDRS}
                ../devices/lime-handler/LMS7002M_parameters.h
                ../devices/lime-handler/LimeSuite.h
                ../devices/lime-handler/lime-handler.h
           )

           set (${objectName}_SRCS
                ${${objectName}_SRCS}
                ../devices/lime-handler/lime-handler.cpp
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
	   ###include_directories (${RTLSDR_INCLUDE_DIR})

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

	if (WAVFILES)
	   include_directories (
	        ../devices/wavfiles/
	   )

	   set (${objectName}_HDRS
	        ${${objectName}_HDRS}
	        ../devices/wavfiles/wavfiles.h
	   )

	   set (${objectName}_SRCS
	        ${${objectName}_SRCS}
	        ../devices/wavfiles/wavfiles.cpp
	   )
	   find_package(LibSndFile)
	   if (NOT LIBSNDFILE_FOUND)
	      message(FATAL_ERROR "please install libsndfile")
	   endif ()
	   list(APPEND extraLibs ${LIBSNDFILE_LIBRARY})

	   add_definitions (-DHAVE_WAVFILES)
	endif()

	if (RAWFILES)
           include_directories (
                ../devices/rawfiles/
           )

           set (${objectName}_HDRS
                ${${objectName}_HDRS}
                ../devices/rawfiles/rawfiles.h
           )

           set (${objectName}_SRCS
                ${${objectName}_SRCS}
                ../devices/rawfiles/rawfiles.cpp
           )

           add_definitions (-DHAVE_RAWFILES)
        endif()

	if (HACKRF)
	   find_path (HACKLIB_INCLUDE_DIR
                      NAMES hackrf.h
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

#######################################################################
#
#	Here we really start

	include_directories (
	           ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
	           .
	           ./
	           ./server-thread
	           ../
	           ../devices
	           ../library
	           ../library/includes
	           ../library/includes/ofdm
	           ../library/includes/backend
	           ../library/includes/backend/audio
	           ../library/includes/backend/data
	           ../library/includes/backend/data/mot
	           ../library/includes/backend/data/journaline
	           ../library/includes/support
	           ../library/includes/support/viterbi-spiral
	           /usr/include/
	)

	set (${objectName}_HDRS
	     ${${objectName}_HDRS}
	     ./ringbuffer.h
	     ./server-thread/tcp-server.h
	     ./streamer.h
	     ../dab-api.h
	     ../devices/device-handler.h
	     ../devices/device-exceptions.h
	     ../library/includes/dab-constants.h
	     ../library/includes/dab-processor.h
	     ../library/includes/ofdm/phasereference.h
	     ../library/includes/ofdm/phasetable.h
	     ../library/includes/ofdm/freq-interleaver.h
	     ../library/includes/ofdm/timesyncer.h
	     ../library/includes/ofdm/fic-handler.h
	     ../library/includes/ofdm/fib-processor.cpp
	     ../library/includes/ofdm/sample-reader.h
	     ../library/includes/ofdm/tii-detector.h
	     ../library/includes/backend/firecode-checker.h
	     ../library/includes/backend/backend-base.h
	     ../library/includes/backend/charsets.h
	     ../library/includes/backend/galois.h
	     ../library/includes/backend/reed-solomon.h
	     ../library/includes/backend/msc-handler.h
	     ../library/includes/backend/virtual-backend.h
	     ../library/includes/backend/audio-backend.h
	     ../library/includes/backend/data-backend.h
	     ../library/includes/backend/audio/faad-decoder.h
	     ../library/includes/backend/audio/mp4processor.h 
	     ../library/includes/backend/audio/mp2processor.h 
	     ../library/includes/backend/data/virtual-datahandler.h 
	     ../library/includes/backend/data/tdc-datahandler.h 
	     ../library/includes/backend/data/pad-handler.h 
	     ../library/includes/backend/data/data-processor.h
	     ../library/includes/backend/data/mot/mot-handler.h
	     ../library/includes/backend/data/mot/mot-dir.h
	     ../library/includes/backend/data/mot/mot-object.h
	     ../library/includes/support/band-handler.h
#	     ../library/includes/support/viterbi-handler.h
	     ../library/includes/support/protTables.h
	     ../library/includes/support/protection.h
	     ../library/includes/support/uep-protection.h
	     ../library/includes/support/eep-protection.h
	     ../library/includes/support/fft-handler.h
	     ../library/includes/support/dab-params.h
	     ../library/includes/support/tii_table.h
	     ../library/includes/support/viterbi-spiral/viterbi-spiral.h
	)

	set (${objectName}_SRCS
	     ${${objectName}_SRCS}
	     ./main.cpp
	     ./streamer.cpp
	     ./server-thread/tcp-server.cpp
	     ../devices/device-handler.cpp
	     ../library/dab-api.cpp
	     ../library/src/dab-processor.cpp
	     ../library/src/ofdm/ofdm-decoder.cpp
	     ../library/src/ofdm/phasereference.cpp
	     ../library/src/ofdm/phasetable.cpp
	     ../library/src/ofdm/freq-interleaver.cpp
	     ../library/src/ofdm/timesyncer.cpp
	     ../library/src/ofdm/sample-reader.cpp
	     ../library/src/ofdm/fib-processor.cpp
	     ../library/src/ofdm/fic-handler.cpp
	     ../library/src/ofdm/tii-detector.cpp
	     ../library/src/backend/firecode-checker.cpp
	     ../library/src/backend/backend-base.cpp
	     ../library/src/backend/charsets.cpp
	     ../library/src/backend/galois.cpp
	     ../library/src/backend/reed-solomon.cpp
	     ../library/src/backend/msc-handler.cpp
	     ../library/src/backend/virtual-backend.cpp
	     ../library/src/backend/audio-backend.cpp
	     ../library/src/backend/data-backend.cpp
	     ../library/src/backend/audio/mp4processor.cpp 
	     ../library/src/backend/audio/mp2processor.cpp 
	     ../library/src/backend/data/virtual-datahandler.cpp 
	     ../library/src/backend/data/tdc-datahandler.cpp 
	     ../library/src/backend/data/pad-handler.cpp 
	     ../library/src/backend/data/data-processor.cpp
	     ../library/src/backend/data/mot/mot-handler.cpp
	     ../library/src/backend/data/mot/mot-dir.cpp
	     ../library/src/backend/data/mot/mot-object.cpp
	     ../library/src/support/band-handler.cpp
#	     ../library/src/support/viterbi-handler.cpp
	     ../library/src/support/protTables.cpp
	     ../library/src/support/protection.cpp
	     ../library/src/support/eep-protection.cpp
	     ../library/src/support/uep-protection.cpp
	     ../library/src/support/fft-handler.cpp
	     ../library/src/support/dab-params.cpp
	     ../library/src/support/tii_table.cpp
	     ../library/src/support/viterbi-spiral/viterbi-spiral.cpp
	)

	if (X64_DEFINED)
           set (${objectName}_SRCS
                ${${objectName}_SRCS}
              ../library/src/support/viterbi-spiral/spiral-sse.c
           )
           set (${objectName}_HDRS
                ${${objectName}_HDRS}
             ../library/src/support/viterbi-spiral/spiral-sse.h
           )
           add_definitions (-DSSE_AVAILABLE)
        elseif (RPI_DEFINED)
           set (${objectName}_SRCS
                ${${objectName}_SRCS}
              ../library/src/support/viterbi-spiral/spiral-neon.c
           )
           set (${objectName}_HDRS
                ${${objectName}_HDRS}
             ../library/src/support/viterbi-spiral/spiral-neon.h
           )
           add_definitions (-DNEON_AVAILABLE)
	# compiler options moved below add_executable .. and target specific
	#set (CMAKE_CXX_FLAGS "${CMAKE_XCC_FLAGS} -mcpu=cortex-a7 -mfloat-abi=hard -mfpu=neon-vfpv4")
	#set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mcpu=cortex-a7 -mfloat-abi=hard -mfpu=neon-vfpv4")
        else (X64_DEFINED)
           set (${objectName}_SRCS
                ${${objectName}_SRCS}
              ../library/src/support/viterbi-spiral/spiral-no-sse.c
           )
           set (${objectName}_HDRS
                ${${objectName}_HDRS}
             ../library/src/support/viterbi-spiral/spiral-no-sse.h
           )
        endif (X64_DEFINED)

	include_directories (
	          ${FFTW_INCLUDE_DIRS}
	          ${FAAD_INCLUDE_DIRS}
	          ${SNDFILES_INCLUDE_DIRS}
	)

#####################################################################

	add_executable (${objectName} 
	                ${${objectName}_SRCS}
	)
	if (RPI_DEFINED)
           # compiler options seem changed with gcc versions. gcc 8.3.0 on Raspbian didn't accept -mcpu ..
           # test options with "make VERBOSE=1"
           target_compile_options(${objectName}  PRIVATE -march=armv7-a -mfloat-abi=hard -mfpu=neon-vfpv4 )
	endif()

	target_link_libraries (${objectName}
	                       ${FFTW3F_LIBRARIES}
	                       ${extraLibs}
	                       ${FAAD_LIBRARIES}
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
