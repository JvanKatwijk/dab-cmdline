cmake_minimum_required( VERSION 2.8.11 )
set (objectName dab-cmdline)
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

	add_definitions (-DMOT_BASICS__) 	# use at your own risk
#	add_definitions (-DMSC_DATA__)		# use at your own risk
#
########################################################################
	find_package (PkgConfig)

        find_package(FFTW3f)
        if (NOT FFTW3F_FOUND)
            message(FATAL_ERROR "please install FFTW3")
        endif ()

        find_package(Portaudio)
        if (NOT PORTAUDIO_FOUND)
            message(FATAL_ERROR "please install portaudio V19")
        endif ()
        list(APPEND extraLibs ${PORTAUDIO_LIBRARIES})

        find_package(Faad)
        if (NOT FAAD_FOUND )
            message(FATAL_ERROR "please install libfaad")
        endif ()

        find_package(zlib)
	if (NOT ZLIB_FOUND)
            message(FATAL_ERROR "please install libz")
        endif ()
	list(APPEND extraLibs ${ZLIB_LIBRARY})

        find_package(LibSndFile)
        if (NOT LIBSNDFILE_FOUND)
            message(FATAL_ERROR "please install libsndfile")
        endif ()
        list(APPEND extraLibs ${LIBSNDFILE_LIBRARY})

        find_package(LibSampleRate)
        if (NOT LIBSAMPLERATE_FOUND)
            message(FATAL_ERROR "please install libsamplerate")
        endif ()
        list(APPEND extraLibs ${LIBSAMPLERATE_LIBRARY})

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
	           ${QT5Widgets_INCLUDES}
	           ${QT_QTCORE_INCLUDE_DIR}
	           ${QT_QTGUI_INCLUDE_DIR}
	           .
	           ./
	           ./includes
	           ./includes/ofdm
	           ./includes/backend
	           ./includes/backend/audio
	           ./includes/backend/data
	           ./includes/backend/data/journaline
	           ./includes/various
	           ./includes/output
	           ./src/input
	           /usr/include/
	)

	set (${objectName}_HDRS
	     ./includes/dab-constants.h
	     ./includes/ofdm/ofdm-processor.h
	     ./includes/ofdm/phasereference.h
	     ./includes/ofdm/phasetable.h
	     ./includes/ofdm/freq-interleaver.h
	     ./includes/backend/viterbi.h
	     ./includes/backend/protection.h
<<<<<<< HEAD
	     ./includes/backend/eep-protection.h
	     ./includes/backend/uep-protection.h
=======
	     ./includes/backend/uep-protection.h
	     ./includes/backend/eep-protection.h
>>>>>>> e4f340834bbae03165f30dde02d9eb25c9f203ed
	     ./includes/backend/firecode-checker.h
	     ./includes/backend/dab-processor.h
	     ./includes/backend/dab-virtual.h
	     ./includes/backend/charsets.h
	     ./includes/backend/galois.h
	     ./includes/backend/msc-handler.h
	     ./incluces/backend/reed-solomon.h
	     ./includes/backend/audio/dab-audio.h
	     ./includes/backend/audio/faad-decoder.h
	     ./includes/backend/audio/mp4processor.h 
	     ./includes/backend/audio/mp2processor.h 
	     ./includes/backend/data/mot-databuilder.h 
	     ./includes/backend/data/virtual-datahandler.h 
	     ./includes/backend/data/pad-handler.h 
	     ./includes/backend/data/mot-data.h 
	     ./includes/backend/data/dab-data.h
	     ./includes/backend/data/data-processor.h
	     ./src/input/virtual-input.h
	     ./includes/output/fir-filters.h
	     ./includes/output/audio-base.h
	     ./includes/output/audiosink.h
	     ./includes/output/newconverter.h
	     ./includes/various/fft.h
	     ./includes/various/ringbuffer.h
	     ./includes/various/Xtan2.h
	     ./includes/various/ensemble-handler.h
	)

	set (${objectName}_SRCS
	     ${${objectName}_SRCS}
	     ./main.cpp
	     ./src/ofdm/ofdm-processor.cpp
	     ./src/ofdm/ofdm-decoder.cpp
	     ./src/ofdm/phasereference.cpp
	     ./src/ofdm/phasetable.cpp
	     ./src/ofdm/freq-interleaver.cpp
	     ./src/backend/viterbi.cpp
	     ./src/backend/fic-handler.cpp
	     ./src/backend/msc-handler.cpp
	     ./src/backend/protection.cpp
	     ./src/backend/eep-protection.cpp
	     ./src/backend/uep-protection.cpp
	     ./src/backend/fib-processor.cpp
	     ./src/backend/firecode-checker.cpp
	     ./src/backend/dab-virtual.cpp
	     ./src/backend/dab-processor.cpp
	     ./src/backend/protTables.cpp
	     ./src/backend/charsets.cpp
	     ./src/backend/dab-virtual.cpp 
	     ./src/backend/galois.cpp
	     ./src/backend/reed-solomon.cpp
	     ./src/backend/audio/dab-audio.cpp
	     ./src/backend/audio/mp4processor.cpp 
	     ./src/backend/audio/mp2processor.cpp 
	     ./src/backend/data/mot-databuilder.cpp 
	     ./src/backend/data/virtual-datahandler.cpp 
	     ./src/backend/data/pad-handler.cpp 
	     ./src/backend/data/mot-data.cpp 
	     ./src/backend/data/dab-data.cpp
	     ./src/backend/data/data-processor.cpp
	     ./src/input/virtual-input.cpp
	     ./src/output/audio-base.cpp
	     ./src/output/audiosink.cpp
	     ./src/output/newconverter.cpp
	     ./src/output/fir-filters.cpp
	     ./src/various/fft.cpp
	     ./src/various/Xtan2.cpp
	     ./src/various/ensemble-handler.cpp
	)


	set ($(objectName)_HDRS
	        ${${objectName}_HDRS} ./radio.h)

	set (${objectName}_SRCS
	        ${${objectName}_SRCS} ./radio.cpp)
	

	if (SDRPLAY)
	   find_path (SDRPLAYLIB_INCLUDE_DIR
                      NAMES mirsdrapi-rsp.h
                      PATHS
                      /usr/local/include/
	   )
           include_directories (${SDRPLAYLIB_INCLUDE_DIR})
	   set (${objectName}_UIS
	        ${${objectName}_UIS}
	        ./src/input/sdrplay-exp/sdrplay-widget.ui 
	   )

	   set (${objectName}_MOCS
	        ${${objectName}_MOCS}
	        ./src/input/sdrplay-exp/sdrplay.h
	   )

	   include_directories (
	     ./src/input/sdrplay-exp
	   )

	   set ($(objectName)_HDRS
	        ${${objectName}_HDRS}
	        ./src/input/sdrplay-exp/sdrplay.h
           )

	   set (${objectName}_SRCS
	        ${${objectName}_SRCS}
	        ./src/input/sdrplay-exp/sdrplay.cpp
	   )

	 add_definitions (-DHAVE_SDRPLAY)
	endif (SDRPLAY)
 
	if (AIRSPY)
           find_package(LibAIRSPY)
           if (NOT LIBAIRSPY_FOUND)
               message(FATAL_ERROR "please install airspy library")
           endif ()
	   ### include_directories (${AIRSPYLIB_INCLUDE_DIR})

	   set (${objectName}_UIS
	        ${${objectName}_UIS}
	        ./src/input/airspy/airspy-widget.ui 
	   )

	   set (${objectName}_MOCS
	        ${${objectName}_MOCS}
	        ./src/input/airspy/airspy-handler.h
	   )

	   include_directories (
	     ./src/input/airspy
	   )

	   set ($(objectName)_HDRS
	        ${${objectName}_HDRS}
	        ./src/input/airspy/airspy-handler.h
	   )

	   set (${objectName}_SRCS
	        ${${objectName}_SRCS}
	        ./src/input/airspy/airspy-handler.cpp
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

	   set (${objectName}_UIS
	        ${${objectName}_UIS}
	   )

	   include_directories (
	        ./src/input/dabstick/
	   )

	   set (${objectName}_HDRS
	        ${${objectName}_HDRS}
	        ./src/input/dabstick/dabstick.h 
	   )

	   set (${objectName}_SRCS
	        ${${objectName}_SRCS}
	        ./src/input/dabstick/dabstick.cpp
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


	add_executable (${objectName}
	                ${${objectName}_SRCS}
	                ${UIS}
	                ${RSCS}
	                ${TRS}
	)

	target_link_libraries (${objectName}
	                       ${RTLTCP_lib}
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

