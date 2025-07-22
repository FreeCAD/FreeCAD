# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2024 bgbsww@gmail.com                                   *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

import sys
import unittest
import FreeCAD as App
import Part

try:
    from guppy import hpy

    Memtest = True
except ImportError:
    Memtest = False

try:
    import cProfile

    Pyprofile = True
except ImportError:
    Pyprofile = False


class PerfTestCase(unittest.TestCase):
    """
    Special Test Case that takes a list of filenames after the "--pass" parameter to FreeCAD, and
    runs a performance test by opening them, starting instrumentation, calling recompute(), and
    then saving results.

    Intended to be run as "<perf profiling>  FreeCAD -t TestPerf --pass <modelname>

    External perf profiling requires Python 3.12 or better, and a linux platform.
    cProfile profiling and guppy memory information can run anywhere.
    """

    def setUp(self):
        if "--pass" in sys.argv:
            self.fileList = sys.argv[sys.argv.index("--pass") + 1 :]
        else:
            raise FileNotFoundError("Must provide filename parameter(s) via --pass")
        if "--save" in sys.argv:
            self.saveModels = True
            self.fileList = sys.argv[sys.argv.index("--save") + 1 :]
        else:
            self.saveModels = False
        if Part.Shape().ElementMapVersion == "":
            self.tnp = ""
        else:
            self.tnp = ".tnp"
        if Memtest:
            # Use filename of first model with ".mprofile" appended for python memory use info.
            self.memfile = open(self.fileList[0] + self.tnp + ".mprofile", "w", encoding="utf-8")

    def testAll(self):
        if Pyprofile:
            # Generate a cProfile file as a python only time profile.
            profile = cProfile.Profile()
            profile.enable()
        try:
            # This is Python 3.12 on supported platforms ( linux ) only so that if we are run under
            # an external 'perf' command, we report the python data.  This can be extremely useful,
            # because it contains not only time consumed, but python and c++ calls that took place
            # so deep analysis can be performed on the resulting file.  See calling script in
            # tools/profile/perftest.sh for a wrapper.
            sys.activate_stack_trampoline("perf")
        except AttributeError:
            pass  # Totally okay if we don't have that, we can use the cProfile if it's there.

        # Walk all files after the --pass.  Normally one to avoid result intermingling.
        for fileName in self.fileList:
            doc = App.openDocument(fileName)
            doc.recompute()  # The heart of the performance measurement.
            if Memtest:
                # If guppy is available, take a heap snapshot and save it.  Note that if multiple
                # files are provided then their heap data sets will be appended to the same file.
                dumpdata = hpy().heap()
                dumpdata.stat.dump(self.memfile)
                self.memfile.flush()
            if self.saveModels:
                filenametnp = App.ActiveDocument.Name + self.tnp
                App.ActiveDocument.saveAs(filenametnp)
            App.closeDocument(doc.Name)

        try:
            sys.deactivate_stack_trampoline()
        except AttributeError:
            pass
        if Pyprofile:
            profile.disable()
            # Use filename of first model with ".cprofile" appended for python profiling information.
            profile.dump_stats(self.fileList[0] + self.tnp + ".cprofile")
        if Memtest:
            self.memfile.close()
