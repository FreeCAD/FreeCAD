# SPDX-License-Identifier: LGPL-2.1-or-later

"""The cad-x assistant dock panel.

A deliberately plain chat surface: header (service + model status),
transcript, composer.  All conversation logic lives in :mod:`CadXSession`;
this module only renders state and marshals session events onto the Qt
event loop.
"""

from __future__ import annotations

import threading

from PySide import QtCore, QtWidgets

from CadXChatClient import TurnEvent, TurnKind
from CadXConfig import set_configured_model
from CadXSession import ChatSession, Listener


class _EventBridge(QtCore.QObject):
    """Marshal worker-thread results onto the GUI thread via queued signals."""

    eventReceived = QtCore.Signal(object)
    statusChanged = QtCore.Signal()
    modelsLoaded = QtCore.Signal(object)


class AssistantPanel(QtWidgets.QWidget):
    """Chat widget bound to one :class:`ChatSession` and one Ollama server."""

    def __init__(self, session: ChatSession, client, parent=None) -> None:
        super().__init__(parent)
        self._session = session
        self._client = client
        self._listener: Listener = self._on_session_event
        self._bridge = _EventBridge()
        self._bridge.eventReceived.connect(self._render_event)
        self._bridge.statusChanged.connect(self._refresh_status)
        self._bridge.modelsLoaded.connect(self._populate_models)
        self._display_items = [
            (message.role, message.text) for message in session.messages
        ]
        self._streaming_index: int | None = None
        self._user_bubbles: list[QtWidgets.QLabel] = []
        self._closed = False

        self._build_ui()
        self._render_transcript()
        self._session.add_listener(self._listener)
        self._refresh_status()
        QtCore.QTimer.singleShot(0, self._load_models_async)

    # -- construction ----------------------------------------------------------

    def _build_ui(self) -> None:
        layout = QtWidgets.QVBoxLayout(self)
        layout.setContentsMargins(8, 8, 8, 8)

        header = QtWidgets.QHBoxLayout()
        title = QtWidgets.QLabel("cad-x Assistant", self)
        title.setStyleSheet("font-weight: bold;")
        header.addWidget(title)
        header.addStretch(1)
        layout.addLayout(header)

        self._transcript = QtWidgets.QScrollArea(self)
        self._transcript.setObjectName("CadXTranscript")
        self._transcript.setWidgetResizable(True)
        self._transcript.setFrameShape(QtWidgets.QFrame.NoFrame)
        self._transcript.setHorizontalScrollBarPolicy(
            QtCore.Qt.ScrollBarAlwaysOff
        )
        self._transcript_content = QtWidgets.QWidget(self._transcript)
        self._transcript_layout = QtWidgets.QVBoxLayout(self._transcript_content)
        self._transcript_layout.setContentsMargins(4, 4, 4, 4)
        self._transcript_layout.setSpacing(6)
        self._transcript.setWidget(self._transcript_content)
        layout.addWidget(self._transcript, 1)

        self._composer_panel = QtWidgets.QFrame(self)
        self._composer_panel.setObjectName("CadXComposerPanel")
        self._composer_panel.setFixedHeight(96)
        self._composer_panel.setStyleSheet(
            """
            QFrame#CadXComposerPanel {
                background-color: #25272b;
                border: 1px solid #4b4f58;
                border-radius: 18px;
            }
            QPlainTextEdit#CadXComposer {
                background: transparent;
                border: none;
                color: #f1f5f9;
                padding: 4px 8px;
            }
            QComboBox#CadXModelSelector {
                background: transparent;
                border: none;
                color: #c5c9d1;
                padding: 2px 4px;
            }
            QComboBox#CadXModelSelector::drop-down {
                border: none;
                width: 18px;
            }
            QToolButton#CadXSendButton {
                background-color: #e5e7eb;
                border: none;
                border-radius: 17px;
                color: #111827;
                font-size: 20px;
                font-weight: bold;
                padding: 0 0 3px 0;
            }
            QToolButton#CadXSendButton:hover {
                background-color: #ffffff;
            }
            QToolButton#CadXSendButton:disabled {
                background-color: #6b7280;
                color: #d1d5db;
            }
            """
        )
        composer_layout = QtWidgets.QVBoxLayout(self._composer_panel)
        composer_layout.setContentsMargins(8, 6, 8, 6)
        composer_layout.setSpacing(2)

        self._composer = QtWidgets.QPlainTextEdit(self._composer_panel)
        self._composer.setObjectName("CadXComposer")
        self._composer.setPlaceholderText(
            "Type a message. Press Return to send, Shift+Return for a new line."
        )
        self._composer.setFrameStyle(QtWidgets.QFrame.NoFrame)
        self._composer.setFixedHeight(42)
        composer_layout.addWidget(self._composer, 1)

        composer_footer = QtWidgets.QHBoxLayout()
        composer_footer.setContentsMargins(0, 0, 0, 0)
        composer_footer.addStretch(1)
        self._model_combo = QtWidgets.QComboBox(self._composer_panel)
        self._model_combo.setObjectName("CadXModelSelector")
        self._model_combo.setAccessibleName("Local Ollama model")
        self._model_combo.setToolTip("Select a model installed in local Ollama")
        self._model_combo.setMinimumWidth(130)
        self._model_combo.addItem("Loading models…")
        self._model_combo.setEnabled(False)
        self._model_combo.currentTextChanged.connect(self._on_model_selected)
        composer_footer.addWidget(self._model_combo, 0, QtCore.Qt.AlignVCenter)

        self._send_button = QtWidgets.QToolButton(self._composer_panel)
        self._send_button.setObjectName("CadXSendButton")
        self._send_button.setText("↑")
        self._send_button.setAccessibleName("Send")
        self._send_button.setToolTip("Send message")
        self._send_button.setFixedSize(34, 34)
        self._send_button.clicked.connect(self._on_send_clicked)
        composer_footer.addWidget(self._send_button, 0, QtCore.Qt.AlignVCenter)
        composer_layout.addLayout(composer_footer)
        layout.addWidget(self._composer_panel)

        self._composer.installEventFilter(self)

    def eventFilter(self, watched, event) -> bool:  # noqa: N802 - Qt API
        if (
            watched is self._composer
            and event.type() == QtCore.QEvent.KeyPress
            and event.key() in (QtCore.Qt.Key_Return, QtCore.Qt.Key_Enter)
        ):
            if event.modifiers() & QtCore.Qt.ShiftModifier:
                return False
            self._on_send_clicked()
            return True
        return super().eventFilter(watched, event)

    # -- status ------------------------------------------------------------------

    def _refresh_status(self) -> None:
        model = self._client.model
        index = self._model_combo.findText(model)
        if index >= 0 and index != self._model_combo.currentIndex():
            self._model_combo.blockSignals(True)
            self._model_combo.setCurrentIndex(index)
            self._model_combo.blockSignals(False)

    def _load_models_async(self) -> None:
        threading.Thread(
            target=self._query_models,
            name="cad-x-ollama-models",
            daemon=True,
        ).start()

    def _query_models(self) -> None:
        try:
            models = self._client.list_models()
        except Exception:
            models = ()
        if not self._closed:
            self._bridge.modelsLoaded.emit(models)

    def _populate_models(self, models) -> None:
        names = list(models or ())
        current = self._client.model
        if not names:
            self._model_combo.clear()
            self._model_combo.addItem("No local models found")
            self._model_combo.setEnabled(False)
            self._model_combo.setToolTip(
                "Ollama is unavailable or has no installed models"
            )
            return
        if current not in names:
            current = names[0]
            self._client.set_model(current)
        self._model_combo.blockSignals(True)
        self._model_combo.clear()
        self._model_combo.addItems(names)
        self._model_combo.setCurrentText(current)
        self._model_combo.blockSignals(False)
        self._model_combo.setEnabled(True)
        self._model_combo.setToolTip("Select a model installed in local Ollama")

    def _on_model_selected(self, model: str) -> None:
        if (
            not self._model_combo.isEnabled()
            or self._session.busy
            or not model.strip()
        ):
            return
        if model == self._client.model:
            return
        self._client.set_model(model)
        set_configured_model(model)
        self._refresh_status()

    # -- sending ---------------------------------------------------------------

    def _on_send_clicked(self) -> None:
        if self._session.busy:
            self._session.cancel()
            return
        text = self._composer.toPlainText().strip()
        if not text:
            return
        accepted = self._session.send(text)
        if not accepted:
            return
        self._composer.clear()
        self._append_message("user", text)
        self._model_combo.setEnabled(False)
        self._update_send_button()

    # -- session events -----------------------------------------------------------

    def _on_session_event(self, event: TurnEvent) -> None:
        # Worker thread context: hop to the GUI thread.
        self._bridge.eventReceived.emit(event)

    def _render_event(self, event: TurnEvent) -> None:
        if event.kind == TurnKind.DELTA:
            self._stream_delta(event.text)
        elif event.kind == TurnKind.COMPLETED:
            self._finish_streaming_chunk(event.text)
        elif event.kind == TurnKind.FAILED:
            self._finish_streaming_chunk()
            self._append_system_note(event.message or "The request failed.")
        elif event.kind == TurnKind.CANCELLED:
            self._finish_streaming_chunk()
            self._append_system_note("Turn cancelled.")
        elif event.kind == TurnKind.STARTED:
            self._begin_streaming_if_needed()
            # The turn may have resolved a better model name; show it.
            self._bridge.statusChanged.emit()

    # -- transcript rendering ------------------------------------------------------

    def _begin_streaming_if_needed(self) -> None:
        if self._streaming_index is not None:
            return
        self._display_items.append(("assistant", ""))
        self._streaming_index = len(self._display_items) - 1
        self._render_transcript()
        self._update_send_button()

    def _stream_delta(self, text: str) -> None:
        self._begin_streaming_if_needed()
        index = self._streaming_index
        if index is None:
            return
        role, current = self._display_items[index]
        self._display_items[index] = (role, current + text)
        self._render_transcript()

    def _finish_streaming_chunk(self, completed_text: str = "") -> None:
        index = self._streaming_index
        if index is not None:
            role, current = self._display_items[index]
            if completed_text:
                self._display_items[index] = (role, completed_text)
            elif not current:
                self._display_items[index] = (role, "")
            self._streaming_index = None
        elif completed_text:
            self._display_items.append(("assistant", completed_text))
        self._render_transcript()
        # The terminal event can be queued before ChatSession clears its
        # worker busy flag. There can be no more deltas after this point, so
        # always restore the action label immediately.
        self._send_button.setText("↑")
        self._send_button.setAccessibleName("Send")
        self._send_button.setToolTip("Send message")
        self._enable_model_selector()

    def _append_message(self, role: str, text: str) -> None:
        self._display_items.append((role, text))
        self._render_transcript()

    def _append_system_note(self, message: str) -> None:
        self._display_items.append(("system", message))
        self._render_transcript()

    def _render_transcript(self) -> None:
        while self._transcript_layout.count():
            item = self._transcript_layout.takeAt(0)
            widget = item.widget()
            if widget is not None:
                widget.deleteLater()
        self._user_bubbles.clear()

        if not self._display_items:
            empty = QtWidgets.QLabel(
                "Ask anything about CAD design. Tool execution arrives later.",
                self._transcript_content,
            )
            empty.setObjectName("CadXTranscriptPlaceholder")
            empty.setWordWrap(True)
            empty.setStyleSheet("color: #9ca3af; padding: 8px;")
            self._transcript_layout.addWidget(empty)
            self._transcript_layout.addStretch(1)
        else:
            for role, text in self._display_items:
                self._transcript_layout.addWidget(self._message_row(role, text))
            self._transcript_layout.addStretch(1)
        self._update_user_bubble_widths()
        QtCore.QTimer.singleShot(0, self._scroll_transcript_to_bottom)

    def _message_row(self, role: str, text: str) -> QtWidgets.QWidget:
        row = QtWidgets.QWidget(self._transcript_content)
        row_layout = QtWidgets.QHBoxLayout(row)
        row_layout.setContentsMargins(0, 0, 0, 0)
        row_layout.setSpacing(0)

        if role == "user":
            message = QtWidgets.QLabel(row)
            message.setObjectName("CadXUserBubble")
            message.setTextFormat(QtCore.Qt.PlainText)
            message.setTextInteractionFlags(QtCore.Qt.TextSelectableByMouse)
            message.setWordWrap(True)
            message.setSizePolicy(
                QtWidgets.QSizePolicy.Preferred,
                QtWidgets.QSizePolicy.Maximum,
            )
            message.setStyleSheet(
                "QLabel#CadXUserBubble {"
                " background-color: #2f6fca;"
                " color: #ffffff;"
                " border-radius: 14px;"
                " padding: 8px 12px;"
                "}"
            )
            message.setText(text)
            self._user_bubbles.append(message)
            row_layout.addStretch(1)
            row_layout.addWidget(message, 0, QtCore.Qt.AlignRight)
        elif role == "assistant":
            message = QtWidgets.QLabel(row)
            message.setObjectName("CadXAssistantText")
            message.setTextFormat(QtCore.Qt.PlainText)
            message.setTextInteractionFlags(QtCore.Qt.TextSelectableByMouse)
            message.setWordWrap(True)
            message.setSizePolicy(
                QtWidgets.QSizePolicy.Expanding,
                QtWidgets.QSizePolicy.Preferred,
            )
            message.setStyleSheet(
                "QLabel#CadXAssistantText { color: #f1f5f9; padding: 6px 0; }"
            )
            message.setText(text)
            row_layout.addWidget(message, 1)
        else:
            row_layout.addStretch(1)
            message = QtWidgets.QLabel(row)
            message.setObjectName("CadXSystemNote")
            message.setTextFormat(QtCore.Qt.PlainText)
            message.setStyleSheet(
                "QLabel#CadXSystemNote { color: #9ca3af; padding: 4px; }"
            )
            message.setText(text)
            row_layout.addWidget(message, 0, QtCore.Qt.AlignCenter)
            row_layout.addStretch(1)
        return row

    def _update_user_bubble_widths(self) -> None:
        width = max(1, int(self._transcript.viewport().width() * 0.75))
        for bubble in self._user_bubbles:
            bubble.setMaximumWidth(width)

    def _scroll_transcript_to_bottom(self) -> None:
        scrollbar = self._transcript.verticalScrollBar()
        scrollbar.setValue(scrollbar.maximum())

    def resizeEvent(self, event) -> None:  # noqa: N802 - Qt API
        super().resizeEvent(event)
        self._update_user_bubble_widths()

    def _update_send_button(self) -> None:
        if self._session.busy:
            self._send_button.setText("×")
            self._send_button.setAccessibleName("Stop")
            self._send_button.setToolTip("Stop response")
        else:
            self._send_button.setText("↑")
            self._send_button.setAccessibleName("Send")
            self._send_button.setToolTip("Send message")

    def _enable_model_selector(self) -> None:
        if self._model_combo.currentText() != "No local models found":
            self._model_combo.setEnabled(True)

    # -- teardown -------------------------------------------------------------------

    def shutdown(self) -> None:
        self._closed = True
        self._session.remove_listener(self._listener)
