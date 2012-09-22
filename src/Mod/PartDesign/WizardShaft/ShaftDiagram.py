#/******************************************************************************
# *   Copyright (c)2012 Jan Rheinlaender <jrheinlaender@users.sourceforge.net> *
# *                                                                            *
# *   This file is part of the FreeCAD CAx development system.                 *
# *                                                                            *
# *   This library is free software; you can redistribute it and/or            *
# *   modify it under the terms of the GNU Library General Public              *
# *   License as published by the Free Software Foundation; either             *
# *   version 2 of the License, or (at your option) any later version.         *
# *                                                                            *
# *   This library  is distributed in the hope that it will be useful,         *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
# *   GNU Library General Public License for more details.                     *
# *                                                                            *
# *   You should have received a copy of the GNU Library General Public        *
# *   License along with this library; see the file COPYING.LIB. If not,       *
# *   write to the Free Software Foundation, Inc., 59 Temple Place,            *
# *   Suite 330, Boston, MA  02111-1307, USA                                   *
# *                                                                            *
# ******************************************************************************/

import os, subprocess
from PyQt4 import QtCore, QtGui
import FreeCAD, FreeCADGui

header = " ### FREECAD SHAFT WIZARD ###\n #\n"

class Diagram:
    function = 0 # This is assumed to be always a SegmentFunction
    fname = "y(x)"
    xlength = 0.0
    xname = "x"
    xunit = ""
    xscale = 1.0
    yname = "y"
    yunit = ""
    yscale = 1.0
    numxpoints = 10
    points = []
    # process object of pyxplot running in the background
    pyxplot = None
    # Filesystem watcher
    graphicsFile = ""
    timer = 0
    watcher = 0
    updatePending = False

    def __init__(self, tmpdir):
        self.tmpdir = tmpdir
        # Set up watcher and timer
        self.watcher = QtCore.QFileSystemWatcher()
        self.watcher.fileChanged.connect(self.updateFinished)
        self.timer = QtCore.QTimer()
        self.timer.setInterval(100)
        self.timer.timeout.connect(self.pollFile)

    def create(self, title, function, xlength, xname, xunit, xscale, yname, yunit, yscale, numxpoints):
        # Initialize
        self.title = title
        self.function = function
        self.xlength = xlength
        self.xname = xname
        self.xunit = xunit
        self.xscale = xscale
        self.yname = yname
        self.yunit = yunit
        self.yscale = yscale
        self.numxpoints = numxpoints
        self.graphicsFile = os.path.join(self.tmpdir, self.function.name + '.png')

        # Calculate points
        self.points = self.function.evaluate(self.xlength, self.numxpoints)
        # Write files
        self.write()

    def update(self, function = None, xlength = None):
        if function is not None:
            self.function = function
        if xlength is not None:
            self.xlength = xlength
        # Calculate points
        self.points = self.function.evaluate(self.xlength, self.numxpoints)
        # Write files
        self.write()

    def write(self):
        "Write diagram files"
        # Check if pyxplot is still running in the background
        if (self.pyxplot is not None) and (self.pyxplot.poll() is None):
            # Process hasn't terminated yet, set flag to update again
            # as soon as the graphicsFile has been written by the current pyxplot process
            self.updatePending = True
            return

        # Get max and min values
        (xmin, xmax) = self.minmaxX()
        (ymin, ymax) = self.minmaxY()

        # Create data file
        dataFile = os.path.join(self.tmpdir, (self.function.name + ".dat"))
        file = open(dataFile, "w")
        file.write(header)
        file.write(" # File automatically exported by FreeCAD Shaft Wizard\n")
        file.write(" # This file contains xy data, filled with following columns:\n")
        file.write(" # 1: %s [%s]\n" % (self.xname, self.xunit))
        file.write(" # 2: %s [%s]\n" % (self.yname, self.yunit))
        file.write(" #\n")
        for (x, y) in self.points:
            file.write("%f %f\n" % (x * self.xscale, y * self.yscale))
        file.close()

        # Create pyxplot file
        commandFile = os.path.join(self.tmpdir, (self.function.name + ".pyxplot"))
        file = open(commandFile, "w")
        file.write(header)
        file.write(" # File automatically exported by FreeCAD Shaft Wizard\n")
        file.write(" # This file contains a script to plot xy data.\n")
        file.write(" # To use it execute:\n")
        file.write(" #\n")
        file.write(" # pyxplot %s\n" % (commandFile))
        file.write(" #\n")
        file.write(" #################################################################\n")
        # Write general options
        file.write("set numeric display latex\n")
        file.write("set terminal png\n")
        file.write("set output '%s'\n" % (self.graphicsFile))
        file.write("set nokey\n")
        file.write("set grid\n")
        file.write("# X axis\n")
        file.write("set xlabel '$%s$ [%s]'\n" % (self.xname, self.xunit))
        file.write("set xrange [%f:%f]\n" % ((xmin * self.xscale * 1.05, xmax * self.xscale * 1.05)))
        file.write("set xtic\n")
        file.write("# Y axis\n")
        file.write("set ylabel '$%s$ [%s]'\n" % (self.yname, self.yunit))
        file.write("set yrange [%f:%f]\n" % ((ymin * self.yscale * 1.05, ymax * self.yscale * 1.05)))
        file.write("set ytic\n")
        file.write("# Line styles\n")
        file.write("set style 1 line linetype 1 linewidth 2 colour rgb (0):(0):(0)\n")
        file.write("# Title\n")
        file.write("set title '%s'" % self.title)
        # Write plot call
        file.write("# Plot\n")
        file.write("plot '%s' using 1:2 axes x1y1 with lines style 1\n" % (dataFile))
        # Close file
        file.close()

        # Run pyxplot on the files, but don't wait for execution to finish
        # Instead, set timer to poll for the process to finish
        try:
            self.pyxplot = subprocess.Popen(["pyxplot", commandFile])
            # Poll the process to add a watcher as soon as it has created the graphics file
            if len(self.watcher.files()) == 0:
                self.timer.start()
        except OSError:
            FreeCAD.Console.PrintError("Can't execute pyxplot. Maybe it is not installed?\n")

        self.updatePending = False

    def pollFile(self):
        # Check if process has finished
        if self.pyxplot.poll() is None:
            return
        # Check if the graphics file has appeared and then set a watcher on it
        if not os.path.isfile(self.graphicsFile):
            return
        # We don't need the timer any more now
        self.timer.stop()
        FreeCADGui.SendMsgToActiveView('Refresh')
        self.watcher.addPath(self.graphicsFile)

    def updateFinished(self, filePath):
        # filePath is not important because we are only watching a single file
        # FIXME: Will give a warning in the console if the active window is not the HTML page
        FreeCADGui.SendMsgToActiveView('Refresh')
        if self.updatePending is True:
            self.write()

    def minmaxX(self):
        xpoints = []
        for (x, y) in self.points:
            xpoints.append(x)
        return (min(xpoints), max(xpoints))

    def minmaxY(self):
        ypoints = []
        for (x, y) in self.points:
            ypoints.append(y)
        return (min(ypoints), max(ypoints))



