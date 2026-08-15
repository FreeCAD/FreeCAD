# SPDX-License-Identifier: LGPL-2.1-or-later
# /**************************************************************************
#                                                                           *
#    Copyright (c) 2026 AstoCAD     <hello@astocad.com>                     *
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

"""Load and bind the Qt Designer task panels used by the Forms editor."""

from pathlib import Path

import FreeCAD as App
import FreeCADGui as Gui
from PySide import QtCore, QtUiTools, QtWidgets

_PAGES = {
    "Forms::Box": (
        "boxPage",
        {
            "Length": "boxLength",
            "Width": "boxWidth",
            "Height": "boxHeight",
            "XSegments": "boxXSegments",
            "YSegments": "boxYSegments",
            "ZSegments": "boxZSegments",
        },
        {"Length": "boxXLabel", "Width": "boxYLabel", "Height": "boxZLabel"},
        "boxSegmentHeader",
    ),
    "Forms::Cylinder": (
        "cylinderPage",
        {
            "Radius": "cylinderRadius",
            "Height": "cylinderHeight",
            "SideSegments": "cylinderSideSegments",
            "HeightSegments": "cylinderHeightSegments",
        },
        {"Radius": "cylinderRadiusLabel", "Height": "cylinderHeightLabel"},
        "cylinderSegmentHeader",
    ),
    "Forms::Quadball": (
        "spherePage",
        {"Radius": "sphereRadius", "Segments": "sphereSegments"},
        {"Radius": "sphereRadiusLabel", "Segments": "sphereSegmentsLabel"},
        None,
    ),
    "Forms::Face": (
        "facePage",
        {
            "Length": "faceLength",
            "Width": "faceWidth",
            "XSegments": "faceXSegments",
            "YSegments": "faceYSegments",
        },
        {"Length": "faceXLabel", "Width": "faceYLabel"},
        "faceSegmentHeader",
    ),
    "Forms::Surface": (
        "surfacePage",
        {"USegments": "surfaceUSegments", "VSegments": "surfaceVSegments"},
        {"USegments": "surfaceUSegmentsLabel", "VSegments": "surfaceVSegmentsLabel"},
        None,
    ),
    "Forms::Torus": (
        "torusPage",
        {
            "MajorRadius": "torusMajorRadius",
            "MinorRadius": "torusMinorRadius",
            "MajorSegments": "torusMajorSegments",
            "MinorSegments": "torusMinorSegments",
        },
        {"MajorRadius": "torusMajorLabel", "MinorRadius": "torusMinorLabel"},
        "torusSegmentHeader",
    ),
    "Forms::Tube": (
        "tubePage",
        {
            "OuterRadius": "tubeOuterRadius",
            "InnerRadius": "tubeInnerRadius",
            "Height": "tubeHeight",
            "SideSegments": "tubeSideSegments",
            "HeightSegments": "tubeHeightSegments",
        },
        {
            "OuterRadius": "tubeOuterRadiusLabel",
            "InnerRadius": "tubeInnerRadiusLabel",
            "Height": "tubeHeightLabel",
            "SideSegments": "tubeSideSegmentsLabel",
            "HeightSegments": "tubeHeightSegmentsLabel",
        },
        None,
    ),
}


def _dynamic_primitive_page(panel, form_type):
    page = QtWidgets.QWidget(panel)
    layout = QtWidgets.QFormLayout(page)
    widgets = {}
    labels = {}
    definitions = (
        (
            ("Radius", "Radius", "length"),
            ("LongitudeSegments", "Longitude segments", "integer"),
            ("LatitudeSegments", "Latitude segments", "integer"),
        )
        if form_type == "Forms::Sphere"
        else (
            ("Diameter", "Global diameter", "length"),
            ("SectionSegments", "Section density (8 sides per level)", "integer"),
            ("PathSamples", "Segments per edge", "integer"),
        )
    )
    for name, text, kind in definitions:
        widget = (
            QtWidgets.QDoubleSpinBox(page)
            if kind == "length"
            else QtWidgets.QSpinBox(page)
        )
        widget.setRange(0.001, 1000000.0) if kind == "length" else widget.setRange(1, 100)
        label = QtWidgets.QLabel(App.Qt.translate("Forms_Edit", text), page)
        layout.addRow(label, widget)
        widgets[name] = widget
        labels[name] = label
    table = None
    if form_type == "Forms::Pipe":
        table = QtWidgets.QTableWidget(0, 3, page)
        table.setHorizontalHeaderLabels(
            (
                App.Qt.translate("Forms_Pipe", "Path segment"),
                App.Qt.translate("Forms_Pipe", "Diameter override"),
                App.Qt.translate("Forms_Pipe", "Segments per edge"),
            )
        )
        table.horizontalHeader().setStretchLastSection(True)
        table.verticalHeader().setVisible(False)
        layout.addRow(table)
    panel.layout().insertWidget(0, page)
    return page, widgets, labels, table


def load_panel(filename):
    """Load a Forms task panel from the installed module directory."""
    path = str(Path(__file__).with_name(filename))
    if hasattr(Gui, "PySideUic"):
        return Gui.PySideUic.loadUi(path)

    # FreeCAD's command-line GUI initializes PySideUic after startup scripts.
    # QUiLoader provides an equivalent fallback for tests and early loading;
    # expose named children as attributes to match PySideUic's contract.
    ui_file = QtCore.QFile(path)
    if not ui_file.open(QtCore.QIODevice.ReadOnly):
        raise OSError(ui_file.errorString())
    try:
        panel = QtUiTools.QUiLoader().load(ui_file)
    finally:
        ui_file.close()
    if panel is None:
        raise RuntimeError(f"Could not load task panel: {filename}")
    for child in panel.findChildren(QtCore.QObject):
        name = child.objectName()
        if name:
            setattr(panel, name, child)
    return panel


def bind_edit_panel(session):
    """Load the main editor and expose its typed controls on *session*."""
    panel = load_panel("TaskFormEdit.ui")
    form_type = str(session.obj.FormType)
    if form_type in ("Forms::Sphere", "Forms::Pipe"):
        page, widgets, labels, table = _dynamic_primitive_page(panel, form_type)
        page_name = None
        widget_names = label_names = {}
        header_name = None
    else:
        page_name, widget_names, label_names, header_name = _PAGES[form_type]
    for candidate_page, *_unused in _PAGES.values():
        getattr(panel, candidate_page).setVisible(
            page_name is not None and candidate_page == page_name
        )
    session.parameter_widgets = (
        widgets
        if page_name is None
        else {
            name: getattr(panel, widget_name)
            for name, widget_name in widget_names.items()
        }
    )
    session.parameter_labels = (
        labels
        if page_name is None
        else {
            name: getattr(panel, label_name)
            for name, label_name in label_names.items()
        }
    )
    session.pipe_segment_table = table if page_name is None else None
    session.segment_header = getattr(panel, header_name) if header_name else None
    session.symmetric = panel.symmetric
    session.symmetry_plane = panel.symmetryPlane
    session.surface_tangent = panel.surfaceTangent
    session.selection_filter = panel.selectionFilter
    session.sharpness_slider = panel.sharpnessSlider
    session.sharpness_spin = panel.sharpnessSpin
    session.coordinate_space = panel.coordinateSpace
    session.set_pivot_button = panel.setPivot
    session.selection_label = panel.selectionStatus
    return panel


def bind_tool_panel(session):
    panel = load_panel("TaskFormTool.ui")
    session.tool_handler_layout = panel.toolLayout
    return panel
