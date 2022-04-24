#
#
# CMAKE file for YASDI
#
#

CMAKE_MINIMUM_REQUIRED(VERSION 2.4)

#needed for CMAKE version 2.6 to supress warnings 
#when linking against wsock32 lib...
IF(COMMAND CMAKE_POLICY)
   CMAKE_POLICY(SET CMP0003 OLD)
ENDIF(COMMAND CMAKE_POLICY)



#
# Define some build options
#
OPTION( YASDI_DRIVER_SERIAL  "Building the Serial driver"                 on)
OPTION( YASDI_DRIVER_IP      "Building the Ethernet/UDP driver"           off)
OPTION( YASDI_DRIVER_TCP     "Building the TCP driver"                    off)
OPTION( YASDI_UNITTEST       "Building the software unit tests"          off)
OPTION( YASDI_DEBUG_OUTPUT   "Building YASDI with debug output"           off)
#OPTION( YASDI_DRIVER_BT      "Building the Bluetooth driver"             off)
#OPTION( YASDI_CPPMASTERLIB   "Building the c++ language mapping library" off)

# Hide some options
MARK_AS_ADVANCED(YASDI_DEBUG_OUTPUT)
MARK_AS_ADVANCED(YASDI_DRIVER_TCP)
MARK_AS_ADVANCED(YASDI_UNITTEST)
MARK_AS_ADVANCED(EXECUTABLE_OUTPUT_PATH)
MARK_AS_ADVANCED(LIBRARY_OUTPUT_PATH)

# Use of c++ or not
if (YASDI_CPPMASTERLIB)
   PROJECT( yasdi C CXX)
else (YASDI_CPPMASTERLIB)
   PROJECT(yasdi C)
endif (YASDI_CPPMASTERLIB)

# Experimental use of testing code here
ENABLE_TESTING()
ADD_TEST(SimpleTest ${EXECUTABLE_OUTPUT_PATH}/unittest Hello)


#
# Main Version from YASDI
#
SET(LIB_YASDI_VER1 1) #Major version
SET(LIB_YASDI_VER2 8) #second major version
SET(LIB_YASDI_VER3 1) #patch number
SET(LIB_YASDI_VER4 9) #build number
SET(YASDI_VERSION ${LIB_YASDI_VER1}.${LIB_YASDI_VER2}.${LIB_YASDI_VER3} )
SET(YASDI_VERSION_FULL ${LIB_YASDI_VER1}.${LIB_YASDI_VER2}.${LIB_YASDI_VER3}Build${LIB_YASDI_VER4} )
MESSAGE(STATUS "Building YASDI Version '"
        ${YASDI_VERSION_FULL} "' (Copyright by SMA Solar Technology AG)")


#
# The source of YASDI core library 
#
set(yasdisrc   ../../smalib/getini.c 
               ../../libs/libyasdi.c
               ../../libs/libyasdi.h               
               ../../core/driver_layer.c 
               ../../core/byteorder.c
               ../../core/netpacket.c 
               ../../core/prot_layer.c
               ../../core/smadata_layer.c 
               ../../core/scheduler.c
               ../../core/defractionizer.c 
               ../../core/router.c
               ../../core/timer.c
               ../../core/tools.c 
               ../../core/repository.c 
               ../../core/fractionizer.c 
               ../../core/statistic_writer.c 
               ../../core/minqueue.c
               ../../core/minmap.c
               ../../core/mempool.c
               ../../core/iorequest.c
               ../../protocol/sunnynet.c 
               ../../protocol/smanet.c
			   ../../smalib/smadef.h
			   ../../include/os.h
)

#
# The source of YASDI master library 
#
set(yasdimaster_src 
               ../../libs/libyasdimaster.c
               ../../libs/libyasdimaster.h               
               ../../master/main.c
               ../../master/objman.c
               ../../master/netdevice.c
               ../../master/plant.c
               ../../master/netchannel.c
               ../../master/stateconfig.c
               ../../master/chanvalrepo.c
               ../../master/ysecurity.c
               ../../master/busevents.c
               ../../master/statereadchan.c
               ../../master/statewritechan.c
               ../../master/stateident.c
               ../../master/statedetection.c
               ../../master/mastercmd.c
)

