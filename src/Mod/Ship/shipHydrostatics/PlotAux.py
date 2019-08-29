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
                None)
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
            for loc, spine in ax.spines.items():
                if loc in ['bottom', 'top']:
                    spine.set_position(('outward', (i + 1) * 35))
            Plot.grid(True)

        disp = []
        draft = []
        warea = []
        t1cm = []
        xcb = []
        for i in range(len(self.points)):
            disp.append(self.points[i].disp.getValueAs("kg").Value / 1000.0)
            draft.append(self.points[i].draft.getValueAs("m").Value)
            warea.append(self.points[i].wet.getValueAs("m^2").Value)
            t1cm.append(self.points[i].mom.getValueAs("kg*m").Value / 1000.0)
            xcb.append(self.points[i].xcb.getValueAs("m").Value)

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
            for loc, spine in ax.spines.items():
                if loc in ['bottom', 'top']:
                    spine.set_position(('outward', (i + 1) * 35))
            Plot.grid(True)

        disp = []
        draft = []
        farea = []
        kbt = []
        bmt = []
        for i in range(len(self.points)):
            disp.append(self.points[i].disp.getValueAs("kg").Value / 1000.0)
            draft.append(self.points[i].draft.getValueAs("m").Value)
            farea.append(self.points[i].farea.getValueAs("m^2").Value)
            kbt.append(self.points[i].KBt.getValueAs("m").Value)
            bmt.append(self.points[i].BMt.getValueAs("m").Value)

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
            for loc, spine in ax.spines.items():
                if loc in ['bottom', 'top']:
                    spine.set_position(('outward', (i + 1) * 35))
            Plot.grid(True)

        disp = []
        draft = []
        cb = []
        cf = []
        cm = []
        for i in range(len(self.points)):
            disp.append(self.points[i].disp.getValueAs("kg").Value / 1000.0)
            draft.append(self.points[i].draft.getValueAs("m").Value)
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
        s = FreeCAD.activeDocument().addObject('Spreadsheet::Sheet',
                                               'Hydrostatics')

        # Print the header
        s.set("A1", "displacement [ton]")
        s.set("B1", "draft [m]")
        s.set("C1", "wetted surface [m^2]")
        s.set("D1", "1cm trimming ship moment [ton*m]")
        s.set("E1", "Floating area [m^2]")
        s.set("F1", "KBl [m]")
        s.set("G1", "KBt [m]")
        s.set("H1", "BMt [m]")
        s.set("I1", "Cb")
        s.set("J1", "Cf")
        s.set("K1", "Cm")

        # Print the data
        for i in range(len(self.points)):
            point = self.points[i]
            s.set("A{}".format(i + 2),
                  str(point.disp.getValueAs("kg").Value / 1000.0))
            s.set("B{}".format(i + 2),
                  str(point.draft.getValueAs("m").Value))
            s.set("C{}".format(i + 2),
                  str(point.wet.getValueAs("m^2").Value))
            s.set("D{}".format(i + 2),
                  str(point.mom.getValueAs("kg*m").Value / 1000.0))
            s.set("E{}".format(i + 2),
                  str(point.farea.getValueAs("m^2").Value))
            s.set("F{}".format(i + 2),
                  str(point.xcb.getValueAs("m").Value))
            s.set("G{}".format(i + 2),
                  str(point.KBt.getValueAs("m").Value))
            s.set("H{}".format(i + 2),
                  str(point.BMt.getValueAs("m").Value))
            s.set("I{}".format(i + 2),
                  str(point.Cb))
            s.set("J{}".format(i + 2),
                  str(point.Cf))
            s.set("K{}".format(i + 2),
                  str(point.Cm))

        # Recompute
        FreeCAD.activeDocument().recompute()
