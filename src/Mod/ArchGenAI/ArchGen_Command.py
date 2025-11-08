# ArchGen_Command.py - v3.0 with Enhanced Integration and Quality Control
import FreeCAD as App
import FreeCADGui as Gui
import requests
import json
import os
import time
import uuid
from pathlib import Path

# --- Configuration ---
DEFAULT_API_URL = "http://127.0.0.1:8000"
MODEL = "qwen/qwen3-coder:free"
API_TIMEOUT = (20, 1200)  # 20s to connect, 20 minutes to wait for response


# --- Configuration Management ---
def get_config_path():
    """Gets the appropriate FreeCAD configuration directory for storing settings."""
    try:
        if hasattr(App, 'getUserConfigDir'):
            return App.getUserConfigDir()
        if hasattr(App, 'getUserAppDataDir'):
            return App.getUserAppDataDir()
        system = __import__('platform').system()
        if system == "Windows":
            return os.path.expandvars("%APPDATA%/FreeCAD")
        if system == "Darwin":
            return os.path.expanduser("~/Library/Application Support/FreeCAD")
        return os.path.expanduser("~/.local/share/FreeCAD")
    except Exception as e:
        App.Console.PrintWarning(f"Could not determine config path, using default: {e}\n")
        return os.path.expanduser("~/.FreeCAD")


CONFIG_FILE = os.path.join(get_config_path(), "ArchGen_config_v3.json")


class ArchGenConfig:
    """Handles loading and saving of workbench configuration like API URL and session ID."""

    def __init__(self):
        self.config = self._load_config()

    def _load_config(self):
        default_config = {
            "api_url": DEFAULT_API_URL,
            "session_id": None,
            "model_name": MODEL,
            "quality_threshold": 0.7,
            "max_iterations": 5,
            "temperature": 0.1
        }
        if os.path.exists(CONFIG_FILE):
            try:
                with open(CONFIG_FILE, 'r') as f:
                    config_data = json.load(f)
                    default_config.update(config_data)
            except (json.JSONDecodeError, IOError) as e:
                App.Console.PrintError(f"Error loading ArchGen config: {e}\n")
        return default_config

    def _save_config(self):
        try:
            os.makedirs(os.path.dirname(CONFIG_FILE), exist_ok=True)
            with open(CONFIG_FILE, 'w') as f:
                json.dump(self.config, f, indent=4)
        except IOError as e:
            App.Console.PrintError(f"Failed to save ArchGen config: {e}\n")

    def get(self, key, default=None):
        return self.config.get(key, default)

    def set(self, key, value):
        self.config[key] = value
        self._save_config()


config = ArchGenConfig()


