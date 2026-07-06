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


class TaskAssemblyInsertFastener:
    def __init__(self, assembly, view):
        self.assembly = assembly
        self.view = view
        self.doc = App.ActiveDocument

        self.form = Gui.PySideUic.loadUi(":/panels/TaskAssemblyInsertFastener.ui")
        self.form.setMinimumWidth(400)

        self.form.cb_category.currentTextChanged.connect(self.on_category_changed)
        self.form.le_search.textChanged.connect(self.on_search_changed)
        self.form.lw_type.itemSelectionChanged.connect(self.on_type_changed)
        self.form.cb_diameter.currentTextChanged.connect(self.on_diameter_changed)
        self.form.cb_length.currentTextChanged.connect(self.on_length_changed)
        self.form.sb_length.valueChanged.connect(self.on_length_changed)
        self.form.cb_reverse.stateChanged.connect(self.on_reverse_checked)

        self.form.lw_type.setIconSize(QtCore.QSize(32, 32))

        self.target_edges = []
        self.links = []
        self.temp_fastener = None
        self.is_updating = True

        self.has_length = True
        self.arbitrary_length = False

        self.doc.openTransaction("Insert Fasteners")

        self._analyze_selection()
        self._build_categories()

        cats = sorted(list(self.categories.keys()))
        self.form.cb_category.addItems(cats)
        if "Screw" in cats:
            self.form.cb_category.setCurrentText("Screw")

        if self.target_edges:
            self.form.info_label.setText(f"Found {len(self.target_edges)} circular edge(s).")
            self.form.info_label.setStyleSheet("color: green;")
        else:
            self.form.info_label.setText("No circular edges selected. Spawning at screen center.")
            self.form.info_label.setStyleSheet("color: gray;")

        self.is_updating = False
        self.on_category_changed()

    def accept(self):
        t = self._get_current_type()
        d = self.form.cb_diameter.currentText()
        l = self._get_current_length()

        if not t or not d:
            return self.reject()

        if self.temp_fastener:
            l_str = f"_{str(l).replace('.', '_')}" if self.has_length else ""
            self.temp_fastener.Label = f"{t}_{d.replace('.', '_')}{l_str}".replace(" ", "_")

        if self.target_edges:
            joint_group = UtilsAssembly.getJointGroup(self.assembly)

            # Import the Assembly Joint system
            try:
                import JointObject
            except ImportError:
                JointObject = None

            for i, edge_info in enumerate(self.target_edges):
                link = self.links[i]

                comp, new_sub = UtilsAssembly.getComponentReference(
                    self.assembly, edge_info["object"], edge_info["subname"]
                )
                if not comp:
                    comp = edge_info["object"]
                    new_sub = edge_info["subname"]

                joint = joint_group.newObject("App::FeaturePython", "Joint")
                joint.Label = f"Fixed_{link.LinkedObject.Label}"
                joint.Visibility = False

                if JointObject:
                    JointObject.Joint(joint, 0)  # 0 = Fixed Joint Type
                    if App.GuiUp:
                        JointObject.ViewProviderJoint(joint.ViewObject)

                    # Ref1 maps directly to the circular edge and uses the edge as the vertex (center point)
                    ref1 = [comp, [new_sub, new_sub]]
                    # Ref2 is the origin of the instanced Fastener Link (Empty subname/vertex)
                    ref2 = [link, ["", ""]]

                    joint.Proxy.setJointConnectors(joint, [ref1, ref2])
                else:
                    print("Error! JointObject.py not found.")

        source_group = self.doc.getObject("Source_shapes")
        if source_group:
            source_group.purgeTouched()
        for link in self.links:
            link.purgeTouched()

        self.doc.commitTransaction()
        self.assembly.recompute()
        return True

    def reject(self):
        self.doc.abortTransaction()
        self.doc.recompute()
        return True

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

    def _analyze_selection(self):
        sel = Gui.Selection.getSelectionEx("*", 0)
        for s in sel:
            if not s.SubElementNames:
                continue
            for sub in s.SubElementNames:
                moving_part, new_sub = UtilsAssembly.getComponentReference(
                    self.assembly, s.Object, sub
                )
                if not moving_part:
                    continue

                element_name = UtilsAssembly.getElementName(new_sub)
                if not element_name.startswith("Edge"):
                    continue

                shape = moving_part.getSubObject(new_sub)
                if not hasattr(shape, "Curve"):
                    continue

                curve = shape.Curve
                if not curve.TypeId in ["Part::GeomCircle", "Part::GeomEllipse"] or not hasattr(
                    curve, "Radius"
                ):
                    continue

                self.target_edges.append(
                    {
                        "object": moving_part,
                        "subname": new_sub,
                        "center": curve.Center,
                        "axis": curve.Axis,
                        "radius": curve.Radius,
                    }
                )

    def _get_current_type(self):
        curr_item = self.form.lw_type.currentItem()
        if curr_item and not curr_item.isHidden():
            return curr_item.data(QtCore.Qt.UserRole)
        return None

    def on_category_changed(self):
        if self.is_updating:
            return
        self.is_updating = True

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

        params = FastenersCmd.FSGetParams(t)
        self.has_length = (
            "Length" in params or "LenByDiamAndWidth" in params or "lengthArbitrary" in params
        )
        self.arbitrary_length = "lengthArbitrary" in params

        if self.has_length:
            self.form.labelLength.setVisible(True)
            if self.arbitrary_length:
                self.form.cb_length.setVisible(False)
                self.form.sb_length.setVisible(True)
            else:
                self.form.cb_length.setVisible(True)
                self.form.sb_length.setVisible(False)
        else:
            self.form.labelLength.setVisible(False)
            self.form.cb_length.setVisible(False)
            self.form.sb_length.setVisible(False)

        if self.target_edges:
            target_d = self.target_edges[0]["radius"] * 2
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

    def _get_or_create_source_group(self):
        source_group = self.doc.getObject("Source_shapes")
        if not source_group:
            source_group = self.doc.addObject("App::DocumentObjectGroup", "Source_shapes")
            source_group.Label = "Source shapes"
        return source_group

    def _find_existing_fastener(self, t, d, l):
        source_group = self._get_or_create_source_group()
        for obj in source_group.Group:
            if not hasattr(obj, "Proxy") or not isinstance(obj.Proxy, FastenerBase.FSBaseObject):
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

    def update_preview(self):
        t = self._get_current_type()
        d = self.form.cb_diameter.currentText()
        l = self._get_current_length()

        if not t or not d:
            return

        active_fastener = self._find_existing_fastener(t, d, l)

        if active_fastener:
            if self.temp_fastener:
                self.doc.removeObject(self.temp_fastener.Name)
                self.temp_fastener = None
        else:
            if self.temp_fastener and getattr(self.temp_fastener, "Type", "") != t:
                self.doc.removeObject(self.temp_fastener.Name)
                self.temp_fastener = None

            if not self.temp_fastener:
                self.temp_fastener = self.doc.addObject("Part::FeaturePython", "PreviewFastener")
                FastenersCmd.FSScrewObject(self.temp_fastener, t, None)
                FastenersCmd.FSViewProviderTree(self.temp_fastener.ViewObject)
                source_group = self._get_or_create_source_group()
                source_group.addObject(self.temp_fastener)
                self.temp_fastener.Visibility = False

            if hasattr(self.temp_fastener, "Type") and self.temp_fastener.Type != t:
                compatible_types = self.temp_fastener.Proxy.GetCompatibleTypes(t)
                self.temp_fastener.Type = compatible_types
                self.temp_fastener.Type = t

            if hasattr(self.temp_fastener, "Diameter"):
                d_enums = self.temp_fastener.getEnumerationsOfProperty("Diameter")
                if d_enums is not None and d not in d_enums:
                    self.temp_fastener.Diameter = d_enums + [d]
                self.temp_fastener.Diameter = d

            if self.has_length and l is not None and hasattr(self.temp_fastener, "Length"):
                if not self.arbitrary_length:
                    l_enums = self.temp_fastener.getEnumerationsOfProperty("Length")
                    if l_enums is not None and l not in l_enums:
                        self.temp_fastener.Length = l_enums + [l]
                self.temp_fastener.Length = l

            try:
                self.temp_fastener.Proxy.execute(self.temp_fastener)
            except Exception as e:
                App.Console.PrintWarning(f"Preview failed: {e}\n")

            active_fastener = self.temp_fastener

        if not self.links:
            self._create_links(active_fastener)
        else:
            for link in self.links:
                link.LinkedObject = active_fastener

        self.doc.recompute()

    def _create_links(self, fastener):
        reverse = self.form.cb_reverse.isChecked()
        rot = App.Rotation(App.Vector(1, 0, 0), 180) if reverse else App.Rotation()

        if not self.target_edges:
            link = self.assembly.newObject("App::Link", "FastenerLink")
            link.LinkedObject = fastener
            x, y = self.view.getSize()
            screenCenter = self.view.getPointOnFocalPlane(x // 2, y // 2)
            link.Placement.Base = screenCenter
            link.Placement.Rotation = rot
            self.links.append(link)
        else:
            for edge_info in self.target_edges:
                link = self.assembly.newObject("App::Link", "FastenerLink")
                link.LinkedObject = fastener
                link.Placement = App.Placement(edge_info["center"], rot)
                self.links.append(link)

    def on_reverse_checked(self):
        reverse = self.form.cb_reverse.isChecked()
        rot = App.Rotation(App.Vector(1, 0, 0), 180) if reverse else App.Rotation()
        for link in self.links:
            link.Placement.Rotation = rot
            link.purgeTouched()


class CommandInsertFastener:
    def GetResources(self):
        return {
            "Pixmap": "Assembly_InsertFastener",
            "MenuText": App.Qt.translate("Assembly_InsertFastener", "Insert Fastener"),
            "ToolTip": App.Qt.translate(
                "Assembly_InsertFastener",
                "Insert standard fasteners into the Assembly.\n\n"
                "Can be used only if the Fasteners workbench is installed. Get it from the addon manager.\n\n"
                "If circular edges are selected before running, the tool will auto-detect "
                "the diameter and position the fasteners with Fixed Joints directly onto the holes.\n"
                "If nothing is selected, the fastener spawns at the center of the screen.",
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
