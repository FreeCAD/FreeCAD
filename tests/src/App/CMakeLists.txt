# SPDX-License-Identifier: LGPL-2.1-or-later

add_executable(App_tests_run
        AsyncRecompute.cpp
        Application.cpp
        ApplicationDirectories.cpp
        ApplicationExportTypes.cpp
        BackupPolicy.cpp
        Branding.cpp
        ComplexGeoData.cpp
        Document.cpp
        DocumentObject.cpp
        DocumentObserver.cpp
        Expression.cpp
        ExpressionParser.cpp
        QuantityInput.cpp
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
        StringHasher.cpp
        VarSet.cpp
        VRMLObject.cpp
)

target_compile_definitions(App_tests_run PRIVATE DATADIR="${CMAKE_SOURCE_DIR}/data")

target_link_libraries(App_tests_run PRIVATE
    GTest::gmock_main
    ${Python3_LIBRARIES}
    FreeCADApp
)

add_test(
    NAME PythonSignalHandlers
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/PythonSignalHandlers.py
            $<TARGET_FILE_DIR:FreeCADMainPy>
)
set_tests_properties(PythonSignalHandlers PROPERTIES TIMEOUT 60)
if(WIN32)
    set_tests_properties(PythonSignalHandlers PROPERTIES
        ENVIRONMENT_MODIFICATION "${_test_env_mod}")
endif()

if(BUILD_PART)
    add_test(
        NAME PythonModelingSignalHandlers
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/PythonSignalHandlers.py
                $<TARGET_FILE_DIR:FreeCADMainPy> $<TARGET_FILE_DIR:Part>
    )
    set_tests_properties(PythonModelingSignalHandlers PROPERTIES TIMEOUT 60)
    if(WIN32)
        set_tests_properties(PythonModelingSignalHandlers PROPERTIES
            ENVIRONMENT_MODIFICATION "${_test_env_mod}")
    endif()
endif()
