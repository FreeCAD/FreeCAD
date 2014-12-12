#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2011, 2012                                              *
#*   Jose Luis Cercos Pita <jlcercos@gmail.com>                            *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************

import os
from PySide import QtGui, QtCore
import FreeCAD
import FreeCADGui
from FreeCAD import Base
import Spreadsheet

class Plot(object):
    def __init__(self, x, y, disp, xcb, ship):
        """ Constructor. performs the plot and shows it.
        @param x X coordinates.
        @param y Transversal computed areas.
        @param disp Ship displacement.
        @param xcb Bouyancy center length.
        @param ship Active ship instance.
        """
        self.plot(x, y, disp, xcb, ship)
        self.spreadSheet(x, y, ship)

    def plot(self, x, y, disp, xcb, ship):
        """ Perform the areas curve plot.
        @param x X coordinates.
        @param y Transversal areas.
        @param disp Ship displacement.
        @param xcb Bouyancy center length.
        @param ship Active ship instance.
        @return True if error happens.
        """
        try:
            import Plot
            plt = Plot.figure('Areas curve')
        except ImportError:
            msg = QtGui.QApplication.translate(
                "ship_console",
                "Plot module is disabled, so I cannot perform the plot",
                None,
                QtGui.QApplication.UnicodeUTF8)
            FreeCAD.Console.PrintWarning(msg + '\n')
            return True
        # Plot areas curve
        areas = Plot.plot(x, y, 'Transversal areas')
        areas.line.set_linestyle('-')
        areas.line.set_linewidth(2.0)
        areas.line.set_color((0.0, 0.0, 0.0))
        # Get perpendiculars data
        Lpp = ship.Length.getValueAs('m').Value
        FPx = 0.5 * Lpp
        APx = -0.5 * Lpp
        maxArea = max(y)
        # Plot perpendiculars
        FP = Plot.plot([FPx, FPx], [0.0, maxArea])
        FP.line.set_linestyle('-')
        FP.line.set_linewidth(1.0)
        FP.line.set_color((0.0, 0.0, 0.0))
        AP = Plot.plot([APx, APx], [0.0, maxArea])
        AP.line.set_linestyle('-')
        AP.line.set_linewidth(1.0)
        AP.line.set_color((0.0, 0.0, 0.0))
        # Add annotations for prependiculars
        ax = Plot.axes()
        ax.annotate('AP', xy=(APx + 0.01 * Lpp, 0.01 * maxArea), size=15)
        ax.annotate('AP', xy=(APx + 0.01 * Lpp, 0.95 * maxArea), size=15)
        ax.annotate('FP', xy=(FPx + 0.01 * Lpp, 0.01 * maxArea), size=15)
        ax.annotate('FP', xy=(FPx + 0.01 * Lpp, 0.95 * maxArea), size=15)
        # Add some additional data
        addInfo = ("$XCB = {0} \\; \\mathrm{{m}}$\n"
                   "$Area_{{max}} = {1} \\; \\mathrm{{m}}^2$\n"
                   "$\\bigtriangleup = {2} \\; \\mathrm{{tons}}$".format(
                   xcb,
                   maxArea,
                   disp))
        ax.text(0.0,
                0.01 * maxArea,
                addInfo,
                verticalalignment='bottom',
                horizontalalignment='center',
                fontsize=20)
        # Write axes titles
        Plot.xlabel(r'$x \; \mathrm{m}$')
        Plot.ylabel(r'$Area \; \mathrm{m}^2$')
        ax.xaxis.label.set_fontsize(20)
        ax.yaxis.label.set_fontsize(20)
        # Show grid
        Plot.grid(True)
        # End
        plt.update()
        return False

    def spreadSheet(self, x, y, ship):
        """ Write the output data file.
        @param x X coordinates.
        @param y Transversal areas.
        @param ship Active ship instance.
        """
        # Create the spreadsheet
        obj = Spreadsheet.makeSpreadsheet()
        s = obj.Proxy
        obj.Label = 'Areas curve'

        # Print the header
        s.a1 = "x [m]"
        s.b1 = "area [m^2]"
        s.c1 = "FP x"
        s.d1 = "FP y"
        s.e1 = "AP x"
        s.f1 = "AP y"

        # Print the perpendiculars data
        Lpp = ship.Length.getValueAs('m').Value
        FPx = 0.5 * Lpp
        APx = -0.5 * Lpp
        maxArea = max(y)
        s.c2 = FPx
        s.d2 = 0.0
        s.c3 = FPx
        s.d3 = maxArea
        s.e2 = APx
        s.f2 = 0.0
        s.e3 = APx
        s.f3 = maxArea

        # Print the data
        for i in range(len(x)):
            s.__setattr__("a{}".format(i + 2), x[i])
            s.__setattr__("b{}".format(i + 2), y[i])

        # Open the spreadsheet
        FreeCADGui.ActiveDocument.setEdit(obj.Name,0)
