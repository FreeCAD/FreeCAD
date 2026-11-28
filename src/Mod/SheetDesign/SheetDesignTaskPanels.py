# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 sliptonic <shopinthewoods@gmail.com>
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

"""Task panels of the SheetDesign module.

A task panel is a .ui file in the compiled resource (Gui/Resources/panels) and a small class
here. The base class connects each widget to the property of the feature that has the same
name as the widget. Thus a new value needs only a property on the feature and a widget with
that name in the .ui file.
"""

import os

import FreeCAD
import FreeCADGui

# The icons and the .ui files are in the resource of this library. A document can open when
# a different workbench is active, thus this module loads the library itself.
import SheetDesignGui  # noqa: F401
from PySide import QtCore, QtGui

translate = FreeCAD.Qt.translate

import SheetDesignViewProviders


def sentence_case(text):
    """The text of a drop-down entry: "Material Outside" shows as "Material outside"."""
    return text[:1] + text[1:].lower()


def label_of(obj):
    return obj.Label if obj is not None else ""


class TaskPanel:
    """Base class: loads the .ui file and connects the widgets to the properties."""

    ui_file = ""
    title = ""
    icon = ""

    def __init__(self, obj):
        self.obj = obj
        self.doc = obj.Document
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/" + self.ui_file)
        self.form.setWindowTitle(translate("SheetDesign", self.title))
        self.form.setWindowIcon(QtGui.QIcon(":/icons/" + self.icon + ".svg"))
        self.bindings = []
        self.enumerations = {}
        # A new feature comes with the open transaction of its command. Cancel then removes it.
        self.hidden = SheetDesignViewProviders.PENDING.pop(obj.Name, None)
        self.created = self.hidden is not None
        if not self.created:
            self.doc.openTransaction(translate("SheetDesign", "Edit {}").format(self.title))
        self.connect_properties()
        self.setup()
        self.update()

    # ------------------------------------------------------------- widgets and properties

    def widget(self, name):
        return self.form.findChild(QtCore.QObject, name)

    def connect_properties(self):
        for name in self.obj.PropertiesList:
            widget = self.widget(name)
            if widget is None:
                continue
            kind = widget.metaObject().className()
            if kind == "Gui::QuantitySpinBox":
                self.connect_quantity(widget, name)
            elif kind == "QComboBox":
                self.connect_enumeration(widget, name)
            elif kind == "QCheckBox":
                self.connect_bool(widget, name)

    def connect_quantity(self, widget, name):
        # The binding gives the widget the expression editor of the property.
        binding = FreeCADGui.ExpressionBinding(widget)
        binding.bind(self.obj, name)
        self.bindings.append(binding)
        value = getattr(self.obj, name)
        widget.setProperty("rawValue", getattr(value, "Value", value))
        widget.valueChanged.connect(lambda _value, n=name, w=widget: self.quantity_changed(n, w))

    def quantity_changed(self, name, widget):
        if self.has_expression(name):
            return  # The expression gives the value.
        value = widget.property("rawValue")
        current = getattr(self.obj, name)
        if abs(getattr(current, "Value", current) - value) > 1e-9:
            setattr(self.obj, name, value)
            self.changed()

    def has_expression(self, name):
        return any(path == name for path, _expression in self.obj.ExpressionEngine)

    def connect_enumeration(self, widget, name):
        values = list(self.obj.getEnumerationsOfProperty(name))
        self.enumerations[name] = values
        widget.blockSignals(True)
        widget.clear()
        for value in values:
            widget.addItem(translate("SheetDesign", sentence_case(value)))
        widget.setCurrentIndex(values.index(getattr(self.obj, name)))
        widget.blockSignals(False)
        widget.currentIndexChanged.connect(lambda index, n=name: self.enumeration_changed(n, index))

    def enumeration_changed(self, name, index):
        setattr(self.obj, name, self.enumerations[name][index])
        self.changed()

    def connect_bool(self, widget, name):
        widget.setChecked(bool(getattr(self.obj, name)))
        widget.toggled.connect(lambda state, n=name: self.bool_changed(n, state))

    def bool_changed(self, name, state):
        setattr(self.obj, name, bool(state))
        self.changed()

    # ------------------------------------------------------------------- panel behavior

    def setup(self):
        """Connect the widgets that are not properties. For the subclasses."""

    def update(self):
        """Show the state of the feature. A subclass adds its own widgets."""
        status = self.widget("statusLabel")
        if status is None:
            return
        text = ""
        if not self.obj.isValid():
            text = self.obj.getStatusString()
        elif getattr(self.obj, "Overlap", None) is not None and self.obj.Overlap.Value > 0:
            text = translate(
                "SheetDesign", "Panels of the folded part are in the same space ({:.1f} mm³)"
            ).format(self.obj.Overlap.Value)
        status.setText(text)
        status.setStyleSheet("color: #cc0000;" if text else "")

    def changed(self):
        self.doc.recompute()
        self.update()

    # ----------------------------------------------------------- interface of the dialog

    def getStandardButtons(self):
        return QtGui.QDialogButtonBox.Ok | QtGui.QDialogButtonBox.Cancel

    def close(self):
        """Stop all that the panel started. For the subclasses."""

    def accept(self):
        self.close()
        self.doc.commitTransaction()
        FreeCADGui.ActiveDocument.resetEdit()
        self.doc.recompute()
        return True

    def reject(self):
        self.close()
        if self.created:
            # The abort removes the feature. The end of the edit mode before the abort would
            # commit the transaction.
            self.doc.abortTransaction()
            FreeCADGui.ActiveDocument.resetEdit()
            for name in self.hidden:
                item = self.doc.getObject(name)
                if item is not None and item.ViewObject is not None:
                    item.ViewObject.show()
        else:
            self.doc.abortTransaction()
            FreeCADGui.ActiveDocument.resetEdit()
        self.doc.recompute()
        return True


