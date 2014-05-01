add_definitions(-DSOQT_DLL -DCOIN_DLL)

include_directories(
    ../include
    ${COIN3D_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${SOQT_INCLUDE_DIR}
    ${QT_INCLUDE_DIR}
)

########################### CoinPy ###########################

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
endif(MSVC)

set(CoinPy_SRCS
    coin_wrap.cpp
    coin_header_includes.h
)

if(MSVC)
    set(CoinPy_LIBS
        debug MSVCRTD.LIB
        debug MSVCPRTD.LIB
        optimized MSVCRT.LIB
        optimized MSVCPRT.LIB
        Rpcrt4.lib
        debug ${COIN3D_LIBRARY_DEBUG}
        optimized ${COIN3D_LIBRARY_RELEASE}
        debug ${PYTHON_DEBUG_LIBRARY}
        optimized ${PYTHON_LIBRARY})
else(MSVC)
    set(CoinPy_LIBS
        ${COIN3D_LIBRARY}
        ${PYTHON_LIBRARY})
endif(MSVC)

add_library(coin SHARED ${CoinPy_SRCS})

target_link_libraries(coin ${CoinPy_LIBS})

SET_BIN_DIR(coin _coin /bin/pivy)
SET_PYTHON_PREFIX_SUFFIX(coin)

fc_copy_sources(coin "${CMAKE_BINARY_DIR}/bin/pivy"
    __init__.py
    coin.py
    sogui.py
)

install(TARGETS coin DESTINATION bin/pivy)
install(FILES __init__.py coin.py sogui.py DESTINATION bin/pivy)

########################### SoQtPy ###########################

set(SoQtPy_SRCS
    soqt_wrap.cpp
)

if(MSVC)
    set(SoQtPy_LIBS
        debug MSVCRTD.LIB
        debug MSVCPRTD.LIB
        optimized MSVCRT.LIB
        optimized MSVCPRT.LIB
        Rpcrt4.lib
        debug ${SOQT_LIBRARY_DEBUG}
        optimized ${SOQT_LIBRARY_RELEASE}
        debug ${COIN3D_LIBRARY_DEBUG}
        optimized ${COIN3D_LIBRARY_RELEASE}
        debug ${PYTHON_DEBUG_LIBRARY}
        optimized ${PYTHON_LIBRARY})
else(MSVC)
    set(SoQtPy_LIBS
        ${SOQT_LIBRARIES}
        ${COIN3D_LIBRARY}
        ${PYTHON_LIBRARY})
endif(MSVC)

add_library(soqt SHARED ${SoQtPy_SRCS})

target_link_libraries(soqt ${SoQtPy_LIBS})

SET_BIN_DIR(soqt _soqt /bin/pivy/gui)
SET_PYTHON_PREFIX_SUFFIX(soqt)

fc_copy_sources(soqt "${CMAKE_BINARY_DIR}/bin/pivy"
    gui/soqt.py
    gui/__init__.py
)

install(TARGETS soqt DESTINATION bin/pivy/gui)
install(FILES gui/__init__.py gui/soqt.py DESTINATION bin/pivy/gui)
