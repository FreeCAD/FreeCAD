add_executable(Base_tests_run
        Axis.cpp
        Base64.cpp
        ByteBuffer.cpp
        Bitmask.cpp
        BoundBox.cpp
        Builder3D.cpp
        Color.cpp
        Console.cpp
        CoordinateSystem.cpp
        CrashReporter_tests.cpp
        DualNumber.cpp
        DualQuaternion.cpp
        Exception.cpp
        FileLock.cpp
        FileInfo.cpp
        InputSource.cpp
        Handle.cpp
        HashUtils.cpp
        Matrix.cpp
        NumericFormatting.cpp
        NumericInput.cpp
        OkLch.cpp
        Parameter.cpp
        ParameterObserver.cpp
        PlatformPaths.cpp
        Placement.cpp
        Persistence.cpp
        PyException.cpp
        Quantity.cpp
        Reader.cpp
        Rotation.cpp
        SchemaTests.cpp
        ServiceProvider.cpp
        Sha1.cpp
        Stream.cpp
        StringUtils.cpp
        TimeInfo.cpp
        Tools.cpp
        Tools2D.cpp
        Tools3D.cpp
        Translation.cpp
        Translate.cpp
        UnlimitedUnsigned.cpp
        UniqueNameManager.cpp
        Unit.cpp
        UnitsApi.cpp
        Uuid.cpp
        UnitsSchemaFormat.cpp
        Vector3D.cpp
        ViewProj.cpp
        Writer.cpp
        XMLParser.cpp
        XMLTools.cpp
)

if(BUILD_GUI)
    setup_qt_test(InventorBuilder)
endif()

target_link_libraries(Base_tests_run PRIVATE
    GTest::gmock_main
    FreeCADApp
    ${Python3_LIBRARIES}
    ICU::uc ICU::i18n
)

# CrashReporter_tests decides what the Reader inside FreeCADBase is capable of from this define,
# so the two targets have to agree on it. FreeCADBase sets it PRIVATE, which does not propagate.
if(cpptrace_FOUND)
    target_compile_definitions(Base_tests_run PRIVATE FC_HAVE_CPPTRACE)
endif()

target_compile_definitions(Base_tests_run PRIVATE
    FREECAD_TEST_HAS_QT=$<BOOL:${FREECAD_HAS_QT}>
)
