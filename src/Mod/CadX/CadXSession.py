# SPDX-License-Identifier: LGPL-2.1-or-later

"""Conversation state for the cad-x assistant panel.

``ChatSession`` is pure Python on purpose: it owns the transcript and the
background turn thread, but knows nothing about Qt or FreeCAD.  The panel
subscribes as a listener and marshals events onto the GUI thread itself.
"""

from __future__ import annotations

from collections.abc import Callable
import threading

from CadXChatClient import ChatClientError, ChatMessage, TurnEvent, TurnKind


Listener = Callable[[TurnEvent], None]
CancelCheck = Callable[[], bool]


class ChatSession:
    """One in-memory conversation driving at most one background turn."""

    def __init__(self, client) -> None:
        self._client = client
        self._messages: list[ChatMessage] = []
        self._listeners: list[Listener] = []
        self._state_lock = threading.RLock()
        self._cancel_event = threading.Event()
        self._busy = False

    # -- transcript ------------------------------------------------------------

    @property
    def client(self):
        """The chat client this session drives (exposed for status display)."""

        return self._client

    @property
    def messages(self) -> tuple[ChatMessage, ...]:
        with self._state_lock:
            return tuple(self._messages)

    @property
    def busy(self) -> bool:
        return self._busy

    # -- listeners ----------------------------------------------------------------

    def add_listener(self, listener: Listener) -> None:
        self._listeners.append(listener)

    def remove_listener(self, listener: Listener) -> None:
        if listener in self._listeners:
            self._listeners.remove(listener)

    # -- turns ---------------------------------------------------------------------

    def send(self, text: str) -> bool:
        """Append one user message and start its assistant turn.

        Returns ``False`` when the message was rejected (empty input or a
        turn already running); no event is emitted in that case.
        """

        clean = str(text or "").strip()
        if not clean:
            return False
        with self._state_lock:
            if self._busy:
                return False
            self._busy = True
            self._cancel_event.clear()
            self._messages.append(ChatMessage("user", clean))
        threading.Thread(
            target=self._run_turn,
            name="cad-x-chat-turn",
            daemon=True,
        ).start()
        return True

    def cancel(self) -> bool:
        """Request cancellation of the running turn."""

        if not self._busy:
            return False
        self._cancel_event.set()
        return True

    # -- internals ---------------------------------------------------------------

    def _run_turn(self) -> None:
        history = list(self.messages)
        try:
            self._client.stream_turn(
                history,
                on_event=self._emit,
                should_cancel=self._cancel_event.is_set,
            )
        except ChatClientError as exc:  # defensive: stream_turn reports failures
            self._emit(TurnEvent(TurnKind.FAILED, message=str(exc)))
        finally:
            with self._state_lock:
                self._busy = False

    def _emit(self, event: TurnEvent) -> None:
        if event.kind == TurnKind.COMPLETED and event.text:
            with self._state_lock:
                self._messages.append(ChatMessage("assistant", event.text))
        elif event.kind == TurnKind.TOOL_CALL and event.tool_call is not None:
            with self._state_lock:
                self._messages.append(ChatMessage.assistant_tool_call(event.tool_call))
        elif event.kind == TurnKind.TOOL_RESULT and event.tool_result is not None:
            with self._state_lock:
                self._messages.append(ChatMessage.tool_result(event.tool_result))
        for listener in list(self._listeners):
            listener(event)
