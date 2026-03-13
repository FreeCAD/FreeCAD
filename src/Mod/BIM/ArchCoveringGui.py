# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Copyright (c) 2026 Furgo
#
# This file is part of the FreeCAD Arch workbench.
# You can find the full license text in the LICENSE file in the root directory.

import os
import FreeCAD
import Part
import Arch
import ArchCovering


def translate(context, sourceText, disambiguation=None, n=-1):
    return sourceText


def QT_TRANSLATE_NOOP(context, sourceText):
    return sourceText


if FreeCAD.GuiUp:
    from PySide import QtGui, QtCore
    import FreeCADGui
    import ArchComponent
    from draftutils import params

    translate = FreeCAD.Qt.translate
    QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP

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

            if "PickRotationStep" not in properties_list:
                vobj.addProperty(
                    "App::PropertyAngle",
                    "PickRotationStep",
                    "Interactive",
                    QT_TRANSLATE_NOOP(
                        "App::Property",
                        "Rotation step (degrees) applied per R / Shift+R keypress "
                        "during interactive grid placement.",
                    ),
                    locked=True,
                )
                vobj.PickRotationStep = 15.0

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

            # Update texture mapping when geometry or texture properties change
            if prop in [
                "Shape",
                "TextureImage",
                "TextureScale",
                "Rotation",
                "TileLength",
                "TileWidth",
                "JointWidth",
                "TileAlignment",
                "AlignmentOffset",
            ]:
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

            # TextureImage and TextureScale live on the DocumentObject. Guard against
            # the case where the object has not yet been fully restored.
            if not hasattr(obj, "TextureImage") or not hasattr(obj, "TextureScale"):
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

            # State check: identify indices of any existing texture nodes in the scene graph.
            # We must identify these before deciding whether to return early.
            found_indices = []
            for i in range(target_node.getNumChildren()):
                child = target_node.getChild(i)
                if isinstance(
                    child,
                    (coin.SoTexture2, coin.SoTextureCoordinatePlane, coin.SoTexture2Transform),
                ):
                    found_indices.append(i)

            # Performance guard: If the user has not set a texture and the scene graph is
            # already clean, exit immediately to avoid unnecessary processing.
            if not obj.TextureImage and not found_indices:
                return

            # Clean up existing texture nodes from FlatRoot.
            # We process the list in reverse to maintain index stability during removal.
            for i in reversed(found_indices):
                target_node.removeChild(i)

            self.texture = None

            # If the property was cleared, the scene is now synchronized and we can exit.
            if not obj.TextureImage or not os.path.exists(obj.TextureImage):
                return

            mapping = self._compute_texture_mapping(obj, vobj.DisplayMode)
            if not mapping:
                return

            dir_u, dir_v, s_offset, t_offset = mapping

            # Create Nodes
            texture_node = coin.SoTexture2()

            # Load Image
            img = self._get_cached_image(obj.TextureImage)
            if img:
                texture_node.image = img
            else:
                texture_node.filename = obj.TextureImage

            texture_node.model = coin.SoTexture2.REPLACE

            texcoords = coin.SoTextureCoordinatePlane()
            texcoords.directionS.setValue(coin.SbVec3f(dir_u.x, dir_u.y, dir_u.z))
            texcoords.directionT.setValue(coin.SbVec3f(dir_v.x, dir_v.y, dir_v.z))

            textrans = coin.SoTexture2Transform()
            textrans.translation.setValue(-s_offset, -t_offset)

            target_node.insertChild(texture_node, 0)
            target_node.insertChild(texcoords, 0)
            target_node.insertChild(textrans, 0)

        def _get_cached_image(self, file_path):
            """
            Retrieves a texture image from the cache or loads it from disk.

            Parameters
            ----------
            file_path : str
                The file system path to the image.

            Returns
            -------
            SoSFImage or None
                The loaded Coin3D image object, or None if loading failed.
            """
            from draftutils import gui_utils

            img = _ViewProviderCovering._texture_cache.get(file_path)
            if img is None:
                img = gui_utils.load_texture(file_path)
                if img:
                    _ViewProviderCovering._texture_cache[file_path] = img
            return img

        def _compute_texture_mapping(self, obj, display_mode):
            """
            Calculates the texture projection vectors and offsets.

            This method derives a localized orthonormal basis and origin point to
            align a texture bitmap with the physical tile grid.

            Parameters
            ----------
            obj : App::FeaturePython
                The Covering object containing dimensional, alignment, and placement data.
            display_mode : str
                The current display mode ("Flat Lines", "Shaded", etc.), which determines whether
                coordinates are resolved in local or global space.

            Returns
            -------
            tuple or None
                A tuple containing (dir_u, dir_v, s_offset, t_offset) where dir_* are normalized
                FreeCAD.Vectors and *_offset are floats representing the normalized texture
                coordinate shift. Returns None if the geometry basis cannot be resolved.

            Notes
            -----
            Texture mapping is performed via linear projection using a period equal to (Tile +
            Joint). Because standard Coin3D mapping nodes cannot skip the empty space of the joint,
            the mapping is shifted backwards by half the joint width. This workaround centers the
            image on the physical tile body, ensuring that any loss of texture due to the joint is
            distributed symmetrically across the edges.

            .. todo::
               The coordinate basis resolution and alignment shift logic replicates the code in
               `ArchCovering.execute`. This should be refactored into a shared helper method to
               ensure consistence and reduce maintenance overhead.
            """
            vobj = obj.ViewObject

            # Geometry calculation (in global space)
            base_face = Arch.getFaceGeometry(obj.Base)
            if not base_face:
                return None

            # Replicate _apply_boundaries: inset by BorderSetback only, NOT joint_width.
            # The bounding box of effective_face is used by getFaceGridOrigin to locate
            # preset alignment corners. It must match the face used by the geometry engine
            # exactly, or the texture origin will be offset from the tile origin by the
            # joint width for non-BottomLeft presets.
            effective_face = base_face.copy()
            joint_width = obj.JointWidth.Value if hasattr(obj, "JointWidth") else 0.0
            border_setback = obj.BorderSetback.Value

            # Note: makeOffset2D returns a new shape; it does not modify in-place.
            # Guard against degenerate offsets (face too small) which raise CADKernelError.
            if border_setback > 0:
                try:
                    effective_face = base_face.makeOffset2D(-border_setback)
                except Exception:
                    effective_face = base_face.copy()

            # Replicate the object's frame resolution logic.
            # Use the face's own canonical UV basis (same as ArchCovering._get_layout_frame),
            # not the covering object's Placement rotation. Deriving u_vec from obj.Placement
            # would make the texture basis track the covering's orientation, which cancels
            # the Flat Lines inverse-rotation transform and breaks rotated objects.
            # getFaceUV now owns the tangent stabilisation (dominant-axis sign normalisation
            # and re-orthonormalisation), so no post-processing is needed here.
            u_vec, v_vec, normal, center_point = Arch.getFaceUV(base_face)
            if base_face.Orientation == "Reversed":
                normal = -normal
                v_vec = normal.cross(u_vec).normalize()
                u_vec = v_vec.cross(normal).normalize()

            # Calculate origin
            if obj.TileAlignment == "Custom":
                origin = (
                    center_point + u_vec * obj.AlignmentOffset.x + v_vec * obj.AlignmentOffset.y
                )
            else:
                origin = ArchCovering.getAlignedGridOrigin(
                    effective_face,
                    center_point,
                    u_vec,
                    v_vec,
                    obj.TileAlignment,
                    obj.TileLength.Value,
                    obj.TileWidth.Value,
                    obj.AlignmentOffset,
                )

            # Apply different logic based on the active DisplayMode
            if display_mode == "Flat Lines":
                # "Flat Lines" mode requires a Global-to-Local transform.
                inv_pl = obj.Placement.inverse()
                calc_u = inv_pl.Rotation.multVec(u_vec)
                calc_v = inv_pl.Rotation.multVec(v_vec)
                calc_norm = inv_pl.Rotation.multVec(normal)
                calc_origin = inv_pl.multVec(origin)
            elif display_mode == "Shaded":
                # "Shaded" mode requires Global coordinates (no transform).
                calc_u = u_vec
                calc_v = v_vec
                calc_norm = normal
                calc_origin = origin
            else:
                return None

            # Vector normalization
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

            # Apply texture rotation
            if hasattr(obj, "Rotation") and obj.Rotation.Value != 0:
                rot = FreeCAD.Rotation(calc_norm, obj.Rotation.Value)
                calc_u = rot.multVec(calc_u)
                calc_v = rot.multVec(calc_v)

            # Apply texture scaling
            scale_u = obj.TextureScale.x if obj.TextureScale.x != 0 else 1.0
            scale_v = obj.TextureScale.y if obj.TextureScale.y != 0 else 1.0

            # Calculate period (tile + joint)
            period_u = (obj.TileLength.Value + joint_width) * scale_u
            period_v = (obj.TileWidth.Value + joint_width) * scale_v

            if period_u == 0:
                period_u = 1000.0
            if period_v == 0:
                period_v = 1000.0

            # Calculate final directions
            dir_u = calc_u.multiply(1.0 / period_u)
            dir_v = calc_v.multiply(1.0 / period_v)

            # Calculate offsets
            s_offset = calc_origin.dot(dir_u)
            t_offset = calc_origin.dot(dir_v)

            # Center the texture on the tile body
            # Subtract half the joint width to pull the image back to the visual center
            if hasattr(obj, "JointWidth"):
                s_offset -= (obj.JointWidth.Value / 2.0) * scale_u / period_u  # Normalized
                t_offset -= (obj.JointWidth.Value / 2.0) * scale_v / period_v

            return dir_u, dir_v, s_offset, t_offset

    class ArchCoveringTaskPanel:
        """
        A Task Panel for creating and editing Arch Covering objects.

        This class manages user input and property states using a session-based buffer (the
        template) to ensure transactional stability and atomic undos.

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
        This class employs a "Buffered Proxy" pattern. Instead of binding UI widgets directly to the
        final architectural objects, it binds exclusively to an internal, hidden _CoveringTemplate
        (the template). This ensures:

        *   Batch creation: A single set of UI settings can be applied to multiple faces.
        *   Undo stability: Internal UI state changes do not pollute the document's undo history.
        *   Atomic continue: Each object created in "Continue" mode is a separate undoable event.

        Data flow and binding:

        1.  **Initialization:** the UI widgets bind exclusively to the template buffer. If in
            Edit Mode, the template is initialized once with the values of the real object.
        2.  **Synchronization:** when the user interacts with the UI, the _sync_ui_to_target()
            method updates the properties of the template buffer.
        3.  **Transfer (the flush):** When accept() is called:
            *   A new, short-lived document transaction is opened.
            *   In Creation: New objects are instantiated and settings are stamped from the
                template master using template.apply_to(new_obj).
            *   In Edition: The existing object is updated using template.apply_to(obj_to_edit).
            *   The transaction is committed immediately.
        4.  **Cleanup:** The template is destroyed only when the panel is closed, ensuring
            it never appears in the document history or Tree View.
        """

        def __init__(self, command=None, obj=None, selection=None):
            self.command = command
            self.obj_to_edit = obj
            self.template = _CoveringTemplate()
            self.selected_obj = None
            self.selected_sub = None
            # Keep references to ExpressionBinding objects to prevent garbage collection
            self.bindings = []
            # Auto-populated by _setup_bound_spinbox; consumed by _rebind_to_buffer
            self._bound_spinboxes = []

            # Handle selection list or single object
            if selection and not isinstance(selection, list):
                self.selection_list = [selection]
            else:
                self.selection_list = selection if selection else []

            # Initialize state from existing object or defaults
            if self.obj_to_edit:
                self.template.copy_from(self.obj_to_edit)
            else:
                # Preload hatch defaults
                self.template.buffer.PatternScale = 100.0
                pat_file = params.get_param("HatchPatternFile")
                if not pat_file or not os.path.exists(pat_file):
                    pat_file = os.path.join(
                        FreeCAD.getResourceDir(), "data", "examples", "FCPAT.pat"
                    )
                    if not os.path.exists(pat_file):
                        pat_file = os.path.join(
                            FreeCAD.getResourceDir(), "Mod", "TechDraw", "PAT", "FCPAT.pat"
                        )
                if os.path.exists(pat_file):
                    self.template.buffer.PatternFile = pat_file

            # Smart Face Detection for pre-selection
            resolved_selection = []
            view_dir = self._get_view_direction()

            for item in self.selection_list:
                if not isinstance(item, tuple):
                    # Access via Proxy of the target object (template buffer)
                    # This ensures we use the logic defined in _Covering
                    face = Arch.getFaceName(item, view_dir)
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
            self.geo_widget.setWindowTitle(translate("Arch", "Covering Definition"))
            self.geo_layout = QtGui.QVBoxLayout(self.geo_widget)

            # Register selection observer
            FreeCADGui.Selection.addObserver(self)

            # Ensure observer is removed when the UI is destroyed (e.g. forced close)
            self.geo_widget.destroyed.connect(self._unregister_observer)

            self._setupTopControls()
            self._setupGeometryStack()
            self._setupBottomControls()

            # Task Box 2: Layout and boundaries
            self.layout_widget = QtGui.QWidget()
            self.layout_widget.setWindowTitle(translate("Arch", "Layout and Boundaries"))
            self.layout_layout = QtGui.QVBoxLayout(self.layout_widget)
            self._setupLayoutControls()

            # Task Box 3: Visuals
            self.vis_widget = QtGui.QWidget()
            self.vis_widget.setWindowTitle(translate("Arch", "Visuals"))
            self._setupVisualUI()

            self.form = [self.geo_widget, self.layout_widget, self.vis_widget]

            # Sync the UI to the initialized template buffer (Handles both Edit and Creation modes)
            self._loadExistingData()

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
                tooltip = (
                    translate("Arch", "The object or face this covering is applied to:")
                    + "\n"
                    + text
                )
                self.le_selection.setToolTip(tooltip)
                return

            # Creation mode (batch target list)
            if not self.selection_list:
                self.le_selection.setText(translate("Arch", "No selection"))
                self.le_selection.setToolTip(
                    translate("Arch", "The object or face this covering is applied to")
                )
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
            tooltip = (
                translate("Arch", "The object or face this covering is applied to:")
                + "\n"
                + ", ".join(tooltip_items)
            )
            self.le_selection.setToolTip(tooltip)

        def _setupTopControls(self):
            top_form = QtGui.QFormLayout()

            # Selection
            h_sel = QtGui.QHBoxLayout()
            self.le_selection = QtGui.QLineEdit()
            self.le_selection.setPlaceholderText(translate("Arch", "No selection"))
            self.le_selection.setEnabled(False)
            self.le_selection.setToolTip(
                translate("Arch", "The object or face this covering is applied to")
            )

            self.btn_selection = QtGui.QPushButton(translate("Arch", "Pick"))
            self.btn_selection.setCheckable(True)
            self.btn_selection.setToolTip(
                translate("Arch", "Enable interactive face selection in the 3D view")
            )

            # Build a two-state icon: full-colour record circle when checked (picking
            # active), auto-greyed version when unchecked. A single SVG source is enough
            # because Qt generates the Disabled-mode pixmap automatically.
            _rec_icon = QtGui.QIcon()
            _px_on = QtGui.QIcon(":/icons/media-record.svg").pixmap(16, 16)
            _px_off = QtGui.QIcon(":/icons/media-record.svg").pixmap(16, 16, QtGui.QIcon.Disabled)
            _rec_icon.addPixmap(_px_on, QtGui.QIcon.Normal, QtGui.QIcon.On)
            _rec_icon.addPixmap(_px_off, QtGui.QIcon.Normal, QtGui.QIcon.Off)
            self.btn_selection.setIcon(_rec_icon)

            # Also change the button text to reinforce the state change
            self.btn_selection.toggled.connect(
                lambda checked: self.btn_selection.setText(
                    translate("Arch", "Picking…") if checked else translate("Arch", "Pick")
                )
            )

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
                    translate("Arch", "Monolithic"),
                    translate("Arch", "Hatch Pattern"),
                ]
            )
            self.combo_mode.setToolTip(
                translate(
                    "Arch",
                    "How the finish is created and displayed:\n"
                    "- Solid Tiles: Physical 3D tiles with real gaps. Best for accurate detail and counting.\n"
                    "- Parametric Pattern: A grid of lines on a single slab. Faster to display than real tiles.\n"
                    "- Monolithic: A single smooth surface. Ideal for paint, plaster, or seamless flooring.\n"
                    "- Hatch Pattern: Technical drafting symbols (hatching) on a single slab.",
                )
            )
            self.combo_mode.setCurrentText(self.template.buffer.FinishMode)
            self.combo_mode.currentIndexChanged.connect(self.onModeChanged)
            top_form.addRow(translate("Arch", "Mode:"), self.combo_mode)

            # Thickness
            self.sb_thick = self._setup_bound_spinbox(
                "TileThickness", translate("Arch", "The thickness of the finish")
            )
            top_form.addRow(translate("Arch", "Thickness:"), self.sb_thick)

            self.geo_layout.addLayout(top_form)

        def _setupGeometryStack(self):
            self.geo_stack = QtGui.QStackedWidget()

            self._setupTilesPage()
            self._setupHatchPage()
            self._setupMonolithicPage()

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

        def _setupLayoutControls(self):
            grp_layout = QtGui.QGroupBox(translate("Arch", "Alignment"))
            vbox = QtGui.QVBoxLayout()

            # Branch 1: Preset
            h_preset = QtGui.QHBoxLayout()
            self.radio_preset = QtGui.QRadioButton(translate("Arch", "Preset:"))
            self.radio_preset.setToolTip(
                translate(
                    "Arch", "Use standard corner or center alignment relative to the boundary"
                )
            )
            self.combo_align = QtGui.QComboBox()
            self.combo_align.addItems(
                ["Center", "TopLeft", "TopRight", "BottomLeft", "BottomRight"]
            )
            self.combo_align.setToolTip(
                translate(
                    "Arch",
                    "Select which part of the usable boundary to anchor the pattern origin to",
                )
            )
            h_preset.addWidget(self.radio_preset)
            h_preset.addWidget(self.combo_align)
            vbox.addLayout(h_preset)

            # Branch 2: Custom
            h_custom = QtGui.QHBoxLayout()
            self.radio_custom = QtGui.QRadioButton(translate("Arch", "Custom:"))
            self.radio_custom.setToolTip(
                translate(
                    "Arch", "Use a manually picked 3D point or match the current Working Plane"
                )
            )
            self.btn_pick = QtGui.QPushButton(translate("Arch", "Interactive"))
            self.btn_pick.setCheckable(True)
            self.btn_pick.setToolTip(
                translate(
                    "Arch",
                    "Enter interactive mode to visually place the grid origin and rotate the grid. "
                    "Click to finish and set the origin. Optionally press R / Shift+R to rotate "
                    "the tile preview by the PickRotationStep angle (configurable in the View "
                    "properties).",
                )
            )
            self.btn_match_wp = QtGui.QPushButton(translate("Arch", "Match Working Plane"))
            self.btn_match_wp.setToolTip(
                translate(
                    "Arch",
                    "Use the position and orientation of the active Working Plane for the covering",
                )
            )
            h_custom.addWidget(self.radio_custom)
            h_custom.addWidget(self.btn_pick)
            h_custom.addWidget(self.btn_match_wp)
            vbox.addLayout(h_custom)

            # Custom offsets — bound to AlignmentOffset.x and AlignmentOffset.y
            offset = self.template.buffer.AlignmentOffset
            self.sb_u_off = self._setup_bound_spinbox(
                "AlignmentOffset.x",
                translate("Arch", "Shift the grid along U"),
                initial_value=FreeCAD.Units.Quantity(offset.x, FreeCAD.Units.Length),
            )
            self.sb_v_off = self._setup_bound_spinbox(
                "AlignmentOffset.y",
                translate("Arch", "Shift the grid along V"),
                initial_value=FreeCAD.Units.Quantity(offset.y, FreeCAD.Units.Length),
            )
            form_offsets = QtGui.QFormLayout()
            form_offsets.addRow(translate("Arch", "U Offset:"), self.sb_u_off)
            form_offsets.addRow(translate("Arch", "V Offset:"), self.sb_v_off)
            vbox.addLayout(form_offsets)

            # Global rotation
            form_rot = QtGui.QFormLayout()
            self.sb_rot = self._setup_bound_spinbox("Rotation", "Manual rotation nudge")
            form_rot.addRow(translate("Arch", "Rotation:"), self.sb_rot)
            vbox.addLayout(form_rot)

            grp_layout.setLayout(vbox)
            self.layout_layout.addWidget(grp_layout)

            # Connect state machine
            self.radio_preset.toggled.connect(self._on_alignment_mode_changed)
            self.btn_pick.clicked.connect(self.onPickOrigin)
            self.btn_match_wp.clicked.connect(self.onMatchWP)

            # Boundaries group
            grp_bound = QtGui.QGroupBox(translate("Arch", "Boundaries"))
            form_bound = QtGui.QFormLayout()

            self.sb_setback = self._setup_bound_spinbox(
                "BorderSetback",
                translate("Arch", "Distance to offset the covering inwards from the boundary"),
            )
            form_bound.addRow(translate("Arch", "Border Setback:"), self.sb_setback)

            grp_bound.setLayout(form_bound)
            self.layout_layout.addWidget(grp_bound)

        # Helper for binding properties to a quantity spinbox with default
        def _setup_bound_spinbox(self, prop_name, tooltip=None, initial_value=None):
            """Creates a Gui::QuantitySpinBox bound to prop_name on the template buffer.

            The widget self-registers into self._bound_spinboxes so that _rebind_to_buffer()
            always has a complete inventory without manual maintenance.

            ExpressionBinding.bind() does not reliably populate the widget's display
            synchronously — it may fire asynchronously via Qt signals, meaning the widget
            can still show 0 when _sync_ui_to_target() reads it on accept(). We therefore
            always set the initial value explicitly before binding.

            For scalar properties, the value is read from the buffer via getattr().
            For dot-path sub-properties (e.g. "AlignmentOffset.x"), getattr() cannot
            resolve sub-components, so the caller must supply initial_value explicitly.

            Parameters
            ----------
            prop_name : str
                Property name on the template buffer, or a dot-path for sub-components
                (e.g. "AlignmentOffset.x").
            tooltip : str, optional
            initial_value : FreeCAD.Units.Quantity, optional
                Explicit initial value. Required when prop_name contains a dot.
            """
            sb = FreeCADGui.UiLoader().createWidget("Gui::QuantitySpinBox")
            if tooltip:
                sb.setToolTip(translate("Arch", tooltip))
            if initial_value is not None:
                sb.setProperty("value", initial_value)
            elif "." not in prop_name:
                sb.setProperty("value", getattr(self.template.buffer, prop_name))
            FreeCADGui.ExpressionBinding(sb).bind(self.template.buffer, prop_name)
            self._bound_spinboxes.append((sb, prop_name))
            return sb

        def _rebind_to_buffer(self):
            """Re-establishes ExpressionBinding on all bound spinboxes after the buffer is replaced.

            In continue mode the buffer is removed and a new one created for each OK cycle.
            ExpressionBinding stores a direct C++ pointer to the bound object, so all bindings
            must be refreshed to point at the new buffer or the f(x) button will crash FreeCAD.
            """
            self.bindings = []
            for sb, prop_name in self._bound_spinboxes:
                binding = FreeCADGui.ExpressionBinding(sb)
                binding.bind(self.template.buffer, prop_name)
                self.bindings.append(binding)

        def _setupTilesPage(self):
            self.page_tiles = QtGui.QWidget()
            form = QtGui.QFormLayout()

            # Tile Length
            self.sb_length = self._setup_bound_spinbox(
                "TileLength", translate("Arch", "The length of the tiles")
            )
            form.addRow(translate("Arch", "Length:"), self.sb_length)

            # Tile Width
            self.sb_width = self._setup_bound_spinbox(
                "TileWidth", translate("Arch", "The width of the tiles")
            )
            form.addRow(translate("Arch", "Width:"), self.sb_width)

            # Joint Width
            self.sb_joint = self._setup_bound_spinbox(
                "JointWidth", translate("Arch", "The width of the joints between tiles")
            )
            form.addRow(translate("Arch", "Joint Width:"), self.sb_joint)

            # Stagger
            h_stagger = QtGui.QHBoxLayout()
            self.combo_stagger = QtGui.QComboBox()
            self.combo_stagger.addItems(
                [
                    translate("Arch", "Stacked (None)"),
                    translate("Arch", "Half Bond (1/2)"),
                    translate("Arch", "Third Bond (1/3)"),
                    translate("Arch", "Quarter Bond (1/4)"),
                    translate("Arch", "Custom"),
                ]
            )
            self.combo_stagger.setToolTip(
                translate(
                    "Arch",
                    "The horizontal shift applied to every second row:\n"
                    "- Stacked: all joints align vertically\n"
                    "- Half/Third/Quarter Bond: shifts by a fraction of the tile length\n"
                    "- Custom: manual offset value",
                )
            )
            self.combo_stagger.currentIndexChanged.connect(self.onStaggerChanged)
            h_stagger.addWidget(self.combo_stagger)

            self.sb_stagger_custom = self._setup_bound_spinbox(
                "StaggerCustom", translate("Arch", "Custom offset for running bond rows")
            )
            h_stagger.addWidget(self.sb_stagger_custom)
            form.addRow(translate("Arch", "Stagger:"), h_stagger)

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

            self.combo_pattern = QtGui.QComboBox()
            self.combo_pattern.setEditable(True)  # Allow custom input if parsing fails
            self.combo_pattern.setToolTip(translate("Arch", "The name of the pattern to use"))
            form.addRow(translate("Arch", "Pattern Name:"), self.combo_pattern)

            self.sb_scale_hatch = QtGui.QDoubleSpinBox()
            self.sb_scale_hatch.setRange(0.001, 1000000.0)
            self.sb_scale_hatch.setDecimals(2)
            self.sb_scale_hatch.setToolTip(translate("Arch", "The scale of the hatch pattern"))
            self.sb_scale_hatch.lineEdit().returnPressed.connect(self.accept)
            form.addRow(translate("Arch", "Pattern Scale:"), self.sb_scale_hatch)

            self.page_hatch.setLayout(form)
            self.geo_stack.addWidget(self.page_hatch)

        def _setupMonolithicPage(self):
            self.page_mono = QtGui.QWidget()
            form = QtGui.QFormLayout(self.page_mono)
            form.setContentsMargins(0, 0, 0, 0)

            self.sb_length_mono = self._setup_bound_spinbox("TileLength", "Pattern repeat length")
            self.sb_width_mono = self._setup_bound_spinbox("TileWidth", "Pattern repeat width")

            form.addRow(translate("Arch", "Length:"), self.sb_length_mono)
            form.addRow(translate("Arch", "Width:"), self.sb_width_mono)

            # Informational context for the user
            lbl_info = QtGui.QLabel(
                translate(
                    "Arch",
                    "Note: In Monolithic mode, dimensions control the repeat interval of the optional surface texture.",
                )
            )
            lbl_info.setWordWrap(True)
            lbl_info.setStyleSheet("font-style: italic; color: gray;")
            form.addRow(lbl_info)
            self.geo_stack.addWidget(self.page_mono)

        def _setupVisualUI(self):
            visual_form = QtGui.QFormLayout(self.vis_widget)
            self.le_tex_image = QtGui.QLineEdit()
            self.le_tex_image.setToolTip(
                translate("Arch", "An image file to map onto each tile or substrate")
            )
            btn_browse = QtGui.QPushButton("...")
            btn_browse.clicked.connect(self.browseTexture)

            h_tex = QtGui.QHBoxLayout()
            h_tex.addWidget(self.le_tex_image)
            h_tex.addWidget(btn_browse)
            visual_form.addRow(translate("Arch", "Texture Image:"), h_tex)

            # Texture scaling multiplier for visual fine-tuning
            h_scale = QtGui.QHBoxLayout()
            self.sb_tex_scale_u = QtGui.QDoubleSpinBox()
            self.sb_tex_scale_u.setRange(0.001, 1000.0)
            self.sb_tex_scale_u.setSingleStep(0.1)
            self.sb_tex_scale_u.setToolTip(translate("Arch", "Horizontal texture multiplier"))
            self.sb_tex_scale_v = QtGui.QDoubleSpinBox()
            self.sb_tex_scale_v.setRange(0.001, 1000.0)
            self.sb_tex_scale_v.setSingleStep(0.1)
            self.sb_tex_scale_v.setToolTip(translate("Arch", "Vertical texture multiplier"))
            h_scale.addWidget(self.sb_tex_scale_u)
            h_scale.addWidget(self.sb_tex_scale_v)
            visual_form.addRow(translate("Arch", "Texture Scale:"), h_scale)

        def _loadExistingData(self):
            # Sync the combobox and force the stacked widget to the correct page
            mode = self.template.buffer.FinishMode
            self.combo_mode.setCurrentText(mode)
            self.onModeChanged(self.combo_mode.currentIndex())

            # Initialize radio button state based on the current alignment mode
            is_custom = self.template.buffer.TileAlignment == "Custom"
            self.radio_custom.setChecked(is_custom)
            self.radio_preset.setChecked(not is_custom)

            if not is_custom:
                self.combo_align.setCurrentText(self.template.buffer.TileAlignment)
            self._on_alignment_mode_changed(not is_custom)

            # Load hatch pattern data
            pat_file = self.template.buffer.PatternFile
            if pat_file:
                self.le_pat.setText(pat_file)
                self.updatePatterns(pat_file)

            pat_name = self.template.buffer.PatternName
            if pat_name:
                self.combo_pattern.setCurrentText(pat_name)

            self.sb_scale_hatch.setValue(self.template.buffer.PatternScale)

            # Stagger data
            if hasattr(self.template.buffer, "StaggerType"):
                self.combo_stagger.setCurrentText(self.template.buffer.StaggerType)
            # Trigger handler to update enabled state of custom box
            self.onStaggerChanged(self.combo_stagger.currentIndex())

            # Visuals
            if self.template.buffer.TextureImage:
                self.le_tex_image.setText(self.template.buffer.TextureImage)

            self.sb_tex_scale_u.setValue(self.template.buffer.TextureScale.x)
            self.sb_tex_scale_v.setValue(self.template.buffer.TextureScale.y)

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
                self.updatePatterns(fn)  # Call helper

        def updatePatterns(self, filename):
            self.combo_pattern.clear()
            if filename and os.path.exists(filename):
                try:
                    with open(filename, "r") as f:
                        for line in f:
                            if line.startswith("*"):
                                # *PatternName, Description -> PatternName
                                self.combo_pattern.addItem(line.split(",")[0][1:].strip())
                except Exception:
                    pass
            if self.combo_pattern.count() > 0:
                self.combo_pattern.setCurrentIndex(0)

        def onModeChanged(self, index):
            """Updates the UI layout based on the selected Finish Mode."""
            if index <= 1:
                # Solid Tiles or Parametric Pattern share the Tiles Page
                self.geo_stack.setCurrentIndex(0)
            elif index == 2:
                # Monolithic uses the dedicated texture period page
                self.geo_stack.setCurrentIndex(2)
            else:
                # Hatch Pattern uses the Hatch Page
                self.geo_stack.setCurrentIndex(1)

            # Disable visuals (textures) only for Hatch Pattern (Index 3)
            self.vis_widget.setEnabled(index != 3)

            self.template.buffer.FinishMode = self.combo_mode.currentText()

        def onStaggerChanged(self, index):
            """Enables or disables the custom stagger input based on selection."""
            is_custom = self.combo_stagger.currentText() == "Custom"
            self.sb_stagger_custom.setEnabled(is_custom)

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

                # If the user picks the template or the object being edited, ignore it.
                if obj == self.template.buffer or obj == self.obj_to_edit:
                    continue

                # PartDesign Normalization
                for parent in obj.InList:
                    if parent.isDerivedFrom("PartDesign::Body"):
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
                    best_face = Arch.getFaceName(obj, view_dir)

                    if best_face:
                        new_list.append((obj, [best_face]))
                    else:
                        new_list.append(obj)

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

        def _on_alignment_mode_changed(self, is_preset):
            """Updates widget enabled states based on the active radio branch."""
            self.combo_align.setEnabled(is_preset)
            self.btn_pick.setEnabled(not is_preset)
            self.btn_match_wp.setEnabled(not is_preset)
            self.sb_u_off.setEnabled(not is_preset)
            self.sb_v_off.setEnabled(not is_preset)

            # Clear the manual offsets when switching to Preset mode to prevent
            # them from being invisibly added to the standard alignment logic.
            if is_preset:
                self.sb_u_off.setProperty("value", FreeCAD.Units.Quantity(0, FreeCAD.Units.Length))
                self.sb_v_off.setProperty("value", FreeCAD.Units.Quantity(0, FreeCAD.Units.Length))

        def onMatchWP(self):
            """Snapshots the current working plane into the covering properties."""
            import WorkingPlane
            import DraftGeomUtils
            import DraftVecUtils
            import math

            # Resolve target geometry from the live object or selection.
            ref = (
                self.obj_to_edit.Base
                if self.obj_to_edit
                else (self.selection_list[0] if self.selection_list else None)
            )
            base_face = Arch.getFaceGeometry(ref)
            if not base_face:
                return

            wp = WorkingPlane.get_working_plane()
            u_basis, v_basis, normal, center = self.template.buffer.Proxy._get_layout_frame(
                base_face
            )

            # Determine rotation angle between the working plane U axis and the face local frame.
            wp_u_proj = wp.u - normal * wp.u.dot(normal)
            if wp_u_proj.Length > 1e-7:
                angle_deg = math.degrees(DraftVecUtils.angle(u_basis, wp_u_proj, normal))

                # Write to the buffer directly, mirroring the AlignmentOffset pattern below.
                # setProperty("value", float) on a Gui::QuantitySpinBox updates the visual
                # display but does NOT update the widget's internal Quantity object, so a
                # bare-float setProperty is not reliably picked up by _sync_ui_to_target.
                # Writing to the buffer first guarantees the value survives accept().
                self.template.buffer.Rotation = angle_deg

                # Sync the display so the user sees what was applied.
                self.sb_rot.setProperty(
                    "value", FreeCAD.Units.Quantity(angle_deg, FreeCAD.Units.Angle)
                )

            # Project the working plane origin onto the face to determine the offset.
            pt_on_face = DraftGeomUtils.project_point_on_plane(wp.position, center, normal)
            delta = pt_on_face - center
            u_off = delta.dot(u_basis)
            v_off = delta.dot(v_basis)

            self.template.buffer.AlignmentOffset = FreeCAD.Vector(u_off, v_off, 0)
            self.sb_u_off.setProperty("value", FreeCAD.Units.Quantity(u_off, FreeCAD.Units.Length))
            self.sb_v_off.setProperty("value", FreeCAD.Units.Quantity(v_off, FreeCAD.Units.Length))
            self.radio_custom.setChecked(True)

        class _PickShortcutFilter(QtCore.QObject):
            """
            Qt event filter that intercepts the R key during interactive grid placement.

            Installed on all task panel form widgets so that R / Shift+R rotates the
            tile wireframe regardless of which widget currently has keyboard focus,
            without interfering with normal text entry in spinboxes or line edits.
            """

            def __init__(self, handler):
                super().__init__()
                self._handler = handler

            def eventFilter(self, watched, event):
                if event.type() == QtCore.QEvent.KeyPress:
                    if event.text().upper() == "R":
                        self._handler(shift=bool(event.modifiers() & QtCore.Qt.ShiftModifier))
                        return True  # Consumed — do not pass to the focused widget
                return False

        def onPickOrigin(self, state):
            """Initializes or terminates the interactive snapper loop."""
            if state:
                # Resolve basis once for the duration of the pick to ensure performance and safety.
                ref = (
                    self.obj_to_edit.Base
                    if self.obj_to_edit
                    else (self.selection_list[0] if self.selection_list else None)
                )
                base_face = Arch.getFaceGeometry(ref)
                if not base_face:
                    FreeCAD.Console.PrintError(
                        translate("Arch", "Could not resolve base geometry.") + "\n"
                    )
                    self.btn_pick.setChecked(False)
                    return

                self._cached_basis = self.template.buffer.Proxy._get_layout_frame(base_face)

                # Seed the incremental rotation counter from the spinbox rather than
                # the buffer: ExpressionBinding may not have flushed the spinbox value
                # back to the buffer yet if the user typed a value and immediately clicked
                # Interactive without first pressing Tab or Enter.
                try:
                    self._pick_rotation_deg = self.sb_rot.property("value").Value
                except Exception:
                    self._pick_rotation_deg = self.template.buffer.Rotation.Value

                # Setup the lead-tile box tracker.
                import draftguitools.gui_trackers as trackers

                self.tracker = trackers.boxTracker()
                self.tracker.length(self.template.buffer.TileLength.Value)
                self.tracker.width(self.template.buffer.TileWidth.Value)
                self.tracker.height(self.template.buffer.TileThickness.Value)
                self.tracker.on()

                # Install a Qt event filter to intercept the R key across all form widgets.
                # Coin3D's SoKeyboardEvent is not delivered when a Qt widget has focus,
                # so the event filter is the only reliable interception point.
                def _rotation_handler(shift=False):
                    try:
                        step = self.template.buffer.ViewObject.PickRotationStep.Value
                    except Exception:
                        step = 15.0
                    if shift:
                        step = -step
                    self._pick_rotation_deg = (
                        getattr(self, "_pick_rotation_deg", 0.0) + step
                    ) % 360.0
                    self.template.buffer.Rotation = self._pick_rotation_deg
                    self.sb_rot.setProperty(
                        "value",
                        FreeCAD.Units.Quantity(self._pick_rotation_deg, FreeCAD.Units.Angle),
                    )
                    if hasattr(self, "pt"):
                        self.onMouseMove(self.pt, None)

                # Install on the application instance so the filter intercepts R
                # regardless of which child widget has keyboard focus. A per-widget
                # filter on self.form only sees events on the container, not on focused
                # children such as spinboxes.
                self._shortcut_filter = self._PickShortcutFilter(_rotation_handler)
                QtGui.QApplication.instance().installEventFilter(self._shortcut_filter)

                # Start the interaction loop using a view callback to keep the task panel active.
                self._view = FreeCADGui.ActiveDocument.ActiveView
                self._callback_id = self._view.addEventCallback("SoEvent", self._handle_interaction)

                # Lock UI and enter a ghost mode for clear 3D context
                self.geo_widget.setEnabled(False)
                self.vis_widget.setEnabled(False)
                if self.obj_to_edit:
                    self._old_transparency = self.obj_to_edit.ViewObject.Transparency
                    self._old_selectable = self.obj_to_edit.ViewObject.Selectable
                    self.obj_to_edit.ViewObject.Transparency = 70
                    self.obj_to_edit.ViewObject.Selectable = False

                # Wake up the Draft Snapper visuals (Grid and Snap Markers)
                FreeCAD.activeDraftCommand = self
                if hasattr(FreeCADGui, "Snapper"):
                    FreeCADGui.Snapper.show()
                    FreeCADGui.Snapper.setTrackers()
            else:
                self._cleanup_snapper()

        def _handle_interaction(self, arg):
            """Processes low-level events for snapping and picking."""
            # Handle movement to update the snapping position and visual tracker.
            if arg["Type"] == "SoLocation2Event":
                ctrl = arg["CtrlDown"]
                shift = arg["ShiftDown"]
                # Invoke the snapper manually to allow standard keyboard modifiers.
                self.pt = FreeCADGui.Snapper.snap(arg["Position"], active=ctrl, constrain=shift)
                self.onMouseMove(self.pt, None)

            # Handle click to confirm the origin point.
            elif (
                arg["Type"] == "SoMouseButtonEvent"
                and arg["State"] == "DOWN"
                and arg["Button"] == "BUTTON1"
            ):
                # Disconnect immediately to prevent re-entrancy during processing.
                if hasattr(self, "_callback_id") and hasattr(self, "_view"):
                    self._view.removeEventCallback("SoEvent", self._callback_id)
                    del self._callback_id

                # Delay the finalization to ensure the Coin3D event traversal has finished.
                from draftutils.todo import ToDo

                ToDo.delay(self.onPointPicked, self.pt)

            # Handle escape to cancel the operation.
            elif arg["Type"] == "SoKeyboardEvent" and arg["Key"] == "ESCAPE":
                from draftutils.todo import ToDo

                ToDo.delay(self._cleanup_snapper, None)

        def onMouseMove(self, point, info):
            """Updates the lead-tile tracker following the cursor."""
            if point and hasattr(self, "tracker") and hasattr(self, "_cached_basis"):
                u_basis, v_basis, normal, _ = self._cached_basis
                l, w, t = (
                    self.template.buffer.TileLength.Value,
                    self.template.buffer.TileWidth.Value,
                    self.template.buffer.TileThickness.Value,
                )

                # Build the final rotation: face frame composed with any pick rotation.
                base_rot = FreeCAD.Rotation(u_basis, v_basis, normal, "XYZ")
                extra_deg = getattr(self, "_pick_rotation_deg", 0.0)
                if extra_deg:
                    extra_rot = FreeCAD.Rotation(normal, extra_deg)
                    final_rot = extra_rot.multiply(base_rot)
                else:
                    final_rot = base_rot

                # Derive the rotated u/v axes so the anchor offset uses the visually
                # correct directions. Without this, the bottom-left corner drifts away
                # from the cursor when a pick rotation is applied.
                u_rotated = final_rot.multVec(FreeCAD.Vector(1, 0, 0))
                v_rotated = final_rot.multVec(FreeCAD.Vector(0, 1, 0))

                # Apply an offset so the cursor tracks the bottom-left corner of the box.
                delta = (u_rotated * (l / 2.0)) + (v_rotated * (w / 2.0)) + (normal * (t / 2.0))
                self.tracker.pos(point + delta)
                self.tracker.setRotation(final_rot)

        def onPointPicked(self, point, obj=None):
            """Calculates the final alignment offset and restores the interface."""
            if point and hasattr(self, "_cached_basis"):
                u_basis, v_basis, _, center = self._cached_basis
                delta = point - center
                self.template.buffer.AlignmentOffset = FreeCAD.Vector(
                    delta.dot(u_basis), delta.dot(v_basis), 0
                )
                self.sb_u_off.setProperty(
                    "value", FreeCAD.Units.Quantity(delta.dot(u_basis), FreeCAD.Units.Length)
                )
                self.sb_v_off.setProperty(
                    "value", FreeCAD.Units.Quantity(delta.dot(v_basis), FreeCAD.Units.Length)
                )
                self.radio_custom.setChecked(True)

            self._cleanup_snapper()

        def _cleanup_snapper(self, arg=None):
            """Terminates the interaction loop and destroys trackers safely."""
            self.btn_pick.setChecked(False)

            # Clean up the event callback if it has not already been removed by a click.
            if hasattr(self, "_callback_id"):
                if hasattr(self, "_view"):
                    self._view.removeEventCallback("SoEvent", self._callback_id)
                del self._callback_id

            if hasattr(self, "tracker"):
                self.tracker.finalize()
                del self.tracker
            if hasattr(self, "_cached_basis"):
                del self._cached_basis
            if hasattr(self, "_pick_rotation_deg"):
                del self._pick_rotation_deg
            if hasattr(self, "_shortcut_filter"):
                QtGui.QApplication.instance().removeEventFilter(self._shortcut_filter)
                del self._shortcut_filter

            # Restore the UI and object state.
            self.geo_widget.setEnabled(True)
            self.vis_widget.setEnabled(True)
            if self.obj_to_edit:
                if hasattr(self, "_old_transparency"):
                    self.obj_to_edit.ViewObject.Transparency = self._old_transparency
                    self.obj_to_edit.ViewObject.Selectable = self._old_selectable

            FreeCAD.activeDraftCommand = None
            FreeCADGui.Snapper.off()

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
            params.set_param_arch("CoveringFinishMode", self.template.buffer.FinishMode)
            params.set_param_arch("CoveringAlignment", self.template.buffer.TileAlignment)
            params.set_param_arch("CoveringRotation", self.template.buffer.Rotation.Value)

            if self.template.buffer.FinishMode != "Hatch Pattern":
                params.set_param_arch("CoveringLength", self.template.buffer.TileLength.Value)
                params.set_param_arch("CoveringWidth", self.template.buffer.TileWidth.Value)
                params.set_param_arch("CoveringJoint", self.template.buffer.JointWidth.Value)
                params.set_param_arch("CoveringThickness", self.template.buffer.TileThickness.Value)

        def _sync_ui_to_target(self):
            """
            Manually transfers values from all UI widgets to the template buffer.

            This ensures that the template reflects the state of the dialog before any logic
            uses it.
            """
            obj = self.template.buffer

            # Sync numeric properties directly from the widget property. Use "value" to get the
            # unit-converted float value.
            obj.TileLength = self.sb_length.property("value")
            obj.TileWidth = self.sb_width.property("value")
            obj.TileThickness = self.sb_thick.property("value")
            obj.JointWidth = self.sb_joint.property("value")
            obj.Rotation = self.sb_rot.property("value")

            # Sync enum properties
            obj.FinishMode = self.combo_mode.currentText()
            if self.radio_custom.isChecked():
                obj.TileAlignment = "Custom"
            else:
                obj.TileAlignment = self.combo_align.currentText()

            # Preserve any dot-path expressions (e.g. "AlignmentOffset.x") that
            # ExpressionBinding wrote to the buffer when the user entered them via
            # the f(x) button. Writing a plain Vector clears those expressions, so
            # we snapshot them first and re-apply them after the write.
            _ao_exprs = {
                path: expr
                for path, expr in getattr(obj, "ExpressionEngine", [])
                if path in ("AlignmentOffset.x", "AlignmentOffset.y")
            }
            obj.AlignmentOffset = FreeCAD.Vector(
                self.sb_u_off.property("value").Value,
                self.sb_v_off.property("value").Value,
                0.0,
            )
            for path, expr in _ao_exprs.items():
                try:
                    obj.setExpression(path, expr)
                except Exception:
                    pass

            # Sync file paths
            # PropertyFileIncluded raises OSError if you assign the path it already holds
            # (source == destination). Guard both FileIncluded properties against no-op writes.
            if obj.FinishMode == "Hatch Pattern":
                new_pat = self.le_pat.text()
                if new_pat != obj.PatternFile:
                    obj.PatternFile = new_pat
                obj.PatternName = self.combo_pattern.currentText()
                obj.PatternScale = self.sb_scale_hatch.value()

            # Sync visual properties to the document object
            new_tex = self.le_tex_image.text()
            if new_tex != obj.TextureImage:
                obj.TextureImage = new_tex
            obj.TextureScale = FreeCAD.Vector(
                self.sb_tex_scale_u.value(), self.sb_tex_scale_v.value(), 0
            )

            if hasattr(obj, "StaggerType"):
                obj.StaggerType = self.combo_stagger.currentText()
                obj.StaggerCustom = self.sb_stagger_custom.property("value")

            obj.BorderSetback = self.sb_setback.property("value")

        def accept(self):
            """
            Process the dialog input and modify or create the Covering object.

            Transaction model
            -----------------
            Creation mode:
                BIM_Covering._launch_session() opened a transaction before the task panel was
                shown. The buffer was created inside it. On OK:
                  - The real covering object(s) are created inside the same transaction.
                  - doc.removeObject(buffer) is called inside the transaction. The C++ optimizer
                    sees create+delete for the buffer in the same transaction and erases both,
                    so the buffer never appears in the undo stack.
                  - The transaction is committed → one clean undo entry per OK press.
                Continue mode: after committing, a new transaction is opened immediately and a
                new buffer is created inside it for the next iteration.

            Edit mode:
                The C++ layer opened a transaction when setEdit was called. The buffer was
                created inside it. Same create+delete optimizer logic applies on OK.
                The C++ transaction is committed by FreeCAD after accept() returns True.
            """
            try:
                self._sync_ui_to_target()

                doc = FreeCAD.ActiveDocument

                if not self.obj_to_edit:
                    targets = self.selection_list
                    if not targets and self.selected_obj:
                        targets = (
                            [(self.selected_obj, [self.selected_sub])]
                            if self.selected_sub
                            else [self.selected_obj]
                        )

                    if targets:
                        for base in targets:
                            new_obj = Arch.makeCovering(base)
                            self.template.apply_to(new_obj)

                    self._save_user_preferences()

                    # Remove buffer inside the open transaction so the optimizer erases it.
                    doc.removeObject(self.template.buffer.Name)
                    self.template.destroy()

                    doc.recompute()
                    doc.commitTransaction()

                    if self.chk_continue.isChecked():
                        doc.openTransaction("Covering")
                        self.template = _CoveringTemplate()
                        self._rebind_to_buffer()
                        FreeCADGui.Selection.clearSelection()
                        self.selection_list = []
                        self.selected_obj = None
                        self.selected_sub = None
                        self._updateSelectionUI()
                        self.setPicking(True)
                        return False

                else:
                    self.template.apply_to(self.obj_to_edit)
                    self._save_user_preferences()
                    doc.removeObject(self.template.buffer.Name)
                    self.template.destroy()
                    doc.recompute()

            except Exception as e:
                FreeCAD.ActiveDocument.abortTransaction()
                import traceback

                traceback.print_exc()
                FreeCAD.Console.PrintError(f"Error updating covering: {e}\n")

            self._cleanup_and_close()
            return True

        def reject(self):
            """Cancels the session and removes the buffer from the document.

            In normal flow an open transaction exists and abortTransaction() rolls the buffer
            back automatically. However the panel may be constructed without a transaction
            (e.g. directly in tests). In that case abortTransaction() is a no-op and we must
            remove the buffer explicitly so no phantom object is left behind.
            """
            doc = FreeCAD.ActiveDocument
            doc.abortTransaction()
            if self.template and self.template.buffer:
                try:
                    if doc.getObject(self.template.buffer.Name):
                        doc.removeObject(self.template.buffer.Name)
                except (ReferenceError, RuntimeError):
                    pass
                self.template.buffer = None
            self._cleanup_and_close()

        def _cleanup_and_close(self):
            """Unregisters observers, drops the template reference, and closes the panel."""
            # If the panel is closed while interactive mode is active (e.g. Cancel button),
            # ensure the event callback, tracker, and event filter are all torn down cleanly.
            self._cleanup_snapper()
            self._unregister_observer()
            if self.template:
                self.template.destroy()
                self.template = None
            if self.obj_to_edit:
                FreeCADGui.ActiveDocument.resetEdit()
            FreeCADGui.Control.closeDialog()


