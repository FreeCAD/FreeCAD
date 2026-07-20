# SPDX-License-Identifier: LGPL-2.1-or-later

add_executable(App_tests_run
        AsyncRecompute.cpp
        Application.cpp
        ApplicationDirectories.cpp
        BackupPolicy.cpp
        Branding.cpp
        ComplexGeoData.cpp
        Document.cpp
        DocumentObject.cpp
        DocumentObserver.cpp
        Expression.cpp
        ExpressionParser.cpp
        ExpressionTokenizer.cpp
        ElementMap.cpp
        ElementNamingUtils.cpp
        IndexedName.cpp
        License.cpp
        Link.cpp
        MappedElement.cpp
        MappedName.cpp
        Metadata.cpp
        ProjectFile.cpp
        PropertyFile.cpp
        Property.h
        Property.cpp
        PropertyExpressionEngine.cpp
        ProgramInformation.cpp
        VarSet.cpp
        VRMLObject.cpp
)

if(FREECAD_HAS_QT)
    target_sources(App_tests_run PRIVATE StringHasher.cpp)
endif()

target_compile_definitions(App_tests_run PRIVATE DATADIR="${CMAKE_SOURCE_DIR}/data")

target_link_libraries(App_tests_run PRIVATE
    GTest::gmock_main
    ${Python3_LIBRARIES}
    FreeCADApp
)