class EdgeGate:
    """Lets the user select only an edge or a side face that can hold a flange."""

    def __init__(self, source):
        self.source = source

    def allow(self, _doc, obj, sub):
        import SheetDesign

        if obj != self.source or not sub:
            return False
        return SheetDesign.edgeInfo(obj, sub.split(".")[-1]) is not None


class TaskFlange(TaskPanel):
    ui_file = "TaskFlange.ui"
    title = "Flange"
    icon = "SheetDesign_Flange"

    def setup(self):
        self.selecting = False
        self.form.selectEdge.toggled.connect(self.select_edge)

    def update(self):
        super().update()
        source, subs = self.obj.Edge if self.obj.Edge else (None, [])
        self.form.edgeLabel.setText(f"{label_of(source)}: {', '.join(subs)}" if source and subs else "")
        profile = self.obj.Profile
        self.form.Radius.setEnabled(not self.obj.UseRuleRadius)
        self.form.CornerType.setEnabled(len(subs) > 1)
        self.form.CornerGap.setEnabled(len(subs) > 1 and self.obj.CornerType != "Open")
        self.form.profileLabel.setText(label_of(profile))
        # A profile gives the length. The explicit relief types use the sizes of the flange.
        for name in ("Length", "LengthType", "Miter1", "Miter2"):
            self.widget(name).setEnabled(profile is None)
        own_relief = self.obj.ReliefType in ("Rectangle", "Round")
        self.form.ReliefWidth.setEnabled(own_relief)
        self.form.ReliefDepth.setEnabled(own_relief)
        self.form.MiterGap.setEnabled(bool(self.obj.AutoMiter))

    def select_edge(self, active):
        """Show the feature before the flange, and take the next edge that the user selects."""
        source = self.obj.Edge[0] if self.obj.Edge else None
        if source is None:
            return
        self.selecting = active
        if active:
            self.obj.ViewObject.hide()
            source.ViewObject.show()
            FreeCADGui.Selection.clearSelection()
            FreeCADGui.Selection.addSelectionGate(EdgeGate(source))
            FreeCADGui.Selection.addObserver(self)
        else:
            FreeCADGui.Selection.removeObserver(self)
            FreeCADGui.Selection.removeSelectionGate()
            source.ViewObject.hide()
            self.obj.ViewObject.show()

    def addSelection(self, _doc, obj_name, sub, _point):
        """Selection observer: a valid edge was selected."""
        source = self.obj.Edge[0]
        if not self.selecting or obj_name != source.Name or not sub:
            return
        self.obj.Edge = (source, [sub.split(".")[-1]])
        self.form.selectEdge.setChecked(False)
        FreeCADGui.Selection.clearSelection()
        self.changed()

    def close(self):
        if self.selecting:
            self.form.selectEdge.setChecked(False)