# --- Main Generation Command ---
class ArchGen_Generate_Command:
    """The main command that opens the conversational UI for model generation."""

    def GetResources(self):
        return {
            "Pixmap": "",  # TODO: Add custom icon
            "MenuText": "🤖 Generate with AI",
            "ToolTip": "Generate a 3D CAD model using text description with ArchGen AI",
            "Accel": "Ctrl+G"
        }

    def Activated(self):
        """Called when the command is activated from the FreeCAD menu or toolbar."""
        self.show_conversational_dialog()

    def show_conversational_dialog(self):
        """Initializes and displays the main user interface dialog."""
        try:
            from PySide2.QtWidgets import (QDialog, QVBoxLayout, QHBoxLayout, QLabel,
                                           QTextEdit, QPushButton, QScrollArea, QWidget,
                                           QProgressBar, QCheckBox, QFrame, QApplication,
                                           QComboBox, QSpinBox, QDoubleSpinBox, QGroupBox)
            from PySide2.QtCore import Qt, QThread, Signal
            from PySide2.QtGui import QFont

            class ConversationalDialog(QDialog):
                STYLE_SHEET = """
                    QDialog { background-color: #2c3e50; }
                    QLabel { font-size: 11pt; color: #ecf0f1; }
                    QCheckBox { font-size: 10pt; color: #bdc3c7; }
                    QTextEdit {
                        background-color: #34495e;
                        border: 1px solid #7f8c8d;
                        border-radius: 8px;
                        padding: 10px;
                        font-size: 11pt;
                        color: #ecf0f1;
                    }
                    QComboBox, QSpinBox, QDoubleSpinBox {
                        background-color: #34495e;
                        border: 1px solid #7f8c8d;
                        border-radius: 4px;
                        padding: 4px;
                        font-size: 10pt;
                        color: #ecf0f1;
                    }
                    QComboBox::drop-down { border: none; }
                    QGroupBox {
                        border: 1px solid #7f8c8d;
                        border-radius: 5px;
                        margin-top: 10px;
                        padding-top: 10px;
                        color: #ecf0f1;
                    }
                    QGroupBox::title {
                        subcontrol-origin: margin;
                        subcontrol-position: top left;
                        padding: 0 5px;
                    }
                """

                def __init__(self):
                    super().__init__()
                    self.setWindowTitle("ArchGen AI - Text-to-CAD Generation")
                    self.setMinimumSize(750, 700)
                    self.setStyleSheet(self.STYLE_SHEET)

                    self.api_worker = None
                    self.session_id = config.get("session_id")
                    if not self.session_id or len(self.session_id) < 10:
                        self.session_id = None
                        config.set("session_id", None)

                    self._setup_ui()

                def _setup_ui(self):
                    main_layout = QVBoxLayout(self)
                    main_layout.setContentsMargins(20, 20, 20, 20)

                    # Header
                    header = QLabel("🤖 ArchGen AI Assistant")
                    header.setFont(QFont("Arial", 18, QFont.Bold))
                    header.setAlignment(Qt.AlignCenter)
                    header.setStyleSheet("color: #ffffff; margin-bottom: 10px;")
                    main_layout.addWidget(header)

                    # Session Options
                    session_layout = QHBoxLayout()
                    self.new_session_cb = QCheckBox("New Session")
                    self.new_session_cb.setChecked(not self.session_id)
                    self.session_label = QLabel(
                        f"Session: {self.session_id[:8] + '...' if self.session_id else 'None'}")
                    self.session_label.setStyleSheet("color: #95a5a6;")

                    session_layout.addWidget(self.new_session_cb)
                    session_layout.addWidget(self.session_label)
                    session_layout.addStretch()
                    main_layout.addLayout(session_layout)

                    # Advanced Settings Group
                    settings_group = QGroupBox("Generation Settings")
                    settings_layout = QHBoxLayout()

                    # Quality Threshold
                    quality_label = QLabel("Quality:")
                    self.quality_combo = QComboBox()
                    self.quality_combo.addItems(["0.6 (Fast)", "0.7 (Standard)", "0.8 (High)", "0.9 (Very High)"])
                    self.quality_combo.setCurrentIndex(1)  # Default to 0.7

                    # Max Iterations
                    iter_label = QLabel("Max Iterations:")
                    self.iter_spin = QSpinBox()
                    self.iter_spin.setRange(1, 10)
                    self.iter_spin.setValue(config.get("max_iterations", 5))

                    # Temperature
                    temp_label = QLabel("Temperature:")
                    self.temp_spin = QDoubleSpinBox()
                    self.temp_spin.setRange(0.0, 1.0)
                    self.temp_spin.setSingleStep(0.1)
                    self.temp_spin.setValue(config.get("temperature", 0.1))

                    settings_layout.addWidget(quality_label)
                    settings_layout.addWidget(self.quality_combo)
                    settings_layout.addWidget(iter_label)
                    settings_layout.addWidget(self.iter_spin)
                    settings_layout.addWidget(temp_label)
                    settings_layout.addWidget(self.temp_spin)
                    settings_layout.addStretch()

                    settings_group.setLayout(settings_layout)
                    main_layout.addWidget(settings_group)

                    # Chat History Area
                    self.scroll_area = QScrollArea()
                    self.scroll_area.setWidgetResizable(True)
                    self.scroll_area.setStyleSheet("QScrollArea { border: none; }")
                    self.chat_widget = QWidget()
                    self.chat_layout = QVBoxLayout(self.chat_widget)
                    self.chat_layout.addStretch()
                    self.scroll_area.setWidget(self.chat_widget)
                    main_layout.addWidget(self.scroll_area)

                    # Prompt Input
                    self.prompt_input = QTextEdit()
                    self.prompt_input.setPlaceholderText(
                        "Describe the CAD model you want to create...\n\n"
                        "Examples:\n"
                        "- Design a hexagonal bolt with M8 threading\n"
                        "- Create a gear with 20 teeth and 5mm thickness\n"
                        "- Make a rectangular enclosure 100x50x30mm with mounting holes"
                    )
                    self.prompt_input.setMaximumHeight(120)
                    main_layout.addWidget(self.prompt_input)

                    # Progress Bar
                    self.progress_bar = QProgressBar()
                    self.progress_bar.setVisible(False)
                    self.progress_bar.setTextVisible(False)
                    self.progress_bar.setStyleSheet(
                        "QProgressBar { border-radius: 5px; background-color: #34495e; } "
                        "QProgressBar::chunk { border-radius: 5px; background-color: #3498db; }"
                    )
                    main_layout.addWidget(self.progress_bar)

                    # Status Label
                    self.status_label = QLabel("Ready for your prompt.")
                    self.status_label.setAlignment(Qt.AlignCenter)
                    self.status_label.setStyleSheet("color: #95a5a6; padding: 5px;")
                    main_layout.addWidget(self.status_label)

                    # Action Buttons
                    button_layout = QHBoxLayout()
                    self.generate_btn = QPushButton("🚀 Generate CAD Model")
                    self.generate_btn.setStyleSheet(
                        "QPushButton {background-color: #3498db; color: white; font-size: 12pt; "
                        "padding: 12px 24px; border-radius: 8px; border: none; font-weight: bold;} "
                        "QPushButton:hover { background-color: #2980b9; }"
                    )
                    self.generate_btn.clicked.connect(self.send_prompt)

                    clear_btn = QPushButton("Clear Chat")
                    clear_btn.setStyleSheet(
                        "QPushButton {background-color: #7f8c8d; color: white; font-size: 11pt; "
                        "padding: 10px 20px; border-radius: 8px; border: none; font-weight: bold;} "
                        "QPushButton:hover { background-color: #95a5a6; }"
                    )
                    clear_btn.clicked.connect(self.clear_chat)

                    button_layout.addWidget(clear_btn)
                    button_layout.addStretch()
                    button_layout.addWidget(self.generate_btn)
                    main_layout.addLayout(button_layout)

                def add_chat_message(self, message, is_user=True):
                    frame = QFrame()
                    layout = QVBoxLayout(frame)
                    label = QLabel(message)
                    label.setWordWrap(True)
                    label.setTextInteractionFlags(Qt.TextSelectableByMouse)
                    layout.addWidget(label)

                    if is_user:
                        frame.setStyleSheet(
                            "background-color: #2980b9; border-radius: 12px; padding: 12px; margin-left: 60px;"
                        )
                    else:
                        frame.setStyleSheet(
                            "background-color: #34495e; border-radius: 12px; padding: 12px; margin-right: 60px;"
                        )

                    self.chat_layout.insertWidget(self.chat_layout.count() - 1, frame)
                    QApplication.processEvents()
                    self.scroll_area.verticalScrollBar().setValue(
                        self.scroll_area.verticalScrollBar().maximum()
                    )

                def send_prompt(self):
                    prompt = self.prompt_input.toPlainText().strip()
                    if not prompt or (self.api_worker and self.api_worker.isRunning()):
                        return

                    self.add_chat_message(prompt, is_user=True)
                    self.prompt_input.clear()
                    self._set_ui_for_generation(True)

                    # Get settings
                    is_new = self.new_session_cb.isChecked() or not self.session_id
                    quality_score_text = self.quality_combo.currentText().split(" ")[0]
                    quality_score = float(quality_score_text)
                    max_iterations = self.iter_spin.value()
                    temperature = self.temp_spin.value()

                    # Save settings
                    config.set("max_iterations", max_iterations)
                    config.set("temperature", temperature)

                    self.api_worker = APIWorker(
                        prompt, is_new, self.session_id,
                        quality_score, max_iterations, temperature
                    )
                    self.api_worker.finished.connect(self.handle_api_response)
                    self.api_worker.progress_update.connect(self.status_label.setText)
                    self.api_worker.start()

                def handle_api_response(self, success, data, generation_time):
                    if success:
                        script = data.get("freecad_script")
                        self.session_id = data.get("session_id")

                        time_str = f"(took {generation_time:.2f}s)"
                        self.status_label.setText(f"API response received {time_str}. Executing script...")

                        final_score = data.get("final_score", 0)
                        iterations = data.get("iterations", 0)

                        self.add_chat_message(
                            f"✅ Success! Score: {final_score:.2f}, Iterations: {iterations}\n"
                            f"Time: {time_str}\n"
                            f"Rendering model...",
                            is_user=False
                        )

                        render_success, error_msg = self.render_freecad_script(script, "GeneratedModel")

                        if render_success:
                            self.add_chat_message("🎉 Model generated successfully!", is_user=False)
                        else:
                            self.add_chat_message(f"❌ Script execution failed:\n\n{error_msg}", is_user=False)
                    else:
                        error_summary = data.get("error_summary", "An unknown error occurred.")
                        self.add_chat_message(f"❌ API request failed: {error_summary}", is_user=False)

                    self._on_generation_finished(success)

                def _on_generation_finished(self, success):
                    self._set_ui_for_generation(False)
                    if success and self.session_id:
                        config.set("session_id", self.session_id)
                        self.session_label.setText(f"Session: {self.session_id[:8] + '...'}")
                        self.new_session_cb.setChecked(False)
                    self.api_worker = None

                def clear_chat(self):
                    while self.chat_layout.count() > 1:
                        widget = self.chat_layout.takeAt(0).widget()
                        if widget:
                            widget.deleteLater()
                    self.session_id = None
                    config.set("session_id", None)
                    self.session_label.setText("Session: None")
                    self.new_session_cb.setChecked(True)
                    self.status_label.setText("Chat cleared. Ready for new session.")

                def _set_ui_for_generation(self, is_generating):
                    self.generate_btn.setEnabled(not is_generating)
                    self.prompt_input.setEnabled(not is_generating)
                    self.progress_bar.setVisible(is_generating)
                    if is_generating:
                        self.status_label.setText("Sending prompt to ArchGen AI...")
                        self.progress_bar.setRange(0, 0)
                    else:
                        self.status_label.setText("Ready for your prompt.")
                        self.progress_bar.setRange(0, 100)

                def render_freecad_script(self, script, name):
                    doc_name = ""
                    try:
                        timestamp = int(time.time())
                        doc_name = f"ArchGen_{name}_{timestamp}"
                        doc = App.newDocument(doc_name)
                        App.setActiveDocument(doc_name)
                        current_doc = App.ActiveDocument

                        if not current_doc:
                            raise Exception("Failed to create and set active document.")

                        execution_globals = {'App': App, 'doc': current_doc}
                        for mod in ['Part', 'math', 'Sketcher', 'PartDesign', 'Draft']:
                            try:
                                execution_globals[mod] = __import__(mod)
                            except ImportError:
                                pass

                        clean_script = self._prepare_script(script)
                        exec(clean_script, execution_globals)
                        current_doc.recompute()

                        if Gui.ActiveDocument:
                            Gui.SendMsgToActiveView("ViewFit")

                        App.Console.PrintMessage(f"✅ Successfully created model: {name}\n")
                        return True, None
                    except Exception:
                        error_trace = __import__('traceback').format_exc()
                        App.Console.PrintError(f">> Script execution error in {doc_name}:\n{error_trace}\n")
                        if doc_name and App.getDocument(doc_name):
                            App.closeDocument(doc_name)
                        return False, error_trace

                def _prepare_script(self, script):
                    if not isinstance(script, str) or not script.strip():
                        raise ValueError("Received an empty or invalid script.")

                    # Basic sanitization - remove potentially dangerous patterns
                    dangerous_patterns = ['os.system', 'subprocess', 'eval(', '__import__']
                    safe_lines = []
                    for line in script.split('\n'):
                        if not any(pattern in line for pattern in dangerous_patterns):
                            safe_lines.append(line)
                        else:
                            App.Console.PrintWarning(f">> Sanitized: {line.strip()}\n")

                    return '\n'.join(safe_lines)

            class APIWorker(QThread):
                finished = Signal(bool, dict, float)
                progress_update = Signal(str)

                def __init__(self, prompt, is_new, session_id, quality_score, max_iterations, temperature):
                    super().__init__()
                    self.prompt = prompt
                    self.is_new = is_new
                    self.session_id = session_id
                    self.quality_score = quality_score
                    self.max_iterations = max_iterations
                    self.temperature = temperature

                def run(self):
                    start_time = time.monotonic()
                    try:
                        api_url = config.get('api_url', DEFAULT_API_URL).rstrip('/')
                        endpoint = f"{api_url}/generate-cad"

                        payload = {
                            "prompt": self.prompt,
                            "new_session": self.is_new,
                            "model_name": config.get("model_name", MODEL),
                            "min_quality_score": self.quality_score,
                            "max_iterations": self.max_iterations,
                            "temperature": self.temperature
                        }
                        if not self.is_new and self.session_id:
                            payload["session_id"] = self.session_id

                        headers = {"Content-Type": "application/json", "ngrok-skip-browser-warning": "true"}
                        self.progress_update.emit("Waiting for API response (this may take several minutes)...")

                        response = requests.post(endpoint, json=payload, headers=headers, timeout=API_TIMEOUT)
                        generation_time = time.monotonic() - start_time
                        response.raise_for_status()

                        data = response.json()
                        if data.get("success") and data.get("freecad_script"):
                            self.finished.emit(True, data, generation_time)
                        else:
                            error_msg = data.get("message") or data.get("error_summary", "API returned failure.")
                            self.finished.emit(False, {"error_summary": error_msg}, generation_time)

                    except requests.exceptions.Timeout:
                        self.finished.emit(False, {"error_summary": "Request timed out. Server took too long."},
                                           time.monotonic() - start_time)
                    except requests.exceptions.RequestException as e:
                        self.finished.emit(False, {"error_summary": f"Network error: {e}"},
                                           time.monotonic() - start_time)
                    except Exception as e:
                        self.finished.emit(False, {"error_summary": f"Unexpected error: {e}"},
                                           time.monotonic() - start_time)

            dialog = ConversationalDialog()
            dialog.exec_()

        except ImportError as e:
            App.Console.PrintError(f"PySide2 is required for ArchGen dialog: {e}\n")
        except Exception as e:
            App.Console.PrintError(f"Failed to launch ArchGen dialog: {e}\n")
            import traceback
            traceback.print_exc()

    def IsActive(self):
        return True


