add_subdirectory(App)

IF (BUILD_GUI)
    PYSIDE_WRAP_RC(Draft_QRC_SRCS Resources/Draft.qrc)
ENDIF (BUILD_GUI)

SET(Draft_SRCS_base
    Init.py
    InitGui.py
    Draft.py
    DraftTools.py
    DraftGui.py
    DraftVecUtils.py
    DraftGeomUtils.py
    WorkingPlane.py
    TestDraft.py
    TestDraftGui.py
)

SET(Draft_import
    importAirfoilDAT.py
    importDXF.py
    importDWG.py
    importOCA.py
    importSVG.py
)

SET (Draft_geoutils
    draftgeoutils/__init__.py
    draftgeoutils/general.py
    draftgeoutils/edges.py
    draftgeoutils/intersections.py
    draftgeoutils/sort_edges.py
    draftgeoutils/faces.py
    draftgeoutils/geometry.py
    draftgeoutils/geo_arrays.py
    draftgeoutils/wires.py
    draftgeoutils/arcs.py
    draftgeoutils/fillets.py
    draftgeoutils/offsets.py
    draftgeoutils/linear_algebra.py
    draftgeoutils/cuboids.py
    draftgeoutils/circles.py
    draftgeoutils/circles_apollonius.py
    draftgeoutils/circle_inversion.py
    draftgeoutils/circles_incomplete.py
)

SET(Draft_tests
    drafttests/__init__.py
    drafttests/auxiliary.py
    drafttests/test_import.py
    drafttests/test_import_gui.py
    drafttests/test_import_tools.py
    drafttests/test_pivy.py
    drafttests/test_array.py
    drafttests/test_creation.py
    drafttests/test_modification.py
    drafttests/test_svg.py
    drafttests/test_dxf.py
    drafttests/test_dwg.py
    drafttests/test_oca.py
    drafttests/test_airfoildat.py
    drafttests/test_draftgeomutils.py
    drafttests/draft_test_objects.py
    drafttests/README.md
)

SET(Draft_utilities
    draftutils/__init__.py
    draftutils/groups.py
    draftutils/init_tools.py
    draftutils/init_draft_statusbar.py
    draftutils/units.py
    draftutils/utils.py
    draftutils/gui_utils.py
    draftutils/todo.py
    draftutils/translate.py
    draftutils/messages.py
    draftutils/README.md
)

SET(Draft_functions
    draftfunctions/__init__.py
    draftfunctions/array.py
    draftfunctions/cut.py
    draftfunctions/downgrade.py
    draftfunctions/draftify.py
    draftfunctions/dxf.py
    draftfunctions/extrude.py
    draftfunctions/fuse.py
    draftfunctions/heal.py
    draftfunctions/join.py
    draftfunctions/mirror.py
    draftfunctions/move.py
    draftfunctions/offset.py
    draftfunctions/rotate.py
    draftfunctions/scale.py
    draftfunctions/split.py
    draftfunctions/svg.py
    draftfunctions/svgshapes.py
    draftfunctions/svgtext.py
    draftfunctions/upgrade.py
    draftfunctions/README.md
)

SET(Draft_make_functions
    draftmake/__init__.py
    draftmake/make_arc_3points.py
    draftmake/make_array.py
    draftmake/make_bezcurve.py
    draftmake/make_block.py
    draftmake/make_bspline.py
    draftmake/make_circle.py
    draftmake/make_circulararray.py
    draftmake/make_clone.py
    draftmake/make_copy.py
    draftmake/make_dimension.py
    draftmake/make_ellipse.py
    draftmake/make_facebinder.py
    draftmake/make_fillet.py
    draftmake/make_label.py
    draftmake/make_layer.py
    draftmake/make_line.py
    draftmake/make_orthoarray.py
    draftmake/make_patharray.py
    draftmake/make_point.py
    draftmake/make_pointarray.py
    draftmake/make_polararray.py
    draftmake/make_polygon.py
    draftmake/make_rectangle.py
    draftmake/make_shape2dview.py
    draftmake/make_shapestring.py
    draftmake/make_sketch.py
    draftmake/make_text.py
    draftmake/make_wire.py
    draftmake/make_wpproxy.py
    draftmake/make_hatch.py
    draftmake/README.md
)

