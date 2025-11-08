"""
ArchGen AI Panel - Integrated AI interface
Provides a dockable AI assistant panel for ArchGen CAD
"""

import FreeCAD
import FreeCADGui as Gui
from PySide2 import QtCore, QtWidgets, QtGui


class ArchGenAIPanel(QtWidgets.QWidget):
    """Main AI assistant panel that can be docked in the main window"""

    def __init__(self, parent=None):
        super(ArchGenAIPanel, self).__init__(parent)
        self.setup_ui()
        self.conversation_history = []

    def setup_ui(self):
        """Create the AI interface"""
        layout = QtWidgets.QVBoxLayout()
        layout.setContentsMargins(10, 10, 10, 10)

        # Header
        header = QtWidgets.QLabel("🤖 ArchGen AI Assistant")
        header.setStyleSheet(
            "font-size: 16px; font-weight: bold; color: #4a90e2; "
            "padding: 10px; background-color: rgba(74, 144, 226, 0.1); "
            "border-radius: 5px;"
        )
        header.setAlignment(QtCore.Qt.AlignCenter)
        layout.addWidget(header)

        # Description
        desc = QtWidgets.QLabel(
            "Describe what you want to create and I'll generate the CAD model for you."
        )
        desc.setWordWrap(True)
        desc.setStyleSheet("color: #888888; padding: 5px; font-size: 10pt;")
        layout.addWidget(desc)

        # Quick actions buttons
        quick_actions = QtWidgets.QGroupBox("Quick Actions")
        quick_layout = QtWidgets.QVBoxLayout()

        open_dialog_btn = QtWidgets.QPushButton("📝 Open Full Dialog")
        open_dialog_btn.setStyleSheet(self._get_button_style("#3498db"))
        open_dialog_btn.clicked.connect(self.open_full_dialog)
        quick_layout.addWidget(open_dialog_btn)

        settings_btn = QtWidgets.QPushButton("⚙️ Settings")
        settings_btn.setStyleSheet(self._get_button_style("#7f8c8d"))
        settings_btn.clicked.connect(self.open_settings)
        quick_layout.addWidget(settings_btn)

        quick_actions.setLayout(quick_layout)
        layout.addWidget(quick_actions)

        # Prompt input section
        prompt_label = QtWidgets.QLabel("Quick Prompt:")
        prompt_label.setStyleSheet("font-weight: bold; margin-top: 10px;")
        layout.addWidget(prompt_label)

        self.prompt_text = QtWidgets.QTextEdit()
        self.prompt_text.setObjectName("ArchGenAIPrompt")
        self.prompt_text.setPlaceholderText(
            "E.g., 'Create a hexagonal bolt with M8 threading'"
        )
        self.prompt_text.setMaximumHeight(80)
        self.prompt_text.setStyleSheet(
            "background-color: #2a2a2a; border: 2px solid #4a90e2; "
            "border-radius: 8px; padding: 5px; font-size: 11pt; color: #ffffff;"
        )
        layout.addWidget(self.prompt_text)

        # Generate button
        self.generate_btn = QtWidgets.QPushButton("🚀 Generate Now")
        self.generate_btn.setObjectName("ArchGenAIButton")
        self.generate_btn.setMinimumHeight(40)
        self.generate_btn.setStyleSheet(self._get_button_style("#e74c3c", hover="#c0392b"))
        self.generate_btn.clicked.connect(self.quick_generate)
        layout.addWidget(self.generate_btn)

        # Example prompts section
        examples_label = QtWidgets.QLabel("Example Prompts:")
        examples_label.setStyleSheet("font-weight: bold; margin-top: 15px;")
        layout.addWidget(examples_label)

        examples = [
            "Design a gear with 20 teeth",
            "Create a bracket for motor mounting",
            "Make a cylindrical bearing housing",
            "Design an enclosure box 100x50x30mm"
        ]

        for example in examples:
            example_btn = QtWidgets.QPushButton(f"💡 {example}")
            example_btn.setStyleSheet(
                """
                QPushButton {
                    text-align: left;
                    padding: 8px;
                    border: 1px solid #666666;
                    border-radius: 4px;
                    background-color: #333333;
                    color: #ffffff;
                    font-size: 9pt;
                }
                QPushButton:hover {
                    background-color: #404040;
                    border-color: #4a90e2;
                }
                """
            )
            example_btn.clicked.connect(lambda checked, text=example: self.use_example(text))
            layout.addWidget(example_btn)

        # Status/Recent generations
        recent_label = QtWidgets.QLabel("Recent Generations:")
        recent_label.setStyleSheet("font-weight: bold; margin-top: 15px;")
        layout.addWidget(recent_label)

        self.history_list = QtWidgets.QListWidget()
        self.history_list.setMaximumHeight(120)
        self.history_list.setStyleSheet(
            "background-color: #2a2a2a; border: 1px solid #666666; "
            "border-radius: 4px; color: #ffffff; font-size: 9pt;"
        )
        layout.addWidget(self.history_list)

        # Spacer
        layout.addStretch()

        # Info footer
        info_label = QtWidgets.QLabel("Powered by ArchGen AI")
        info_label.setAlignment(QtCore.Qt.AlignCenter)
        info_label.setStyleSheet("color: #666666; font-size: 8pt; padding: 5px;")
        layout.addWidget(info_label)

        self.setLayout(layout)

    def _get_button_style(self, color, hover=None):
        """Generate consistent button styles"""
        if hover is None:
            hover = color

        return f"""
            QPushButton {{
                background: {color};
                border: none;
                border-radius: 6px;
                color: white;
                font-weight: bold;
                padding: 10px 16px;
            }}
            QPushButton:hover {{
                background: {hover};
            }}
            QPushButton:pressed {{
                background: {color};
            }}
        """

    def use_example(self, text):
        """Fill prompt with example text"""
        self.prompt_text.setPlainText(text)
        self.prompt_text.setFocus()

    def quick_generate(self):
        """Quick generation from the panel"""
        prompt = self.prompt_text.toPlainText().strip()
        if prompt:
            # Add to history
            timestamp = QtCore.QDateTime.currentDateTime().toString("hh:mm:ss")
            self.history_list.addItem(f"[{timestamp}] {prompt[:40]}{'...' if len(prompt) > 40 else ''}")

            # Open full dialog with this prompt pre-filled
            self.open_full_dialog(prefilled_prompt=prompt)
        else:
            QtWidgets.QMessageBox.information(
                self,
                "ArchGen AI",
                "Please enter a description of what you want to create."
            )

    def open_full_dialog(self, prefilled_prompt=None):
        """Open the full conversational dialog"""
        try:
            # Execute the ArchGen_Generate command
            Gui.runCommand("ArchGen_Generate")

            # If we have a prefilled prompt, we could set it in the dialog
            # This would require the command to check for a global variable or similar
            if prefilled_prompt:
                # Store the prompt globally for the dialog to pick up
                FreeCAD.archgen_prefilled_prompt = prefilled_prompt

        except Exception as e:
            FreeCAD.Console.PrintError(f"Failed to open AI dialog: {e}\n")
            QtWidgets.QMessageBox.critical(
                self,
                "Error",
                f"Failed to open AI generation dialog:\n{str(e)}"
            )

    def open_settings(self):
        """Open settings dialog"""
        try:
            Gui.runCommand("ArchGen_Settings")
        except Exception as e:
            FreeCAD.Console.PrintError(f"Failed to open settings: {e}\n")

    def add_to_history(self, prompt):
        """Add a generation to the history list"""
        timestamp = QtCore.QDateTime.currentDateTime().toString("hh:mm:ss")
        self.history_list.addItem(f"[{timestamp}] {prompt[:40]}{'...' if len(prompt) > 40 else ''}")

        # Keep only last 10 items
        while self.history_list.count() > 10:
            self.history_list.takeItem(0)


# Standalone test
if __name__ == "__main__":
    import sys

    app = QtWidgets.QApplication(sys.argv)
    panel = ArchGenAIPanel()
    panel.setMinimumSize(350, 600)
    panel.show()
    sys.exit(app.exec_())
