# SPDX-License-Identifier: LGPL-2.1-or-later
# /**************************************************************************
#                                                                           *
#    Copyright (c) 2026 AstoCAD <hello@astocad.com>                         *
#                                                                           *
#    This file is part of FreeCAD.                                          *
#                                                                           *
#    FreeCAD is free software: you can redistribute it and/or modify it     *
#    under the terms of the GNU Lesser General Public License as            *
#    published by the Free Software Foundation, either version 2.1 of the   *
#    License, or (at your option) any later version.                        *
#                                                                           *
#    FreeCAD is distributed in the hope that it will be useful, but         *
#    WITHOUT ANY WARRANTY; without even the implied warranty of             *
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
#    Lesser General Public License for more details.                        *
#                                                                           *
#    You should have received a copy of the GNU Lesser General Public       *
#    License along with FreeCAD. If not, see                                *
#    <https://www.gnu.org/licenses/>.                                       *
#                                                                           *
# **************************************************************************/

import os
import math
import FreeCAD as App

if App.GuiUp:
    import FreeCADGui as Gui
    from PySide import QtCore, QtGui, QtWidgets

import FastenerLinkObject
import UtilsAssembly

# Only active if Fasteners workbench is installed
try:
    import FastenerBase
    import ScrewMaker
    import FastenersCmd
    import FSutils
    from FSAliases import FSGetIconAlias

    FASTENERS_AVAILABLE = True
except ImportError:
    FASTENERS_AVAILABLE = False

translate = App.Qt.translate


def isFastener(obj):
    """Whether the object is a fastener of the Fasteners workbench"""
    return (
        FASTENERS_AVAILABLE
        and obj is not None
        and isinstance(getattr(obj, "Proxy", None), FastenerBase.FSBaseObject)
    )


if App.GuiUp:

    class DeleteKeyFilter(QtCore.QObject):
        """Calls back when Delete is pressed in the watched widget"""

        def __init__(self, parent, callback):
            super().__init__(parent)
            self.callback = callback

        def eventFilter(self, watched, event):
            if event.type() == QtCore.QEvent.KeyPress and event.key() == QtCore.Qt.Key_Delete:
                self.callback()
                return True
            return super().eventFilter(watched, event)


