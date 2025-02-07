FILE( GLOB_RECURSE KDL_SRCS [^.]*.cpp [^.]*.cxx)
FILE( GLOB KDL_HPPS [^.]*.hpp [^.]*.inl)

FILE( GLOB UTIL_HPPS utilities/[^.]*.h utilities/[^.]*.hpp)

INCLUDE(CheckCXXSourceCompiles)
SET(CMAKE_REQUIRED_FLAGS)
CHECK_CXX_SOURCE_COMPILES("
    #include <string>
    #include <map>
    #include <vector.hpp>

    class TreeElement;
    typedef std::map<std::string, TreeElement> SegmentMap;

    class TreeElement
    {
        TreeElement(const std::string& name): number(0) {}

    public:
        int number;
        SegmentMap::const_iterator parent;
        std::vector<SegmentMap::const_iterator> children;

        static TreeElement Root(std::string& name)
        {
            return TreeElement(name);
        }
    };

    int main()
    {
        return 0;
    }
    "
    HAVE_STL_CONTAINER_INCOMPLETE_TYPES)

if(HAVE_STL_CONTAINER_INCOMPLETE_TYPES)
    SET(KDL_USE_NEW_TREE_INTERFACE_DEFAULT Off)
ELSE(HAVE_STL_CONTAINER_INCOMPLETE_TYPES)
    SET(KDL_USE_NEW_TREE_INTERFACE_DEFAULT On)
ENDIF(HAVE_STL_CONTAINER_INCOMPLETE_TYPES)

SET(KDL_USE_NEW_TREE_INTERFACE ${KDL_USE_NEW_TREE_INTERFACE_DEFAULT} CACHE BOOL "Use the new KDL Tree interface")

#Sanity check, inform the user
IF(NOT HAVE_STL_CONTAINER_INCOMPLETE_TYPES AND NOT KDL_USE_NEW_TREE_INTERFACE)
    MESSAGE(WARNING "You have chosen to use the current Tree Interface, but your platform doesn't support containers of "
        "incomplete types, this configuration is likely invalid")
ENDIF()

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

ADD_LIBRARY(orocos-kdl ${LIB_TYPE} ${KDL_SRCS} config.h)

SET_TARGET_PROPERTIES( orocos-kdl PROPERTIES
  SOVERSION "${KDL_VERSION_MAJOR}.${KDL_VERSION_MINOR}"
  VERSION "${KDL_VERSION}"
  COMPILE_FLAGS "${CMAKE_CXX_FLAGS_ADD} ${KDL_CFLAGS}"
  PUBLIC_HEADER "${KDL_HPPS};${CMAKE_CURRENT_BINARY_DIR}/config.h"
  )

IF(NOT OROCOSKDL_ENABLE_RPATH)
    SET_TARGET_PROPERTIES( orocos-kdl PROPERTIES
      INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}")
ENDIF()

# Needed so that the generated config.h can be used
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES(orocos-kdl ${Boost_LIBRARIES})

INSTALL(TARGETS orocos-kdl
  EXPORT OrocosKDLTargets
  ARCHIVE DESTINATION lib${LIB_SUFFIX}
  LIBRARY DESTINATION lib${LIB_SUFFIX}
  PUBLIC_HEADER DESTINATION include/kdl
)

INSTALL(FILES ${UTIL_HPPS} DESTINATION include/kdl/utilities)

# Orocos convention:
CONFIGURE_FILE( kdl.pc.in ${CMAKE_CURRENT_BINARY_DIR}/orocos-kdl.pc @ONLY)

INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/orocos-kdl.pc DESTINATION lib${LIB_SUFFIX}/pkgconfig)
