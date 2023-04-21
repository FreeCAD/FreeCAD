# -*- coding: utf-8 -*-
#/******************************************************************************
# *   Copyright (c) 2012 Jan Rheinländer <jrheinlaender@users.sourceforge.net> *
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

from PySide import QtCore, QtGui
import FreeCAD, FreeCADGui

class Diagram:
    def create(self, title, function, xlength, xname, xunit, xscale, yname, yunit, yscale, numxpoints):
        # Initialize
        from FreeCAD.Plot import Plot
        self.title = title
        self.function = function # This is assumed to be always a SegmentFunction
        self.xlength = xlength
        self.xname = xname
        self.xunit = xunit
        self.xscale = xscale
        self.yname = yname
        self.yunit = yunit
        self.yscale = yscale
        self.numxpoints = numxpoints

        # Create a plot window
        self.win = Plot.figure(title)
        # Get the plot object from the window
        self.thePlot = Plot.getPlot()
        # Format the plot object
        Plot.xlabel("$%s$ [%s]" % (xname, xunit))
        Plot.ylabel("$%s$ [%s]" % (yname, yunit))
        Plot.grid(True)

        # Calculate points
        (self.xpoints, self.ypoints) = self.function.evaluate(self.xlength, self.numxpoints)
        # Create plot
        self.plot()

    def update(self, function = None, xlength = None):
        if function is not None:
            self.function = function
        if xlength is not None:
            self.xlength = xlength
        # Calculate points
        (self.xpoints, self.ypoints) = self.function.evaluate(self.xlength, self.numxpoints)
        # Create plot
        self.plot()

    def plot(self):
        plots = self.thePlot.series

        if plots:
            # Remove line from plot
            axes = plots[0].axes
            axes.lines.pop(plots[0].lid)
            # Remove serie from list
            del self.thePlot.series[0]

        self.thePlot.update()
        self.xpoints = [p * self.xscale for p in self.xpoints]
        self.ypoints = [p * self.yscale for p in self.ypoints]
        self.thePlot.plot(self.xpoints, self.ypoints)
        plots = self.thePlot.series
        axes = plots[0].axes
        axes.set_xlim(right = max(self.xpoints) * 1.05)
        axes.set_ylim(min(self.ypoints) * 1.05, max(self.ypoints) * 1.05)
        self.thePlot.update()

    def close(self):
        # Close the associated mdiSubWindow
        self.win.parent().close()
