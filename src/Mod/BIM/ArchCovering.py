# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Copyright (c) 2026 Furgo
#
# This file is part of the FreeCAD BIM workbench.
# You can find the full license text in the LICENSE file in the root directory.

"""The Covering object and tools.

This module provides tools to build Covering objects. Coverings are claddings, floorings,
wallpapers, etc. applied to other construction elements, but can also be independent. They support
solid 3D tiles, parametric 2D patterns, and hatch patterns.
"""

import FreeCAD
import ArchComponent
import Arch
import ArchTessellation
from draftutils import params


# Fallback stubs so the module loads without a GUI.
def translate(context, sourceText, disambiguation=None, n=-1):
    return sourceText


def QT_TRANSLATE_NOOP(context, sourceText):
    return sourceText


if FreeCAD.GuiUp:
    # Runtime override using native FreeCAD.Qt abstraction
    translate = FreeCAD.Qt.translate
    QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP


class _Covering(ArchComponent.Component):
    """
    A parametric object representing an architectural surface finish.

    This class manages the generation of surface treatments such as tiles, panels, flooring, or
    hatch patterns. Coverings are typically linked to a specific face of a base object.

    Parameters
    ----------
    obj : Part::FeaturePython
        The document object to be initialized as a Covering.

    """

    def __init__(self, obj):
        super().__init__(obj)

        # Override the parent's object type to set a specific one for Covering
        # Note: the `TypeId` property returns the C++ class identifier (e.g., "Part::FeaturePython"),
        # while the `Type` property is used to distinguish between different BIM objects that share
        # the same underlying C++ class implementation.
        self.Type = "Covering"

        self.setProperties(obj)

        # On first creation, seed the object's properties from the user's saved global preferences
        # (params). This is skipped on document restore, where properties already hold their saved
        # per-object values.
        obj.TileLength = params.get_param_arch("CoveringLength")
        obj.TileWidth = params.get_param_arch("CoveringWidth")
        obj.TileThickness = params.get_param_arch("CoveringThickness")
        obj.JointWidth = params.get_param_arch("CoveringJoint")
        obj.Rotation = params.get_param_arch("CoveringRotation")
        obj.TileAlignment = params.get_param_arch("CoveringAlignment")
        obj.FinishMode = params.get_param_arch("CoveringFinishMode")

    def setProperties(self, obj):
        """
        Declare and configure properties specific to the Covering object.

        Called twice in the object's lifetime: once on creation (via ``__init__``) and once each
        time the document is restored (via ``onDocumentRestored``). Both scenarios should be handled
        safely:

        - Properties are only added if they do not already exist, so saved values are never
          overwritten on document restore.
        - Defaults are applied immediately after adding a property, so they too only take effect
          on fresh creation.
        - Editor modes are re-applied unconditionally in both cases because FreeCAD does not save
          them to the document file.
        """
        # Override parent properties to ensure 'Base' is of type LinkSub (the parent defines it as
        # Link). Covering objects use sub-element links (LinkSub) because they need to target
        # specific subelements (faces).
        if (
            "Base" in obj.PropertiesList
            and obj.getTypeIdOfProperty("Base") != "App::PropertyLinkSub"
        ):
            obj.setPropertyStatus("Base", "-LockDynamic")
            obj.removeProperty("Base")
        if "Base" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyLinkSub",
                "Base",
                "Covering",
                QT_TRANSLATE_NOOP(
                    "App::Property", "The object or face this covering is applied to"
                ),
                locked=True,
            )

        # Apply the parent's property schema.
        super().setProperties(obj)

        # Snapshot once to avoid repeated C++ calls in the loop below; the cached list
        # is used to skip properties that already exist (e.g. on document restore).
        properties_list = obj.PropertiesList

        # Apply the local property schema
        # (Type, Name, Group, Tooltip, InitialValue)
        properties_schema = [
            (
                "App::PropertyEnumeration",
                "FinishMode",
                "Covering",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "How the finish is created and displayed:\n"
                    "- Solid Tiles: Physical 3D tiles with real gaps. Best for accurate detail and counting.\n"
                    "- Parametric Pattern: A grid of lines on a single slab. Faster to display than real tiles.\n"
                    "- Monolithic: A single smooth surface. Ideal for paint, plaster, or seamless flooring.\n"
                    "- Hatch Pattern: Technical drafting symbols (hatching) on a single slab.",
                ),
                ["Solid Tiles", "Parametric Pattern", "Monolithic", "Hatch Pattern"],
            ),
            (
                "App::PropertyEnumeration",
                "TileAlignment",
                "Covering",
                QT_TRANSLATE_NOOP("App::Property", "The alignment of the tile grid"),
                ["Center", "Top Left", "Top Right", "Bottom Left", "Bottom Right", "Custom"],
            ),
            (
                "App::PropertyAngle",
                "Rotation",
                "Covering",
                QT_TRANSLATE_NOOP("App::Property", "Rotation of the finish"),
                0,
            ),
            (
                "App::PropertyLength",
                "TileLength",
                "Tiles",
                QT_TRANSLATE_NOOP("App::Property", "The length of the tiles"),
                0,
            ),
            (
                "App::PropertyLength",
                "TileWidth",
                "Tiles",
                QT_TRANSLATE_NOOP("App::Property", "The width of the tiles"),
                0,
            ),
            (
                "App::PropertyLength",
                "TileThickness",
                "Tiles",
                QT_TRANSLATE_NOOP("App::Property", "The thickness of the tiles"),
                0,
            ),
            (
                "App::PropertyLength",
                "JointWidth",
                "Tiles",
                QT_TRANSLATE_NOOP("App::Property", "The width of the joints"),
                None,
            ),
            (
                "App::PropertyEnumeration",
                "StaggerType",
                "Tiles",
                QT_TRANSLATE_NOOP("App::Property", "The running bond logic"),
                [
                    "Stacked (None)",
                    "Half Bond (1/2)",
                    "Third Bond (1/3)",
                    "Quarter Bond (1/4)",
                    "Custom",
                ],
            ),
            (
                "App::PropertyLength",
                "StaggerCustom",
                "Tiles",
                QT_TRANSLATE_NOOP("App::Property", "Custom offset for running bond rows"),
                0.0,
            ),
            (
                "App::PropertyVector",
                "AlignmentOffset",
                "Tiles",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "A manual offset to shift the grid origin (X=U, Y=V). The Z component is ignored",
                ),
                None,
            ),
            (
                "App::PropertyLength",
                "BorderSetback",
                "Boundaries",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Distance to offset the covering inwards from the base boundary",
                ),
                0.0,
            ),
            (
                "App::PropertyArea",
                "NetArea",
                "Quantities",
                QT_TRANSLATE_NOOP("App::Property", "The surface area of the base face"),
                0,
            ),
            (
                "App::PropertyArea",
                "OuterArea",
                "Quantities",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "The area enclosed by the outer boundary of the base face, ignoring any holes",
                ),
                0,
            ),
            (
                "App::PropertyFloatList",
                "HoleAreas",
                "Quantities",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "The individual area of each hole in the base face, in mm², sorted by size (largest first)",
                ),
                None,
            ),
            (
                "App::PropertyArea",
                "GrossArea",
                "Quantities",
                QT_TRANSLATE_NOOP(
                    "App::Property", "Total area of material units consumed (Full + Partial)"
                ),
                0,
            ),
            (
                "App::PropertyArea",
                "WasteArea",
                "Quantities",
                QT_TRANSLATE_NOOP("App::Property", "The area of discarded material (Gross - Net)"),
                0,
            ),
            (
                "App::PropertyLength",
                "TotalJointLength",
                "Quantities",
                QT_TRANSLATE_NOOP("App::Property", "The total linear length of all joints"),
                0,
            ),
            (
                "App::PropertyLength",
                "OuterPerimeter",
                "Quantities",
                QT_TRANSLATE_NOOP("App::Property", "The length of the substrate outer perimeter"),
                0,
            ),
            (
                "App::PropertyInteger",
                "CountFullTiles",
                "Quantities",
                QT_TRANSLATE_NOOP("App::Property", "The number of full tiles"),
                0,
            ),
            (
                "App::PropertyInteger",
                "CountPartialTiles",
                "Quantities",
                QT_TRANSLATE_NOOP("App::Property", "The number of cut/partial tiles"),
                0,
            ),
            (
                "App::PropertyFileIncluded",
                "TextureImage",
                "Visual",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "An image file to map onto each tile. The file is embedded in the document.",
                ),
                None,
            ),
            (
                "App::PropertyVector",
                "TextureScale",
                "Visual",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Scaling of the texture on each tile (X=U, Y=V). The Z component is ignored.",
                ),
                FreeCAD.Vector(1, 1, 0),
            ),
            (
                "App::PropertyFileIncluded",
                "PatternFile",
                "Pattern",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "The PAT file to use for hatching. The file is embedded in the document.",
                ),
                None,
            ),
            (
                "App::PropertyString",
                "PatternName",
                "Pattern",
                QT_TRANSLATE_NOOP("App::Property", "The name of the pattern in the PAT file"),
                "",
            ),
            (
                "App::PropertyFloat",
                "PatternScale",
                "Pattern",
                QT_TRANSLATE_NOOP("App::Property", "The scale of the hatch pattern"),
                1.0,
            ),
            (
                "App::PropertyVector",
                "ReferenceDirection",
                "Covering",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Stored U-axis direction that keeps the tiling basis stable across recomputes",
                ),
                FreeCAD.Vector(0, 0, 0),
            ),
            (
                "App::PropertyEnumeration",
                "PredefinedType",
                "IFC Attributes",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "The specific IFC subtype of this covering. Exported as IfcCovering.PredefinedType.",
                ),
                [
                    "FLOORING",
                    "CLADDING",
                    "ROOFING",
                    "MOLDING",
                    "SKIRTINGBOARD",
                    "CEILING",
                    "WRAPPING",
                    "NOTDEFINED",
                ],
            ),
        ]

        for prop_type, name, group, tooltip, default in properties_schema:
            if name not in properties_list:
                obj.addProperty(prop_type, name, group, tooltip, locked=True)

                # Apply defined default values
                if default is not None:
                    setattr(obj, name, default)

        # Mark computed quantity properties as ReadOnly (they are set during recompute, not by the
        # user). Editor modes are not persisted in the document, so this must run on both creation
        # and restore.
        obj.setEditorMode("NetArea", ["ReadOnly"])
        obj.setEditorMode("OuterArea", ["ReadOnly"])
        obj.setEditorMode("HoleAreas", ["ReadOnly"])
        obj.setEditorMode("GrossArea", ["ReadOnly"])
        obj.setEditorMode("WasteArea", ["ReadOnly"])
        obj.setEditorMode("TotalJointLength", ["ReadOnly"])
        obj.setEditorMode("OuterPerimeter", ["ReadOnly"])
        obj.setEditorMode("CountFullTiles", ["ReadOnly"])
        obj.setEditorMode("CountPartialTiles", ["ReadOnly"])
        obj.setEditorMode("ReferenceDirection", ["Hidden"])

    def loads(self, state):
        """
        Called when loading a saved document to reconstruct this proxy object. Sets ``Type``
        explicitly because it is a Python instance attribute, not a document property, so it is
        not saved to the file and must be restored manually.
        """
        self.Type = "Covering"

    def onDocumentRestored(self, obj):
        """
        Called after the document is restored. Re-applies the property schema and removes any
        orphaned CoveringTemplate objects left behind by a crash.

        The removal is deferred via `ToDo.delay` (a zero-millisecond QTimer) because
        `doc.removeObject()` is not safe to call directly from `onDocumentRestored`: FreeCAD calls
        this method while still iterating over the document's object list; removing an object here
        invalidates that iteration and causes a segfault.
        """
        super().onDocumentRestored(obj)
        self.setProperties(obj)

        if not FreeCAD.GuiUp:
            return  # No Qt event loop, no ToDo, and no GUI task panel that could create orphans.

        doc = obj.Document
        orphans = [
            doc_obj.Name
            for doc_obj in doc.Objects
            if doc_obj.Name.startswith("CoveringTemplate")
            and getattr(getattr(doc_obj, "Proxy", None), "Type", None) == "Covering"
        ]

        if orphans:
            from draftutils import todo

            def _purge(doc_name):
                doc = FreeCAD.getDocument(doc_name)
                if doc is None:
                    return
                # Disable undo recording so the deletion does not appear in the undo stack.
                # Orphan cleanup is not a user action and should not be undoable.
                old_undo_mode = doc.UndoMode
                doc.UndoMode = 0  # 0 = disabled
                try:
                    for name in orphans:
                        if doc.getObject(name):
                            FreeCAD.Console.PrintMessage(
                                f"ArchCovering: removing orphaned template '{name}'\n"
                            )
                            doc.removeObject(name)
                finally:
                    doc.UndoMode = old_undo_mode

            todo.ToDo.delay(_purge, doc.Name)

    def getExtrusionData(self, obj):
        """
        Returns None to suppress the parametric extrusion path during IFC export.

        - The base implementation calls ``obj.Base.isDerivedFrom()``, which raises
          ``AttributeError`` because Covering's ``Base`` is a ``LinkSub`` (a tuple, not a Link).
        - Covering geometry is never a simple extrusion. Even Monolithic mode produces a placed
          solid.

        Returning None sends the exporter down the generic brep path, which handles all finish
        modes correctly.
        """
        return None

    def execute(self, obj):
        """
        Builds the finish geometry from the selected base face. The face is transformed into a
        stable local UV coordinate system, optionally inset by ``BorderSetback``, and the tile grid
        is anchored according to ``TileAlignment``. The active finish mode then determines which
        tessellator runs. Shape, placement, and BIM quantities (tile counts, areas, joint length)
        are written back to the object's properties.
        """
        import Part

        if self.clone(obj):
            # Skip recompute if this object is a clone; the geometry will be generated by the source
            return

        frame = build_tiling_frame(obj, persist_reference=True)
        if frame is None:
            FreeCAD.Console.PrintWarning(
                f"ArchCovering [{obj.Label}]: no base face found, skipping recompute.\n"
            )
            return
        tiling_face, u_vec, v_vec, normal, tile_grid_origin = frame

        if obj.FinishMode == "Hatch Pattern":
            config = ArchTessellation.HatchConfig(
                filename=obj.PatternFile,
                pattern_name=obj.PatternName,
                scale=obj.PatternScale,
                thickness=obj.TileThickness.Value,
            )
        else:
            config = ArchTessellation.TileConfig(
                finish_mode=obj.FinishMode,
                length=obj.TileLength.Value,
                width=obj.TileWidth.Value,
                thickness=obj.TileThickness.Value,
                joint=obj.JointWidth.Value,
                stagger_type=obj.StaggerType,
                stagger_custom=obj.StaggerCustom.Value,
            )

        tessellator = ArchTessellation.create_tessellator(config)
        tessellation_result = tessellator.compute(
            tiling_face,
            tile_grid_origin,
            u_vec,
            normal,
        )

        # Report any warnings from the tessellator to the console.
        #
        # INVALID_DIMENSIONS aborts early, as no usable geometry was produced. All other non-OK
        # statuses are warnings: they report the issue but allow execution to continue so that
        # quantities are still written to the object.
        match tessellation_result.status:
            case ArchTessellation.TessellationStatus.INVALID_DIMENSIONS:
                FreeCAD.Console.PrintWarning(
                    translate("Arch", "The specified tile size is too small to be modeled.") + "\n"
                )
                obj.Shape = Part.Shape()
                return
            case ArchTessellation.TessellationStatus.JOINT_TOO_SMALL:
                FreeCAD.Console.PrintWarning(
                    translate("Arch", "The joint width is too small to model individual units.")
                    + "\n"
                )
            case ArchTessellation.TessellationStatus.COUNT_TOO_HIGH:
                FreeCAD.Console.PrintWarning(
                    translate(
                        "Arch",
                        "The number of tiles is too high for individual units to be modeled.",
                    )
                    + "\n"
                )
            case ArchTessellation.TessellationStatus.EXTREME_COUNT:
                FreeCAD.Console.PrintWarning(
                    translate(
                        "Arch",
                        "The number of tiles is extremely high. Layout lines are hidden.",
                    )
                    + "\n"
                )
            case _:
                pass

        if tessellation_result.geometry:
            # The tessellator produces geometry in a local frame (origin at the grid anchor,
            # axes aligned to the face). Transform it to world space so the shape encodes the
            # face position.
            pl_matrix = tessellation_result.placement.toMatrix()
            obj.Shape = tessellation_result.geometry.transformGeometry(pl_matrix)
        else:
            obj.Shape = Part.Shape()

        # Write tessellator output to the quantity properties.
        obj.CountFullTiles = tessellation_result.quantities.count_full
        obj.CountPartialTiles = tessellation_result.quantities.count_partial
        obj.NetArea = tessellation_result.quantities.area_net
        obj.OuterArea = tessellation_result.quantities.area_outer
        obj.HoleAreas = tessellation_result.quantities.areas_holes
        obj.GrossArea = tessellation_result.quantities.area_gross
        obj.WasteArea = tessellation_result.quantities.waste_area
        obj.TotalJointLength = tessellation_result.quantities.length_joints
        obj.OuterPerimeter = tessellation_result.quantities.length_perimeter