class TaskBasePanel(TaskPanel):
    ui_file = "TaskBasePanel.ui"
    title = "Base Panel"
    icon = "SheetDesign_Base"

    def update(self):
        super().update()
        self.form.profileLabel.setText(label_of(self.obj.Profile))
        self.form.ruleLabel.setText(label_of(self.obj.Rule))
        # The width is for an open profile: the section of the part.
        profile = self.obj.Profile
        is_open = bool(profile and not profile.Shape.Faces and not any(w.isClosed() for w in profile.Shape.Wires))
        self.form.Width.setEnabled(is_open)
        self.form.Symmetric.setEnabled(is_open)


class TaskCut(TaskPanel):
    ui_file = "TaskCut.ui"
    title = "Cut"
    icon = "SheetDesign_Cut"

    def update(self):
        super().update()
        self.form.profileLabel.setText(label_of(self.obj.Profile))


class TaskRefold(TaskPanel):
    ui_file = "TaskRefold.ui"
    title = "Refold"
    icon = "SheetDesign_Refold"

    def setup(self):
        """One row with an angle for each bend line."""
        table = self.form.anglesTable
        lines = len(self.obj.BendLines.Shape.Edges) if self.obj.BendLines else 0
        angles = list(self.obj.Angles) or [90.0]
        table.setRowCount(lines)
        table.horizontalHeader().setStretchLastSection(True)
        table.setHorizontalHeaderLabels([translate("SheetDesign", "Angle")])
        self.spins = []
        for row in range(lines):
            spin = QtGui.QDoubleSpinBox()
            spin.setRange(-180.0, 180.0)
            spin.setDecimals(2)
            spin.setSuffix(" °")
            # A line with no value uses the last value of the list.
            spin.setValue(angles[min(row, len(angles) - 1)])
            spin.valueChanged.connect(self.angles_changed)
            table.setCellWidget(row, 0, spin)
            self.spins.append(spin)

    def angles_changed(self, _value):
        self.obj.Angles = [spin.value() for spin in self.spins]
        self.changed()

    def update(self):
        super().update()
        self.form.profileLabel.setText(label_of(self.obj.Profile))
        self.form.linesLabel.setText(label_of(self.obj.BendLines))


class TaskUnfold(TaskPanel):
    ui_file = "TaskUnfold.ui"
    title = "Unfold"
    icon = "SheetDesign_Unfold"

    def update(self):
        super().update()
        source, subs = self.obj.Face if self.obj.Face else (None, [])
        self.form.faceLabel.setText(f"{label_of(source)}.{subs[0]}" if source and subs else "")
        self.form.ruleLabel.setText(label_of(self.obj.Rule))
        # The values that Unfold found in the solid.
        thickness = ""
        bends = ""
        if self.obj.isValid():
            names = self.obj.Shape.ElementReverseMap.values()
            panels = {name.rsplit("_", 1)[0] for name in names if name.endswith(("_Top", "_Bottom"))}
            bends = str(max(len(panels) - 1, 0))
            box = self.obj.Shape.BoundBox
            volume, area = self.obj.Shape.Volume, self.obj.Shape.Area
            thickness = f"{2 * volume / area:.2f} mm" if area > 0 and box.isValid() else ""
        self.form.thicknessLabel.setText(thickness)
        self.form.bendsLabel.setText(bends)


