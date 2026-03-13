# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Copyright (c) 2026 Furgo
#
# This file is part of the FreeCAD Arch workbench.
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

MIN_DIMENSION = 0.1


# Translation shims for headless and static analysis
def translate(context, sourceText, disambiguation=None, n=-1):
    return sourceText


def QT_TRANSLATE_NOOP(context, sourceText):
    return sourceText


if FreeCAD.GuiUp:
    # Runtime override using native FreeCAD.Qt abstraction
    translate = FreeCAD.Qt.translate
    QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP


def getFaceGridOrigin(face, center_point, u_vec, v_vec, alignment="Center", offset=None):
    """
    Calculates the raw bounding-box vertex of a face that corresponds to the
    requested alignment preset, optionally shifted by an additional offset.

    This is the low-level building block used by getAlignedGridOrigin.
    Also useful for tests or callers that need the raw bounding-box vertex.  It
    returns the point where the bottom-left (0,0) tile corner would land if
    the grid were anchored without any tile-body correction.

    Parameters
    ----------
    face : Part.Face
    center_point : FreeCAD.Vector
        Face centre in global space (as returned by getFaceUV).
    u_vec, v_vec : FreeCAD.Vector
        Unit vectors of the face's local coordinate system.
    alignment : str
        One of "Center", "BottomLeft", "BottomRight", "TopLeft", "TopRight".
    offset : FreeCAD.Vector or None
        Additional UV offset applied after the alignment vertex is selected
        (corresponds to AlignmentOffset on the covering object).
    """
    v0 = face.Vertexes[0].Point
    vec0 = v0.sub(center_point)

    min_u = max_u = vec0.dot(u_vec)
    min_v = max_v = vec0.dot(v_vec)

    for v in face.Vertexes[1:]:
        vec_to_vert = v.Point.sub(center_point)
        proj_u = vec_to_vert.dot(u_vec)
        proj_v = vec_to_vert.dot(v_vec)
        if proj_u < min_u:
            min_u = proj_u
        if proj_u > max_u:
            max_u = proj_u
        if proj_v < min_v:
            min_v = proj_v
        if proj_v > max_v:
            max_v = proj_v

    origin_offset = FreeCAD.Vector(0, 0, 0)
    if alignment == "Center":
        mid_u = (min_u + max_u) / 2
        mid_v = (min_v + max_v) / 2
        origin_offset = (u_vec * mid_u) + (v_vec * mid_v)
    elif alignment == "BottomLeft":
        origin_offset = (u_vec * min_u) + (v_vec * min_v)
    elif alignment == "BottomRight":
        origin_offset = (u_vec * max_u) + (v_vec * min_v)
    elif alignment == "TopLeft":
        origin_offset = (u_vec * min_u) + (v_vec * max_v)
    elif alignment == "TopRight":
        origin_offset = (u_vec * max_u) + (v_vec * max_v)

    if offset:
        align_shift = (u_vec * offset.x) + (v_vec * offset.y)
        origin_offset = origin_offset.add(align_shift)

    return center_point.add(origin_offset)


def getAlignedGridOrigin(
    face, center_point, u_vec, v_vec, alignment, tile_length, tile_width, offset=None
):
    """
    Calculates the tiling grid origin so that the *named* corner of a tile
    coincides with the *named* corner of the face bounding box.

    This is the single authoritative implementation of the alignment-preset
    semantics shared by the geometry engine (_Covering.execute via
    _calculate_origin) and the texture mapper (_ViewProviderCovering via
    _compute_texture_mapping).  Both call sites must use this function so
    that tile geometry and texture always share the same origin.

    How it works
    ------------
    getFaceGridOrigin returns the raw bounding-box vertex, which is where
    the bottom-left (0,0) tile corner lands by default.  For presets other
    than BottomLeft we shift the grid so the *correct* tile corner arrives
    at the face corner instead:

      BottomLeft : no shift — (0,0) corner already at min_u, min_v.
      BottomRight: shift left  by tile_length so the right  edge reaches max_u.
      TopLeft    : shift down  by tile_width  so the top    edge reaches max_v.
      TopRight   : both shifts.
      Center     : shift by half a tile so a tile centre lands at the face centre.

    The shift uses tile body size (not pitch = size + joint) so the tile
    corner is flush with the face edge and the joint bleeds outside — the
    physically correct behaviour at a wall or kerb edge.

    Parameters
    ----------
    face : Part.Face
    center_point : FreeCAD.Vector
    u_vec, v_vec : FreeCAD.Vector
    alignment : str
        One of "Center", "BottomLeft", "BottomRight", "TopLeft", "TopRight".
    tile_length : float
        Tile body length in mm (TileLength.Value).
    tile_width : float
        Tile body width in mm (TileWidth.Value).
    offset : FreeCAD.Vector or None
        Additional UV offset (AlignmentOffset) passed through to
        getFaceGridOrigin.
    """
    origin = getFaceGridOrigin(face, center_point, u_vec, v_vec, alignment, offset)

    if alignment == "Center":
        origin = origin - u_vec * (tile_length / 2.0)
        origin = origin - v_vec * (tile_width / 2.0)
    elif alignment == "BottomRight":
        origin = origin - u_vec * tile_length
    elif alignment == "TopLeft":
        origin = origin - v_vec * tile_width
    elif alignment == "TopRight":
        origin = origin - u_vec * tile_length
        origin = origin - v_vec * tile_width

    return origin


