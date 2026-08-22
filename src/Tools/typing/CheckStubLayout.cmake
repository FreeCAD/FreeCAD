# SPDX-License-Identifier: LGPL-2.1-or-later

if(NOT DEFINED FREECAD_PYTHON_STUBS_LAYOUT_ROOT)
    message(FATAL_ERROR "FREECAD_PYTHON_STUBS_LAYOUT_ROOT must point to the stub tree to check")
endif()

set(_expected_stubs
    FreeCAD/__init__.pyi
    FreeCAD/Base.pyi
    FreeCADGui/__init__.pyi
    Part/__init__.pyi
)

foreach(_stub IN LISTS _expected_stubs)
    set(_stub_path "${FREECAD_PYTHON_STUBS_LAYOUT_ROOT}/${_stub}")
    if(NOT EXISTS "${_stub_path}")
        message(FATAL_ERROR "Expected generated stub does not exist: ${_stub_path}")
    endif()
endforeach()

message(STATUS "FreeCAD Python stub layout is valid")
