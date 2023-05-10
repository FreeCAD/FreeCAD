include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Boost_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${XercesC_INCLUDE_DIRS}
)

generate_from_xml(ViewProviderSpreadsheetPy)

# The XML files
set(SpreadsheetGui_XML_SRCS
    ViewProviderSpreadsheetPy.xml
)

set(SpreadsheetGui_LIBS
    Spreadsheet
    FreeCADGui
)

set_property(SOURCE qtcolorpicker.h qtcolorpicker.cpp PROPERTY SKIP_AUTOMOC ON)
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/moc_qtcolorpicker-internal.cpp
    COMMAND ${QtCore_MOC_EXECUTABLE} -o ${CMAKE_CURRENT_BINARY_DIR}/moc_qtcolorpicker-internal.cpp ${CMAKE_CURRENT_SOURCE_DIR}/qtcolorpicker.cpp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/qtcolorpicker.cpp)

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/moc_qtcolorpicker.cpp
    COMMAND ${QtCore_MOC_EXECUTABLE} -o ${CMAKE_CURRENT_BINARY_DIR}/moc_qtcolorpicker.cpp ${CMAKE_CURRENT_SOURCE_DIR}/qtcolorpicker.h
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/qtcolorpicker.h)


set_property(SOURCE qtcolorpicker.cpp APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/moc_qtcolorpicker-internal.cpp)
set_property(SOURCE qtcolorpicker.h APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/moc_qtcolorpicker.cpp)

SET(SpreadsheetGui_RES_SRCS
    Resources/Spreadsheet.qrc
)

set (Spreadsheet_TR_QRC ${CMAKE_CURRENT_BINARY_DIR}/Resources/Spreadsheet_translation.qrc)
qt_find_and_add_translation(QM_SRCS "Resources/translations/*_*.ts"
    ${CMAKE_CURRENT_BINARY_DIR}/Resources/translations)
qt_create_resource_file(${Spreadsheet_TR_QRC} ${QM_SRCS})
qt_add_resources(SpreadsheetGui_QRC_SRCS ${SpreadsheetGui_RES_SRCS} ${Spreadsheet_TR_QRC})

set(SpreadsheetGui_UIC_SRCS
   Sheet.ui
   PropertiesDialog.ui
   DlgBindSheet.ui
   DlgSheetConf.ui
)

SET(SpreadsheetGui_SRCS
    ${SpreadsheetGui_QRC_SRCS}
    ${SpreadsheetGui_XML_SRCS}
    AppSpreadsheetGui.cpp
    Command.cpp
    DlgSettings.ui
    DlgSettingsImp.cpp
    DlgSettingsImp.h
    LineEdit.h
    LineEdit.cpp
    ViewProviderSpreadsheet.cpp
    ViewProviderSpreadsheet.h
    ViewProviderSpreadsheetPyImp.cpp
    Resources/Spreadsheet.qrc
    SpreadsheetView.cpp
    SpreadsheetView.h
    SpreadsheetDelegate.h
    SpreadsheetDelegate.cpp
    SheetTableView.cpp
    SheetTableView.h
    SheetTableViewAccessibleInterface.h
    SheetTableViewAccessibleInterface.cpp
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
    DlgBindSheet.h
    DlgBindSheet.cpp
    DlgSheetConf.h
    DlgSheetConf.cpp
    ${SpreadsheetGui_UIC_HDRS}
)

SET(SpreadsheetGuiIcon_SVG
    Resources/icons/SpreadsheetWorkbench.svg
)

set(SpreadsheetGui_MOC_HDRS
    DlgSettingsImp.h
)

SOURCE_GROUP("Moc" FILES ${SpreadsheetGui_MOC_SRCS})

SET(Resource_SRCS
    ${Resource_SRCS}
    Resources/Spreadsheet.qrc
)

set(SpreadsheetGui_UIC_SRCS
    DlgSettings.ui
)

add_library(SpreadsheetGui SHARED ${SpreadsheetGui_SRCS} ${SpreadsheetGuiIcon_SVG})
target_link_libraries(SpreadsheetGui ${SpreadsheetGui_LIBS})

SET_BIN_DIR(SpreadsheetGui SpreadsheetGui /Mod/Spreadsheet)
SET_PYTHON_PREFIX_SUFFIX(SpreadsheetGui)

fc_copy_sources(SpreadsheetGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Spreadsheet" ${SpreadsheetGuiIcon_SVG})

INSTALL(TARGETS SpreadsheetGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
INSTALL(FILES ${SpreadsheetGuiIcon_SVG} DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/Spreadsheet/Resources/icons")
