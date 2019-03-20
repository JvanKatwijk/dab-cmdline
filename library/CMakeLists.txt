cmake_minimum_required( VERSION 2.8.11 )
set (objectName dab_lib)
add_definitions ( -Wall -g -std=c++11 -O3)
#    modify if you want
set (CMAKE_INSTALL_PREFIX /usr/local/)

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

#    Here we really start

    include_directories (
               ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
               .
               ./
               ../
               ./includes
               ./includes/ofdm
               ./includes/backend
               ./includes/backend/audio
               ./includes/backend/data
               ./includes/backend/data/mot
               ./includes/backend/data/journaline
               ./includes/support
               /usr/include/
    )

    set (${objectName}_HDRS
         ${${objectName}_HDRS}
         ../dab-api.h
         ../device-handler.h
         ../ringbuffer.h
         ./includes/dab-constants.h
         ./includes/dab-processor.h
         ./includes/ofdm/phasereference.h
         ./includes/ofdm/phasetable.h
         ./includes/ofdm/freq-interleaver.h
         ./includes/ofdm/ofdm-decoder.h
         ./includes/ofdm/sample-reader.h
         ./includes/ofdm/timesyncer.h
         ./includes/ofdm/fic-handler.h
         ./includes/ofdm/fib-processor.h
         ./includes/backend/firecode-checker.h
         ./includes/backend/backend-base.h
         ./includes/backend/charsets.h
         ./includes/backend/galois.h
         ./includes/backend/reed-solomon.h
         ./includes/backend/msc-handler.h
         ./includes/backend/virtual-backend.h
         ./includes/backend/audio-backend.h
         ./includes/backend/data-backend.h
         ./includes/backend/audio/faad-decoder.h
         ./includes/backend/audio/mp4processor.h 
         ./includes/backend/audio/mp2processor.h 
         ./includes/backend/data/virtual-datahandler.h 
         ./includes/backend/data/pad-handler.h 
         ./includes/backend/data/tdc-datahandler.h 
         ./includes/backend/data/data-processor.h
         ./includes/backend/data/mot/mot-handler.h
         ./includes/backend/data/mot/mot-dir.h
         ./includes/backend/data/mot/mot-object.h
         ./includes/support/band-handler.h
         ./includes/support/protTables.h
         ./includes/support/viterbi-handler.h
         ./includes/support/protection.h
         ./includes/support/semaphore.h
         ./includes/support/uep-protection.h
         ./includes/support/eep-protection.h
         ./includes/support/fft_handler.h
         ./includes/support/dab-params.h
         ./includes/support/tii_table.h
    )

    set (${objectName}_SRCS
         ${${objectName}_SRCS}
         ./dab-api.cpp
         ./src/dab-processor.cpp
         ./src/ofdm/ofdm-decoder.cpp
         ./src/ofdm/phasereference.cpp
         ./src/ofdm/phasetable.cpp
         ./src/ofdm/freq-interleaver.cpp
         ./src/ofdm/ofdm-decoder.cpp
         ./src/ofdm/sample-reader.cpp
         ./src/ofdm/timesyncer.cpp
         ./src/ofdm/fic-handler.cpp
         ./src/ofdm/fib-processor.cpp
         ./src/backend/firecode-checker.cpp
         ./src/backend/backend-base.cpp
         ./src/backend/charsets.cpp
         ./src/backend/galois.cpp
         ./src/backend/reed-solomon.cpp
         ./src/backend/msc-handler.cpp
         ./src/backend/virtual-backend.cpp
         ./src/backend/audio-backend.cpp
         ./src/backend/data-backend.cpp
         ./src/backend/audio/mp4processor.cpp 
         ./src/backend/audio/mp2processor.cpp 
         ./src/backend/data/virtual-datahandler.cpp 
         ./src/backend/data/tdc-datahandler.cpp
         ./src/backend/data/pad-handler.cpp 
         ./src/backend/data/data-processor.cpp
         ./src/backend/data/mot/mot-handler.cpp 
         ./src/backend/data/mot/mot-dir.cpp 
         ./src/backend/data/mot/mot-object.cpp
         ./src/support/band-handler.cpp
         ./src/support/viterbi-handler.cpp
         ./src/support/protection.cpp
         ./src/support/protTables.cpp
         ./src/support/eep-protection.cpp
         ./src/support/uep-protection.cpp
         ./src/support/fft_handler.cpp
         ./src/support/dab-params.cpp
         ./src/support/tii_table.cpp
    )

    include_directories (
              ${FFTW_INCLUDE_DIRS}
              ${PORTAUDIO_INCLUDE_DIRS}
              ${FAAD_INCLUDE_DIRS}
              ${SNDFILES_INCLUDE_DIRS}
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

    INSTALL (TARGETS ${objectName} DESTINATION ./lib)

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
