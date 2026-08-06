# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Copyright (c) 2026 Furgo
#
# This file is part of the FreeCAD Arch workbench.
# You can find the full license text in the LICENSE file in the root directory.

import os
import FreeCAD
import Arch
import ArchCovering
import ArchTessellation  # resolve_stagger() is there to avoid a circular import with ArchCovering

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
        """

        EDIT_MODE_STANDARD = 0
        DEFAULT_ROTATION_STEP = 15.0
        DEFAULT_TEXTURE_PERIOD = 1000.0
        BUTTON_ICON_SIZE = 16
        # Static cache to store loaded images (path -> SbImage).
        # This prevents reloading the same file from disk for every object.
        # No eviction: entries accumulate for the session lifetime. Acceptable for typical
        # use (a handful of texture files), but will grow if many distinct files are loaded.
        _texture_cache = {}

        def __init__(self, vobj):
            super().__init__(vobj)
            self.setProperties(vobj)
            self.texture = None
            self.texcoords = None

        def setProperties(self, vobj):
            """Defines and initializes the visual properties of the Covering."""
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
                vobj.PickRotationStep = self.DEFAULT_ROTATION_STEP

        def onDocumentRestored(self, vobj):
            self.setProperties(vobj)

        def getIcon(self):
            """
            Returns the icon representing the object in the Tree View.

            Returns
            -------
            str
                A string containing either the resource path to an SVG file (e.g.,
                ":/icons/BIM_Covering.svg") or a valid XPM-formatted image string.
            """
            if hasattr(self, "Object"):
                if hasattr(self.Object, "CloneOf"):
                    if self.Object.CloneOf:
                        return ":/icons/BIM_Covering_Clone.svg"
            return ":/icons/BIM_Covering_Tree.svg"

        def setEdit(self, vobj, mode=0):
            """
            Opens the task panel for edit mode 0; delegates other modes to C++ defaults.

            No transaction is opened here — see the class docstring for the transaction model.

            Parameters
            ----------
            vobj : Gui::ViewProviderPython
            mode : int
                * 0: opens the task panel.
                * 1: Transform gizmo.
                * 2: Cutting plane.
                * 3: Per-face color.

            Returns
            -------
            bool or None
                True if handled, None to delegate to C++.
            """
            if mode != self.EDIT_MODE_STANDARD:
                return None

            task_control = FreeCADGui.Control
            task_panel = task_control.activeDialog()

            # Prevent re-entrant calls
            if task_panel and isinstance(task_panel, ArchCoveringTaskPanel):
                return True

            task_control.showDialog(ArchCoveringTaskPanel(obj=vobj.Object))

            return True

        def getTransactionText(self):
            """
            C++ callback: supplies the undo stack label for the transaction opened before calling
            doubleClicked(). The default is "Edit"; this override supplies "Edit Covering"
            so the undo entry is specific to this object type, and is translatable.
            """
            return QT_TRANSLATE_NOOP("Command", "Edit Covering")

        def unsetEdit(self, _vobj, mode=0):
            """Closes the task panel when leaving edit mode 0; ignores other modes."""
            if mode != self.EDIT_MODE_STANDARD:
                return None

            task_control = FreeCADGui.Control

            task_control.closeDialog()

            return True

        def doubleClicked(self, _vobj):
            """Opens edit mode on double-click instead of toggling visibility."""
            self.edit()
            return True

        def attach(self, vobj):
            super().attach(vobj)
            self.Object = vobj.Object
            self.updateTexture(self.Object)

        def updateData(self, obj, prop):
            # Skip the parent call when Base is a tuple (linked sub-element): the parent class
            # does not support that form and would error.
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
                "StaggerType",
                "StaggerCustom",
            ]:
                self.updateTexture(obj)

        def updateTexture(self, obj):
            """Rebuild the scene-graph nodes that map a texture image onto the covering.

            Runs on property changes (TextureImage, TextureScale, geometry) to keep the Coin3D
            scene graph in sync with the document. Clears any previously attached texture nodes
            and, if a valid TextureImage is set, attaches fresh ones.
            """
            vobj = obj.ViewObject
            if not vobj or not vobj.RootNode:
                return

            # Guard against incomplete restore from file.
            if not hasattr(self, "texture"):
                self.texture = None

            target_node = self._find_flat_root(vobj)
            if not target_node:
                return

            had_texture_nodes = self._clear_texture_nodes(target_node)
            self.texture = None

            # Fast path: nothing to apply and nothing left over from before.
            if not obj.TextureImage and not had_texture_nodes:
                return

            if not obj.TextureImage or not os.path.exists(obj.TextureImage):
                return

            mapping = self._compute_texture_mapping(obj)
            if not mapping:
                return

            cycle, shift_per_row = self._resolve_stagger(obj)
            self._attach_texture_nodes(
                target_node, obj.TextureImage, cycle, shift_per_row, *mapping
            )

        def _find_flat_root(self, vobj):
            """Return the ``FlatRoot`` SoSeparator in the view provider's scene graph, or None.

            ``FlatRoot`` is the subtree FreeCAD renders for face geometry in both "Flat Lines"
            (composited with ``NormalRoot``) and "Shaded" (standalone) display modes. Targeting
            it means texture mapping works in either mode.
            """
            import pivy.coin as coin
            from draftutils import gui_utils

            switch = gui_utils.find_coin_node(vobj.RootNode, coin.SoSwitch)
            if not switch:
                return None

            for i in range(switch.getNumChildren()):
                child = switch.getChild(i)
                if child.getName().getString() == "FlatRoot":
                    return child
            return None

        def _clear_texture_nodes(self, target_node):
            """Remove any texture-mapping nodes previously attached to ``target_node``.

            Returns True if any nodes were removed, so callers can distinguish "nothing to do"
            from "had nodes, now cleared".
            """
            import pivy.coin as coin

            found_indices = []
            for i in range(target_node.getNumChildren()):
                child = target_node.getChild(i)
                if isinstance(
                    child,
                    (coin.SoTexture2, coin.SoTextureCoordinatePlane, coin.SoTexture2Transform),
                ):
                    found_indices.append(i)

            # Iterate in reverse so earlier indices stay valid as nodes are removed.
            for i in reversed(found_indices):
                target_node.removeChild(i)

            return bool(found_indices)

        def _attach_texture_nodes(
            self, target_node, image_path, cycle, shift_per_row, dir_u, dir_v, s_offset, t_offset
        ):
            """Build and insert the SoTexture2 / SoTextureCoordinatePlane / SoTexture2Transform
            triple that applies the texture image to ``target_node``.
            """
            import pivy.coin as coin

            texture_node = coin.SoTexture2()
            img = self._get_cached_image(image_path, cycle, shift_per_row)
            if img:
                texture_node.image = img
            else:
                texture_node.filename = image_path

            # REPLACE overrides the SoMaterial applied in Shaded mode, which would otherwise
            # wash out the texture colors.
            texture_node.model = coin.SoTexture2.REPLACE

            texcoords = coin.SoTextureCoordinatePlane()
            texcoords.directionS.setValue(coin.SbVec3f(dir_u.x, dir_u.y, dir_u.z))
            texcoords.directionT.setValue(coin.SbVec3f(dir_v.x, dir_v.y, dir_v.z))

            textrans = coin.SoTexture2Transform()
            textrans.translation.setValue(-s_offset, -t_offset)

            target_node.insertChild(texture_node, 0)
            target_node.insertChild(texcoords, 0)
            target_node.insertChild(textrans, 0)

        def _resolve_stagger(self, obj):
            """Return ``(cycle, shift_per_row)`` for the object's current stagger settings.

            ``shift_per_row`` is expressed as a fraction of the U period (tile length + joint).
            Delegates to ``ArchTessellation.resolve_stagger`` for the cycle and mm offset, then
            converts the offset to a fraction for texture image pre-baking.
            """
            period_u = obj.TileLength.Value + obj.JointWidth.Value
            cycle, offset_u = ArchTessellation.resolve_stagger(
                obj.StaggerType, obj.StaggerCustom.Value, obj.TileLength.Value, obj.JointWidth.Value
            )
            shift_per_row = (offset_u / period_u) if period_u > 0 else 0.0
            return (cycle, shift_per_row)

        def _get_cached_image(self, file_path, cycle, shift_per_row):
            """Return an SoSFImage for ``file_path``, optionally pre-composed to bake a stagger
            pattern of ``cycle`` rows each shifted by ``shift_per_row`` of the image width.

            Cache entries are keyed by (path, cycle, rounded shift) so different coverings sharing a
            texture file and bond pattern share the baked image, while dimension edits that change
            the derived shift fraction produce a fresh entry.
            """
            cache_key = (file_path, cycle, round(shift_per_row, 4))
            cached = _ViewProviderCovering._texture_cache.get(cache_key)
            if cached is not None:
                return cached

            qimage = QtGui.QImage(file_path)
            if qimage.isNull():
                return None
            if cycle > 1 and shift_per_row > 0:
                qimage = self._compose_staggered_qimage(qimage, cycle, shift_per_row)

            img = self._qimage_to_sosfimage(qimage)
            if img is not None:
                _ViewProviderCovering._texture_cache[cache_key] = img
            return img

        @staticmethod
        def _compose_staggered_qimage(src, cycle, shift_per_row):
            """Stack ``cycle`` copies of ``src`` vertically, shifting each by ``k * shift_per_row``
            of the width (with horizontal wrap-around). The result is tileable in both axes and,
            when projected with ``period_v = cycle * (tile_width + joint)``, reproduces the
            geometric bond pattern.
            """
            width = src.width()
            height = src.height()
            composed = QtGui.QImage(width, height * cycle, src.format())
            composed.fill(QtCore.Qt.transparent)

            painter = QtGui.QPainter(composed)
            try:
                for k in range(cycle):
                    shift_px = int(round(k * shift_per_row * width)) % width
                    y = k * height
                    painter.drawImage(shift_px, y, src)
                    if shift_px > 0:
                        painter.drawImage(shift_px - width, y, src)
            finally:
                painter.end()
            return composed

        @staticmethod
        def _qimage_to_sosfimage(qimage):
            """Convert a QImage to the SoSFImage format Coin3D expects.

            Mirrors the conversion in ``draftutils.gui_utils.load_texture`` but takes a QImage
            directly so callers can compose the image in Qt before handing it off to Coin.
            """
            import pivy.coin as coin

            width = qimage.width()
            height = qimage.height()
            if width == 0 or height == 0:
                return None

            buffer_size = qimage.sizeInBytes()
            components = int(buffer_size / (width * height))
            size = coin.SbVec2s(width, height)

            buf = bytearray()
            for y in range(height):
                for x in range(width):
                    rgba = qimage.pixel(x, y)
                    if components <= 2:
                        buf.append(QtGui.qGray(rgba))
                        if components == 2:
                            buf.append(QtGui.qAlpha(rgba))
                    else:
                        buf.append(QtGui.qRed(rgba))
                        buf.append(QtGui.qGreen(rgba))
                        buf.append(QtGui.qBlue(rgba))
                        if components == 4:
                            buf.append(QtGui.qAlpha(rgba))

            img = coin.SoSFImage()
            img.setValue(size, components, bytes(buf))
            return img

        def _compute_texture_mapping(self, obj):
            """
            Calculates the projection vectors and origin offset that align a texture image with
            the physical tile grid.

            Parameters
            ----------
            obj : App::FeaturePython
                The Covering object containing dimensional, alignment, and placement data.

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

            """
            # Use the same tiling face, frame, and grid origin as execute(), so that the texture
            # projection stays aligned with the physical tile grid.
            frame = ArchCovering.build_tiling_frame(obj)
            if frame is None:
                return None
            _, u_vec, v_vec, normal, origin = frame
            joint_width = obj.JointWidth.Value

            # Texture coordinates are generated by SoTextureCoordinatePlane from the shape's
            # object-space vertex positions — the raw world-space baked coordinates, unaffected
            # by obj.Placement. So the texture origin and axes must also be in world space.

            # Apply texture scaling
            scale_u = obj.TextureScale.x if obj.TextureScale.x != 0 else 1.0
            scale_v = obj.TextureScale.y if obj.TextureScale.y != 0 else 1.0

            # Period = tile + joint, so the texture repeats in sync with the tile grid. For
            # non-stacked bonds the baked texture spans ``cycle`` rows, so period_v is stretched
            # accordingly to keep one texture repeat aligned with one full stagger cycle.
            cycle, _ = self._resolve_stagger(obj)
            period_u = (obj.TileLength.Value + joint_width) * scale_u
            period_v = (obj.TileWidth.Value + joint_width) * scale_v * cycle

            if period_u == 0:
                period_u = self.DEFAULT_TEXTURE_PERIOD
            if period_v == 0:
                period_v = self.DEFAULT_TEXTURE_PERIOD

            # Calculate final directions
            dir_u = u_vec.multiply(1.0 / period_u)
            dir_v = v_vec.multiply(1.0 / period_v)

            # Calculate offsets
            s_offset = origin.dot(dir_u)
            t_offset = origin.dot(dir_v)

            # Center the texture on the tile body
            # Subtract half the joint width to pull the image back to the visual center
            s_offset -= (joint_width / 2.0) * scale_u / period_u  # Normalized
            t_offset -= (joint_width / 2.0) * scale_v / period_v

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
            A list of preselected objects or sub-elements to apply the covering to.

        Notes
        -----
        This class uses a buffered proxy pattern. Instead of binding UI widgets directly to the
        final BIM objects, it binds them to an internal, hidden _CoveringTemplate (the template).
        This enables:

        *   Batch creation: a single set of UI settings can be applied to multiple faces.
        *   Undo transparency: internal UI state changes do not pollute the document's undo history.
        *   Atomic continue: each object created in "Continue" mode is a separate undoable event.

        Data flow and binding:

        1.  **Initialization:** the UI widgets bind exclusively to the template buffer. If in
            Edit Mode, the template is initialized once with the values of the real object.
        2.  **Synchronization:** when the user interacts with the UI, the _sync_ui_to_target()
            method updates the properties of the template buffer.
        3.  **Transfer:** when accept() is called, the template's property values are copied
            to the real object(s) via template.apply_to(), the buffer is removed inside the
            open transaction, and the transaction is committed.
        4.  **Cleanup:** the template Python reference is dropped and the panel is closed.

        Transaction model:

        There are two entry paths with different transaction ownership.

        *Creation path* (BIM_Covering command): Activated() opens a "Create Covering"
        transaction before the panel is constructed, so the buffer's creation falls inside it
        immediately. On OK, the real object(s) are created and the buffer is deleted inside the
        same transaction — a transaction that creates and deletes the same object has no net
        effect, so the buffer never appears in the undo stack. The transaction is then committed,
        producing one clean undo entry. In Continue mode a new transaction and buffer are opened
        immediately for the next object. On Cancel, abortTransaction() rolls everything back.

        *Edit path* (double-click in Tree View): Tree.cpp opens an "Edit Covering" transaction
        before calling doubleClicked() — this is standard FreeCAD behaviour for any document
        object, using the label from getTransactionText(). setEdit() opens no transaction of its
        own. On OK the same create+delete cancellation applies to the buffer. The transaction is
        committed as a side-effect of leaving edit mode: _cleanup_and_close() calls
        FreeCADGui.ActiveDocument.resetEdit(), which triggers C++ _resetEdit(), which commits the
        booked transaction. On Cancel, abortTransaction() is called directly; the subsequent
        resetEdit() finds no active transaction and the commit is a no-op.
        """

        SCALE_MIN = 0.001
        # Upper bound for hatch pattern scale; large enough for any real-world file.
        SCALE_MAX = 1000000.0
        # Upper bound for the per-axis texture scale multiplier.
        TEX_SCALE_MAX = 1000.0
        BUTTON_ICON_SIZE = 16
        # Stacked-widget page indices — must match the order added in _setupGeometryStack.
        GEO_PAGE_TILES = 0
        GEO_PAGE_HATCH = 1
        GEO_PAGE_MONOLITHIC = 2
        # Combo-box index for the Hatch Pattern finish mode.
        FINISH_MODE_HATCH = 3
        # Textual values for enum-based comboboxes. Translated in the UI, saved in English in
        # the property values
        MODE_VALUES = [
            QT_TRANSLATE_NOOP("Arch", "Solid Tiles"),
            QT_TRANSLATE_NOOP("Arch", "Parametric Pattern"),
            QT_TRANSLATE_NOOP("Arch", "Monolithic"),
            QT_TRANSLATE_NOOP("Arch", "Hatch Pattern"),
        ]
        ALIGN_VALUES = [
            QT_TRANSLATE_NOOP("Arch", "Center"),
            QT_TRANSLATE_NOOP("Arch", "Top Left"),
            QT_TRANSLATE_NOOP("Arch", "Top Right"),
            QT_TRANSLATE_NOOP("Arch", "Bottom Left"),
            QT_TRANSLATE_NOOP("Arch", "Bottom Right"),
        ]
        STAGGER_VALUES = [
            QT_TRANSLATE_NOOP("Arch", "Stacked (None)"),
            QT_TRANSLATE_NOOP("Arch", "Half Bond (1/2)"),
            QT_TRANSLATE_NOOP("Arch", "Third Bond (1/3)"),
            QT_TRANSLATE_NOOP("Arch", "Quarter Bond (1/4)"),
            QT_TRANSLATE_NOOP("Arch", "Custom"),
        ]

        class _PickShortcutFilter(QtCore.QObject):
            """
            Qt event filter that intercepts the R key during interactive grid placement.

            Installed on all task panel form widgets so that R / Shift+R rotates the tile wireframe
            regardless of which widget currently has keyboard focus, without interfering with normal
            text entry in spinboxes or line edits.
            """

            def __init__(self, handler):
                super().__init__()
                self._handler = handler

            def eventFilter(self, _watched, event):
                if event.type() == QtCore.QEvent.KeyPress:
                    if event.text().upper() == "R":
                        self._handler(shift=bool(event.modifiers() & QtCore.Qt.ShiftModifier))
                        return True  # Consumed — do not pass to the focused widget
                return False

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

            # Resolve whole-object selections to their most visible face.
            resolved_selection = []
            view_dir = self._get_view_direction()

            for item in self.selection_list:
                if not isinstance(item, tuple):
                    face = Arch.pickMainFaceName(item, view_dir)
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
                self.le_selection.setText(tooltip_items[0])

            elif len(unique_objects) == 1 and total_faces > 0:
                # Multiple faces on the same object (e.g. Wall (3 faces))
                first_item = self.selection_list[0]
                obj_label = (
                    first_item[0].Label if isinstance(first_item, tuple) else first_item.Label
                )
                self.le_selection.setText(
                    translate("Arch", "%1 (%2 faces)")
                    .replace("%1", obj_label)
                    .replace("%2", str(total_faces))
                )

            else:
                # Multiple distinct objects or complex mixed selection
                self.le_selection.setText(
                    translate("Arch", "%1 objects selected").replace("%1", str(count))
                )

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

            # Build a two-state icon: full-colour record circle when checked (picking active),
            # auto-greyed version when unchecked. A single SVG source is enough because Qt generates
            # the Disabled-mode pixmap automatically.
            _rec_icon = QtGui.QIcon()
            _px_on = QtGui.QIcon(":/icons/media-record.svg").pixmap(
                self.BUTTON_ICON_SIZE, self.BUTTON_ICON_SIZE
            )
            _px_off = QtGui.QIcon(":/icons/media-record.svg").pixmap(
                self.BUTTON_ICON_SIZE, self.BUTTON_ICON_SIZE, QtGui.QIcon.Disabled
            )
            _rec_icon.addPixmap(_px_on, QtGui.QIcon.Normal, QtGui.QIcon.On)
            _rec_icon.addPixmap(_px_off, QtGui.QIcon.Normal, QtGui.QIcon.Off)
            self.btn_selection.setIcon(_rec_icon)

            self.btn_selection.toggled.connect(
                lambda checked: self.btn_selection.setText(
                    translate("Arch", "Picking…") if checked else translate("Arch", "Pick")
                )
            )
            self.btn_selection.toggled.connect(lambda _: self.update_hints())

            self._updateSelectionUI()

            h_sel.addWidget(self.le_selection)
            h_sel.addWidget(self.btn_selection)
            top_form.addRow(translate("Arch", "Base"), h_sel)

            # Mode
            self.combo_mode = QtGui.QComboBox()
            self.combo_mode.addItems([translate("Arch", v) for v in self.MODE_VALUES])
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
            self.combo_mode.setCurrentIndex(self.MODE_VALUES.index(self.template.buffer.FinishMode))
            self.combo_mode.currentIndexChanged.connect(self.onModeChanged)
            top_form.addRow(translate("Arch", "Mode"), self.combo_mode)

            # Thickness
            self.sb_thick = self._setup_bound_spinbox(
                "TileThickness", translate("Arch", "The thickness of the finish")
            )
            top_form.addRow(translate("Arch", "Thickness"), self.sb_thick)

            self.geo_layout.addLayout(top_form)

        def _setupGeometryStack(self):
            self.geo_stack = QtGui.QStackedWidget()

            self._setupTilesPage()
            self._setupHatchPage()
            self._setupMonolithicPage()

            self.geo_layout.addWidget(self.geo_stack)

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

            # Preset
            h_preset = QtGui.QHBoxLayout()
            self.radio_preset = QtGui.QRadioButton(translate("Arch", "Preset"))
            self.radio_preset.setToolTip(
                translate(
                    "Arch", "Use standard corner or center alignment relative to the boundary"
                )
            )
            self.combo_align = QtGui.QComboBox()
            self.combo_align.addItems([translate("Arch", v) for v in self.ALIGN_VALUES])
            self.combo_align.setToolTip(
                translate(
                    "Arch",
                    "Select which part of the usable boundary to anchor the pattern origin to",
                )
            )
            h_preset.addWidget(self.radio_preset)
            h_preset.addWidget(self.combo_align)
            vbox.addLayout(h_preset)

            # Custom
            h_custom = QtGui.QHBoxLayout()
            self.radio_custom = QtGui.QRadioButton(translate("Arch", "Custom"))
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
            form_offsets.addRow(translate("Arch", "U offset"), self.sb_u_off)
            form_offsets.addRow(translate("Arch", "V offset"), self.sb_v_off)
            vbox.addLayout(form_offsets)

            # Global rotation
            form_rot = QtGui.QFormLayout()
            self.sb_rot = self._setup_bound_spinbox(
                "Rotation", translate("Arch", "Manual rotation of the tile grid")
            )
            form_rot.addRow(translate("Arch", "Rotation"), self.sb_rot)
            vbox.addLayout(form_rot)

            grp_layout.setLayout(vbox)
            self.layout_layout.addWidget(grp_layout)

            # Connect signals
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
            form_bound.addRow(translate("Arch", "Border setback"), self.sb_setback)

            grp_bound.setLayout(form_bound)
            self.layout_layout.addWidget(grp_bound)

        def _setup_bound_spinbox(self, prop_name, tooltip=None, initial_value=None):
            """Creates a Gui::QuantitySpinBox bound to prop_name on the template buffer.

            The widget self-registers into self._bound_spinboxes so that _rebind_to_buffer() always
            has a complete inventory without manual maintenance.

            ExpressionBinding.bind() does not reliably populate the widget's display synchronously —
            it may fire asynchronously via Qt signals, meaning the widget can still show 0 when
            _sync_ui_to_target() reads it on accept(). We therefore always set the initial value
            explicitly before binding.

            For scalar properties, the value is read from the buffer via getattr(). For dot-path
            sub-properties (e.g. "AlignmentOffset.x"), getattr() cannot resolve sub-components, so
            the caller must supply initial_value explicitly.

            Parameters
            ----------
            prop_name : str
                Property name on the template buffer, or a dot-path for sub-components (e.g.
                "AlignmentOffset.x").
            tooltip : str, optional
                Already-translated string. Callers are responsible for wrapping in
                ``translate("Arch", ...)``.
            initial_value : FreeCAD.Units.Quantity, optional
                Explicit initial value. Required when prop_name contains a dot.
            """
            sb = FreeCADGui.UiLoader().createWidget("Gui::QuantitySpinBox")
            if tooltip:
                sb.setToolTip(tooltip)
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
            ExpressionBinding stores a direct C++ pointer to the bound object, so all bindings must
            be refreshed to point at the new buffer or the f(x) button will crash FreeCAD.
            """
            self.bindings = []
            for sb, prop_name in self._bound_spinboxes:
                binding = FreeCADGui.ExpressionBinding(sb)
                binding.bind(self.template.buffer, prop_name)
                self.bindings.append(binding)

        def _setupTilesPage(self):
            self.page_tiles = QtGui.QWidget()
            form = QtGui.QFormLayout()
            form.setContentsMargins(0, 0, 0, 0)

            # Tile Length
            self.sb_length = self._setup_bound_spinbox(
                "TileLength", translate("Arch", "The length of the tiles")
            )
            form.addRow(translate("Arch", "Length"), self.sb_length)

            # Tile Width
            self.sb_width = self._setup_bound_spinbox(
                "TileWidth", translate("Arch", "The width of the tiles")
            )
            form.addRow(translate("Arch", "Width"), self.sb_width)

            # Joint Width
            self.sb_joint = self._setup_bound_spinbox(
                "JointWidth", translate("Arch", "The width of the joints between tiles")
            )
            form.addRow(translate("Arch", "Joint width"), self.sb_joint)

            # Stagger
            h_stagger = QtGui.QHBoxLayout()
            self.combo_stagger = QtGui.QComboBox()
            self.combo_stagger.addItems([translate("Arch", v) for v in self.STAGGER_VALUES])
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
            form.addRow(translate("Arch", "Stagger"), h_stagger)

            self.page_tiles.setLayout(form)
            self.geo_stack.addWidget(self.page_tiles)

        def _setupHatchPage(self):
            self.page_hatch = QtGui.QWidget()
            form = QtGui.QFormLayout()
            form.setContentsMargins(0, 0, 0, 0)

            self.le_pat = QtGui.QLineEdit()
            self.le_pat.setToolTip(translate("Arch", "The PAT file to use for hatching"))

            btn_browse_pat = QtGui.QPushButton("...")
            btn_browse_pat.clicked.connect(self.browsePattern)
            h_pat = QtGui.QHBoxLayout()
            h_pat.addWidget(self.le_pat)
            h_pat.addWidget(btn_browse_pat)
            form.addRow(translate("Arch", "Pattern file"), h_pat)

            self.combo_pattern = QtGui.QComboBox()
            self.combo_pattern.setEditable(True)  # Allow custom input if parsing fails
            self.combo_pattern.setToolTip(translate("Arch", "The name of the pattern to use"))
            form.addRow(translate("Arch", "Pattern name"), self.combo_pattern)

            self.sb_scale_hatch = QtGui.QDoubleSpinBox()
            self.sb_scale_hatch.setRange(self.SCALE_MIN, self.SCALE_MAX)
            self.sb_scale_hatch.setDecimals(2)
            self.sb_scale_hatch.setToolTip(translate("Arch", "The scale of the hatch pattern"))
            self.sb_scale_hatch.lineEdit().returnPressed.connect(self.accept)
            form.addRow(translate("Arch", "Pattern scale"), self.sb_scale_hatch)

            self.page_hatch.setLayout(form)
            self.geo_stack.addWidget(self.page_hatch)

        def _setupMonolithicPage(self):
            self.page_mono = QtGui.QWidget()
            form = QtGui.QFormLayout(self.page_mono)
            form.setContentsMargins(0, 0, 0, 0)

            self.sb_length_mono = self._setup_bound_spinbox(
                "TileLength", translate("Arch", "Texture repeat interval along U")
            )
            self.sb_width_mono = self._setup_bound_spinbox(
                "TileWidth", translate("Arch", "Texture repeat interval along V")
            )

            form.addRow(translate("Arch", "Length"), self.sb_length_mono)
            form.addRow(translate("Arch", "Width"), self.sb_width_mono)

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
            visual_form.addRow(translate("Arch", "Texture image"), h_tex)

            # Texture scaling multiplier for visual fine-tuning
            h_scale = QtGui.QHBoxLayout()
            self.sb_tex_scale_u = QtGui.QDoubleSpinBox()
            self.sb_tex_scale_u.setRange(self.SCALE_MIN, self.TEX_SCALE_MAX)
            self.sb_tex_scale_u.setSingleStep(0.1)
            self.sb_tex_scale_u.setToolTip(translate("Arch", "Horizontal texture multiplier"))
            self.sb_tex_scale_v = QtGui.QDoubleSpinBox()
            self.sb_tex_scale_v.setRange(self.SCALE_MIN, self.TEX_SCALE_MAX)
            self.sb_tex_scale_v.setSingleStep(0.1)
            self.sb_tex_scale_v.setToolTip(translate("Arch", "Vertical texture multiplier"))
            h_scale.addWidget(self.sb_tex_scale_u)
            h_scale.addWidget(self.sb_tex_scale_v)
            visual_form.addRow(translate("Arch", "Texture scale"), h_scale)

        def _loadExistingData(self):
            # Sync the combobox and force the stacked widget to the correct page
            self.combo_mode.setCurrentIndex(self.MODE_VALUES.index(self.template.buffer.FinishMode))
            self.onModeChanged(self.combo_mode.currentIndex())

            # Initialize radio button state based on the current alignment mode
            is_custom = self.template.buffer.TileAlignment == "Custom"
            self.radio_custom.setChecked(is_custom)
            self.radio_preset.setChecked(not is_custom)

            if not is_custom:
                self.combo_align.setCurrentIndex(
                    self.ALIGN_VALUES.index(self.template.buffer.TileAlignment)
                )
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
            self.combo_stagger.setCurrentIndex(
                self.STAGGER_VALUES.index(self.template.buffer.StaggerType)
            )
            # Trigger handler to update enabled state of custom box
            self.onStaggerChanged(self.combo_stagger.currentIndex())

            # Visuals
            if self.template.buffer.TextureImage:
                self.le_tex_image.setText(self.template.buffer.TextureImage)

            self.sb_tex_scale_u.setValue(self.template.buffer.TextureScale.x)
            self.sb_tex_scale_v.setValue(self.template.buffer.TextureScale.y)

        def browseTexture(self):
            filename = QtGui.QFileDialog.getOpenFileName(
                self.vis_widget,
                translate("Arch", "Select Texture"),
                "",
                "Images (*.png *.jpg *.jpeg *.bmp)",
            )[0]
            if filename:
                self.le_tex_image.setText(filename)

        def browsePattern(self):
            filename = QtGui.QFileDialog.getOpenFileName(
                self.geo_widget, translate("Arch", "Select Pattern"), "", "Pattern files (*.pat)"
            )[0]
            if filename:
                self.le_pat.setText(filename)
                self.updatePatterns(filename)

        def updatePatterns(self, filename):
            self.combo_pattern.clear()
            for name in Arch.read_pat_pattern_names(filename):
                self.combo_pattern.addItem(name)
            if self.combo_pattern.count() > 0:
                self.combo_pattern.setCurrentIndex(0)

        def onModeChanged(self, index):
            """Updates the UI layout based on the selected Finish Mode."""
            if index == self.GEO_PAGE_MONOLITHIC:
                # Monolithic uses the dedicated texture period page
                self.geo_stack.setCurrentIndex(self.GEO_PAGE_MONOLITHIC)
            elif index == self.FINISH_MODE_HATCH:
                # Hatch Pattern uses the Hatch Page
                self.geo_stack.setCurrentIndex(self.GEO_PAGE_HATCH)
            else:
                # Solid Tiles or Parametric Pattern share the Tiles Page
                self.geo_stack.setCurrentIndex(self.GEO_PAGE_TILES)

            # Disable visuals (textures) only for Hatch Pattern
            self.vis_widget.setEnabled(index != self.FINISH_MODE_HATCH)

            self.template.buffer.FinishMode = self.MODE_VALUES[self.combo_mode.currentIndex()]

        def onStaggerChanged(self, _index):
            """Enables or disables the custom stagger input based on selection."""
            is_custom = self.STAGGER_VALUES[self.combo_stagger.currentIndex()] == "Custom"
            self.sb_stagger_custom.setEnabled(is_custom)

        def _get_view_direction(self):
            """Safely retrieve the view direction if the GUI is active."""
            if FreeCAD.GuiUp and FreeCADGui.ActiveDocument:
                view = FreeCADGui.ActiveDocument.ActiveView
                if hasattr(view, "getViewDirection"):
                    return view.getViewDirection()
            return None

        def isPicking(self):
            return self.btn_selection.isChecked()

        def setPicking(self, state):
            self.btn_selection.setChecked(state)
            if state:
                # Sync immediately when enabled so selection_list reflects the current viewport
                # selection without waiting for the next observer event.
                self._onSelectionChanged()
            self.update_hints()

        # Selection Observer methods — required.
        # All four delegate to a single reader because the panel only cares about the complete
        # current selection, not which delta just occurred.
        def addSelection(self, _doc, _obj, _sub, _pos):
            self._onSelectionChanged()

        def removeSelection(self, _doc, _obj, _sub):
            self._onSelectionChanged()

        def setSelection(self, _doc):
            self._onSelectionChanged()

        def clearSelection(self, _doc):
            self._onSelectionChanged()

        def _onSelectionChanged(self):
            """Syncs internal state with FreeCAD selection."""
            if not self.isPicking():
                return

            # Get Standard Selection
            sel = FreeCADGui.Selection.getSelectionEx()

            # Process into internal list structure
            new_list = []
            for sel_item in sel:
                obj = sel_item.Object

                # If the user picks the template or the object being edited, ignore it.
                if obj == self.template.buffer or obj == self.obj_to_edit:
                    continue

                obj = Arch.resolve_pd_object(obj)

                if sel_item.SubElementNames:
                    for sub in sel_item.SubElementNames:
                        if sub.startswith("Face"):
                            new_list.append((obj, [sub]))
                else:
                    # No sub-element: resolve to the most visible face.
                    view_dir = self._get_view_direction()
                    best_face = Arch.pickMainFaceName(obj, view_dir)

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

        def _resolve_base_ref(self):
            """Returns the base reference for the current mode (edit or creation)."""
            return (
                self.obj_to_edit.Base
                if self.obj_to_edit
                else (self.selection_list[0] if self.selection_list else None)
            )

        def _set_rotation(self, angle_deg):
            """Writes a rotation value to the buffer and syncs the spinbox display."""
            self.template.buffer.Rotation = angle_deg
            self.sb_rot.setProperty("value", FreeCAD.Units.Quantity(angle_deg, FreeCAD.Units.Angle))

        def _set_custom_offset(self, u_off, v_off):
            """Writes custom alignment offsets to the buffer, spinboxes, and radio button."""
            self.template.buffer.AlignmentOffset = FreeCAD.Vector(u_off, v_off, 0)
            self.sb_u_off.setProperty("value", FreeCAD.Units.Quantity(u_off, FreeCAD.Units.Length))
            self.sb_v_off.setProperty("value", FreeCAD.Units.Quantity(v_off, FreeCAD.Units.Length))
            self.radio_custom.setChecked(True)

        def onMatchWP(self):
            """Snapshots the current working plane into the covering properties."""
            import WorkingPlane
            import DraftGeomUtils
            import DraftVecUtils
            import math

            base_face = Arch.resolveFace(self._resolve_base_ref())
            if not base_face:
                return

            wp = WorkingPlane.get_working_plane()
            reference_direction = None
            if self.obj_to_edit:
                reference_direction = ArchCovering.get_reference_direction(self.obj_to_edit)
            u_basis, v_basis, normal, center = Arch.getFaceFrame(base_face, reference_direction)

            # Determine rotation angle between the working plane U axis and the face local frame.
            wp_u_proj = wp.u - normal * wp.u.dot(normal)
            # Skip if the WP u-axis is nearly perpendicular to the face (degenerate projection).
            if wp_u_proj.Length > 0.001:
                angle_deg = math.degrees(DraftVecUtils.angle(u_basis, wp_u_proj, normal))

                # Write to the buffer directly: setProperty("value", float) on a
                # Gui::QuantitySpinBox updates the visual display but does not update the widget's
                # internal Quantity object, so a bare float setProperty is not reliably picked up by
                # _sync_ui_to_target. Writing to the buffer first guarantees the value survives
                # accept().
                self._set_rotation(angle_deg)

            # Project the working plane origin onto the face to determine the offset.
            pt_on_face = DraftGeomUtils.project_point_on_plane(wp.position, center, normal)
            delta = pt_on_face - center
            self._set_custom_offset(delta.dot(u_basis), delta.dot(v_basis))

        def onPickOrigin(self, state):
            """Initializes or terminates the interactive snapper loop."""
            if not state:
                self._cleanup_snapper()
                return

            if not self._cache_base_frame():
                return
            self._init_pick_rotation()
            self._install_pick_tracker()
            self._install_shortcut_filter()
            self._install_view_callback()
            self._lock_ui_and_set_transparency()
            self._wake_snapper()
            self.update_hints()

        def _cache_base_frame(self):
            """Resolve the base face and cache its frame for the duration of the pick.

            The cached frame is used by onMouseMove and onPointPicked. On failure, it releases the
            pick button and prints an error message before returning False.
            """
            base_face = Arch.resolveFace(self._resolve_base_ref())
            if not base_face:
                FreeCAD.Console.PrintError(
                    translate("Arch", "Could not resolve base geometry.") + "\n"
                )
                self.btn_pick.setChecked(False)
                self.update_hints()
                return False

            reference_direction = None
            if self.obj_to_edit:
                reference_direction = ArchCovering.get_reference_direction(self.obj_to_edit)
            self._cached_basis = Arch.getFaceFrame(base_face, reference_direction)
            return True

        def _init_pick_rotation(self):
            """Initializes the rotation counter from the current spinbox value.

            Reading directly from the UI ensures that any manually typed angle is captured, even if
            the interactive button is clicked before the input field has lost focus to update the
            underlying property buffer.

            """
            try:
                self._pick_rotation_deg = self.sb_rot.property("value").Value
            except Exception:
                self._pick_rotation_deg = self.template.buffer.Rotation.Value

        def _install_pick_tracker(self):
            """Create and enable the lead-tile wireframe tracker that follows the cursor."""
            import draftguitools.gui_trackers as trackers

            self.tracker = trackers.boxTracker()
            self.tracker.length(self.template.buffer.TileLength.Value)
            self.tracker.width(self.template.buffer.TileWidth.Value)
            self.tracker.height(self.template.buffer.TileThickness.Value)
            self.tracker.on()

        def _install_shortcut_filter(self):
            """Install a Qt application-level event filter to intercept the R rotation shortcut.

            Coin3D's SoKeyboardEvent is not processed when a Qt widget has focus, so the event
            filter is the only reliable way to intercept it. Installing the filter at the
            application level catches R regardless of which child widget has keyboard focus.
            """

            def _rotation_handler(shift=False):
                try:
                    step = self.template.buffer.ViewObject.PickRotationStep.Value
                except Exception:
                    step = _ViewProviderCovering.DEFAULT_ROTATION_STEP
                if shift:
                    step = -step
                self._pick_rotation_deg = (getattr(self, "_pick_rotation_deg", 0.0) + step) % 360.0
                self._set_rotation(self._pick_rotation_deg)
                if hasattr(self, "_pt"):
                    self.onMouseMove(self._pt, None)

            self._shortcut_filter = self._PickShortcutFilter(_rotation_handler)
            QtGui.QApplication.instance().installEventFilter(self._shortcut_filter)

        def _install_view_callback(self):
            """Register the SoEvent callback that drives the interaction loop.

            Using a view callback instead of a modal command keeps the task panel active.
            """
            self._view = FreeCADGui.ActiveDocument.ActiveView
            self._callback_id = self._view.addEventCallback("SoEvent", self._handle_interaction)

        def _lock_ui_and_set_transparency(self):
            """Disables task panel widgets and applies transparency to the edited covering.

            The covering is set to 70% transparency and made non-selectable, allowing the user to
            see and click through to the base geometry. Original visual settings are stored so they
            can be restored during cleanup.
            """
            self.geo_widget.setEnabled(False)
            self.vis_widget.setEnabled(False)
            if self.obj_to_edit:
                self._old_transparency = self.obj_to_edit.ViewObject.Transparency
                self._old_selectable = self.obj_to_edit.ViewObject.Selectable
                self.obj_to_edit.ViewObject.Transparency = 70
                self.obj_to_edit.ViewObject.Selectable = False

        def _wake_snapper(self):
            """Wake up the Draft Snapper visuals (grid and snap markers)."""
            FreeCAD.activeDraftCommand = self
            if hasattr(FreeCADGui, "Snapper"):
                FreeCADGui.Snapper.show()
                FreeCADGui.Snapper.setTrackers()

        def _handle_interaction(self, arg):
            """Processes low-level events for snapping and picking."""
            from draftutils.todo import ToDo

            # Handle movement to update the snapping position and visual tracker.
            if arg["Type"] == "SoLocation2Event":
                ctrl = arg["CtrlDown"]
                shift = arg["ShiftDown"]
                # Invoke the snapper manually to allow standard keyboard modifiers.
                self._pt = FreeCADGui.Snapper.snap(arg["Position"], active=ctrl, constrain=shift)
                self.onMouseMove(self._pt, None)

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
                ToDo.delay(self.onPointPicked, self._pt)

            # Handle escape to cancel the operation.
            elif arg["Type"] == "SoKeyboardEvent" and arg["Key"] == "ESCAPE":
                ToDo.delay(self._cleanup_snapper, None)

        def onMouseMove(self, point, _info):
            """Updates the lead-tile tracker following the cursor."""
            if point and hasattr(self, "tracker") and hasattr(self, "_cached_basis"):
                u_basis, v_basis, normal, _ = self._cached_basis
                length, width, thickness = (
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

                # Derive the rotated u/v axes so the anchor offset uses the visually correct
                # directions. Without this, the bottom-left corner drifts away from the cursor when
                # a pick rotation is applied.
                u_rotated = final_rot.multVec(FreeCAD.Vector(1, 0, 0))
                v_rotated = final_rot.multVec(FreeCAD.Vector(0, 1, 0))

                # Apply an offset so the cursor tracks the bottom-left corner of the box.
                delta = (
                    (u_rotated * (length / 2.0))
                    + (v_rotated * (width / 2.0))
                    + (normal * (thickness / 2.0))
                )
                self.tracker.pos(point + delta)
                self.tracker.setRotation(final_rot)

        def onPointPicked(self, point, _obj=None):
            """Calculates the final alignment offset and restores the interface."""
            if point and hasattr(self, "_cached_basis"):
                u_basis, v_basis, _, center = self._cached_basis
                delta = point - center
                self._set_custom_offset(delta.dot(u_basis), delta.dot(v_basis))

            self._cleanup_snapper()

        def _cleanup_snapper(self, _arg=None):
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
            self.vis_widget.setEnabled(self.combo_mode.currentIndex() != self.FINISH_MODE_HATCH)
            if self.obj_to_edit:
                if hasattr(self, "_old_transparency"):
                    self.obj_to_edit.ViewObject.Transparency = self._old_transparency
                    self.obj_to_edit.ViewObject.Selectable = self._old_selectable

            FreeCAD.activeDraftCommand = None
            FreeCADGui.Snapper.off()
            self.update_hints()

        def get_hints(self):
            if self.btn_pick.isChecked():
                return [
                    FreeCADGui.InputHint(
                        translate("Arch", "%1 pick tile origin"),
                        FreeCADGui.UserInput.MouseLeft,
                    ),
                    FreeCADGui.InputHint(
                        translate("Arch", "%1 rotate tile CW / Shift+%1 rotate tile CCW"),
                        FreeCADGui.UserInput.KeyR,
                    ),
                ]
            if self.isPicking():
                if self.obj_to_edit:
                    return [
                        FreeCADGui.InputHint(
                            translate("Arch", "%1 pick new base face or object"),
                            FreeCADGui.UserInput.MouseLeft,
                        ),
                        FreeCADGui.InputHint(
                            translate("Arch", "%1+%2 add face or object"),
                            FreeCADGui.UserInput.KeyControl,
                            FreeCADGui.UserInput.MouseLeft,
                        ),
                    ]
                return [
                    FreeCADGui.InputHint(
                        translate("Arch", "%1 pick planar face or object"),
                        FreeCADGui.UserInput.MouseLeft,
                    ),
                    FreeCADGui.InputHint(
                        translate("Arch", "%1+%2 add planar face or object"),
                        FreeCADGui.UserInput.KeyControl,
                        FreeCADGui.UserInput.MouseLeft,
                    ),
                ]
            return []

        def update_hints(self):
            hints = self.get_hints()
            if hints:
                FreeCADGui.HintManager.show(*hints)
            else:
                QtCore.QTimer.singleShot(0, FreeCADGui.HintManager.hide)

        def getStandardButtons(self):
            return QtGui.QDialogButtonBox.Ok | QtGui.QDialogButtonBox.Cancel

        def _unregister_observer(self, _val=None):
            """Safely remove self from selection observers."""
            try:
                FreeCADGui.Selection.removeObserver(self)
            except Exception:
                # Observer might already be removed, ignore
                pass

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
            """Transfers values from all UI widgets to the template buffer."""
            obj = self.template.buffer

            # Sync numeric properties directly from the widget property. Use "value" to get the
            # unit-converted float value.
            obj.TileLength = self.sb_length.property("value")
            obj.TileWidth = self.sb_width.property("value")
            obj.TileThickness = self.sb_thick.property("value")
            obj.JointWidth = self.sb_joint.property("value")
            obj.Rotation = self.sb_rot.property("value")

            # Sync enum properties
            obj.FinishMode = self.MODE_VALUES[self.combo_mode.currentIndex()]
            if self.radio_custom.isChecked():
                obj.TileAlignment = "Custom"
            else:
                obj.TileAlignment = self.ALIGN_VALUES[self.combo_align.currentIndex()]

            # Preserve any dot-path expressions (e.g. "AlignmentOffset.x") that ExpressionBinding
            # wrote to the buffer when the user entered them via the f(x) button. Writing a plain
            # Vector clears those expressions, so they are snapshot first and reapplied after the
            # write.
            alignment_offset_exprs = {
                path: expr
                for path, expr in getattr(obj, "ExpressionEngine", [])
                if path in ("AlignmentOffset.x", "AlignmentOffset.y")
            }
            obj.AlignmentOffset = FreeCAD.Vector(
                self.sb_u_off.property("value").Value,
                self.sb_v_off.property("value").Value,
                0.0,
            )
            for path, expr in alignment_offset_exprs.items():
                try:
                    obj.setExpression(path, expr)
                except Exception as e:
                    FreeCAD.Console.PrintWarning(
                        f"ArchCovering [{obj.Label}]: failed to restore expression on "
                        f"{path} ({expr!r}): {e}\n"
                    )

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

            obj.StaggerType = self.STAGGER_VALUES[self.combo_stagger.currentIndex()]
            obj.StaggerCustom = self.sb_stagger_custom.property("value")

            obj.BorderSetback = self.sb_setback.property("value")

        def accept(self):
            """
            Process the dialog input and modify or create the Covering object.

            See the class docstring for the transaction model.
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

                    # Remove the buffer inside the open transaction so the create and delete records
                    # cancel out.
                    doc.removeObject(self.template.buffer.Name)
                    self.template.destroy()

                    doc.recompute()
                    doc.commitTransaction()

                    if self.chk_continue.isChecked():
                        doc.openTransaction(QT_TRANSLATE_NOOP("Command", "Create Covering"))
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
            """Cancels the operation and rolls back all changes made during the panel's lifetime."""
            doc = FreeCAD.ActiveDocument

            # Aborts the open transaction (see class docstring). No-op when there is none.
            doc.abortTransaction()
            if self.template and self.template.buffer:
                try:
                    # If abortTransaction() was a no-op, the buffer is still in the document and
                    # must be removed explicitly. The getObject() guard makes this safe either way:
                    # if the transaction did roll it back, getObject() returns None.
                    if doc.getObject(self.template.buffer.Name):
                        doc.removeObject(self.template.buffer.Name)
                except (ReferenceError, RuntimeError):
                    # The buffer was already removed; the desired end state is reached.
                    pass
                self.template.buffer = None
            self._cleanup_and_close()

        def _cleanup_and_close(self):
            """Unregisters observers, drops the template reference, and closes the panel."""
            # If the panel is closed while interactive mode is active (e.g. Cancel button),
            # ensure the event callback, tracker, and event filter are all torn down cleanly.
            self._cleanup_snapper()
            # Ensure hints are hidden regardless of the state _cleanup_snapper left behind.
            # This is queued after any deferred show/hide that _cleanup_snapper may have posted.
            QtCore.QTimer.singleShot(0, FreeCADGui.HintManager.hide)
            self._unregister_observer()
            if self.template:
                self.template.destroy()
                self.template = None
            if self.obj_to_edit:
                FreeCADGui.ActiveDocument.resetEdit()
            # FreeCAD's task panel infrastructure orphans (unparents) the form widgets on close
            # rather than deleting them. Schedule deletion now so Qt destroys them while the event
            # dispatcher is still alive.
            for widget in self.form:
                widget.deleteLater()
            FreeCADGui.Control.closeDialog()


