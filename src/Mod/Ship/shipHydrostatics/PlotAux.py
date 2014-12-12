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
import math
from PySide import QtGui, QtCore
import FreeCAD
import FreeCADGui
import Spreadsheet
from shipUtils import Paths


class Plot(object):
    def __init__(self, ship, trim, points):
        """ Constructor. performs plot and show it (Using pyxplot).
        @param ship Selected ship instance
        @param trim Trim in degrees.
        @param points List of computed hydrostatics.
        """
        self.points = points[:]
        # Try to plot
        self.plotVolume()
        self.plotStability()
        self.plotCoeffs()
        # Save data
        if self.spreadSheet(ship, trim):
            return

    def plotVolume(self):
        """ Perform volumetric hydrostatics.
        @return True if error happens.
        """
        try:
            import Plot
            plt = Plot.figure('Volume')
        except ImportError:
            msg = QtGui.QApplication.translate(
                "ship_console",
                "Plot module is disabled, so I cannot perform the plot",
                None,
                QtGui.QApplication.UnicodeUTF8)
            FreeCAD.Console.PrintWarning(msg + '\n')
            return True

        # Generate the set of axes
        Plot.grid(True)
        for i in range(0, 3):
            ax = Plot.addNewAxes()
            # Y axis can be placed at right
            ax.yaxis.tick_right()
            ax.spines['right'].set_color((0.0, 0.0, 0.0))
            ax.spines['left'].set_color('none')
            ax.yaxis.set_ticks_position('right')
            ax.yaxis.set_label_position('right')
            # And X axis can be placed at bottom
            for loc, spine in ax.spines.iteritems():
                if loc in ['bottom', 'top']:
                    spine.set_position(('outward', (i + 1) * 35))
            Plot.grid(True)

        disp = []
        draft = []
        warea = []
        t1cm = []
        xcb = []
        for i in range(len(self.points)):
            disp.append(self.points[i].disp)
            draft.append(self.points[i].draft)
            warea.append(self.points[i].wet)
            t1cm.append(self.points[i].mom)
            xcb.append(self.points[i].xcb)

        axes = Plot.axesList()
        for ax in axes:
            ax.set_position([0.1, 0.2, 0.8, 0.75])

        plt.axes = axes[0]
        serie = Plot.plot(draft, disp, r'$T$')
        serie.line.set_linestyle('-')
        serie.line.set_linewidth(2.0)
        serie.line.set_color((0.0, 0.0, 0.0))
        Plot.xlabel(r'$T \; \left[ \mathrm{m} \right]$')
        Plot.ylabel(r'$\bigtriangleup \; \left[ \mathrm{tons} \right]$')
        plt.axes.xaxis.label.set_fontsize(15)
        plt.axes.yaxis.label.set_fontsize(15)
        plt.axes = axes[1]
        serie = Plot.plot(warea, disp, r'Wetted area')
        serie.line.set_linestyle('-')
        serie.line.set_linewidth(2.0)
        serie.line.set_color((1.0, 0.0, 0.0))
        Plot.xlabel(r'$Wetted \; area \; \left[ \mathrm{m}^2 \right]$')
        Plot.ylabel(r'$\bigtriangleup \; \left[ \mathrm{tons} \right]$')
        plt.axes.xaxis.label.set_fontsize(15)
        plt.axes.yaxis.label.set_fontsize(15)
        plt.axes = axes[2]
        serie = Plot.plot(t1cm, disp, r'Moment to trim 1cm')
        serie.line.set_linestyle('-')
        serie.line.set_linewidth(2.0)
        serie.line.set_color((0.0, 0.0, 1.0))
        Plot.xlabel(r'$Moment \; to \; trim \; 1 \mathrm{cm} \; \left['
                    r' \mathrm{tons} \; \times \; \mathrm{m} \right]$')
        plt.axes.xaxis.label.set_fontsize(15)
        plt.axes.yaxis.label.set_fontsize(15)
        plt.axes = axes[3]
        serie = Plot.plot(xcb, disp, r'$XCB$')
        serie.line.set_linestyle('-')
        serie.line.set_linewidth(2.0)
        serie.line.set_color((0.2, 0.8, 0.2))
        Plot.xlabel(r'$XCB \; \left[ \mathrm{m} \right]$')
        plt.axes.xaxis.label.set_fontsize(15)
        plt.axes.yaxis.label.set_fontsize(15)

        Plot.legend(True)
        plt.update()
        return False

    def plotStability(self):
        """ Perform stability hydrostatics.
        @return True if error happens.
        """
        try:
            import Plot
            plt = Plot.figure('Stability')
        except ImportError:
            return True

        # Generate the sets of axes
        Plot.grid(True)
        for i in range(0, 3):
            ax = Plot.addNewAxes()
            # Y axis can be placed at right
            ax.yaxis.tick_right()
            ax.spines['right'].set_color((0.0, 0.0, 0.0))
            ax.spines['left'].set_color('none')
            ax.yaxis.set_ticks_position('right')
            ax.yaxis.set_label_position('right')
            # And X axis can be placed at bottom
            for loc, spine in ax.spines.iteritems():
                if loc in ['bottom', 'top']:
                    spine.set_position(('outward', (i + 1) * 35))
            Plot.grid(True)

        disp = []
        draft = []
        farea = []
        kbt = []
        bmt = []
        for i in range(len(self.points)):
            disp.append(self.points[i].disp)
            draft.append(self.points[i].draft)
            farea.append(self.points[i].farea)
            kbt.append(self.points[i].KBt)
            bmt.append(self.points[i].BMt)

        axes = Plot.axesList()
        for ax in axes:
            ax.set_position([0.1, 0.2, 0.8, 0.75])

        plt.axes = axes[0]
        serie = Plot.plot(draft, disp, r'$T$')
        serie.line.set_linestyle('-')
        serie.line.set_linewidth(2.0)
        serie.line.set_color((0.0, 0.0, 0.0))
        Plot.xlabel(r'$T \; \left[ \mathrm{m} \right]$')
        Plot.ylabel(r'$\bigtriangleup \; \left[ \mathrm{tons} \right]$')
        plt.axes.xaxis.label.set_fontsize(15)
        plt.axes.yaxis.label.set_fontsize(15)
        plt.axes = axes[1]
        serie = Plot.plot(farea, disp, r'Floating area')
        serie.line.set_linestyle('-')
        serie.line.set_linewidth(2.0)
        serie.line.set_color((1.0, 0.0, 0.0))
        Plot.xlabel(r'$Floating \; area \; \left[ \mathrm{m}^2 \right]$')
        Plot.ylabel(r'$\bigtriangleup \; \left[ \mathrm{tons} \right]$')
        plt.axes.xaxis.label.set_fontsize(15)
        plt.axes.yaxis.label.set_fontsize(15)
        plt.axes = axes[2]
        serie = Plot.plot(kbt, disp, r'$KB_T$')
        serie.line.set_linestyle('-')
        serie.line.set_linewidth(2.0)
        serie.line.set_color((0.0, 0.0, 1.0))
        Plot.xlabel(r'$KB_T \; \left[ \mathrm{m} \right]$')
        plt.axes.xaxis.label.set_fontsize(15)
        plt.axes.yaxis.label.set_fontsize(15)
        plt.axes = axes[3]
        serie = Plot.plot(bmt, disp, r'$BM_T$')
        serie.line.set_linestyle('-')
        serie.line.set_linewidth(2.0)
        serie.line.set_color((0.2, 0.8, 0.2))
        Plot.xlabel(r'$BM_T \; \left[ \mathrm{m} \right]$')
        plt.axes.xaxis.label.set_fontsize(15)
        plt.axes.yaxis.label.set_fontsize(15)

        Plot.legend(True)
        plt.update()
        return False

    def plotCoeffs(self):
        """ Perform stability hydrostatics.
        @return True if error happens.
        """
        # Create plot
        try:
            import Plot
            plt = Plot.figure('Coefficients')
        except ImportError:
            return True

        # Generate the set of axes
        Plot.grid(True)
        for i in range(0, 3):
            ax = Plot.addNewAxes()
            # Y axis can be placed at right
            ax.yaxis.tick_right()
            ax.spines['right'].set_color((0.0, 0.0, 0.0))
            ax.spines['left'].set_color('none')
            ax.yaxis.set_ticks_position('right')
            ax.yaxis.set_label_position('right')
            # And X axis can be placed at bottom
            for loc, spine in ax.spines.iteritems():
                if loc in ['bottom', 'top']:
                    spine.set_position(('outward', (i + 1) * 35))
            Plot.grid(True)

        disp = []
        draft = []
        cb = []
        cf = []
        cm = []
        for i in range(len(self.points)):
            disp.append(self.points[i].disp)
            draft.append(self.points[i].draft)
            cb.append(self.points[i].Cb)
            cf.append(self.points[i].Cf)
            cm.append(self.points[i].Cm)

        axes = Plot.axesList()
        for ax in axes:
            ax.set_position([0.1, 0.2, 0.8, 0.75])

        plt.axes = axes[0]
        serie = Plot.plot(draft, disp, r'$T$')
        serie.line.set_linestyle('-')
        serie.line.set_linewidth(2.0)
        serie.line.set_color((0.0, 0.0, 0.0))
        Plot.xlabel(r'$T \; \left[ \mathrm{m} \right]$')
        Plot.ylabel(r'$\bigtriangleup \; \left[ \mathrm{tons} \right]$')
        plt.axes.xaxis.label.set_fontsize(15)
        plt.axes.yaxis.label.set_fontsize(15)
        plt.axes = axes[1]
        serie = Plot.plot(cb, disp, r'$Cb$')
        serie.line.set_linestyle('-')
        serie.line.set_linewidth(2.0)
        serie.line.set_color((1.0, 0.0, 0.0))
        Plot.xlabel(r'$Cb$ (Block coefficient)')
        Plot.ylabel(r'$\bigtriangleup \; \left[ \mathrm{tons} \right]$')
        plt.axes.xaxis.label.set_fontsize(15)
        plt.axes.yaxis.label.set_fontsize(15)
        plt.axes = axes[2]
        serie = Plot.plot(cf, disp, r'$Cf$')
        serie.line.set_linestyle('-')
        serie.line.set_linewidth(2.0)
        serie.line.set_color((0.0, 0.0, 1.0))
        Plot.xlabel(r'$Cf$ (floating area coefficient)')
        plt.axes.xaxis.label.set_fontsize(15)
        plt.axes.yaxis.label.set_fontsize(15)
        plt.axes = axes[3]
        serie = Plot.plot(cm, disp, r'$Cm$')
        serie.line.set_linestyle('-')
        serie.line.set_linewidth(2.0)
        serie.line.set_color((0.2, 0.8, 0.2))
        Plot.xlabel(r'$Cm$  (Main section coefficient)')
        plt.axes.xaxis.label.set_fontsize(15)
        plt.axes.yaxis.label.set_fontsize(15)

        Plot.legend(True)
        plt.update()
        return False

    def spreadSheet(self, ship, trim):
        """ Write data file.
        @param ship Selected ship instance
        @param trim Trim in degrees.
        @return True if error happens.
        """
        # Create the spreadsheet
        obj = Spreadsheet.makeSpreadsheet()
        s = obj.Proxy
        obj.Label = 'Hydrostatics'

        # Print the header
        s.a1 = "displacement [ton]"
        s.b1 = "draft [m]"
        s.c1 = "wetted surface [m^2]"
        s.d1 = "1cm triming ship moment [ton*m]"
        s.e1 = "Floating area [m^2]"
        s.f1 = "KBl [m]"
        s.g1 = "KBt [m]"
        s.h1 = "BMt [m]"
        s.i1 = "Cb"
        s.j1 = "Cf"
        s.k1 = "Cm"

        for i in range(len(self.points)):
            point = self.points[i]
            s.__setattr__("a{}".format(i + 2), point.disp)
            s.__setattr__("b{}".format(i + 2), point.draft)
            s.__setattr__("c{}".format(i + 2), point.wet)
            s.__setattr__("d{}".format(i + 2), point.mom)
            s.__setattr__("e{}".format(i + 2), point.farea)
            s.__setattr__("f{}".format(i + 2), point.xcb)
            s.__setattr__("g{}".format(i + 2), point.KBt)
            s.__setattr__("h{}".format(i + 2), point.BMt)
            s.__setattr__("i{}".format(i + 2), point.Cb)
            s.__setattr__("j{}".format(i + 2), point.Cf)
            s.__setattr__("k{}".format(i + 2), point.Cm)

        # Open the spreadsheet
        FreeCADGui.ActiveDocument.setEdit(obj.Name,0)
