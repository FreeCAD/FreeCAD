if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif(MSVC)


include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

set(DraftUtils_LIBS
    Part
    FreeCADApp
)

SET(DraftUtils_SRCS
    AppDraftUtils.cpp
    AppDraftUtilsPy.cpp
    PreCompiled.cpp
    PreCompiled.h
)

add_library(DraftUtils SHARED ${DraftUtils_SRCS})
target_link_libraries(DraftUtils ${DraftUtils_LIBS})
if (FREECAD_WARN_ERROR)
    target_compile_warn_error(DraftUtils)
endif()


SET_BIN_DIR(DraftUtils DraftUtils /Mod/Draft)
SET_PYTHON_PREFIX_SUFFIX(DraftUtils)

INSTALL(TARGETS DraftUtils DESTINATION ${CMAKE_INSTALL_LIBDIR})