class _CoveringTemplate:
    """
    Manages a hidden internal buffer for Arch Covering properties.

    It is a hidden Covering object that the task panel reads from and writes to, keeping all
    intermediate edits out of the user-visible undo history. Because ``Gui::QuantitySpinBox`` with
    ``ExpressionBinding`` requires a real FreeCAD document object to bind to, the buffer is a live
    (but hidden) Covering object. Expressions entered via the ``f(x)`` button are stored on the
    buffer's ``ExpressionEngine`` and only transferred to the real object on accept. The buffer also
    acts as a master template for batch creation.

    The buffer is created inside an already-open transaction (opened when creating a new covering
    or when entering edit mode on an existing one) and removed before that transaction commits.
    FreeCAD's transaction system tracks each object's lifecycle: when ``doc.removeObject()`` is
    called on an object that was created in the same still-open transaction, the create and delete
    records cancel each other out and both are dropped entirely. The buffer therefore never appears
    in the undo stack.
    """

    def __init__(self, name="CoveringTemplate"):
        self.buffer = Arch.makeCovering(name=name)

        if self.buffer.ViewObject:
            self.buffer.ViewObject.ShowInTree = False
            self.buffer.ViewObject.hide()

    def destroy(self):
        """Drops the Python reference to the buffer object.

        The C++ object's lifecycle is handled externally:
        - On accept: the caller removes it via doc.removeObject() before calling this, so the
          create and delete records cancel out and the buffer never appears in the undo stack.
        - On cancel: abortTransaction() rolls back the entire transaction, including the creation.
        """
        self.buffer = None

    # Properties that are never copied between covering objects.
    _NON_TRANSFERABLE_PROPERTIES = frozenset(
        {
            # Unique to each object, so copying them would break identity.
            "Label",
            "Placement",
            "Base",  # links this covering's own host
            # Other:
            "Proxy",  # Python object slot, not document data
            "Visibility",  # display state
            "ExpressionEngine",  # copied separately in _transfer so dot-path expressions round-trip
            "ReferenceDirection",  # auto-seeded per object by Covering.execute()
        }
    )

    def _get_transferable_props(self, obj):
        """Returns the properties that can be copied between covering objects with setattr.

        Used to initialize the buffer from an existing object (edit mode) and to write the buffer's
        values back to the real object on accept.
        """
        props = []
        for prop in obj.PropertiesList:
            if prop in self._NON_TRANSFERABLE_PROPERTIES:
                continue
            status = obj.getPropertyStatus(prop)
            if "ReadOnly" in status or "Output" in status:
                continue
            props.append(prop)
        return props

    def _transfer(self, source, target):
        """Copies transferable property values and expressions from source to target.

        Expressions are handled in a second pass because dot-path sub-property expressions (e.g.
        "AlignmentOffset.x") never match a top-level property name and would be silently skipped by
        the first loop.
        """
        for prop in self._get_transferable_props(source):
            setattr(target, prop, getattr(source, prop))

        if hasattr(source, "ExpressionEngine"):
            for path, expr in source.ExpressionEngine:
                try:
                    target.setExpression(path, expr)
                except Exception as e:
                    FreeCAD.Console.PrintWarning(
                        f"Covering: could not transfer expression for '{path}': {e}\n"
                    )

    def copy_from(self, source):
        """Initializes the buffer with values and expressions from a source object."""
        self._transfer(source, self.buffer)

    def apply_to(self, target):
        """Copies the buffer's values and expressions to a target object."""
        self._transfer(self.buffer, target)
