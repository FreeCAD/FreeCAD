# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2018 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

"""This module contains FreeCAD commands for the BIM workbench"""

import FreeCAD
import FreeCADGui
import ifcopenshell
from PySide.QtWidgets import (
    QDialog,
    QVBoxLayout,
    QMessageBox,
    QWidget,
    QLineEdit,
    QLabel,
    QComboBox,
    QDialogButtonBox,
    QFormLayout,
)
from nativeifc import ifc_tools, ifc_commands


QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate


class BIM_IfcProfiles:
    def GetResources(self):
        return {
            "Pixmap": "Arch_Profile",
            "MenuText": QT_TRANSLATE_NOOP("BIM_IfcProfiles", "Manage IFC profiles..."),
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_IfcProfiles",
                "Manage the IFC profiles in your IFC objects",
            ),
        }

    def IsActive(self):
        "Only Activated when have a IfcFile is selected or it open a IfcFile(Lock Mode)"
        project = ifc_commands.get_project()
        if project:
            self.ifc_file = ifc_tools.get_ifcfile(project)
            return True
        else:
            return False

    def Activated(self):
        from PySide import QtGui

        # load the form and set the tree model up
        self.form = FreeCADGui.PySideUic.loadUi(":/ui/dialogIfcProfiles.ui")
        # self.form.setWindowIcon(QtGui.QIcon(":/icons/BIM_IfcElements.svg"))
        self.form.ifcFileObj.addItem(ifc_commands.get_project().Label)
        # tree model
        self.model = QtGui.QStandardItemModel()
        self.form.profileTree.setModel(self.model)
        self.form.profileTree.setUniformRowHeights(True)
        self.form.profileTree.setItemDelegate(QtGui.QStyledItemDelegate())
        # connect signals
        self.form.profileTree.selectionModel().selectionChanged.connect(
            self.updateProfileInfo
        )
        self.form.buttonAdd.clicked.connect(self.add_new_profile)
        self.form.buttonEdit.clicked.connect(self.edit_profile)
        self.form.buttonDel.clicked.connect(self.delete_profile)
        self.form.buttonPrune.clicked.connect(self.prune_profile)
        # center the dialog over FreeCAD window
        mw = FreeCADGui.getMainWindow()
        self.form.move(
            mw.frameGeometry().topLeft()
            + mw.rect().center()
            - self.form.rect().center()
        )

        self.update()
        self.form.show()

        return

    def update(self):
        from PySide import QtGui

        self.form.profileTree.reset()
        self.model.clear()
        self.model.setHorizontalHeaderLabels(
            [
                translate("BIM", "Profile Name"),
                translate("BIM", "IfcProfileDef"),
            ]
        )
        self.profiles = self.ifc_file.by_type("IfcProfileDef")
        for profile in self.profiles:
            self.model.appendRow(
                [
                    QtGui.QStandardItem(profile.ProfileName),
                    QtGui.QStandardItem(profile.is_a()),
                ]
            )
        self.form.profilePreview.clear()
        clear_layout(self.form.profileInfoLayout)

    def updateProfileInfo(self):
        from PySide import QtGui

        # update thumbnail
        sel_profile_index = self.form.profileTree.currentIndex().row()
        profile = self.profiles[sel_profile_index]
        profile_thumbnail = QtGui.QPixmap(
            self.generate_thumbnail_for_active_profile(profile)
        )
        self.form.profilePreview.setPixmap(profile_thumbnail)

        # update Display Info
        clear_layout(self.form.profileInfoLayout)
        self.schema_ver = self.ifc_file.schema
        self.schema = ifcopenshell.ifcopenshell_wrapper.schema_by_name(self.schema_ver)
        props = get_profile_props(self.schema, profile.is_a())
        attributes = profile.get_info()
        self.name_label = QLabel("Profile Name:")
        self.name_prop = QLabel(attributes.get("ProfileName"))
        self.class_label = QLabel("IfcProfileDef:")
        self.class_prop = QLabel(profile.is_a())
        self.form.profileInfoLayout.addRow(self.name_label, self.name_prop)
        self.form.profileInfoLayout.addRow(self.class_label, self.class_prop)
        for prop in props:
            attri_value = attributes.get(prop.name())
            if attri_value is None:
                attri_value = "None"
            else:
                if "Slope" not in prop.name():
                    attri_value /= ifc_tools.get_scale(self.ifc_file)
            self.form.profileInfoLayout.addRow(
                QLabel(f"{prop.name()}:"), QLabel(str(attri_value))
            )

    def generate_thumbnail_for_active_profile(self, profile):
        from PIL import Image, ImageDraw, ImageQt

        # generate image
        size = 128
        img = Image.new("RGBA", (size, size))
        draw = ImageDraw.Draw(img)
        draw_image_for_ifc_profile(draw, profile, size)
        preview_img = ImageQt.toqpixmap(img)
        return preview_img

    def add_new_profile(self):
        dialog = AddNewProfileDialog(self.ifc_file)
        if dialog.exec():
            self.ifc_file = dialog.add_profile()
            self.update()

    def edit_profile(self):
        sel_profile_index = self.form.profileTree.currentIndex().row()
        if sel_profile_index is None:  # No selection made
            QMessageBox.warning(self, "Warning!", "No profile selected!")
            return
        self.profile = self.profiles[sel_profile_index]
        if self.profile.is_a("IfcParameterizedProfileDef"):
            dialog = EditProfileDialog(self.ifc_file, self.profile)
            if dialog.exec():
                self.ifc_file = dialog.edit_profile()
                self.update()
        else:  # IfcArbitraryClosedProfileDef and maybe add with voids after
            self.form.close()
            active_doc = FreeCAD.ActiveDocument
            profile_shape = display_arbitrary_closed_profile(self.profile)

            # view obj selecion
            FreeCAD.setActiveDocument("EditProfile")
            FreeCADGui.Selection.clearSelection()
            FreeCADGui.Selection.addSelection(profile_shape)
            FreeCAD.ActiveDocument.recompute()
            FreeCADGui.SendMsgToActiveView("ViewSelection")

            self.edit_ab_dialog = EditArbitraryProfile(self.profile, active_doc.Name)

            # Connect signals and slots
            self.edit_ab_dialog.form.buttonBox.accepted.connect(
                self.update_arbitrary_profile
            )
            self.edit_ab_dialog.form.buttonBox.rejected.connect(
                self.edit_ab_dialog.reject
            )
            self.edit_ab_dialog.form.buttonBox.rejected.connect(self.show)

    def update_arbitrary_profile(self):
        selections = FreeCADGui.Selection.getSelection()
        if not bool(selections):
            QMessageBox.warning(None, "Error", "Please select one profile shape")
        else:
            profile_shape = selections[0]

            scaling = ifc_tools.get_scale(self.ifc_file)

            points = []
            for pts in profile_shape.Shape.OuterWire.Vertexes:
                points.append((pts.Point.x * scaling, pts.Point.y * scaling))
            points.append(points[0])  # close shape
            print(points)
            old_profile = self.profile
            new_profile = ifc_tools.api_run(
                "profile.add_arbitrary_profile",
                self.ifc_file,
                profile=points,
                name=self.edit_ab_dialog.form.input_name.text(),
            )
            for inverse in self.ifc_file.get_inverse(old_profile):
                ifcopenshell.util.element.replace_attribute(
                    inverse, old_profile, new_profile
                )
            ifcopenshell.util.element.remove_deep2(self.ifc_file, old_profile)
            print("Update Profile:", new_profile)

            self.edit_ab_dialog.finish()
            self.update()
            self.show()

    def show(self):
        self.form.show()

    def delete_profile(self):
        sel_profile_index = self.form.profileTree.currentIndex().row()

        if sel_profile_index is None:  # No selection made
            QMessageBox.warning(self, "Warning!", "No profile selected!")
            return

        profile = self.profiles[sel_profile_index]
        relation_obj_num = self.ifc_file.get_total_inverses(profile)

        if relation_obj_num:
            reply = QMessageBox.warning(
                None,
                "Warning!",
                f"Profile {profile.ProfileName} has {relation_obj_num} inverse relationship(s) in project, still want to delete it?",
                QMessageBox.Yes | QMessageBox.No,
                QMessageBox.No,
            )
            if reply != QMessageBox.Yes:
                return

        print("Delete IfcProfile", profile)
        ifc_tools.api_run("profile.remove_profile", self.ifc_file, profile=profile)
        self.update()

    def prune_profile(self):
        reply = QMessageBox.information(
            None,
            "Confirm",
            "Are you sure you want to continue deleting all unused profiles?",
            QMessageBox.Yes | QMessageBox.No,
            QMessageBox.No,
        )
        if reply == QMessageBox.Yes:
            pruned_profiles = 0
            for profile in self.profiles:
                if self.ifc_file.get_total_inverses(profile) > 0:
                    continue
                ifc_tools.api_run(
                    "profile.remove_profile", self.ifc_file, profile=profile
                )
                pruned_profiles += 1
            print(f"{pruned_profiles} profiles were deleted.")
            self.update()


