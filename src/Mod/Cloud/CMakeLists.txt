#----------------------------- Control certificate validation ------------
option(ALLOW_SELF_SIGNED_CERTIFICATE "Allow self signed certificate" OFF)
if (ALLOW_SELF_SIGNED_CERTIFICATE)
add_compile_options("-DALLOW_SELF_SIGNED_CERTIFICATE")
endif ()
#------------------------------ OpenSSL and CURL -------------------------
if (APPLE)
    find_package(PkgConfig REQUIRED)
    pkg_search_module(OPENSSL REQUIRED openssl)
    set (OPENSSL_LIBRARIES ${OPENSSL_LINK_LIBRARIES})
elseif(WIN32 AND LIBPACK_FOUND)
    SET( OPENSSL_INCLUDE_DIR ${FREECAD_LIBPACK_DIR}/include)
    SET( OPENSSL_LIBRARIES
      optimized ${FREECAD_LIBPACK_DIR}/lib/openssl.lib debug ${FREECAD_LIBPACK_DIR}/lib/openssld.lib
      optimized ${FREECAD_LIBPACK_DIR}/lib/libssl.lib debug ${FREECAD_LIBPACK_DIR}/lib/libssld.lib
      optimized ${FREECAD_LIBPACK_DIR}/lib/libcrypto.lib debug ${FREECAD_LIBPACK_DIR}/lib/libcryptod.lib)
    if( EXISTS ${FREECAD_LIBPACK_DIR}/lib/openssl.lib)
       SET( OPENSSL_FOUND True)
     else()
        SET( OPENSSL_FOUND False)
        message( FATAL_ERROR "openssl not found!" )
    endif(EXISTS ${FREECAD_LIBPACK_DIR}/lib/openssl.lib)

    if(OPENSSL_INCLUDE_DIR AND EXISTS "${OPENSSL_INCLUDE_DIR}/openssl/opensslv.h")
      file(STRINGS "${OPENSSL_INCLUDE_DIR}/openssl/opensslv.h" openssl_version_str
           REGEX "OPENSSL_VERSION_STR")
      string(REGEX MATCH "\".*"
        openssl_version_str ${openssl_version_str})
      string(REPLACE "\"" "" openssl_version_str ${openssl_version_str})
      set(OPENSSL_VERSION ${openssl_version_str})
  endif ()
else()
    find_package(OpenSSL REQUIRED)
endif(APPLE)

if(OPENSSL_FOUND)
    message(STATUS "openssl-${OPENSSL_VERSION} has been found\n")
else()
    message( FATAL_ERROR "openssl not found!" )
endif(OPENSSL_FOUND)

IF(WIN32)
    SET( CURL_FOUND True)
    SET( CURL_INCLUDE_DIR ${FREECAD_LIBPACK_DIR}/include)
    SET( CURL_LIBRARIES
      optimized ${FREECAD_LIBPACK_DIR}/lib/libcurl.lib debug ${FREECAD_LIBPACK_DIR}/lib/libcurl_debug.lib)
    if( EXISTS ${FREECAD_LIBPACK_DIR}/lib/libcurl.lib)
       SET( CURL_FOUND True)
     else()
        SET( CURL_FOUND False)
        message( FATAL_ERROR "curl not found!" )
    endif(EXISTS ${FREECAD_LIBPACK_DIR}/lib/libcurl.lib)
ELSE()
    find_package(CURL REQUIRED)
ENDIF(WIN32)


if(CURL_FOUND)
    message(STATUS "CURL has been found\n")
endif(CURL_FOUND)
# ------------------------------------------------------------------------

add_subdirectory(App)
if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

set(Cloud_Scripts
    Init.py
)

if(BUILD_GUI)
    list (APPEND Cloud_Scripts InitGui.py)
endif(BUILD_GUI)

add_custom_target(CloudScripts ALL
    SOURCES ${Cloud_Scripts}
)

fc_target_copy_resource_flat(CloudScripts
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/Cloud
    ${Cloud_Scripts}
)

install(
    FILES
        ${Cloud_Scripts}
    DESTINATION
        Mod/Cloud
)