class TaskAssemblyInsertFastener:
    """Inserts fasteners in an assembly, or edits the fasteners of a fastener link.

    The fasteners of one use of the tool are one fastener link (see FastenerLinkObject).
    Everything is done in the document as the user goes, in a transaction that is committed
    or aborted when the task ends.
    """

    def __init__(self, assembly, view, link=None):
        self.assembly = assembly
        self.view = view
        self.doc = assembly.Document
        self.link = link
        self.editing = link is not None

        self.form = Gui.PySideUic.loadUi(":/panels/TaskAssemblyInsertFastener.ui")
        self.form.setMinimumWidth(400)
        if self.editing:
            self.form.setWindowTitle(translate("Assembly", "Edit Fasteners"))

        self.form.cb_category.currentTextChanged.connect(self.on_category_changed)
        self.form.le_search.textChanged.connect(self.on_search_changed)
        self.form.lw_type.itemSelectionChanged.connect(self.on_type_changed)
        self.form.cb_diameter.currentTextChanged.connect(self.on_diameter_changed)
        self.form.cb_length.currentTextChanged.connect(self.on_length_changed)
        self.form.sb_length.valueChanged.connect(self.on_length_changed)
        self.form.pb_add_selected.clicked.connect(self.on_add_selected)
        self.form.pb_add_unattached.clicked.connect(self.on_add_unattached)
        self.form.lw_refs.setContextMenuPolicy(QtCore.Qt.CustomContextMenu)
        self.form.lw_refs.customContextMenuRequested.connect(self.on_reference_menu)
        self.deleteFilter = DeleteKeyFilter(self.form.lw_refs, self.on_delete_selected)
        self.form.lw_refs.installEventFilter(self.deleteFilter)

        self.form.lw_type.setIconSize(QtCore.QSize(32, 32))

        self.is_updating = True
        self.has_length = True
        self.arbitrary_length = False
        # The fastener that the link uses was made or changed by this task: name it at the end
        self.rename_source = False
        # The only fastener was put in front of the user for lack of a hole: the first hole
        # that is added takes it
        self.placeholder = False
        # The names of the joints made by this task: named after the fastener at the end
        self.new_joints = []
        # The label of the fastener that the edited link uses: the link is named after the
        # fastener at the end if it still has this label
        source = link.LinkedObject if self.editing else None
        self.source_label = source.Label if source is not None else None

        self.doc.openTransaction("Edit Fasteners" if self.editing else "Insert Fastener")

        # The holes selected before the tool, that get the fasteners of a new link
        self.pending_holes = [] if self.editing else self._selected_holes()
        self._build_categories()

        cats = sorted(list(self.categories.keys()))
        self.form.cb_category.addItems(cats)

        source = link.LinkedObject if self.editing else None
        if isFastener(source):
            self._show_fastener(source)
            self.is_updating = False
        else:
            if "Screw" in cats:
                self.form.cb_category.setCurrentText("Screw")
            self.is_updating = False
            self.on_category_changed()

        self._update_reference_list()

    # ------------------------------------------------------------------------------------
    # Task

    def accept(self):
        if self.link is None:
            return self.reject()

        source = self.link.LinkedObject
        if self.rename_source and isFastener(source):
            t = source.Type
            d = str(source.Diameter)
            label = f"{t}_{d.replace('.', '_')}"
            if self.has_length and hasattr(source, "Length"):
                label += f"_{str(self._get_current_length()).replace('.', '_')}"
            source.Label = label.replace(" ", "_")
        self._name_new_objects(source)

        source_group = self.doc.getObject("Source_shapes")
        if source_group:
            source_group.purgeTouched()
        for component in FastenerLinkObject.components(self.link):
            component.purgeTouched()

        self.doc.commitTransaction()
        self.assembly.recompute()
        return True

    def _name_new_objects(self, source):
        """Name the link and the joints made by this task after the fastener they use. The
        fastener may have been made, renamed or changed after they were made. An edited link
        keeps a name that the user gave it."""
        if source is None:
            return
        if not self.editing or self.link.Label == self.source_label:
            self.link.Label = source.Label
        for name in self.new_joints:
            joint = self.doc.getObject(name)
            if joint is not None:
                joint.Label = FastenerLinkObject.jointLabel(source)

    def reject(self):
        self.doc.abortTransaction()
        self.doc.recompute()
        return True

    # ------------------------------------------------------------------------------------
    # Kind of fastener

    def _build_categories(self):
        self.categories = {}
        others = []
        for cat, fstype in FastenerBase.FSFastenerTypeDB.items():
            if len(fstype.items) <= 1 or cat in ["T-Slot", "SetScrew", "HeatSet"]:
                others.extend(fstype.items)
            else:
                self.categories[cat] = fstype.items
        if others:
            self.categories["Other"] = others

    def _show_fastener(self, fastener):
        """Show the kind, diameter and length of a fastener, without changing anything.
        Called with is_updating set."""
        t = fastener.Type
        for cat, types in self.categories.items():
            if t in types:
                self.form.cb_category.setCurrentText(cat)
                break
        self._fill_types()
        for i in range(self.form.lw_type.count()):
            item = self.form.lw_type.item(i)
            if item.data(QtCore.Qt.UserRole) == t:
                self.form.lw_type.setCurrentItem(item)
                break

        diameter = str(fastener.Diameter)
        self.form.cb_diameter.clear()
        self.form.cb_diameter.addItems(ScrewMaker.Instance.GetAllDiams(t))
        self.form.cb_diameter.setCurrentText(diameter)

        self._show_length_widgets(t)
        if self.has_length and hasattr(fastener, "Length"):
            if self.arbitrary_length:
                self.form.sb_length.setValue(float(fastener.Length.Value))
            else:
                self.form.cb_length.clear()
                self.form.cb_length.addItems(ScrewMaker.Instance.GetAllLengths(t, diameter, False))
                self.form.cb_length.setCurrentText(str(fastener.Length))

    def _get_current_type(self):
        curr_item = self.form.lw_type.currentItem()
        if curr_item and not curr_item.isHidden():
            return curr_item.data(QtCore.Qt.UserRole)
        return None

    def _fill_types(self):
        self.form.lw_type.clear()
        self.form.le_search.clear()

        cat = self.form.cb_category.currentText()
        for t in self.categories.get(cat, []):
            desc = FastenersCmd.FSGetDescription(t)
            if not desc:
                desc = t

            item = QtWidgets.QListWidgetItem(desc)
            item.setData(QtCore.Qt.UserRole, t)

            icon_path = os.path.join(FSutils.iconPath, FSGetIconAlias(t) + ".svg")
            if os.path.exists(icon_path):
                item.setIcon(QtGui.QIcon(icon_path))

            self.form.lw_type.addItem(item)

    def on_category_changed(self):
        if self.is_updating:
            return
        self.is_updating = True
        self._fill_types()
        self.is_updating = False

        if self.form.lw_type.count() > 0:
            self.form.lw_type.setCurrentRow(0)
        else:
            self.on_type_changed()

    def on_search_changed(self, text):
        search_str = text.lower()
        first_visible = -1

        for i in range(self.form.lw_type.count()):
            item = self.form.lw_type.item(i)
            matches = search_str in item.text().lower()
            item.setHidden(not matches)
            if matches and first_visible == -1:
                first_visible = i

        curr_item = self.form.lw_type.currentItem()
        if curr_item and curr_item.isHidden() and first_visible != -1:
            self.form.lw_type.setCurrentRow(first_visible)

    def _show_length_widgets(self, t):
        params = FastenersCmd.FSGetParams(t)
        self.has_length = (
            "Length" in params or "LenByDiamAndWidth" in params or "lengthArbitrary" in params
        )
        self.arbitrary_length = "lengthArbitrary" in params

        self.form.labelLength.setVisible(self.has_length)
        self.form.cb_length.setVisible(self.has_length and not self.arbitrary_length)
        self.form.sb_length.setVisible(self.has_length and self.arbitrary_length)

    def on_type_changed(self):
        if self.is_updating:
            return
        self.is_updating = True

        t = self._get_current_type()
        if not t:
            self.is_updating = False
            return

        diams = ScrewMaker.Instance.GetAllDiams(t)
        self.form.cb_diameter.clear()
        self.form.cb_diameter.addItems(diams)

        self._show_length_widgets(t)

        radius = self._first_hole_radius()
        if radius is not None and diams:
            target_d = radius * 2
            best_dia = diams[0]
            min_diff = float("inf")
            for d_str in diams:
                try:
                    d_num = FastenerBase.DiaStr2Num(d_str)
                except Exception:
                    # Can happen if there is a data error in the fastener wb.
                    # For instance (at time of writing) "ASME B18.3.5D UNC hexagone socket with cup point"
                    # fails for size #7 because Fasteners wb database is actually missing an entry for #7
                    # in its master diameter list (DiaList.csv), even though #7 is listed as a valid size
                    # for this fastener in its specific dimension table
                    continue
                if abs(d_num - target_d) < min_diff:
                    min_diff = abs(d_num - target_d)
                    best_dia = d_str
            self.form.cb_diameter.setCurrentText(best_dia)

        self.is_updating = False
        self.on_diameter_changed()

    def on_diameter_changed(self):
        if self.is_updating:
            return
        self.is_updating = True

        t = self._get_current_type()
        diam = self.form.cb_diameter.currentText()

        if not t or not diam:
            self.is_updating = False
            return

        if self.has_length and not self.arbitrary_length:
            lengths = ScrewMaker.Instance.GetAllLengths(t, diam, False)
            self.form.cb_length.clear()
            self.form.cb_length.addItems(lengths)

        self.is_updating = False
        self.update_preview()

    def on_length_changed(self, _=None):
        if self.is_updating:
            return
        self.update_preview()

    def _get_current_length(self):
        if not self.has_length:
            return None
        if self.arbitrary_length:
            return str(self.form.sb_length.value())
        return self.form.cb_length.currentText()

    # ------------------------------------------------------------------------------------
    # Source fastener

    def _get_or_create_source_group(self):
        source_group = self.doc.getObject("Source_shapes")
        if not source_group:
            source_group = self.doc.addObject("App::DocumentObjectGroup", "Source_shapes")
            source_group.Label = "Source shapes"
        return source_group

    def _find_existing_fastener(self, t, d, l):
        source_group = self._get_or_create_source_group()
        for obj in source_group.Group:
            if not isFastener(obj):
                continue

            if obj.Type != t or obj.Diameter != d:
                continue

            if not self.has_length:
                return obj

            if not hasattr(obj, "Length"):
                continue

            if (
                self.arbitrary_length
                and math.isclose(float(obj.Length.Value), float(l), abs_tol=1e-5)
            ) or str(obj.Length) == str(l):
                return obj

        return None

    def _make_fastener(self, t):
        fastener = self.doc.addObject("Part::FeaturePython", "Fastener")
        FastenersCmd.FSScrewObject(fastener, t, None)
        FastenersCmd.FSViewProviderTree(fastener.ViewObject)
        self._get_or_create_source_group().addObject(fastener)
        fastener.Visibility = False
        return fastener

    def _configure_fastener(self, fastener, t, d, l):
        if hasattr(fastener, "Type") and fastener.Type != t:
            compatible_types = fastener.Proxy.GetCompatibleTypes(t)
            fastener.Type = compatible_types
            fastener.Type = t

        if hasattr(fastener, "Diameter"):
            d_enums = fastener.getEnumerationsOfProperty("Diameter")
            if d_enums is not None and d not in d_enums:
                fastener.Diameter = d_enums + [d]
            fastener.Diameter = d

        if self.has_length and l is not None and hasattr(fastener, "Length"):
            if not self.arbitrary_length:
                l_enums = fastener.getEnumerationsOfProperty("Length")
                if l_enums is not None and l not in l_enums:
                    fastener.Length = l_enums + [l]
            fastener.Length = l

        try:
            fastener.Proxy.execute(fastener)
        except Exception as e:
            App.Console.PrintWarning(f"Preview failed: {e}\n")

    def _source_for(self, t, d, l):
        """The fastener of this kind and size that the link is to use.

        One that exists in the document is used again. Else the fastener that the link uses
        is changed, if nothing else uses it and it is of the same kind, and a new fastener
        is made otherwise. Returns the fastener, and the fastener that is not needed any
        more once the link uses the returned one, or None.
        """
        current = self.link.LinkedObject if self.link is not None else None
        mine = (
            isFastener(current)
            and not FastenerLinkObject.otherUsers(self.link)
            and current in self._get_or_create_source_group().Group
        )

        existing = self._find_existing_fastener(t, d, l)
        if existing is not None:
            if existing != current:
                self.rename_source = False  # not a fastener of this task
            return existing, (current if mine and existing != current else None)

        if mine and getattr(current, "Type", None) == t:
            self._configure_fastener(current, t, d, l)
            self.rename_source = True
            return current, None

        fastener = self._make_fastener(t)
        self._configure_fastener(fastener, t, d, l)
        self.rename_source = True
        return fastener, (current if mine else None)

    def update_preview(self):
        t = self._get_current_type()
        d = self.form.cb_diameter.currentText()
        l = self._get_current_length()

        if not t or not d:
            return

        source, unused = self._source_for(t, d, l)
        if self.link is None:
            self._create_link(source)
        else:
            FastenerLinkObject.setSource(self.link, source)
        if unused is not None:
            self.doc.removeObject(unused.Name)

        self._refresh()

    # ------------------------------------------------------------------------------------
    # Fasteners and their holes

    def _selected_holes(self):
        """The circular edges of the components of the assembly that are selected, as joint
        references"""
        holes = []
        own = FastenerLinkObject.components(self.link) if self.link is not None else []
        for sel in Gui.Selection.getSelectionEx("*", 0):
            for sub in sel.SubElementNames:
                component, new_sub = UtilsAssembly.getComponentReference(
                    self.assembly, sel.Object, sub
                )
                if not component or component in own:
                    continue
                ref = (component, [new_sub, new_sub])
                if FastenerLinkObject.holeInfo(ref) is None:
                    continue
                if not any(h[0] == component and h[1][0] == new_sub for h in holes):
                    holes.append(ref)
        return holes

    def _attached_holes(self):
        """The references to the holes of the fasteners of the link, None when unattached"""
        if self.link is None:
            return []
        return [
            FastenerLinkObject.attachment(component)[1]
            for component in FastenerLinkObject.components(self.link)
        ]

    def _first_hole_radius(self):
        holes = self.pending_holes if self.link is None else self._attached_holes()
        for ref in holes:
            info = FastenerLinkObject.holeInfo(ref) if ref else None
            if info is not None:
                return info["radius"]
        return None

    def _screen_center(self):
        x, y = self.view.getSize()
        return App.Placement(self.view.getPointOnFocalPlane(x // 2, y // 2), App.Rotation())

    def _create_link(self, source):
        self.link = FastenerLinkObject.makeFastenerLink(self.assembly, source)
        if not self.pending_holes:
            self.link.Placement = self._screen_center()
            self.placeholder = True
            return
        self._attach(self.link, self.pending_holes[0])
        for ref in self.pending_holes[1:]:
            component = FastenerLinkObject.addFastener(self.link, App.Placement())
            self._attach(component, ref)
        self.pending_holes = []

    def _attach(self, component, ref):
        joint = FastenerLinkObject.attach(component, ref)
        if joint is not None:
            self.new_joints.append(joint.Name)

    def _refresh(self):
        """After a change: compute, solve and show the fasteners"""
        import JointObject

        self.doc.recompute()
        JointObject.solveIfAllowed(self.assembly)
        self._update_reference_list()

    def _update_reference_list(self):
        selected = [index.row() for index in self.form.lw_refs.selectedIndexes()]
        self.form.lw_refs.clear()
        if self.link is None:
            holes = self.pending_holes
            fasteners = [None] * len(holes)
        else:
            fasteners = FastenerLinkObject.components(self.link)
            holes = self._attached_holes()

        unattached = 0
        for component, ref in zip(fasteners, holes):
            if ref:
                text = f"{ref[0].Label} : {UtilsAssembly.getElementName(ref[1][0])}"
            else:
                text = translate("Assembly", "Unattached")
                unattached += 1

            row = QtWidgets.QWidget()
            layout = QtWidgets.QHBoxLayout(row)
            layout.setContentsMargins(4, 1, 4, 1)
            layout.addWidget(QtWidgets.QLabel(text), 1)
            if component is not None:
                button = QtWidgets.QPushButton(translate("Assembly", "Reverse"))
                button.setSizePolicy(QtWidgets.QSizePolicy.Fixed, QtWidgets.QSizePolicy.Fixed)
                button.setToolTip(translate("Assembly", "Turns this fastener upside down"))
                button.clicked.connect(lambda _=False, c=component: self.on_reverse(c))
                layout.addWidget(button)

            item = QtWidgets.QListWidgetItem()
            item.setSizeHint(row.sizeHint())
            self.form.lw_refs.addItem(item)
            self.form.lw_refs.setItemWidget(item, row)

        for row in selected:
            if row < self.form.lw_refs.count():
                self.form.lw_refs.item(row).setSelected(True)

        count = len(fasteners)
        if count and unattached:
            text = translate("Assembly", "Fasteners: %1, unattached: %2")
            text = text.replace("%1", str(count)).replace("%2", str(unattached))
        elif count:
            text = translate("Assembly", "Fasteners: %1").replace("%1", str(count))
        else:
            text = translate(
                "Assembly", "No circular edge selected: the fastener goes to the screen center."
            )
        self.form.info_label.setText(text)

    def on_add_selected(self):
        if self.link is None:
            return
        known = [ref for ref in self._attached_holes() if ref]
        added = False
        for ref in self._selected_holes():
            if any(k[0] == ref[0] and k[1][0] == ref[1][0] for k in known):
                continue
            component = self._component_for_new_hole()
            self._attach(component, ref)
            known.append(ref)
            added = True
        if added:
            self._refresh()

    def _component_for_new_hole(self):
        """The fastener that goes on a hole that is added: an unattached fastener whose row
        is selected, or the fastener that was waiting for a hole, else a new one"""
        fasteners = FastenerLinkObject.components(self.link)
        for index in sorted(index.row() for index in self.form.lw_refs.selectedIndexes()):
            if index < len(fasteners):
                if FastenerLinkObject.attachment(fasteners[index])[0] is None:
                    self.form.lw_refs.item(index).setSelected(False)
                    return fasteners[index]
        if self.placeholder:
            self.placeholder = False
            if len(fasteners) == 1 and FastenerLinkObject.attachment(fasteners[0])[0] is None:
                return fasteners[0]
        return FastenerLinkObject.addFastener(self.link, App.Placement())

    def on_add_unattached(self):
        if self.link is None:
            return
        self.placeholder = False
        FastenerLinkObject.addFastener(self.link, self._screen_center())
        self._refresh()

    def on_delete_selected(self):
        if self.link is None:
            return
        rows = sorted({index.row() for index in self.form.lw_refs.selectedIndexes()}, reverse=True)
        removed = False
        for row in rows:
            if len(FastenerLinkObject.components(self.link)) < 2:
                break  # a link has at least one fastener
            FastenerLinkObject.removeFastener(self.link, row)
            removed = True
        self.form.lw_refs.clearSelection()
        if removed:
            self._refresh()

    def on_reference_menu(self, pos):
        if not self.form.lw_refs.selectedIndexes():
            return
        menu = QtWidgets.QMenu(self.form.lw_refs)
        action = menu.addAction(translate("Assembly", "Delete"))
        action.triggered.connect(self.on_delete_selected)
        menu.exec_(self.form.lw_refs.viewport().mapToGlobal(pos))

    def on_reverse(self, component):
        FastenerLinkObject.reverse(component)
        self._refresh()


def editFastenerLink(link):
    """Open the task on the fasteners of a fastener link. Returns whether it was opened."""
    if not FASTENERS_AVAILABLE:
        App.Console.PrintWarning(
            translate("Assembly", "Install the Fasteners workbench to edit these fasteners.") + "\n"
        )
        return False

    assembly = link.getParentGeoFeatureGroup()
    if assembly is None or not assembly.isDerivedFrom("Assembly::AssemblyObject"):
        return False

    link.Document.abortTransaction()  # Close the auto-transaction

    task = Gui.Control.activeTaskDialog()
    if task:
        task.reject()

    gui_doc = Gui.getDocument(link.Document.Name)
    if UtilsAssembly.activeAssembly() != assembly:
        gui_doc.setEdit(assembly)

    panel = TaskAssemblyInsertFastener(assembly, gui_doc.activeView(), link)
    dialog = Gui.Control.showDialog(panel)
    if dialog is not None:
        dialog.setAutoCloseOnTransactionChange(True)
        dialog.setAutoCloseOnDeletedDocument(True)
        dialog.setDocumentName(link.Document.Name)
    return True


class CommandInsertFastener:
    def GetResources(self):
        return {
            "Pixmap": "Assembly_InsertFastener",
            "MenuText": App.Qt.translate("Assembly_InsertFastener", "Insert Fastener"),
            "ToolTip": App.Qt.translate(
                "Assembly_InsertFastener",
                "Inserts standard fasteners into the Assembly.\n\n"
                "Works only if the Fasteners workbench is installed. Available from the Addon Manager.\n\n"
                "If circular edges are selected before running, the tool will auto-detect "
                "the diameter and position the fasteners with Fixed Joints directly onto the holes.\n"
                "If nothing is selected, the fastener is placed at the center of the screen.\n"
                "Holes can be added and removed in the tool, and afterwards with a "
                "double-click on the fasteners in the tree.",
            ),
            "Accel": "J",
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        return UtilsAssembly.isAssemblyCommandActive() and FASTENERS_AVAILABLE

    def Activated(self):
        assembly = UtilsAssembly.activeAssembly()
        if not assembly:
            return
        view = Gui.activeDocument().activeView()
        Gui.Control.showDialog(TaskAssemblyInsertFastener(assembly, view))


if App.GuiUp:
    Gui.addCommand("Assembly_InsertFastener", CommandInsertFastener())
