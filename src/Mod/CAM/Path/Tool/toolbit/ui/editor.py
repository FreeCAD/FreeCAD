# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2025 Samuel Abels <knipknap@gmail.com>                  *
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

"""Widget for editing a ToolBit object."""

from typing import Optional
from PySide import QtGui, QtCore
import FreeCAD
import FreeCADGui
from ...shape.ui.shapewidget import ShapeWidget
from ...docobject.ui import DocumentObjectEditorWidget
from ..models.base import ToolBit


class ToolBitPropertiesWidget(QtGui.QWidget):
    """
    A composite widget for editing the properties and shape of a ToolBit.
    """

    # Signal emitted when the toolbit data has been modified
    toolBitChanged = QtCore.Signal()

    def __init__(self, toolbit: Optional[ToolBit] = None, parent=None, icon: bool = True):
        super().__init__(parent)
        self._toolbit = None
        self._show_shape = icon

        # UI Elements
        self._label_edit = QtGui.QLineEdit()
        self._id_label = QtGui.QLabel()  # Read-only ID
        self._id_label.setTextInteractionFlags(QtCore.Qt.TextSelectableByMouse)

        theicon = toolbit.get_icon() if toolbit else None
        abbr = theicon.abbreviations if theicon else {}
        self._property_editor = DocumentObjectEditorWidget(property_suffixes=abbr)
        self._property_editor.setSizePolicy(
            QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Expanding
        )
        self._shape_widget = None  # Will be created in load_toolbit

        # Layout
        toolbit_group_box = QtGui.QGroupBox(FreeCAD.Qt.translate("CAM", "Tool Bit"))
        form_layout = QtGui.QFormLayout(toolbit_group_box)
        form_layout.addRow("Label:", self._label_edit)
        form_layout.addRow("ID:", self._id_label)

        main_layout = QtGui.QVBoxLayout(self)
        main_layout.addWidget(toolbit_group_box)

        properties_group_box = QtGui.QGroupBox(FreeCAD.Qt.translate("CAM", "Properties"))
        properties_group_box.setSizePolicy(QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Expanding)
        properties_layout = QtGui.QVBoxLayout(properties_group_box)
        properties_layout.setSpacing(5)
        properties_layout.addWidget(self._property_editor)

        # Ensure the layout expands horizontally
        properties_layout.setAlignment(QtCore.Qt.AlignLeft | QtCore.Qt.AlignTop)

        # Set stretch factor to make property editor expand
        properties_layout.setStretchFactor(self._property_editor, 1)

        main_layout.addWidget(properties_group_box)

        # Add stretch before shape widget to push it towards the bottom
        main_layout.addStretch(1)

        # Layout for centering the shape widget (created later)
        self._shape_display_layout = QtGui.QHBoxLayout()
        self._shape_display_layout.addStretch(1)

        # Placeholder for the widget
        self._shape_display_layout.addStretch(1)
        main_layout.addLayout(self._shape_display_layout)

        # Connections
        self._label_edit.editingFinished.connect(self._on_label_changed)
        self._property_editor.propertyChanged.connect(self.toolBitChanged)

        if toolbit:
            self.load_toolbit(toolbit)

    def _on_label_changed(self):
        """Update the toolbit's label when the line edit changes."""
        if self._toolbit and self._toolbit.obj:
            new_label = self._label_edit.text()
            if self._toolbit.obj.Label != new_label:
                self._toolbit.obj.Label = new_label
                self.toolBitChanged.emit()

    def load_toolbit(self, toolbit: ToolBit):
        """Load a ToolBit object into the editor."""
        self._toolbit = toolbit
        if not self._toolbit or not self._toolbit.obj:
            # Clear or disable fields if toolbit is invalid
            self._label_edit.clear()
            self._label_edit.setEnabled(False)
            self._id_label.clear()
            self._property_editor.setObject(None)
            # Clear existing shape widget if any
            if self._shape_widget:
                self._shape_display_layout.removeWidget(self._shape_widget)
                self._shape_widget.deleteLater()
                self._shape_widget = None
            self.setEnabled(False)
            return

        self.setEnabled(True)
        self._label_edit.setEnabled(True)
        self._label_edit.setText(self._toolbit.obj.Label)
        self._id_label.setText(self._toolbit.get_id())

        # Get properties and suffixes
        props_to_show = self._toolbit._get_props(("Shape", "Attributes"))
        icon = self._toolbit._tool_bit_shape.get_icon()
        suffixes = icon.abbreviations if icon else {}
        self._property_editor.setObject(self._toolbit.obj)
        self._property_editor.setPropertiesToShow(props_to_show, suffixes)

        # Clear old shape widget and create/add new one if shape exists
        if self._shape_widget:
            self._shape_display_layout.removeWidget(self._shape_widget)
            self._shape_widget.deleteLater()
            self._shape_widget = None

        if self._show_shape and self._toolbit._tool_bit_shape:
            self._shape_widget = ShapeWidget(shape=self._toolbit._tool_bit_shape, parent=self)
            self._shape_widget.setMinimumSize(200, 150)
            # Insert into the middle slot of the HBox layout
            self._shape_display_layout.insertWidget(1, self._shape_widget)

    def save_toolbit(self):
        """
        Applies changes from the editor widgets back to the ToolBit object.
        Note: Most changes are applied via signals, but this can be called
        for explicit save actions.
        """
        # Ensure label is updated if focus is lost without pressing Enter
        self._on_label_changed()

        # No need to explicitly save the toolbit object itself here,
        # as properties were modified directly on toolbit.obj