class FoldPreview:
    """A temporary object that shows a shape. The panel removes it when it closes."""

    NAME = "FoldPreview"

    def __init__(self):
        self.obj = None

    def show(self, shape, like):
        if self.obj is None:
            self.obj = like.Document.addObject("Part::Feature", self.NAME)
            view, source = self.obj.ViewObject, like.ViewObject
            if view is not None and source is not None:
                for name in ("ShapeColor", "LineColor", "LineWidth", "DisplayMode"):
                    try:
                        setattr(view, name, getattr(source, name))
                    except (AttributeError, ValueError):
                        pass
                view.Selectable = False
        self.obj.Shape = shape

    def remove(self):
        if self.obj is not None:
            try:
                self.obj.Document.removeObject(self.obj.Name)
            except (ReferenceError, RuntimeError):  # The object is gone, for example after an undo.
                pass
        self.obj = None

    def is_shown(self):
        return self.obj is not None


class TaskFlatPattern(TaskPanel):
    ui_file = "TaskFlatPattern.ui"
    title = "Flat Pattern"
    icon = "SheetDesign_FlatPattern"

    def setup(self):
        self.form.exportButton.clicked.connect(self.export)
        self.form.drawingButton.clicked.connect(self.drawing)
        self.form.bendsTable.itemChanged.connect(self.sequence_changed)
        # A flat pattern from an older file has no bend data before its first recompute.
        if self.obj.Source is not None and not list(self.obj.BendAngles):
            self.obj.touch()
            self.doc.recompute()
        self.preview = FoldPreview()
        self.source_was_visible = None
        self.form.foldSequence.setChecked(True)
        self.form.foldSlider.valueChanged.connect(self.fold_changed)
        self.form.foldSlider.sliderReleased.connect(self.fold_measured)
        self.form.foldSequence.toggled.connect(lambda _state: self.fold_changed(self.form.foldSlider.value()))
        self.form.foldCheck.clicked.connect(self.fold_check)
        self.form.foldFrames.clicked.connect(self.fold_frames)

    # ------------------------------------------------------------------- fold state

    def fold_state(self):
        return self.form.foldSlider.value() / 100.0, self.form.foldSequence.isChecked()

    def fold_changed(self, value):
        """Show the part in the state of the slider. This is a view only."""
        import SheetDesignFold

        source = self.obj.Source
        if source is None or source.ViewObject is None:
            return
        if value >= 100:
            self.fold_end()
            self.form.foldLabel.setText("")
            return
        state, in_sequence = self.fold_state()
        try:
            shape, _overlap = SheetDesignFold.shape(self.obj, state, in_sequence)
        except RuntimeError as error:
            self.form.foldLabel.setText(str(error))
            return
        if self.source_was_visible is None:
            self.source_was_visible = source.ViewObject.Visibility
            source.ViewObject.hide()
        self.preview.show(shape, source)
        text = translate("SheetDesign", "Fold state: {} %").format(value)
        note = SheetDesignFold.message(self.obj)
        self.form.foldLabel.setText(text + (". " + translate("SheetDesign", note) if note else ""))

    def fold_measured(self):
        """The slider stopped: measure the overlap of this state."""
        import SheetDesignFold

        value = self.form.foldSlider.value()
        if value >= 100 or self.obj.Source is None:
            return
        if SheetDesignFold.motion(self.obj) is not None:
            # The solver moves thin panels. The thick panels of a coupled fold touch on the
            # way, thus an overlap value has no meaning before the end.
            return
        state, in_sequence = self.fold_state()
        _shape, overlap = SheetDesignFold.shape(self.obj, state, in_sequence, True)
        self.form.foldLabel.setText(
            translate("SheetDesign", "Fold state: {} %. Overlap: {:.1f} mm³").format(value, overlap)
        )

    def fold_check(self):
        import SheetDesignFold

        if SheetDesignFold.motion(self.obj) is not None:
            self.form.foldLabel.setText(
                translate("SheetDesign", "A part with coupled folds has no overlap check on the way. ")
                + translate("SheetDesign", SheetDesignFold.message(self.obj))
            )
            return
        worst, where = SheetDesignFold.worst_overlap(self.obj, 20, self.form.foldSequence.isChecked())
        if worst > 0.0:
            text = translate("SheetDesign", "Largest overlap in the fold: {:.1f} mm³ at {:.0f} %").format(
                worst, where * 100.0
            )
        else:
            text = translate("SheetDesign", "No overlap in 20 steps of the fold")
        self.form.foldLabel.setText(text)

    def fold_frames(self, folder=None, steps=60, size=(1280, 720)):
        """Save one image for each step of the fold."""
        if not folder:
            folder = QtGui.QFileDialog.getExistingDirectory(
                FreeCADGui.getMainWindow(), translate("SheetDesign", "Folder for the Frames")
            )
        if not folder:
            return 0
        view = FreeCADGui.ActiveDocument.ActiveView
        before = self.form.foldSlider.value()
        for index in range(steps + 1):
            self.form.foldSlider.setValue(round(100.0 * index / steps))
            FreeCADGui.updateGui()
            view.saveImage(os.path.join(folder, "fold_%04d.png" % index), size[0], size[1], "Current")
        self.form.foldSlider.setValue(before)
        return steps + 1

    def fold_end(self):
        self.preview.remove()
        source = self.obj.Source
        if self.source_was_visible is not None and source is not None and source.ViewObject is not None:
            source.ViewObject.Visibility = self.source_was_visible
        self.source_was_visible = None

    def close(self):
        self.fold_end()

    def update(self):
        super().update()
        self.form.sourceLabel.setText(label_of(self.obj.Source))
        # The table of the bends: what the operator of the press brake must know.
        import SheetDesignExport

        table = self.form.bendsTable
        angles, radii = list(self.obj.BendAngles), list(self.obj.BendRadii)
        # One row for each score: the bends on one straight line fold in one action.
        self.score_groups, numbers = SheetDesignExport.scores_of(self.obj, len(angles))
        table.blockSignals(True)
        table.setRowCount(len(self.score_groups))
        table.setHorizontalHeaderLabels(
            [
                translate("SheetDesign", "Sequence"),
                translate("SheetDesign", "Direction"),
                translate("SheetDesign", "Angle"),
                translate("SheetDesign", "Radius"),
            ]
        )
        table.horizontalHeader().setStretchLastSection(True)
        up, down = translate("SheetDesign", "Up"), translate("SheetDesign", "Down")

        def unique(values):
            result = []
            for value in values:
                if value not in result:
                    result.append(value)
            return "/".join(result)

        for row, group in enumerate(self.score_groups):
            texts = (
                str(numbers[row]),
                unique(up if angles[i] >= 0 else down for i in group),
                unique(f"{abs(angles[i]):g} °" for i in group),
                unique(f"{radii[i]:g} mm" for i in group),
            )
            for column, text in enumerate(texts):
                item = QtGui.QTableWidgetItem(text)
                if column > 0:
                    item.setFlags(item.flags() & ~QtCore.Qt.ItemIsEditable)
                table.setItem(row, column, item)
        table.blockSignals(False)

    def sequence_changed(self, item):
        """A number in the first column: put the score at that place of the sequence."""
        if item.column() != 0:
            return
        import SheetDesignExport

        groups, numbers = SheetDesignExport.scores_of(self.obj, len(self.obj.BendAngles))
        count = len(groups)
        try:
            wanted = int(item.text())
        except ValueError:
            wanted = 0
        if 1 <= wanted <= count and wanted != numbers[item.row()]:
            # The score that had this number gets the old number of the changed score.
            other = numbers.index(wanted)
            numbers[other], numbers[item.row()] = numbers[item.row()], wanted
            order = [0] * len(self.obj.BendAngles)
            for group, number in zip(groups, numbers):
                for index in group:
                    order[index] = number
            self.obj.BendOrder = order
            self.changed()
        else:
            self.update()

    def drawing(self):
        import SheetDesignDrawing

        page = SheetDesignDrawing.make_drawing(self.obj)[0]
        if page.ViewObject is not None:
            page.ViewObject.show()

    def export(self):
        FreeCADGui.Selection.clearSelection()
        FreeCADGui.Selection.addSelection(self.obj)
        FreeCADGui.runCommand("SheetDesign_ExportDxf")


