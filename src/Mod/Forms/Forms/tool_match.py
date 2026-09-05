# SPDX-License-Identifier: LGPL-2.1-or-later
"""Match tool controls, preview, and commit/cancel lifecycle."""

import FreeCAD as App
import FreeCADGui as Gui
import Part
from .feedback import MODELING_ERRORS, report_modeling_error
from .taskpanels import load_panel
from .tool_controller import ToolController


class MatchTool(ToolController):
    def __init__(self, session):
        super().__init__(session)
        self.match_inputs = None
        self.match_mode = None
        self.match_apply_button = None
        self.match_cancel_button = None
        self.match_preview_status = None
        self.match_preview_root = None
        self.match_preview_switch = None
        self.match_preview_shape = Part.Shape()
        self.match_visibility_before = []

    def start_match_tool(self, obj, edges, support):
        """Open the Match task handler after its opening and support are selected."""
        if self.session.cleaned or self.session.has_active_tool() or obj != self.session.obj:
            return False
        self.session._flush_pending_updates()
        self.match_inputs = (obj, set(edges), support)
        self.session.active_tool = "match"

        widget = load_panel("TaskFormMatch.ui")
        self.match_mode = widget.continuityMode
        for index, value in enumerate(("AdjacentFaces", "SelectedFace", "Connected")):
            self.match_mode.setItemData(index, value)
        current = (
            "Connected"
            if str(self.session.obj.MatchContinuity) == "Connected"
            else str(getattr(self.session.obj, "MatchTangentMode", "AdjacentFaces"))
        )
        self.match_mode.setCurrentIndex(max(self.match_mode.findData(current), 0))
        self.match_mode.currentIndexChanged.connect(self._update_match_preview)

        self.match_preview_status = widget.previewStatus
        self.match_apply_button = widget.applyButton
        self.match_cancel_button = widget.cancelButton
        self.match_apply_button.clicked.connect(self.apply_match_tool)
        self.match_cancel_button.clicked.connect(self.stop_match_tool)
        self.session._clear_editor_selection()
        self.session._show_tool_handler(
            App.Qt.translate("Forms_Edit", "Match"),
            widget,
            "Forms_Match",
        )
        self.session._suspend_selection_for_tool()
        self._start_match_preview_visibility()
        self._update_match_preview()
        Gui.HintManager.show(
            Gui.InputHint(
                App.Qt.translate("Forms_Edit", "%1 cancel match"),
                Gui.UserInput.KeyEscape,
            )
        )
        Gui.Command.update()
        return True

    def _start_match_preview_visibility(self):
        self.match_visibility_before = []
        base = getattr(self.session.obj, "BaseFeature", None)
        features = ([base] if base is not None else []) + [self.session.obj]
        for feature in features:
            view_object = getattr(feature, "ViewObject", None)
            if view_object is not None:
                self.match_visibility_before.append((feature, bool(view_object.Visibility)))
        self.session.obj.ViewObject.Visibility = False
        if base is not None and getattr(base, "ViewObject", None) is not None:
            base.ViewObject.Visibility = True

    def _restore_match_preview_visibility(self):
        for feature, visible in self.match_visibility_before:
            if feature.Document is not None:
                feature.ViewObject.Visibility = visible
        self.match_visibility_before = []

    def _set_match_preview_shape(self, shape):
        self._clear_match_preview()
        self.match_preview_root, self.match_preview_switch = self.session._make_shape_preview(shape)
        self.match_preview_shape = shape

    def _clear_match_preview(self):
        self.session._remove_shape_preview(self.match_preview_root)
        self.match_preview_root = None
        self.match_preview_switch = None
        self.match_preview_shape = Part.Shape()

    def _update_match_preview(self, _index=None):
        if not self.session.match_tool_active or self.match_inputs is None:
            return
        from .matching import preview_match_shape

        obj, edges, support = self.match_inputs
        mode = str(self.match_mode.currentData())
        continuity = "Connected" if mode == "Connected" else "Tangent"
        tangent_mode = (
            str(getattr(obj, "MatchTangentMode", "AdjacentFaces")) if mode == "Connected" else mode
        )
        try:
            shape = preview_match_shape(obj, edges, support, continuity, tangent_mode)
            self._set_match_preview_shape(shape)
            self.match_preview_status.setText(
                App.Qt.translate("Forms_Edit", "Match preview")
            )
            self.match_apply_button.setEnabled(True)
        except Exception as error:
            self._clear_match_preview()
            self.match_preview_status.setText(str(error))
            self.match_apply_button.setEnabled(False)
        self.session.view.redraw()

    def apply_match_tool(self):
        """Apply the selected Match continuity mode as one undoable action."""
        if not self.session.match_tool_active or self.match_inputs is None:
            return False
        from .matching import match_boundary

        obj, edges, support = self.match_inputs
        mode = str(self.match_mode.currentData())
        continuity = "Connected" if mode == "Connected" else "Tangent"
        tangent_mode = (
            str(getattr(obj, "MatchTangentMode", "AdjacentFaces")) if mode == "Connected" else mode
        )
        transaction = self.session._begin_action(App.Qt.translate("Forms_Edit", "Match form opening"))
        try:
            match_boundary(obj, edges, support, continuity, tangent_mode)
            obj.Document.recompute()
            self.session.topology_changed()
        except MODELING_ERRORS as error:
            self.session._finish_action(transaction, commit=False)
            return report_modeling_error(
                App.Qt.translate("Forms_Match", "Match"),
                error,
                self.match_preview_status,
            )
        except Exception:
            self.session._finish_action(transaction, commit=False)
            raise
        self.session._finish_action(transaction)
        self.stop_match_tool()
        return True

    def stop_match_tool(self):
        """Dismiss Match without ending the surrounding Form edit."""
        if not self.session.match_tool_active:
            return
        self._clear_match_preview()
        self._restore_match_preview_visibility()
        self.session.active_tool = None
        self.match_inputs = None
        self.match_mode = None
        self.match_apply_button = None
        self.match_cancel_button = None
        self.match_preview_status = None
        self.session._resume_selection_after_tool()
        self.session._hide_tool_handler()
        if not self.session.cleaned:
            self.session._show_input_hints()
        self.session.view.redraw()
        Gui.Command.update()
