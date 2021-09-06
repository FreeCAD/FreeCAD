if (APPLE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OPENSSL_CFLAGS}")
endif()

include_directories(
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${XercesC_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    ${CURL_INCLUDE_DIR}
)

set(Cloud_LIBS
    FreeCADApp
    ${OPENSSL_LIBRARIES}
    ${CURL_LIBRARIES}
    ${XercesC_LIBRARIES}
    ${Boost_LIBRARIES}
    ${ZLIB_LIBRARIES}
)

SET(Cloud_SRCS
    AppCloud.cpp
    PreCompiled.cpp
    PreCompiled.h
)

add_library(Cloud SHARED ${Cloud_SRCS})
target_link_libraries(Cloud ${Cloud_LIBS})


SET_BIN_DIR(Cloud Cloud /Mod/Cloud)
SET_PYTHON_PREFIX_SUFFIX(Cloud)

install(TARGETS Cloud DESTINATION ${CMAKE_INSTALL_LIBDIR})
