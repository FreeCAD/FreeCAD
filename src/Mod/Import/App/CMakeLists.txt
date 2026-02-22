# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

set(Import_LIBS
    Part
    ${OCC_OCAF_LIBRARIES}
    ${OCC_OCAF_DEBUG_LIBRARIES}
)

SET(Import_SRCS
    AppImport.cpp
    AppImportPy.cpp
    ExportOCAF.cpp
    ExportOCAF.h
    ExportOCAF2.cpp
    ExportOCAF2.h
    ImportOCAF.cpp
    ImportOCAF.h
    ImportOCAF2.cpp
    ImportOCAF2.h
    #ImportOCAFAssembly.cpp
    #ImportOCAFAssembly.h
    ReaderGltf.cpp
    ReaderGltf.h
    ReaderIges.cpp
    ReaderIges.h
    ReaderStep.cpp
    ReaderStep.h
    StepShape.pyi
    StepShape.h
    StepShape.cpp
    StepShapePyImp.cpp
    PreCompiled.h
    Tools.cpp
    Tools.h
    WriterGltf.cpp
    WriterGltf.h
    WriterIges.cpp
    WriterIges.h
    WriterStep.cpp
    WriterStep.h
    SketchExportHelper.cpp
    SketchExportHelper.h
    dxf/ImpExpDxf.cpp
    dxf/ImpExpDxf.h
    dxf/dxf.cpp
    dxf/dxf.h
)

SET(SCL_Resources
    SCL/__init__.py
    SCL/AggregationDataTypes.py
    SCL/BaseType.py
    SCL/Builtin.py
    SCL/ConstructedDataTypes.py
    SCL/essa_par.py
    SCL/Model.py
    SCL/Part21.py
    SCL/Rules.py
    SCL/SCLBase.py
    SCL/SimpleDataTypes.py
    SCL/TypeChecker.py
    SCL/Utils.py
    SCL/SimpleReader.py
    SCL/Aufspannung.stp
    SCL/gasket1.p21
    SCL/Product1.stp
    SCL_output/automotive_design.py     # AP214e3
    SCL_output/ifc2x3.py                # IFC
    SCL_output/ifc4.py                  # IFC 4
    PlmXmlParser.py
)
SOURCE_GROUP("SCL" FILES ${SCL_Resources})

generate_from_py(StepShape)

add_library(Import SHARED ${Import_SRCS})

if(FREECAD_USE_PCH)
    target_precompile_headers(Import PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:"${CMAKE_CURRENT_LIST_DIR}/PreCompiled.h">
    )
endif(FREECAD_USE_PCH)

target_include_directories(
    Import
    PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(Import ${Import_LIBS})

if (MSVC)
    target_compile_options(Import PRIVATE /wd4251 /wd4275)
endif()

if (FREECAD_WARN_ERROR)
    target_compile_warn_error(Import)
endif()

ADD_CUSTOM_TARGET(ImportPy ALL
    SOURCES ${SCL_Resources}
)

fc_target_copy_resource(ImportPy
    ${CMAKE_SOURCE_DIR}/src/Mod/Import/App
    ${CMAKE_BINARY_DIR}/Mod/Import
    ${SCL_Resources})

SET_BIN_DIR(Import Import /Mod/Import)
SET_PYTHON_PREFIX_SUFFIX(Import)

INSTALL(TARGETS Import DESTINATION ${CMAKE_INSTALL_LIBDIR})
