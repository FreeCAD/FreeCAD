macro(SetupOpenGL)
# -------------------------------- OpenGL --------------------------------

    # If on a system with both a legacy GL library and GLVND, prefer the legacy library.
    # This is probably needed until we no longer have any gl.*ARB calls in the codebase
    # See, e.g. SoBrepFaceSet.cpp
    set(OpenGL_GL_PREFERENCE LEGACY)

    find_package(OpenGL)
    include(FindPackageMessage)

endmacro(SetupOpenGL)