class ToolBitEditorPanel(QtGui.QWidget):
    """
    A widget for editing a ToolBit object, wrapping ToolBitEditorWidget
    and providing standard dialog buttons.
    """

    # Signals
    accepted = QtCore.Signal(ToolBit)
    rejected = QtCore.Signal()
    toolBitChanged = QtCore.Signal()  # Re-emit signal from inner widget

    def __init__(self, toolbit: ToolBit | None = None, parent=None):
        super().__init__(parent)

        # Create the main editor widget
        self._editor_widget = ToolBitPropertiesWidget(toolbit, self)

        # Create the button box
        buttons = QtGui.QDialogButtonBox.Ok | QtGui.QDialogButtonBox.Cancel
        self._button_box = QtGui.QDialogButtonBox(buttons)

        # Connect button box signals to custom signals
        self._button_box.accepted.connect(self._accepted)
        self._button_box.rejected.connect(self.rejected.emit)

        # Layout
        main_layout = QtGui.QVBoxLayout(self)
        main_layout.addWidget(self._editor_widget)
        main_layout.addWidget(self._button_box)

        # Connect the toolBitChanged signal from the inner widget
        self._editor_widget.toolBitChanged.connect(self.toolBitChanged)

    def _accepted(self):
        self.accepted.emit(self._editor_widget._toolbit)

    def load_toolbit(self, toolbit: ToolBit):
        """Load a ToolBit object into the editor."""
        self._editor_widget.load_toolbit(toolbit)

    def save_toolbit(self):
        """Applies changes from the editor widgets back to the ToolBit object."""
        self._editor_widget.save_toolbit()


class ToolBitEditor(QtGui.QWidget):
    """
    A widget for editing a ToolBit object, wrapping ToolBitEditorWidget
    and providing standard dialog buttons.
    """

    # Signals
    toolBitChanged = QtCore.Signal()  # Re-emit signal from inner widget

    def __init__(self, toolbit: ToolBit, parent=None):
        super().__init__(parent)
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/ToolBitEditor.ui")

        self.toolbit = toolbit
        # self.tool_no = tool_no
        self.default_title = self.form.windowTitle()

        # Get first tab from the form, add the shape widget at the top.
        tool_tab_layout = self.form.toolTabLayout
        widget = ShapeWidget(toolbit._tool_bit_shape)
        tool_tab_layout.addWidget(widget)

        # Add tool properties editor to the same tab.
        props = ToolBitPropertiesWidget(toolbit, self, icon=False)
        props.toolBitChanged.connect(self._update)
        # props.toolNoChanged.connect(self._on_tool_no_changed)
        tool_tab_layout.addWidget(props)

        self.form.tabWidget.setCurrentIndex(0)
        self.form.tabWidget.currentChanged.connect(self._on_tab_switched)

        # Hide second tab (tool notes) for now.
        self.form.tabWidget.setTabVisible(1, False)

        # Feeds & Speeds
        self.feeds_tab_idx = None
        """
        TODO: disabled for now.
        if tool.supports_feeds_and_speeds():
            label = translate('CAM', 'Feeds && Speeds')
            self.feeds = FeedsAndSpeedsWidget(db, serializer, tool, parent=self)
            self.feeds_tab_idx = self.form.tabWidget.insertTab(1, self.feeds, label)
        else:
            self.feeds = None
            self.feeds_tab_idx = None

        self.form.lineEditCoating.setText(toolbit.get_coating())
        self.form.lineEditCoating.textChanged.connect(toolbit.set_coating)
        self.form.lineEditHardness.setText(toolbit.get_hardness())
        self.form.lineEditHardness.textChanged.connect(toolbit.set_hardness)
        self.form.lineEditMaterials.setText(toolbit.get_materials())
        self.form.lineEditMaterials.textChanged.connect(toolbit.set_materials)
        self.form.lineEditSupplier.setText(toolbit.get_supplier())
        self.form.lineEditSupplier.textChanged.connect(toolbit.set_supplier)
        self.form.plainTextEditNotes.setPlainText(tool.get_notes())
        self.form.plainTextEditNotes.textChanged.connect(self._on_notes_changed)
        """

    def _update(self):
        title = self.default_title
        tool_name = self.toolbit.label
        if tool_name:
            title = "{} - {}".format(tool_name, title)
        self.form.setWindowTitle(title)

    def _on_tab_switched(self, index):
        if index == self.feeds_tab_idx:
            self.feeds.update()

    def _on_notes_changed(self):
        self.toolbit.set_notes(self.form.plainTextEditNotes.toPlainText())

    def _on_tool_no_changed(self, value):
        self.tool_no = value

    def show(self):
        return self.form.exec_()
