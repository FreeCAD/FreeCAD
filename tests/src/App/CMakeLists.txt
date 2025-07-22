add_executable(App_tests_run
        Application.cpp
        BackupPolicy.cpp
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

target_compile_definitions(App_tests_run PRIVATE DATADIR="${CMAKE_SOURCE_DIR}/data")

target_link_libraries(App_tests_run PRIVATE
    GTest::gtest_main
    GTest::gmock_main
    ${Google_Tests_LIBS}
    FreeCADApp
)
