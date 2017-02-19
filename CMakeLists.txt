cmake_minimum_required( VERSION 2.8.11 )
set (objectName example)
add_definitions ( -Wall)
#	modify if you want
set (CMAKE_INSTALL_PREFIX ../)

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
#       R E A D T H I S
#####################################################################
#	adjust to your likings

#	if you want support for any of these devices, uncomment the line
	set(SDRPLAY true)
	set(AIRSPY true)
	set(DABSTICK true)

#
########################################################################
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

#######################################################################
#
#	Here we really start

	include_directories (
	           ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
	           .
	           ./
	           ./dab-library/includes
	           ./dab-library/includes/ofdm
	           ./dab-library/includes/backend
	           ./dab-library/includes/backend/audio
	           ./dab-library/includes/backend/data
	           ./dab-library/includes/backend/data/journaline
	           ./dab-library/includes/various
	           ./devices
	           /usr/include/
	)

	set (${objectName}_HDRS
	     ./dab-api.h
	     ./dab-library/dab-class.h
	     ./dab-library/includes/dab-constants.h
	     ./dab-library/includes/ofdm/ofdm-processor.h
	     ./dab-library/includes/ofdm/phasereference.h
	     ./dab-library/includes/ofdm/phasetable.h
	     ./dab-library/includes/ofdm/freq-interleaver.h
	     ./dab-library/includes/backend/viterbi.h
	     ./dab-library/includes/backend/protection.h
	     ./dab-library/includes/backend/uep-protection.h
	     ./dab-library/includes/backend/eep-protection.h
	     ./dab-library/includes/backend/firecode-checker.h
	     ./dab-library/includes/backend/dab-processor.h
	     ./dab-library/includes/backend/dab-virtual.h
	     ./dab-library/includes/backend/charsets.h
	     ./dab-library/includes/backend/galois.h
	     ./dab-library/includes/backend/msc-handler.h
	     ./dab-library/includes/backend/reed-solomon.h
	     ./dab-library/includes/backend/audio/dab-audio.h
	     ./dab-library/includes/backend/audio/faad-decoder.h
	     ./dab-library/includes/backend/audio/mp4processor.h 
	     ./dab-library/includes/backend/audio/mp2processor.h 
	     ./dab-library/includes/backend/data/mot-databuilder.h 
	     ./dab-library/includes/backend/data/virtual-datahandler.h 
	     ./dab-library/includes/backend/data/pad-handler.h 
	     ./dab-library/includes/backend/data/mot-data.h 
	     ./dab-library/includes/backend/data/dab-data.h
	     ./dab-library/includes/backend/data/data-processor.h
	     ./dab-library/includes/various/ringbuffer.h
	     ./dab-library/includes/various/fft.h
	     ./dab-library/includes/various/ensemble-handler.h
	     ./devices/virtual-input.h
	)

	set (${objectName}_SRCS
	     ${${objectName}_SRCS}
	     ./dab-library/dab-api.cpp
	     ./dab-library/dab-class.cpp
	     ./dab-library/src/ofdm/ofdm-processor.cpp
	     ./dab-library/src/ofdm/ofdm-decoder.cpp
	     ./dab-library/src/ofdm/phasereference.cpp
	     ./dab-library/src/ofdm/phasetable.cpp
	     ./dab-library/src/ofdm/freq-interleaver.cpp
	     ./dab-library/src/backend/viterbi.cpp
	     ./dab-library/src/backend/fic-handler.cpp
	     ./dab-library/src/backend/msc-handler.cpp
	     ./dab-library/src/backend/protection.cpp
	     ./dab-library/src/backend/eep-protection.cpp
	     ./dab-library/src/backend/uep-protection.cpp
	     ./dab-library/src/backend/fib-processor.cpp
	     ./dab-library/src/backend/firecode-checker.cpp
	     ./dab-library/src/backend/dab-virtual.cpp
	     ./dab-library/src/backend/dab-processor.cpp
	     ./dab-library/src/backend/protTables.cpp
	     ./dab-library/src/backend/charsets.cpp
	     ./dab-library/src/backend/galois.cpp
	     ./dab-library/src/backend/reed-solomon.cpp
	     ./dab-library/src/backend/audio/dab-audio.cpp
	     ./dab-library/src/backend/audio/mp4processor.cpp 
	     ./dab-library/src/backend/audio/mp2processor.cpp 
	     ./dab-library/src/backend/data/mot-databuilder.cpp 
	     ./dab-library/src/backend/data/virtual-datahandler.cpp 
	     ./dab-library/src/backend/data/pad-handler.cpp 
	     ./dab-library/src/backend/data/mot-data.cpp 
	     ./dab-library/src/backend/data/dab-data.cpp
	     ./dab-library/src/backend/data/data-processor.cpp
	     ./dab-library/src/various/fft.cpp
	     ./dab-library/src/various/ensemble-handler.cpp
	     ./devices/virtual-input.cpp
	)

	if (SDRPLAY)
	   find_path (SDRPLAYLIB_INCLUDE_DIR
                      NAMES mirsdrapi-rsp.h
                      PATHS
                      /usr/local/include/
	   )
           include_directories (${SDRPLAYLIB_INCLUDE_DIR})

	   include_directories (
	     ./devices/sdrplay-exp
	   )

	   set ($(objectName)_HDRS
	        ${${objectName}_HDRS}
	        ./devices/sdrplay-exp/sdrplay.h
           )

	   set (${objectName}_SRCS
	        ${${objectName}_SRCS}
	        ./devices/sdrplay-exp/sdrplay.cpp
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
	     ./devices/airspy
	   )

	   set ($(objectName)_HDRS
	        ${${objectName}_HDRS}
	        ./devices/input/airspy/airspy-handler.h
	   )

	   set (${objectName}_SRCS
	        ${${objectName}_SRCS}
	        ./devices/airspy/airspy-handler.cpp
	   )

	   add_definitions (-DHAVE_AIRSPY)
	endif (AIRSPY)
