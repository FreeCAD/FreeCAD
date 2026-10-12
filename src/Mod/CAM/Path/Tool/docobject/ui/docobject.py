# SPDX-License-Identifier: LGPL-2.1-or-later

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

"""Widget for editing a list of properties of a DocumentObject."""

import re
from PySide import QtGui, QtCore
from .property import BasePropertyEditorWidget


def _resolve_key(mapping, prop_name):
    """
    Look a property name up in `mapping`, ignoring case if need be.

    The shape drawings name their labels after the properties they dimension,
    but not always in the same case - one shape spells its property cuttingAngle
    where the drawing says cutting_angle.
    """
    if prop_name in mapping:
        return prop_name
    lowered = prop_name.lower()
    return next((key for key in mapping if key.lower() == lowered), None)


def _get_label_text(prop_name, keep_case=False, preserve_consecutive_caps=False):
    """Generate a human-readable label from a property name."""
    # First, replace underscores and hyphens with spaces
    prop_name = prop_name.replace("_", " ").replace("-", " ")
    # Add space before capital letters (CamelCase splitting)
    if preserve_consecutive_caps:
        s1 = re.sub(r"(?<![A-Z])([A-Z][a-z]+)", r" \1", prop_name)
        # Skip splitting short capital sequences (e.g., VBit stays VBit)
        s2 = re.sub(r"([A-Z]{3,})([A-Z][a-z])", r"\1 \2", s1)
    else:
        s1 = re.sub(r"([A-Z][a-z]+)", r" \1", prop_name)
        # Add space before sequences of capitals (e.g., ID) followed by lowercase
        s2 = re.sub(r"([A-Z]+)([A-Z][a-z])", r"\1 \2", s1)
    # Add space before sequences of capitals followed by end of string
    s3 = re.sub(r"([A-Z]+)$", r" \1", s2)
    # Remove leading/trailing spaces and collapse multiple spaces
    result = " ".join(s3.split())
    if not keep_case:
        return result.capitalize()
    return result.title()


def _reserve_bold_width(label):
    """
    Widen `label` to the space its text needs in bold.

    Highlighting a row bolds its label, and a bolder label is a wider one. In a
    form layout every label shares one column, so the longest label growing
    drags the editors sideways with it. Claiming the bold width up front keeps
    the column still.
    """
    plain = label.font()
    bold = QtGui.QFont(plain)
    bold.setBold(True)
    # Ask the label itself rather than the font metrics: its size hint also
    # covers the margins and indent, and rounds the way the layout will.
    label.setFont(bold)
    width = label.sizeHint().width()
    label.setFont(plain)
    label.setMinimumWidth(width)