def get_reference_direction(covering):
    """Returns the covering's ReferenceDirection in world coordinates, or None if not yet set.

    Because ReferenceDirection is stored relative to the base object, the U-axis rotates along with
    the base geometry. This helper takes that local direction and applies the base objects's
    rotation to produce the world-space vector expected by ``getFaceFrame``.
    """
    import Part

    stored = covering.ReferenceDirection
    if stored.Length <= Part.Precision.confusion():
        return None
    base = covering.Base[0] if isinstance(covering.Base, tuple) else covering.Base
    return base.Placement.Rotation.multVec(stored) if base else stored


def set_reference_direction(covering, world_u):
    """Store a world U direction into ``covering.ReferenceDirection`` as base-local coordinates.

    The inverse of ``get_reference_direction``: applies the inverted base rotation so the
    persisted value rotates with the base on future recomputes.
    """
    base = covering.Base[0] if isinstance(covering.Base, tuple) else covering.Base
    covering.ReferenceDirection = (
        base.Placement.Rotation.inverted().multVec(world_u) if base else world_u
    )


def build_tiling_frame(obj, persist_reference=False):
    """Build the tiling face and its coordinate frame for a covering.

    Resolves the base face, applies the border setback, builds a U/V/normal frame from
    ``obj.ReferenceDirection``, and computes the tile grid origin. Hatch patterns have no
    individual tiles, so the grid origin is computed with zero tile dimensions; every
    ``TileAlignment`` preset then produces the same face-corner origin.

    Parameters
    ----------
    obj : App::FeaturePython
        The covering object.
    persist_reference : bool, optional
        If True, the computed U vector is saved to ``obj.ReferenceDirection`` to lock the
        orientation for future recomputes. Only ``execute`` should pass True: View providers must
        use False to ensure that visual operations (e.g. texture mapping) do not modify the
        object's properties.

    Returns
    -------
    tuple or None
        ``(tiling_face, u_vec, v_vec, normal, grid_origin)`` in world space, or ``None`` if the
        base face cannot be resolved.
    """
    base_face = Arch.resolveFace(obj.Base)
    if not base_face:
        return None

    reference_direction = get_reference_direction(obj)
    u_vec, v_vec, normal, center = Arch.getFaceFrame(base_face, reference_direction)
    if persist_reference and reference_direction is None:
        set_reference_direction(obj, u_vec)

    # Apply the covering's in-plane Rotation to the frame so the grid anchor, the tessellator,
    # and the texture mapping all iterate in the same oriented frame.
    rotation = FreeCAD.Rotation(normal, obj.Rotation.Value)
    u_vec = rotation.multVec(u_vec)
    v_vec = rotation.multVec(v_vec)

    tiling_face = apply_setback(base_face, obj.BorderSetback.Value)

    is_hatch = obj.FinishMode == "Hatch Pattern"
    tile_length = 0.0 if is_hatch else obj.TileLength.Value
    tile_width = 0.0 if is_hatch else obj.TileWidth.Value
    grid_origin = get_tile_grid_origin(
        tiling_face,
        center,
        u_vec,
        v_vec,
        obj.TileAlignment,
        tile_length,
        tile_width,
        obj.AlignmentOffset,
    )

    return tiling_face, u_vec, v_vec, normal, grid_origin


