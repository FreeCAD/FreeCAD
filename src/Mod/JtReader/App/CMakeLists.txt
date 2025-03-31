set(JTREADER_TKJT_INCLUDE_DIRS CACHE PATH  "Include directory of TKJT headers")
set(JTREADER_TKJT_LIBRARIES CACHE FILEPATH  "File path to TKJT library")

include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

set(JtReader_LIBS
    Mesh
    ${OCC_LIBRARIES}
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

if (EXISTS "${JTREADER_TKJT_INCLUDE_DIRS}/JtData_Object.hxx")
    add_definitions(-DJTREADER_HAVE_TKJT)
    include_directories(SYSTEM ${JTREADER_TKJT_INCLUDE_DIRS})
    list (APPEND JtReader_LIBS
        ${JTREADER_TKJT_LIBRARIES}
    )

    list (APPEND JtReader_SRCS
        TKJtReader.cpp
        TKJtReader.h
    )
endif ()

set (JtReader_Scripts
    ../Init.py
)

FILE( GLOB JRJT_SRCS JrJt/*.cpp JrJt/*.h )

SOURCE_GROUP("JrJt" FILES ${JRJT_SRCS}  )

SOURCE_GROUP("Module" FILES ${JtReader_SRCS})

add_library(JtReader SHARED ${JtReader_SRCS} ${JRJT_SRCS} ${JtReader_Scripts})
target_link_libraries(JtReader ${JtReader_LIBS})
if (FREECAD_WARN_ERROR)
    target_compile_warn_error(JtReader)
endif()

fc_target_copy_resource_flat(JtReader
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/JtReader
    ${JtReader_Scripts})

SET_BIN_DIR(JtReader JtReader /Mod/JtReader)
SET_PYTHON_PREFIX_SUFFIX(JtReader)

INSTALL(TARGETS JtReader DESTINATION ${CMAKE_INSTALL_LIBDIR})
