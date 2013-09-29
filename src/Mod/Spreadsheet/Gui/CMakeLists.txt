include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Boost_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIR}
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
    Workbench.h
    PropertiesDialog.h
    qtcolorpicker.h
)

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/moc_qtcolorpicker-internal.cpp
    COMMAND ${QT_MOC_EXECUTABLE} -o ${CMAKE_CURRENT_BINARY_DIR}/moc_qtcolorpicker-internal.cpp ${CMAKE_CURRENT_SOURCE_DIR}/qtcolorpicker.cpp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/qtcolorpicker.cpp)

set_property(SOURCE qtcolorpicker.cpp APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/moc_qtcolorpicker-internal.cpp)

fc_wrap_cpp(SpreadsheetGui_MOC_SRCS ${SpreadsheetGui_MOC_HDRS})
SOURCE_GROUP("Moc" FILES ${SpreadsheetGui_MOC_SRCS})


SET(SpreadsheetGui_RES_SRCS
    Resources/Spreadsheet.qrc
)

#fc_add_resources(SpreadsheetGui_QRC_SRCS ${SpreadsheetGui_RES_SRCS})
qt4_add_resources(SpreadsheetGui_QRC_SRCS ${SpreadsheetGui_RES_SRCS})

set(SpreadsheetGui_UIC_SRCS
   Sheet.ui
   PropertiesDialog.ui
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
    qtcolorpicker.h
    qtcolorpicker.cpp
    PropertiesDialog.h
    PropertiesDialog.cpp
    ${SpreadsheetGui_UIC_HDRS}
)

add_library(SpreadsheetGui SHARED ${SpreadsheetGui_SRCS})
target_link_libraries(SpreadsheetGui ${SpreadsheetGui_LIBS})

fc_target_copy_resource(SpreadsheetGui 
    ${CMAKE_SOURCE_DIR}/src/Mod/Spreadsheet
    ${CMAKE_BINARY_DIR}/Mod/Spreadsheet
    InitGui.py)

SET_BIN_DIR(SpreadsheetGui SpreadsheetGui /Mod/Spreadsheet)
SET_PYTHON_PREFIX_SUFFIX(SpreadsheetGui)

INSTALL(TARGETS SpreadsheetGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
