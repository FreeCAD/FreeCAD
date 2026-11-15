FILE( GLOB_RECURSE KDL_SRCS [^.]*.cpp [^.]*.cxx)
FILE( GLOB KDL_HPPS [^.]*.hpp [^.]*.inl)

FILE( GLOB UTIL_HPPS utilities/[^.]*.h utilities/[^.]*.hpp)

#In Windows (Visual Studio) it is necessary to specify the postfix
#of the debug library name and no symbols are exported by kdl, 
#so it is necessary to compile it as a static library
IF(MSVC)
    SET(CMAKE_DEBUG_POSTFIX "d")
    SET(LIB_TYPE STATIC)
ELSE(MSVC)
    SET(LIB_TYPE SHARED)
ENDIF(MSVC)

CONFIGURE_FILE(config.h.in config.h @ONLY)

#### Settings for rpath
IF(NOT MSVC)
    #add the option to disable RPATH
    OPTION(OROCOSKDL_ENABLE_RPATH "Enable RPATH during installation" TRUE)
    MARK_AS_ADVANCED(OROCOSKDL_ENABLE_RPATH)
ENDIF(NOT MSVC)

IF(OROCOSKDL_ENABLE_RPATH)
    #Configure RPATH
    SET(CMAKE_MACOSX_RPATH TRUE) #enable RPATH on OSX. This also suppress warnings on CMake >= 3.0
    # when building, don't use the install RPATH already
    # (but later on when installing)
    SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
    #build directory by default is built with RPATH
    SET(CMAKE_SKIP_BUILD_RPATH  FALSE)

    #This is relative RPATH for libraries built in the same project
    #I assume that the directory is
    # - install_dir/something for binaries
    # - install_dir/lib for libraries
    LIST(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
    IF("${isSystemDir}" STREQUAL "-1")
        FILE(RELATIVE_PATH _rel_path "${CMAKE_INSTALL_PREFIX}/bin" "${CMAKE_INSTALL_PREFIX}/lib")
        IF (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
            SET(CMAKE_INSTALL_RPATH "@loader_path/${_rel_path}")
        ELSE()
            SET(CMAKE_INSTALL_RPATH "\$ORIGIN/${_rel_path}")
        ENDIF()
    ENDIF("${isSystemDir}" STREQUAL "-1")
    # add the automatically determined parts of the RPATH
    # which point to directories outside the build tree to the install RPATH
    SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE) #very important!
ENDIF()
#####end RPATH

ADD_LIBRARY(orocos-kdl ${LIB_TYPE} ${KDL_SRCS})

TARGET_INCLUDE_DIRECTORIES(orocos-kdl PUBLIC
  "$<INSTALL_INTERFACE:include>")

SET_TARGET_PROPERTIES( orocos-kdl PROPERTIES
  SOVERSION "${KDL_VERSION_MAJOR}.${KDL_VERSION_MINOR}"
  VERSION "${KDL_VERSION}"
  COMPILE_FLAGS "${CMAKE_CXX_FLAGS_ADD} ${KDL_CFLAGS}"
  PUBLIC_HEADER "${KDL_HPPS};${CMAKE_CURRENT_BINARY_DIR}/config.h"
  )

#### Settings for rpath disabled (back-compatibility)
IF(NOT OROCOSKDL_ENABLE_RPATH)
    SET_TARGET_PROPERTIES( orocos-kdl PROPERTIES
      INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}")
ENDIF()
#####end RPATH

# Needed so that the generated config.h can be used
TARGET_INCLUDE_DIRECTORIES(orocos-kdl PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>")
TARGET_LINK_LIBRARIES(orocos-kdl ${Boost_LIBRARIES})

INSTALL(TARGETS orocos-kdl
  EXPORT OrocosKDLTargets
  ARCHIVE DESTINATION lib${LIB_SUFFIX}
  LIBRARY DESTINATION lib${LIB_SUFFIX}
  RUNTIME DESTINATION bin
  PUBLIC_HEADER DESTINATION include/kdl
)

INSTALL(FILES ${UTIL_HPPS} DESTINATION include/kdl/utilities)
