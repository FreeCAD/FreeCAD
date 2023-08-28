if(WIN32)
    add_definitions(-DFCApp)
    add_definitions(-DBOOST_DYN_LINK)
endif(WIN32)

if(FREECAD_RELEASE_SEH)
    add_definitions(-DHAVE_SEH)
endif(FREECAD_RELEASE_SEH)

# This causes some problems with the resource files to be found, especially with the StartPage
IF(RESOURCEDIR)
    add_definitions(-DRESOURCEDIR="${RESOURCEDIR}")
ENDIF(RESOURCEDIR)

IF(LIBRARYDIR)
    add_definitions(-DLIBRARYDIR="${LIBRARYDIR}")
ENDIF(LIBRARYDIR)

IF(DOCDIR)
    add_definitions(-DDOCDIR="${DOCDIR}")
ENDIF(DOCDIR)

#if you want to use the old DAG structure uncomment this line
#add_definitions(-DUSE_OLD_DAG)

# -----------------------------------------------------------------------------

#write relevant cmake variables to a file for later access with python. Exported are all variables
#starting with BUILD. As the variable only exists if the user set it to ON a dict is useless, we
#use a python list for export.
set(_vars "const char CMakeVariables[] =\"cmake = [")
set(_delim "")
get_cmake_property(_variableNames VARIABLES)
foreach (_variableName ${_variableNames})
    if (${_variableName})
        STRING(REGEX MATCH "^[_]?[^_]*" _prefix "${_variableName}_")
        if(${_prefix} STREQUAL "BUILD")
            STRING(REPLACE "\\" "\\\\" _name ${_variableName})
            set(_vars "${_vars}${_delim}\\n\"\n\"\\\"${_name}\\\"")
            set(_delim ",")
        endif()
    endif ()
endforeach()
set(_vars "${_vars}]\\n\" \n;")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/CMakeScript.hh "${_vars}" )

fc_copy_file_if_different(
    "${CMAKE_CURRENT_BINARY_DIR}/CMakeScript.hh"
    "${CMAKE_CURRENT_BINARY_DIR}/CMakeScript.h"
)

# -----------------------------------------------------------------------------

include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${PYTHON_INCLUDE_DIRS}
    ${XercesC_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
)

set(FreeCADApp_LIBS
    FreeCADBase
    ${Boost_LIBRARIES}
    fmt::fmt
)

include_directories(
    ${QtCore_INCLUDE_DIRS}
    ${QtXml_INCLUDE_DIRS}
)
list(APPEND FreeCADApp_LIBS
        ${QtCore_LIBRARIES}
        ${QtXml_LIBRARIES}
)

generate_from_xml(DocumentPy)
generate_from_xml(DocumentObjectPy)
generate_from_xml(ExtensionPy)
generate_from_xml(ExtensionContainerPy)
generate_from_xml(DocumentObjectExtensionPy)
generate_from_xml(GroupExtensionPy)
generate_from_xml(LinkBaseExtensionPy)
generate_from_xml(DocumentObjectGroupPy)
generate_from_xml(GeoFeaturePy)
generate_from_xml(GeoFeatureGroupExtensionPy)
generate_from_xml(MetadataPy)
generate_from_xml(OriginGroupExtensionPy)
generate_from_xml(PartPy)
generate_from_xml(StringHasherPy)
generate_from_xml(StringIDPy)

generate_from_xml(ComplexGeoDataPy)
generate_from_xml(PropertyContainerPy)
generate_from_xml(MaterialPy)

generate_from_py(FreeCADInit InitScript.h)
generate_from_py(FreeCADTest TestScript.h)

SET(FreeCADApp_XML_SRCS
    ExtensionPy.xml
    ExtensionContainerPy.xml
    DocumentObjectExtensionPy.xml
    GroupExtensionPy.xml
    LinkBaseExtensionPy.xml
    MetadataPy.xml
    DocumentObjectGroupPy.xml
    DocumentObjectPy.xml
    GeoFeaturePy.xml
    GeoFeatureGroupExtensionPy.xml
    OriginGroupExtensionPy.xml
    PartPy.xml
    DocumentPy.xml
    PropertyContainerPy.xml
    ComplexGeoDataPy.xml
    MaterialPy.xml
    StringHasherPy.xml
    StringIDPy.xml
)
SOURCE_GROUP("XML" FILES ${FreeCADApp_XML_SRCS})

# The document stuff
SET(Document_CPP_SRCS
    Annotation.cpp
    Document.cpp
    DocumentObject.cpp
    Extension.cpp
    ExtensionPyImp.cpp
    DocumentObjectExtension.cpp
    DocumentObjectExtensionPyImp.cpp
    ExtensionContainer.cpp
    ExtensionContainerPyImp.cpp
    Graphviz.cpp
    GroupExtension.cpp
    GroupExtensionPyImp.cpp
    DocumentObjectFileIncluded.cpp
    DocumentObjectGroup.cpp
    DocumentObjectGroupPyImp.cpp
    GeoFeaturePyImp.cpp
    DocumentObjectPyImp.cpp
    DocumentObserver.cpp
    DocumentObserverPython.cpp
    DocumentPyImp.cpp
    Expression.cpp
    ExpressionTokenizer.cpp
    FeaturePython.cpp
    FeatureTest.cpp
    GeoFeature.cpp
    GeoFeatureGroupExtensionPyImp.cpp
    GeoFeatureGroupExtension.cpp
    ImagePlane.cpp
    OriginGroupExtensionPyImp.cpp
    OriginGroupExtension.cpp
    PartPyImp.cpp
    Part.cpp
    Origin.cpp
    Path.cpp
    InventorObject.cpp
    MeasureDistance.cpp
    Placement.cpp
    OriginFeature.cpp
    Range.cpp
    Transactions.cpp
    TransactionalObject.cpp
    VRMLObject.cpp
    MaterialObject.cpp
    MergeDocuments.cpp
    TextDocument.cpp
    Link.cpp
    LinkBaseExtensionPyImp.cpp
    License.h
)

