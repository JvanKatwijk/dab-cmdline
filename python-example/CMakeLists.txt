cmake_minimum_required( VERSION 2.8.11 )
set (objectName dab_lib)
add_definitions ( -Wall -g -std=c++11 -O3)
#	modify if you want
set (CMAKE_INSTALL_PREFIX .)

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
if(DEFINED AIRSPY)
   set(AIRSPY true)
endif ()

if(DEFINED SDRPLAY)
   set(SDRPLAY true)
endif ()

if(DEFINED RTLSDR)
   set(RTLSDR true)
endif ()

########################################################################
	find_package (PkgConfig)
########################################################################
#	The devices
#
	if (SDRPLAY)
	   find_path (SDRPLAYLIB_INCLUDE_DIR
                      NAMES mirsdrapi-rsp.h
                      PATHS
                      /usr/local/include/
	   )
           include_directories (${SDRPLAYLIB_INCLUDE_DIR})

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

#######################################################################
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

#######################################################################
#
#	Here we really start

	include_directories (
	           ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
	           .
	           ./
	           ../
	           ../library/
	           ../library/includes
	           ../library/includes/ofdm
	           ../library/includes/backend
	           ../library/includes/backend/viterbi_768
	           ../library/includes/backend/audio
	           ../library/includes/backend/data
	           ../library/includes/backend/data/journaline
	           ../library/includes/various
	           /usr/include/
	)

	include_directories (/usr/include/python3.5m)

	set (${objectName}_HDRS
	     ${${objectName}_HDRS}
	     ./band-handler.h
	     ../dab-api.h
	     ../device-handler.h
	     ../ringbuffer.h
	     ../library/includes/dab-class.h
	     ../library/includes/dab-constants.h
	     ../library/includes/ofdm/ofdm-processor.h
	     ../library/includes/ofdm/phasereference.h
	     ../library/includes/ofdm/phasetable.h
	     ../library/includes/ofdm/freq-interleaver.h
	     ../library/includes/backend/viterbi_768/viterbi-768.h
	     ../library/includes/backend/protection.h
	     ../library/includes/backend/uep-protection.h
	     ../library/includes/backend/eep-protection.h
	     ../library/includes/backend/firecode-checker.h
	     ../library/includes/backend/dab-processor.h
	     ../library/includes/backend/dab-virtual.h
	     ../library/includes/backend/charsets.h
	     ../library/includes/backend/galois.h
	     ../library/includes/backend/msc-handler.h
	     ../library/includes/backend/reed-solomon.h
	     ../library/includes/backend/audio/dab-audio.h
	     ../library/includes/backend/audio/faad-decoder.h
	     ../library/includes/backend/audio/mp4processor.h 
	     ../library/includes/backend/audio/mp2processor.h 
	     ../library/includes/backend/data/mot-databuilder.h 
	     ../library/includes/backend/data/virtual-datahandler.h 
	     ../library/includes/backend/data/pad-handler.h 
	     ../library/includes/backend/data/mot-data.h 
	     ../library/includes/backend/data/tdc-datahandler.h 
	     ../library/includes/backend/data/dab-data.h
	     ../library/includes/backend/data/data-processor.h
	     ../library/includes/various/fft.h
	     ../library/includes/various/dab-params.h
	     ../library/includes/various/tii_table.h
	)

	set (${objectName}_SRCS
	     ${${objectName}_SRCS}
	     ./dab-python.cpp
	)

	set (${objectName}_SRCS
	     ${${objectName}_SRCS}
	     ./band-handler.cpp
	     ../devices/device-handler.cpp
	     ../library/dab-api.cpp
	     ../library/src/dab-class.cpp
	     ../library/src/ofdm/ofdm-processor.cpp
	     ../library/src/ofdm/ofdm-decoder.cpp
	     ../library/src/ofdm/phasereference.cpp
	     ../library/src/ofdm/phasetable.cpp
	     ../library/src/ofdm/freq-interleaver.cpp
	     ../library/src/backend/viterbi_768/viterbi-768.cpp
	     ../library/src/backend/viterbi_768/spiral-no-sse.c
	     ../library/src/backend/fic-handler.cpp
	     ../library/src/backend/msc-handler.cpp
	     ../library/src/backend/protection.cpp
	     ../library/src/backend/eep-protection.cpp
	     ../library/src/backend/uep-protection.cpp
	     ../library/src/backend/fib-processor.cpp
	     ../library/src/backend/firecode-checker.cpp
	     ../library/src/backend/dab-virtual.cpp
	     ../library/src/backend/dab-processor.cpp
	     ../library/src/backend/protTables.cpp
	     ../library/src/backend/charsets.cpp
	     ../library/src/backend/galois.cpp
	     ../library/src/backend/reed-solomon.cpp
	     ../library/src/backend/audio/dab-audio.cpp
	     ../library/src/backend/audio/mp4processor.cpp 
	     ../library/src/backend/audio/mp2processor.cpp 
	     ../library/src/backend/data/mot-databuilder.cpp 
	     ../library/src/backend/data/virtual-datahandler.cpp 
	     ../library/src/backend/data/tdc-datahandler.cpp
	     ../library/src/backend/data/pad-handler.cpp 
	     ../library/src/backend/data/mot-data.cpp 
	     ../library/src/backend/data/dab-data.cpp
	     ../library/src/backend/data/data-processor.cpp
	     ../library/src/various/fft.cpp
	     ../library/src/various/dab-params.cpp
	     ../library/src/various/tii_table.cpp
	)

	include_directories (
	          ${SDRPLAY_INCLUDES}
	          ${FFTW_INCLUDE_DIRS}
	          ${PORTAUDIO_INCLUDE_DIRS}
	          ${FAAD_INCLUDE_DIRS}
	          ${SNDFILES_INCLUDE_DIRS}
	)

	list(APPEND extraLibs -lpython3.5m)
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

	INSTALL (TARGETS ${objectName} DESTINATION ./lib)

########################################################################
# Create uninstall target
########################################################################

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
