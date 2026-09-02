# SPDX-License-Identifier: LGPL-2.1-or-later

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


class TestPathLogNew(unittest.TestCase):
    """Some basic tests for the new logging framework."""

    logger = None

    def setUp(self):
        self.logger = Path.Log.getModuleLogger(withLevel=Path.Log.Level.RESET, enableTracking=False)
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
        self.assertEqual(self.callerFile(), self.logger.getModule())

    def test01(self):
        """Check for proper function extraction."""
        self.assertEqual(self.callerFunc(), "test01")

    def test10(self):
        """Verify default log levels is NOTICE."""
        self.assertEqual(self.logger.getLevel(), Path.Log.Level.NOTICE)
        self.assertEqual(Path.Log.getLevel(self.logger.getModule()), Path.Log.Level.NOTICE)

    def test11(self):
        """Verify setting global log level."""
        Path.Log.setLevel(Path.Log.Level.DEBUG)

        self.assertEqual(self.logger.getLevel(), Path.Log.Level.DEBUG)
        self.assertEqual(Path.Log.getLevel(self.logger.getModule()), Path.Log.Level.DEBUG)

    def test12(self):
        """Verify setting module log level."""
        self.logger.setLevel(Path.Log.Level.DEBUG)

        self.assertEqual(Path.Log.getLevel(), Path.Log.Level.NOTICE)
        self.assertEqual(self.logger.getLevel(), Path.Log.Level.DEBUG)
        self.assertEqual(Path.Log.getLevel(self.logger.getModule()), Path.Log.Level.DEBUG)

    def test13(self):
        """Verify setting other modul's log level doesn't change this one's."""
        # if this test fails then most likely the global RESET is broken
        Path.Log.setLevel(Path.Log.Level.DEBUG, "SomeOtherModule")

        self.assertEqual(self.logger.getLevel(), Path.Log.Level.NOTICE)
        self.assertEqual(Path.Log.getLevel(self.logger.getModule()), Path.Log.Level.NOTICE)

    def test14(self):
        """Verify resetting log level for module falls back to global level."""
        self.logger.setLevel(Path.Log.Level.DEBUG)
        self.assertEqual(self.logger.getLevel(), Path.Log.Level.DEBUG)
        self.assertEqual(Path.Log.getLevel(self.logger.getModule()), Path.Log.Level.DEBUG)
        # changing global log level does not affect module
        Path.Log.setLevel(Path.Log.Level.ERROR)
        self.assertEqual(self.logger.getLevel(), Path.Log.Level.DEBUG)
        self.assertEqual(Path.Log.getLevel(self.logger.getModule()), Path.Log.Level.DEBUG)
        # resetting module log level restores global log level for module
        self.logger.setLevel(Path.Log.Level.RESET)
        self.assertEqual(self.logger.getLevel(), Path.Log.Level.ERROR)
        self.assertEqual(Path.Log.getLevel(self.logger.getModule()), Path.Log.Level.ERROR)
        # changing the global log level will also change the module log level
        Path.Log.setLevel(Path.Log.Level.DEBUG)
        self.assertEqual(self.logger.getLevel(), Path.Log.Level.DEBUG)
        self.assertEqual(Path.Log.getLevel(self.logger.getModule()), Path.Log.Level.DEBUG)

    def test20(self):
        """Verify debug logs aren't logged by default."""
        self.assertIsNone(self.logger.debug("this"))

    def test21(self):
        """Verify debug logs are logged if log level is set to DEBUG."""
        self.logger.setLevel(Path.Log.Level.DEBUG)
        self.assertIsNotNone(self.logger.debug("this"))

    def test30(self):
        """Verify log level ERROR."""
        self.logger.setLevel(Path.Log.Level.ERROR)
        self.assertIsNone(self.logger.debug("something"))
        self.assertIsNone(self.logger.info("something"))
        self.assertIsNone(self.logger.notice("something"))
        self.assertIsNone(self.logger.warning("something"))
        self.assertIsNotNone(self.logger.error("something"))

    def test31(self):
        """Verify log level WARNING."""
        self.logger.setLevel(Path.Log.Level.WARNING)
        self.assertIsNone(self.logger.debug("something"))
        self.assertIsNone(self.logger.info("something"))
        self.assertIsNone(self.logger.notice("something"))
        self.assertIsNotNone(self.logger.warning("something"))
        self.assertIsNotNone(self.logger.error("something"))

    def test32(self):
        """Verify log level NOTICE."""
        self.logger.setLevel(Path.Log.Level.NOTICE)
        self.assertIsNone(self.logger.debug("something"))
        self.assertIsNone(self.logger.info("something"))
        self.assertIsNotNone(self.logger.notice("something"))
        self.assertIsNotNone(self.logger.warning("something"))
        self.assertIsNotNone(self.logger.error("something"))

    def test33(self):
        """Verify log level INFO."""
        self.logger.setLevel(Path.Log.Level.INFO)
        self.assertIsNone(self.logger.debug("something"))
        self.assertIsNotNone(self.logger.info("something"))
        self.assertIsNotNone(self.logger.notice("something"))
        self.assertIsNotNone(self.logger.warning("something"))
        self.assertIsNotNone(self.logger.error("something"))

    def test34(self):
        """Verify log level DEBUG."""
        self.logger.setLevel(Path.Log.Level.DEBUG)
        self.assertIsNotNone(self.logger.debug("something"))
        self.assertIsNotNone(self.logger.info("something"))
        self.assertIsNotNone(self.logger.notice("something"))
        self.assertIsNotNone(self.logger.warning("something"))
        self.assertIsNotNone(self.logger.error("something"))

    def test50(self):
        """Verify no tracking by default."""
        self.assertIsNone(self.logger.track("this", "and", "that"))

    def test51(self):
        """Verify enabling tracking for module results in tracking."""
        self.logger.enableTracking()
        # Don't want to rely on the line number matching - still want some
        # indication that track does the right thing ....
        msg = self.logger.track("this", "and", "that")
        self.assertTrue(msg.startswith(self.logger.getModule()))
        self.assertTrue(msg.endswith("test51(this, and, that)"))

    def test52(self):
        """Verify untracking stops tracking."""
        self.logger.enableTracking()
        self.assertIsNotNone(self.logger.track("this", "and", "that"))
        self.logger.disableTracking()
        self.assertIsNone(self.logger.track("this", "and", "that"))

    def test53(self):
        """Verify trackAllModules works correctly."""
        Path.Log.trackAllModules(True)
        self.assertIsNotNone(self.logger.track("this", "and", "that"))
        Path.Log.trackAllModules(False)
        self.assertIsNone(self.logger.track("this", "and", "that"))
        Path.Log.trackAllModules(True)
        self.logger.enableTracking()
        self.assertIsNotNone(self.logger.track("this", "and", "that"))
        Path.Log.trackAllModules(False)
        self.assertIsNotNone(self.logger.track("this", "and", "that"))

    def test60(self):
        """Verify track handles no argument."""
        self.logger.enableTracking()
        msg = self.logger.track()
        self.assertTrue(msg.startswith(self.logger.getModule()))
        self.assertTrue(msg.endswith("test60()"))

    def test61(self):
        """Verify track handles arbitrary argument types correctly."""
        self.logger.enableTracking()
        msg = self.logger.track("this", None, 1, 18.25)
        self.assertTrue(msg.startswith(self.logger.getModule()))
        self.assertTrue(msg.endswith("test61(this, None, 1, 18.25)"))

    def test70(self):
        """Verify format args are properly passed to track."""
        self.logger.enableTracking()
        msg = self.logger.track("this", None, 1, 18.25, fmt="foo={} bar={} a={} b={}")
        self.assertTrue(msg.startswith(f"{self.logger.getModule()}("))
        self.assertTrue(msg.endswith(").test70(foo=this bar=None a=1 b=18.25)"))

    def test71(self):
        self.logger.setLevel(Path.Log.Level.DEBUG)

        module = self.logger.getModule()
        debug_message = self.logger.debug("something x={}", 1)
        self.assertTrue(debug_message.startswith(f"{module}.DEBUG: ("))
        self.assertTrue(debug_message.endswith(") - something x=1\n"))
        self.assertEqual(self.logger.info("something x={}", 2), f"{module}.INFO: something x=2\n")
        self.assertEqual(
            self.logger.notice("something x={}", 3), f"{module}.NOTICE: something x=3\n"
        )
        self.assertEqual(
            self.logger.warning("something x={}", 4), f"{module}.WARNING: something x=4\n"
        )
        self.assertEqual(self.logger.error("something x={}", 5), f"{module}.ERROR: something x=5\n")

    def testzz(self):
        """Restoring environment after tests."""
        self.logger.setLevel(Path.Log.Level.RESET)
