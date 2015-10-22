FILE( GLOB KDL_SRCS [^.]*.cpp )
FILE( GLOB KDL_HPPS [^.]*.hpp [^.]*.inl)

FILE( GLOB UTIL_SRCS utilities/[^.]*.cpp utilities/[^.]*.cxx)
FILE( GLOB UTIL_HPPS utilities/[^.]*.h utilities/[^.]*.hpp)

ADD_LIBRARY(orocos-kdl SHARED ${KDL_SRCS} ${UTIL_SRCS}) 
SET_TARGET_PROPERTIES( orocos-kdl PROPERTIES
  SOVERSION "${KDL_VERSION_MAJOR}.${KDL_VERSION_MINOR}"
  VERSION "${KDL_VERSION}"
  COMPILE_FLAGS "${CMAKE_CXX_FLAGS_ADD} ${KDL_CFLAGS}")

INSTALL_TARGETS( /lib orocos-kdl)

# CMake 2.2:
INSTALL_FILES( /include/kdl FILES ${KDL_HPPS})
INSTALL_FILES( /include/kdl/utilities FILES ${UTIL_HPPS})

# Orocos convention:
CONFIGURE_FILE( kdl.pc.in src/orocos-kdl.pc @ONLY)
INSTALL_FILES( /lib/pkgconfig FILES orocos-kdl.pc)

IF( OROCOS_PLUGIN )
  ADD_SUBDIRECTORY( bindings/rtt )
ENDIF( OROCOS_PLUGIN )

IF( PYTHON_BINDINGS )
  ADD_SUBDIRECTORY( bindings/python )
ENDIF( PYTHON_BINDINGS )