class _Covering(ArchComponent.Component):
    """
    A parametric object representing an architectural surface finish.

    This class manages the generation of surface treatments such as tiles, panels, flooring, or
    hatch patterns. Coverings are typically linked to a specific face of a base object and remain
    parametric, updating automatically if the underlying geometry changes.

    Parameters
    ----------
    obj : App::FeaturePython
        The base C++ object to be initialized as a Covering.

    Notes
    -----
    While the standard FreeCAD `TypeId` attribute identifies the underlying C++ class, the `Type`
    attribute is used by the Arch module to distinguish functional object types that share the same
    C++ implementation.

    Examples
    --------
    >>> print(obj.TypeId, "->", obj.Proxy.Type)
    Part::FeaturePython -> Covering
    >>> import Draft; Draft.get_type(obj)
    'Covering'
    """

    def __init__(self, obj):
        super().__init__(obj)

        # Override the parent's object type to set a specific one for Covering
        self.Type = "Covering"

        # Apply the property schema
        self.setProperties(obj)

        # Initialize properties with user preferences (params) upon object creation (but not on
        # document restore).
        # The properties mapped to parameters here are generally the ones that are editable in the
        # object's Task Panel. Since the UI binds to these properties, the Task Panel will
        # automatically display these values as the defaults without extra UI logic.
        obj.TileLength = params.get_param_arch("CoveringLength")
        obj.TileWidth = params.get_param_arch("CoveringWidth")
        obj.TileThickness = params.get_param_arch("CoveringThickness")
        obj.JointWidth = params.get_param_arch("CoveringJoint")
        obj.Rotation = params.get_param_arch("CoveringRotation")
        obj.TileAlignment = params.get_param_arch("CoveringAlignment")
        obj.FinishMode = params.get_param_arch("CoveringFinishMode")

    def setProperties(self, obj):
        """
        Overrides the parent method to define properties specific to the Covering object, including
        tiling and pattern schema, and ensures 'Base' supports sub-element face targeting.
        """
        # Override parent properties to ensure 'Base' is LinkSub (parent defines it as Link).
        # Covering objects use sub-element links (LinkSub) because they need to target specific
        # faces.
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

        # Snapshot properties once
        properties_list = obj.PropertiesList

        # Apply the local property schema
        # (Type, Name, Group, Tooltip, InitialValue)
        properties_schema = [
            (
                "App::PropertyEnumeration",
                "FinishMode",
                "Covering",
                "How the finish is created and displayed:\n"
                "- Solid Tiles: Physical 3D tiles with real gaps. Best for accurate detail and counting.\n"
                "- Parametric Pattern: A grid of lines on a single slab. Faster to display than real tiles.\n"
                "- Monolithic: A single smooth surface. Ideal for paint, plaster, or seamless flooring.\n"
                "- Hatch Pattern: Technical drafting symbols (hatching) on a single slab.",
                ["Solid Tiles", "Parametric Pattern", "Monolithic", "Hatch Pattern"],
            ),
            (
                "App::PropertyEnumeration",
                "TileAlignment",
                "Covering",
                "The alignment of the tile grid",
                ["Center", "TopLeft", "TopRight", "BottomLeft", "BottomRight", "Custom"],
            ),
            ("App::PropertyAngle", "Rotation", "Covering", "Rotation of the finish", 0),
            ("App::PropertyLength", "TileLength", "Tiles", "The length of the tiles", 0),
            ("App::PropertyLength", "TileWidth", "Tiles", "The width of the tiles", 0),
            ("App::PropertyLength", "TileThickness", "Tiles", "The thickness of the tiles", 0),
            ("App::PropertyLength", "JointWidth", "Tiles", "The width of the joints", None),
            (
                "App::PropertyEnumeration",
                "StaggerType",
                "Tiles",
                "The running bond logic",
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
                "Custom offset for running bond rows",
                0.0,
            ),
            (
                "App::PropertyVector",
                "AlignmentOffset",
                "Tiles",
                "A manual offset to shift the grid origin (X=U, Y=V). The Z component is ignored",
                None,
            ),
            (
                "App::PropertyLength",
                "BorderSetback",
                "Boundaries",
                "Distance to offset the covering inwards from the base boundary",
                0.0,
            ),
            ("App::PropertyArea", "NetArea", "Quantities", "The surface area of the base face", 0),
            (
                "App::PropertyArea",
                "GrossArea",
                "Quantities",
                "Total area of material units consumed (Full + Partial)",
                0,
            ),
            (
                "App::PropertyArea",
                "WasteArea",
                "Quantities",
                "The area of discarded material (Gross - Net)",
                0,
            ),
            (
                "App::PropertyLength",
                "TotalJointLength",
                "Quantities",
                "The total linear length of all joints",
                0,
            ),
            (
                "App::PropertyLength",
                "PerimeterLength",
                "Quantities",
                "The length of the substrate perimeter",
                0,
            ),
            ("App::PropertyInteger", "CountFullTiles", "Quantities", "The number of full tiles", 0),
            (
                "App::PropertyInteger",
                "CountPartialTiles",
                "Quantities",
                "The number of cut/partial tiles",
                0,
            ),
            (
                "App::PropertyFileIncluded",
                "TextureImage",
                "Visual",
                "An image file to map onto each tile. The file is embedded in the document.",
                None,
            ),
            (
                "App::PropertyVector",
                "TextureScale",
                "Visual",
                "Scaling of the texture on each tile (X=U, Y=V). The Z component is ignored.",
                None,
            ),
            (
                "App::PropertyFileIncluded",
                "PatternFile",
                "Pattern",
                "The PAT file to use for hatching. The file is embedded in the document.",
                None,
            ),
            (
                "App::PropertyString",
                "PatternName",
                "Pattern",
                "The name of the pattern in the PAT file",
                "",
            ),
            (
                "App::PropertyFloat",
                "PatternScale",
                "Pattern",
                "The scale of the hatch pattern",
                1.0,
            ),
            (
                "App::PropertyEnumeration",
                "PredefinedType",
                "IFC Attributes",
                "The specific IFC subtype of this covering. Exported as IfcCovering.PredefinedType.",
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
                obj.addProperty(
                    prop_type, name, group, QT_TRANSLATE_NOOP("App::Property", tooltip), locked=True
                )

                # Apply defined default values
                if default is not None:
                    setattr(obj, name, default)

        # TextureScale default cannot be expressed as a plain value in the schema tuple,
        # so it is set here after the loop.
        if "TextureScale" in obj.PropertiesList and obj.TextureScale.Length == 0:
            obj.TextureScale = FreeCAD.Vector(1, 1, 0)

        # Property status configuration (Read-Only fields)
        obj.setEditorMode("NetArea", 1)
        obj.setEditorMode("GrossArea", 1)
        obj.setEditorMode("WasteArea", 1)
        obj.setEditorMode("TotalJointLength", 1)
        obj.setEditorMode("PerimeterLength", 1)
        obj.setEditorMode("CountFullTiles", 1)
        obj.setEditorMode("CountPartialTiles", 1)

    def loads(self, state):
        """
        Overrides the parent callback used by FreeCAD's persistence engine to restore the
        Python proxy instance and reset non-persistent internal attributes like 'Type'.
        """
        self.Type = "Covering"

    def onDocumentRestored(self, obj):
        """
        Overrides the parent callback triggered after the document is fully restored.
        Ensures property schema consistency and schedules a purge of any orphaned
        CoveringTemplate objects left behind by a crash.

        The purge is deferred via ToDo.delay (a zero-millisecond QTimer) because
        doc.removeObject() is not safe to call from within onDocumentRestored: the C++
        Document::afterRestore() holds a raw-pointer vector of all objects that it continues
        to iterate after the Python callbacks return. Deleting an object during the callback
        leaves a dangling pointer in that vector and causes a segfault.
        """
        super().onDocumentRestored(obj)
        self.setProperties(obj)

        if not FreeCAD.GuiUp:
            return  # No Qt event loop, no ToDo, and no GUI task panel that could create orphans.

        doc = obj.Document
        orphans = [
            o.Name
            for o in doc.Objects
            if o.Label.startswith("CoveringTemplate")
            and getattr(getattr(o, "Proxy", None), "Type", None) == "Covering"
        ]
        if orphans:
            from draftutils import todo

            def _purge(doc_name):
                doc = FreeCAD.getDocument(doc_name)
                if doc is None:
                    return
                old_undo_mode = doc.UndoMode
                doc.UndoMode = 0
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
        Overrides ArchComponent.getExtrusionData to suppress the parametric
        extrusion path during IFC export.

        The base implementation assumes obj.Base is a plain Link and calls
        obj.Base.isDerivedFrom(), which raises AttributeError because Covering
        uses a LinkSub (a tuple). More importantly, Covering geometry is never
        a simple extrusion of a single profile — even Monolithic and Parametric
        Pattern modes produce a placed solid rather than a canonical extrusion.
        Returning None forces the exporter to use the generic brep path, which
        correctly serialises the computed Shape for all finish modes.
        """
        return None

    def onChanged(self, obj, prop):
        """Method called when a property is changed."""
        ArchComponent.Component.onChanged(self, obj, prop)

    def _get_layout_frame(self, face):
        """
        Returns a stable, right-handed orthonormal basis for the face.

        The stabilisation of the raw surface tangents (dominant-axis alignment,
        sign normalisation, re-orthonormalisation) is handled inside
        ``Arch.getFaceUV``.  The only caller-side adjustment needed here is the
        orientation flip: ``face.Orientation == "Reversed"`` is a topological
        property of how this face sits within its parent solid, not a property
        of the surface geometry, so it must be handled here rather than inside
        the geometry utility.
        """
        u_vec, v_vec, normal, center = Arch.getFaceUV(face)

        # If the face is reversed (common in solid sub-elements), negate the
        # normal so the basis points outward from the material surface.
        if face.Orientation == "Reversed":
            normal = -normal
            v_vec = normal.cross(u_vec).normalize()
            u_vec = v_vec.cross(normal).normalize()

        return u_vec, v_vec, normal, center

    def _apply_boundaries(self, base_face, obj):
        """
        Returns the effective tiling face after applying the BorderSetback offset.

        Design intent
        -------------
        BorderSetback shrinks the **outer perimeter** of the face inward by the
        specified distance.  Inner wires (holes such as drain cutouts or column
        penetrations) are intentionally left at their original size and position.
        The setback models a finishing margin at the room boundary — for example,
        keeping tiles away from skirting boards — not a clearance around all
        obstacles.  Expanding holes by the setback distance would be a separate
        feature, not a natural extension of this property.

        Complexity note
        ---------------
        A simple ``Part.makeFace()`` cannot be used here because it crashes when
        the shrunken outer boundary crosses an existing inner hole.  Instead the
        function shrinks the outer wire, then re-cuts each original inner hole
        via boolean subtraction.
        """
        import Part

        effective_face = base_face.copy()
        border_setback = obj.BorderSetback.Value

        if border_setback > 0:
            # Shrink the outer boundary
            shrunk_outer_face = Part.Face(effective_face.OuterWire).makeOffset2D(-border_setback)

            if shrunk_outer_face.isNull() or shrunk_outer_face.Area <= 0:
                return effective_face

            # Extract inner wires (holes)
            outer_hash = effective_face.OuterWire.hashCode()
            inner_wires = [w for w in effective_face.Wires if w.hashCode() != outer_hash]

            if not inner_wires:
                return shrunk_outer_face

            # Boolean cut
            shrunk_face = shrunk_outer_face
            for w in inner_wires:
                # Force the wire into a valid, forward-facing Part.Face Passing it as a list to
                # Part.Face() triggers FaceMakerSimple, which ensures the right
                # clockwise/counter-clockwise orientation.
                try:
                    hole_face = Part.Face([w])
                    if not hole_face.isNull():
                        # The boolean cut handles cases where the hole intersects the perimeter or
                        # falls completely outside it.
                        shrunk_face = shrunk_face.cut(hole_face)
                except Exception:
                    pass  # Skip problematic holes but keep processing the rest

            if not shrunk_face.isNull() and shrunk_face.Area > 0:
                effective_face = shrunk_face

        return effective_face

    def _calculate_origin(self, effective_face, center, u_vec, v_vec, obj):
        """
        Calculates the 3D origin for the pattern grid, applying semantic alignment shifts.

        The grid origin defines the anchor point (0,0) for the first material unit (tile,
        board, or pattern repetition). The behavior varies based on the TileAlignment property:

        - 'Center': The grid is shifted so that the centre of a tile coincides with the
          centre of the boundary, rather than a grout intersection.
        - Corner Presets ('BottomLeft', 'BottomRight', 'TopLeft', 'TopRight'): The *named*
          corner of a tile is aligned to the *named* corner of the bounding box.
          Implemented by getAlignedGridOrigin — see its docstring for the full
          shift semantics.
        - 'Custom': Boundary-based semantic logic is bypassed. The AlignmentOffset
          property is treated as an absolute local coordinate relative to the face centre.
        """
        # In Custom mode, the AlignmentOffset is treated as an absolute
        # coordinate relative to the face center.
        if obj.TileAlignment == "Custom":
            return center + u_vec * obj.AlignmentOffset.x + v_vec * obj.AlignmentOffset.y

        return getAlignedGridOrigin(
            effective_face,
            center,
            u_vec,
            v_vec,
            obj.TileAlignment,
            obj.TileLength.Value,
            obj.TileWidth.Value,
            obj.AlignmentOffset,
        )

    def execute(self, obj):
        """
        Calculates the geometry of the finish by deriving the boundaries in world space
        and calling the pattern tessellator.
        """
        import Part

        if self.clone(obj):
            return

        base_face = Arch.getFaceGeometry(obj.Base)
        if not base_face:
            FreeCAD.Console.PrintWarning(
                f"ArchCovering [{obj.Label}]: no base face found, skipping recompute.\n"
            )
            return

        # Establish a stable coordinate system based on the global axes.
        u_vec, v_vec, normal, center = self._get_layout_frame(base_face)

        # Apply boundary offsets in global space.
        effective_face = self._apply_boundaries(base_face, obj)

        # Determine the pattern origin in global space.
        origin_3d = self._calculate_origin(effective_face, center, u_vec, v_vec, obj)

        # Build a typed config so that missing or misspelled fields raise AttributeError
        # at construction time instead of silently defaulting to zero inside the engine.
        if obj.FinishMode == "Hatch Pattern":
            config = ArchTessellation.HatchConfig(
                filename=obj.PatternFile,
                pattern_name=obj.PatternName,
                scale=obj.PatternScale,
                rotation=obj.Rotation.Value,
                thickness=obj.TileThickness.Value,
            )
        else:
            config = ArchTessellation.TileConfig(
                finish_mode=obj.FinishMode,
                length=obj.TileLength.Value,
                width=obj.TileWidth.Value,
                thickness=obj.TileThickness.Value,
                joint=obj.JointWidth.Value,
                rotation=obj.Rotation.Value,
                stagger_type=getattr(obj, "StaggerType", "Stacked (None)"),
                stagger_custom=getattr(obj, "StaggerCustom", FreeCAD.Units.Quantity(0)).Value,
            )

        tessellator = ArchTessellation.create_tessellator(config)
        res = tessellator.compute(
            effective_face,
            origin_3d,
            u_vec,
            normal,
        )

        # Update the UI with any status messages from the engine.
        #
        # Control-flow contract: INVALID_DIMENSIONS is the only status that
        # prevents geometry assignment (it returns early because the engine
        # produced no usable shape).  All other non-OK statuses are warnings:
        # they print a message but allow execution to continue so that the
        # analytical fallback geometry is still assigned below.  Any new status
        # added in future must explicitly decide which category it belongs to.
        match res.status:
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
                        "Arch", "The number of tiles is extremely high. Layout lines are hidden."
                    )
                    + "\n"
                )
            case _:
                pass

        if res.geometry:
            # We assign the local-space geometry and the placement returned by the engine.
            obj.Shape = res.geometry
            obj.Placement = res.placement
        else:
            obj.Shape = Part.Shape()
            obj.Placement = FreeCAD.Placement()

        # Update Quantity Take-Off properties.
        obj.CountFullTiles = res.quantities.count_full
        obj.CountPartialTiles = res.quantities.count_partial
        obj.NetArea = res.quantities.area_net
        obj.GrossArea = res.quantities.area_gross
        obj.WasteArea = res.quantities.waste_area
        obj.TotalJointLength = res.quantities.length_joints
        obj.PerimeterLength = res.quantities.length_perimeter