SET(Draft_objects
    draftobjects/__init__.py
    draftobjects/array.py
    draftobjects/base.py
    draftobjects/bezcurve.py
    draftobjects/block.py
    draftobjects/bspline.py
    draftobjects/circle.py
    draftobjects/clone.py
    draftobjects/ellipse.py
    draftobjects/facebinder.py
    draftobjects/draft_annotation.py
    draftobjects/fillet.py
    draftobjects/draftlink.py
    draftobjects/label.py
    draftobjects/layer.py
    draftobjects/dimension.py
    draftobjects/patharray.py
    draftobjects/pathtwistedarray.py
    draftobjects/point.py
    draftobjects/pointarray.py
    draftobjects/polygon.py
    draftobjects/rectangle.py
    draftobjects/shapestring.py
    draftobjects/shape2dview.py
    draftobjects/text.py
    draftobjects/wire.py
    draftobjects/wpproxy.py
    draftobjects/hatch.py
    draftobjects/README.md
)

SET(Draft_view_providers
    draftviewproviders/__init__.py
    draftviewproviders/view_array.py
    draftviewproviders/view_base.py
    draftviewproviders/view_bezcurve.py
    draftviewproviders/view_bspline.py
    draftviewproviders/view_circulararray.py
    draftviewproviders/view_clone.py
    draftviewproviders/view_facebinder.py
    draftviewproviders/view_orthoarray.py
    draftviewproviders/view_polararray.py
    draftviewproviders/view_draft_annotation.py
    draftviewproviders/view_fillet.py
    draftviewproviders/view_draftlink.py
    draftviewproviders/view_label.py
    draftviewproviders/view_layer.py
    draftviewproviders/view_dimension.py
    draftviewproviders/view_point.py
    draftviewproviders/view_rectangle.py
    draftviewproviders/view_text.py
    draftviewproviders/view_wire.py
    draftviewproviders/view_wpproxy.py
    draftviewproviders/view_hatch.py
    draftviewproviders/view_shapestring.py
    draftviewproviders/README.md
)

SET(Creator_tools
    draftguitools/gui_lines.py
    draftguitools/gui_fillets.py
    draftguitools/gui_splines.py
    draftguitools/gui_beziers.py
    draftguitools/gui_rectangles.py
    draftguitools/gui_arcs.py
    draftguitools/gui_circles.py
    draftguitools/gui_polygons.py
    draftguitools/gui_ellipses.py
    draftguitools/gui_texts.py
    draftguitools/gui_dimensions.py
    draftguitools/gui_shapestrings.py
    draftguitools/gui_points.py
    draftguitools/gui_facebinders.py
    draftguitools/gui_labels.py
    draftguitools/gui_hatch.py
)

SET(Modifier_tools
    draftguitools/gui_subelements.py
    draftguitools/gui_move.py
    draftguitools/gui_styles.py
    draftguitools/gui_rotate.py
    draftguitools/gui_offset.py
    draftguitools/gui_stretch.py
    draftguitools/gui_join.py
    draftguitools/gui_split.py
    draftguitools/gui_upgrade.py
    draftguitools/gui_downgrade.py
    draftguitools/gui_trimex.py
    draftguitools/gui_scale.py
    draftguitools/gui_wire2spline.py
    draftguitools/gui_shape2dview.py
    draftguitools/gui_draft2sketch.py
    draftguitools/gui_arrays.py
    draftguitools/gui_circulararray.py
    draftguitools/gui_orthoarray.py
    draftguitools/gui_patharray.py
    draftguitools/gui_pathtwistedarray.py
    draftguitools/gui_pointarray.py
    draftguitools/gui_polararray.py
    draftguitools/gui_clone.py
    draftguitools/gui_mirror.py
)

