configure_file(freecad.rc.cmake ${CMAKE_CURRENT_BINARY_DIR}/freecad.rc)
configure_file(freecadCmd.rc.cmake ${CMAKE_CURRENT_BINARY_DIR}/freecadCmd.rc)
file(COPY icon.ico DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

if(BUILD_GUI)
    include_directories(
        ${Boost_INCLUDE_DIRS}
        ${COIN3D_INCLUDE_DIRS}
        ${ZLIB_INCLUDE_DIR}
        ${QtWidgets_INCLUDE_DIRS}
        ${QT_INCLUDE_DIR}
        ${PYTHON_INCLUDE_DIRS}
        ${XercesC_INCLUDE_DIRS})
else(BUILD_GUI)
    include_directories(${Boost_INCLUDE_DIRS} ${ZLIB_INCLUDE_DIR} ${QT_INCLUDE_DIR}
                        ${PYTHON_INCLUDE_DIRS} ${XercesC_INCLUDE_DIRS})
endif(BUILD_GUI)

# ####################### FreeCADMain ########################
if(BUILD_GUI)

    set(FreeCAD_SRCS ${CMAKE_CURRENT_BINARY_DIR}/freecad.rc icon.ico MainGui.cpp)

    set(FreeCAD_LIBS FreeCADGui)

    if(NOT BUILD_DYNAMIC_LINK_PYTHON)
        # executables have to be linked against python libraries, because extension modules are not.
        list(APPEND FreeCAD_LIBS ${PYTHON_LIBRARIES})
    endif(NOT BUILD_DYNAMIC_LINK_PYTHON)

    add_executable(FreeCADMain WIN32 ${FreeCAD_SRCS})
    target_link_libraries(FreeCADMain ${FreeCAD_LIBS})
    if(FREECAD_WARN_ERROR)
        target_compile_warn_error(FreeCADMain)
    endif()

    set_bin_dir(FreeCADMain FreeCAD)

    if(WIN32)
        install(
            TARGETS FreeCADMain
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
    elseif(APPLE AND NOT BUILD_WITH_CONDA)
        install(
            TARGETS FreeCADMain
            RUNTIME DESTINATION MacOS
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
    else()
        install(TARGETS FreeCADMain RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    endif()
endif(BUILD_GUI)
# ####################### FreeCADMainCmd ########################

set(FreeCADMainCmd_SRCS ${CMAKE_CURRENT_BINARY_DIR}/freecadCmd.rc icon.ico MainCmd.cpp)

add_executable(FreeCADMainCmd ${FreeCADMainCmd_SRCS})

set(FreeCADMainCmd_LIBS FreeCADApp ${QtCore_LIBRARIES} ${QtXml_LIBRARIES})

if(NOT BUILD_DYNAMIC_LINK_PYTHON)
    # executables have to be linked against python libraries, because extension modules are not.
    list(APPEND FreeCADMainCmd_LIBS ${PYTHON_LIBRARIES})
endif(NOT BUILD_DYNAMIC_LINK_PYTHON)

target_link_libraries(FreeCADMainCmd ${FreeCADMainCmd_LIBS})
if(FREECAD_WARN_ERROR)
    target_compile_warn_error(FreeCADMainCmd)
endif()

set_bin_dir(FreeCADMainCmd FreeCADCmd)

if(WIN32)
    install(
        TARGETS FreeCADMainCmd
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
elseif(APPLE AND NOT BUILD_WITH_CONDA)
    install(
        TARGETS FreeCADMainCmd
        RUNTIME DESTINATION MacOS
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
else()
    install(TARGETS FreeCADMainCmd RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

# ####################### FreeCADMainPy ########################

set(FreeCADMainPy_SRCS MainPy.cpp)

add_library(FreeCADMainPy SHARED ${FreeCADMainPy_SRCS})

target_link_libraries(FreeCADMainPy FreeCADApp)
if(FREECAD_WARN_ERROR)
    target_compile_warn_error(FreeCADMainPy)
endif()

set_bin_dir(FreeCADMainPy FreeCAD)
set_python_prefix_suffix(FreeCADMainPy)

if(WIN32)
    install(
        TARGETS FreeCADMainPy
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
else(WIN32)
    install(TARGETS FreeCADMainPy LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif(WIN32)

# ####################### FreeCADGuiPy ########################
if(BUILD_GUI)

    set(FreeCADGuiPy_SRCS FreeCADGuiPy.cpp)

    add_library(FreeCADGuiPy SHARED ${FreeCADGuiPy_SRCS})

    target_link_libraries(FreeCADGuiPy FreeCADGui)
    if(FREECAD_WARN_ERROR)
        target_compile_warn_error(FreeCADGuiPy)
    endif()

    set_bin_dir(FreeCADGuiPy FreeCADGui)
    set_python_prefix_suffix(FreeCADGuiPy)
    if(WIN32)
        # Name clash with target "FreeCADGui" Must be called "FreeCADGuiPy_d" and "FreeCADGuiPy" to
        # work so override default
        set_target_properties(FreeCADGuiPy PROPERTIES PDB_NAME_DEBUG "FreeCADGuiPy_d")
        set_target_properties(FreeCADGuiPy PROPERTIES PDB_NAME_RELEASE "FreeCADGuiPy")
    endif(WIN32)

    if(WIN32)
        install(
            TARGETS FreeCADGuiPy
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
    else(WIN32)
        install(TARGETS FreeCADGuiPy LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
    endif(WIN32)
endif(BUILD_GUI)
