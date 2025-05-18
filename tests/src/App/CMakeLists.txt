target_compile_definitions(Tests_run PRIVATE DATADIR="${CMAKE_SOURCE_DIR}/data")

target_sources(Tests_run PRIVATE
        Application.cpp
        Branding.cpp
        ComplexGeoData.cpp
        Document.cpp
        DocumentObject.cpp
        DocumentObserver.cpp
        Expression.cpp
        ExpressionParser.cpp
        ElementMap.cpp
        ElementNamingUtils.cpp
        IndexedName.cpp
        License.cpp
        MappedElement.cpp
        MappedName.cpp
        Metadata.cpp
        ProjectFile.cpp
        Property.h
        Property.cpp
        PropertyExpressionEngine.cpp
        StringHasher.cpp
        VarSet.cpp
        VRMLObject.cpp
)
