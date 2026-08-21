# SPDX-License-Identifier: LGPL-2.1-or-later

"""Task panel for MbDFEM animation playback controls."""

import FreeCAD as App
import FreeCADGui as Gui

import FreeCADMbDAnimation


def owning_assembly(animation_parameters):
    """Return the MbDAssembly that owns *animation_parameters*."""
    document = getattr(animation_parameters, "Document", None)
    if document is None:
        return None

    for obj in document.Objects:
        try:
            if obj.isDerivedFrom("MbDFEM::MbDAssembly") and obj.getAnimationParameters() == animation_parameters:
                return obj
        except Exception:
            pass
    return None


def is_animation_parameters(obj):
    try:
        return obj is not None and obj.isDerivedFrom("MbDFEM::MbDAnimationParameters")
    except Exception:
        return False


class AnimationTaskPanel:
    """Task-tab controls for an MbDAnimationParameters object."""

    def __init__(self, animation_parameters):
        from PySide import QtCore, QtWidgets

        self.animation_parameters = animation_parameters
        self.assembly = owning_assembly(animation_parameters)
        self.controller = (
            FreeCADMbDAnimation.AnimationController(self.assembly) if self.assembly is not None else None
        )
        self._original_parameters = self._parameter_values()
        self._updating = False
        self._timer = QtCore.QTimer()
        self._timer.timeout.connect(self._tick)

        self.form = QtWidgets.QWidget()
        self.form.setWindowTitle("MbDFEM Animation")

        layout = QtWidgets.QVBoxLayout(self.form)
        layout.setSpacing(8)

        self.status_label = QtWidgets.QLabel()
        self.status_label.setWordWrap(True)
        layout.addWidget(self.status_label)

        button_layout = QtWidgets.QHBoxLayout()
        self.step_back_button = QtWidgets.QToolButton()
        self.step_back_button.setText("<")
        self.step_back_button.setToolTip("Step backward")
        self.play_button = QtWidgets.QToolButton()
        self.play_button.setText("Play")
        self.play_button.setToolTip("Play")
        self.pause_button = QtWidgets.QToolButton()
        self.pause_button.setText("Pause")
        self.pause_button.setToolTip("Pause")
        self.stop_button = QtWidgets.QToolButton()
        self.stop_button.setText("Stop")
        self.stop_button.setToolTip("Stop")
        self.step_forward_button = QtWidgets.QToolButton()
        self.step_forward_button.setText(">")
        self.step_forward_button.setToolTip("Step forward")

        for button in (
            self.step_back_button,
            self.play_button,
            self.pause_button,
            self.stop_button,
            self.step_forward_button,
        ):
            button_layout.addWidget(button)
        layout.addLayout(button_layout)

        self.slider = QtWidgets.QSlider(QtCore.Qt.Horizontal)
        layout.addWidget(self.slider)

        frame_layout = QtWidgets.QHBoxLayout()
        frame_layout.addWidget(QtWidgets.QLabel("Result frame:"))
        self.frame_spin = QtWidgets.QSpinBox()
        self.frame_spin.setToolTip("Result-series frame index")
        frame_layout.addWidget(self.frame_spin)
        self.frame_count_label = QtWidgets.QLabel()
        frame_layout.addWidget(self.frame_count_label)
        frame_layout.addStretch()
        frame_layout.addWidget(QtWidgets.QLabel("Simulation time:"))
        self.time_label = QtWidgets.QLabel()
        frame_layout.addWidget(self.time_label)
        layout.addLayout(frame_layout)

        settings = QtWidgets.QFormLayout()
        self.update_rate_spin = QtWidgets.QSpinBox()
        self.update_rate_spin.setRange(1, 240)
        self.update_rate_spin.setSuffix(" updates/sec")
        self.update_rate_spin.setToolTip("Real-time UI ticks per second")
        self.start_frame_spin = QtWidgets.QSpinBox()
        self.start_frame_spin.setToolTip("First result-series frame index used for playback")
        self.end_frame_spin = QtWidgets.QSpinBox()
        self.end_frame_spin.setToolTip("Last result-series frame index used for playback")
        self.speed_spin = QtWidgets.QDoubleSpinBox()
        self.speed_spin.setRange(0.01, 100.0)
        self.speed_spin.setDecimals(2)
        self.speed_spin.setSingleStep(0.25)
        self.speed_spin.setSuffix("x")
        self.speed_spin.setToolTip("Simulation seconds per real second multiplier")
        self.scale_spin = QtWidgets.QDoubleSpinBox()
        self.scale_spin.setRange(0.001, 1000000.0)
        self.scale_spin.setDecimals(3)
        self.scale_spin.setSingleStep(100.0)
        self.loop_check = QtWidgets.QCheckBox()
        self.interpolate_check = QtWidgets.QCheckBox()
        settings.addRow("Update rate:", self.update_rate_spin)
        settings.addRow("Start frame:", self.start_frame_spin)
        settings.addRow("End frame:", self.end_frame_spin)
        settings.addRow("Playback speed:", self.speed_spin)
        settings.addRow("Position scale:", self.scale_spin)
        settings.addRow("Loop:", self.loop_check)
        settings.addRow("Interpolate:", self.interpolate_check)
        layout.addLayout(settings)

        self.step_back_button.clicked.connect(self._step_backward)
        self.play_button.clicked.connect(self._play)
        self.pause_button.clicked.connect(self._pause)
        self.stop_button.clicked.connect(self._stop)
        self.step_forward_button.clicked.connect(self._step_forward)
        self.slider.valueChanged.connect(self._set_frame)
        self.frame_spin.valueChanged.connect(self._set_frame)
        self.update_rate_spin.valueChanged.connect(self._set_update_rate)
        self.start_frame_spin.valueChanged.connect(self._set_start_frame)
        self.end_frame_spin.valueChanged.connect(self._set_end_frame)
        self.speed_spin.valueChanged.connect(self._set_speed)
        self.scale_spin.valueChanged.connect(self._set_scale)
        self.loop_check.toggled.connect(self._set_loop)
        self.interpolate_check.toggled.connect(self._set_interpolate)

        self._load_parameters()
        self._configure_frame_controls()
        self._refresh()

    def getStandardButtons(self):
        from PySide import QtGui

        return QtGui.QDialogButtonBox.Ok | QtGui.QDialogButtonBox.Cancel

    def accept(self):
        self._pause()
        return True

    def reject(self):
        self._pause()
        self._restore_parameters(self._original_parameters)
        return True

    def _parameter_values(self):
        return {
            "updateRate": getattr(self.animation_parameters, "updateRate", 30),
            "startFrame": getattr(self.animation_parameters, "startFrame", 1),
            "endFrame": getattr(self.animation_parameters, "endFrame", -1),
            "playbackSpeed": getattr(self.animation_parameters, "playbackSpeed", 1.0),
            "showTrails": getattr(self.animation_parameters, "showTrails", False),
            "trailLength": getattr(self.animation_parameters, "trailLength", 60),
            "loop": getattr(self.animation_parameters, "loop", True),
            "interpolateFrames": getattr(self.animation_parameters, "interpolateFrames", True),
            "lengthScale": getattr(self.controller, "length_scale", None),
        }

    def _restore_parameters(self, values):
        self.animation_parameters.updateRate = values["updateRate"]
        self.animation_parameters.startFrame = values["startFrame"]
        self.animation_parameters.endFrame = values["endFrame"]
        self.animation_parameters.playbackSpeed = values["playbackSpeed"]
        self.animation_parameters.showTrails = values["showTrails"]
        self.animation_parameters.trailLength = values["trailLength"]
        self.animation_parameters.loop = values["loop"]
        self.animation_parameters.interpolateFrames = values["interpolateFrames"]
        if self.controller is not None and values["lengthScale"] is not None:
            self.controller.length_scale = values["lengthScale"]
            self.controller.setFrame(self.controller.current_frame)

    def _load_parameters(self):
        self._updating = True
        try:
            self.update_rate_spin.setValue(
                int(
                    getattr(
                        self.animation_parameters,
                        "updateRate",
                        30,
                    )
                )
            )
            self.speed_spin.setValue(float(getattr(self.animation_parameters, "playbackSpeed", 1.0)))
            self.scale_spin.setValue(float(getattr(self.controller, "length_scale", 1000.0)))
            self.loop_check.setChecked(bool(getattr(self.animation_parameters, "loop", True)))
            self.interpolate_check.setChecked(
                bool(getattr(self.animation_parameters, "interpolateFrames", True))
            )
        finally:
            self._updating = False

    def _configure_frame_controls(self):
        source_frame_count = self.controller.source_frame_count if self.controller is not None else 0
        maximum = max(source_frame_count - 1, 0)
        start_frame = self.controller.start_frame if self.controller is not None else 0
        end_frame = self.controller.end_frame if self.controller is not None else 0

        self._updating = True
        try:
            self.start_frame_spin.setRange(0, maximum)
            self.end_frame_spin.setRange(0, maximum)
            self.start_frame_spin.setValue(start_frame)
            self.end_frame_spin.setValue(end_frame)
            self.slider.setRange(0, maximum)
            self.frame_spin.setRange(0, maximum)
        finally:
            self._updating = False

        enabled = source_frame_count > 0
        for widget in (
            self.step_back_button,
            self.play_button,
            self.pause_button,
            self.stop_button,
            self.step_forward_button,
            self.slider,
            self.frame_spin,
            self.start_frame_spin,
            self.end_frame_spin,
        ):
            widget.setEnabled(enabled)

    def _refresh(self):
        source_frame_count = self.controller.source_frame_count if self.controller is not None else 0
        if self.assembly is None:
            self.status_label.setText("No owning MbDAssembly found.")
        elif source_frame_count == 0:
            self.status_label.setText("No solved MbDFEM result series.")
        else:
            self.status_label.setText(self.assembly.Label)

        current_frame = self.controller.current_frame if self.controller is not None else 0
        current_time = self.controller.current_time if self.controller is not None else 0.0
        self._updating = True
        try:
            self.slider.setValue(current_frame)
            self.frame_spin.setValue(current_frame)
        finally:
            self._updating = False
        maximum = max(source_frame_count - 1, 0)
        self.frame_count_label.setText(f"/ {maximum}")
        self.time_label.setText(f"{current_time:.6g} s")

    def _play(self):
        if self.controller is None or self.controller.frame_count == 0:
            return
        if self.controller.current_frame < self.controller.start_frame or self.controller.current_frame > self.controller.end_frame:
            self.controller.setFrame(self.controller.start_frame)
        interval = max(int(1000.0 / self.controller.update_rate), 1)
        self.controller.is_playing = True
        self._timer.start(interval)

    def _pause(self):
        self._timer.stop()
        if self.controller is not None:
            self.controller.pause()
        self._refresh()

    def _stop(self):
        self._pause()
        if self.controller is not None:
            self.controller.stop()
        self._refresh()

    def _tick(self):
        if self.controller is None:
            return
        self.controller.tick()
        if not self.controller.is_playing:
            self._timer.stop()
        self._refresh()

    def _step_forward(self):
        if self.controller is not None:
            self.controller.stepForward()
        self._refresh()

    def _step_backward(self):
        if self.controller is not None:
            self.controller.stepBackward()
        self._refresh()

    def _set_frame(self, frame):
        if self._updating or self.controller is None:
            return
        self.controller.setFrame(frame)
        self._refresh()

    def _set_update_rate(self, value):
        if self._updating:
            return
        self.animation_parameters.updateRate = int(value)
        if self._timer.isActive():
            self._play()

    def _set_start_frame(self, value):
        if self._updating or self.controller is None:
            return
        self.animation_parameters.startFrame = int(value)
        if int(getattr(self.animation_parameters, "endFrame", -1)) >= 0:
            self.animation_parameters.endFrame = max(int(self.animation_parameters.endFrame), int(value))
        self._configure_frame_controls()
        self._refresh()

    def _set_end_frame(self, value):
        if self._updating or self.controller is None:
            return
        self.animation_parameters.endFrame = max(int(value), self.controller.start_frame)
        self._configure_frame_controls()
        self._refresh()

    def _set_speed(self, value):
        if self._updating:
            return
        self.animation_parameters.playbackSpeed = float(value)

    def _set_scale(self, value):
        if self.controller is not None:
            self.controller.length_scale = float(value)
            self.controller.setFrame(self.controller.current_frame)
        self._refresh()

    def _set_loop(self, checked):
        if not self._updating:
            self.animation_parameters.loop = bool(checked)

    def _set_interpolate(self, checked):
        if not self._updating:
            self.animation_parameters.interpolateFrames = bool(checked)


