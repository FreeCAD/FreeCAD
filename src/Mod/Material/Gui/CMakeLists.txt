if(MSVC)
    add_definitions(-DHAVE_ACOSH -DHAVE_ATANH -DHAVE_ASINH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H)
endif(MSVC)

include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${XercesC_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
)
link_directories(${OCC_LIBRARY_DIR})

set(MatGui_LIBS
    Part
    FreeCADGui
)

# if(MSVC)
#     include_directories(
#         ${CMAKE_SOURCE_DIR}/src/3rdParty/OpenGL/api
#     )
# endif(MSVC)

include_directories(
    ${QtConcurrent_INCLUDE_DIRS}
)
list(APPEND MatGui_LIBS
    ${QtConcurrent_LIBRARIES}
)

set (Material_TR_QRC ${CMAKE_CURRENT_BINARY_DIR}/Resources/Material_translation.qrc)
qt_find_and_add_translation(QM_SRCS "Resources/translations/*_*.ts"
    ${CMAKE_CURRENT_BINARY_DIR}/Resources/translations)
qt_create_resource_file(${Material_TR_QRC} ${QM_SRCS})
qt_add_resources(MatGui_QRC_SRCS Resources/Material.qrc ${Material_TR_QRC})

set(MatGui_UIC_SRCS
    Materials.ui
)

SET(MatGui_SRCS
    ${MatGui_QRC_SRCS}
    ${MatGui_UIC_HDRS}
    AppMatGui.cpp
    Command.cpp
    Materials.cpp
    Materials.h
    Materials.ui
    PreCompiled.cpp
    PreCompiled.h
)

if(FREECAD_USE_PCH)
    add_definitions(-D_PreComp_)
    GET_MSVC_PRECOMPILED_SOURCE("PreCompiled.cpp" PCH_SRCS ${MatGui_SRCS})
    ADD_MSVC_PRECOMPILED_HEADER(MatGui PreCompiled.h PreCompiled.cpp PCH_SRCS)
endif(FREECAD_USE_PCH)

SET(MatGuiIcon_SVG
    Resources/icons/MaterialWorkbench.svg
)

add_library(MatGui SHARED ${MatGui_SRCS} ${MatGuiIcon_SVG})
target_link_libraries(MatGui ${MatGui_LIBS})


SET_BIN_DIR(MatGui MatGui /Mod/Material)
SET_PYTHON_PREFIX_SUFFIX(MatGui)

fc_copy_sources(MatGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Material" ${MatGuiIcon_SVG})

INSTALL(TARGETS MatGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
INSTALL(FILES ${MatGuiIcon_SVG} DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/Material/Resources/icons")