#
# The IP/UDP Driver
#
set(ipdriver_src ../../driver/ip_generic.c)
if (WIN32)
   set( ipdriver_add_lib wsock32 )
endif (WIN32) 



#
# Build the serial driver (for the specific operating system)
#
if (WIN32)
   set(driv_ser_src ../../driver/serial_windows.c)
endif (WIN32) 
if (UNIX)
   set(driv_ser_src ../../driver/serial_posix.c)
   set(needed_libs dl pthread)
   # rt (=> for "aio")
endif (UNIX)


#
# The Bluetooth driver
#
if(YASDI_DRIVER_BT)
   INCLUDE(bluetooth.cmake)
endif(YASDI_DRIVER_BT)


#
# The pure TCP driver
#
if(YASDI_DRIVER_TCP)
   INCLUDE(tcp_driver.cmake)
endif(YASDI_DRIVER_TCP)

   


#
# The optional c++ language mapping for master lib
#
set(cpp_lib_src ../../libs++/libyasdimaster++.cpp)


#
# The demo program "yasdishell"
#
set(shell_src ../../shell/CommonShellUIMain.c)


#
# The include directories...
#
include_directories(
            ../../include 
            ../../os 
            ../../core 
            ../../include 
            ../../smalib 
            ../../os 
            ../../protocol 
            ../../master 
            ../../libs 
            ../../libs++
            ./incprj 
)



#
# Link the right operating object module to the HAL layer 
#
if (UNIX)
   set(yasdisrc ${yasdisrc} ../../os/os_linux.c)
endif (UNIX)
if (WIN32)
   set(yasdisrc ${yasdisrc} ../../os/os_windows.c)
endif (WIN32)


#
# Unit tests
#
set (unittest_src ../../testunits/unittestmain.c)




#
# Do some GCC compiler specific things...
#
if (CMAKE_COMPILER_IS_GNUCC)
   # On GNUC systems, warn everything in one line (do not wrap line to use in eclipse)...
   set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -Wall -fmessage-length=0 ")
   if (APPLE)
      set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -fno-common") # needed! 
   endif (APPLE)
   if (WIN32)
      #
      # There are some problems compiling RC files with mingw on windows (with "windres")
      # So add windows RC support here...
      #
      ENABLE_LANGUAGE(RC)
      SET(CMAKE_RC_COMPILER "windres")
      FIND_PROGRAM( H windres )
      IF(NOT H)
         MESSAGE(FATAL "Can't find 'windres' in path. Please reinstall Mingw!")
      endif(NOT H)
      SET(CMAKE_RC_COMPILER ${H} )
      #MESSAGE( "RC Compiler:" ${CMAKE_RC_COMPILER} )
      SET(CMAKE_RC_COMPILE_OBJECT
          "<CMAKE_RC_COMPILER> --output-format=COFF -I=${CMAKE_CURRENT_SOURCE_DIR}/../../include <FLAGS> -i <SOURCE> -o <OBJECT> ")

      # Export all functions with "_" prefix...
      SET(CMAKE_LD_FLAGS "-Wl,--add-stdcall-alias -Wl,--enable-stdcall-fixup")
      # On Windows with GCC the version info seams not to work. Do it by hand here
      SET(version_info_rc ../../include/yasdi_version.rc)
      # shared libs are prefixed with "lib" on windows and gcc. Why? I don't want them. Remove prefix..
      SET( CMAKE_SHARED_LIBRARY_PREFIX "")

      #I want to export only some functions not all
      SET( yasdi_lib_def        ${CMAKE_CURRENT_SOURCE_DIR}/../../libs/libyasdi.def )
      SET( yasdi_master_lib_def ${CMAKE_CURRENT_SOURCE_DIR}/../../libs/libyasdimaster.def )
      
   endif(WIN32)   
endif(CMAKE_COMPILER_IS_GNUCC)


# set version into source (the file "ihnclude/version.h"...
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/../../include/version.h.in 
               ${CMAKE_CURRENT_SOURCE_DIR}/../../include/version.h
                 @ONLY )