class AddNewProfileDialog(QDialog):
    def __init__(self, ifc_file):
        super().__init__()

        from PySide import QtCore
        from ifcopenshell.util.doc import get_entity_doc

        self.ifc_file = ifc_file
        self.schema_ver = ifc_file.schema
        self.schema = ifcopenshell.ifcopenshell_wrapper.schema_by_name(self.schema_ver)

        profile_classes = get_profile_classes(self.schema)
        self.setWindowTitle("Add New Profile")
        self.layout = QVBoxLayout()
        self.prop_layout = QVBoxLayout()
        self.class_label = QLabel("Profile Class:")
        self.sel_profile_class = QComboBox()
        self.sel_profile_class.addItems(profile_classes)
        for index, p_cls in enumerate(profile_classes):
            self.sel_profile_class.setItemData(
                index,
                get_entity_doc(self.schema_ver, p_cls)["description"],
                QtCore.Qt.ToolTipRole,
            )
        self.sel_profile_class.currentIndexChanged.connect(self.update_props)
        self.name_label = QLabel("Profile Name:")
        self.name_input = QLineEdit()

        self.layout.addWidget(self.class_label)
        self.layout.addWidget(self.sel_profile_class)
        self.layout.addWidget(self.name_label)
        self.layout.addWidget(self.name_input)

        self.layout.addLayout(self.prop_layout)

        self.button_box = QDialogButtonBox(
            QDialogButtonBox.Ok | QDialogButtonBox.Cancel
        )
        self.button_box.accepted.connect(self.accept)
        self.button_box.rejected.connect(self.reject)

        self.layout.addWidget(self.button_box)
        self.setLayout(self.layout)
        self.update_props()
        self.adjustSize()

    # signals
    def update_props(self):
        from ifcopenshell.util.doc import get_entity_doc

        clear_layout(self.prop_layout)
        self.p_cls = self.sel_profile_class.currentText()
        # display spec url
        spec_url = get_entity_doc(self.schema_ver, self.p_cls)["spec_url"]
        linkTemplate = "<a href={0}>{1}</a>"
        self.p_cls_label = HyperlinkLabel()
        self.p_cls_label.setText(
            linkTemplate.format(spec_url, f"Property of {self.p_cls}: Click Me")
        )
        self.unit_label = QLabel("Length Unit: mm")
        self.unit_label.setStyleSheet("font-size: 12px")
        self.prop_layout.addWidget(self.p_cls_label)
        self.prop_layout.addWidget(self.unit_label)

        props = get_profile_props(self.schema, self.p_cls)
        self.prop_sliders = []
        for prop in props:
            editor = PropEditor(prop.name())
            self.prop_layout.addWidget(editor)
            self.prop_sliders.append((prop.name(), editor))

        self.adjustSize()

    def add_profile(self):
        scaling = ifc_tools.get_scale(self.ifc_file)
        self.prop_editors = {}
        for prop_name, editor in self.prop_sliders:
            prop_value = editor.get_value()
            if prop_value != 0:
                if "Slope" not in prop_name:
                    prop_value *= scaling
                self.prop_editors[prop_name] = prop_value

        attributes = self.prop_editors
        attributes["ProfileName"] = self.name_input.text()
        attributes["ProfileType"] = "AREA"

        if self.p_cls == "IfcArbitraryClosedProfileDef":
            # 500 is mm in FreeCAD , scaling turn to IfcUnit(Length)
            points = [
                (-500 * scaling, -500 * scaling),
                (500 * scaling, -500 * scaling),
                (500 * scaling, 500 * scaling),
                (-500 * scaling, 500 * scaling),
                (-500 * scaling, -500 * scaling),
            ]
            profile = ifc_tools.api_run(
                "profile.add_arbitrary_profile",
                self.ifc_file,
                profile=points,
                name=self.name_input.text(),
            )
        else:
            profile = ifc_tools.api_run(
                "profile.add_parameterized_profile",
                self.ifc_file,
                ifc_class=self.p_cls,
            )
            ifc_tools.api_run(
                "profile.edit_profile",
                self.ifc_file,
                profile=profile,
                attributes=attributes,
            )
        print("Add New IfcProfile", profile)
        return self.ifc_file


