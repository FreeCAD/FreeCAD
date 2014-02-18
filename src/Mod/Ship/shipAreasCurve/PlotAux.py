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


header = """ #################################################################

 #####                 ####  ###   ####      ##### #   # ### ####
 #                    #      # #   #   #    #      #   #  #  #   #
 #     ##  #### ####  #     #   #  #   #     #     #   #  #  #   #
 ####  # # #  # #  #  #     #####  #   # ##   ##   #####  #  ####
 #     #   #### ####  #    #     # #   #        #  #   #  #  #
 #     #   #    #     #    #     # #   #         # #   #  #  #
 #     #   #### ####   ### #     # ####     #####  #   # ### #

 #################################################################
"""


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
        if self.createDirectory():
            return
        if self.saveData(x, y, ship):
            return

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
        Lpp = ship.Length
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

    def createDirectory(self):
        """ Create the needed folder to write the output data.
        @return True if error happens.
        """
        self.path = FreeCAD.ConfigGet("UserAppData") + "ShipOutput/"
        if not os.path.exists(self.path):
            os.makedirs(self.path)
        if not os.path.exists(self.path):
            msg = QtGui.QApplication.translate(
                "ship_console",
                "Failure creating the folder",
                None,
                QtGui.QApplication.UnicodeUTF8)
            FreeCAD.Console.PrintError(msg + ":\n\t'" + self.path + "'\n")
            return True
        return False

    def saveData(self, x, y, ship):
        """ Write the output data file.
        @param x X coordinates.
        @param y Transversal areas.
        @param ship Active ship instance.
        @return True if error happens.
        """
        filename = self.path + 'areas.dat'
        try:
            Output = open(filename, "w")
        except IOError:
            msg = QtGui.QApplication.translate(
                "ship_console",
                "Failure writing to file",
                None,
                QtGui.QApplication.UnicodeUTF8)
            FreeCAD.Console.PrintError(msg + ":\n\t'" + filename + "'\n")
            return True
        # Print the header
        Output.write(header)
        Output.write(" #\n")
        Output.write(" # File automatically exported by FreeCAD-Ship\n")
        Output.write(" # This file contains transversal areas data, filled"
                     " with following columns:\n")
        Output.write(" # 1: X coordiante [m]\n")
        Output.write(" # 2: Transversal area [m2]\n")
        Output.write(" # 3: X FP coordinate [m]\n")
        Output.write(" # 4: Y FP coordinate (bounds in order to draw it)\n")
        Output.write(" # 3: X AP coordinate [m]\n")
        Output.write(" # 4: Y AP coordinate (bounds in order to draw it)\n")
        Output.write(" #\n")
        Output.write(" ######################################################"
                     "###########\n")
        # Get perpendiculars data
        Lpp = ship.Length
        FPx = 0.5 * Lpp
        APx = -0.5 * Lpp
        maxArea = max(y)
        # Print the data
        string = "{0} {1} {2} {3} {4} {5}\n".format(x[0],
                                                    y[0],
                                                    FPx,
                                                    0.0,
                                                    APx,
                                                    0.0)
        Output.write(string)
        for i in range(1, len(x)):
            string = "{0} {1} {2} {3} {4} {5}\n".format(x[i],
                                                        y[i],
                                                        FPx,
                                                        maxArea,
                                                        APx,
                                                        maxArea)
            Output.write(string)
        Output.close()
        self.dataFile = filename
        msg = QtGui.QApplication.translate("ship_console",
                                           "Data saved",
                                           None,
                                           QtGui.QApplication.UnicodeUTF8)
        FreeCAD.Console.PrintMessage(msg + ":\n\t'" + self.dataFile + "'\n")
        return False
