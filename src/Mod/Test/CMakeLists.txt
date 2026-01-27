
SET(Test_SRCS
    __init__.py
    Init.py
    BaseTests.py
    Document.py
    GuiDocument.py
    Metadata.py
    StringHasher.py
    Menu.py
    TestApp.py
    TestGui.py
    UnicodeTests.py
    UnitTests.py
    Workbench.py
    unittestgui.py
    testmakeWireString.py
    TestPythonSyntax.py
    TestPerf.py
    TestCoinNodeSnapshots.py
)

SET(TestData_SRCS
    TestData/basic_metadata.xml
    TestData/bad_root_node.xml
    TestData/bad_xml.xml
    TestData/bad_version.xml
    TestData/content_items.xml
    TestData/DXFSample.dxf
)

SOURCE_GROUP("" FILES ${Test_SRCS} ${TestData_SRCS})

if(BUILD_GUI)
    add_subdirectory(Gui)
    list (APPEND Test_SRCS InitGui.py)
endif(BUILD_GUI)

ADD_CUSTOM_TARGET(Test ALL
    SOURCES ${Test_SRCS} ${TestData_SRCS}
)

fc_copy_sources(Test "${CMAKE_BINARY_DIR}/Mod/Test" ${Test_SRCS} ${TestData_SRCS})

# Coin node snapshot visual test baselines (Python unittest `TestCoinNodeSnapshots`).
# For out-of-source builds, copy baselines into the build tree so the test can find them
# without requiring FC_VISUAL_BASELINE_DIR to be set.
set(_coin_node_snapshot_baseline_dir "${CMAKE_SOURCE_DIR}/tests/visual/baselines/coin-nodes")
if(EXISTS "${_coin_node_snapshot_baseline_dir}")
    file(
        GLOB
        TestCoinNodeSnapshotBaselines
        CONFIGURE_DEPENDS
        "${_coin_node_snapshot_baseline_dir}/*.png"
    )
    if(TestCoinNodeSnapshotBaselines)
        set(_coin_node_snapshot_baseline_outdir "${CMAKE_BINARY_DIR}/tests/visual/baselines/coin-nodes")
        set(_coin_node_snapshot_baseline_outputs)
        foreach(_baseline ${TestCoinNodeSnapshotBaselines})
            get_filename_component(_baseline_name "${_baseline}" NAME)
            set(_baseline_out "${_coin_node_snapshot_baseline_outdir}/${_baseline_name}")
            add_custom_command(
                OUTPUT "${_baseline_out}"
                COMMAND "${CMAKE_COMMAND}" -E make_directory "${_coin_node_snapshot_baseline_outdir}"
                COMMAND "${CMAKE_COMMAND}" -E copy "${_baseline}" "${_baseline_out}"
                DEPENDS "${_baseline}"
                COMMENT "Copying ${_baseline} to ${_baseline_out}"
            )
            list(APPEND _coin_node_snapshot_baseline_outputs "${_baseline_out}")
        endforeach()
        add_custom_target(TestCoinNodeSnapshotBaselines ALL DEPENDS ${_coin_node_snapshot_baseline_outputs})
        add_dependencies(Test TestCoinNodeSnapshotBaselines)
    endif()
endif()

# Vendored fonts used by visual snapshot tests.
# For out-of-source builds, copy them into the build tree so the test can find them
# without relying on host-installed fonts.
set(_coin_node_snapshot_font_dir "${CMAKE_SOURCE_DIR}/tests/visual/fonts")
if(EXISTS "${_coin_node_snapshot_font_dir}")
    file(
        GLOB
        TestCoinNodeSnapshotFonts
        CONFIGURE_DEPENDS
        "${_coin_node_snapshot_font_dir}/*"
    )
    if(TestCoinNodeSnapshotFonts)
        set(_coin_node_snapshot_font_outdir "${CMAKE_BINARY_DIR}/tests/visual/fonts")
        set(_coin_node_snapshot_font_outputs)
        foreach(_font_file ${TestCoinNodeSnapshotFonts})
            get_filename_component(_font_name "${_font_file}" NAME)
            set(_font_out "${_coin_node_snapshot_font_outdir}/${_font_name}")
            add_custom_command(
                OUTPUT "${_font_out}"
                COMMAND "${CMAKE_COMMAND}" -E make_directory "${_coin_node_snapshot_font_outdir}"
                COMMAND "${CMAKE_COMMAND}" -E copy "${_font_file}" "${_font_out}"
                DEPENDS "${_font_file}"
                COMMENT "Copying ${_font_file} to ${_font_out}"
            )
            list(APPEND _coin_node_snapshot_font_outputs "${_font_out}")
        endforeach()
        add_custom_target(TestCoinNodeSnapshotFonts ALL DEPENDS ${_coin_node_snapshot_font_outputs})
        add_dependencies(Test TestCoinNodeSnapshotFonts)
    endif()
endif()

INSTALL(
    FILES
        ${Test_SRCS}
    DESTINATION
        Mod/Test
)

INSTALL(
    FILES
        ${TestData_SRCS}
    DESTINATION
        Mod/Test/TestData
)
