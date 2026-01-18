from __future__ import annotations

import unittest
from unittest.mock import patch

from tools.devstack.core.jobs import distcc_slots, resolve_jobs


class TestJobs(unittest.TestCase):
    def test_distcc_slots(self) -> None:
        self.assertEqual(distcc_slots("desktop/12"), 12)
        self.assertEqual(distcc_slots("desktop:3632/12 localhost/4"), 16)
        self.assertEqual(distcc_slots("desktop,lzo localhost/4"), 4)
        self.assertEqual(distcc_slots("--randomize desktop/8"), 8)
        self.assertEqual(distcc_slots("desktop --localslots=2 localhost/4"), 4)

    def test_resolve_jobs_precedence(self) -> None:
        env = {"DEVSTACK_JOBS": "7", "DISTCC_HOSTS": "desktop/12"}
        self.assertEqual(resolve_jobs(5, env, distcc_enabled=True), (5, "cli"))
        self.assertEqual(resolve_jobs(None, env, distcc_enabled=True), (7, "env"))
        self.assertEqual(resolve_jobs(None, {"DISTCC_HOSTS": "desktop/12"}, distcc_enabled=True), (12, "distcc"))

        with patch("os.cpu_count", return_value=9):
            self.assertEqual(resolve_jobs(None, {}, distcc_enabled=False), (9, "cpu"))

