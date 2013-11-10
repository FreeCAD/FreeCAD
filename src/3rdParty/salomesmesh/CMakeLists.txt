# SGEOM cmake build file
#PROJECT(SMESH Fortran CXX)

SET(SMESH_VERSION_MAJOR 5)
SET(SMESH_VERSION_MINOR 1)
SET(SMESH_VERSION_PATCH 2)
SET(SMESH_VERSION_TWEAK 2)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

include_directories(
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${NGLIB_INCLUDE_DIR}
    ${NETGEN_INCLUDE_DIRS}
    src/SMDS
    src/Driver
    src/DriverUNV
    src/DriverDAT
    src/DriverSTL
    src/StdMeshers
    inc
)
link_directories(${OCC_LIBRARY_DIR})

if(MSVC)
	if(FREECAD_BUILD_FEM_NETGEN)
		set(SMESH_LIBS
			debug     MSVCRTD.LIB
			debug     MSVCPRTD.LIB
			optimized MSVCRT.LIB
			optimized MSVCPRT.LIB
			Rpcrt4.lib
			${NGLIB_LIBRARIES}
			${NGLIB_DEBUG_LIBRARIES}
			${OCC_LIBRARIES}
			${OCC_DEBUG_LIBRARIES}
			${OCC_OCAF_DEBUG_LIBRARIES}
			${OCC_OCAF_LIBRARIES}
		)
	else(FREECAD_BUILD_FEM_NETGEN)
		set(SMESH_LIBS
			debug     MSVCRTD.LIB
			debug     MSVCPRTD.LIB
			optimized MSVCRT.LIB
			optimized MSVCPRT.LIB
			Rpcrt4.lib
			${OCC_LIBRARIES}
			${OCC_DEBUG_LIBRARIES}
			${OCC_OCAF_DEBUG_LIBRARIES}
			${OCC_OCAF_LIBRARIES}
		)
	endif(FREECAD_BUILD_FEM_NETGEN)
	
else(MSVC)
    if(FREECAD_BUILD_FEM_NETGEN)
        set(SMESH_LIBS
            ${NGLIB_LIBRARIES}
            ${OCC_LIBRARIES}
            ${OCC_OCAF_LIBRARIES}
        )
    else(FREECAD_BUILD_FEM_NETGEN)
        set(SMESH_LIBS
            ${OCC_LIBRARIES}
            ${OCC_OCAF_LIBRARIES}
        )
    endif(FREECAD_BUILD_FEM_NETGEN)
endif(MSVC)


