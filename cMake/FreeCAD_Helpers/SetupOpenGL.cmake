macro(SetupOpenGL)
# -------------------------------- OpenGL --------------------------------

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