SET(Draft_GUI_tools
    draftguitools/__init__.py
    draftguitools/gui_annotationstyleeditor.py
    draftguitools/gui_base.py
    draftguitools/gui_base_original.py
    draftguitools/gui_tool_utils.py
    draftguitools/gui_planeproxy.py
    draftguitools/gui_selectplane.py
    draftguitools/gui_snaps.py
    draftguitools/gui_snapper.py
    draftguitools/gui_trackers.py
    draftguitools/gui_edit_base_object.py
    draftguitools/gui_edit_draft_objects.py
    draftguitools/gui_edit_arch_objects.py
    draftguitools/gui_edit_part_objects.py
    draftguitools/gui_edit_sketcher_objects.py
    draftguitools/gui_edit.py
    draftguitools/gui_togglemodes.py
    draftguitools/gui_groups.py
    draftguitools/gui_grid.py
    draftguitools/gui_heal.py
    draftguitools/gui_dimension_ops.py
    draftguitools/gui_lineslope.py
    draftguitools/gui_layers.py
    draftguitools/gui_setstyle.py
    ${Creator_tools}
    ${Modifier_tools}
    draftguitools/README.md
)

SET(Draft_task_panels
    drafttaskpanels/__init__.py
    drafttaskpanels/task_circulararray.py
    drafttaskpanels/task_orthoarray.py
    drafttaskpanels/task_polararray.py
    drafttaskpanels/task_scale.py
    drafttaskpanels/task_selectplane.py
    drafttaskpanels/task_shapestring.py
    drafttaskpanels/README.md
)

SET(Draft_SRCS_all
    ${Draft_SRCS_base}
    ${Draft_import}
    ${Draft_geoutils}
    ${Draft_tests}
    ${Draft_utilities}
    ${Draft_functions}
    ${Draft_make_functions}
    ${Draft_objects}
    ${Draft_view_providers}
    ${Draft_GUI_tools}
    ${Draft_task_panels}
)

# Cmake documentation: source_group defines a group into which sources
# will be placed in project files. This is intended to set up file tabs
# in Visual Studio.
#
# Maybe we don't need this anymore? For example, FEM doesn't use it.
SOURCE_GROUP("" FILES ${Draft_SRCS_all})

SET(DraftGuiIcon_SVG
    Resources/icons/DraftWorkbench.svg
)

ADD_CUSTOM_TARGET(Draft ALL
    SOURCES ${Draft_SRCS_all} ${Draft_QRC_SRCS} ${DraftGuiIcon_SVG}
)

fc_copy_sources(Draft "${CMAKE_BINARY_DIR}/Mod/Draft" ${Draft_SRCS_all})

fc_copy_sources(Draft "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Draft" ${DraftGuiIcon_SVG})

IF (BUILD_GUI)
    fc_target_copy_resource(Draft
        "${CMAKE_CURRENT_BINARY_DIR}"
        "${CMAKE_BINARY_DIR}/Mod/Draft"
        Draft_rc.py)
ENDIF (BUILD_GUI)

INSTALL(
    FILES
        ${Draft_SRCS_base}
        ${Draft_import}
        ${Draft_QRC_SRCS}
    DESTINATION
        Mod/Draft
)

INSTALL(FILES ${Draft_tests} DESTINATION Mod/Draft/drafttests)
INSTALL(FILES ${Draft_geoutils} DESTINATION Mod/Draft/draftgeoutils)
INSTALL(FILES ${Draft_utilities} DESTINATION Mod/Draft/draftutils)
INSTALL(FILES ${Draft_functions} DESTINATION Mod/Draft/draftfunctions)
INSTALL(FILES ${Draft_make_functions} DESTINATION Mod/Draft/draftmake)
INSTALL(FILES ${Draft_objects} DESTINATION Mod/Draft/draftobjects)
INSTALL(FILES ${Draft_view_providers} DESTINATION Mod/Draft/draftviewproviders)
INSTALL(FILES ${Draft_GUI_tools} DESTINATION Mod/Draft/draftguitools)
INSTALL(FILES ${Draft_task_panels} DESTINATION Mod/Draft/drafttaskpanels)

INSTALL(
    FILES
        ${DraftGuiIcon_SVG}
    DESTINATION
       "${CMAKE_INSTALL_DATADIR}/Mod/Draft/Resources/icons"
)
