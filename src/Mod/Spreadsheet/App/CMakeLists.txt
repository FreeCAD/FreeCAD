if(WIN32)
    add_definitions(-DFCAppSpreadsheet)
endif(WIN32)

include_directories(
    #${OPENCV_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${Boost_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${XERCESC_INCLUDE_DIR}
)

set(Spreadsheet_LIBS
    #${OPENCV_LIBRARIES}
    FreeCADApp
)

set(Spreadsheet_SRCS
    Sheet.cpp
    Sheet.h
    PreCompiled.cpp
    PreCompiled.h
    SheetPy.xml
    SheetPyImp.cpp
    AppSpreadsheet.cpp
)

generate_from_xml(SheetPy)

add_library(Spreadsheet SHARED ${Spreadsheet_SRCS})
target_link_libraries(Spreadsheet ${Spreadsheet_LIBS})


fc_target_copy_resource(Spreadsheet 
    ${CMAKE_SOURCE_DIR}/src/Mod/Spreadsheet
    ${CMAKE_BINARY_DIR}/Mod/Spreadsheet
    Init.py)

if(MSVC)
    set_target_properties(Spreadsheet PROPERTIES SUFFIX ".pyd")
    set_target_properties(Spreadsheet PROPERTIES DEBUG_OUTPUT_NAME "Spreadsheet_d")
    set_target_properties(Spreadsheet PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Spreadsheet)
    set_target_properties(Spreadsheet PROPERTIES PREFIX "../")
elseif(MINGW)
    set_target_properties(Spreadsheet PROPERTIES SUFFIX ".pyd")
    set_target_properties(Spreadsheet PROPERTIES DEBUG_OUTPUT_NAME "Spreadsheet_d")
    set_target_properties(Spreadsheet PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Spreadsheet)
    set_target_properties(Spreadsheet PROPERTIES PREFIX "")
else(MSVC)
    set_target_properties(Spreadsheet PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Spreadsheet)
    set_target_properties(Spreadsheet PROPERTIES PREFIX "")
    set_target_properties(Spreadsheet PROPERTIES INSTALL_RPATH ${INSTALL_RPATH})
endif(MSVC)

INSTALL(TARGETS Spreadsheet DESTINATION lib)