# configure main project file to make debug outputs at runtime or not...
IF (YASDI_DEBUG_OUTPUT)
   set (YASDI_DEBUG_OUTPUT "1")
ELSE (YASDI_DEBUG_OUTPUT)   
   set (YASDI_DEBUG_OUTPUT "0")
ENDIF (YASDI_DEBUG_OUTPUT)   
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/incprj/project.h.in 
               ${CMAKE_CURRENT_SOURCE_DIR}/incprj/project.h
               @ONLY )




#
# Build the libraries
#
add_library(yasdi            SHARED ${yasdisrc}        ${version_info_rc})
TARGET_LINK_LIBRARIES(yasdi  ${needed_libs}   )
SET_TARGET_PROPERTIES(yasdi PROPERTIES LINKER_LANGUAGE C)

add_library(yasdimaster      SHARED ${yasdimaster_src} ${version_info_rc} )
TARGET_LINK_LIBRARIES(yasdimaster yasdi)
SET_TARGET_PROPERTIES(yasdimaster PROPERTIES LINKER_LANGUAGE C)

add_library(yasdi_drv_ip     SHARED ${ipdriver_src}    ${version_info_rc})
TARGET_LINK_LIBRARIES(yasdi_drv_ip yasdi ${ipdriver_add_lib} )
SET_TARGET_PROPERTIES(yasdi_drv_ip PROPERTIES LINKER_LANGUAGE C)

add_library(yasdi_drv_serial SHARED ${driv_ser_src}    ${version_info_rc})
TARGET_LINK_LIBRARIES(yasdi_drv_serial yasdi)
SET_TARGET_PROPERTIES(yasdi_drv_serial PROPERTIES LINKER_LANGUAGE C)

add_executable(yasdishell           ${shell_src}       ${version_info_rc})
TARGET_LINK_LIBRARIES(yasdishell yasdimaster)
SET_TARGET_PROPERTIES(yasdishell PROPERTIES LINKER_LANGUAGE C)

if (YASDI_CPPMASTERLIB)
   add_library(yasdimaster++ SHARED ${cpp_lib_src}     ${version_info_rc})
   TARGET_LINK_LIBRARIES(yasdimaster++ yasdimaster)
endif(YASDI_CPPMASTERLIB)

if (YASDI_UNITTEST)
   add_executable(unittest          ${unittest_src} )
   TARGET_LINK_LIBRARIES(unittest yasdimaster)
   SET_TARGET_PROPERTIES(unittest PROPERTIES LINKER_LANGUAGE C)
endif (YASDI_UNITTEST)


# Add verion infos to the libs...(seams not work with mingw 3.4 on windows)
SET_TARGET_PROPERTIES( yasdi            PROPERTIES VERSION  ${YASDI_VERSION} SOVERSION ${LIB_YASDI_VER1} )
SET_TARGET_PROPERTIES( yasdimaster      PROPERTIES VERSION  ${YASDI_VERSION} SOVERSION ${LIB_YASDI_VER1} )
SET_TARGET_PROPERTIES( yasdi_drv_ip     PROPERTIES VERSION  ${YASDI_VERSION} SOVERSION ${LIB_YASDI_VER1} )
SET_TARGET_PROPERTIES( yasdi_drv_serial PROPERTIES VERSION  ${YASDI_VERSION} SOVERSION ${LIB_YASDI_VER1} )

#Test: Build MacOSX Framework
#SET_TARGET_PROPERTIES( yasdimaster      PROPERTIES FRAMEWORK TRUE )



# Add the Windows def files to export only some functions of the libs
IF(MINGW)
   SET_TARGET_PROPERTIES( yasdi            PROPERTIES LINK_FLAGS ${yasdi_lib_def} )
   SET_TARGET_PROPERTIES( yasdimaster      PROPERTIES LINK_FLAGS ${yasdi_master_lib_def} )
ENDIF(MINGW)


#
# Install roules
#
INSTALL(TARGETS yasdishell yasdi yasdimaster yasdi_drv_ip yasdi_drv_serial
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
#  FRAMEWORK DESTINATION "/Library/Frameworks"
)
