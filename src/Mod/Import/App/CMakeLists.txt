include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${XercesC_INCLUDE_DIRS}
)

link_directories(${OCC_LIBRARY_DIR})

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
    StepShapePy.xml
    StepShape.h
    StepShape.cpp
    StepShapePyImp.cpp
    PreCompiled.cpp
    PreCompiled.h
    Tools.cpp
    Tools.h
    WriterGltf.cpp
    WriterGltf.h
    WriterIges.cpp
    WriterIges.h
    WriterStep.cpp
    WriterStep.h
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

generate_from_xml(StepShapePy)

add_library(Import SHARED ${Import_SRCS})
target_link_libraries(Import ${Import_LIBS})

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
