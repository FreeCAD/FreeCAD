# SPDX-License-Identifier: LGPL-2.1-or-later

"""Multi-flight tree construction and editing."""

import math

import FreeCAD
from PySide import QtCore, QtGui

from .object_factory import resize_flights
from .object_utils import (
    get_flights,
    linked_circular_radii,
    linked_flight_side_lengths_for_difference,
    straight_turn_side_difference,
    sync_all_flight_side_lengths,
)

translate = FreeCAD.Qt.translate

from .taskpanel_widgets import (
    _FlightTreeWidget,
    _length_spin,
    _percent_spin,
    _value,
)


class FlightPanelMixin:
    """Task-panel methods grouped by responsibility."""

    def _make_multiflight_panel(self):
        self.flight_tree = _FlightTreeWidget(self._remove_flight)
        self.flight_tree.setColumnCount(2)
        self.flight_tree.setHeaderHidden(True)
        self.flight_tree.setRootIsDecorated(True)
        self.flight_tree.setMinimumHeight(245)
        self.flight_tree.header().setSectionResizeMode(0, QtGui.QHeaderView.ResizeToContents)
        self.flight_tree.header().setSectionResizeMode(1, QtGui.QHeaderView.Stretch)
        self.flight_tree.setContextMenuPolicy(QtCore.Qt.CustomContextMenu)
        self.flight_tree.customContextMenuRequested.connect(self._show_flight_context_menu)

        self._populate_flight_tree()
        return self.flight_tree

    def _populate_flight_tree(self):
        self.flight_tree.clear()
        self.flight_editors = []
        flights = get_flights(self.stair)
        single_flight = len(flights) == 1
        all_straight = all(str(flight.FlightType) == "Straight" for flight in flights)
        for index, flight in enumerate(flights):
            root = QtGui.QTreeWidgetItem(self.flight_tree)
            root.setText(0, flight.Label)
            root.setFirstColumnSpanned(True)
            record = {"flight": flight, "item": root}

            flight_type = QtGui.QComboBox()
            flight_type.addItem(translate("BIM", "Straight flight"), "Straight")
            flight_type.addItem(translate("BIM", "Circular flight"), "Circular")
            flight_type.addItem(translate("BIM", "Straight landing"), "Straight landing")
            flight_type.addItem(translate("BIM", "Circular landing"), "Circular landing")
            self._select_data(flight_type, str(flight.FlightType))
            record["flight_type"] = flight_type
            record["left_length"] = _length_spin(_value(flight.LeftLength), 1.0)
            record["right_length"] = _length_spin(_value(flight.RightLength), 1.0)
            record["inner_radius"] = _length_spin(_value(flight.InnerRadius), 1.0)
            record["outer_radius"] = _length_spin(_value(flight.OuterRadius), 1.0)
            record["width"] = _length_spin(_value(flight.Width), 1.0)
            record["angle"] = self._angle_spin(_value(flight.Angle))
            rotation = QtGui.QComboBox()
            rotation.addItem(translate("BIM", "Left"), "Left")
            rotation.addItem(translate("BIM", "Right"), "Right")
            self._select_data(rotation, str(flight.Rotation))
            record["rotation"] = rotation
            is_circular = str(flight.FlightType).startswith("Circular")
            is_landing = str(flight.FlightType).endswith("landing")
            previous_is_circular = index > 0 and str(flights[index - 1].FlightType).startswith(
                "Circular"
            )
            previous_is_landing = index > 0 and str(flights[index - 1].FlightType).endswith(
                "landing"
            )
            parameters = [(translate("BIM", "Type"), flight_type)]
            if index:
                parameters.append((translate("BIM", "Rotation"), rotation))
            winding_rows = []
            if is_circular:
                record["angle"].setRange(0.01, 359.99)
                parameters.extend(
                    (
                        (translate("BIM", "Inner radius"), record["inner_radius"]),
                        (translate("BIM", "Outer radius"), record["outer_radius"]),
                        (translate("BIM", "Width"), record["width"]),
                        (translate("BIM", "Angle"), record["angle"]),
                    )
                )
            else:
                if single_flight:
                    parameters.extend(
                        (
                            (translate("BIM", "Length"), record["left_length"]),
                            (translate("BIM", "Width"), record["width"]),
                        )
                    )
                else:
                    parameters.extend(
                        (
                            (
                                translate("BIM", "Left length"),
                                record["left_length"],
                            ),
                            (
                                translate("BIM", "Right length"),
                                record["right_length"],
                            ),
                            (translate("BIM", "Width"), record["width"]),
                        )
                    )
            if index:
                turn_type = QtGui.QComboBox()
                turn_type.addItem(translate("BIM", "Herse balancing"), "Herse balancing")
                turn_type.addItem(translate("BIM", "Landing"), "Landing")
                self._select_data(turn_type, str(flight.TurnType))
                record["turn_type"] = turn_type
                if not is_circular:
                    parameters.append((translate("BIM", "Angle"), record["angle"]))
                    supports_winding = not (
                        previous_is_circular or is_landing or previous_is_landing
                    )
                    if supports_winding:
                        local_winding = _percent_spin(flight.WindingLocal)
                        distant_winding = _percent_spin(flight.WindingDistant)
                        local_winding.setToolTip(
                            translate(
                                "BIM",
                                "Adjusts winders nearest the inner corner",
                            )
                        )
                        distant_winding.setToolTip(
                            translate(
                                "BIM",
                                "Adjusts how far winding extends from the inner corner",
                            )
                        )
                        record["winding_local"] = local_winding
                        record["winding_distant"] = distant_winding
                        parameters.append((translate("BIM", "Turn type"), turn_type))
                        winding_rows.append((translate("BIM", "Local winding"), local_winding))
                        winding_rows.append(
                            (
                                translate("BIM", "Distant winding"),
                                distant_winding,
                            )
                        )
            if is_landing and not is_circular:
                entry_direction = QtGui.QComboBox()
                entry_direction.addItem(translate("BIM", "Straight"), "Straight")
                entry_direction.addItem(translate("BIM", "From left"), "From left")
                entry_direction.addItem(translate("BIM", "From right"), "From right")
                self._select_data(entry_direction, str(flight.EntryDirection))
                record["entry_direction"] = entry_direction
                exit_direction = QtGui.QComboBox()
                exit_direction.addItem(translate("BIM", "Straight"), "Straight")
                exit_direction.addItem(translate("BIM", "To left"), "To left")
                exit_direction.addItem(translate("BIM", "To right"), "To right")
                self._select_data(exit_direction, str(flight.ExitDirection))
                record["exit_direction"] = exit_direction
                parameters.append((translate("BIM", "Entry direction"), entry_direction))
                parameters.append((translate("BIM", "Exit direction"), exit_direction))
            if index == 0 and all_straight:
                record["start_angle"] = self._angle_spin(_value(flight.StartAngle))
                record["start_angle"].setRange(-89.0, 89.0)
                entry_direction = QtGui.QComboBox()
                entry_direction.addItem(translate("BIM", "Straight"), "Straight")
                entry_direction.addItem(translate("BIM", "From left"), "From left")
                entry_direction.addItem(translate("BIM", "From right"), "From right")
                self._select_data(entry_direction, str(flight.EntryDirection))
                record["entry_direction"] = entry_direction
                parameters.append((translate("BIM", "Start angle"), record["start_angle"]))
                parameters.append((translate("BIM", "Entry direction"), entry_direction))
            if index == len(flights) - 1 and all_straight:
                record["end_angle"] = self._angle_spin(_value(flight.EndAngle))
                record["end_angle"].setRange(-89.0, 89.0)
                exit_direction = QtGui.QComboBox()
                exit_direction.addItem(translate("BIM", "Straight"), "Straight")
                exit_direction.addItem(translate("BIM", "To left"), "To left")
                exit_direction.addItem(translate("BIM", "To right"), "To right")
                self._select_data(exit_direction, str(flight.ExitDirection))
                record["exit_direction"] = exit_direction
                parameters.append((translate("BIM", "End angle"), record["end_angle"]))
                parameters.append((translate("BIM", "Exit direction"), exit_direction))
            turn_type_item = None
            for label, editor in parameters:
                child = QtGui.QTreeWidgetItem(root)
                child.setText(0, label)
                self.flight_tree.setItemWidget(child, 1, editor)
                if editor is record.get("turn_type"):
                    turn_type_item = child
            if turn_type_item is not None:
                record["winding_items"] = []
                hide_winding = str(flight.TurnType) == "Landing"
                for label, editor in winding_rows:
                    child = QtGui.QTreeWidgetItem(turn_type_item)
                    child.setText(0, label)
                    child.setHidden(hide_winding)
                    self.flight_tree.setItemWidget(child, 1, editor)
                    record["winding_items"].append(child)
                turn_type_item.setExpanded(True)
            root.setExpanded(True)
            self.flight_editors.append(record)
            for key in ("left_length", "right_length"):
                editor = record.get(key)
                if editor:
                    editor.valueChanged.connect(
                        lambda _value, current=record, side=key: (
                            self._flight_length_changed(current, side)
                        )
                    )
            for key in ("inner_radius", "outer_radius"):
                editor = record[key]
                editor.valueChanged.connect(
                    lambda _value, current=record, radius=key: (
                        self._flight_radius_changed(current, radius)
                    )
                )
            for key in ("width", "angle"):
                editor = record.get(key)
                if editor:
                    editor.valueChanged.connect(
                        lambda _value, current=record: self._turn_geometry_changed(current)
                    )
            for key in ("start_angle", "end_angle"):
                editor = record.get(key)
                if editor:
                    editor.valueChanged.connect(
                        lambda _value, current=record: (self._endpoint_angle_changed(current))
                    )
            for key in ("entry_direction", "exit_direction"):
                editor = record.get(key)
                if editor:
                    editor.currentIndexChanged.connect(self._apply)
            flight_type.currentIndexChanged.connect(
                lambda _index, current=record: self._flight_type_changed(current)
            )
            record["rotation"].currentIndexChanged.connect(
                lambda _index, current=record: self._turn_geometry_changed(current)
            )
            if record.get("turn_type") is not None:
                record["turn_type"].currentIndexChanged.connect(
                    lambda _index, current=record: self._turn_type_changed(current)
                )
            for key in ("winding_local", "winding_distant"):
                editor = record.get(key)
                if editor:
                    editor.valueChanged.connect(self._apply)

    def _flight_length_changed(self, record, side):
        if self._loading:
            return
        self._sync_flight_length_editors(
            self.flight_editors.index(record),
            "LeftLength" if side == "left_length" else "RightLength",
        )
        self._apply()

    def _flight_radius_changed(self, record, radius):
        if self._loading:
            return
        self._sync_flight_radius_editors(
            self.flight_editors.index(record),
            "InnerRadius" if radius == "inner_radius" else "OuterRadius",
        )
        self._apply()

    def _turn_geometry_changed(self, record):
        if self._loading:
            return
        index = self.flight_editors.index(record)
        if self._editor_flight_type(record).startswith("Circular"):
            self._sync_flight_radius_editors(index)
        else:
            self._sync_flight_length_editors(index)
        if index:
            self._sync_flight_length_editors(index - 1)
        if index + 1 < len(self.flight_editors):
            self._sync_flight_length_editors(index + 1)
        self._apply()

    def _turn_type_changed(self, record):
        if self._loading:
            return
        editor = record["turn_type"]
        is_landing = str(editor.itemData(editor.currentIndex())) == "Landing"
        for item in record.get("winding_items", ()):
            item.setHidden(is_landing)
        self._apply()

    def _endpoint_angle_changed(self, record):
        if self._loading:
            return
        self._sync_flight_length_editors(self.flight_editors.index(record), "LeftLength")
        self._apply()

    def _flight_type_changed(self, record):
        if self._loading:
            return
        index = self.flight_editors.index(record)
        flight_type = self._editor_flight_type(record)
        if flight_type.startswith("Circular"):
            record["angle"].setRange(0.01, 359.99)
            blocked = record["angle"].blockSignals(True)
            record["angle"].setValue(90.0)
            record["angle"].blockSignals(blocked)
            self._sync_flight_radius_editors(index)
        else:
            record["angle"].setRange(-360.0, 360.0)
            if flight_type.endswith("landing"):
                blocked = record["angle"].blockSignals(True)
                record["angle"].setValue(90.0)
                record["angle"].blockSignals(blocked)
            self._sync_flight_length_editors(index)
        if index:
            self._sync_flight_length_editors(index - 1)
        if index + 1 < len(self.flight_editors):
            self._sync_flight_length_editors(index + 1)
        self._apply()
        self._loading = True
        try:
            self._populate_flight_tree()
            if hasattr(self, "stringer_tree"):
                self._populate_stringer_tree()
            if hasattr(self, "handrail_tree"):
                self._populate_handrail_tree()
        finally:
            self._loading = False

    @staticmethod
    def _editor_flight_type(record):
        editor = record["flight_type"]
        return str(editor.itemData(editor.currentIndex()))

    def _sync_flight_length_editors(self, index, driver=None):
        record = self.flight_editors[index]
        if self._editor_flight_type(record).startswith("Circular"):
            self._sync_flight_radius_editors(index)
            return
        next_record = (
            self.flight_editors[index + 1] if index + 1 < len(self.flight_editors) else None
        )
        previous_record = self.flight_editors[index - 1] if index > 0 else None
        incoming_straight_turn = (
            previous_record
            and not self._editor_flight_type(previous_record).startswith("Circular")
            and abs(record["angle"].value()) > 1e-7
        )
        straight_turn = (
            next_record
            and not self._editor_flight_type(next_record).startswith("Circular")
            and abs(next_record["angle"].value()) > 1e-7
        )
        signed_length_difference = 0.0
        if incoming_straight_turn:
            turn_difference = straight_turn_side_difference(
                record["width"].value(),
                previous_record["width"].value(),
                record["angle"].value(),
            )
            rotation = record["rotation"]
            if str(rotation.itemData(rotation.currentIndex())) == "Right":
                turn_difference = -turn_difference
            signed_length_difference += turn_difference
        if straight_turn:
            turn_difference = straight_turn_side_difference(
                record["width"].value(),
                next_record["width"].value(),
                next_record["angle"].value(),
            )
            rotation = next_record["rotation"]
            if str(rotation.itemData(rotation.currentIndex())) == "Right":
                turn_difference = -turn_difference
            signed_length_difference += turn_difference
        all_straight = all(
            self._editor_flight_type(item) == "Straight" for item in self.flight_editors
        )
        if all_straight and index == 0 and record.get("start_angle") is not None:
            signed_length_difference += record["width"].value() * math.tan(
                math.radians(record["start_angle"].value())
            )
        if (
            all_straight
            and index == len(self.flight_editors) - 1
            and record.get("end_angle") is not None
        ):
            signed_length_difference -= record["width"].value() * math.tan(
                math.radians(record["end_angle"].value())
            )
        left, right = linked_flight_side_lengths_for_difference(
            record["left_length"].value(),
            record["right_length"].value(),
            signed_length_difference,
            driver,
        )
        for editor, value in (
            (record["left_length"], left),
            (record["right_length"], right),
        ):
            blocked = editor.blockSignals(True)
            editor.setValue(value)
            editor.blockSignals(blocked)

    def _sync_flight_radius_editors(self, index, driver=None):
        record = self.flight_editors[index]
        inner, outer = linked_circular_radii(
            record["inner_radius"].value(),
            record["outer_radius"].value(),
            record["width"].value(),
            driver,
        )
        for editor, value in (
            (record["inner_radius"], inner),
            (record["outer_radius"], outer),
        ):
            blocked = editor.blockSignals(True)
            editor.setValue(value)
            editor.blockSignals(blocked)

    def _add_flight(self):
        if self._loading:
            return
        self._loading = True
        try:
            flights = get_flights(self.stair)
            resize_flights(self.stair, len(flights) + 1)
            self.flight = self._first_flight()
            self._populate_flight_tree()
            if hasattr(self, "stringer_tree"):
                self._populate_stringer_tree()
            if hasattr(self, "handrail_tree"):
                self._populate_handrail_tree()
        finally:
            self._loading = False
        self.stair.Proxy.rebuild(self.stair, allow_structure_changes=True)
        self.stair.Document.recompute()
        self._refresh_diagnostics()

    def _show_flight_context_menu(self, position):
        clicked = self.flight_tree.itemAt(position)
        if clicked is not None:
            self.flight_tree.setCurrentItem(clicked)
        selected = self.flight_tree.currentItem()
        menu = QtGui.QMenu(self.flight_tree)
        add_action = menu.addAction(
            QtGui.QIcon(":/icons/Arch_Add.svg"),
            translate("BIM", "Add Flight"),
        )
        add_action.triggered.connect(self._add_flight)
        if selected is not None:
            menu.addSeparator()
            delete_action = menu.addAction(
                QtGui.QIcon(":/icons/Arch_Remove.svg"),
                translate("BIM", "Delete Flight"),
            )
            delete_action.setEnabled(len(get_flights(self.stair)) > 1)
            delete_action.triggered.connect(self._remove_flight)
        menu.exec_(self.flight_tree.viewport().mapToGlobal(position))

    def _remove_flight(self):
        flights = get_flights(self.stair)
        if self._loading or len(flights) <= 1:
            return
        selected = self.flight_tree.currentItem()
        selected_flight = None
        while selected and selected.parent():
            selected = selected.parent()
        for record in self.flight_editors:
            if record["item"] is selected:
                selected_flight = record["flight"]
                break
        if selected_flight is None:
            return
        self._loading = True
        proxy = self.stair.Proxy
        proxy._updating = True
        try:
            self.stair.FlightsGroup.removeObject(selected_flight)
            self.stair.Document.removeObject(selected_flight.Name)
            remaining = get_flights(self.stair)
            if str(remaining[0].FlightType) == "Straight":
                remaining[0].Angle = 0.0
            for index, flight in enumerate(remaining):
                flight.Label = f"{translate('BIM', 'Flight')} {index + 1}"
            sync_all_flight_side_lengths(self.stair)
            self.flight = self._first_flight()
            self._populate_flight_tree()
            if hasattr(self, "stringer_tree"):
                self._populate_stringer_tree()
            if hasattr(self, "handrail_tree"):
                self._populate_handrail_tree()
        finally:
            proxy._updating = False
            self._loading = False
        self.stair.Proxy.rebuild(self.stair, allow_structure_changes=True)
        self.stair.Document.recompute()
        self._refresh_diagnostics()