class EditArbitraryProfile:
    def __init__(self, profile, active_doc_name):
        from PySide import QtCore

        # set ui
        self.form = FreeCADGui.PySideUic.loadUi(":/ui/dialogEditIfcArbitraryProfile.ui")
        # Set the window to stay on top
        self.form.setWindowFlag(QtCore.Qt.WindowStaysOnTopHint, True)

        self.form.output_ifc_cls.addItem("IfcArbitraryClosedProfileDef")
        self.form.input_name.setText(profile.ProfileName)
        self.ifc_file = profile.file
        self.doc_name = active_doc_name
        self.form.show()

    def reject(self):
        print("Cancel Edit Profile")
        self.finish()

    def finish(self):
        try:
            edit_doc = FreeCAD.getDocument("EditProfile")
        except NameError:
            edit_doc = FreeCAD.ActiveDocument
        FreeCAD.closeDocument(edit_doc.Name)
        FreeCAD.setActiveDocument(self.doc_name)
        FreeCADGui.activateWorkbench("BIMWorkbench")
        self.form.close()


class EditProfileDialog(QDialog):
    def __init__(self, ifc_file, profile):
        super().__init__()

        self.ifc_file = ifc_file
        self.profile = profile
        self.schema_ver = ifc_file.schema
        self.schema = ifcopenshell.ifcopenshell_wrapper.schema_by_name(self.schema_ver)

        self.setWindowTitle("Edit Profile")
        self.layout = QVBoxLayout()
        self.prop_layout = QVBoxLayout()
        self.class_label = QLabel("Profile Class:")
        self.sel_profile_class = QComboBox()
        self.sel_profile_class.addItem(self.profile.is_a())
        self.name_label = QLabel("Profile Name:")
        self.name_input = QLineEdit()
        self.name_input.setText(self.profile.ProfileName)

        self.layout.addWidget(self.class_label)
        self.layout.addWidget(self.sel_profile_class)
        self.layout.addWidget(self.name_label)
        self.layout.addWidget(self.name_input)

        self.update_props()
        self.layout.addLayout(self.prop_layout)

        self.button_box = QDialogButtonBox(
            QDialogButtonBox.Ok | QDialogButtonBox.Cancel
        )
        self.button_box.accepted.connect(self.accept)
        self.button_box.rejected.connect(self.reject)

        self.layout.addWidget(self.button_box)
        self.setLayout(self.layout)
        self.adjustSize()

    # signals
    def update_props(self):
        from ifcopenshell.util.doc import get_entity_doc

        clear_layout(self.prop_layout)
        self.p_cls = self.sel_profile_class.currentText()

        # display spec url
        spec_url = get_entity_doc(self.schema_ver, self.p_cls)["spec_url"]
        linkTemplate = "<a href={0}>{1}</a>"
        self.p_cls_label = HyperlinkLabel()
        self.p_cls_label.setText(
            linkTemplate.format(spec_url, f"Property of {self.p_cls}: Click Me")
        )
        self.unit_label = QLabel("Length Unit: mm")
        self.unit_label.setStyleSheet("font-size: 12px")

        self.prop_layout.addWidget(self.p_cls_label)
        self.prop_layout.addWidget(self.unit_label)

        props = get_profile_props(self.schema, self.p_cls)
        self.prop_sliders = []
        attributes = self.profile.get_info()
        for prop in props:
            editor = PropEditor(prop.name())
            attri_value = attributes.get(prop.name())
            if attri_value is None:
                attri_value = 0
            if "Slope" not in prop.name():
                attri_value /= ifc_tools.get_scale(self.ifc_file)
            editor.set_value(attri_value)
            self.prop_layout.addWidget(editor)
            self.prop_sliders.append((prop.name(), editor))

        self.adjustSize()

    def edit_profile(self):
        self.prop_editors = {}
        for prop_name, editor in self.prop_sliders:
            prop_value = editor.get_value()
            if prop_value != 0:
                if "Slope" not in prop_name:
                    prop_value *= ifc_tools.get_scale(self.ifc_file)
                self.prop_editors[prop_name] = prop_value
        attributes = self.prop_editors
        attributes["ProfileName"] = self.name_input.text()
        attributes["ProfileType"] = "AREA"

        ifcopenshell.api.run(
            "profile.edit_profile",
            self.ifc_file,
            profile=self.profile,
            attributes=attributes,
        )
        print("Update IfcProfile:", self.profile)

        return self.ifc_file


