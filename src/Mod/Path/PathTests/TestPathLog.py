# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
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

import PathScripts.PathLog as PathLog
import unittest

class TestPathLog(unittest.TestCase):
    """Some basic tests for the logging framework."""

    MODULE = 'TestPathLog' # file name without extension

    def setUp(self):
        PathLog.setLevel(PathLog.Level.RESET)
        PathLog.untrackAllModules()

    def callerFile(self):
        return PathLog._caller()[0] # pylint: disable=protected-access
    def callerLine(self):
        return PathLog._caller()[1] # pylint: disable=protected-access
    def callerFunc(self):
        return PathLog._caller()[2] # pylint: disable=protected-access

    def test00(self):
        """Check for proper module extraction."""
        self.assertEqual(self.callerFile(), self.MODULE)

    def test01(self):
        """Check for proper function extraction."""
        self.assertEqual(self.callerFunc(), 'test01')

    def test10(self):
        """Verify default log levels is NOTICE."""
        self.assertEqual(PathLog.getLevel(), PathLog.Level.NOTICE)
        self.assertEqual(PathLog.getLevel(self.MODULE), PathLog.Level.NOTICE)

    def test11(self):
        """Verify setting global log level."""
        PathLog.setLevel(PathLog.Level.DEBUG)

        self.assertEqual(PathLog.getLevel(), PathLog.Level.DEBUG)
        self.assertEqual(PathLog.getLevel(self.MODULE), PathLog.Level.DEBUG)

    def test12(self):
        """Verify setting module log level."""
        PathLog.setLevel(PathLog.Level.DEBUG, self.MODULE)

        self.assertEqual(PathLog.getLevel(), PathLog.Level.NOTICE)
        self.assertEqual(PathLog.getLevel(self.MODULE), PathLog.Level.DEBUG)

    def test13(self):
        """Verify setting other modul's log level doesn't change this one's."""
        # if this test fails then most likely the global RESET is broken
        PathLog.setLevel(PathLog.Level.DEBUG, 'SomeOtherModule')

        self.assertEqual(PathLog.getLevel(), PathLog.Level.NOTICE)
        self.assertEqual(PathLog.getLevel(self.MODULE), PathLog.Level.NOTICE)

    def test14(self):
        """Verify resetting log level for module falls back to global level."""
        PathLog.setLevel(PathLog.Level.DEBUG, self.MODULE)
        self.assertEqual(PathLog.getLevel(self.MODULE), PathLog.Level.DEBUG)
        # changing global log level does not affect module
        PathLog.setLevel(PathLog.Level.ERROR)
        self.assertEqual(PathLog.getLevel(self.MODULE), PathLog.Level.DEBUG)
        # resetting module log level restores global log level for module
        PathLog.setLevel(PathLog.Level.RESET, self.MODULE)
        self.assertEqual(PathLog.getLevel(self.MODULE), PathLog.Level.ERROR)
        # changing the global log level will also change the module log level
        PathLog.setLevel(PathLog.Level.DEBUG)
        self.assertEqual(PathLog.getLevel(self.MODULE), PathLog.Level.DEBUG)

    def test20(self):
        """Verify debug logs aren't logged by default."""
        self.assertIsNone(PathLog.debug("this"))

    def test21(self):
        """Verify debug logs are logged if log level is set to DEBUG."""
        PathLog.setLevel(PathLog.Level.DEBUG)
        self.assertIsNotNone(PathLog.debug("this"))

    def test30(self):
        """Verify log level ERROR."""
        PathLog.setLevel(PathLog.Level.ERROR)
        self.assertIsNone(PathLog.debug('something'))
        self.assertIsNone(PathLog.info('something'))
        self.assertIsNone(PathLog.notice('something'))
        self.assertIsNone(PathLog.warning('something'))
        self.assertIsNotNone(PathLog.error('something'))

    def test31(self):
        """Verify log level WARNING."""
        PathLog.setLevel(PathLog.Level.WARNING)
        self.assertIsNone(PathLog.debug('something'))
        self.assertIsNone(PathLog.info('something'))
        self.assertIsNone(PathLog.notice('something'))
        self.assertIsNotNone(PathLog.warning('something'))
        self.assertIsNotNone(PathLog.error('something'))

    def test32(self):
        """Verify log level NOTICE."""
        PathLog.setLevel(PathLog.Level.NOTICE)
        self.assertIsNone(PathLog.debug('something'))
        self.assertIsNone(PathLog.info('something'))
        self.assertIsNotNone(PathLog.notice('something'))
        self.assertIsNotNone(PathLog.warning('something'))
        self.assertIsNotNone(PathLog.error('something'))

    def test33(self):
        """Verify log level INFO."""
        PathLog.setLevel(PathLog.Level.INFO)
        self.assertIsNone(PathLog.debug('something'))
        self.assertIsNotNone(PathLog.info('something'))
        self.assertIsNotNone(PathLog.notice('something'))
        self.assertIsNotNone(PathLog.warning('something'))
        self.assertIsNotNone(PathLog.error('something'))

    def test34(self):
        """Verify log level DEBUG."""
        PathLog.setLevel(PathLog.Level.DEBUG)
        self.assertIsNotNone(PathLog.debug('something'))
        self.assertIsNotNone(PathLog.info('something'))
        self.assertIsNotNone(PathLog.notice('something'))
        self.assertIsNotNone(PathLog.warning('something'))
        self.assertIsNotNone(PathLog.error('something'))

    def test50(self):
        """Verify no tracking by default."""
        self.assertIsNone(PathLog.track('this', 'and', 'that'))

    def test51(self):
        """Verify enabling tracking for module results in tracking."""
        PathLog.trackModule()
        # Don't want to rely on the line number matching - still want some
        # indication that track does the right thing ....
        msg = PathLog.track('this', 'and', 'that')
        self.assertTrue(msg.startswith(self.MODULE))
        self.assertTrue(msg.endswith('test51(this, and, that)'))

    def test52(self):
        """Verify untracking stops tracking."""
        PathLog.trackModule()
        self.assertIsNotNone(PathLog.track('this', 'and', 'that'))
        PathLog.untrackModule()
        self.assertIsNone(PathLog.track('this', 'and', 'that'))

    def test53(self):
        """Verify trackAllModules works correctly."""
        PathLog.trackAllModules(True)
        self.assertIsNotNone(PathLog.track('this', 'and', 'that'))
        PathLog.trackAllModules(False)
        self.assertIsNone(PathLog.track('this', 'and', 'that'))
        PathLog.trackAllModules(True)
        PathLog.trackModule()
        self.assertIsNotNone(PathLog.track('this', 'and', 'that'))
        PathLog.trackAllModules(False)
        self.assertIsNotNone(PathLog.track('this', 'and', 'that'))

    def test60(self):
        """Verify track handles no argument."""
        PathLog.trackModule()
        msg = PathLog.track()
        self.assertTrue(msg.startswith(self.MODULE))
        self.assertTrue(msg.endswith('test60()'))

    def test61(self):
        """Verify track handles arbitrary argument types correctly."""
        PathLog.trackModule()
        msg = PathLog.track('this', None, 1, 18.25)
        self.assertTrue(msg.startswith(self.MODULE))
        self.assertTrue(msg.endswith('test61(this, None, 1, 18.25)'))

    def testzz(self):
        """Restoring environment after tests."""
        PathLog.setLevel(PathLog.Level.RESET)
