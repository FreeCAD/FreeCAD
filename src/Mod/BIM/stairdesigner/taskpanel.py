# SPDX-License-Identifier: LGPL-2.1-or-later

"""Task-panel orchestration, application, and lifecycle."""

import FreeCAD
import FreeCADGui
from PySide import QtCore, QtGui

translate = FreeCAD.Qt.translate

from .taskpanel_widgets import (
    _CurrentPageTabWidget,
    _float_spin,
    _load_task_form,
    _value,
)

from .taskpanel_steps import StepPanelMixin

from .taskpanel_flights import FlightPanelMixin

from .taskpanel_stringers import StringerPanelMixin

from .taskpanel_handrails import HandrailPanelMixin

from .taskpanel_position import _AUTO_UPDATE, _PARAMETERS, PositionPanelMixin
from .taskpanel_bom import BomPanelMixin
from .object_utils import get_flights


class StairDesignerTaskPanel(
    PositionPanelMixin,
    StepPanelMixin,
    FlightPanelMixin,
    StringerPanelMixin,
    HandrailPanelMixin,
    BomPanelMixin,
):
    """Edit one or more Stair Designer parameter sections."""

    def __init__(
        self,
        stair,
        sections=None,
        edit_object=None,
        is_creating=False,
        active_section=None,
    ):
        self.stair = stair
        self.edit_object = edit_object or stair
        self.is_creating = is_creating
        requested_sections = sections or (
            "stairs",
            "steps",
            "stringers",
            "handrails",
            "bom",
        )
        self.sections = tuple(
            section
            for section in requested_sections
            if section in {"stairs", "steps", "stringers", "handrails", "bom"}
        )
        self._loading = True
        self._closed = False
        self._ui_forms = []
        self.section_widgets = {}
        self.form = _load_task_form(
            ":/ui/TaskStairDesigner.ui",
            self,
            ("section_tabs",),
        )
        self.form.setWindowIcon(QtGui.QIcon(":/icons/Arch_Stairs.svg"))
        tabs_layout = self.form.ui.verticalLayout
        old_tabs = self.section_tabs
        old_tabs.hide()
        tabs_layout.removeWidget(old_tabs)
        old_tabs.setObjectName("section_tabs_placeholder")
        self.section_tabs = _CurrentPageTabWidget(self.form.ui)
        self.section_tabs.setObjectName("section_tabs")
        tabs_layout.addWidget(self.section_tabs)
        old_tabs.deleteLater()
        regenerate_row = QtGui.QHBoxLayout()
        self.auto_update = QtGui.QCheckBox(translate("BIM", "Auto update"))
        self.auto_update.setToolTip(
            translate(
                "BIM",
                "Rebuilds the stair after every change. Unchecked, changes wait " "for Regenerate",
            )
        )
        self.auto_update.setChecked(FreeCAD.ParamGet(_PARAMETERS).GetBool(_AUTO_UPDATE, True))
        self.regenerate_button = QtGui.QPushButton(translate("BIM", "Regenerate"))
        self.regenerate_button.setToolTip(translate("BIM", "Rebuilds the stair now"))
        regenerate_row.addWidget(self.auto_update)
        regenerate_row.addStretch(1)
        regenerate_row.addWidget(self.regenerate_button)
        tabs_layout.addLayout(regenerate_row)
        self.stair.Proxy.defer_rebuilds(self.stair, not self.auto_update.isChecked())
        self._component_sections_hidden = False
        self._loading_override = False
        self._selection_observer_registered = False
        self._initialize_position_tools()
        self.form.destroyed.connect(self._form_destroyed)
        self.selected_step_component = None
        self.selected_step = None
        self.selected_stringer = None
        self.flight = self._first_flight()

        section_labels = {
            "stairs": translate("BIM", "Stair"),
            "steps": translate("BIM", "Step"),
            "stringers": translate("BIM", "Stringer"),
            "handrails": translate("BIM", "Handrail"),
            "bom": translate("BIM", "BOM"),
        }
        section_icons = {
            "stairs": QtGui.QIcon(":/icons/Arch_Stairs.svg"),
            "steps": QtGui.QIcon(":/icons/Arch_Stairs.svg"),
            "stringers": QtGui.QIcon(":/icons/Arch_Stringer.svg"),
            "handrails": QtGui.QIcon(":/icons/Arch_Handrail.svg"),
            "bom": QtGui.QIcon(":/icons/Arch_Schedule.svg"),
        }
        for section in self.sections:
            if section == "stairs":
                panel = self._make_stair_panel()
            elif section == "steps":
                panel = self._make_step_panel()
            elif section == "stringers":
                panel = self._make_stringer_panel()
            elif section == "handrails":
                panel = self._make_handrail_panel()
            elif section == "bom":
                panel = self._make_bom_panel()
            else:
                continue
            self.section_widgets[section] = panel
            panel.layout().setContentsMargins(6, 6, 6, 6)
            self.section_tabs.addTab(
                panel,
                section_icons[section],
                section_labels[section],
            )

        if active_section in self.section_widgets:
            self.section_tabs.setCurrentWidget(self.section_widgets[active_section])

        # Typed values apply once entered, not on every keystroke.
        for spin in self.form.findChildren(QtGui.QAbstractSpinBox):
            spin.setKeyboardTracking(False)
        self.auto_update.toggled.connect(self._auto_update_toggled)
        self.regenerate_button.clicked.connect(self._regenerate)
        self._update_regenerate_button()

        self._loading = False
        if {"stairs", "steps", "stringers", "handrails"}.intersection(self.sections):
            self._update_type_visibility()
        if "stairs" in self.sections:
            self._refresh_diagnostics()
        if {"steps", "stringers"}.intersection(self.sections) and FreeCAD.GuiUp:
            FreeCADGui.Selection.addObserver(self)
            self._selection_observer_registered = True
            QtCore.QTimer.singleShot(0, self._update_component_selection)
        if FreeCAD.GuiUp:
            QtCore.QTimer.singleShot(0, self._setup_position_dragger)

    def _update_component_selection(self):
        if self._closed:
            return
        self._update_step_selection()
        self._update_stringer_selection()

    def _auto_update_toggled(self, checked):
        FreeCAD.ParamGet(_PARAMETERS).SetBool(_AUTO_UPDATE, bool(checked))
        pending = self.stair.Proxy.rebuild_pending()
        self.stair.Proxy.defer_rebuilds(self.stair, not checked)
        if checked and pending:
            self.stair.Document.recompute()
            self._after_rebuild()
        self._update_regenerate_button()

    def _regenerate(self, *args):
        self.stair.Proxy.regenerate(self.stair)
        self.stair.Document.recompute()
        self._after_rebuild()

    def _update_regenerate_button(self):
        pending = self.stair.Proxy.rebuild_pending()
        font = self.regenerate_button.font()
        font.setBold(pending)
        self.regenerate_button.setFont(font)
        self.regenerate_button.setToolTip(
            translate("BIM", "The stair has changes to rebuild")
            if pending
            else translate("BIM", "Rebuilds the stair now")
        )

    def _end_deferral(self):
        """Leave manual mode as the panel closes, keeping pending changes."""

        try:
            proxy = getattr(self.stair, "Proxy", None)
        except ReferenceError:
            return
        if proxy is not None and hasattr(proxy, "defer_rebuilds"):
            proxy._deferred = False

    def _form_destroyed(self, *args):
        self._closed = True
        self._loading = True
        self._end_deferral()
        self._remove_position_dragger()
        self._remove_selection_observer()

    def addSelection(self, *args):
        self._update_component_selection()

    def removeSelection(self, *args):
        self._update_component_selection()

    def setSelection(self, *args):
        self._update_component_selection()

    def clearSelection(self, *args):
        self._update_component_selection()

    def _remove_selection_observer(self):
        if self._selection_observer_registered:
            FreeCADGui.Selection.removeObserver(self)
            self._selection_observer_registered = False

    def _apply(self, *args):
        if self._loading or not self.flight:
            return
        proxy = self.stair.Proxy
        proxy._updating = True
        try:
            if "stairs" in self.sections:
                self.stair.StairType = str(self.stair_type.itemData(self.stair_type.currentIndex()))
                self.stair.FloorHeight = self.floor_height.value()
                self.stair.EndWithRiser = self.end_with_riser.isChecked()
                self.stair.NumberOfSteps = self.step_count.value()
                self.stair.WalkingLineOffset = self.walking_line_offset.value()
                self.stair.ConcreteThickness = self.concrete_thickness.value()
                self.stair.BottomCutDistance = self.bottom_cut_distance.value()
                self.stair.TopCutDistance = self.top_cut_distance.value()
            if "steps" in self.sections:
                wood = str(self.stair.StairType) == "Wood"
                if not wood:
                    self.stair.StepsEnabled = self.steps_enabled.isChecked()
                self.stair.StepThickness = self.step_thickness.value()
                self.stair.Nosing = self.nosing.value()
                self.stair.RoundedFirstStep = str(
                    self.rounded_first_step.itemData(self.rounded_first_step.currentIndex())
                )
                self.stair.StructureWidthOffset = self.structure_width_offset.value()
                self.stair.RisersEnabled = self.risers_group.isChecked()
                self.stair.RiserThickness = self.riser_thickness.value()
                self.stair.PriorityToRiser = self.priority_to_riser.isChecked()
                self.stair.StepRiserOverlap = self.step_riser_overlap.value() if wood else 0.0
                self.stair.RiserUpperOffset = self.riser_upper_offset.value()
                self.stair.RiserLowerOffset = self.riser_lower_offset.value()
            if "stringers" in self.sections:
                for record in self.stringer_flight_editors:
                    flight = record["flight"]
                    flight_proxy = flight.Proxy
                    was_updating = getattr(flight_proxy, "_updating", False)
                    flight_proxy._updating = True
                    try:
                        for side in ("Left", "Right"):
                            editor = record[f"{side.lower()}_type"]
                            setattr(
                                flight,
                                f"{side}StringerType",
                                str(editor.itemData(editor.currentIndex())),
                            )
                    finally:
                        flight_proxy._updating = was_updating
                self.stair.StringerThickness = self.stringer_thickness.value()
                self.stair.StringerCustomWidth = self.stringer_custom_width.isChecked()
                if self.stair.StringerCustomWidth:
                    self.stair.StringerWidth = self.stringer_width.value()
                self.stair.StringerStepOverlap = self.stringer_step_overlap.value()
                self.stair.StringerStartExtension = self.stringer_start_extension.value()
                self.stair.StringerEndExtension = self.stringer_end_extension.value()
                self.stair.StringerNosingOffsetDirection = str(
                    self.stringer_nosing_direction.itemData(
                        self.stringer_nosing_direction.currentIndex()
                    )
                )
                self.stair.StringerNosingOffset = self.stringer_nosing_offset.value()
                self.stair.StringerUndersideOffsetDirection = str(
                    self.stringer_underside_direction.itemData(
                        self.stringer_underside_direction.currentIndex()
                    )
                )
                self.stair.StringerUndersideOffset = self.stringer_underside_offset.value()
                self.stair.StringerJoinery = self.stringer_joinery.isChecked()
                self.stair.StringerJointType = str(
                    self.stringer_joint_type.itemData(self.stringer_joint_type.currentIndex())
                )
                self.stair.StringerOpenHousings = self.stringer_open_housings.isChecked()
                self.stair.StringerWedgedHousings = self.stringer_wedged_housings.isChecked()
            if "handrails" in self.sections:
                for record in self.handrail_flight_editors:
                    flight = record["flight"]
                    flight_proxy = flight.Proxy
                    was_updating = getattr(flight_proxy, "_updating", False)
                    flight_proxy._updating = True
                    try:
                        for side in ("Left", "Right"):
                            setattr(
                                flight,
                                f"{side}HandrailEnabled",
                                record[f"{side.lower()}_enabled"].isChecked(),
                            )
                    finally:
                        flight_proxy._updating = was_updating
                self.stair.HandrailHeightAboveNosing = self.handrail_height.value()
                self.stair.HandrailOffset = self.handrail_offset.value()
                self.stair.HandrailPicketMaximumSpacing = self.handrail_picket_spacing.value()
                self.stair.HandrailPicketShape = str(
                    self.handrail_picket_shape.itemData(self.handrail_picket_shape.currentIndex())
                )
                self.stair.HandrailPicketWidth = self.handrail_picket_width.value()
                self.stair.HandrailPicketThickness = self.handrail_picket_thickness.value()
                self.stair.HandrailPicketStringerPenetration = (
                    self.handrail_picket_stringer_penetration.value()
                )
                self.stair.HandrailPicketTopRailPenetration = (
                    self.handrail_picket_top_rail_penetration.value()
                )
                self.stair.HandrailPostShape = str(
                    self.handrail_post_shape.itemData(self.handrail_post_shape.currentIndex())
                )
                self.stair.HandrailPostCap = str(
                    self.handrail_post_cap.itemData(self.handrail_post_cap.currentIndex())
                )
                self.stair.HandrailPostWidth = self.handrail_post_width.value()
                self.stair.HandrailPostThickness = self.handrail_post_thickness.value()
                self.stair.HandrailPostAboveTopRail = self.handrail_post_above.value()
                self.stair.HandrailPostBelowStringer = self.handrail_post_below.value()
                self.stair.HandrailTopRailShape = str(
                    self.handrail_top_rail_shape.itemData(
                        self.handrail_top_rail_shape.currentIndex()
                    )
                )
                self.stair.HandrailTopRailWidth = self.handrail_top_rail_width.value()
                self.stair.HandrailTopRailThickness = self.handrail_top_rail_thickness.value()
                self.stair.HandrailTopRailPostPenetration = (
                    self.handrail_top_rail_penetration.value()
                )
            for record in getattr(self, "flight_editors", ()):
                flight = record["flight"]
                flight_proxy = flight.Proxy
                was_updating = getattr(flight_proxy, "_updating", False)
                flight_proxy._updating = True
                try:
                    flight_type = str(
                        record["flight_type"].itemData(record["flight_type"].currentIndex())
                    )
                    flight.FlightType = flight_type
                    flight_proxy._update_dimension_visibility(flight)
                    flight.LeftLength = record["left_length"].value()
                    flight.RightLength = record["right_length"].value()
                    flight.Width = record["width"].value()
                    flight.InnerRadius = record["inner_radius"].value()
                    flight.OuterRadius = record["outer_radius"].value()
                    flight.Angle = record["angle"].value()
                    if record.get("turn_type") is not None:
                        flight.TurnType = str(
                            record["turn_type"].itemData(record["turn_type"].currentIndex())
                        )
                    if record.get("winding_local") is not None:
                        flight.WindingLocal = record["winding_local"].value()
                        flight.WindingDistant = record["winding_distant"].value()
                    if record.get("rotation") is not None:
                        flight.Rotation = str(
                            record["rotation"].itemData(record["rotation"].currentIndex())
                        )
                    if record.get("start_angle") is not None:
                        flight.StartAngle = record["start_angle"].value()
                    if record.get("end_angle") is not None:
                        flight.EndAngle = record["end_angle"].value()
                    if flight_type.startswith("Circular"):
                        flight.EntryDirection = "Straight"
                        flight.ExitDirection = "Straight"
                    elif record.get("entry_direction") is not None:
                        flight.EntryDirection = str(
                            record["entry_direction"].itemData(
                                record["entry_direction"].currentIndex()
                            )
                        )
                    if (
                        not flight_type.startswith("Circular")
                        and record.get("exit_direction") is not None
                    ):
                        flight.ExitDirection = str(
                            record["exit_direction"].itemData(
                                record["exit_direction"].currentIndex()
                            )
                        )
                    if (
                        record.get("exit_width") is not None
                        and "ExitWidth" in flight.PropertiesList
                    ):
                        # 0 follows the flight width.
                        flight.ExitWidth = (
                            0.0 if record.get("exit_width_auto") else record["exit_width"].value()
                        )
                    flight_proxy._update_dimension_visibility(flight)
                finally:
                    flight_proxy._updating = was_updating
        finally:
            proxy._updating = False
        proxy.rebuild(self.stair, allow_structure_changes=True)
        self.stair.Document.recompute()
        self._after_rebuild()

    def _after_rebuild(self):
        """Refresh what the panel shows of the rebuilt stair."""

        self._update_regenerate_button()
        if "steps" in self.sections:
            self._update_overlap_label()
        if "stringers" in self.sections:
            blocked = self.stringer_width.blockSignals(True)
            self.stringer_width.setValue(_value(self.stair.StringerWidth))
            self.stringer_width.blockSignals(blocked)
            self._update_stringer_selection()
        self._update_type_visibility()
        if "stairs" in self.sections:
            self._refresh_diagnostics()

    def _update_type_visibility(self):
        if hasattr(self, "walking_line_offset"):
            self.walking_line_offset.setEnabled(
                all(str(flight.FlightType) == "Straight" for flight in get_flights(self.stair))
            )
        if hasattr(self, "stair_type"):
            stair_type = str(self.stair_type.itemData(self.stair_type.currentIndex()))
        else:
            stair_type = str(self.stair.StairType)
        wood = stair_type == "Wood"
        if hasattr(self, "concrete_thickness"):
            self.concrete_thickness_label.setVisible(not wood)
            self.concrete_thickness.setVisible(not wood)
            self.cut_distance_label.setVisible(not wood)
            self.bottom_cut_distance_label.setVisible(not wood)
            self.bottom_cut_distance.setVisible(not wood)
            self.top_cut_distance_label.setVisible(not wood)
            self.top_cut_distance.setVisible(not wood)
        steps_panel = self.section_widgets.get("steps")
        if steps_panel:
            self._set_section_visible(steps_panel, True)
        if hasattr(self, "steps_enabled"):
            self.steps_enabled.setVisible(not wood)
            steps_active = wood or self.steps_enabled.isChecked()
            self.step_thickness.setEnabled(steps_active)
            self.nosing.setEnabled(steps_active)
            self.structure_width_offset_label.setVisible(not wood)
            self.structure_width_offset.setVisible(not wood)
            self.structure_width_offset_label.setEnabled(steps_active)
            self.structure_width_offset.setEnabled(steps_active)
            self.risers_group.setEnabled(steps_active)
            self.step_riser_overlap_label.setVisible(wood)
            self.step_riser_overlap.setVisible(wood)
        if hasattr(self, "handrail_picket_stringer_penetration"):
            self.handrail_picket_stringer_penetration.setEnabled(wood)
            self.handrail_post_below.setEnabled(wood)
        if not wood:
            for section in ("stringers",):
                panel = self.section_widgets.get(section)
                if panel:
                    self._set_section_visible(panel, False)
            self._component_sections_hidden = True
        elif self._component_sections_hidden:
            for section in ("stringers",):
                panel = self.section_widgets.get(section)
                if panel:
                    self._set_section_visible(panel, True)
            self._component_sections_hidden = False

    def _set_section_visible(self, panel, visible):
        index = self.section_tabs.indexOf(panel)
        if index >= 0:
            self.section_tabs.setTabVisible(index, visible)
        else:
            panel.setVisible(visible)

    def _first_flight(self):
        group = self.stair.FlightsGroup
        if not group:
            return None
        return next(
            (flight for flight in group.Group if getattr(flight.Proxy, "Type", "") == "Flight"),
            None,
        )

    @staticmethod
    def _angle_spin(value):
        spin = _float_spin(value, -360.0, 360.0, 2)
        spin.setSuffix(" deg")
        return spin

    @staticmethod
    def _select_data(combo, value):
        for index in range(combo.count()):
            if str(combo.itemData(index)) == value:
                combo.setCurrentIndex(index)
                return

    def accept(self):
        self._closed = True
        self._loading = True
        self._remove_position_dragger()
        self._remove_selection_observer()
        # Changes left waiting for Regenerate are kept.
        if self.stair.Proxy.rebuild_pending():
            self.stair.Proxy.regenerate(self.stair)
        self._end_deferral()
        doc = self.stair.Document
        doc.recompute()
        gui_doc = FreeCADGui.ActiveDocument
        if self.is_creating:
            gui_doc.commitCommand()
        elif doc.getBookedTransactionID():
            gui_doc.commitCommand()
        if not self.is_creating:
            gui_doc.resetEdit()
        return True

    def reject(self):
        self._closed = True
        self._loading = True
        self._remove_position_dragger()
        self._remove_selection_observer()
        self._end_deferral()
        doc = self.stair.Document
        gui_doc = FreeCADGui.ActiveDocument
        if self.is_creating:
            gui_doc.abortCommand()
        elif doc.getBookedTransactionID():
            gui_doc.abortCommand()
        doc.recompute()
        if not self.is_creating:
            gui_doc.resetEdit()
        return True

    def open(self):
        gui_doc = FreeCADGui.ActiveDocument
        if not self.stair.Document.getBookedTransactionID():
            gui_doc.openCommand(translate("BIM", "Edit Stair"))

    def getStandardButtons(self):
        return QtGui.QDialogButtonBox.Ok | QtGui.QDialogButtonBox.Cancel

    def isAllowedAlterSelection(self):
        return True

    def isAllowedAlterView(self):
        return True