def apply_setback(base_face, border_setback):
    """
    Returns the reduced face after applying a setback inset to the outer perimeter.

    Parameters
    ----------
    base_face : Part.Face
        The original face to inset.
    border_setback : float
        Inset distance in document units. Values <= 0 return ``base_face`` unchanged.

    Returns
    -------
    Part.Face
        The inset face, or ``base_face`` unchanged if ``border_setback`` is zero or the inset
        collapses the face.

    Notes
    -----
    Only the outer perimeter is shrunk. Inner wires (holes such as drain cutouts or column
    penetrations) are left at their original size. The setback defines a finishing margin at the
    room boundary (e.g. keeping tiles away from skirting boards), not a clearance around obstacles.

    """
    import Part

    if border_setback <= 0:
        return base_face

    # Copy before modifying so the passed face is not modified.
    tiling_face = base_face.copy()

    # Shrink the outer boundary. makeOffset2D raises when the setback is too large to leave any
    # area; the null/zero-area check below catches any remaining degenerate cases.
    _setback_too_large = translate(
        "Arch", "BorderSetback is too large and collapses the face. Setback ignored."
    )
    try:
        shrunk_outer_face = Part.Face(tiling_face.OuterWire).makeOffset2D(-border_setback)
    except Exception:
        FreeCAD.Console.PrintWarning(_setback_too_large + "\n")
        return base_face

    # The setback is larger than the face; fall back to the original.
    if shrunk_outer_face.isNull() or shrunk_outer_face.Area <= 0:
        FreeCAD.Console.PrintWarning(_setback_too_large + "\n")
        return base_face

    # Extract inner wires (holes)
    outer_hash = tiling_face.OuterWire.hashCode()
    inner_wires = [wire for wire in tiling_face.Wires if wire.hashCode() != outer_hash]

    if not inner_wires:
        return shrunk_outer_face

    # Re-cut each original hole into the shrunk outer face. makeOffset2D only operates on the outer
    # wire and discards holes, so they must be restored manually. Part.makeFace() cannot be used
    # here: it crashes when the shrunken boundary crosses a hole.
    shrunk_face = shrunk_outer_face
    for wire in inner_wires:
        try:
            hole_face = Part.Face(wire)
            if not hole_face.isNull():
                # The boolean cut handles cases where the hole intersects the perimeter or falls
                # completely outside it.
                cut_result = shrunk_face.cut(hole_face)

                if cut_result.Faces:
                    shrunk_face = cut_result.Faces[0]
                else:
                    # This specific hole consumed the whole face. Warn and keep the face from the
                    # previous iteration.
                    FreeCAD.Console.PrintWarning(
                        translate(
                            "Arch",
                            "A hole is larger than the shrunken area. Skipping this hole.",
                        )
                        + "\n"
                    )
        except Exception as e:
            FreeCAD.Console.PrintMessage(
                f"ArchCovering: skipping problematic hole during setback: {e}\n"
            )

    # Only apply if the boolean cuts produced a valid result; otherwise keep the shrunk outer face.
    if not shrunk_face.isNull() and shrunk_face.Area > 0:
        return shrunk_face

    return shrunk_outer_face


