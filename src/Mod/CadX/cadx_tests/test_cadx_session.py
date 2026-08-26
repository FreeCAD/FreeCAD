# SPDX-License-Identifier: LGPL-2.1-or-later

"""Tests for CadXSession: transcript, threading contract, and events."""

from __future__ import annotations

import sys
import threading
import time
import unittest
from pathlib import Path


sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from CadXChatClient import TurnEvent, TurnKind  # noqa: E402
from CadXSession import ChatSession  # noqa: E402


def wait_until(predicate, timeout=5.0):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if predicate():
            return True
        time.sleep(0.01)
    return predicate()


class ScriptedClient:
    """Replays a scripted event sequence inside stream_turn."""

    def __init__(self, script=None):
        self.script = list(script or [])
        self.turns: list[list] = []
        self.started = threading.Event()
        self.finished = threading.Event()
        self.cancel_check = None

    def stream_turn(self, history, on_event, should_cancel):
        self.turns.append(list(history))
        self.cancel_check = should_cancel
        self.started.set()
        for event in self.script:
            on_event(event)
        self.finished.set()


class RecordingListener:
    def __init__(self):
        self.events: list[TurnEvent] = []
        self.seen_completion = threading.Event()

    def __call__(self, event: TurnEvent) -> None:
        self.events.append(event)
        if event.kind in (TurnKind.COMPLETED, TurnKind.FAILED, TurnKind.CANCELLED):
            self.seen_completion.set()

    def wait(self, timeout=5.0):
        return self.seen_completion.wait(timeout)


def _delta(text):
    return TurnEvent(TurnKind.DELTA, text=text)


class ChatSessionTest(unittest.TestCase):
    def test_send_appends_user_message_and_completes_turn(self):
        client = ScriptedClient(
            [
                TurnEvent(TurnKind.STARTED),
                _delta("Hel"),
                _delta("lo"),
                TurnEvent(TurnKind.COMPLETED, text="Hello"),
            ]
        )
        session = ChatSession(client)
        listener = RecordingListener()
        session.add_listener(listener)

        self.assertTrue(session.send("hi there"))
        self.assertTrue(listener.wait())

        roles = [(message.role, message.text) for message in session.messages]
        self.assertEqual(roles, [("user", "hi there"), ("assistant", "Hello")])
        self.assertEqual(
            [event.kind for event in listener.events],
            [
                TurnKind.STARTED,
                TurnKind.DELTA,
                TurnKind.DELTA,
                TurnKind.COMPLETED,
            ],
        )
        self.assertFalse(session.busy)
        # The turn saw the user message but not its own answer.
        self.assertEqual([m.role for m in client.turns[0]], ["user"])

    def test_empty_message_rejected_without_events(self):
        client = ScriptedClient()
        session = ChatSession(client)
        listener = RecordingListener()
        session.add_listener(listener)

        self.assertFalse(session.send("   "))

        self.assertEqual(session.messages, ())
        self.assertEqual(listener.events, [])
        self.assertFalse(client.started.is_set())

    def test_concurrent_send_rejected_until_turn_finishes(self):
        release = threading.Event()

        class GatedClient(ScriptedClient):
            def stream_turn(self, history, on_event, should_cancel):
                super().stream_turn(history, on_event, should_cancel)
                # Stay "running" after the completion event was emitted, so
                # the test observes busy while the worker is still in flight.
                release.wait(5.0)

        client = GatedClient([TurnEvent(TurnKind.COMPLETED, text="done")])
        session = ChatSession(client)
        listener = RecordingListener()
        session.add_listener(listener)

        self.assertTrue(session.send("first"))
        self.assertTrue(listener.wait())  # completion emitted...
        self.assertTrue(session.busy)  # ...but the turn is still running.
        self.assertFalse(session.send("second"))  # concurrent send rejected

        release.set()
        self.assertTrue(wait_until(lambda: not session.busy))
        self.assertTrue(session.send("second"))
        # The released client finishes instantly; wait for the final transcript.
        self.assertTrue(wait_until(lambda: len(session.messages) == 4))
        roles = [(message.role, message.text) for message in session.messages]
        self.assertEqual(
            roles,
            [
                ("user", "first"),
                ("assistant", "done"),
                ("user", "second"),
                ("assistant", "done"),
            ],
        )

    def test_failed_turn_keeps_user_message_only(self):
        client = ScriptedClient([TurnEvent(TurnKind.FAILED, message="boom")])
        session = ChatSession(client)
        listener = RecordingListener()
        session.add_listener(listener)

        self.assertTrue(session.send("hi"))
        self.assertTrue(listener.wait())

        roles = [(message.role, message.text) for message in session.messages]
        self.assertEqual(roles, [("user", "hi")])
        self.assertEqual(listener.events[-1].message, "boom")

    def test_cancel_requests_stop_and_reports_cancelled(self):
        cancel_observed = threading.Event()

        class CancellingClient(ScriptedClient):
            def stream_turn(self, history, on_event, should_cancel):
                super().stream_turn(history, on_event, should_cancel)
                while not should_cancel():
                    if cancel_observed.wait(0.05):
                        break
                if should_cancel():
                    on_event(TurnEvent(TurnKind.CANCELLED))

        client = CancellingClient()
        session = ChatSession(client)
        listener = RecordingListener()
        session.add_listener(listener)

        self.assertFalse(session.cancel())  # nothing running yet
        self.assertTrue(session.send("long story"))
        self.assertTrue(client.started.wait(5.0))
        self.assertTrue(session.cancel())
        self.assertTrue(listener.wait())

        self.assertEqual(listener.events[-1].kind, TurnKind.CANCELLED)
        roles = [(message.role,) for message in session.messages]
        self.assertEqual(roles, [("user",)])

    def test_remove_listener_stops_delivery(self):
        client = ScriptedClient([TurnEvent(TurnKind.COMPLETED, text="done")])
        session = ChatSession(client)
        listener = RecordingListener()
        session.add_listener(listener)
        session.remove_listener(listener)

        session.send("hi")
        client.finished.wait(5.0)

        self.assertEqual(listener.events, [])


if __name__ == "__main__":
    unittest.main()