# --- Settings Command ---
class ArchGen_Settings_Command:
    def GetResources(self):
        return {
            "Pixmap": "",
            "MenuText": "⚙️ ArchGen Settings",
            "ToolTip": "Configure ArchGen AI settings and API connection"
        }

    def Activated(self):
        try:
            from PySide2.QtWidgets import (QDialog, QVBoxLayout, QHBoxLayout, QLabel,
                                           QLineEdit, QPushButton, QGroupBox, QFormLayout,
                                           QComboBox, QSpinBox, QDoubleSpinBox)

            class SettingsDialog(QDialog):
                def __init__(self):
                    super().__init__()
                    self.setWindowTitle("ArchGen AI Settings")
                    self.setMinimumSize(500, 400)

                    layout = QVBoxLayout(self)

                    # API Settings Group
                    api_group = QGroupBox("API Configuration")
                    api_layout = QFormLayout()

                    self.api_url_input = QLineEdit(config.get("api_url", DEFAULT_API_URL))
                    api_layout.addRow("API URL:", self.api_url_input)

                    self.model_combo = QComboBox()
                    self.model_combo.addItems([
                        "qwen/qwen3-coder:free",
                        "deepseek/deepseek-chat-v3.1:free",
                        "meta-llama/llama-3.1-8b-instruct:free"
                    ])
                    current_model = config.get("model_name", MODEL)
                    index = self.model_combo.findText(current_model)
                    if index >= 0:
                        self.model_combo.setCurrentIndex(index)
                    api_layout.addRow("Model:", self.model_combo)

                    api_group.setLayout(api_layout)
                    layout.addWidget(api_group)

                    # Generation Settings Group
                    gen_group = QGroupBox("Default Generation Settings")
                    gen_layout = QFormLayout()

                    self.quality_spin = QDoubleSpinBox()
                    self.quality_spin.setRange(0.1, 1.0)
                    self.quality_spin.setSingleStep(0.1)
                    self.quality_spin.setValue(config.get("quality_threshold", 0.7))
                    gen_layout.addRow("Quality Threshold:", self.quality_spin)

                    self.iter_spin = QSpinBox()
                    self.iter_spin.setRange(1, 10)
                    self.iter_spin.setValue(config.get("max_iterations", 5))
                    gen_layout.addRow("Max Iterations:", self.iter_spin)

                    self.temp_spin = QDoubleSpinBox()
                    self.temp_spin.setRange(0.0, 1.0)
                    self.temp_spin.setSingleStep(0.1)
                    self.temp_spin.setValue(config.get("temperature", 0.1))
                    gen_layout.addRow("Temperature:", self.temp_spin)

                    gen_group.setLayout(gen_layout)
                    layout.addWidget(gen_group)

                    # Info Label
                    info_label = QLabel(f"Configuration file:\n{CONFIG_FILE}")
                    info_label.setStyleSheet("color: gray; font-size: 9pt;")
                    layout.addWidget(info_label)

                    # Buttons
                    button_layout = QHBoxLayout()
                    save_btn = QPushButton("Save")
                    save_btn.clicked.connect(self.save_settings)
                    cancel_btn = QPushButton("Cancel")
                    cancel_btn.clicked.connect(self.reject)

                    button_layout.addStretch()
                    button_layout.addWidget(cancel_btn)
                    button_layout.addWidget(save_btn)
                    layout.addLayout(button_layout)

                def save_settings(self):
                    config.set("api_url", self.api_url_input.text())
                    config.set("model_name", self.model_combo.currentText())
                    config.set("quality_threshold", self.quality_spin.value())
                    config.set("max_iterations", self.iter_spin.value())
                    config.set("temperature", self.temp_spin.value())

                    App.Console.PrintMessage("ArchGen AI settings saved successfully\n")
                    self.accept()

            dialog = SettingsDialog()
            dialog.exec_()

        except Exception as e:
            App.Console.PrintError(f"Failed to open settings dialog: {e}\n")

    def IsActive(self):
        return True


