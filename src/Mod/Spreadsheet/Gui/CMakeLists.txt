include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Boost_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIR}
    #${OPENCV_INCLUDE_DIR}
    ${QT_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${SOQT_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${XERCESC_INCLUDE_DIR}
)

set(SpreadsheetGui_LIBS
    Spreadsheet
    FreeCADGui
)

set(SpreadsheetGui_MOC_HDRS
    SpreadsheetView.h
    SheetModel.h
    SheetTableView.h
    SpreadsheetDelegate.h
)


fc_wrap_cpp(SpreadsheetGui_MOC_SRCS ${SpreadsheetGui_MOC_HDRS})
SOURCE_GROUP("Moc" FILES ${SpreadsheetGui_MOC_SRCS})

SET(SpreadsheetGui_RES_SRCS
    Resources/Spreadsheet.qrc
)

fc_add_resources(SpreadsheetGui_QRC_SRCS ${SpreadsheetGui_RES_SRCS})
#qt4_add_resources(SpreadsheetGui_QRC_SRCS ${SpreadsheetGui_RES_SRCS})

set(SpreadsheetGui_UIC_SRCS
   Sheet.ui
)

qt4_wrap_ui(SpreadsheetGui_UIC_HDRS ${SpreadsheetGui_UIC_SRCS})

SET(SpreadsheetGui_SRCS
#   ${SpreadsheetGui_MOC_SRCS}
#    ${SpreadsheetGui_QRC_SRCS}
    AppSpreadsheetGui.cpp
    AppSpreadsheetGuiPy.cpp
    Command.cpp
    ViewProviderSpreadsheet.cpp
    ViewProviderSpreadsheet.h
    Resources/Spreadsheet.qrc
    SpreadsheetView.cpp
    SpreadsheetView.h
    SpreadsheetDelegate.h
    SpreadsheetDelegate.cpp
    SheetTableView.cpp
    SheetTableView.h
    SheetModel.h
    SheetModel.cpp
    PreCompiled.cpp
    PreCompiled.h
    Workbench.cpp
    Workbench.h

    ${SpreadsheetGui_UIC_HDRS}
)

add_library(SpreadsheetGui SHARED ${SpreadsheetGui_SRCS})
target_link_libraries(SpreadsheetGui ${SpreadsheetGui_LIBS})


fc_target_copy_resource(SpreadsheetGui 
    ${CMAKE_SOURCE_DIR}/src/Mod/Spreadsheet
    ${CMAKE_BINARY_DIR}/Mod/Spreadsheet
    InitGui.py)

if(MSVC)
    set_target_properties(SpreadsheetGui PROPERTIES SUFFIX ".pyd")
    set_target_properties(SpreadsheetGui PROPERTIES DEBUG_OUTPUT_NAME "SpreadsheetGui_d")
    set_target_properties(SpreadsheetGui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Spreadsheet)
    set_target_properties(SpreadsheetGui PROPERTIES PREFIX "../")
elseif(MINGW)
    set_target_properties(SpreadsheetGui PROPERTIES SUFFIX ".pyd")
    set_target_properties(SpreadsheetGui PROPERTIES DEBUG_OUTPUT_NAME "SpreadsheetGui_d")
    set_target_properties(SpreadsheetGui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Spreadsheet)
    set_target_properties(SpreadsheetGui PROPERTIES PREFIX "")
else(MSVC)
    set_target_properties(SpreadsheetGui PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Spreadsheet)
    set_target_properties(SpreadsheetGui PROPERTIES PREFIX "")
    set_target_properties(SpreadsheetGui PROPERTIES INSTALL_RPATH ${INSTALL_RPATH})
endif(MSVC)

INSTALL(TARGETS SpreadsheetGui DESTINATION lib)