def edge_text(link):
    source, subs = link if link else (None, [])
    return f"{label_of(source)}.{subs[0]}" if source and subs else label_of(source)


class TaskExtend(TaskPanel):
    ui_file = "TaskExtend.ui"
    title = "Extend"
    icon = "SheetDesign_Extend"

    def update(self):
        super().update()
        profile = self.obj.Profile
        self.form.edgeLabel.setText(edge_text(self.obj.Edge))
        self.form.profileLabel.setText(label_of(profile))
        # A profile gives the added material. The length and the gaps are for an edge.
        for name in ("Length", "Gap1", "Gap2"):
            self.widget(name).setEnabled(profile is None)


class TaskFold(TaskPanel):
    ui_file = "TaskFold.ui"
    title = "Fold"
    icon = "SheetDesign_Fold"

    def update(self):
        super().update()
        self.form.linesLabel.setText(label_of(self.obj.BendLines))


class TaskHem(TaskPanel):
    ui_file = "TaskHem.ui"
    title = "Hem"
    icon = "SheetDesign_Hem"

    def update(self):
        super().update()
        self.form.edgeLabel.setText(edge_text(self.obj.Edge))
        self.form.Gap.setEnabled(self.obj.Type != "Flat")
        self.form.Length.setEnabled(self.obj.Type != "Rolled")