class _CoveringTemplate:
    """
    Manages a hidden internal buffer for Arch Covering properties.

    This class decouples the UI state from the document's undo history, providing a sandbox for
    expressions and a master template for batch creation.
    """

    def __init__(self, name="CoveringTemplate"):
        # The buffer is created inside an already-open transaction (opened by BIM_Covering or
        # by the C++ setEdit mechanism). It will be removed before that transaction commits,
        # so the transaction optimizer cancels out the create+delete pair and the buffer
        # never appears in the undo stack.
        self.buffer = Arch.makeCovering(name=name)

        if self.buffer.ViewObject:
            self.buffer.ViewObject.ShowInTree = False
            self.buffer.ViewObject.hide()

    def destroy(self):
        """Drops the Python reference to the buffer.

        The actual C++ object is handled by the transaction system: the optimizer cancels
        the create+delete pair on commit (accept), and abortTransaction() rolls it back on
        cancel (reject). This method must never call doc.removeObject() directly.
        """
        self.buffer = None

    def _get_transferable_props(self, obj):
        """Returns a list of properties that can be safely copied."""
        system_props = [
            "Shape",
            "Proxy",
            "Label",
            "Base",
            "ExpressionEngine",
            "Placement",
            "Visibility",
            "ViewObject",
        ]
        props = []
        for prop in obj.PropertiesList:
            if prop in system_props:
                continue
            if "ReadOnly" in obj.getPropertyStatus(prop):
                continue
            props.append(prop)
        return props

    def copy_from(self, source):
        """Initializes the buffer with values and expressions from a source object."""
        for prop in self._get_transferable_props(source):
            setattr(self.buffer, prop, getattr(source, prop))

        # Transfer all expressions in a second pass, including dot-path sub-property
        # expressions such as "AlignmentOffset.x" and "AlignmentOffset.y". These are
        # skipped by a prop == path comparison in the loop above because the path
        # contains a dot and never matches a top-level property name.
        if hasattr(source, "ExpressionEngine"):
            for path, expr in source.ExpressionEngine:
                try:
                    self.buffer.setExpression(path, expr)
                except Exception:
                    pass

    def apply_to(self, target):
        """Transfers the buffer state (values and expressions) to a target object."""
        for prop in self._get_transferable_props(self.buffer):
            setattr(target, prop, getattr(self.buffer, prop))

        # Transfer all expressions in a second pass, including dot-path sub-property
        # expressions such as "AlignmentOffset.x" and "AlignmentOffset.y".
        if hasattr(self.buffer, "ExpressionEngine"):
            for path, expr in self.buffer.ExpressionEngine:
                try:
                    target.setExpression(path, expr)
                except Exception:
                    pass
