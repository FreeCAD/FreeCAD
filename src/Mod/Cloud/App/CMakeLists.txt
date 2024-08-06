if(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OPENSSL_CFLAGS}")
endif()

include_directories(
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${XercesC_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    ${CURL_INCLUDE_DIR})

set(Cloud_LIBS FreeCADApp ${OPENSSL_LIBRARIES} ${CURL_LIBRARIES} ${XercesC_LIBRARIES}
               ${Boost_LIBRARIES} ${ZLIB_LIBRARIES})

set(Cloud_SRCS AppCloud.cpp PreCompiled.cpp PreCompiled.h)

add_library(Cloud SHARED ${Cloud_SRCS})
target_link_libraries(Cloud ${Cloud_LIBS})

set_bin_dir(Cloud Cloud /Mod/Cloud)
set_python_prefix_suffix(Cloud)

install(TARGETS Cloud DESTINATION ${CMAKE_INSTALL_LIBDIR})
