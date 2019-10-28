#------------------------------ OpenSSL and CURL -------------------------
if (APPLE)
    set(OPENSSL_ROOT_DIR ${HOMEBREW_PREFIX}/Cellar/openssl/*)
endif(APPLE)
if (UNIX AND NOT APPLE)
    find_package(PkgConfig REQUIRED)
    pkg_search_module(OPENSSL REQUIRED openssl)
else()
    find_package(OPENSSL REQUIRED)
endif(UNIX)
if(OPENSSL_FOUND)
    message(STATUS "openssl-${OPENSSL_VERSION} has been found\n")
endif(OPENSSL_FOUND)
    find_package(CURL REQUIRED)
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