# --- About Command ---
class ArchGen_About_Command:
    def GetResources(self):
        return {
            "Pixmap": "",
            "MenuText": "ℹ️ About ArchGen AI",
            "ToolTip": "About ArchGen AI workbench"
        }

    def Activated(self):
        try:
            from PySide2.QtWidgets import QMessageBox
            QMessageBox.about(
                None,
                "About ArchGen AI",
                "<h2>ArchGen AI - v3.0</h2>"
                "<p>AI-Powered CAD Generation for ArchGen CAD</p>"
                "<p><b>Features:</b></p>"
                "<ul>"
                "<li>Text-to-CAD generation using advanced AI models</li>"
                "<li>Conversational interface with session management</li>"
                "<li>Adjustable quality thresholds and iteration limits</li>"
                "<li>Multi-agent workflow with validation</li>"
                "<li>RAG-enhanced generation for better results</li>"
                "</ul>"
                "<p><b>Powered by:</b> LangChain, OpenRouter API, FreeCAD</p>"
                "<p>© 2024 ArchGen Team</p>"
            )
        except Exception as e:
            App.Console.PrintError(f"Failed to show about dialog: {e}\n")

    def IsActive(self):
        return True


# Register commands (will be called from ArchGen_Global.py)
if __name__ != "__main__":
    try:
        Gui.addCommand('ArchGen_Generate', ArchGen_Generate_Command())
        Gui.addCommand('ArchGen_Settings', ArchGen_Settings_Command())
        Gui.addCommand('ArchGen_About', ArchGen_About_Command())
        App.Console.PrintMessage("✅ ArchGen commands registered successfully\n")
    except Exception as e:
        App.Console.PrintError(f"❌ Failed to register ArchGen commands: {e}\n")