class PropEditor(QWidget):
    def __init__(self, prop_name):
        super().__init__()
        from PySide.QtCore import Qt

        self.layout = QFormLayout()

        self.label = QLabel(f"{prop_name}:")
        self.label.setFixedWidth(200)
        self.line_edit = QLineEdit()
        self.line_edit.setFixedWidth(80)  # Set a fixed width for the line edit
        self.line_edit.setAlignment(Qt.AlignRight)
        self.layout.addRow(self.label, self.line_edit)
        self.setLayout(self.layout)
        self.set_value(0)  # default value = 0

    def get_value(self):
        try:
            value = float(self.line_edit.text())
            return value
        except ValueError:
            QMessageBox.warning(None, "Error", "Accept only Number")
            return

    def set_value(self, value):
        self.line_edit.setText(str(value))


class HyperlinkLabel(QLabel):
    def __init__(self):
        super().__init__()
        self.setStyleSheet("font-size: 12px")
        self.setOpenExternalLinks(True)


# general function
def draw_image_for_ifc_profile(
    draw, profile: ifcopenshell.entity_instance, size: float
):
    """generates image based on `profile` using `PIL.ImageDraw`"""

    settings = ifcopenshell.geom.settings()
    # ifcopenshell ver 0.7
    settings.set(settings.INCLUDE_CURVES, True)
    # ifcopenshell ver 0.8
    # settings.set("dimensionality", ifcopenshell.ifcopenshell_wrapper.CURVES_SURFACES_AND_SOLIDS)
    shape = ifcopenshell.geom.create_shape(settings, profile)
    verts = shape.verts
    edges = shape.edges

    grouped_verts = [[verts[i], verts[i + 1]] for i in range(0, len(verts), 3)]
    grouped_edges = [[edges[i], edges[i + 1]] for i in range(0, len(edges), 2)]

    max_x = max([v[0] for v in grouped_verts])
    min_x = min([v[0] for v in grouped_verts])
    max_y = max([v[1] for v in grouped_verts])
    min_y = min([v[1] for v in grouped_verts])

    dim_x = max_x - min_x
    dim_y = max_y - min_y
    max_dim = max([dim_x, dim_y])
    scale = 100 / max_dim

    for vert in grouped_verts:
        vert[0] = round(scale * (vert[0] - min_x)) + ((size / 2) - scale * (dim_x / 2))
        vert[1] = round(scale * (vert[1] - min_y)) + ((size / 2) - scale * (dim_y / 2))

    for e in grouped_edges:
        draw.line(
            (tuple(grouped_verts[e[0]]), tuple(grouped_verts[e[1]])),
            fill="black",
            width=2,
        )

    return draw