class DocumentObjectEditorWidget(QtGui.QWidget):
    """
    A widget that displays a user friendly form for editing properties of a
    FreeCAD DocumentObject.
    """

    # Signal emitted when any underlying property value might have changed
    propertyChanged = QtCore.Signal()
    # Emitted with the property name the mouse is over, or "" when it leaves
    propertyHovered = QtCore.Signal(str)

    # Color used to pick out a highlighted row. Callers that highlight the same
    # property elsewhere should set this to match.
    highlight_color = "#ff8c00"

    def __init__(self, obj=None, properties_to_show=None, property_suffixes=None, parent=None):
        """
        Initialize the editor widget.

        Args:
            obj (App.DocumentObject, optional): The object to edit. Defaults to None.
            properties_to_show (list[str], optional): List of property names to display.
                                                     Defaults to None (shows nothing).
            property_suffixes (dict[str, str], optional): Dictionary mapping property names
                                                          to suffixes for their labels.
                                                          Defaults to None.
            parent (QWidget, optional): The parent widget. Defaults to None.
        """
        super().__init__(parent)
        self._obj = obj
        self._properties_to_show = properties_to_show if properties_to_show else []
        self._property_suffixes = property_suffixes if property_suffixes else {}
        self._property_editors = {}  # Store {prop_name: editor_widget}
        self._property_labels = {}  # Store {prop_name: label}
        self._hover_targets = {}  # Store {widget: prop_name}
        self._highlighted = ""
        self._hovered = ""

        self.setMouseTracking(True)
        self._layout = QtGui.QFormLayout(self)
        self._layout.setContentsMargins(0, 0, 0, 0)
        self._layout.setFieldGrowthPolicy(QtGui.QFormLayout.FieldGrowthPolicy.ExpandingFieldsGrow)

        self._populate_form()

    def _clear_form(self):
        """Remove all rows from the form layout."""
        while self._layout.rowCount() > 0:
            self._layout.removeRow(0)
        self._property_editors.clear()
        self._property_labels.clear()
        self._hover_targets.clear()
        self._highlighted = ""
        self._hovered = ""

    def _populate_form(self):
        """Create and add property editors to the form."""
        self._clear_form()
        if not self._obj:
            return

        for prop_name in self._properties_to_show:
            # Only create an editor if the property exists on the object
            if not hasattr(self._obj, prop_name):
                continue

            editor_widget = BasePropertyEditorWidget.for_property(self._obj, prop_name, self)
            label_text = _get_label_text(prop_name)
            suffix_key = _resolve_key(self._property_suffixes, prop_name)
            suffix = self._property_suffixes.get(suffix_key) if suffix_key else None
            if suffix:
                label_text = f"{label_text} ({suffix}):"
            else:
                label_text = f"{label_text}:"

            label = QtGui.QLabel(label_text)
            _reserve_bold_width(label)
            self._layout.addRow(label, editor_widget)
            self._property_editors[prop_name] = editor_widget
            self._property_labels[prop_name] = label

            # Watch both halves of the row, and the editor's inner input, so
            # that hovering any part of the row counts.
            watched = [label, editor_widget]
            watched.extend(editor_widget.findChildren(QtGui.QWidget))
            for widget in watched:
                widget.installEventFilter(self)
                self._hover_targets[widget] = prop_name

            # Connect the editor's signal to our own signal
            editor_widget.propertyChanged.connect(self.propertyChanged)

    def eventFilter(self, obj, event):
        """Report the property whose row the mouse is over."""
        prop_name = self._hover_targets.get(obj)
        if prop_name is not None:
            if event.type() == QtCore.QEvent.Enter:
                self._set_hovered(prop_name)
            # A row is made of several widgets, so leaving one of them is not
            # necessarily leaving the row.
            elif event.type() == QtCore.QEvent.Leave:
                self._set_hovered(self._row_at_cursor())
        return super().eventFilter(obj, event)

    def _row_at_cursor(self):
        """The property whose row the pointer is over, "" for none."""
        widget = QtGui.QApplication.widgetAt(QtGui.QCursor.pos())
        while widget is not None and widget not in self._hover_targets:
            if widget is self:
                return self._row_at(self.mapFromGlobal(QtGui.QCursor.pos()))
            widget = widget.parentWidget()
        return self._hover_targets.get(widget, "")

    def _row_at(self, pos):
        """
        The property whose row `pos` falls in, "" for none.

        Only the vertical position counts, so the whole width of the form
        belongs to a row. The rows are treated as touching: the spacing
        between two of them goes to the nearer, otherwise crossing it would
        read as a moment of hovering nothing.
        """
        bands = []
        for prop_name, label in self._property_labels.items():
            editor = self._property_editors.get(prop_name)
            if editor is not None:
                bands.append((label.geometry().united(editor.geometry()), prop_name))
        if not bands:
            return ""
        if pos.y() < min(b.top() for b, _ in bands) or pos.y() > max(b.bottom() for b, _ in bands):
            return ""
        return min(bands, key=lambda b: max(b[0].top() - pos.y(), pos.y() - b[0].bottom(), 0))[1]

    def _set_hovered(self, prop_name):
        if prop_name == self._hovered:
            return
        self._hovered = prop_name
        self.propertyHovered.emit(prop_name)

    def mouseMoveEvent(self, event):
        # Reaches us only for the parts of a row no child widget covers.
        self._set_hovered(self._row_at(event.pos()))
        super().mouseMoveEvent(event)

    def leaveEvent(self, event):
        self._set_hovered(self._row_at_cursor())
        super().leaveEvent(event)

    def highlight_property(self, prop_name):
        """Pick out one property's row; pass "" or None to clear."""
        prop_name = _resolve_key(self._property_labels, prop_name or "") or ""
        if prop_name == self._highlighted:
            return
        previous, self._highlighted = self._highlighted, prop_name
        for name in (previous, prop_name):
            label = self._property_labels.get(name)
            if label is None:
                continue
            picked = name == prop_name
            label.setStyleSheet(
                f"color: {self.highlight_color}; font-weight: bold;" if picked else ""
            )
            self._set_mouse_over(self._property_editors.get(name), picked)

    @staticmethod
    def _set_mouse_over(editor, on):
        """
        Paint an editor as the theme paints it under the mouse.

        Styles key their hover off WA_UnderMouse, so setting it and repolishing
        borrows whatever the current theme does, rather than guessing a color.
        """
        widget = getattr(editor, "_editor_widget", None) if editor else None
        if widget is None:
            return
        if not on and widget.rect().contains(widget.mapFromGlobal(QtGui.QCursor.pos())):
            return  # the pointer really is over it
        widget.setAttribute(QtCore.Qt.WA_UnderMouse, on)
        widget.style().unpolish(widget)
        widget.style().polish(widget)
        widget.update()

    def focus_property(self, prop_name):
        """Give keyboard focus to one property's editor."""
        editor = self._property_editors.get(_resolve_key(self._property_editors, prop_name))
        if not editor:
            return
        # The editor is a container around the input; the input takes the focus.
        widget = getattr(editor, "_editor_widget", None) or editor
        widget.setFocus(QtCore.Qt.OtherFocusReason)
        if hasattr(widget, "selectAll"):
            widget.selectAll()

    def setObject(self, obj):
        """Set or change the DocumentObject being edited."""
        if obj != self._obj:
            self._obj = obj
            # Re-populate might be too slow if only object changes,
            # better to just re-attach existing editors.
            # self._populate_form()
            for prop_name, editor in self._property_editors.items():
                editor.attachTo(self._obj, prop_name)

    def setPropertiesToShow(self, properties_to_show, property_suffixes=None):
        """Set or change the list of properties to display."""
        self._properties_to_show = properties_to_show if properties_to_show else []
        self._property_suffixes = property_suffixes if property_suffixes else {}
        self._populate_form()  # Rebuild the form completely

    def updateUI(self):
        """Update all child editor widgets from the object's properties."""
        for editor in self._property_editors.values():
            editor.updateWidget()

    def updateObject(self):
        """Update the object's properties from all child editor widgets."""
        # This might not be strictly necessary if signals are connected,
        # but can be useful for explicit save actions.
        for editor in self._property_editors.values():
            editor.updateProperty()