def get_tile_grid_origin(
    face, center_point, u_vec, v_vec, alignment, tile_length, tile_width, offset=None
):
    """
    Calculates the tiling grid origin so that a tile body edge coincides with the named corner of
    the face.

    Used by both the geometry builder (``_Covering.execute``) and the texture mapper
    (``_ViewProviderCovering._compute_texture_mapping``). This ensures tile geometry and texture
    coordinates are always anchored at the same point.

    Parameters
    ----------
    face : Part.Face
        The face to tile, after any border setback has been applied.
    center_point : FreeCAD.Vector
        Face center in world space, as returned by ``getFaceFrame``.
    u_vec, v_vec : FreeCAD.Vector
        Unit vectors of the face's local basis, as returned by ``getFaceFrame``.
    alignment : str
        One of "Center", "Bottom Left", "Bottom Right", "Top Left", "Top Right", "Custom".
    tile_length : float
        Tile body length in mm (``TileLength.Value``). Unused in Custom mode.
    tile_width : float
        Tile body width in mm (``TileWidth.Value``). Unused in Custom mode.
    offset : FreeCAD.Vector or None
        In preset modes: additional UV shift applied after the alignment corner is selected. In
        Custom mode: absolute UV position relative to the face center.

    Notes
    -----
    All presets are calculated based on the face's local U and V axes, as returned by
    ``getFaceFrame``. This keeps the anchor pinned to a specific face corner, regardless of how the
    base geometry is rotated. Rotating the base or the covering does not shift the tile pattern.

    Corner labels are thus face-relative, representing the minimum and maximum coordinates along the
    face's local U and V axes. While these local axes happen to align with global directions for
    standard, unrotated bases, the labels will always follow the rotation of the face.

    For corner presets alignments, the grid is anchored so that the outer edges of the starting tile
    sit flush against the chosen corner of the face. Any grout gaps will extend outward beyond the
    face's edge. For the "Center" preset, the grid is anchored by placing the center of a single
    tile on the center point of the face.
    """
    # In Custom mode, offset is an absolute position from the face center, not a shift.
    if alignment == "Custom":
        if offset is None:
            return center_point
        return center_point + u_vec * offset.x + v_vec * offset.y

    # Project each vertex onto the UV axes (relative to the face center).
    u_coords = [vertex.Point.sub(center_point).dot(u_vec) for vertex in face.Vertexes]
    v_coords = [vertex.Point.sub(center_point).dot(v_vec) for vertex in face.Vertexes]
    min_u, max_u = min(u_coords), max(u_coords)
    min_v, max_v = min(v_coords), max(v_coords)

    if alignment == "Center":
        mid_u = (min_u + max_u) / 2
        mid_v = (min_v + max_v) / 2
        u_target = mid_u - tile_length / 2.0
        v_target = mid_v - tile_width / 2.0
    else:
        # Corner presets: align the first tile so its origin corner is flush with the face's UV
        # boundaries in both directions.
        # - "Left" or "Bottom": the grid's origin is at the face's minimum U or V boundary.
        # - "Right" or "Top": the origin is shifted back by one tile dimension so the tile sits
        #   inside the face's maximum U or V boundary.
        u_target = min_u if "Left" in alignment else max_u - tile_length
        v_target = min_v if "Bottom" in alignment else max_v - tile_width

    origin = center_point + u_vec * u_target + v_vec * v_target

    if offset:
        origin = origin.add(u_vec * offset.x + v_vec * offset.y)

    return origin
