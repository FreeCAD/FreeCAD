target_compile_definitions(Tests_run PRIVATE DATADIR="${CMAKE_SOURCE_DIR}/data")

target_sources(
    Tests_run
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/Application.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/Branding.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/Color.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/ComplexGeoData.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/Document.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/DocumentObject.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/DocumentObserver.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/Expression.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/ExpressionParser.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/ElementMap.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/ElementNamingUtils.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/IndexedName.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/License.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/MappedElement.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/MappedName.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/Metadata.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/ProjectFile.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/Property.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/PropertyExpressionEngine.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/StringHasher.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/VarSet.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/VRMLObject.cpp
)
