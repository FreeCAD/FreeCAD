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


class _Covering(ArchComponent.Component):
    """
    A parametric object representing an architectural surface finish.

    This class manages the generation of surface treatments such as tiles,
    panels, flooring, or hatch patterns. Coverings are typically linked to a
    specific face of a base object and remain parametric, updating
    automatically if the underlying geometry changes.

    Parameters
    ----------
    obj : App::FeaturePython
        The base C++ object to be initialized as a Covering.

    Notes
    -----
    While the standard FreeCAD `TypeId` attribute identifies the underlying
    C++ class, the `Type` attribute is used by the Arch module to distinguish
    functional object types that share the same C++ implementation.

    Example
    -------
    >>> print(obj.TypeId, "->", obj.Proxy.Type)
    Part::FeaturePython -> Covering
    """

    def __init__(self, obj):
        super().__init__(obj)
        self.Type = "Covering"
        self.setProperties(obj)

        # Apply defaults from params immediately upon creation
        # This allows the UI to simply bind to these properties and show the correct default values
        obj.TileLength = params.get_param_arch("CoveringLength", 300.0)
        obj.TileWidth = params.get_param_arch("CoveringWidth", 300.0)
        obj.TileThickness = params.get_param_arch("CoveringThickness", 10.0)
        obj.JointWidth = params.get_param_arch("CoveringJoint", 5.0)

    def setProperties(self, obj):
        """
        Defines and initializes the properties of the Covering object.

        This method is a Python-side helper that manages the object's parametric structure. It is
        responsible for creating new properties unique to this class and performing
        version-migration or type-overrides on inherited properties.

        Parameters
        ----------
        obj : Part::FeaturePython
            The base C++ object to which the properties are added.

        Notes
        -----
        This method is not a C++ callback; it must be invoked manually during initialization
        (`__init__`) and document restoration (`onDocumentRestored`).
        """
        # The parent class (ArchComponent) defines 'Base' as a whole-object link. Coverings require
        # a sub-element link (LinkSub) to target specific faces. Since FreeCAD property types are
        # immutable once created, we must detect and replace the generic version to support
        # face-level finish applications.
        if "Base" in obj.PropertiesList:
            if obj.getTypeIdOfProperty("Base") != "App::PropertyLinkSub":
                obj.setPropertyStatus("Base", "-LockDynamic")
                obj.removeProperty("Base")

        properties_list = obj.PropertiesList

        # Core Properties
        if not "Base" in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyLinkSub",
                "Base",
                "Covering",
                QT_TRANSLATE_NOOP(
                    "App::Property", "The object or face this covering is applied to"
                ),
                locked=True,
            )
        if not "FinishMode" in properties_list:
            obj.addProperty(
                "App::PropertyEnumeration",
                "FinishMode",
                "Covering",
                QT_TRANSLATE_NOOP("App::Property", "The type of finish to create"),
                locked=True,
            )
            obj.FinishMode = ["Solid Tiles", "Parametric Pattern", "Hatch Pattern"]

        # Alignment and rotation
        if not "TileAlignment" in properties_list:
            obj.addProperty(
                "App::PropertyEnumeration",
                "TileAlignment",
                "Covering",
                QT_TRANSLATE_NOOP("App::Property", "The alignment of the tile grid"),
                locked=True,
            )
            obj.TileAlignment = ["Center", "TopLeft", "TopRight", "BottomLeft", "BottomRight"]

        if not "Rotation" in properties_list:
            obj.addProperty(
                "App::PropertyAngle",
                "Rotation",
                "Covering",
                QT_TRANSLATE_NOOP(
                    "App::Property", "Rotation of the finish (tiles, pattern, texture)"
                ),
                locked=True,
            )

        # Tile properties
        if not "TileLength" in properties_list:
            obj.addProperty(
                "App::PropertyLength",
                "TileLength",
                "Tiles",
                QT_TRANSLATE_NOOP("App::Property", "The length of the tiles"),
                locked=True,
            )
        if not "TileWidth" in properties_list:
            obj.addProperty(
                "App::PropertyLength",
                "TileWidth",
                "Tiles",
                QT_TRANSLATE_NOOP("App::Property", "The width of the tiles"),
                locked=True,
            )
        if not "TileThickness" in properties_list:
            obj.addProperty(
                "App::PropertyLength",
                "TileThickness",
                "Tiles",
                QT_TRANSLATE_NOOP("App::Property", "The thickness of the tiles (Solid mode only)"),
                locked=True,
            )
        if not "JointWidth" in properties_list:
            obj.addProperty(
                "App::PropertyLength",
                "JointWidth",
                "Tiles",
                QT_TRANSLATE_NOOP("App::Property", "The width of the joints between tiles"),
                locked=True,
            )
        if not "TileOffset" in properties_list:
            obj.addProperty(
                "App::PropertyVector",
                "TileOffset",
                "Tiles",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "The offset of alternating rows/columns (e.g. for running bond)",
                ),
                locked=True,
            )

        # Quantity calculation
        if not "CountFullTiles" in properties_list:
            obj.addProperty(
                "App::PropertyInteger",
                "CountFullTiles",
                "Stats",
                QT_TRANSLATE_NOOP("App::Property", "The number of full tiles"),
                locked=True,
            )
            obj.setEditorMode("CountFullTiles", 1)
        if not "CountPartialTiles" in properties_list:
            obj.addProperty(
                "App::PropertyInteger",
                "CountPartialTiles",
                "Stats",
                QT_TRANSLATE_NOOP("App::Property", "The number of cut/partial tiles"),
                locked=True,
            )
            obj.setEditorMode("CountPartialTiles", 1)

        # Pattern properties
        # These are here and not in the view object because while conceptually they modify
        # the visual appearance, they most importantly affect the geometry (shape) of the object.
        if not "PatternFile" in properties_list:
            obj.addProperty(
                "App::PropertyFile",
                "PatternFile",
                "Pattern",
                QT_TRANSLATE_NOOP("App::Property", "The PAT file to use for hatching"),
                locked=True,
            )
        if not "PatternName" in properties_list:
            obj.addProperty(
                "App::PropertyString",
                "PatternName",
                "Pattern",
                QT_TRANSLATE_NOOP("App::Property", "The name of the pattern in the PAT file"),
                locked=True,
            )
        if not "PatternScale" in properties_list:
            obj.addProperty(
                "App::PropertyFloat",
                "PatternScale",
                "Pattern",
                QT_TRANSLATE_NOOP("App::Property", "The scale of the hatch pattern"),
                locked=True,
            )
            obj.PatternScale = 1.0

        # IFC
        # Neither of the ancestors currently define this property, so define it and populate it
        # specifically for coverings
        if not "IfcPredefinedType" in properties_list:
            obj.addProperty(
                "App::PropertyEnumeration",
                "IfcPredefinedType",
                "IFC",
                QT_TRANSLATE_NOOP("App::Property", "The specific type of covering"),
                locked=True,
            )
            obj.IfcPredefinedType = [
                "FLOORING",
                "CLADDING",
                "ROOFING",
                "MOLDING",
                "SKIRTINGBOARD",
                "CEILING",
                "WRAPPING",
                "NOTDEFINED",
            ]

    def loads(self, state):
        # Override the parent's type to set a specific type for Covering
        self.Type = "Covering"

    def onDocumentRestored(self, obj):
        super().onDocumentRestored(obj)
        self.setProperties(obj)

    def get_base_face(self, obj):
        """Extracts the base face from the linked object/subobject"""
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
                # If solid, find the "Top" face based on Normal
                faces = linked_obj.Shape.Faces
                faces.sort(key=lambda f: f.CenterOfMass.z, reverse=True)
                if faces:
                    face = faces[0]
            # Support for closed Wires (e.g. Draft Circle/Rectangle with MakeFace=False)
            elif linked_obj.Shape.ShapeType in ["Wire", "Edge"] and linked_obj.Shape.isClosed():
                try:
                    face = Part.Face(Part.Wire(linked_obj.Shape.Edges))
                except Part.OCCError as e:
                    FreeCAD.Console.PrintWarning(
                        "ArchCovering: Unable to create face from wire: {}\n".format(e)
                    )

        if face:
            # Ensure we are working with a copy in global coordinates
            return face

        return None

    def _get_grid_basis(self, base_face):
        # Determine Grid Basis (u, v directions from face) using Generic Surface API
        u_p, v_p = base_face.Surface.parameter(base_face.BoundBox.Center)
        u_vec, v_vec = base_face.Surface.tangent(u_p, v_p)
        u_vec.normalize()

        # Calculate normal and re-orthogonalize V vector
        normal = u_vec.cross(v_vec)
        normal.normalize()
        v_vec = normal.cross(u_vec)
        v_vec.normalize()

        center_point = base_face.Surface.value(u_p, v_p)
        return u_vec, v_vec, normal, center_point

    def _get_grid_origin(self, obj, base_face, u_vec, v_vec, center_point):
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
            origin_offset = u_vec.multiply(mid_u).add(v_vec.multiply(mid_v))
        elif align == "BottomLeft":
            origin_offset = u_vec.multiply(min_u).add(v_vec.multiply(min_v))
        elif align == "BottomRight":
            origin_offset = u_vec.multiply(max_u).add(v_vec.multiply(min_v))
        elif align == "TopLeft":
            origin_offset = u_vec.multiply(min_u).add(v_vec.multiply(max_v))
        elif align == "TopRight":
            origin_offset = u_vec.multiply(max_u).add(v_vec.multiply(max_v))

        return center_point.add(origin_offset)

    def _build_cutters(self, obj, bbox, t_len, t_wid, j_len, j_wid, cut_thick):
        # Step size
        step_x = t_len + j_len
        step_y = t_wid + j_wid

        if step_x == 0 or step_y == 0:
            return [], []

        # Estimate count
        diag = bbox.DiagonalLength
        count_x = int(diag / step_x) + 4
        count_y = int(diag / step_y) + 4

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

        # Generate Horizontal Strips (Rows)
        full_len_x = 2 * count_x * step_x
        start_x = -count_x * step_x

        for j in range(-count_y, count_y):
            row_y_offset = off_y if (j % 2 != 0) else 0
            local_y = j * step_y + t_wid + row_y_offset
            jh_box = Part.makeBox(
                full_len_x, j_wid, cut_thick, FreeCAD.Vector(start_x, local_y, z_gen_offset)
            )
            cutters_h.append(jh_box)

        # Generate Vertical Strips (Cols)
        is_stack_bond = obj.TileOffset.Length < 1e-5

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

        # Prepare transformation
        tr = FreeCAD.Placement()
        tr.Base = origin
        rot_mat = FreeCAD.Matrix(
            u_vec.x,
            v_vec.x,
            normal.x,
            0,
            u_vec.y,
            v_vec.y,
            normal.y,
            0,
            u_vec.z,
            v_vec.z,
            normal.z,
            0,
            0,
            0,
            0,
            1,
        )
        face_rot = FreeCAD.Placement(rot_mat).Rotation
        tile_rot = FreeCAD.Rotation(FreeCAD.Vector(0, 0, 1), obj.Rotation.Value)
        tr.Rotation = face_rot.multiply(tile_rot)

        result_shape = None
        t_len = obj.TileLength.Value
        t_wid = obj.TileWidth.Value

        if obj.FinishMode == "Solid Tiles":
            # Extrude Base Face
            tile_layer = base_face.extrude(normal.multiply(obj.TileThickness.Value))

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
            full_cnt = 0
            part_cnt = 0
            for sol in result_shape.Solids:
                if abs(sol.Volume - full_vol) < 0.001:
                    full_cnt += 1
                else:
                    part_cnt += 1
            obj.CountFullTiles = full_cnt
            obj.CountPartialTiles = part_cnt

        elif obj.FinishMode == "Parametric Pattern":
            # 2D Cut
            temp_face = base_face

            if cutters_h:
                comp_h = Part.Compound(cutters_h)
                comp_h.Placement = tr
                temp_face = temp_face.cut(comp_h)

            if cutters_v:
                comp_v = Part.Compound(cutters_v)
                comp_v.Placement = tr
                result_shape = temp_face.cut(comp_v)
            else:
                result_shape = temp_face

            # Count
            full_area = t_len * t_wid
            full_cnt = 0
            part_cnt = 0
            for f in result_shape.Faces:
                if abs(f.Area - full_area) < 0.001:
                    full_cnt += 1
                else:
                    part_cnt += 1
            obj.CountFullTiles = full_cnt
            obj.CountPartialTiles = part_cnt

            # Convert Faces to Wires for lightweight representation
            if result_shape:
                wires = []
                for f in result_shape.Faces:
                    wires.extend(f.Wires)
                if wires:
                    result_shape = Part.Compound(wires)

        return result_shape

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
            from draftutils import hatch

            if obj.PatternFile:
                # Use unified Rotation property for Hatch rotation
                pat = hatch.hatch(
                    base_face,
                    obj.PatternFile,
                    obj.PatternName,
                    scale=obj.PatternScale,
                    rotation=obj.Rotation,
                )
                if pat:
                    obj.Shape = Part.Compound([base_face, pat])
            return

        u_vec, v_vec, normal, center_point = self._get_grid_basis(base_face)
        origin = self._get_grid_origin(obj, base_face, u_vec, v_vec, center_point)

        # Dimensions
        t_len = obj.TileLength.Value
        t_wid = obj.TileWidth.Value
        j_len = obj.JointWidth.Value
        j_wid = obj.JointWidth.Value

        # Cut thickness
        cut_thick = obj.TileThickness.Value * 1.1 if obj.FinishMode == "Solid Tiles" else 1.0

        cutters_h, cutters_v = self._build_cutters(
            obj, base_face.BoundBox, t_len, t_wid, j_len, j_wid, cut_thick
        )

        if not cutters_h and not cutters_v:
            return

        obj.Shape = self._perform_cut(
            obj, base_face, cutters_h, cutters_v, normal, origin, u_vec, v_vec
        )


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
            img = gui_utils.load_texture(vobj.TextureImage)
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
        def __init__(self, command=None, obj=None, selection=None):
            self.command = command
            self.obj_to_edit = obj

            # Initialize selection variables before the UI setup
            self.selected_obj = None
            self.selected_sub = None

            # Setup event callback for picking (works for both create and edit)
            # TODO: investigate ArchComponent.SelectionTaskPanel and
            # ArchComponent.ArchSelectionObserver
            self.view = FreeCADGui.ActiveDocument.ActiveView
            self.callback = self.view.addEventCallback("SoEvent", self.action)

            # Pre-fill selection based on obj properties
            if self.obj_to_edit:
                # Edition mode: get selection from object
                if self.obj_to_edit.Base:
                    val = self.obj_to_edit.Base
                    if isinstance(val, tuple):
                        self.selected_obj = val[0]
                        self.selected_sub = val[1][0] if val[1] else None
                    else:
                        self.selected_obj = val
            else:
                # Creation mode: get selection from command snapshot
                if selection:
                    if isinstance(selection, tuple):
                        self.selected_obj = selection[0]
                        self.selected_sub = selection[1][0] if selection[1] else None
                    else:
                        self.selected_obj = selection

            # Build the task panel UI

            # Task Box 1: Geometry
            self.geo_widget = QtGui.QWidget()
            self.geo_widget.setWindowTitle(translate("Arch", "Geometry"))
            self.geo_layout = QtGui.QVBoxLayout(self.geo_widget)

            self._setupTopControls()
            self._setupGeometryStack()

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

            if self.selected_obj:
                if self.selected_sub:
                    self.le_selection.setText(f"{self.selected_obj.Label}.{self.selected_sub}")
                else:
                    self.le_selection.setText(f"{self.selected_obj.Label}")
            h_sel.addWidget(self.le_selection)
            h_sel.addWidget(self.btn_selection)
            top_form.addRow(translate("Arch", "Selection:"), h_sel)

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
            if not self.obj_to_edit:
                # Load default finish mode from preferences in create mode
                self.combo_mode.setCurrentText(params.get_param_arch("CoveringFinishMode"))
            self.combo_mode.currentIndexChanged.connect(self.onModeChanged)
            top_form.addRow(translate("Arch", "Mode:"), self.combo_mode)

            self.geo_layout.addLayout(top_form)

        def _setupGeometryStack(self):
            self.geo_stack = QtGui.QStackedWidget()

            self._setupTilesPage()
            self._setupHatchPage()

            self.geo_layout.addWidget(self.geo_stack)

        def _setupTilesPage(self):
            self.page_tiles = QtGui.QWidget()
            form = QtGui.QFormLayout()
            ui = FreeCADGui.UiLoader()

            # Helper for binding properties to a quantity spinbox with default
            def setup_spinbox(prop_name, param_name):
                sb = ui.createWidget("Gui::QuantitySpinBox")
                unit_str = FreeCAD.Units.Quantity(0, FreeCAD.Units.Length).getUserPreferred()[2]
                # TODO: cleaner alternative, but we net to get the correct obj
                # unit_str = self.obj_to_edit.PropertyGet(prop_name).getUserPreferred()[2]
                sb.setProperty("unit", unit_str)
                if self.obj_to_edit:
                    FreeCADGui.ExpressionBinding(sb).bind(self.obj_to_edit, prop_name)
                    sb.setProperty("rawValue", getattr(self.obj_to_edit, prop_name).Value)
                else:
                    sb.setProperty("rawValue", params.get_param_arch(param_name))
                return sb

            self.sb_length = setup_spinbox("TileLength", "CoveringLength")
            self.sb_length.setToolTip(translate("Arch", "The length of the tiles"))
            form.addRow(translate("Arch", "Length:"), self.sb_length)

            self.sb_width = setup_spinbox("TileWidth", "CoveringWidth")
            self.sb_width.setToolTip(translate("Arch", "The width of the tiles"))
            form.addRow(translate("Arch", "Width:"), self.sb_width)

            self.sb_thick = setup_spinbox("TileThickness", "CoveringThickness")
            self.sb_thick.setToolTip(translate("Arch", "The thickness of the tiles"))
            self.lbl_thick = QtGui.QLabel(translate("Arch", "Thickness:"))
            form.addRow(self.lbl_thick, self.sb_thick)

            self.sb_joint = setup_spinbox("JointWidth", "CoveringJoint")
            self.sb_joint.setToolTip(translate("Arch", "The width of the joints between tiles"))
            form.addRow(translate("Arch", "Joint:"), self.sb_joint)

            self.combo_align = QtGui.QComboBox()
            self.combo_align.addItems(
                ["Center", "TopLeft", "TopRight", "BottomLeft", "BottomRight"]
            )
            self.combo_align.setToolTip(translate("Arch", "The alignment of the tile grid"))
            if not self.obj_to_edit:
                self.combo_align.setCurrentText(params.get_param_arch("CoveringAlignment"))
            form.addRow(translate("Arch", "Alignment:"), self.combo_align)

            self.sb_rot = ui.createWidget("Gui::QuantitySpinBox")
            self.sb_rot.setProperty("unit", "deg")
            if self.obj_to_edit:
                FreeCADGui.ExpressionBinding(self.sb_rot).bind(self.obj_to_edit, "Rotation")
                self.sb_rot.setProperty("rawValue", self.obj_to_edit.Rotation.Value)
            else:
                self.sb_rot.setProperty("rawValue", params.get_param_arch("CoveringRotation"))
            self.sb_rot.setToolTip(translate("Arch", "Rotation of the finish"))
            form.addRow(translate("Arch", "Rotation:"), self.sb_rot)

            self.page_tiles.setLayout(form)
            self.geo_stack.addWidget(self.page_tiles)

        def _setupHatchPage(self):
            self.page_hatch = QtGui.QWidget()
            form = QtGui.QFormLayout()
            ui = FreeCADGui.UiLoader()

            self.le_pat = QtGui.QLineEdit()
            self.le_pat.setToolTip(translate("Arch", "The PAT file to use for hatching"))

            btn_browse_pat = QtGui.QPushButton("...")
            btn_browse_pat.clicked.connect(self.browsePattern)
            h_pat = QtGui.QHBoxLayout()
            h_pat.addWidget(self.le_pat)
            h_pat.addWidget(btn_browse_pat)
            form.addRow(translate("Arch", "Pattern File:"), h_pat)

            self.sb_rot_hatch = ui.createWidget("Gui::QuantitySpinBox")
            self.sb_rot_hatch.setProperty("unit", "deg")
            if self.obj_to_edit:
                FreeCADGui.ExpressionBinding(self.sb_rot_hatch).bind(self.obj_to_edit, "Rotation")
                self.sb_rot_hatch.setProperty("rawValue", self.obj_to_edit.Rotation.Value)
            else:
                self.sb_rot_hatch.setProperty("rawValue", params.get_param_arch("CoveringRotation"))
            self.sb_rot_hatch.setToolTip(translate("Arch", "The rotation of the hatch pattern"))
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
                if index == 0:  # Solid tiles
                    self.sb_thick.setEnabled(True)
                    self.sb_thick.setProperty("rawValue", self.stored_thickness)
                    if self.obj_to_edit:
                        self.obj_to_edit.TileThickness = self.stored_thickness
                else:  # Parametric pattern
                    if self.sb_thick.isEnabled():
                        self.stored_thickness = self.sb_thick.value()
                        params.set_param_arch("CoveringThickness", self.stored_thickness)
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

        def action(self, arg):
            try:
                if arg["Type"] == "SoMouseButtonEvent" and arg["State"] == "DOWN":
                    # Check if picking is enabled in the task panel
                    if not self.isPicking():
                        return

                    pos = arg["Position"]
                    # Use standard View API to get object under cursor (returns list of dicts)
                    # We explicitly cast to int to avoid TypeErrors with some bindings
                    p_info = FreeCADGui.ActiveDocument.ActiveView.getObjectsInfo(
                        (int(pos[0]), int(pos[1]))
                    )

                    if p_info:
                        # p_info[0] is the top-most object
                        picked = p_info[0]
                        obj_name = picked.get("Object")
                        sub_name = picked.get("Component")

                        obj = FreeCAD.ActiveDocument.getObject(obj_name)

                        if obj:
                            # Check if the sub_name string contains "Face"
                            if sub_name and "Face" in sub_name:
                                self.setSelectedFace(obj, sub_name)
                            else:
                                self.setSelectedObject(obj)
            except Exception:
                import traceback

                traceback.print_exc()

        def onGetSelection(self):
            sel = FreeCADGui.Selection.getSelectionEx()
            if len(sel) > 0:
                # Take the first selected object
                obj = sel[0].Object
                if sel[0].SubElementNames and "Face" in sel[0].SubElementNames[0]:
                    self.setSelectedFace(obj, sel[0].SubElementNames[0])
                else:
                    self.setSelectedObject(obj)

        def setSelectedFace(self, obj, sub):
            self.selected_obj = obj
            self.selected_sub = sub
            self.le_selection.setText(f"{obj.Label}.{sub}")
            self.setPicking(False)
            self.updateBase()

        def setSelectedObject(self, obj):
            self.selected_obj = obj
            self.selected_sub = None
            self.le_selection.setText(f"{obj.Label}")
            self.setPicking(False)
            self.updateBase()

        def updateBase(self):
            # Update the Base property of the live object
            if self.obj_to_edit:
                if self.selected_sub:
                    self.obj_to_edit.Base = (self.selected_obj, [self.selected_sub])
                else:
                    self.obj_to_edit.Base = self.selected_obj

        def getStandardButtons(self):
            return QtGui.QDialogButtonBox.Ok | QtGui.QDialogButtonBox.Cancel

        def _cleanup_and_close(self):
            # Ensure callback is always removed
            self.view.removeEventCallback("SoEvent", self.callback)
            FreeCADGui.Control.closeDialog()

        def accept(self):
            try:
                if not self.obj_to_edit:
                    # Creation mode
                    import Arch

                    base = (
                        (self.selected_obj, [self.selected_sub])
                        if self.selected_sub
                        else self.selected_obj
                    )
                    obj = Arch.makeCovering(base)
                    obj.FinishMode = self.combo_mode.currentText()
                    if obj.FinishMode != "Hatch Pattern":
                        obj.TileLength = self.sb_length.property("rawValue")
                        obj.TileWidth = self.sb_width.property("rawValue")
                        obj.JointWidth = self.sb_joint.property("rawValue")
                        obj.TileAlignment = self.combo_align.currentText()
                        obj.Rotation = self.sb_rot.property("rawValue")
                        if obj.FinishMode == "Solid Tiles":
                            obj.TileThickness = self.sb_thick.property("rawValue")
                    else:
                        obj.PatternFile = self.le_pat.text()
                        obj.Rotation = self.sb_rot_hatch.property("rawValue")
                else:
                    # Edition mode
                    obj = self.obj_to_edit
                    obj.TileAlignment = self.combo_align.currentText()
                    if obj.FinishMode == "Hatch Pattern":
                        obj.PatternFile = self.le_pat.text()

                # Common visual setup
                if hasattr(obj.ViewObject, "TextureImage"):
                    obj.ViewObject.TextureImage = self.le_tex_image.text()

                # Persist user preferences
                params.set_param_arch("CoveringFinishMode", self.combo_mode.currentText())
                params.set_param_arch("CoveringAlignment", self.combo_align.currentText())

                if self.combo_mode.currentIndex() == 2:  # Hatch Pattern
                    rot_val = self.sb_rot_hatch.property("rawValue")
                else:
                    rot_val = self.sb_rot.property("rawValue")
                params.set_param_arch("CoveringRotation", rot_val)

                if obj.FinishMode != "Hatch Pattern":
                    params.set_param_arch("CoveringLength", obj.TileLength.Value)
                    params.set_param_arch("CoveringWidth", obj.TileWidth.Value)
                    params.set_param_arch("CoveringJoint", obj.JointWidth.Value)
                    if obj.FinishMode == "Solid Tiles":
                        params.set_param_arch("CoveringThickness", obj.TileThickness.Value)

                if FreeCAD.ActiveDocument.HasPendingTransaction:
                    FreeCAD.ActiveDocument.commitTransaction()

                FreeCAD.ActiveDocument.recompute()

            finally:
                self._cleanup_and_close()

        def reject(self):
            if self.obj_to_edit:
                FreeCADGui.ActiveDocument.resetEdit()
                if FreeCAD.ActiveDocument.HasPendingTransaction:
                    FreeCAD.ActiveDocument.abortTransaction()
            self._cleanup_and_close()