SET(Document_HPP_SRCS
    Annotation.h
    Document.h
    DocumentObject.h
    Extension.h
    ExtensionContainer.h
    GroupExtension.h
    DocumentObjectExtension.h
    DocumentObjectFileIncluded.h
    DocumentObjectGroup.h
    DocumentObserver.h
    DocumentObserverPython.h
    Expression.h
    ExpressionParser.h
    ExpressionTokenizer.h
    ExpressionVisitors.h
    FeatureCustom.h
    FeaturePython.h
    FeaturePythonPyImp.h
    FeaturePythonPyImp.inl
    FeatureTest.h
    GeoFeature.h
    GeoFeatureGroupExtension.h
    ImagePlane.h
    OriginGroupExtension.h
    Part.h
    Origin.h
    Path.h
    InventorObject.h
    MeasureDistance.h
    Placement.h
    OriginFeature.h
    Range.h
    Transactions.h
    TransactionalObject.h
    VRMLObject.h
    MaterialObject.h
    MergeDocuments.h
    TextDocument.h
    Link.h
)
SET(Document_SRCS
    ${Document_CPP_SRCS}
    ${Document_HPP_SRCS}
)
SOURCE_GROUP("Document" FILES ${Document_SRCS})

# The property stuff
SET(Properties_CPP_SRCS
    DynamicProperty.cpp
    ObjectIdentifier.cpp
    Property.cpp
    PropertyContainer.cpp
    PropertyContainerPyImp.cpp
    PropertyFile.cpp
    PropertyGeo.cpp
    PropertyLinks.cpp
    PropertyPythonObject.cpp
    PropertyStandard.cpp
    PropertyUnits.cpp
    PropertyExpressionEngine.cpp
)
SET(Properties_HPP_SRCS
    DynamicProperty.h
    ObjectIdentifier.h
    Property.h
    PropertyContainer.h
    PropertyFile.h
    PropertyGeo.h
    PropertyLinks.h
    PropertyPythonObject.h
    PropertyStandard.h
    PropertyUnits.h
    PropertyExpressionEngine.h
)
SET(Properties_SRCS
    ${Properties_CPP_SRCS}
    ${Properties_HPP_SRCS}
)
SOURCE_GROUP("Properties" FILES ${Properties_SRCS})

SET(FreeCADApp_CPP_SRCS
    ${Document_CPP_SRCS}
    ${Properties_CPP_SRCS}
    Application.cpp
    ApplicationPy.cpp
    AutoTransaction.cpp
    Branding.cpp
    Color.cpp
    ColorModel.cpp
    ComplexGeoData.cpp
    ComplexGeoDataPyImp.cpp
    ElementMap.cpp
    Enumeration.cpp
    IndexedName.cpp
    MappedElement.cpp
    MappedName.cpp
    Material.cpp
    MaterialPyImp.cpp
    Metadata.cpp
    MetadataPyImp.cpp
    ElementNamingUtils.cpp
    StringHasher.cpp
    StringHasherPyImp.cpp
    StringIDPyImp.cpp
)

SET(FreeCADApp_HPP_SRCS
    ${Document_HPP_SRCS}
    ${Properties_HPP_SRCS}
    Application.h
    AutoTransaction.h
    Branding.h
    Color.h
    ColorModel.h
    ComplexGeoData.h
    ElementMap.h
    Enumeration.h
    IndexedName.h
    MappedName.h
    MappedElement.h
    Material.h
    Metadata.h
    ElementNamingUtils.h
    StringHasher.h
)

SET(FreeCADApp_SRCS
    ${FreeCADApp_CPP_SRCS}
    ${FreeCADApp_HPP_SRCS}
    ${FreeCADApp_XML_SRCS}
    FreeCADInit.py
    FreeCADTest.py
    PreCompiled.cpp
    PreCompiled.h
)

if(FREECAD_USE_PCH)
    add_definitions(-D_PreComp_)
    ADD_MSVC_PRECOMPILED_HEADER(FreeCADApp PreCompiled.h PreCompiled.cpp FreeCADApp_CPP_SRCS)
endif(FREECAD_USE_PCH)

add_library(FreeCADApp SHARED ${FreeCADApp_SRCS})
target_link_libraries(FreeCADApp ${FreeCADApp_LIBS})

add_dependencies(FreeCADApp fc_version)

SET_BIN_DIR(FreeCADApp FreeCADApp)

if(WIN32)
    INSTALL(TARGETS FreeCADApp
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
else(WIN32)
    INSTALL(TARGETS FreeCADApp
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
endif(WIN32)
