# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Joao Matos
# SPDX-FileNotice: Part of the FreeCAD project.

setup_qt_test(AsyncPreviewControllerTest)
setup_qt_test(DeferredDialogRejectTest)

target_include_directories(AsyncPreviewControllerTest_Tests_run PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_include_directories(DeferredDialogRejectTest_Tests_run PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

set(_partdesign_material_resource_targets
    MaterialLib
    FluidMaterialLib
    AppearanceLib
    PatternLib
    MachiningLib
    MaterialModelLib
)

setup_qt_module_test(TaskTransformedParametersTest
    MODULE_LIBS PartDesignGui
    MODULE_DEPENDS ${_partdesign_material_resource_targets}
)
setup_qt_module_test(TaskExtrudeParametersTest
    MODULE_LIBS PartDesignGui
    MODULE_DEPENDS ${_partdesign_material_resource_targets}
)
