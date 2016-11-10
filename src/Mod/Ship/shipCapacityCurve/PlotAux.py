#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2011, 2016                                              *
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
import matplotlib.ticker as mtick


class Plot(object):
    def __init__(self, l, z, v, tank):
        """ Constructor. performs the plot and shows it.
        @param l Percentages of filling level.
        @param z Level z coordinates.
        @param v Volume of fluid.
        @param tank Active tank instance.
        """
        self.plot(l, z, v, tank)
        self.spreadSheet(l, z, v, tank)

    def plot(self, l, z, v, tank):
        """ Perform the areas curve plot.
        @param l Percentages of filling level.
        @param z Level z coordinates.
        @param v Volume of fluid.
        @param tank Active tank instance.
        @return True if error happens.
        """
        try:
            import Plot
            plt = Plot.figure('Capacity curve')
        except ImportError:
            msg = QtGui.QApplication.translate(
                "ship_console",
                "Plot module is disabled, so I cannot perform the plot",
                None,
                QtGui.QApplication.UnicodeUTF8)
            FreeCAD.Console.PrintWarning(msg + '\n')
            return True

        # Plot the volume as a function of the level percentage
        vols = Plot.plot(l, v, 'Capacity')
        vols.line.set_linestyle('-')
        vols.line.set_linewidth(2.0)
        vols.line.set_color((0.0, 0.0, 0.0))
        Plot.xlabel(r'$\mathrm{level}$')
        Plot.ylabel(r'$V \; [\mathrm{m}^3]$')
        plt.axes.xaxis.label.set_fontsize(20)
        plt.axes.yaxis.label.set_fontsize(20)
        Plot.grid(True)

        # Special percentage formatter for the x axis
        fmt = '%.0f%%'
        xticks = mtick.FormatStrFormatter(fmt)
        plt.axes.xaxis.set_major_formatter(xticks)

        # Now duplicate the axes
        ax = Plot.addNewAxes()
        # Y axis can be placed at right
        ax.yaxis.tick_right()
        ax.spines['right'].set_color((0.0, 0.0, 0.0))
        ax.spines['left'].set_color('none')
        ax.yaxis.set_ticks_position('right')
        ax.yaxis.set_label_position('right')
        # And X axis can be placed at top
        ax.xaxis.tick_top()
        ax.spines['top'].set_color((0.0, 0.0, 1.0))
        ax.spines['bottom'].set_color('none')
        ax.xaxis.set_ticks_position('top')
        ax.xaxis.set_label_position('top')

        # Plot the volume as a function of the level z coordinate
        vols = Plot.plot(z, v, 'level')
        vols.line.set_linestyle('-')
        vols.line.set_linewidth(2.0)
        vols.line.set_color((0.0, 0.0, 1.0))
        Plot.xlabel(r'$z \; [\mathrm{m}]$')
        Plot.ylabel(r'$V \; [\mathrm{m}^3]$')
        ax.xaxis.label.set_fontsize(20)
        ax.yaxis.label.set_fontsize(20)
        ax.xaxis.label.set_color((0.0, 0.0, 1.0))
        ax.tick_params(axis='x', colors=(0.0, 0.0, 1.0))
        Plot.grid(True)

        # End
        plt.update()
        return False

    def spreadSheet(self, l, z, v, tank):
        """ Write the output data file.
        @param l Percentages of filling level.
        @param z Level z coordinates.
        @param v Volume of fluid.
        @param tank Active tank instance.
        """
        s = FreeCAD.activeDocument().addObject('Spreadsheet::Sheet',
                                               'Capacity curve')

        # Print the header
        s.set("A1", "Percentage of filling level")
        s.set("B1", "Level [m]")
        s.set("C1", "Volume [m^3]")

        # Print the data
        for i in range(len(l)):
            s.set("A{}".format(i + 2), str(l[i]))
            s.set("B{}".format(i + 2), str(z[i]))
            s.set("C{}".format(i + 2), str(v[i]))

        # Recompute
        FreeCAD.activeDocument().recompute()