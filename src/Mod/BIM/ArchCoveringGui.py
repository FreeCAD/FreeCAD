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


def translate(context, sourceText, disambiguation=None, n=-1):
    return sourceText


def QT_TRANSLATE_NOOP(context, sourceText):
    return sourceText


if FreeCAD.GuiUp:
    from PySide import QtGui
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

            # Calculate Mapping
            if not vobj.TextureImage or not os.path.exists(vobj.TextureImage):
                return

            mapping = self._compute_texture_mapping(obj, vobj.DisplayMode)
            if not mapping:
                return

            dir_u, dir_v, s_offset, t_offset = mapping

            # Create Nodes
            texture_node = coin.SoTexture2()

            # Load Image
            img = self._get_cached_image(vobj.TextureImage)
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

            Parameters
            ----------
            obj : App::FeaturePython
                The Covering object.
            display_mode : str
                The current display mode ("Flat Lines", "Shaded", etc.).

            Returns
            -------
            tuple or None
                (dir_u, dir_v, s_offset, t_offset) where dir_* are FreeCAD.Vectors
                and *_offset are floats. Returns None if mapping cannot be computed.
            """
            vobj = obj.ViewObject

            # Geometry calculation (in Global Space, done once)
            base_face = Arch.getFaceGeometry(obj.Base)
            if not base_face:
                return None

            u_vec, v_vec, normal, center_point = Arch.getFaceUV(base_face)
            origin = Arch.getFaceGridOrigin(
                base_face, center_point, u_vec, v_vec, obj.TileAlignment, obj.AlignmentOffset
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

            # Apply Texture Rotation
            if hasattr(obj, "Rotation") and obj.Rotation.Value != 0:
                rot = FreeCAD.Rotation(calc_norm, obj.Rotation.Value)
                calc_u = rot.multVec(calc_u)
                calc_v = rot.multVec(calc_v)

            # Apply Texture Scaling
            scale_u = vobj.TextureScale.x if vobj.TextureScale.x != 0 else 1.0
            scale_v = vobj.TextureScale.y if vobj.TextureScale.y != 0 else 1.0

            # Calculate Period (Tile + Joint)
            period_u = (obj.TileLength.Value + obj.JointWidth.Value) * scale_u
            period_v = (obj.TileWidth.Value + obj.JointWidth.Value) * scale_v

            if period_u == 0:
                period_u = 1000.0
            if period_v == 0:
                period_v = 1000.0

            # Calculate Final Directions
            dir_u = calc_u.multiply(1.0 / period_u)
            dir_v = calc_v.multiply(1.0 / period_v)

            # Calculate Offsets
            s_offset = calc_origin.dot(dir_u)
            t_offset = calc_origin.dot(dir_v)

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

            # Handle selection list or single object
            if selection and not isinstance(selection, list):
                self.selection_list = [selection]
            else:
                self.selection_list = selection if selection else []

            # Initialize state from existing object or defaults
            if self.obj_to_edit:
                self.template.copy_from(self.obj_to_edit)
                self.stored_thickness = self.obj_to_edit.TileThickness.Value
            else:
                self.stored_thickness = params.get_param_arch("CoveringThickness")

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
                self._loadExistingData()
            else:
                # Note: widget defaults are loaded in _setupTilesPage
                pass

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
            self.combo_mode.setCurrentText(self.template.buffer.FinishMode)
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
            """Binds a quantity spinbox to the template buffer."""
            sb = FreeCADGui.UiLoader().createWidget("Gui::QuantitySpinBox")
            prop = getattr(self.template.buffer, prop_name)
            sb.setProperty("unit", prop.getUserPreferred()[2])
            sb.setToolTip(translate("Arch", tooltip))

            # This enables the f(x) icon, but we don't rely on it for value syncing anymore
            FreeCADGui.ExpressionBinding(sb).bind(self.template.buffer, prop_name)

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
            self.combo_align.setCurrentText(self.template.buffer.TileAlignment)
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
            mode = self.template.buffer.FinishMode
            self.combo_mode.setCurrentText(mode)
            self.onModeChanged(self.combo_mode.currentIndex())

            # Numerical values are auto-loaded by ExpressionBinding
            self.combo_align.setCurrentText(self.template.buffer.TileAlignment)

            self.le_pat.setText(self.template.buffer.PatternFile)

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
                    self.template.buffer.TileThickness = self.stored_thickness

                else:  # Parametric pattern mode
                    # Save current thickness before zeroing
                    if self.sb_thick.isEnabled():
                        self.stored_thickness = self.sb_thick.property("rawValue")

                    # Disable and zero
                    self.sb_thick.setEnabled(False)
                    self.sb_thick.setProperty("rawValue", 0.0)
                    self.template.buffer.TileThickness = 0.0

            self.template.buffer.FinishMode = self.combo_mode.currentText()

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
                if self.template.buffer.FinishMode == "Solid Tiles":
                    params.set_param_arch(
                        "CoveringThickness", self.template.buffer.TileThickness.Value
                    )

        def _sync_ui_to_target(self):
            """
            Manually transfers values from all UI widgets to the template buffer.

            This ensures that the template reflects the state of the dialog before any logic
            uses it.
            """
            obj = self.template.buffer

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
                # Sync all values from UI widgets (bound and unbound) to the template buffer
                self._sync_ui_to_target()

                # Open a new transaction for the document modification
                FreeCAD.ActiveDocument.openTransaction("Covering")

                # Prepare visual properties
                tex_image = self.le_tex_image.text()

                if not self.obj_to_edit:
                    # Creation mode

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

                            # Copy properties from the template buffer
                            self.template.apply_to(new_obj)

                            # Apply texture to view object
                            if hasattr(new_obj.ViewObject, "TextureImage"):
                                new_obj.ViewObject.TextureImage = tex_image
                else:
                    # Edition mode
                    self.template.apply_to(self.obj_to_edit)

                    # Apply texture to view object
                    if hasattr(self.obj_to_edit.ViewObject, "TextureImage"):
                        self.obj_to_edit.ViewObject.TextureImage = tex_image

                # Recompute the document inside the transaction to catch geometry errors
                FreeCAD.ActiveDocument.recompute()

                # Save user preferences for next run
                self._save_user_preferences()

                # Handle the 'continue' workflow
                if not self.obj_to_edit and self.chk_continue.isChecked():
                    # Commit the transaction so the current object is atomic in the Undo stack
                    FreeCAD.ActiveDocument.commitTransaction()
                    FreeCADGui.Selection.clearSelection()
                    self.selection_list = []
                    self.selected_obj = None
                    self.selected_sub = None
                    self._updateSelectionUI()
                    self.setPicking(True)
                    return False

                # Commit the transaction successfully
                FreeCAD.ActiveDocument.commitTransaction()

            except Exception as e:
                # Ensure transaction is closed on failure to prevent corruption
                FreeCAD.ActiveDocument.abortTransaction()
                import traceback

                traceback.print_exc()
                FreeCAD.Console.PrintError(f"Error updating covering: {e}\n")

            self._cleanup_and_close()
            return True

        def reject(self):
            """Terminates the session and cleans up the template."""
            self._cleanup_and_close()

        def _cleanup_and_close(self):
            """Removes temporary observers and the template buffer, then closes the task panel."""
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

        # Create the internal buffer object
        self.buffer = Arch.makeCovering(name=name)

        # Ensure the buffer is truly invisible to the user
        if self.buffer.ViewObject:
            self.buffer.ViewObject.ShowInTree = False
            self.buffer.ViewObject.hide()

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
            if hasattr(source, "ExpressionEngine"):
                for path, expr in source.ExpressionEngine:
                    if path == prop:
                        self.buffer.setExpression(prop, expr)
                        break

    def apply_to(self, target):
        """Transfers the buffer state (values and expressions) to a target object."""
        for prop in self._get_transferable_props(self.buffer):
            setattr(target, prop, getattr(self.buffer, prop))
            if hasattr(self.buffer, "ExpressionEngine"):
                for path, expr in self.buffer.ExpressionEngine:
                    if path == prop:
                        target.setExpression(prop, expr)
                        break

    def destroy(self):
        """Safely removes the buffer from the document."""
        doc = self.buffer.Document
        if doc and doc.getObject(self.buffer.Name):
            doc.removeObject(self.buffer.Name)
        self.buffer = None