class AnimationParametersSelectionObserver:
    """Open the animation Task panel when an AnimationParameters object is selected."""

    def addSelection(self, document_name, object_name, sub_name, mouse_position):
        obj = self._selected_object(document_name, object_name, sub_name)
        if not is_animation_parameters(obj):
            return
        show_animation_task_panel(obj)

    @staticmethod
    def _selected_object(document_name, object_name, sub_name=""):
        document = App.getDocument(document_name)
        root = document.getObject(object_name) if document is not None else None
        if root is None:
            return None
        if is_animation_parameters(root):
            return root

        if sub_name:
            try:
                resolved = root.getSubObject(sub_name)
                if is_animation_parameters(resolved):
                    return resolved
            except Exception:
                pass

            object_from_subname = document.getObject(sub_name.rstrip(".").rsplit(".", 1)[-1])
            if is_animation_parameters(object_from_subname):
                return object_from_subname

        return root


def show_animation_task_panel(animation_parameters):
    active = Gui.Control.activeDialog()
    if isinstance(active, AnimationTaskPanel):
        if active.animation_parameters == animation_parameters:
            return active
        Gui.Control.closeDialog()
    elif active is not None:
        Gui.Control.closeDialog()

    panel = AnimationTaskPanel(animation_parameters)
    Gui.Control.showDialog(panel)
    return panel
