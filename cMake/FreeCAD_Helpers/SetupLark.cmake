# SPDX-FileNotice: Part of the FreeCAD project.

macro(SetupLark)
    # ------------------------------ Lark ------------------------------

    find_package(LARK MODULE REQUIRED)
    message(STATUS "Found Lark: version ${LARK_VERSION}")

endmacro()
