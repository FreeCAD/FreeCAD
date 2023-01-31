macro(PrintFinalReport)

    # simple report line
    macro(simple)
        set(name ${ARGV0})
        set(value ${ARGV1})
        if(NOT value)
            set(value "-undefined-")
        endif()
        string(APPEND name ":                     ")
        string(SUBSTRING ${name} 0 20 nameStr)
        list(APPEND simpleLines "${nameStr} ${value}")
    endmacro()

    # report line with conditional data
    macro(conditional)
        set(name ${ARGV0})
        set(condition ${ARGV1})
        set(ifFalse ${ARGV2})
        set(ifTrue ${ARGV3})
        if(${condition})
            set(out ${ifTrue})
        else()
            set(out ${ifFalse})
        endif()
        simple(${name} ${out})
    endmacro()

    ################ Report data ##################

    simple(BuildType ${CMAKE_BUILD_TYPE})
    simple(Compiler "${CMAKE_CXX_COMPILER} (${CMAKE_CXX_COMPILER_VERSION})")
    simple(Flags ${CMAKE_CXX_FLAGS})
    simple(Standard "Requires C++${CMAKE_CXX_STANDARD}")
    # Qt5 needs/sets PYTHON_CONFIG_SUFFIX regarding Shiboken
    simple(Python "${Python3_VERSION} [${PYTHON_EXECUTABLE}] Suffix: [${PYTHON_CONFIG_SUFFIX}]")
    conditional(pybind11 pybind11_FOUND "not enabled" ${pybind11_VERSION})
    simple(Boost ${Boost_VERSION})
    simple(XercesC "${XercesC_VERSION} [${XercesC_LIBRARIES}] [${XercesC_INCLUDE_DIRS}]")
    simple(ZLIB "${ZLIB_VERSION_STRING}")
    simple(PyCXX "${PYCXX_VERSION} [${PYCXX_INCLUDE_DIR}]")
    simple(OCC "${OCC_VERSION_STRING} [${OCC_LIBRARY_DIR}] [${OCC_INCLUDE_DIR}]")
    simple(OCC_Libs "[${OCC_LIBRARIES}]")
    if(BUILD_SMESH)
        if(FREECAD_USE_EXTERNAL_SMESH)
            simple(SMESH "${SMESH_VERSION_MAJOR}.${SMESH_VERSION_MINOR}.${SMESH_VERSION_PATCH}.${SMESH_VERSION_TWEAK}")
        else()
            simple(SMESH "${SMESH_VERSION_MAJOR}.${SMESH_VERSION_MINOR}.${SMESH_VERSION_PATCH}.${SMESH_VERSION_TWEAK} build internal")
            simple(MEDFile "${MEDFILE_VERSION} [${MEDFILE_LIBRARIES}] [${MEDFILE_INCLUDE_DIRS}]")
            simple(HDF5 ${HDF5_VERSION})
            simple(VTK ${VTK_VERSION})
        endif()
    else()
        simple(SMESH "do not build")
    endif()
    conditional(NETGEN NETGEN_FOUND
                "not enabled"
                "${NETGEN_VERSION_MAJOR}.${NETGEN_VERSION_MINOR}.${NETGEN_VERSION_PATCH} (${NETGEN_VERSION}) [${NETGEN_DEFINITIONS}] [${NETGEN_CXX_FLAGS}] [${NGLIB_INCLUDE_DIR}] [${NGLIB_LIBRARIES}] [${NETGEN_INCLUDE_DIRS}]"
    )
    #simple(OpenCV ${OpenCV_VERSION})
    conditional(SWIG SWIG_FOUND "not found" ${SWIG_VERSION})
    conditional(Eigen3 EIGEN3_FOUND "not found" ${EIGEN3_VERSION})
    conditional(QtConcurrent BUILD_GUI "not needed" ${QtConcurrent_VERSION})
    simple(QtCore ${QtCore_VERSION})
    simple(QtNetwork ${QtNetwork_VERSION})
    conditional(QtOpenGL BUILD_GUI "not needed" ${QtOpenGL_VERSION})
    conditional(QtPrintSupport BUILD_GUI "not needed" ${QtPrintSupport_VERSION})
    conditional(QtSvg BUILD_GUI "not needed" ${QtSvg_VERSION})
    conditional(QtUiTools BUILD_GUI "not needed" ${QtUiTools_VERSION})
    conditional(QtWidgets BUILD_GUI "not needed" ${QtWidgets_VERSION})
    simple(QtXml ${QtXml_VERSION})
    if (BUILD_GUI)
        conditional(QtWebEngineWidgets BUILD_WEB "not needed (BUILD_WEB is OFF)" ${QtWebEngineWidgets_VERSION})
        conditional(DesignerPlugin BUILD_DESIGNER_PLUGIN
                    "not built (BUILD_DESIGNER_PLUGIN is OFF)"
                    "[${DESIGNER_PLUGIN_LOCATION}/${libFreeCAD_widgets}]"
        )
    else()
        simple(QtWebKitWidgets "not needed")
    endif()
    conditional(Shiboken SHIBOKEN_FOUND "not found" "${SHIBOKEN_VERSION} Base: [${SHIBOKEN_BASEDIR}]")
    conditional(PySide PYSIDE_FOUND "not found" "${PYSIDE_VERSION} [${PYSIDE_INCLUDE_DIR}]")
    conditional(PySideTools PYSIDE_TOOLS_FOUND
                "not found"
                "uic: [${PYSIDE_UIC_EXECUTABLE}] rcc: [${PYSIDE_RCC_EXECUTABLE}]"
    )
    if(FREECAD_USE_FREETYPE)
        conditional(Freetype FREETYPE_FOUND "not found" ${FREETYPE_VERSION_STRING})
    else()
        simple(Freetype "disabled")
    endif()
    simple(OpenGL_Lib [${OPENGL_gl_LIBRARY}])
    simple(OpenGLU_Lib [${OPENGL_glu_LIBRARY}])
    simple(OpenGLU_Incl [${OPENGL_INCLUDE_DIR}])
    simple(Coin3D "${COIN3D_VERSION} [${COIN3D_LIBRARIES}] [${COIN3D_INCLUDE_DIRS}]")
    simple(Pivy ${PIVY_VERSION})
    if (WIN32)
        #simple(SPNAV "not available yet for your OS") # FREECAD_USE_3DCONNEXION instead...
    else()
        conditional(SPNAV SPNAV_FOUND "not found" "[${SPNAV_LIBRARY}] [${SPNAV_INCLUDE_DIR}]")
    endif()
    conditional(Matplotlib MATPLOTLIB_FOUND "not found" ${MATPLOTLIB_VERSION})
    if(BUILD_VR)
        conditional(Rift RIFT_FOUND "not found" ${Rift_VERSION})
    else()
        simple(Rift "not enabled (BUILD_VR)")
    endif()
    if(DOXYGEN_FOUND)
        simple(Doxygen ${DOXYGEN_VERSION})
        simple(Language ${DOXYGEN_LANGUAGE})
        conditional(Coin3D_DOC COIN3D_DOC_FOUND "not found" ${COIN3D_DOC_PATH})
    else()
        simple(Doxygen "not found")
    endif()
    conditional(Tests BUILD_TEST "OFF" "ON")

    ############## End report data ####################

    message(STATUS "\n==============\n"
                   "Summary Report\n"
                   "==============\n")

    list(SORT simpleLines CASE INSENSITIVE)
    foreach(line ${simpleLines})
        message(STATUS ${line})
    endforeach()

endmacro()
