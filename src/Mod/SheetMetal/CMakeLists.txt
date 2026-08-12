# SPDX-License-Identifier: LGPL-2.1-or-later

set(SheetMetal_SRCS
    current.py
    engineering_mode.py
    ExtrudedCutout.py
    InitGui.py
    lookup.py
    SheetMetalBaseCmd.py
    SheetMetalBaseShapeCmd.py
    SheetMetalBend.py
    SheetMetalBendSolid.py
    SheetMetalCmd.py
    SheetMetalCornerReliefCmd.py
    SheetMetalExtendCmd.py
    SheetMetalFoldCmd.py
    SheetMetalFormingCmd.py
    SheetMetalFromSolid.py
    SheetMetalHem.py
    SheetMetalJunction.py
    SheetMetalKfactor.py
    SheetMetalNewUnfolder.py
    SheetMetalRelief.py
    SheetMetalSketch.py
    SheetMetalText.py
    SheetMetalTools.py
    SheetMetalUnfoldCmd.py
    SheetMetalUnfolder.py
    SketchOnSheetMetalCmd.py
    smwb_locator.py
    TestSheetMetal.py
)

set(SheetMetal_TEST_SRCS
    SMTests/__init__.py
    SMTests/testFolder.py
    SMTests/testKfactor.py
)

# These are runtime resources. CONFIGURE_DEPENDS makes CMake regenerate when
# translations, icons, or task panels are added or removed.
file(GLOB_RECURSE SheetMetal_RESOURCE_SRCS
    CONFIGURE_DEPENDS
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "Resources/icons/*.svg"
    "Resources/panels/*.ui"
    "Resources/translations/*.qm"
)

set(SheetMetal_METADATA_SRCS
    LICENSE
    package.xml
)

add_custom_target(SheetMetal ALL
    SOURCES
        ${SheetMetal_SRCS}
        ${SheetMetal_TEST_SRCS}
        ${SheetMetal_RESOURCE_SRCS}
        ${SheetMetal_METADATA_SRCS}
)

fc_copy_sources(
    SheetMetal
    "${CMAKE_BINARY_DIR}/Mod/SheetMetal"
    ${SheetMetal_SRCS}
    ${SheetMetal_TEST_SRCS}
    ${SheetMetal_RESOURCE_SRCS}
    ${SheetMetal_METADATA_SRCS}
)

install(
    FILES
        ${SheetMetal_SRCS}
        ${SheetMetal_METADATA_SRCS}
    DESTINATION
        Mod/SheetMetal
)

install(
    FILES
        ${SheetMetal_TEST_SRCS}
    DESTINATION
        Mod/SheetMetal/SMTests
)

foreach(resource_file IN LISTS SheetMetal_RESOURCE_SRCS)
    get_filename_component(resource_directory "${resource_file}" DIRECTORY)
    install(
        FILES
            "${resource_file}"
        DESTINATION
            "Mod/SheetMetal/${resource_directory}"
    )
endforeach()
