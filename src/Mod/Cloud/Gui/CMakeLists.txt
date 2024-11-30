
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
)

set(CloudGui_LIBS
    Cloud
    FreeCADGui
)

qt_add_resources(Cloud_QRC_SRCS Resources/Cloud.qrc)

SET(CloudGui_SRCS
    ${Cloud_QRC_SRCS}
    AppCloudGui.cpp
    Command.cpp
    PreCompiled.cpp
    PreCompiled.h
    Workbench.cpp
    Workbench.h
)

SET(Cloud_SVG
    Resources/icons/CloudWorkbench.svg
)

add_library(CloudGui SHARED ${CloudGui_SRCS} ${Cloud_SVG})
target_link_libraries(CloudGui ${CloudGui_LIBS})

fc_copy_sources(CloudGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Cloud" ${Cloud_SVG})

SET_BIN_DIR(CloudGui CloudGui /Mod/Cloud)
SET_PYTHON_PREFIX_SUFFIX(CloudGui)

install(TARGETS CloudGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
INSTALL(FILES ${Cloud_SVG} DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/Cloud/Resources/icons")