#
	if (DABSTICK)
           find_package(LibRTLSDR)
           if (NOT LIBRTLSDR_FOUND)
               message(FATAL_ERROR "please install librtlsdr")
           endif ()
	   ###include_directories (${RTLSDR_INCLUDE_DIR})

	   include_directories (
	      ./devices/dabstick/
	   )

	   set (${objectName}_HDRS
	        ${${objectName}_HDRS}
	        ./devices/dabstick/dabstick.h 
	   )

	   set (${objectName}_SRCS
	        ${${objectName}_SRCS}
	        ./devices/dabstick/dabstick.cpp
	   )

	   add_definitions (-DHAVE_DABSTICK)
	endif()
#
	include_directories (
	          ${SDRPLAY_INCLUDES}
	          ${FFTW_INCLUDE_DIRS}
	          ${PORTAUDIO_INCLUDE_DIRS}
	          ${FAAD_INCLUDE_DIRS}
	          ${SNDFILES_INCLUDE_DIRS}
	)

#####################################################################
#	Now for the example
#
        find_package(LibSampleRate)
        if (NOT LIBSAMPLERATE_FOUND)
            message(FATAL_ERROR "please install libsamplerate")
        endif ()
        list(APPEND extraLibs ${LIBSAMPLERATE_LIBRARY})

        find_package(Portaudio)
        if (NOT PORTAUDIO_FOUND)
            message(FATAL_ERROR "please install portaudio V19")
        endif ()
        list(APPEND extraLibs ${PORTAUDIO_LIBRARIES})
	
	include_directories (./example)

	set (${objectName}_HDRS
	     ${${objectName}_HDRS}
	     ./example/ringbuffer.h
	     ./example/audio-base.h
	     ./example/audiosink.h
	     ./example/newconverter.h
	)

	set (${objectName}_SRCS
	     ${${objectName}_SRCS}
	     ./example/main.cpp
	     ./example/audio-base.cpp
	     ./example/audiosink.cpp
	     ./example/newconverter.cpp
	)

	add_executable (${objectName}
	                ${${objectName}_SRCS}
#	                ${RSCS}
#	                ${TRS}
	)

	target_link_libraries (${objectName}
	                       ${FFTW3F_LIBRARIES}
	                       ${extraLibs}
	                       ${FAAD_LIBRARIES}
	                       ${CMAKE_DL_LIBS}
	)

	INSTALL (TARGETS ${objectName} DESTINATION ./linux-bin)

########################################################################
# Create uninstall target
########################################################################

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

