# -------------------------------- Qt --------------------------------

set(FREECAD_QT_COMPONENTS Core Concurrent Network Xml)
set(Qt6Core_MOC_EXECUTABLE Qt6::moc)

if(BUILD_GUI)
    list (APPEND FREECAD_QT_COMPONENTS GuiTools)
    list (APPEND FREECAD_QT_COMPONENTS SvgWidgets)
    list (APPEND FREECAD_QT_COMPONENTS OpenGLWidgets)

    list (APPEND FREECAD_QT_COMPONENTS OpenGL PrintSupport Svg UiTools Widgets LinguistTools)

    if(BUILD_DESIGNER_PLUGIN)
        list (APPEND FREECAD_QT_COMPONENTS Designer)
    endif()
endif()

if (ENABLE_DEVELOPER_TESTS)
    list (APPEND FREECAD_QT_COMPONENTS Test)
endif ()

foreach(COMPONENT IN LISTS FREECAD_QT_COMPONENTS)
    find_package(Qt${FREECAD_QT_MAJOR_VERSION} REQUIRED COMPONENTS ${COMPONENT})
    if(TARGET Qt${FREECAD_QT_MAJOR_VERSION}::${COMPONENT})
        set(Qt${COMPONENT}_LIBRARIES Qt${FREECAD_QT_MAJOR_VERSION}::${COMPONENT})
        get_target_property(Qt${COMPONENT}_INCLUDE_DIRS Qt${FREECAD_QT_MAJOR_VERSION}::${COMPONENT} INTERFACE_INCLUDE_DIRECTORIES)
        if(NOT Qt${COMPONENT}_INCLUDE_DIRS)
            set(Qt${COMPONENT}_INCLUDE_DIRS "")
        endif()
    else()
        set(Qt${COMPONENT}_LIBRARIES ${Qt${FREECAD_QT_MAJOR_VERSION}${COMPONENT}_LIBRARIES})
        set(Qt${COMPONENT}_INCLUDE_DIRS ${Qt${FREECAD_QT_MAJOR_VERSION}${COMPONENT}_INCLUDE_DIRS})
    endif()
    set(Qt${COMPONENT}_FOUND ${Qt${FREECAD_QT_MAJOR_VERSION}${COMPONENT}_FOUND})
    set(Qt${COMPONENT}_VERSION ${Qt${FREECAD_QT_MAJOR_VERSION}${COMPONENT}_VERSION})
endforeach()

set(CMAKE_AUTOMOC TRUE)
set(CMAKE_AUTOUIC TRUE)
set(QtCore_MOC_EXECUTABLE ${Qt${FREECAD_QT_MAJOR_VERSION}Core_MOC_EXECUTABLE})

add_definitions(-DQT_NO_KEYWORDS)

message(STATUS "Set up to compile with Qt ${Qt${FREECAD_QT_MAJOR_VERSION}Core_VERSION}")

configure_file(${CMAKE_SOURCE_DIR}/src/QtWidgets.h.cmake ${CMAKE_BINARY_DIR}/src/QtWidgets.h)

function(qt_find_and_add_translation _qm_files _tr_dir _qm_dir)
    file(GLOB _ts_files ${_tr_dir})
    set_source_files_properties(${_ts_files} PROPERTIES OUTPUT_LOCATION ${_qm_dir})
    qt_add_translation("${_qm_files}" ${_ts_files})
    set("${_qm_files}" "${${_qm_files}}" PARENT_SCOPE)
endfunction()

function(qt_create_resource_file outfile)
    set(QRC "<RCC>\n  <qresource>\n")
    foreach (it ${ARGN})
        get_filename_component(qmfile "${it}" NAME)
        string(APPEND QRC "        <file>translations/${qmfile}</file>")
    endforeach()
    string(APPEND QRC "  </qresource>\n</RCC>\n")
    file(WRITE ${outfile} ${QRC})
endfunction()

function(qt_create_resource_file_prefix outfile)
    set(QRC "<RCC>\n  <qresource prefix=\"/translations\">\n")
    foreach (it ${ARGN})
        get_filename_component(qmfile "${it}" NAME)
        string(APPEND QRC "        <file>${qmfile}</file>")
    endforeach()
    string(APPEND QRC "  </qresource>\n</RCC>\n")
    file(WRITE ${outfile} ${QRC})
endfunction()
