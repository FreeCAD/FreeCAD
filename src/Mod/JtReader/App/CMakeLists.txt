include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${QT_INCLUDE_DIR}
)

link_directories(${OCC_LIBRARY_DIR})

set(JtReader_LIBS
    Mesh
    #${OCC_OCAF_LIBRARIES}
    #${OCC_OCAF_DEBUG_LIBRARIES}
)

SET(JtReader_SRCS
    AppJtReader.cpp
    AppJtReaderPy.cpp
    PreCompiled.cpp
    PreCompiled.h
    TestJtReader.cpp
    TestJtReader.h
    FcLodHandler.h
    FcLodHandler.cpp
)

set (JtReader_Scripts
    ../Init.py
)

FILE( GLOB JRJT_SRCS JrJt/*.cpp JrJt/*.h )

SOURCE_GROUP("JrJt" FILES ${JRJT_SRCS}  )

SOURCE_GROUP("Module" FILES ${JtReader_SRCS})

add_library(JtReader SHARED ${JtReader_SRCS} ${JRJT_SRCS} ${JtReader_Scripts})
target_link_libraries(JtReader ${JtReader_LIBS})

fc_target_copy_resource_flat(JtReader
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/JtReader
    ${JtReader_Scripts})

SET_BIN_DIR(JtReader JtReader /Mod/JtReader)
SET_PYTHON_PREFIX_SUFFIX(JtReader)

INSTALL(TARGETS JtReader DESTINATION ${CMAKE_INSTALL_LIBDIR})
