if(WIN32)
    add_definitions(-DFCApp)
    add_definitions(-DBOOST_DYN_LINK)
endif(WIN32)

# This causes some problems with the resource files to be found, especially with the StartPage
IF(RESOURCEDIR)
    add_definitions(-DRESOURCEDIR="${RESOURCEDIR}")
ENDIF(RESOURCEDIR)

IF(DOCDIR)
    add_definitions(-DDOCDIR="${DOCDIR}")
ENDIF(DOCDIR)

include_directories(
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${PYTHON_INCLUDE_DIRS}
    ${XercesC_INCLUDE_DIRS}
    ${QT_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
)

set(FreeCADApp_LIBS
    FreeCADBase
    ${Boost_LIBRARIES}
    ${QT_QTCORE_LIBRARY}
    ${QT_QTXML_LIBRARY}
)

generate_from_xml(DocumentPy)
generate_from_xml(DocumentObjectPy)
generate_from_xml(DocumentObjectGroupPy)
generate_from_xml(ComplexGeoDataPy)
generate_from_xml(PropertyContainerPy)
generate_from_xml(MaterialPy)

generate_from_py(FreeCADInit InitScript.h)
generate_from_py(FreeCADTest TestScript.h)

SET(FreeCADApp_XML_SRCS
    DocumentObjectGroupPy.xml
    DocumentObjectPy.xml
    DocumentPy.xml
    PropertyContainerPy.xml
    ComplexGeoDataPy.xml
    MaterialPy.xml
)
SOURCE_GROUP("XML" FILES ${FreeCADApp_XML_SRCS})

# The document stuff
SET(Document_CPP_SRCS
    Annotation.cpp
    Document.cpp
    DocumentObject.cpp
    DocumentObjectFileIncluded.cpp
    DocumentObjectGroup.cpp
    DocumentObjectGroupPyImp.cpp
    DocumentObjectPyImp.cpp
    DocumentObserver.cpp
    DocumentObserverPython.cpp
    DocumentPyImp.cpp
    Expression.cpp
    FeaturePython.cpp
    FeatureTest.cpp
    GeoFeature.cpp
    InventorObject.cpp
    MeasureDistance.cpp
    Placement.cpp
    Plane.cpp
    Range.cpp
    Transactions.cpp
    VRMLObject.cpp
    MaterialObject.cpp
    MergeDocuments.cpp
)

SET(Document_HPP_SRCS
    Annotation.h
    Document.h
    DocumentObject.h
    DocumentObjectFileIncluded.h
    DocumentObjectGroup.h
    DocumentObserver.h
    DocumentObserverPython.h
    Expression.h
    ExpressionVisitors.h
    FeatureCustom.h
    FeaturePython.h
    FeaturePythonPyImp.h
    FeaturePythonPyImp.inl
    FeatureTest.h
    GeoFeature.h
    InventorObject.h
    MeasureDistance.h
    Placement.h
    Plane.h
    Range.h
    Transactions.h
    VRMLObject.h
    MaterialObject.h
    MergeDocuments.h
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
    Branding.cpp
    ColorModel.cpp
    ComplexGeoData.cpp
    ComplexGeoDataPyImp.cpp
    Enumeration.cpp
    Material.cpp
    MaterialPyImp.cpp
)

SET(FreeCADApp_HPP_SRCS
    ${Document_HPP_SRCS}
    ${Properties_HPP_SRCS}
    Application.h
    Branding.h
    ColorModel.h
    ComplexGeoData.h
    Enumeration.h
    Material.h
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

if(MSVC)
    add_definitions(-D_PreComp_)
    ADD_MSVC_PRECOMPILED_HEADER(FreeCADApp PreCompiled.h PreCompiled.cpp FreeCADApp_CPP_SRCS)
endif(MSVC)

add_library(FreeCADApp SHARED ${FreeCADApp_SRCS})

target_link_libraries(FreeCADApp ${FreeCADApp_LIBS})

SET_BIN_DIR(FreeCADApp FreeCADApp)

if(WIN32)
    INSTALL(TARGETS FreeCADApp
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
else(WIN32)
    INSTALL(TARGETS FreeCADApp
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
endif(WIN32)