#SET(CMAKE_BUILD_TYPE "Release") # By default, build in release mode
#SET(BUILD_SHARED_LIBS ON)
#SET(LIBRARY_OUTPUT_PATH cmake-build)
SET(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
#INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
#INCLUDE_DIRECTORIES(inc)

#############
# FindBoost #
#############
#FIND_PACKAGE(Boost REQUIRED)
#MESSAGE("-- Boost found: ${Boost_FOUND}")
#INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})
#MESSAGE("Popo: ${Boost_INCLUDE_DIR}")

################
# Check Fortran#
################
INCLUDE(CMakeDetermineFortranCompiler)

########################################
# OpenCASCADE include and lib location #
########################################
# Find OpenCASCADE library, set include and lib path
#SET(OCC_INCLUDE_PATH ${OCC_INCLUDE_DIR})
#SET(OCC_LIB_PATH ${OCC_LIBRARY_DIR})

# Tells CMake where are the required headers
#INCLUDE_DIRECTORIES(${OCC_INCLUDE_PATH})
#LINK_DIRECTORIES(${OCC_LIB_PATH})


############################
# Check 32/64 bit platform #
############################
IF (${CMAKE_SIZEOF_VOID_P} MATCHES "8") # It is 64bit, 32 bit systems returns 4
    ADD_DEFINITIONS(-D_OCC64)
    IF(CMAKE_COMPILER_IS_GNUCXX)
        ADD_DEFINITIONS(-DPCLINUX64)
    ENDIF(CMAKE_COMPILER_IS_GNUCXX)
    #MESSAGE("-- Build 64bit")
ELSE (${CMAKE_SIZEOF_VOID_P} MATCHES "8")
    #MESSAGE("-- Build 32bit")
ENDIF(${CMAKE_SIZEOF_VOID_P} MATCHES "8")

######################################################
# Configure Build settings according to the platform #
######################################################
IF(UNIX)
    # Same settings are used for both MacOSX and Unix/Linux
    ADD_DEFINITIONS(-DHAVE_CONFIG_H -DHAVE_LIMITS_H -DCSFDB -DLIN -DOCC_CONVERT_SIGNALS)
ELSE(UNIX)
  IF(WIN32)
    ######### Windows ###########
    #MESSAGE("-- Windows system detected")
    IF(MSVC)
        ADD_DEFINITIONS(-DWNT -DWIN32 -D_WINDOWS -DCSFDB /wd4290 /wd4251 /wd4018 /wd4800 /wd4996 /wd4244 /wd4806 /wd4275)
    ELSE(MSVC)
        ADD_DEFINITIONS(-DWNT -DWIN32 -D_WINDOWS -DCSFDB)
    ENDIF(MSVC)
  ELSE(WIN32)
    MESSAGE("Unknown platform")
  ENDIF(WIN32)
ENDIF(UNIX)

#########
# SMDS #
#########
FILE(GLOB SMDS_source_files src/SMDS/*.cpp inc/SMDS*.hxx)
ADD_LIBRARY(SMDS SHARED ${SMDS_source_files})
TARGET_LINK_LIBRARIES(SMDS ${SMESH_LIBS})

if(MSVC)
    set_target_properties(SMDS PROPERTIES COMPILE_FLAGS "-DSMDS_EXPORTS")
    set_target_properties(SMDS PROPERTIES DEBUG_OUTPUT_NAME "SMDSd")
    set_target_properties(SMDS PROPERTIES OUTPUT_NAME "SMDS")
    set_target_properties(SMDS PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set_target_properties(SMDS PROPERTIES PREFIX "../")
elseif(MINGW)
    set_target_properties(SMDS PROPERTIES COMPILE_FLAGS "-DSMDS_EXPORTS")
    set_target_properties(SMDS PROPERTIES OUTPUT_NAME "SMDS")
    set_target_properties(SMDS PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set_target_properties(SMDS PROPERTIES PREFIX "")
else(MSVC)
    set_target_properties(SMDS PROPERTIES OUTPUT_NAME "SMDS")
    set_target_properties(SMDS PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set_target_properties(SMDS PROPERTIES PREFIX "")
    set_target_properties(SMDS PROPERTIES INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib)
endif(MSVC)

##########
# Driver #
##########
FILE(GLOB Driver_source_files src/Driver/*.cpp inc/Driver_*.hxx)
ADD_LIBRARY(Driver SHARED ${Driver_source_files})
TARGET_LINK_LIBRARIES(Driver ${SMESH_LIBS})

if(MSVC)
    set_target_properties(Driver PROPERTIES COMPILE_FLAGS "-DMESHDRIVER_EXPORTS")
    set_target_properties(Driver PROPERTIES DEBUG_OUTPUT_NAME "Driverd")
    set_target_properties(Driver PROPERTIES OUTPUT_NAME "Driver")
    set_target_properties(Driver PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set_target_properties(Driver PROPERTIES PREFIX "../")
elseif(MINGW)
    set_target_properties(Driver PROPERTIES COMPILE_FLAGS "-DMESHDRIVER_EXPORTS")
    set_target_properties(Driver PROPERTIES OUTPUT_NAME "Driver")
    set_target_properties(Driver PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set_target_properties(Driver PROPERTIES PREFIX "")
else(MSVC)
    set_target_properties(Driver PROPERTIES OUTPUT_NAME "Driver")
    set_target_properties(Driver PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set_target_properties(Driver PROPERTIES PREFIX "")
    set_target_properties(Driver PROPERTIES INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib)
endif(MSVC)



#IF(WIN32)
#  SET_TARGET_PROPERTIES(Driver PROPERTIES COMPILE_FLAGS "-DMESHDRIVER_EXPORTS")
#ENDIF(WIN32)
#IF(MINGW)
#  SET_TARGET_PROPERTIES(Driver PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
#ENDIF(MINGW)

#############
# DriverSTL #
#############
FILE(GLOB DriverSTL_source_files src/DriverSTL/*.cpp inc/DriverSTL*.h*)
ADD_LIBRARY(DriverSTL SHARED ${DriverSTL_source_files})
TARGET_LINK_LIBRARIES(DriverSTL ${SMESH_LIBS} Driver SMDS)

if(MSVC)
    set_target_properties(DriverSTL PROPERTIES COMPILE_FLAGS "-DMESHDRIVERSTL_EXPORTS")
    set_target_properties(DriverSTL PROPERTIES DEBUG_OUTPUT_NAME "DriverSTLd")
    set_target_properties(DriverSTL PROPERTIES OUTPUT_NAME "DriverSTL")
    set_target_properties(DriverSTL PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set_target_properties(DriverSTL PROPERTIES PREFIX "../")
elseif(MINGW)
    set_target_properties(DriverSTL PROPERTIES COMPILE_FLAGS "-DMESHDRIVERSTL_EXPORTS")
    set_target_properties(DriverSTL PROPERTIES OUTPUT_NAME "DriverSTL")
    set_target_properties(DriverSTL PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set_target_properties(DriverSTL PROPERTIES PREFIX "")
else(MSVC)
    set_target_properties(DriverSTL PROPERTIES OUTPUT_NAME "DriverSTL")
    set_target_properties(DriverSTL PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set_target_properties(DriverSTL PROPERTIES PREFIX "")
    set_target_properties(DriverSTL PROPERTIES INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib)
endif(MSVC)


#TARGET_LINK_LIBRARIES(DriverSTL TKernel TKMath Driver SMDS)
#IF(WIN32)
#  SET_TARGET_PROPERTIES(DriverSTL PROPERTIES COMPILE_FLAGS "-DMESHDRIVERSTL_EXPORTS")
#ENDIF(WIN32)
#IF(MINGW)
#  SET_TARGET_PROPERTIES(DriverSTL PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
#ENDIF(MINGW)

#############
# DriverDAT #
#############
FILE(GLOB DriverDAT_source_files src/DriverDAT/*.cpp inc/DriverDAT*.h*)
ADD_LIBRARY(DriverDAT SHARED ${DriverDAT_source_files})
TARGET_LINK_LIBRARIES(DriverDAT ${SMESH_LIBS} SMDS Driver)

if(MSVC)
    set_target_properties(DriverDAT PROPERTIES COMPILE_FLAGS "-DMESHDRIVERDAT_EXPORTS")
    set_target_properties(DriverDAT PROPERTIES DEBUG_OUTPUT_NAME "DriverDATd")
    set_target_properties(DriverDAT PROPERTIES OUTPUT_NAME "DriverDAT")
    set_target_properties(DriverDAT PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set_target_properties(DriverDAT PROPERTIES PREFIX "../")
elseif(MINGW)
    set_target_properties(DriverDAT PROPERTIES COMPILE_FLAGS "-DMESHDRIVERDAT_EXPORTS")
    set_target_properties(DriverDAT PROPERTIES OUTPUT_NAME "DriverDAT")
    set_target_properties(DriverDAT PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set_target_properties(DriverDAT PROPERTIES PREFIX "")
else(MSVC)
    set_target_properties(DriverDAT PROPERTIES OUTPUT_NAME "DriverDAT")
    set_target_properties(DriverDAT PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set_target_properties(DriverDAT PROPERTIES PREFIX "")
    set_target_properties(DriverDAT PROPERTIES INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib)
endif(MSVC)



#IF(WIN32)
# SET_TARGET_PROPERTIES(DriverDAT PROPERTIES COMPILE_FLAGS "-DMESHDRIVERDAT_EXPORTS")
#ENDIF(WIN32)
#IF(MINGW)
#  SET_TARGET_PROPERTIES(DriverDAT PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
#ENDIF(MINGW)

#############
# DriverUNV #
#############
FILE(GLOB DriverUNV_source_files src/DriverUNV/*.cpp inc/DriverUNV*.h*)
ADD_LIBRARY(DriverUNV SHARED ${DriverUNV_source_files})
TARGET_LINK_LIBRARIES(DriverUNV ${SMESH_LIBS} SMESHDS SMDS Driver)

if(MSVC)
    set_target_properties(DriverUNV PROPERTIES COMPILE_FLAGS "-DMESHDRIVERUNV_EXPORTS")
    set_target_properties(DriverUNV PROPERTIES DEBUG_OUTPUT_NAME "DriverUNVd")
    set_target_properties(DriverUNV PROPERTIES OUTPUT_NAME "DriverUNV")
    set_target_properties(DriverUNV PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set_target_properties(DriverUNV PROPERTIES PREFIX "../")
elseif(MINGW)
    set_target_properties(DriverUNV PROPERTIES COMPILE_FLAGS "-DMESHDRIVERUNV_EXPORTS")
    set_target_properties(DriverUNV PROPERTIES OUTPUT_NAME "DriverUNV")
    set_target_properties(DriverUNV PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set_target_properties(DriverUNV PROPERTIES PREFIX "")
else(MSVC)
    set_target_properties(DriverUNV PROPERTIES OUTPUT_NAME "DriverUNV")
    set_target_properties(DriverUNV PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set_target_properties(DriverUNV PROPERTIES PREFIX "")
    set_target_properties(DriverUNV PROPERTIES INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib)
endif(MSVC)



#IF(WIN32)
#  SET_TARGET_PROPERTIES(DriverUNV PROPERTIES COMPILE_FLAGS "-DMESHDRIVERUNV_EXPORTS")
#ENDIF(WIN32)
#IF(MINGW)
#  SET_TARGET_PROPERTIES(DriverUNV PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
#ENDIF(MINGW)

###########
# SMESHDS #
###########
FILE(GLOB SMESHDS_source_files src/SMESHDS/*.cpp inc/SMESHDS_*.h*)
ADD_LIBRARY(SMESHDS SHARED ${SMESHDS_source_files})
TARGET_LINK_LIBRARIES(SMESHDS ${SMESH_LIBS} SMDS )

if(MSVC)
    set_target_properties(SMESHDS PROPERTIES DEBUG_OUTPUT_NAME "SMESHDSd")
    set_target_properties(SMESHDS PROPERTIES OUTPUT_NAME "SMESHDS")
    set_target_properties(SMESHDS PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set_target_properties(SMESHDS PROPERTIES PREFIX "../")
elseif(MINGW)
    set_target_properties(SMESHDS PROPERTIES OUTPUT_NAME "SMESHDS")
    set_target_properties(SMESHDS PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set_target_properties(SMESHDS PROPERTIES PREFIX "")
else(MSVC)
    set_target_properties(SMESHDS PROPERTIES OUTPUT_NAME "SMESHDS")
    set_target_properties(SMESHDS PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set_target_properties(SMESHDS PROPERTIES PREFIX "")
    set_target_properties(SMESHDS PROPERTIES INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib)
endif(MSVC)



#IF(MINGW)
#  SET_TARGET_PROPERTIES(SMESHDS PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
#ENDIF(MINGW)

#########
# SMESH #
#########
FILE(GLOB SMESH_source_files src/SMESH/*.cpp src/Controls/*.cpp inc/SMESH_*.h*)
ADD_LIBRARY(SMESH SHARED ${SMESH_source_files})
TARGET_LINK_LIBRARIES(SMESH SMDS SMESHDS  Driver DriverSTL DriverDAT DriverUNV ${SMESH_LIBS})
 
if(MSVC)
    set_target_properties(SMESH PROPERTIES COMPILE_FLAGS "-DSMESH_EXPORTS -DSMESHCONTROLS_EXPORTS -DSMESHimpl_EXPORTS")
    set_target_properties(SMESH PROPERTIES DEBUG_OUTPUT_NAME "SMESHd")
    set_target_properties(SMESH PROPERTIES OUTPUT_NAME "SMESH")
    set_target_properties(SMESH PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set_target_properties(SMESH PROPERTIES PREFIX "../")
elseif(MINGW)
    set_target_properties(SMESH PROPERTIES COMPILE_FLAGS "-DSMESH_EXPORTS -DSMESHCONTROLS_EXPORTS -DSMESHimpl_EXPORTS")
    set_target_properties(SMESH PROPERTIES OUTPUT_NAME "SMESH")
    set_target_properties(SMESH PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set_target_properties(SMESH PROPERTIES PREFIX "")
else(MSVC)
    set_target_properties(SMESH PROPERTIES OUTPUT_NAME "SMESH") 
    set_target_properties(SMESH PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set_target_properties(SMESH PROPERTIES PREFIX "")
    set_target_properties(SMESH PROPERTIES INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib)
endif(MSVC)
					  
					  
#IF(WIN32)
#  SET_TARGET_PROPERTIES(SMESH PROPERTIES COMPILE_FLAGS "-DSMESHimpl_EXPORTS -DSMESHCONTROLS_EXPORTS")
#ENDIF(WIN32)
#IF(MINGW)
#  SET_TARGET_PROPERTIES(SMESH PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
#ENDIF(MINGW)


##############
# MEFISTO2   #
##############
if (MSVC)
    FILE(GLOB MEFISTO2_source_files
        src/MEFISTO2/*.cpp
        src/MEFISTO2/MEFISTO2F.def
        inc/Rn.h
        inc/aptrte.h
    )
else(MSVC)
    FILE(GLOB MEFISTO2_source_files
        src/MEFISTO2/*.cpp
        src/MEFISTO2/trte.f
        inc/Rn.h
        inc/aptrte.h
    )
endif(MSVC)

ADD_LIBRARY(MEFISTO2 SHARED ${MEFISTO2_source_files})

if(MSVC)
    TARGET_LINK_LIBRARIES(MEFISTO2 ${SMESH_LIBS} MEFISTO2f.lib) # in case of Windows link the prebuild Fortran lib
    set_target_properties(MEFISTO2 PROPERTIES COMPILE_FLAGS "-DMEFISTO2D_EXPORTS -D__WATCOM__")
    set_target_properties(MEFISTO2 PROPERTIES DEBUG_OUTPUT_NAME "MEFISTO2d")
    set_target_properties(MEFISTO2 PROPERTIES OUTPUT_NAME "MEFISTO2")
    set_target_properties(MEFISTO2 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set_target_properties(MEFISTO2 PROPERTIES PREFIX "../")
elseif(MINGW)
    TARGET_LINK_LIBRARIES(MEFISTO2 ${SMESH_LIBS} gfortran)
    set_target_properties(MEFISTO2 PROPERTIES COMPILE_FLAGS "-DMEFISTO2D_EXPORTS")
    set_target_properties(MEFISTO2 PROPERTIES OUTPUT_NAME "MEFISTO2")
    set_target_properties(MEFISTO2 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set_target_properties(MEFISTO2 PROPERTIES PREFIX "")
else(MSVC)
    TARGET_LINK_LIBRARIES(MEFISTO2 ${SMESH_LIBS} gfortran)
    set_target_properties(MEFISTO2 PROPERTIES COMPILE_FLAGS "${MEFISTO2_CFLAGS}")
    set_target_properties(MEFISTO2 PROPERTIES OUTPUT_NAME "MEFISTO2") 
    set_target_properties(MEFISTO2 PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set_target_properties(MEFISTO2 PROPERTIES PREFIX "")
    set_target_properties(MEFISTO2 PROPERTIES INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib)
endif(MSVC)


##############
# StdMeshers #
##############
FILE(GLOB StdMeshers_source_files src/StdMeshers/*.cpp inc/StdMeshers_*.h*)
ADD_LIBRARY(StdMeshers SHARED ${StdMeshers_source_files})

if(MSVC)
    TARGET_LINK_LIBRARIES(StdMeshers SMESH ${SMESH_LIBS} MEFISTO2)
    set_target_properties(StdMeshers PROPERTIES COMPILE_FLAGS "-DSTDMESHERS_EXPORTS")
    set_target_properties(StdMeshers PROPERTIES DEBUG_OUTPUT_NAME "StdMeshersd")
    set_target_properties(StdMeshers PROPERTIES OUTPUT_NAME "StdMeshers")
    set_target_properties(StdMeshers PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set_target_properties(StdMeshers PROPERTIES PREFIX "../")
elseif(MINGW)
    TARGET_LINK_LIBRARIES(StdMeshers SMESH ${SMESH_LIBS} MEFISTO2)
    set_target_properties(StdMeshers PROPERTIES COMPILE_FLAGS "-DSTDMESHERS_EXPORTS")
    set_target_properties(StdMeshers PROPERTIES OUTPUT_NAME "StdMeshers")
    set_target_properties(StdMeshers PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set_target_properties(StdMeshers PROPERTIES PREFIX "")
else(MSVC)
    TARGET_LINK_LIBRARIES(StdMeshers SMESH ${SMESH_LIBS}  MEFISTO2)
    set_target_properties(StdMeshers PROPERTIES COMPILE_FLAGS "${StdMeshers_CFLAGS}")
    set_target_properties(StdMeshers PROPERTIES OUTPUT_NAME "StdMeshers") 
    set_target_properties(StdMeshers PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set_target_properties(StdMeshers PROPERTIES PREFIX "")
    set_target_properties(StdMeshers PROPERTIES INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib)
endif(MSVC)

#SET(StdMeshers_CFLAGS "")
#IF(WIN32)
#  SET(StdMeshers_CFLAGS "-DSTDMESHERS_EXPORTS -DMEFISTO2D_EXPORTS")
#ENDIF(WIN32)
#IF(MINGW)
#  SET_TARGET_PROPERTIES(StdMeshers PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
#ENDIF(MINGW)

# Disable for all other compilers but MSVC
if (FREECAD_BUILD_FEM_NETGEN)

################
# NETGENPlugin #
################
FILE(GLOB NETGENPlugin_source_files src/NETGENPlugin/*.cpp inc/NETGENPlugin_*.h* )
ADD_LIBRARY(NETGENPlugin SHARED ${NETGENPlugin_source_files})
TARGET_LINK_LIBRARIES(NETGENPlugin SMDS SMESHDS SMESH StdMeshers  ${SMESH_LIBS} )

if(MSVC)
    set_target_properties(NETGENPlugin PROPERTIES COMPILE_FLAGS "-DNETGENPLUGIN_EXPORTS -DNO_PARALLEL_THREADS -DOCCGEOMETRY -DNETGEN_V5")
    set_target_properties(NETGENPlugin PROPERTIES DEBUG_OUTPUT_NAME "NETGENPlugind")
    set_target_properties(NETGENPlugin PROPERTIES OUTPUT_NAME "NETGENPlugin")
    set_target_properties(NETGENPlugin PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set_target_properties(NETGENPlugin PROPERTIES PREFIX "../")
elseif(MINGW)
    set_target_properties(NETGENPlugin PROPERTIES COMPILE_FLAGS "-DNETGENPLUGIN_EXPORTS -DNO_PARALLEL_THREADS -DOCCGEOMETRY -DNETGEN_V5")
    set_target_properties(NETGENPlugin PROPERTIES OUTPUT_NAME "NETGENPlugin")
    set_target_properties(NETGENPlugin PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set_target_properties(NETGENPlugin PROPERTIES PREFIX "")
else(MSVC)
    set_target_properties(NETGENPlugin PROPERTIES OUTPUT_NAME "NETGENPlugin") 
    set_target_properties(NETGENPlugin PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set_target_properties(NETGENPlugin PROPERTIES PREFIX "")
    set_target_properties(NETGENPlugin PROPERTIES INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib)
endif(MSVC)

endif (FREECAD_BUILD_FEM_NETGEN)

#IF(WIN32)
#  SET_TARGET_PROPERTIES(SMESH PROPERTIES COMPILE_FLAGS "-DNETGENPluginimpl_EXPORTS ")
#ENDIF(WIN32)
#IF(MINGW)
#  SET_TARGET_PROPERTIES(SMESH PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
#ENDIF(MINGW)


if(WIN32)
    INSTALL(TARGETS SMDS Driver DriverSTL DriverDAT DriverUNV SMESHDS SMESH MEFISTO2 StdMeshers
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
    )
else(WIN32)
    INSTALL(TARGETS SMDS Driver DriverSTL DriverDAT DriverUNV SMESHDS SMESH MEFISTO2 StdMeshers
        LIBRARY DESTINATION lib
    )
endif(WIN32)

if (FREECAD_BUILD_FEM_NETGEN)
    if(WIN32)
        INSTALL(TARGETS NETGENPlugin
            RUNTIME DESTINATION bin
            LIBRARY DESTINATION lib
        )
    else(WIN32)
        INSTALL(TARGETS NETGENPlugin
            LIBRARY DESTINATION lib
        )
    endif(WIN32)
endif (FREECAD_BUILD_FEM_NETGEN)

