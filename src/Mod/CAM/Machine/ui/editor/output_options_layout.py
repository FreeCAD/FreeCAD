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

"""Shared layout builder for machine output and processing options.

Provides a single function that builds all output-option and processing-option
widgets from a Machine object.  Used by both the Machine Editor dialog and
the Post-Processing dialog so that both UIs stay in sync automatically.
"""

from PySide import QtGui
import FreeCAD

translate = FreeCAD.Qt.translate


def build_output_options(machine, layout, context="CAM_MachineEditor"):
    """Build output-option and processing-option widgets into *layout*.

    Parameters
    ----------
    machine : Machine
        A Machine model instance whose ``output`` and ``processing``
        attributes are read for initial values.
    layout : QBoxLayout
        The layout (typically a QVBoxLayout inside a scroll area) to which
        group boxes are added.
    context : str
        Translation context – callers may pass ``"CAM_Post"`` when
        the widgets appear inside the post-processing dialog.

    Returns
    -------
    dict
        A nested dict ``{section_name: {field_name: widget}}`` where every
        *widget* carries a ``value_getter`` callable that returns its
        current value.  Section names are ``"main"``, ``"header"``,
        ``"comments"``, ``"formatting"``, ``"precision"``, ``"duplicates"``,
        and ``"processing"``.
    """
    from Machine.ui.editor.machine_editor import DataclassGUIGenerator

    all_widgets = {}

    # ------------------------------------------------------------------
    # Main output options (units + top-level booleans)
    # ------------------------------------------------------------------
    main_group = QtGui.QGroupBox(translate(context, "Main Options"))
    main_layout = QtGui.QFormLayout(main_group)

    units_combo = QtGui.QComboBox()
    units_combo.addItem(translate(context, "Metric"), "metric")
    units_combo.addItem(translate(context, "Imperial"), "imperial")
    try:
        units_val = machine.output.units.value
        idx = units_combo.findData(units_val)
        if idx >= 0:
            units_combo.setCurrentIndex(idx)
    except Exception:
        # units_val might not be a valid enum value, just skip
        pass
    main_layout.addRow(translate(context, "Units"), units_combo)
    units_combo.value_getter = lambda: units_combo.itemData(units_combo.currentIndex())

    main_widgets = {"units": units_combo}

    for field_name, label in [
        ("output_header", translate(context, "Output Header")),
        ("output_tool_length_offset", translate(context, "Output Tool Length Offset (G43)")),
        ("remote_post", translate(context, "Enable Remote Posting")),
    ]:
        cb = QtGui.QCheckBox()
        cb.setChecked(bool(getattr(getattr(machine, "output", None), field_name, False)))
        cb.value_getter = lambda w=cb: w.isChecked()
        main_layout.addRow(label, cb)
        main_widgets[field_name] = cb

    layout.addWidget(main_group)
    all_widgets["main"] = main_widgets

    # ------------------------------------------------------------------
    # Sub-dataclass groups from machine.output
    # ------------------------------------------------------------------
    output = getattr(machine, "output", None)
    if output is not None:
        sub_sections = [
            ("header", translate(context, "Header Options")),
            ("comments", translate(context, "Comment Options")),
            ("formatting", translate(context, "Formatting Options")),
            ("precision", translate(context, "Precision Options")),
            ("duplicates", translate(context, "Duplicate Output Options")),
        ]
        for attr_name, title in sub_sections:
            dc_instance = getattr(output, attr_name, None)
            if dc_instance is None:
                continue
            try:
                group, widgets = DataclassGUIGenerator.create_group_for_dataclass(
                    dc_instance, title
                )
            except Exception:
                # dc_instance might not be a valid dataclass, just skip
                continue
            layout.addWidget(group)
            all_widgets[attr_name] = widgets

    # ------------------------------------------------------------------
    # Processing options from machine.processing
    # ------------------------------------------------------------------
    processing = getattr(machine, "processing", None)
    if processing is not None:
        try:
            processing_group, processing_widgets = DataclassGUIGenerator.create_group_for_dataclass(
                processing, translate(context, "Processing Options")
            )
            layout.addWidget(processing_group)
            all_widgets["processing"] = processing_widgets
        except Exception:
            # processing might not be a valid dataclass, just skip
            pass

    return all_widgets
