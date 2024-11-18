# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2016 sliptonic <shopinthewoods@gmail.com>               *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

import Path
import unittest


class TestPathLog(unittest.TestCase):
    """Some basic tests for the logging framework."""

    MODULE = "TestPathLog"  # file name without extension

    def setUp(self):
        Path.Log.setLevel(Path.Log.Level.RESET)
        Path.Log.untrackAllModules()

    def callerFile(self):
        return Path.Log._caller()[0]

    def callerLine(self):
        return Path.Log._caller()[1]

    def callerFunc(self):
        return Path.Log._caller()[2]

    def test00(self):
        """Check for proper module extraction."""
        self.assertEqual(self.callerFile(), self.MODULE)

    def test01(self):
        """Check for proper function extraction."""
        self.assertEqual(self.callerFunc(), "test01")

    def test10(self):
        """Verify default log levels is NOTICE."""
        self.assertEqual(Path.Log.getLevel(), Path.Log.Level.NOTICE)
        self.assertEqual(Path.Log.getLevel(self.MODULE), Path.Log.Level.NOTICE)

    def test11(self):
        """Verify setting global log level."""
        Path.Log.setLevel(Path.Log.Level.DEBUG)

        self.assertEqual(Path.Log.getLevel(), Path.Log.Level.DEBUG)
        self.assertEqual(Path.Log.getLevel(self.MODULE), Path.Log.Level.DEBUG)

    def test12(self):
        """Verify setting module log level."""
        Path.Log.setLevel(Path.Log.Level.DEBUG, self.MODULE)

        self.assertEqual(Path.Log.getLevel(), Path.Log.Level.NOTICE)
        self.assertEqual(Path.Log.getLevel(self.MODULE), Path.Log.Level.DEBUG)

    def test13(self):
        """Verify setting other modul's log level doesn't change this one's."""
        # if this test fails then most likely the global RESET is broken
        Path.Log.setLevel(Path.Log.Level.DEBUG, "SomeOtherModule")

        self.assertEqual(Path.Log.getLevel(), Path.Log.Level.NOTICE)
        self.assertEqual(Path.Log.getLevel(self.MODULE), Path.Log.Level.NOTICE)

    def test14(self):
        """Verify resetting log level for module falls back to global level."""
        Path.Log.setLevel(Path.Log.Level.DEBUG, self.MODULE)
        self.assertEqual(Path.Log.getLevel(self.MODULE), Path.Log.Level.DEBUG)
        # changing global log level does not affect module
        Path.Log.setLevel(Path.Log.Level.ERROR)
        self.assertEqual(Path.Log.getLevel(self.MODULE), Path.Log.Level.DEBUG)
        # resetting module log level restores global log level for module
        Path.Log.setLevel(Path.Log.Level.RESET, self.MODULE)
        self.assertEqual(Path.Log.getLevel(self.MODULE), Path.Log.Level.ERROR)
        # changing the global log level will also change the module log level
        Path.Log.setLevel(Path.Log.Level.DEBUG)
        self.assertEqual(Path.Log.getLevel(self.MODULE), Path.Log.Level.DEBUG)

    def test20(self):
        """Verify debug logs aren't logged by default."""
        self.assertIsNone(Path.Log.debug("this"))

    def test21(self):
        """Verify debug logs are logged if log level is set to DEBUG."""
        Path.Log.setLevel(Path.Log.Level.DEBUG)
        self.assertIsNotNone(Path.Log.debug("this"))

    def test30(self):
        """Verify log level ERROR."""
        Path.Log.setLevel(Path.Log.Level.ERROR)
        self.assertIsNone(Path.Log.debug("something"))
        self.assertIsNone(Path.Log.info("something"))
        self.assertIsNone(Path.Log.notice("something"))
        self.assertIsNone(Path.Log.warning("something"))
        self.assertIsNotNone(Path.Log.error("something"))

    def test31(self):
        """Verify log level WARNING."""
        Path.Log.setLevel(Path.Log.Level.WARNING)
        self.assertIsNone(Path.Log.debug("something"))
        self.assertIsNone(Path.Log.info("something"))
        self.assertIsNone(Path.Log.notice("something"))
        self.assertIsNotNone(Path.Log.warning("something"))
        self.assertIsNotNone(Path.Log.error("something"))

    def test32(self):
        """Verify log level NOTICE."""
        Path.Log.setLevel(Path.Log.Level.NOTICE)
        self.assertIsNone(Path.Log.debug("something"))
        self.assertIsNone(Path.Log.info("something"))
        self.assertIsNotNone(Path.Log.notice("something"))
        self.assertIsNotNone(Path.Log.warning("something"))
        self.assertIsNotNone(Path.Log.error("something"))

    def test33(self):
        """Verify log level INFO."""
        Path.Log.setLevel(Path.Log.Level.INFO)
        self.assertIsNone(Path.Log.debug("something"))
        self.assertIsNotNone(Path.Log.info("something"))
        self.assertIsNotNone(Path.Log.notice("something"))
        self.assertIsNotNone(Path.Log.warning("something"))
        self.assertIsNotNone(Path.Log.error("something"))

    def test34(self):
        """Verify log level DEBUG."""
        Path.Log.setLevel(Path.Log.Level.DEBUG)
        self.assertIsNotNone(Path.Log.debug("something"))
        self.assertIsNotNone(Path.Log.info("something"))
        self.assertIsNotNone(Path.Log.notice("something"))
        self.assertIsNotNone(Path.Log.warning("something"))
        self.assertIsNotNone(Path.Log.error("something"))

    def test50(self):
        """Verify no tracking by default."""
        self.assertIsNone(Path.Log.track("this", "and", "that"))

    def test51(self):
        """Verify enabling tracking for module results in tracking."""
        Path.Log.trackModule()
        # Don't want to rely on the line number matching - still want some
        # indication that track does the right thing ....
        msg = Path.Log.track("this", "and", "that")
        self.assertTrue(msg.startswith(self.MODULE))
        self.assertTrue(msg.endswith("test51(this, and, that)"))

    def test52(self):
        """Verify untracking stops tracking."""
        Path.Log.trackModule()
        self.assertIsNotNone(Path.Log.track("this", "and", "that"))
        Path.Log.untrackModule()
        self.assertIsNone(Path.Log.track("this", "and", "that"))

    def test53(self):
        """Verify trackAllModules works correctly."""
        Path.Log.trackAllModules(True)
        self.assertIsNotNone(Path.Log.track("this", "and", "that"))
        Path.Log.trackAllModules(False)
        self.assertIsNone(Path.Log.track("this", "and", "that"))
        Path.Log.trackAllModules(True)
        Path.Log.trackModule()
        self.assertIsNotNone(Path.Log.track("this", "and", "that"))
        Path.Log.trackAllModules(False)
        self.assertIsNotNone(Path.Log.track("this", "and", "that"))

    def test60(self):
        """Verify track handles no argument."""
        Path.Log.trackModule()
        msg = Path.Log.track()
        self.assertTrue(msg.startswith(self.MODULE))
        self.assertTrue(msg.endswith("test60()"))

    def test61(self):
        """Verify track handles arbitrary argument types correctly."""
        Path.Log.trackModule()
        msg = Path.Log.track("this", None, 1, 18.25)
        self.assertTrue(msg.startswith(self.MODULE))
        self.assertTrue(msg.endswith("test61(this, None, 1, 18.25)"))

    def testzz(self):
        """Restoring environment after tests."""
        Path.Log.setLevel(Path.Log.Level.RESET)