class TaskCornerRelief(TaskPanel):
    ui_file = "TaskCornerRelief.ui"
    title = "Corner Relief"
    icon = "SheetDesign_CornerRelief"


class TaskBaseShape(TaskPanel):
    ui_file = "TaskBaseShape.ui"
    title = "Base Shape"
    icon = "SheetDesign_BaseShape"

    def update(self):
        super().update()
        self.form.ruleLabel.setText(label_of(self.obj.Rule))
        self.form.Height.setEnabled(self.obj.Type != "Plate")
        self.form.FlangeWidth.setEnabled(self.obj.Type == "Hat")


class TaskConvert(TaskPanel):
    ui_file = "TaskConvert.ui"
    title = "Convert"
    icon = "SheetDesign_Convert"

    def update(self):
        super().update()
        source, subs = self.obj.Faces if self.obj.Faces else (None, [])
        text = translate("SheetDesign", "{} ({} faces)").format(label_of(source), len(subs))
        self.form.facesLabel.setText(text if source else "")
        self.form.ruleLabel.setText(label_of(self.obj.Rule))


class TaskForm(TaskPanel):
    ui_file = "TaskForm.ui"
    title = "Form"
    icon = "SheetDesign_Form"

    def update(self):
        super().update()
        tool, subs = self.obj.Tool if self.obj.Tool else (None, [])
        self.form.toolLabel.setText(f"{label_of(tool)} ({', '.join(subs)})" if tool else "")


class TaskPerforation(TaskPanel):
    ui_file = "TaskPerforation.ui"
    title = "Perforation"
    icon = "SheetDesign_Perforation"

    def update(self):
        super().update()
        _source, subs = self.obj.Edges if self.obj.Edges else (None, [])
        self.form.edgesLabel.setText(", ".join(subs) if subs else translate("SheetDesign", "All bends"))


PANELS = {
    "SheetDesign::Convert": TaskConvert,
    "SheetDesign::Form": TaskForm,
    "SheetDesign::Perforation": TaskPerforation,
    "SheetDesign::Extend": TaskExtend,
    "SheetDesign::Fold": TaskFold,
    "SheetDesign::Hem": TaskHem,
    "SheetDesign::CornerRelief": TaskCornerRelief,
    "SheetDesign::BaseShape": TaskBaseShape,
    "SheetDesign::BasePanel": TaskBasePanel,
    "SheetDesign::Flange": TaskFlange,
    "SheetDesign::Cut": TaskCut,
    "SheetDesign::Refold": TaskRefold,
    "SheetDesign::Unfold": TaskUnfold,
    "SheetDesign::FlatPattern": TaskFlatPattern,
}


def panel_for(obj):
    """Return a new task panel for the feature, or None if the type has no panel."""
    panel_class = PANELS.get(obj.TypeId)
    return panel_class(obj) if panel_class else None
