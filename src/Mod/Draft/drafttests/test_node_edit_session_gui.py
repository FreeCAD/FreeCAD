# SPDX-License-Identifier: LGPL-2.1-or-later

"""GUI resource lifecycle tests for the viewport node-editing session."""

from types import SimpleNamespace
import unittest
from unittest import mock

import FreeCAD as App
from pivy import coin
from draftguitools import gui_node_edit_session


class DraftGuiNodeEditSession(unittest.TestCase):

    def setUp(self):
        self.obj = SimpleNamespace(Name="NodeEditTarget")
        self.tools = mock.Mock()
        self.tools.get_edit_points.return_value = [App.Vector(), App.Vector(1000, 0, 0)]
        self.view = mock.Mock()
        self.wp = mock.Mock()
        self.snapper = mock.Mock()
        self.trackers = [mock.Mock(), mock.Mock()]

        for target, attribute, value in (
            (gui_node_edit_session.gui_utils, "get_3d_view", self.view),
            (gui_node_edit_session.WorkingPlane, "get_working_plane", self.wp),
            (gui_node_edit_session.gui_snapper, "get_snapper", self.snapper),
        ):
            patcher = mock.patch.object(target, attribute, return_value=value)
            patcher.start()
            self.addCleanup(patcher.stop)

        patcher = mock.patch.object(
            gui_node_edit_session.trackers, "editTracker", side_effect=self.trackers
        )
        self.create_tracker = patcher.start()
        self.addCleanup(patcher.stop)
        self.session = gui_node_edit_session.NodeEditSession([(self.obj, self.tools)])
        self.addCleanup(self.session.stop)

    def assert_session_stopped(self):
        self.assertFalse(self.session.is_running())
        self.assertIsNone(self.session._view)
        self.assertEqual(self.session._trackers, {})
        self.assertEqual(self.session._objs_formats, {})
        self.session.stop()
        self.snapper.off.assert_called_once_with()
        self.wp._restore.assert_called_once_with()

    def test_snapper_start_failure_restores_working_plane(self):
        self.snapper.setTrackers.side_effect = RuntimeError("Snapper setup failed")

        with self.assertRaisesRegex(RuntimeError, "Snapper setup failed"):
            self.session.start()

        self.assert_session_stopped()
        self.create_tracker.assert_not_called()
        self.view.addEventCallbackPivy.assert_not_called()

    def test_tracker_start_failure_removes_preceding_trackers(self):
        self.create_tracker.side_effect = [self.trackers[0], RuntimeError("Tracker failed")]

        with self.assertRaisesRegex(RuntimeError, "Tracker failed"):
            self.session.start()

        self.assert_session_stopped()
        self.trackers[0].finalize.assert_called_once_with()
        self.tools.restore_object_style.assert_called_once_with(
            self.obj, self.tools.get_object_style.return_value
        )
        self.view.addEventCallbackPivy.assert_not_called()

    def test_callback_start_failure_releases_registered_callbacks(self):
        keyboard_callback = object()
        location_callback = object()
        self.view.addEventCallbackPivy.side_effect = [
            keyboard_callback,
            location_callback,
            RuntimeError("Callback registration failed"),
        ]

        with self.assertRaisesRegex(RuntimeError, "Callback registration failed"):
            self.session.start()

        self.assert_session_stopped()
        for tracker in self.trackers:
            tracker.finalize.assert_called_once_with()
        self.tools.restore_object_style.assert_called_once_with(
            self.obj, self.tools.get_object_style.return_value
        )
        self.assertEqual(
            self.view.removeEventCallbackSWIG.call_args_list,
            [
                mock.call(coin.SoKeyboardEvent.getClassTypeId(), keyboard_callback),
                mock.call(coin.SoLocation2Event.getClassTypeId(), location_callback),
            ],
        )
