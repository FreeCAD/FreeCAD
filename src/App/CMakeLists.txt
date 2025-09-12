add_library(FreeCADApp SHARED)

if(WIN32)
    add_definitions(-DFCApp)
    add_definitions(-DBOOST_DYN_LINK)
endif(WIN32)

if(BUILD_TRACY_FRAME_PROFILER)
    add_definitions(-DBUILD_TRACY_FRAME_PROFILER)
endif()

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

target_include_directories(
    FreeCADApp
    PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_include_directories(
    FreeCADApp
    SYSTEM
    PUBLIC
    ${QtCore_INCLUDE_DIRS}
    ${QtXml_INCLUDE_DIRS}
)

set(FreeCADApp_LIBS
    FreeCADBase
    ${Boost_LIBRARIES}
    fmt::fmt
)

if(BUILD_TRACY_FRAME_PROFILER)
    list(APPEND FreeCADApp_LIBS TracyClient)
endif()

list(APPEND FreeCADApp_LIBS
        ${QtCore_LIBRARIES}
        ${QtXml_LIBRARIES}
)

generate_from_py(ApplicationDirectories)
generate_from_py(Document)
generate_from_py(DocumentObject)
generate_from_py(Extension)
generate_from_py(ExtensionContainer)
generate_from_py(DocumentObjectExtension)
generate_from_py(GroupExtension)
generate_from_py(LinkBaseExtension)
generate_from_py(DocumentObjectGroup)
generate_from_py(GeoFeature)
generate_from_py(GeoFeatureGroupExtension)
generate_from_py(SuppressibleExtension)
generate_from_py(Metadata)
generate_from_py(OriginGroupExtension)
generate_from_py(Part)
generate_from_py(StringHasher)
generate_from_py(StringID)

generate_from_py(ComplexGeoData)
generate_from_py(PropertyContainer)
generate_from_py(Material)
generate_from_py(MeasureManager)

generate_embed_from_py(FreeCADInit InitScript.h)
generate_embed_from_py(FreeCADTest TestScript.h)

SET(FreeCADApp_Pyi_SRCS
    ApplicationDirectories.pyi
    Extension.pyi
    ExtensionContainer.pyi
    DocumentObjectExtension.pyi
    GroupExtension.pyi
    LinkBaseExtension.pyi
    Metadata.pyi
    DocumentObjectGroup.pyi
    DocumentObject.pyi
    GeoFeature.pyi
    GeoFeatureGroupExtension.pyi
    OriginGroupExtension.pyi
    SuppressibleExtension.pyi
    Part.pyi
    Document.pyi
    PropertyContainer.pyi
    ComplexGeoData.pyi
    Material.pyi
    MeasureManager.pyi
    StringHasher.pyi
    StringID.pyi
)
SOURCE_GROUP("Pyi" FILES ${FreeCADApp_Pyi_SRCS})

# The document stuff
SET(Document_CPP_SRCS
    Annotation.cpp
    BackupPolicy.cpp
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
    SuppressibleExtensionPyImp.cpp
    SuppressibleExtension.cpp
    PartPyImp.cpp
    Part.cpp
    Origin.cpp
    Path.cpp
    InventorObject.cpp
    Placement.cpp
    ProjectFile.cpp
    Datums.cpp
    Range.cpp
    Transactions.cpp
    TransactionalObject.cpp
    VRMLObject.cpp
    MaterialObject.cpp
    MergeDocuments.cpp
    TextDocument.cpp
    Link.cpp
    LinkBaseExtensionPyImp.cpp
    VarSet.cpp
    License.h
)

SET(Document_HPP_SRCS
    Annotation.h
    BackupPolicy.h
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
    SuppressibleExtension.h
    Part.h
    Origin.h
    Path.h
    InventorObject.h
    Placement.h
    ProjectFile.h
    Datums.h
    Range.h
    Transactions.h
    TransactionalObject.h
    VRMLObject.h
    MaterialObject.h
    MergeDocuments.h
    TextDocument.h
    VarSet.h
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
    PropertyOverrides.h
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
    ApplicationDirectories.cpp
    ApplicationDirectoriesPyImp.cpp
    ApplicationPy.cpp
    AutoTransaction.cpp
    Branding.cpp
    CleanupProcess.cpp
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
    MeasureManager.cpp
    MeasureManagerPyImp.cpp
    Metadata.cpp
    MetadataPyImp.cpp
    ElementNamingUtils.cpp
    SafeMode.cpp
    Services.cpp
    StringHasher.cpp
    StringHasherPyImp.cpp
    StringIDPyImp.cpp
)

SET(FreeCADApp_HPP_SRCS
    ${Document_HPP_SRCS}
    ${Properties_HPP_SRCS}
    Application.h
    ApplicationDirectories.h
    AutoTransaction.h
    Branding.h
    CleanupProcess.h
    ColorModel.h
    ComplexGeoData.h
    ElementMap.h
    Enumeration.h
    IndexedName.h
    MappedName.h
    MappedElement.h
    Material.h
    MeasureManager.h
    Metadata.h
    ElementNamingUtils.h
    Services.h
    StringHasher.h
)

# auto-generate resource file with all available translations
set (FreeCADApp_TR_QRC ${CMAKE_CURRENT_BINARY_DIR}/Resources/App_translation.qrc)
qt_find_and_add_translation(QM_SRCS "Resources/translations/*_*.ts"
    ${CMAKE_CURRENT_BINARY_DIR}/Resources/translations)
qt_create_resource_file(${FreeCADApp_TR_QRC} ${QM_SRCS})
qt_add_resources(FreeCADApp_QRC_SRCS ${FreeCADApp_TR_QRC})

SET(FreeCADApp_SRCS
    ${FreeCADApp_QRC_SRCS}
    ${FreeCADApp_CPP_SRCS}
    ${FreeCADApp_HPP_SRCS}
    ${FreeCADApp_Pyi_SRCS}
    FreeCADInit.py
    FreeCADTest.py
    PreCompiled.h
)

if(FREECAD_USE_PCH)
    target_precompile_headers(FreeCADApp PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:"${CMAKE_CURRENT_LIST_DIR}/PreCompiled.h">
    )
endif(FREECAD_USE_PCH)

target_sources(FreeCADApp PRIVATE ${FreeCADApp_SRCS})
target_link_libraries(FreeCADApp ${FreeCADApp_LIBS})

if (MSVC)
    target_compile_definitions(FreeCADApp PRIVATE WIN32_LEAN_AND_MEAN)
    target_compile_options(FreeCADApp PRIVATE /wd4251 /wd4273 /wd4275)
endif()

if (FREECAD_WARN_ERROR)
    target_compile_warn_error(FreeCADApp)
endif()

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