def get_profile_classes(schema):
    classes = ["IfcArbitraryClosedProfileDef"]
    queue = ["IfcParameterizedProfileDef"]
    while queue:
        item = queue.pop()
        subtypes = [s.name() for s in schema.declaration_by_name(item).subtypes()]
        classes.extend(subtypes)
        queue.extend(subtypes)
    return [c for c in sorted(classes)]


def get_profile_props(schema, p_cls):
    props = schema.declaration_by_name(p_cls).all_attributes()
    return props[3:]


def clear_layout(layout):
    while layout.count():
        item = layout.takeAt(0)
        widget = item.widget()
        if widget is not None:
            widget.deleteLater()


# dealing profile shape
def display_arbitrary_closed_profile(profile):
    import Draft

    ifc_file = profile.file
    edit_doc_name = "EditProfile"
    FreeCAD.newDocument(edit_doc_name)
    FreeCAD.setActiveDocument(edit_doc_name)
    FreeCAD.DraftWorkingPlane.setTop()
    FreeCADGui.activateWorkbench("DraftWorkbench")
    FreeCADGui.ActiveDocument.ActiveView.viewTop()

    scaling = 1 / ifc_tools.get_scale(ifc_file)  # FreeCAD use mm  IFC use SI Meter
    settings = ifcopenshell.geom.settings()
    settings.set(settings.INCLUDE_CURVES, True)
    shape = ifcopenshell.geom.create_shape(settings, profile)
    verts = shape.verts
    edges = shape.edges

    grouped_verts = [[verts[i], verts[i + 1]] for i in range(0, len(verts), 3)]
    grouped_edges = [[edges[i], edges[i + 1]] for i in range(0, len(edges), 2)]

    node_order = get_ordered_path_nodes(grouped_edges)
    verts_in_order = []
    for order in node_order:
        v = grouped_verts[order]
        vector = FreeCAD.Vector(v[0], v[1], 0).multiply(scaling)
        verts_in_order.append(vector)

    profile_shape = Draft.make_wire(verts_in_order, closed=True)
    profile_shape.Label = "ProfileShape"
    # profile_shape.MakeFace = False
    FreeCAD.ActiveDocument.recompute()

    return profile_shape


