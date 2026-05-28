# SPDX-License-Identifier: LGPL-2.1-or-later
"""Qt dock panel for FreeCAD Copilot."""

import base64
import mimetypes
import os
import traceback

import FreeCAD as App
import FreeCADGui as Gui

try:
    from PySide import QtCore, QtGui
    QtWidgets = QtGui
except ImportError:
    from PySide2 import QtCore, QtWidgets

from executor import CopilotExecutor
from provider import describe_plan, get_provider


PANEL_OBJECT_NAME = "CopilotDockPanel"


class CopilotPanel(QtWidgets.QDockWidget):
    """Dockable prompt interface."""

    def __init__(self, parent=None):
        super(CopilotPanel, self).__init__("Copilot", parent)
        self.setObjectName(PANEL_OBJECT_NAME)
        self.provider = get_provider()
        self.executor = CopilotExecutor()
        self.image_path = None
        self.image_data_url = None
        self._build_ui()

    def _build_ui(self):
        root = QtWidgets.QWidget()
        layout = QtWidgets.QVBoxLayout(root)

        self.prompt = QtWidgets.QPlainTextEdit()
        self.prompt.setPlaceholderText(
            "Try: create a box length 40 width 20 height 10\n"
            "Try: move selected x 10 y 0 z 5\n"
            "Try: color selected blue\n"
            "Or attach an image and ask: create this object"
        )
        self.prompt.setMinimumHeight(90)
        layout.addWidget(self.prompt)

        image_row = QtWidgets.QHBoxLayout()
        self.image_label = QtWidgets.QLabel("No image attached")
        self.attach_image_button = QtWidgets.QPushButton("Attach Image")
        self.clear_image_button = QtWidgets.QPushButton("Clear Image")
        image_row.addWidget(self.image_label, 1)
        image_row.addWidget(self.attach_image_button)
        image_row.addWidget(self.clear_image_button)
        layout.addLayout(image_row)

        button_row = QtWidgets.QHBoxLayout()
        self.run_button = QtWidgets.QPushButton("Run")
        self.plan_button = QtWidgets.QPushButton("Preview Plan")
        self.clear_button = QtWidgets.QPushButton("Clear")
        button_row.addWidget(self.run_button)
        button_row.addWidget(self.plan_button)
        button_row.addWidget(self.clear_button)
        layout.addLayout(button_row)

        self.output = QtWidgets.QPlainTextEdit()
        self.output.setReadOnly(True)
        self.output.setMinimumHeight(180)
        layout.addWidget(self.output)

        self.setWidget(root)

        self.run_button.clicked.connect(self.run_prompt)
        self.plan_button.clicked.connect(self.preview_plan)
        self.clear_button.clicked.connect(self.output.clear)
        self.attach_image_button.clicked.connect(self.attach_image)
        self.clear_image_button.clicked.connect(self.clear_image)

    def attach_image(self):
        try:
            path, _filter = QtWidgets.QFileDialog.getOpenFileName(
                self,
                "Attach reference image",
                "",
                "Images (*.png *.jpg *.jpeg *.webp *.bmp);;All Files (*)",
            )
            if not path:
                return
            max_bytes = 8 * 1024 * 1024
            if os.path.getsize(path) > max_bytes:
                raise ValueError("Image is larger than 8 MB. Please choose a smaller image.")
            mime_type = mimetypes.guess_type(path)[0] or "image/png"
            with open(path, "rb") as image_file:
                encoded = base64.b64encode(image_file.read()).decode("ascii")
            self.image_path = path
            self.image_data_url = "data:{0};base64,{1}".format(mime_type, encoded)
            self.image_label.setText(os.path.basename(path))
            self._write("Attached image: {0}".format(path))
        except Exception as err:
            self._write_error(err)

    def clear_image(self):
        self.image_path = None
        self.image_data_url = None
        self.image_label.setText("No image attached")

    def preview_plan(self):
        try:
            plan = self.provider.plan(self.prompt.toPlainText(), self._context())
            self._write(describe_plan(plan))
        except Exception as err:
            self._write_error(err)

    def run_prompt(self):
        try:
            plan = self.provider.plan(self.prompt.toPlainText(), self._context())
            self._write("Plan:\n{0}".format(describe_plan(plan)))
            results = self.executor.run(plan)
            self._write("\nResult:\n{0}".format("\n".join(results)))
        except Exception as err:
            self._write_error(err)

    def _write(self, text):
        self.output.setPlainText(text)
        App.Console.PrintMessage("{0}\n".format(text))

    def _write_error(self, err):
        message = "Copilot error: {0}".format(err)
        self.output.setPlainText(message)
        App.Console.PrintError("{0}\n{1}\n".format(message, traceback.format_exc()))

    def _context(self):
        context = _context()
        if self.image_data_url:
            context["image"] = {
                "path": self.image_path,
                "data_url": self.image_data_url,
            }
        return context


def show_panel():
    main_window = Gui.getMainWindow()
    panel = main_window.findChild(QtWidgets.QDockWidget, PANEL_OBJECT_NAME)
    if panel is None:
        panel = CopilotPanel(main_window)
        main_window.addDockWidget(QtCore.Qt.RightDockWidgetArea, panel)
    panel.show()
    panel.raise_()
    return panel


def _context():
    doc = App.ActiveDocument
    selection = []
    try:
        selection = [obj.Label for obj in Gui.Selection.getSelection()]
    except Exception:
        selection = []
    return {
        "document": doc.Name if doc else None,
        "selection": selection,
    }
