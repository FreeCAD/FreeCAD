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
import Part
import ArchComponent
from draftutils import params

if FreeCAD.GuiUp:
    translate = FreeCAD.Qt.translate
    QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
else:

    def translate(context, sourceText, disambiguation=None, n=-1):
        return sourceText

    def QT_TRANSLATE_NOOP(context, sourceText):
        return sourceText


# Anything smaller than this is considered effectively zero to prevent numerical instability
# in the geometry calculations or division-by-zero errors in Python.
MIN_DIMENSION = 0.1
TOO_MANY_TILES = 10000
# Minimum percentage of a tile's area/volume to be considered "full" for quantity take-off, to
# account for minor clipping from forced joint widths.
FULL_TILE_THRESHOLD_RATIO = 0.995


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
            ("App::PropertyLength", "JointWidth", "Tiles", "The width of the joints", None),
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

    def onChanged(self, obj, prop):
        """Method called when a property is changed."""
        ArchComponent.Component.onChanged(self, obj, prop)
        if prop == "JointWidth":
            if obj.JointWidth.Value < MIN_DIMENSION:
                obj.JointWidth = MIN_DIMENSION
                FreeCAD.Console.PrintWarning(
                    translate(
                        "Arch",
                        f"Covering: The joint width has been adjusted to {MIN_DIMENSION} mm. "
                        "A minimum width is required to divide the finish into individual tiles.",
                    )
                    + "\n"
                )

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
        if j_wid >= MIN_DIMENSION:
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
        if j_len >= MIN_DIMENSION:
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
                        if sol.Volume >= (full_vol * FULL_TILE_THRESHOLD_RATIO):
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
                        if f.Area >= (full_area * FULL_TILE_THRESHOLD_RATIO):
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
        if obj.JointWidth.Value < MIN_DIMENSION:
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