def build_adjacency_list(segments):
    adjacency_list = {}

    for segment in segments:
        start, end = segment
        if start not in adjacency_list:
            adjacency_list[start] = []
        if end not in adjacency_list:
            adjacency_list[end] = []
        adjacency_list[start].append(end)
        adjacency_list[end].append(start)

    return adjacency_list


def find_ordered_path(segments):
    adjacency_list = build_adjacency_list(segments)
    visited = set()
    ordered_path = []

    def dfs(node):
        visited.add(node)
        ordered_path.append(node)
        for neighbor in sorted(adjacency_list[node]):  # Sort for deterministic order
            if neighbor not in visited:
                dfs(neighbor)

    start_node = segments[0][0]  # Start from the first segment's start point
    dfs(start_node)

    # Convert ordered_path to segment pairs
    ordered_segments = []
    for i in range(len(ordered_path) - 1):
        ordered_segments.append([ordered_path[i], ordered_path[i + 1]])

    return ordered_segments


def get_ordered_path_nodes(segments):
    ordered_segments = find_ordered_path(segments)
    ordered_nodes = [s[0] for s in ordered_segments]
    ordered_nodes.append(ordered_segments[-1][-1])
    return ordered_nodes


FreeCADGui.addCommand("BIM_IfcProfiles", BIM_IfcProfiles())
