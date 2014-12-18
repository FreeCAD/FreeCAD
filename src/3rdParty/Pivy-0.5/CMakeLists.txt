add_definitions(-DCOIN_DLL)

include_directories(
    ../include
    ${COIN3D_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
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
        ${PYTHON_LIBRARY})
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
