macro(SetupOpenGL)
# -------------------------------- OpenGL --------------------------------

    # If on a system with both a legacy GL library and GLVND, prefer the legacy library.
    # This is probably needed until we no longer have any gl.*ARB calls in the codebase
    # See, e.g. SoBrepFaceSet.cpp
    set(OpenGL_GL_PREFERENCE LEGACY)

    find_package(OpenGL)
    include(FindPackageMessage)
    if(OPENGL_GLU_FOUND)
        find_package_message(OPENGL_GLU
            "Found OpenGLU: ${OPENGL_glu_LIBRARY}"
            "[${OPENGL_glu_LIBRARY}][${OPENGL_INCLUDE_DIR}]")
    else(OPENGL_GLU_FOUND)
        message(FATAL_ERROR "======================\n"
                            "GLU library not found.\n"
                            "======================\n")
    endif(OPENGL_GLU_FOUND)

endmacro(SetupOpenGL)
