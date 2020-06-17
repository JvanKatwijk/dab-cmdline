cmake_minimum_required( VERSION 2.8.11 )
set (objectName dab_cmdline-7)
#set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11 -flto")
#set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11 -g")
set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -g")
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

if(DEFINED X64_DEFINED)
   set(X64_DEFINED true)
endif ()

if(DEFINED RPI_DEFINED)
   set(RPI_DEFINED true)
endif ()

#add_definitions (-D__THREADED_DECODING)
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


        find_package(LibSampleRate)
        if (NOT LIBSAMPLERATE_FOUND)
            message(FATAL_ERROR "please install libsamplerate")
        endif ()
        list(APPEND extraLibs ${LIBSAMPLERATE_LIBRARY})

#########################################################################
        find_package (PkgConfig)

##########################################################################
#	The device
#

	include_directories (
	  ../devices/stdin-handler
	)
	set ($(objectName)_HDRS
	     ${${objectName}_HDRS}
	    ../devices/stdin-handler/stdin-handler.h
        )

	set (${objectName}_SRCS
	     ${${objectName}_SRCS}
	    ../devices/stdin-handler/stdin-handler.cpp
	)


        find_package(Portaudio)
        if (NOT PORTAUDIO_FOUND)
            message(FATAL_ERROR "please install portaudio V19")
        endif ()
        list(APPEND extraLibs ${PORTAUDIO_LIBRARIES})

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
	     ./audio-base.h
	     ./audiosink.h
	     ./filesink.h
	     ./newconverter.h
	     ../dab-api.h
	     ../devices/device-handler.h
	     ../library/includes/dab-constants.h
	     ../library/includes/dab-processor.h
	     ../library/includes/ofdm/phasereference.h
	     ../library/includes/ofdm/phasetable.h
	     ../library/includes/ofdm/freq-interleaver.h
	     ../library/includes/ofdm/timesyncer.h
	     ../library/includes/ofdm/fic-handler.h
	     ../library/includes/ofdm/fib-processor.cpp
	     ../library/includes/ofdm/sample-reader.h
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
	     ../library/includes/backend/data/mot/mot-handler.h 
	     ../library/includes/backend/data/mot/mot-dir.h 
	     ../library/includes/backend/data/mot/mot-object.h 
	     ../library/includes/backend/data/data-processor.h
	     ../library/includes/support/band-handler.h
#	     ../library/includes/support/viterbi-handler.h
	     ../library/includes/support/protTables.h
	     ../library/includes/support/protection.h
	     ../library/includes/support/uep-protection.h
	     ../library/includes/support/eep-protection.h
	     ../library/includes/support/fft_handler.h
	     ../library/includes/support/dab-params.h
	     ../library/includes/support/tii_table.h
	     ../library/includes/support/viterbi-spiral/viterbi-spiral.h
	)

	set (${objectName}_SRCS
	     ${${objectName}_SRCS}
	     ./main.cpp
	     ./audio-base.cpp
	     ./audiosink.cpp
	     ./filesink.cpp
	     ./newconverter.cpp
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
	     ../library/src/backend/data/mot/mot-handler.cpp 
	     ../library/src/backend/data/mot/mot-dir.cpp 
	     ../library/src/backend/data/mot/mot-object.cpp 
	     ../library/src/backend/data/data-processor.cpp
	     ../library/src/support/band-handler.cpp
#	     ../library/src/support/viterbi-handler.cpp
	     ../library/src/support/protTables.cpp
	     ../library/src/support/protection.cpp
	     ../library/src/support/eep-protection.cpp
	     ../library/src/support/uep-protection.cpp
	     ../library/src/support/fft_handler.cpp
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

#
	include_directories (
	          ${FFTW_INCLUDE_DIRS}
	          ${PORTAUDIO_INCLUDE_DIRS}
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
