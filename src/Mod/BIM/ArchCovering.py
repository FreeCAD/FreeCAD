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

import os
import time
import FreeCAD
import Part
import ArchComponent
from draftutils import params

if FreeCAD.GuiUp:
    from PySide import QtGui
    import FreeCADGui

    QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
    translate = FreeCAD.Qt.translate
else:

    def translate(ctxt, txt):
        return txt

    def QT_TRANSLATE_NOOP(ctxt, txt):
        return txt


# Anything smaller than this is considered effectively zero to prevent numerical instability
# in the geometry calculations or division-by-zero errors in Python.
MIN_DIMENSION = 0.1
TOO_MANY_TILES = 10000


def profile_it(func):
    """Simple performance counter decorator."""

    def wrapper(*args, **kwargs):
        # Capture start time
        t0 = time.perf_counter()

        # Run the actual function
        result = func(*args, **kwargs)

        # Calculate duration
        dt = time.perf_counter() - t0

        # Try to identify the object Label for context
        label = "Unknown"
        try:
            # In Arch, args[1] is almost always the 'obj' (App::FeaturePython)
            if len(args) > 1 and hasattr(args[1], "Label"):
                label = args[1].Label
        except Exception:
            pass

        # Log if it took longer than 1ms (reduces noise)
        if dt > 0.001:
            FreeCAD.Console.PrintLog(f"[PERF] {label} -> {func.__name__}: {dt:.4f}s\n")

        return result

    return wrapper


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
                "The type of finish to create",
                ["Solid Tiles", "Parametric Pattern", "Hatch Pattern"],
            ),
            (
                "App::PropertyEnumeration",
                "TileAlignment",
                "Covering",
                "The alignment of the tile grid",
                ["Center", "TopLeft", "TopRight", "BottomLeft", "BottomRight"],
            ),
            ("App::PropertyAngle", "Rotation", "Covering", "Rotation of the finish", 0),
            ("App::PropertyLength", "TileLength", "Tiles", "The length of the tiles", 0),
            ("App::PropertyLength", "TileWidth", "Tiles", "The width of the tiles", 0),
            ("App::PropertyLength", "TileThickness", "Tiles", "The thickness of the tiles", 0),
            ("App::PropertyLength", "JointWidth", "Tiles", "The width of the joints", 0),
            ("App::PropertyVector", "TileOffset", "Tiles", "The offset of alternating rows", None),
            (
                "App::PropertyVector",
                "AlignmentOffset",
                "Tiles",
                "A manual offset to shift the grid origin (X=U, Y=V). The Z component is ignored",
                None,
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
            ("App::PropertyInteger", "CountFullTiles", "Quantities", "The number of full tiles", 0),
            (
                "App::PropertyInteger",
                "CountPartialTiles",
                "Quantities",
                "The number of cut/partial tiles",
                0,
            ),
            (
                "App::PropertyFile",
                "PatternFile",
                "Pattern",
                "The PAT file to use for hatching",
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
                "IfcPredefinedType",
                "IFC",
                "The specific type of covering",
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

        # Property status configuration (Read-Only fields)
        obj.setEditorMode("NetArea", 1)
        obj.setEditorMode("GrossArea", 1)
        obj.setEditorMode("WasteArea", 1)
        obj.setEditorMode("TotalJointLength", 1)
        obj.setEditorMode("CountFullTiles", 1)
        obj.setEditorMode("CountPartialTiles", 1)

        # Cleanup deprecated Stats group if present
        if "CountFullTiles" in properties_list:
            if obj.getGroupOfProperty("CountFullTiles") == "Stats":
                obj.setGroupOfProperty("CountFullTiles", "Quantities")
                obj.setGroupOfProperty("CountPartialTiles", "Quantities")

    def loads(self, state):
        """
        Overrides the parent callback used by FreeCAD's persistence engine to restore the
        Python proxy instance and reset non-persistent internal attributes like 'Type'.
        """
        self.Type = "Covering"

    def onDocumentRestored(self, obj):
        """
        Overrides the parent callback triggered after the document is fully restored. Used to
        ensure property schema consistency and perform backward compatibility migrations.
        """
        super().onDocumentRestored(obj)
        self.setProperties(obj)
        # Ensure the ViewProvider schema is also updated
        vobj = getattr(obj, "ViewObject", None)
        vproxy = getattr(vobj, "Proxy", None)
        if hasattr(vproxy, "setProperties"):
            vproxy.setProperties(vobj)

    def execute(self, obj):
        """
        Calculates the geometry and updates the shape of the object.

        This is a standard FreeCAD C++ callback triggered during a document recompute. It translates
        the numerical and textual properties of the object into a geometric representation (solids,
        faces, or wires) assigned to the `Shape` attribute.

        Parameters
        ----------
        obj : Part::FeaturePython
            The base C++ object whose shape is updated.

        """
        if self.clone(obj):
            return

        base_face = self.get_base_face(obj)
        if not base_face:
            return

        if obj.FinishMode == "Hatch Pattern":
            from draftobjects.hatch import Hatch

            # Always assign the base face first. This ensures the Covering has geometry even if the
            # pattern file is missing or invalid.
            obj.Shape = base_face

            # Force a minimum scale to prevent math errors or infinite loops in the hatching engine.
            safe_scale = max(MIN_DIMENSION, obj.PatternScale)

            if obj.PatternFile:
                pat = Hatch.hatch(
                    base_face,
                    obj.PatternFile,
                    obj.PatternName,
                    scale=safe_scale,
                    rotation=obj.Rotation.Value,
                )
                if pat:
                    obj.Shape = Part.Compound([base_face, pat])

            self.computeAreas(obj)
            return

        # Establish the local coordinate system and grid origin for the tiling pattern.
        u_vec, v_vec, normal, center_point = self._get_grid_basis(base_face)
        origin = self._get_grid_origin(obj, base_face, u_vec, v_vec, center_point)

        # Cache dimensions
        t_len = obj.TileLength.Value
        t_wid = obj.TileWidth.Value
        j_len = obj.JointWidth.Value
        j_wid = obj.JointWidth.Value

        # Determine cut thickness, increase thickness for solid tiles to avoid intersections between
        # cutters and the tile geometry.
        cut_thick = obj.TileThickness.Value * 1.1 if obj.FinishMode == "Solid Tiles" else 1.0

        # Create the cutter tools
        cutters_h, cutters_v = self._build_cutters(
            obj, base_face.BoundBox, t_len, t_wid, j_len, j_wid, cut_thick
        )

        # Perform the cut operations and assign the resulting shape to the covering
        obj.Shape = self._perform_cut(
            obj, base_face, cutters_h, cutters_v, normal, origin, u_vec, v_vec
        )

    def get_best_face(self, obj, view_direction=None):
        """
        Returns the name of the best candidate face (e.g. 'Face1') for a covering.
        Heuristics:
        1. Filter for faces with the largest area (within 5% tolerance).
        2. Preference for face opposing the view direction (Camera facing).
        """
        if not hasattr(obj, "Shape") or not obj.Shape.Faces:
            return None

        # 1. Gather Face Data: (Name, FaceObj, Area)
        faces_data = []
        for i, f in enumerate(obj.Shape.Faces):
            # Check for planarity
            if f.findPlane() is None:
                continue
            faces_data.append((f"Face{i+1}", f, f.Area))

        # 2. Filter by Area (Keep faces within 95% of max area)
        if not faces_data:
            return None

        faces_data.sort(key=lambda x: x[2], reverse=True)
        max_area = faces_data[0][2]
        candidates = [x for x in faces_data if x[2] >= max_area * 0.95]

        if not candidates:
            return None

        # 3. Heuristic: View Direction (Camera facing)
        if view_direction:
            # Dot product: Lower value means vectors are opposing (face looks at camera)
            candidates.sort(key=lambda x: x[1].normalAt(0, 0).dot(view_direction))
            return candidates[0][0]

        # Default: Return the first large face found
        return candidates[0][0]

    def get_base_face(self, obj):
        """
        Resolves the 'Base' link to identify and return the target planar face for the covering.
        Handles sub-element selection, solid top-face detection, and closed-wire conversion.
        Returns a Part.Face or None if a valid planar surface cannot be determined.
        """
        if not obj.Base:
            return None

        val = obj.Base
        if isinstance(val, tuple):
            linked_obj = val[0]
            sub_elements = val[1]
        else:
            linked_obj = val
            sub_elements = []

        face = None

        if len(sub_elements) > 0:
            # Sub-element linked (e.g. Face6)
            try:
                sub_shape = linked_obj.getSubObject(sub_elements[0])
                if sub_shape.ShapeType == "Face":
                    face = sub_shape
            except Exception as e:
                FreeCAD.Console.PrintWarning(
                    "ArchCovering: Unable to retrieve subobject: {}\n".format(e)
                )

        if not face and hasattr(linked_obj, "Shape"):
            # Whole object linked
            if linked_obj.Shape.ShapeType == "Face":
                face = linked_obj.Shape
            elif linked_obj.Shape.Solids:
                # Use smart detection to find the best face (e.g. largest)
                best_face_name = self.get_best_face(linked_obj)
                if best_face_name:
                    face = linked_obj.getSubObject(best_face_name)
            # Support for closed Wires (e.g. Draft Circle/Rectangle with MakeFace=False)
            elif linked_obj.Shape.ShapeType in ["Wire", "Edge"]:
                if linked_obj.Shape.isClosed():
                    try:
                        face = Part.Face(Part.Wire(linked_obj.Shape.Edges))
                    except Part.OCCError as e:
                        FreeCAD.Console.PrintWarning(
                            "ArchCovering: Unable to create face from wire: {}\n".format(e)
                        )
                else:
                    FreeCAD.Console.PrintWarning(
                        translate(
                            "Arch",
                            "Covering: The base wire is not closed. A closed loop is required to create a surface finish.",
                        )
                        + "\n"
                    )

        if face:
            # findPlane() returns a Plane object for flat faces, or None for curved ones.
            if face.findPlane() is None:
                FreeCAD.Console.PrintWarning(
                    translate("Arch", "Covering: Only planar surfaces are currently supported.")
                    + "\n"
                )
                return None
            return face

        return None

    def _get_grid_basis(self, base_face):
        """
        Computes the local coordinate system basis vectors for a face.

        Calculates the tangent U and V directions at the face center, ensures orthogonality, and
        determines the face normal and center point.

        Parameters
        ----------
        base_face : Part.Face
            The face to analyze.

        Returns
        -------
        tuple
            (u_vec, v_vec, normal, center_point) as FreeCAD.Vectors.
        """
        # Determine grid basis (U, V directions from face)
        # Map 3D center to 2D parameters to establish a surface reference point.
        u_p, v_p = base_face.Surface.parameter(base_face.BoundBox.Center)
        # Derive local surface axes; normalize U to serve as a consistent direction unit.
        u_vec, v_vec = base_face.Surface.tangent(u_p, v_p)
        u_vec.normalize()

        # Calculate normal and re-orthogonalize V vector.
        # Ensure the tiling grid is perfectly square by forcing V to be perpendicular to U and the
        # surface normal.
        normal = u_vec.cross(v_vec)
        normal.normalize()
        v_vec = normal.cross(u_vec)
        v_vec.normalize()

        center_point = base_face.Surface.value(u_p, v_p)
        return u_vec, v_vec, normal, center_point

    def _get_grid_origin(self, obj, base_face, u_vec, v_vec, center_point):
        """
        Calculates the starting 3D point for the tiling grid.

        Projects the face vertices onto the U and V basis vectors to find the face extents, then
        determines the origin point based on the TileAlignment property.

        Parameters
        ----------
        obj : App.FeaturePython
            The covering object containing the alignment property.
        base_face : Part.Face
            The face defining the geometric boundaries.
        u_vec : FreeCAD.Vector
            The local horizontal direction of the grid.
        v_vec : FreeCAD.Vector
            The local vertical direction of the grid.
        center_point : FreeCAD.Vector
            The reference center point of the face.

        Returns
        -------
        FreeCAD.Vector
            The global 3D coordinate for the grid starting point.
        """
        # Find extents - Initialize with first vertex
        v0 = base_face.Vertexes[0].Point
        vec0 = v0.sub(center_point)

        min_u = max_u = vec0.dot(u_vec)
        min_v = max_v = vec0.dot(v_vec)

        # Project remaining vertices
        for v in base_face.Vertexes[1:]:
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

        # Determine the Grid Origin based on Alignment
        align = obj.TileAlignment
        origin_offset = FreeCAD.Vector(0, 0, 0)

        if align == "Center":
            mid_u = (min_u + max_u) / 2
            mid_v = (min_v + max_v) / 2
            origin_offset = (u_vec * mid_u) + (v_vec * mid_v)
        elif align == "BottomLeft":
            origin_offset = (u_vec * min_u) + (v_vec * min_v)
        elif align == "BottomRight":
            origin_offset = (u_vec * max_u) + (v_vec * min_v)
        elif align == "TopLeft":
            origin_offset = (u_vec * min_u) + (v_vec * max_v)
        elif align == "TopRight":
            origin_offset = (u_vec * max_u) + (v_vec * max_v)

        # Apply manual AlignmentOffset (X moves in U, Y moves in V, Z is ignored)
        if hasattr(obj, "AlignmentOffset"):
            align_shift = (u_vec * obj.AlignmentOffset.x) + (v_vec * obj.AlignmentOffset.y)
            origin_offset = origin_offset.add(align_shift)

        return center_point.add(origin_offset)

    def _build_cutters(self, obj, bbox, t_len, t_wid, j_len, j_wid, cut_thick):
        """
        Generates the grid of solids representing the joints between tiles.

        These boxes are used as Boolean tools to subtract the gaps from the base face or volume.
        The function estimates the required number of rows and columns based on the face
        bounding box and incorporates pattern offsets for staggered bonds.

        Parameters
        ----------
        obj : App::FeaturePython
            The covering object containing pattern settings like TileOffset.
        bbox : Base::BoundBox
            The bounding box of the target face used to determine the grid extent.
        t_len : float
            The local length of a single tile.
        t_wid : float
            The local width of a single tile.
        j_len : float
            The width of the vertical joint gap.
        j_wid : float
            The width of the horizontal joint gap.
        cut_thick : float
            The thickness (z-height) of the generated joint solids.

        Returns
        -------
        tuple
            A pair of lists (cutters_h, cutters_v) containing Part.Box solids, or (None, None)
            if dimensions are invalid or the calculated tile count exceeds safety limits.
        """
        # Step size
        step_x = t_len + j_len
        step_y = t_wid + j_wid

        # Prevent division by zero or extremely small values
        if step_x < MIN_DIMENSION or step_y < MIN_DIMENSION:
            return None, None

        # Estimate count. Use the base face's diagonal to ensure full coverage, even if the tiling
        # grid is rotated 45° (worst-case scenario) via the Rotation property.
        diag = bbox.DiagonalLength
        count_x = int(diag / step_x) + 4
        count_y = int(diag / step_y) + 4

        # Prevent memory exhaustion by too many tiles
        if (count_x * count_y) > TOO_MANY_TILES:
            FreeCAD.Console.PrintWarning(
                translate("Arch", "Covering: Tile count too high. Aborting.") + "\n"
            )
            return None, None

        # Determine Z offset for cutters
        if obj.FinishMode == "Solid Tiles":
            z_gen_offset = (obj.TileThickness.Value - cut_thick) / 2
        else:
            z_gen_offset = -cut_thick / 2

        cutters_h = []
        cutters_v = []

        # Offsets
        off_x = obj.TileOffset.x
        off_y = obj.TileOffset.y

        # Mutual exclusivity guard: prioritize x over y
        if (
            abs(off_x) > Part.Precision.approximation()
            and abs(off_y) > Part.Precision.approximation()
        ):
            FreeCAD.Console.PrintWarning(
                translate("Arch", "Covering: TileOffset.x/y are exclusive. Ignoring y.") + "\n"
            )
            off_y = 0.0

        # Generate horizontal strips (rows). These will always be a set of long strips running the
        # full width of the face, with width the size of the joint
        # OpenCascade requires dimensions > 0 for solids
        if j_wid > MIN_DIMENSION:
            full_len_x = 2 * count_x * step_x
            start_x = -count_x * step_x

            for j in range(-count_y, count_y):
                row_y_offset = off_y if (j % 2 != 0) else 0
                local_y = j * step_y + t_wid + row_y_offset
                jh_box = Part.makeBox(
                    full_len_x, j_wid, cut_thick, FreeCAD.Vector(start_x, local_y, z_gen_offset)
                )
                cutters_h.append(jh_box)

        # Generate vertical strips (cols). If no tile offset is specified, we're laying a stack bond
        # and the vertical strips are built similarly to horizontal strips, but in the vertical
        # direction. If a tile offset is specified, we're laying a running bond, and the vertical
        # cutters are generated as individual segments for each row to create the offset.
        if j_len > MIN_DIMENSION:
            is_stack_bond = (
                abs(off_x) < Part.Precision.approximation()
                and abs(off_y) < Part.Precision.approximation()
            )

            if is_stack_bond:
                full_len_y = 2 * count_y * step_y
                start_y = -count_y * step_y
                for i in range(-count_x, count_x):
                    local_x = i * step_x + t_len
                    jv_box = Part.makeBox(
                        j_len, full_len_y, cut_thick, FreeCAD.Vector(local_x, start_y, z_gen_offset)
                    )
                    cutters_v.append(jv_box)
            else:
                for j in range(-count_y, count_y):
                    row_off_x = off_x if (j % 2 != 0) else 0
                    row_off_y = off_y if (j % 2 != 0) else 0
                    row_y = j * step_y + row_off_y
                    for i in range(-count_x, count_x):
                        local_x = i * step_x + t_len + row_off_x
                        jv_box = Part.makeBox(
                            j_len, step_y, cut_thick, FreeCAD.Vector(local_x, row_y, z_gen_offset)
                        )
                        cutters_v.append(jv_box)

        return cutters_h, cutters_v

    @profile_it
    def _perform_cut(self, obj, base_face, cutters_h, cutters_v, normal, origin, u_vec, v_vec):
        """
        Executes the Boolean operations to divide the base face into tiles.

        This function transforms the pre-generated cutters to the correct grid orientation and
        location, then performs sequential cuts to subtract the joint gaps. It handles both 3D solid
        tiles and 2D parametric patterns. It also calculates and updates tile count statistics based
        on geometry volume or area.

        Parameters
        ----------
        obj : App::FeaturePython
            The covering object containing user settings and where statistics are updated.
        base_face : Part.Face
            The primary planar surface to be tiled.
        cutters_h : list of Part.Box
            The horizontal joint solids generated by _build_cutters.
        cutters_v : list of Part.Box
            The vertical joint solids generated by _build_cutters.
        normal : Base.Vector
            The face normal vector used for extrusion and rotation calculation.
        origin : Base.Vector
            The 3D coordinate of the grid alignment point.
        u_vec : Base.Vector
            Local horizontal unit vector of the face.
        v_vec : Base.Vector
            Local vertical unit vector of the face.

        Returns
        -------
        Part.Shape
            A compound of solids (for Solid Tiles mode) or a compound of wires (for Parametric
            Pattern mode).
        """
        # If the cutter builder returned None, some of the parameters (e.g. tile size or joint
        # width) were invalid.
        # Return an empty Part.Shape() and allow the document to continue recomputing other objects.
        if cutters_h is None or cutters_v is None:
            return Part.Shape()

        # Prepare transformation
        tr = FreeCAD.Placement()
        tr.Base = origin
        # Assumes the vectors are already unit vectors and orthogonal
        face_rot = FreeCAD.Rotation(u_vec, v_vec, normal)
        tile_rot = FreeCAD.Rotation(FreeCAD.Vector(0, 0, 1), obj.Rotation.Value)
        tr.Rotation = face_rot.multiply(tile_rot)

        # Grout/Joint length calculation (centerlines)
        joint_len = self._calculate_joint_length(obj, base_face, tr)

        # Fallback for zero-joint width where no cutters were generated.
        if not cutters_h and not cutters_v:
            obj.NetArea = base_face.Area
            obj.GrossArea = base_face.Area
            obj.WasteArea = 0
            obj.TotalJointLength = 0
            if obj.FinishMode == "Solid Tiles":
                return base_face.extrude(normal * obj.TileThickness.Value)
            else:
                # For 2D modes, apply a micro-offset to prevent z-fighting
                offset_matrix = FreeCAD.Matrix()
                offset_matrix.move(normal.normalize() * 0.05)
                return base_face.transformGeometry(offset_matrix)

        t_len = obj.TileLength.Value
        t_wid = obj.TileWidth.Value

        full_cnt = 0
        part_cnt = 0
        shape_to_return = Part.Shape()

        try:
            if obj.FinishMode == "Solid Tiles":
                # Extrude Base Face
                try:
                    tile_layer = base_face.extrude(normal * obj.TileThickness.Value)
                except Part.OCCError as e:
                    FreeCAD.Console.PrintError(f"Covering: Caught OCCError in _perform_cut: {e}")
                    tile_layer = Part.Shape()

                # Cut
                if cutters_h:
                    comp_h = Part.Compound(cutters_h)
                    comp_h.Placement = tr
                    tile_layer = tile_layer.cut(comp_h)

                if cutters_v:
                    comp_v = Part.Compound(cutters_v)
                    comp_v.Placement = tr
                    result_shape = tile_layer.cut(comp_v)
                else:
                    result_shape = tile_layer

                # Count
                full_vol = t_len * t_wid * obj.TileThickness.Value
                if result_shape.Solids:
                    for sol in result_shape.Solids:
                        if abs(sol.Volume - full_vol) < 0.001:
                            full_cnt += 1
                        else:
                            part_cnt += 1

                shape_to_return = result_shape

            elif obj.FinishMode == "Parametric Pattern":
                # Perform the cuts on the 2D base face
                result_shape = base_face

                if cutters_h:
                    comp_h = Part.Compound(cutters_h)
                    comp_h.Placement = tr
                    result_shape = result_shape.cut(comp_h)

                if cutters_v:
                    comp_v = Part.Compound(cutters_v)
                    comp_v.Placement = tr
                    result_shape = result_shape.cut(comp_v)

                # Count the resulting faces
                full_area = t_len * t_wid
                if result_shape.Faces:
                    for f in result_shape.Faces:
                        if abs(f.Area - full_area) < 0.001:
                            full_cnt += 1
                        else:
                            part_cnt += 1

                # Convert Faces to Wires for lightweight representation
                wires = []
                for f in result_shape.Faces:
                    wires.extend(f.Wires)

                if wires:
                    final_pattern = Part.Compound(wires)
                    # Apply a micro offset (0.05mm) along the normal. This prevents "Z-fighting"
                    # (visual flickering) in the 3D viewer by ensuring the pattern sits slightly
                    # above the base face.
                    offset_matrix = FreeCAD.Matrix()
                    offset_matrix.move(normal.normalize() * 0.05)
                    shape_to_return = final_pattern.transformGeometry(offset_matrix)
                else:
                    shape_to_return = Part.Shape()

            # Sync quantities for both modes
            q = self._calculate_quantities(base_face, full_cnt, part_cnt, t_len, t_wid)
            obj.CountFullTiles = full_cnt
            obj.CountPartialTiles = part_cnt
            obj.NetArea = q["NetArea"]
            obj.GrossArea = q["GrossArea"]
            obj.WasteArea = q["WasteArea"]
            obj.TotalJointLength = joint_len

        except Part.OCCError as e:
            # Catch OpenCascade kernel errors to prevent crashing the document recompute chain.
            FreeCAD.Console.PrintWarning(
                translate("Arch", "Covering: OpenCascade error during boolean operations on")
                + f" {obj.Label}: {str(e)}\n"
            )
            return Part.Shape()

        return shape_to_return

    def _calculate_quantities(self, base_face, count_full, count_partial, t_len, t_wid):
        """Helper to calculate area quantities."""
        net_area = base_face.Area
        gross_area = (count_full + count_partial) * (t_len * t_wid)
        # Waste area is clamped to zero as App::PropertyArea does not accept negative values.
        waste_area = max(0.0, gross_area - net_area)
        return {"NetArea": net_area, "GrossArea": gross_area, "WasteArea": waste_area}

    def _calculate_joint_length(self, obj, base_face, tr):
        """Calculates and updates the TotalJointLength property."""
        if obj.JointWidth.Value <= MIN_DIMENSION:
            obj.TotalJointLength = 0
            return

        t_len = obj.TileLength.Value
        t_wid = obj.TileWidth.Value
        j_wid = obj.JointWidth.Value
        step_x = t_len + j_wid
        step_y = t_wid + j_wid

        diag = base_face.BoundBox.DiagonalLength
        count_x = int(diag / step_x) + 4
        count_y = int(diag / step_y) + 4
        off_x = obj.TileOffset.x
        off_y = obj.TileOffset.y

        centerlines = []
        full_len_x = 2 * count_x * step_x
        start_x = -count_x * step_x
        full_len_y = 2 * count_y * step_y
        start_y = -count_y * step_y

        # Horizontal joint centerlines
        for j in range(-count_y, count_y):
            row_y_offset = off_y if (j % 2 != 0) else 0
            local_y = j * step_y + t_wid + row_y_offset + (j_wid / 2)
            centerlines.append(
                Part.makeLine(
                    FreeCAD.Vector(start_x, local_y, 0),
                    FreeCAD.Vector(start_x + full_len_x, local_y, 0),
                )
            )

        # Vertical joint centerlines
        if (
            abs(off_x) < Part.Precision.approximation()
            and abs(off_y) < Part.Precision.approximation()
        ):
            # Stack bond
            for i in range(-count_x, count_x):
                local_x = i * step_x + t_len + (j_wid / 2)
                centerlines.append(
                    Part.makeLine(
                        FreeCAD.Vector(local_x, start_y, 0),
                        FreeCAD.Vector(local_x, start_y + full_len_y, 0),
                    )
                )
        else:
            # Running bond: segmented lines per row
            for j in range(-count_y, count_y):
                row_off_x = off_x if (j % 2 != 0) else 0
                row_off_y = off_y if (j % 2 != 0) else 0
                row_y = j * step_y + row_off_y
                for i in range(-count_x, count_x):
                    local_x = i * step_x + t_len + row_off_x + (j_wid / 2)
                    centerlines.append(
                        Part.makeLine(
                            FreeCAD.Vector(local_x, row_y, 0),
                            FreeCAD.Vector(local_x, row_y + step_y, 0),
                        )
                    )

        grid_compound = Part.Compound(centerlines)
        grid_compound.Placement = tr
        try:
            clipped_joints = base_face.common(grid_compound)
            return clipped_joints.Length
        except Exception:
            return 0.0

    def computeAreas(self, obj):
        """Overrides the default calculation with lightweight quantity updates."""
        if obj.FinishMode == "Hatch Pattern":
            for prop in ["NetArea", "GrossArea", "WasteArea", "TotalJointLength"]:
                setattr(obj, prop, 0)
            obj.CountFullTiles = 0
            obj.CountPartialTiles = 0


if FreeCAD.GuiUp:

    class _ViewProviderCovering(ArchComponent.ViewProviderComponent):
        """
        The GUI ViewProvider for the Covering object.

        This class manages the visual representation of an Arch Covering within the FreeCAD 3D view
        and the Tree View. It is responsible for constructing the Coin3D scene graph, applying
        texture mapping logic, managing the object's icon, and handling high-level UI events like
        entering edit mode.

        Parameters
        ----------
        vobj : PartGui::ViewProviderPython
            The base C++ ViewProvider object to be initialized.
        """

        EDIT_MODE_STANDARD = 0
        # Static cache to store loaded images (path -> SbImage)
        # This prevents reloading the same file from disk for every object.
        _texture_cache = {}

        def __init__(self, vobj):
            super().__init__(vobj)
            self.setProperties(vobj)
            self.texture = None
            self.texcoords = None

        def setProperties(self, vobj):
            """
            Defines and initializes the visual properties of the Covering.

            This method is a Python-side helper that manages properties attached directly to the
            `ViewObject`. These properties control the object's appearance—such as texture images
            and visual scaling—but do not affect the underlying geometric model or IFC data.

            Parameters
            ----------
            vobj : Gui::ViewProviderPython
                The C++ ViewProvider object to which the properties are added.

            Notes
            -----
            This method is not a C++ callback; it is invoked manually during ViewProvider
            initialization (`__init__`) and document restoration (`loads`) to ensure the property
            editor stays synchronized.
            """
            properties_list = vobj.PropertiesList

            # Texture properties
            if not "TextureImage" in properties_list:
                vobj.addProperty(
                    "App::PropertyFile",
                    "TextureImage",
                    "Visual",
                    QT_TRANSLATE_NOOP("App::Property", "An image file to map onto each tile"),
                    locked=True,
                )
            if not "TextureScale" in properties_list:
                vobj.addProperty(
                    "App::PropertyVector",
                    "TextureScale",
                    "Visual",
                    QT_TRANSLATE_NOOP("App::Property", "The scaling of the texture on each tile"),
                    locked=True,
                )
                vobj.TextureScale = FreeCAD.Vector(1, 1, 0)

        def onDocumentRestored(self, vobj):
            self.setProperties(vobj)

        def getIcon(self):
            """
            Returns the icon representing the object in the Tree View.

            This is a standard FreeCAD C++ callback. The application invokes this method whenever it
            needs to draw the icon associated with the object in the document tree.

            Returns
            -------
            str
                A string containing either the resource path to an SVG file
                (e.g., ":/icons/BIM_Covering.svg") or a valid XPM-formatted image
                string.
            """
            return ":/icons/BIM_Covering.svg"

        def setEdit(self, vobj, mode=0):
            """
            Prepares the interface for entering Edit Mode.

            This callback is triggered when the object is double-clicked in the Tree View or edited
            via the context menu. It is responsible for launching the Task Panel and preparing the
            visual state of the object for modification.

            Parameters
            ----------
            vobj : Gui::ViewProviderPython
                The ViewProvider object associated with this covering.
            mode : int
                The edit strategy requested by the user or the system:
                * 0: Standard/Default (usually opens the custom Task Panel).
                * 1: Transform (graphical movement and rotation gizmo).
                * 2: Cutting (clipping plane tool).
                * 3: Color (per-face color editing tool).

            Returns
            -------
            bool or None
                * True: Python has handled the request; formally enter Edit Mode.
                * False: An action was performed, but do not enter Edit Mode.
                * None: Delegate handling to built-in C++ default behaviors.

            Notes
            -----
            Returning None for modes other than 0 allows standard FreeCAD tools, like the Transform
            gizmo, to function without custom implementation.
            """
            # Only handle the edit mode with a custom Task Panel.
            if mode != self.EDIT_MODE_STANDARD:
                return None

            task_control = FreeCADGui.Control
            task_panel = task_control.activeDialog()

            # Prevent re-entrant calls
            if task_panel and isinstance(task_panel, ArchCoveringTaskPanel):
                return True

            task_control.showDialog(ArchCoveringTaskPanel(obj=vobj.Object))

            return True

        def unsetEdit(self, vobj, mode=0):
            """
            Cleans up the interface when exiting Edit Mode.

            This callback is triggered when an edit session is terminated by the user (clicking
            OK/Cancel or pressing Escape) or by the system. It is responsible for closing the Task
            Panel and restoring the object's original visual state.

            Parameters
            ----------
            vobj : PartGui::ViewProviderPython
                The ViewProvider object associated with this covering.
            mode : int
                The edit strategy being terminated (see `setEdit` for values).

            Returns
            -------
            bool or None
                * True: Cleanup was successfully handled by Python.
                * None: Fall back to default C++ cleanup logic.
            """
            # Only handle the edit mode with a custom Task Panel.
            if mode != self.EDIT_MODE_STANDARD:
                return None

            task_control = FreeCADGui.Control

            task_control.closeDialog()

            return True

        def doubleClicked(self, vobj):
            """
            Handles the mouse double-click event in the Tree View.

            This method acts as a high-level UI listener. Its primary role is to intercept the
            default FreeCAD behavior (which is to toggle visibility) and redirect the event to start
            a formal edit session.

            Parameters
            ----------
            vobj : PartGui::ViewProviderPython
                The C++ ViewProvider object associated with this covering.

            Returns
            -------
            bool
                Returns True to signal that the event has been handled, preventing FreeCAD from
                executing the default action (generally falling back to editing the object's Label
                in the Tree View).

            Notes
            -----
            This method typically invokes the inherited `self.edit()` helper, which triggers the
            formal `setEdit` handshake with the GUI.
            """
            # Start the edit session with the standard mode. We use the parent class to handle it.
            self.edit()
            return True

        def attach(self, vobj):
            """
            Initializes the Coin3D scene graph for the object.

            This method is called when the object is first created or when a document is loaded. it
            is responsible for creating the OpenInventor nodes (materials, textures, geometry) and
            attaching them to the ViewProvider's root scene graph.

            Parameters
            ----------
            vobj : PartGui::ViewProviderPython
                The C++ ViewProvider object associated with this covering.

            Notes
            -----
            In the BIM code, this method is usually used to call the parent class's `attach`
            logic first, ensuring the base geometry is constructed before child-specific visual
            modifications (like texture mapping) are applied.
            """
            # Let the parent class build the default scene graph first.
            super().attach(vobj)
            self.Object = vobj.Object
            # Apply the texture logic to the graph that the parent created.
            self.updateTexture(self.Object)

        @profile_it
        def updateData(self, obj, prop):
            """
            Reacts to changes in the object's Data properties.

            This callback is triggered by FreeCAD when a property belonging to the
            data object (the C++ host) is modified. Its role is to synchronize
            the visual representation in the 3D view with the underlying
            parametric data.

            Parameters
            ----------
            obj : Part::FeaturePython
                The base C++ data object whose property has changed.
            prop : str
                The name of the modified property (e.g., "Shape", "Rotation").

            Notes
            -----
            For Arch Coverings, this method is used for refreshing texture
            mapping when the geometry is recalculated or the orientation changes.
            """
            # Call the parent's updateData to regenerate the shape's coin representation.
            # Skip it if the Base is a tuple (linked sub-element), as the parent class does not
            # support them.
            if not (prop == "Shape" and isinstance(obj.Base, tuple)):
                super().updateData(obj, prop)

            # Apply the texture modifications once the scene graph is rebuilt and stable.
            if prop in ["Shape", "TextureImage", "TextureScale", "Rotation"]:
                self.updateTexture(obj)

        def onChanged(self, vobj, prop):
            """
            Reacts to changes in the ViewProvider's View properties.

            This callback is triggered when a property belonging to the ViewProvider itself is
            modified. These properties usually control visual styles rather than geometric
            dimensions.

            Parameters
            ----------
            vobj : PartGui::ViewProviderPython
                The C++ ViewProvider object associated with this covering.
            prop : str
                The name of the modified view property (e.g., "TextureImage",
                "ShapeColor", "Transparency").

            Notes
            -----
            This method typically ensures that child-specific logic, such as reloading a texture
            image from disk, is executed after the parent class has handled standard property
            updates.
            """
            # Let the parent class handle its properties first.
            super().onChanged(vobj, prop)

            # Apply the texture logic after the parent is done.
            if prop in ["TextureImage", "TextureScale"]:
                self.updateTexture(vobj.Object)

        @profile_it
        def updateTexture(self, obj):
            """Configures and applies the texture to the object's scene graph.

            This method injects Coin3D nodes into the 'FlatRoot' container to map a texture onto the
            covering, with one texture image per tile.

            Implementation details:
              - Target node: The method targets the 'FlatRoot' SoSeparator, which is reused by
                FreeCAD to render face geometry in both "Flat Lines" and "Shaded" display modes.
              - Rendering contexts: The rendering context for 'FlatRoot' differs between display
                modes, requiring separate logic:
                - "Flat Lines": Renders as part of a composite view alongside other nodes (e.g.,
                  NormalRoot). This context requires texture coordinates to be transformed into the
                  object's Local Space.
                - "Shaded": Renders 'FlatRoot' in isolation. This context requires texture
                  coordinates to be in Global Space.
              - Material Override: For "Shaded" mode, the texture's blend model is set to REPLACE to
                override the mode's default SoMaterial, which would otherwise wash out the texture.

            Texture mapping:
              - A SoTextureCoordinatePlane node defines the texture projection.
              - The mapping period is (TileLength + JointWidth) to ensure the texture repeats in
                sync with the tile grid, preventing drift.
              - A SoTexture2Transform node aligns the texture's origin with the calculated grid
                origin to respect the TileAlignment property.
            """
            vobj = obj.ViewObject
            if not vobj or not vobj.RootNode:
                return

            # Lazy Initialization (safe restore from file)
            if not hasattr(self, "texture"):
                self.texture = None

            # If we are loading the file and "Shape" loaded before "TextureImage", vobj.TextureImage
            # will raise AttributeError. We simply return. The loader will call updateData again
            # when it reaches "TextureImage".
            if not hasattr(vobj, "TextureImage") or not hasattr(vobj, "TextureScale"):
                return

            # Also check for value validity if needed
            if not vobj.TextureImage:
                return

            import pivy.coin as coin
            from draftutils import gui_utils

            # Find the single, correct target node: 'FlatRoot'
            switch = gui_utils.find_coin_node(vobj.RootNode, coin.SoSwitch)
            if not switch:
                return

            target_node = None
            for i in range(switch.getNumChildren()):
                child = switch.getChild(i)
                if child.getName().getString() == "FlatRoot":
                    target_node = child
                    break

            if not target_node:
                return

            # Clean up existing texture nodes from FlatRoot
            for i in range(target_node.getNumChildren() - 1, -1, -1):
                child = target_node.getChild(i)
                if isinstance(
                    child,
                    (coin.SoTexture2, coin.SoTextureCoordinatePlane, coin.SoTexture2Transform),
                ):
                    target_node.removeChild(i)

            self.texture = None

            # Load texture
            if not vobj.TextureImage or not os.path.exists(vobj.TextureImage):
                return

            # Geometry calculation (in Global Space, done once)
            base_face = obj.Proxy.get_base_face(obj)
            if not base_face:
                return

            u_vec, v_vec, normal, center_point = obj.Proxy._get_grid_basis(base_face)
            origin = obj.Proxy._get_grid_origin(obj, base_face, u_vec, v_vec, center_point)

            # Apply different logic based on the active DisplayMode, respecting the different
            # rendering contexts.
            if vobj.DisplayMode == "Flat Lines":
                # "Flat Lines" mode requires a Global-to-Local transform.
                inv_pl = obj.Placement.inverse()
                calc_u = inv_pl.Rotation.multVec(u_vec)
                calc_v = inv_pl.Rotation.multVec(v_vec)
                calc_norm = inv_pl.Rotation.multVec(normal)
                calc_origin = inv_pl.multVec(origin)
            elif vobj.DisplayMode == "Shaded":
                # "Shaded" mode requires Global coordinates (no transform).
                calc_u = u_vec
                calc_v = v_vec
                calc_norm = normal
                calc_origin = origin
            else:
                # For any other mode, do nothing.
                return

            # Common math logic (applied to the transformed vectors)
            if calc_u.Length < Part.Precision.approximation():
                calc_u = FreeCAD.Vector(1, 0, 0)
            else:
                calc_u.normalize()

            if calc_v.Length < Part.Precision.approximation():
                calc_v = FreeCAD.Vector(0, 1, 0)
            else:
                calc_v.normalize()

            if calc_norm.Length > Part.Precision.approximation():
                calc_norm.normalize()

            if hasattr(obj, "Rotation") and obj.Rotation.Value != 0:
                rot = FreeCAD.Rotation(calc_norm, obj.Rotation.Value)
                calc_u = rot.multVec(calc_u)
                calc_v = rot.multVec(calc_v)

            scale_u = vobj.TextureScale.x if vobj.TextureScale.x != 0 else 1.0
            scale_v = vobj.TextureScale.y if vobj.TextureScale.y != 0 else 1.0

            period_u = (obj.TileLength.Value + obj.JointWidth.Value) * scale_u
            period_v = (obj.TileWidth.Value + obj.JointWidth.Value) * scale_v

            if period_u == 0:
                period_u = 1000.0
            if period_v == 0:
                period_v = 1000.0

            dir_u = calc_u.multiply(1.0 / period_u)
            dir_v = calc_v.multiply(1.0 / period_v)

            s_offset = calc_origin.dot(dir_u)
            t_offset = calc_origin.dot(dir_v)

            # Create and insert nodes
            texture_node = coin.SoTexture2()

            # Load texture from cache or disk
            file_path = vobj.TextureImage
            img = _ViewProviderCovering._texture_cache.get(file_path)
            if img is None:
                img = gui_utils.load_texture(file_path)
                if img:
                    _ViewProviderCovering._texture_cache[file_path] = img

            if img:
                texture_node.image = img
            else:
                texture_node.filename = vobj.TextureImage

            # Use REPLACE for Shaded mode to override the default material
            if vobj.DisplayMode == "Shaded":
                texture_node.model = coin.SoTexture2.REPLACE

            texcoords = coin.SoTextureCoordinatePlane()
            texcoords.directionS.setValue(coin.SbVec3f(dir_u.x, dir_u.y, dir_u.z))
            texcoords.directionT.setValue(coin.SbVec3f(dir_v.x, dir_v.y, dir_v.z))

            textrans = coin.SoTexture2Transform()
            textrans.translation.setValue(-s_offset, -t_offset)

            target_node.insertChild(texture_node, 0)
            target_node.insertChild(texcoords, 0)
            target_node.insertChild(textrans, 0)

    class ArchCoveringTaskPanel:
        """
        A Task Panel for creating and editing Arch Covering objects.

        This class manages user input and property states during the object creation lifecycle.

        Parameters
        ----------
        command : object, optional
            The FreeCAD command instance that invoked this panel.
        obj : App.DocumentObject, optional
            The existing Covering object to edit. If None, the panel operates in creation mode.
        selection : list, optional
            A list of pre-selected objects or sub-elements to apply the covering to.

        Notes
        -----
        In creation mode (`obj` is None), this class creates a temporary, hidden
        `App::FeaturePython` object (the "phantom") within the active document. This phantom object
        serves as a proxy data container. It allows the UI widgets to bind to actual FreeCAD
        properties, enabling native features like:

        *   Unit conversion and display.
        *   The Expression Engine (f(x) support).
        *   Default value management via user parameters.

        Data flow and binding:

        1.  **Initialization:** UI widgets (`QuantitySpinBox`, etc.) are initialized. If in creation
            mode, they bind to the phantom's properties. If in edit mode, they bind to the real
            object's properties.
        2.  **Synchronization:** while `ExpressionBinding` handles the visual indication of
            expressions, value propagation is explicitly managed. The `_sync_ui_to_target()` method
            pulls "rawValue" data from widgets and updates the target object (phantom or real)
            before any logic uses it.
        3.  **Transfer:**
            *   *Creation:* When the user accepts the dialog, the properties are read from the fully
                configured phantom object and copied (via `_transfer_props`) to the newly
                instantiated real Covering objects.
            *   *Edition:* The UI writes directly to the existing object (via sync), so no transfer
                step is required.
        """

        def __init__(self, command=None, obj=None, selection=None):
            self.command = command
            self.obj_to_edit = obj
            self.phantom = None
            self.target_obj = None
            self.selected_obj = None
            self.selected_sub = None
            # Keep references to ExpressionBinding objects to prevent garbage collection
            self.bindings = []

            # Handle selection list or single object
            if selection and not isinstance(selection, list):
                self.selection_list = [selection]
            else:
                self.selection_list = selection if selection else []

            # Determine if we are in edit mode or creation mode
            if self.obj_to_edit:
                self.target_obj = self.obj_to_edit
                FreeCAD.ActiveDocument.openTransaction("Edit Covering")
            else:
                FreeCAD.ActiveDocument.openTransaction("Create Covering")
                # Create a lightweight phantom object to hold properties for the UI
                self.phantom = FreeCAD.ActiveDocument.addObject(
                    "App::FeaturePython", "CoveringSettings"
                )
                _Covering(self.phantom)
                if self.phantom.ViewObject:
                    self.phantom.ViewObject.hide()
                self.target_obj = self.phantom

            # Smart Face Detection for pre-selection
            resolved_selection = []
            view_dir = self._get_view_direction()

            for item in self.selection_list:
                if not isinstance(item, tuple):
                    # Access via Proxy of the target object (phantom or real)
                    # This ensures we use the logic defined in _Covering
                    face = self.target_obj.Proxy.get_best_face(item, view_dir)
                    if face:
                        resolved_selection.append((item, [face]))
                    else:
                        resolved_selection.append(item)
                else:
                    resolved_selection.append(item)

            self.selection_list = resolved_selection

            # Build the task panel UI

            # Task Box 1: Geometry
            self.geo_widget = QtGui.QWidget()
            self.geo_widget.setWindowTitle(translate("Arch", "Geometry"))
            self.geo_layout = QtGui.QVBoxLayout(self.geo_widget)

            # Register selection observer
            FreeCADGui.Selection.addObserver(self)

            # Ensure observer is removed when the UI is destroyed (e.g. forced close)
            self.geo_widget.destroyed.connect(self._unregister_observer)

            self._setupTopControls()
            self._setupGeometryStack()
            self._setupBottomControls()

            # Task Box 2: Visuals
            self.vis_widget = QtGui.QWidget()
            self.vis_widget.setWindowTitle(translate("Arch", "Visuals"))
            self._setupVisualUI()

            self.form = [self.geo_widget, self.vis_widget]

            # Handle defaults and initial state
            if self.obj_to_edit:
                self.stored_thickness = self.obj_to_edit.TileThickness.Value
                self._loadExistingData()
                if not FreeCAD.ActiveDocument.HasPendingTransaction:
                    FreeCAD.ActiveDocument.openTransaction("Edit Covering")
            else:
                self.stored_thickness = params.get_param_arch("CoveringThickness")
                # Note: widget defaults are loaded in _setupTilesPage

        def _updateSelectionUI(self):
            """Updates the selection text and tooltip based on current selection state."""
            if self.obj_to_edit:
                # Edit mode: Use interactive selection if available, otherwise fall back to
                # object property
                if self.selected_obj:
                    if self.selected_sub:
                        text = f"{self.selected_obj.Label}.{self.selected_sub}"
                    else:
                        text = self.selected_obj.Label
                else:
                    # Initial load: read from the actual object's Base link
                    base_link = self.obj_to_edit.Base
                    if base_link:
                        if isinstance(base_link, tuple):
                            if base_link[1]:
                                text = f"{base_link[0].Label}.{base_link[1][0]}"
                            else:
                                text = base_link[0].Label
                        else:
                            text = base_link.Label
                    else:
                        text = translate("Arch", "No selection")

                self.le_selection.setText(text)
                self.le_selection.setToolTip(text)
                return

            # Creation mode (batch target list)
            if not self.selection_list:
                self.le_selection.setText(translate("Arch", "No selection"))
                self.le_selection.setToolTip(translate("Arch", "The selected object or face"))
                return

            # Analyze list to determine appropriate label
            unique_objects = set()
            total_faces = 0
            tooltip_items = []

            for item in self.selection_list:
                if isinstance(item, tuple):
                    obj, sub = item
                    unique_objects.add(obj.Name)
                    total_faces += len(sub)
                    tooltip_items.append(f"{obj.Label}.{sub[0]}")
                else:
                    unique_objects.add(item.Name)
                    tooltip_items.append(item.Label)

            count = len(self.selection_list)

            if count == 1:
                # Explicitly display label for single selection (Whole or Face)
                # This prevents "1 objects selected"
                self.le_selection.setText(tooltip_items[0])

            elif len(unique_objects) == 1 and total_faces > 0:
                # Multiple faces on the same object (e.g. Wall (3 faces))
                first_item = self.selection_list[0]
                obj_label = (
                    first_item[0].Label if isinstance(first_item, tuple) else first_item.Label
                )
                self.le_selection.setText(translate("Arch", f"{obj_label} ({total_faces} faces)"))

            else:
                # Multiple distinct objects or complex mixed selection
                self.le_selection.setText(translate("Arch", f"{count} objects selected"))

            # Show specific elements in the tooltip
            self.le_selection.setToolTip(", ".join(tooltip_items))

        def _setupTopControls(self):
            top_form = QtGui.QFormLayout()

            # Selection
            h_sel = QtGui.QHBoxLayout()
            self.le_selection = QtGui.QLineEdit()
            self.le_selection.setPlaceholderText(translate("Arch", "No selection"))
            self.le_selection.setEnabled(False)
            self.le_selection.setToolTip(translate("Arch", "The selected object or face"))

            self.btn_selection = QtGui.QPushButton(translate("Arch", "Pick"))
            self.btn_selection.setCheckable(True)
            self.btn_selection.setToolTip(translate("Arch", "Enable picking in the 3D view"))

            # Use the smart label helper to populate initial state
            self._updateSelectionUI()

            h_sel.addWidget(self.le_selection)
            h_sel.addWidget(self.btn_selection)
            top_form.addRow(translate("Arch", "Base:"), h_sel)

            # Mode
            self.combo_mode = QtGui.QComboBox()
            self.combo_mode.addItems(
                [
                    translate("Arch", "Solid Tiles"),
                    translate("Arch", "Parametric Pattern"),
                    translate("Arch", "Hatch Pattern"),
                ]
            )
            self.combo_mode.setToolTip(translate("Arch", "The type of finish to create"))
            self.combo_mode.setCurrentText(self.target_obj.FinishMode)
            self.combo_mode.currentIndexChanged.connect(self.onModeChanged)
            top_form.addRow(translate("Arch", "Mode:"), self.combo_mode)

            self.geo_layout.addLayout(top_form)

        def _setupGeometryStack(self):
            self.geo_stack = QtGui.QStackedWidget()

            self._setupTilesPage()
            self._setupHatchPage()

            self.geo_layout.addWidget(self.geo_stack)

        def _get_view_direction(self):
            """Safely retrieve the view direction if the GUI is active."""
            if FreeCAD.GuiUp and FreeCADGui.ActiveDocument:
                view = FreeCADGui.ActiveDocument.ActiveView
                if hasattr(view, "getViewDirection"):
                    return view.getViewDirection()
            return None

        def _setupBottomControls(self):
            # Continue Mode
            self.chk_continue = QtGui.QCheckBox(translate("Arch", "Continue"))
            self.chk_continue.setToolTip(
                translate(
                    "Arch",
                    "If checked, the dialog stays open after creating the covering, "
                    "allowing to pick another face",
                )
            )
            self.chk_continue.setShortcut(QtGui.QKeySequence("N"))

            if self.obj_to_edit:
                self.chk_continue.hide()
            else:
                self.chk_continue.show()
                self.chk_continue.setChecked(False)

            # Add to the main layout below the stack
            self.geo_layout.addWidget(self.chk_continue)

        # Helper for binding properties to a quantity spinbox with default
        def _setup_bound_spinbox(self, prop_name, tooltip):
            sb = FreeCADGui.UiLoader().createWidget("Gui::QuantitySpinBox")
            prop = getattr(self.target_obj, prop_name)
            sb.setProperty("unit", prop.getUserPreferred()[2])
            sb.setToolTip(translate("Arch", tooltip))

            # This enables the f(x) icon, but we don't rely on it for value syncing anymore
            FreeCADGui.ExpressionBinding(sb).bind(self.target_obj, prop_name)

            sb.setProperty("rawValue", prop.Value)
            return sb

        def _setupTilesPage(self):
            self.page_tiles = QtGui.QWidget()
            form = QtGui.QFormLayout()

            self.sb_length = self._setup_bound_spinbox(
                "TileLength", translate("Arch", "The length of the tiles")
            )
            form.addRow(translate("Arch", "Length:"), self.sb_length)

            self.sb_width = self._setup_bound_spinbox(
                "TileWidth", translate("Arch", "The width of the tiles")
            )
            form.addRow(translate("Arch", "Width:"), self.sb_width)

            self.sb_thick = self._setup_bound_spinbox(
                "TileThickness", translate("Arch", "The thickness of the tiles")
            )
            # Specify label so that we can refer to it later when switching thickness value and
            # status when changing finish modes
            self.lbl_thick = QtGui.QLabel(translate("Arch", "Thickness:"))
            form.addRow(self.lbl_thick, self.sb_thick)

            self.sb_joint = self._setup_bound_spinbox(
                "JointWidth", translate("Arch", "The width of the joints between tiles")
            )
            form.addRow(translate("Arch", "Joint:"), self.sb_joint)

            self.combo_align = QtGui.QComboBox()
            self.combo_align.addItems(
                ["Center", "TopLeft", "TopRight", "BottomLeft", "BottomRight"]
            )
            self.combo_align.setToolTip(translate("Arch", "The alignment of the tile grid"))
            self.combo_align.setCurrentText(self.target_obj.TileAlignment)
            form.addRow(translate("Arch", "Alignment:"), self.combo_align)

            self.sb_rot = self._setup_bound_spinbox(
                "Rotation", translate("Arch", "Rotation of the finish")
            )
            self.sb_rot.lineEdit().returnPressed.connect(self.accept)
            form.addRow(translate("Arch", "Rotation:"), self.sb_rot)

            self.page_tiles.setLayout(form)
            self.geo_stack.addWidget(self.page_tiles)

        def _setupHatchPage(self):
            self.page_hatch = QtGui.QWidget()
            form = QtGui.QFormLayout()

            self.le_pat = QtGui.QLineEdit()
            self.le_pat.setToolTip(translate("Arch", "The PAT file to use for hatching"))

            btn_browse_pat = QtGui.QPushButton("...")
            btn_browse_pat.clicked.connect(self.browsePattern)
            h_pat = QtGui.QHBoxLayout()
            h_pat.addWidget(self.le_pat)
            h_pat.addWidget(btn_browse_pat)
            form.addRow(translate("Arch", "Pattern File:"), h_pat)

            self.sb_rot_hatch = self._setup_bound_spinbox(
                "Rotation", translate("Arch", "Rotation of the hatch pattern")
            )
            self.sb_rot_hatch.lineEdit().returnPressed.connect(self.accept)
            form.addRow(translate("Arch", "Rotation:"), self.sb_rot_hatch)

            self.page_hatch.setLayout(form)
            self.geo_stack.addWidget(self.page_hatch)

        def _setupVisualUI(self):
            visual_form = QtGui.QFormLayout(self.vis_widget)
            self.le_tex_image = QtGui.QLineEdit()
            self.le_tex_image.setToolTip(translate("Arch", "An image file to map onto each tile"))
            btn_browse = QtGui.QPushButton("...")
            btn_browse.clicked.connect(self.browseTexture)

            h_tex = QtGui.QHBoxLayout()
            h_tex.addWidget(self.le_tex_image)
            h_tex.addWidget(btn_browse)
            visual_form.addRow(translate("Arch", "Texture Image:"), h_tex)

        def _loadExistingData(self):
            mode = self.obj_to_edit.FinishMode
            self.combo_mode.setCurrentText(mode)
            self.onModeChanged(self.combo_mode.currentIndex())

            # Numerical values are auto-loaded by ExpressionBinding
            self.combo_align.setCurrentText(self.obj_to_edit.TileAlignment)

            self.le_pat.setText(self.obj_to_edit.PatternFile)

            if hasattr(self.obj_to_edit.ViewObject, "TextureImage"):
                self.le_tex_image.setText(self.obj_to_edit.ViewObject.TextureImage)

        def browseTexture(self):
            fn = QtGui.QFileDialog.getOpenFileName(
                self.vis_widget,
                translate("Arch", "Select Texture"),
                "",
                "Images (*.png *.jpg *.jpeg *.bmp)",
            )[0]
            if fn:
                self.le_tex_image.setText(fn)

        def browsePattern(self):
            fn = QtGui.QFileDialog.getOpenFileName(
                self.geo_widget, translate("Arch", "Select Pattern"), "", "Pattern files (*.pat)"
            )[0]
            if fn:
                self.le_pat.setText(fn)

        def onModeChanged(self, index):
            if index == 2:  # Hatch
                self.geo_stack.setCurrentIndex(1)
                self.vis_widget.setEnabled(False)
            else:  # Tiles (solid or parametric)
                self.geo_stack.setCurrentIndex(0)
                self.vis_widget.setEnabled(index == 0)  # Only enable visual for solid tiles

                if index == 0:  # Solid tiles mode
                    self.sb_thick.setEnabled(True)
                    # Restore stored thickness
                    self.sb_thick.setProperty("rawValue", self.stored_thickness)
                    if self.obj_to_edit:
                        self.obj_to_edit.TileThickness = self.stored_thickness

                else:  # Parametric pattern mode
                    # Save current thickness before zeroing
                    if self.sb_thick.isEnabled():
                        self.stored_thickness = self.sb_thick.property("rawValue")

                    # Disable and zero
                    self.sb_thick.setEnabled(False)
                    self.sb_thick.setProperty("rawValue", 0.0)

                    if self.obj_to_edit:
                        self.obj_to_edit.TileThickness = 0.0

            if self.obj_to_edit:
                self.obj_to_edit.FinishMode = self.combo_mode.currentText()

        def isPicking(self):
            return self.btn_selection.isChecked()

        def setPicking(self, state):
            self.btn_selection.setChecked(state)
            if state:
                # Sync immediately when enabled
                self._onSelectionChanged()

        # --- Selection Observer methods ---
        def addSelection(self, doc, obj, sub, pos):
            self._onSelectionChanged()

        def removeSelection(self, doc, obj, sub):
            self._onSelectionChanged()

        def setSelection(self, doc):
            self._onSelectionChanged()

        def clearSelection(self, doc):
            self._onSelectionChanged()

        def _onSelectionChanged(self):
            """Syncs internal state with FreeCAD selection."""
            # Gate: Only update if Picking is enabled
            if not self.isPicking():
                return

            # Get Standard Selection
            sel = FreeCADGui.Selection.getSelectionEx()

            # Process into internal list structure
            new_list = []
            for s in sel:
                obj = s.Object

                # If the user picks the covering itself, ignore it to avoid circular references.
                if obj == self.phantom or obj == self.obj_to_edit:
                    continue

                # PartDesign Normalization
                for parent in obj.InList:
                    if parent.isDerivedFrom("PartDesign::Body"):
                        # Check if obj is part of the Body's geometry definition
                        # Group contains features (Pad, Pocket), BaseFeature contains the root
                        if (obj in parent.Group) or (getattr(parent, "BaseFeature", None) == obj):
                            obj = parent
                            break

                if s.SubElementNames:
                    for sub in s.SubElementNames:
                        if "Face" in sub:
                            new_list.append((obj, [sub]))
                else:
                    # Smart detection for whole object clicks
                    view_dir = self._get_view_direction()
                    best_face = self.target_obj.Proxy.get_best_face(obj, view_dir)

                    if best_face:
                        new_list.append((obj, [best_face]))
                    else:
                        new_list.append(obj)

            # If we are editing, we can only assign one base. If the user selects multiple,
            # we keep only the most recent one to match the behavior of the assignment logic below.
            if self.obj_to_edit and new_list:
                new_list = [new_list[-1]]

            self.selection_list = new_list

            # Handle edit mode assignment
            if self.obj_to_edit and self.selection_list:
                last_item = self.selection_list[-1]
                if isinstance(last_item, tuple):
                    self.selected_obj = last_item[0]
                    self.selected_sub = last_item[1][0]
                    self.obj_to_edit.Base = (self.selected_obj, [self.selected_sub])
                else:
                    self.selected_obj = last_item
                    self.selected_sub = None
                    self.obj_to_edit.Base = self.selected_obj

            # Update UI label
            self._updateSelectionUI()

        def updateBase(self):
            # Update the Base property of the live object
            if self.obj_to_edit:
                if self.selected_sub:
                    self.obj_to_edit.Base = (self.selected_obj, [self.selected_sub])
                else:
                    self.obj_to_edit.Base = self.selected_obj

        def getStandardButtons(self):
            return QtGui.QDialogButtonBox.Ok | QtGui.QDialogButtonBox.Cancel

        def _unregister_observer(self, val=None):
            """Safely remove self from selection observers."""
            try:
                FreeCADGui.Selection.removeObserver(self)
            except Exception:
                # Observer might already be removed, ignore
                pass

        def _cleanup_and_close(self):
            """Removes temporary oobservers, then closes the task panel."""
            # Standard cleanup for listeners and UI
            self._unregister_observer()
            FreeCADGui.Control.closeDialog()

        def _transfer_props(self, source, target, props):
            """Copies values and expressions from the source object to the target."""
            for prop in props:
                # Set the static value first
                setattr(target, prop, getattr(source, prop))

                # Check for and transfer expressions
                # Expressions are stored in the ExpressionEngine property as [(path, expr), ...]
                if hasattr(source, "ExpressionEngine"):
                    for path, expr in source.ExpressionEngine:
                        if path == prop:
                            target.setExpression(prop, expr)
                            break

        def _save_user_preferences(self):
            """
            Save the current settings to the user parameter storage.

            This ensures that the next time the tool is used, it defaults to the last-used values.
            """
            params.set_param_arch("CoveringFinishMode", self.target_obj.FinishMode)
            params.set_param_arch("CoveringAlignment", self.target_obj.TileAlignment)
            params.set_param_arch("CoveringRotation", self.target_obj.Rotation.Value)

            if self.target_obj.FinishMode != "Hatch Pattern":
                params.set_param_arch("CoveringLength", self.target_obj.TileLength.Value)
                params.set_param_arch("CoveringWidth", self.target_obj.TileWidth.Value)
                params.set_param_arch("CoveringJoint", self.target_obj.JointWidth.Value)
                if self.target_obj.FinishMode == "Solid Tiles":
                    params.set_param_arch("CoveringThickness", self.target_obj.TileThickness.Value)

        def _sync_ui_to_target(self):
            """
            Manually transfers values from all UI widgets to the target object.

            This ensures that the target object (whether Phantom or Real) reflects
            the state of the dialog before any logic uses it.
            """
            obj = self.target_obj

            # Sync numeric properties directly from the widget property. Use "rawValue" to get the
            # float value, ignoring unit strings
            obj.TileLength = self.sb_length.property("rawValue")
            obj.TileWidth = self.sb_width.property("rawValue")

            # Handle conditional properties
            if self.sb_thick.isEnabled():
                obj.TileThickness = self.sb_thick.property("rawValue")

            obj.JointWidth = self.sb_joint.property("rawValue")

            # Handle rotation based on active mode
            if self.combo_mode.currentText() == "Hatch Pattern":
                obj.Rotation = self.sb_rot_hatch.property("rawValue")
            else:
                obj.Rotation = self.sb_rot.property("rawValue")

            # Sync enum properties
            obj.FinishMode = self.combo_mode.currentText()
            obj.TileAlignment = self.combo_align.currentText()

            # Sync file paths
            if obj.FinishMode == "Hatch Pattern":
                obj.PatternFile = self.le_pat.text()

        def accept(self):
            """
            Process the dialog input and modify or create the Covering object.

            This method handles data synchronization from UI widgets to the underlying FreeCAD
            objects, manages the undo/redo transaction, and handles the 'continue' workflow for
            batch creation.

            Returns
            -------
            bool
                Always returns True to signal the dialog to close (unless 'continue'
                is checked).

            Notes
            -----
            1. Transaction management is manual here. This method opens a transaction explicitly.
               Any early `return` statement added within the `try` block must ensure the transaction
               is committed or aborted first, otherwise it will block the undo stack.
            2. This method is connected to a Qt signal. Uncaught exceptions are frequently
               suppressed by the Python/Qt bridge. We use `traceback.print_exc()` to ensure the full
               stack trace appears in FreeCAD's Report View for notification purposes.
            """
            try:
                # Sync all values from UI widgets (bound and unbound) to the target object
                self._sync_ui_to_target()

                # Define list of properties to transfer from phantom to real object.
                # This ensures we copy only the configuration specific to the chosen mode
                props_to_transfer = ["Rotation", "FinishMode", "TileAlignment"]
                if self.combo_mode.currentText() == "Hatch Pattern":
                    props_to_transfer.extend(["PatternFile", "PatternName", "PatternScale"])
                else:
                    props_to_transfer.extend(
                        ["TileLength", "TileWidth", "JointWidth", "TileOffset"]
                    )
                    if self.combo_mode.currentText() == "Solid Tiles":
                        props_to_transfer.append("TileThickness")

                # Prepare visual properties
                tex_image = self.le_tex_image.text()

                if not self.obj_to_edit:
                    # Creation mode
                    import Arch

                    # Determine targets from selection
                    targets = self.selection_list
                    if not targets and self.selected_obj:
                        targets = (
                            [(self.selected_obj, [self.selected_sub])]
                            if self.selected_sub
                            else [self.selected_obj]
                        )

                    if targets:
                        for base in targets:
                            # Create new object
                            new_obj = Arch.makeCovering(base)

                            # Copy properties from the synchronized phantom object
                            self._transfer_props(self.target_obj, new_obj, props_to_transfer)

                            # Apply texture to view object
                            if hasattr(new_obj.ViewObject, "TextureImage"):
                                new_obj.ViewObject.TextureImage = tex_image
                else:
                    # Edition mode
                    # The properties on self.target_obj (which is self.obj_to_edit) are already
                    # updated by _sync_ui_to_target call above.

                    # Apply texture to view object
                    if hasattr(self.target_obj.ViewObject, "TextureImage"):
                        self.target_obj.ViewObject.TextureImage = tex_image

                # Recompute the document inside the transaction to catch geometry errors
                FreeCAD.ActiveDocument.recompute()

                # Save user preferences for next run
                self._save_user_preferences()

                # Handle the 'continue' workflow
                if not self.obj_to_edit and self.chk_continue.isChecked():
                    FreeCADGui.Selection.clearSelection()
                    self.selection_list = []
                    self.selected_obj = None
                    self.selected_sub = None
                    self._updateSelectionUI()
                    self.setPicking(True)
                    return False

                # Remove phantom before commit to keep it out of permanent history
                if self.phantom:
                    doc = getattr(self.phantom, "Document", None)
                    if doc and doc.getObject(self.phantom.Name):
                        doc.removeObject(self.phantom.Name)
                    self.phantom = None

                # Commit the transaction successfully
                FreeCAD.ActiveDocument.commitTransaction()

            except Exception as e:
                # Ensure transaction is closed on failure to prevent corruption
                FreeCAD.ActiveDocument.abortTransaction()
                import traceback

                traceback.print_exc()
                FreeCAD.Console.PrintError(f"Error updating covering: {e}\n")

            if self.obj_to_edit:
                FreeCADGui.ActiveDocument.resetEdit()

            self._cleanup_and_close()
            return True

        def reject(self):
            if FreeCAD.ActiveDocument.HasPendingTransaction:
                FreeCAD.ActiveDocument.abortTransaction()

            if self.obj_to_edit:
                FreeCADGui.ActiveDocument.resetEdit()

            self._cleanup_and_close()
