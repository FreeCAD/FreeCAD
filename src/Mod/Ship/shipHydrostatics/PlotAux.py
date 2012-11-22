#***************************************************************************
#*																		 *
#*   Copyright (c) 2011, 2012											  *  
#*   Jose Luis Cercos Pita <jlcercos@gmail.com>							*  
#*																		 *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)	*
#*   as published by the Free Software Foundation; either version 2 of	 *
#*   the License, or (at your option) any later version.				   *
#*   for detail see the LICENCE text file.								 *
#*																		 *
#*   This program is distributed in the hope that it will be useful,	   *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of		*
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the		 *
#*   GNU Library General Public License for more details.				  *
#*																		 *
#*   You should have received a copy of the GNU Library General Public	 *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA																   *
#*																		 *
#***************************************************************************

import os
import math
# Qt library
from PyQt4 import QtGui,QtCore
# FreeCAD modules
import FreeCAD,FreeCADGui
# FreeCADShip modules
from shipUtils import Paths

header = """ #################################################################

 #####				 ####  ###   ####	  ##### #   # ### ####
 #					#	  # #   #   #	#	  #   #  #  #   #
 #	 ##  #### ####  #	 #   #  #   #	 #	 #   #  #  #   #
 ####  # # #  # #  #  #	 #####  #   # ##   ##   #####  #  ####
 #	 #   #### ####  #	#	 # #   #		#  #   #  #  #
 #	 #   #	#	 #	#	 # #   #		 # #   #  #  #
 #	 #   #### ####   ### #	 # ####	 #####  #   # ### #

 #################################################################
"""

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
		if self.createDirectory():
			return
		if self.saveData(ship, trim):
			return

	def plotVolume(self):
		""" Perform volumetric hydrostatics.
		@return True if error happens.
		"""
		# Create plot
		try:
			import Plot
			plt = Plot.figure('Volume')
		except ImportError:
			msg = QtGui.QApplication.translate("ship_console", "Plot module is disabled, can't perform plot",
                                       None,QtGui.QApplication.UnicodeUTF8)
			FreeCAD.Console.PrintWarning(msg + '\n')
			return True
		# Generate sets of axes
		Plot.grid(True)
		for i in range(0,3):
			ax = Plot.addNewAxes()
			# Y axes can be moved to right
			ax.yaxis.tick_right()
			ax.spines['right'].set_color((0.0,0.0,0.0))
			ax.spines['left'].set_color('none')
			ax.yaxis.set_ticks_position('right')
			ax.yaxis.set_label_position('right')
			# And X axes moved down with an offset
			for loc, spine in ax.spines.iteritems():
				if loc in ['bottom', 'top']:
					spine.set_position(('outward',(i+1)*35))
			Plot.grid(True)
		# Setup data
		disp  = []
		draft = []
		warea = []
		t1cm  = []
		xcb   = []
		for i in range(0,len(self.points)):
			disp.append(self.points[i].disp)
			draft.append(self.points[i].draft)
			warea.append(self.points[i].wet)
			t1cm.append(self.points[i].mom)
			xcb.append(self.points[i].xcb)
		# Set plot size
		axes = Plot.axesList()
		for ax in axes:
			ax.set_position([0.1, 0.2, 0.8, 0.75])
		# Plot curves
		plt.axes = axes[0]
		serie = Plot.plot(draft,disp,r'$T$')
		serie.line.set_linestyle('-')
		serie.line.set_linewidth(2.0)
		serie.line.set_color((0.0,0.0,0.0))
		Plot.xlabel(r'$T \; \mathrm{m}$')
		Plot.ylabel(r'$\bigtriangleup \; \mathrm{tons}$')
		plt.axes.xaxis.label.set_fontsize(15)
		plt.axes.yaxis.label.set_fontsize(15)
		plt.axes = axes[1]
		serie = Plot.plot(warea,disp,r'Wetted area')
		serie.line.set_linestyle('-')
		serie.line.set_linewidth(2.0)
		serie.line.set_color((1.0,0.0,0.0))
		Plot.xlabel(r'$Wetted \; area \; \mathrm{m}^2$')
		Plot.ylabel(r'$\bigtriangleup \; \mathrm{tons}$')
		plt.axes.xaxis.label.set_fontsize(15)
		plt.axes.yaxis.label.set_fontsize(15)
		plt.axes = axes[2]
		serie = Plot.plot(t1cm,disp,r'Moment to trim 1cm')
		serie.line.set_linestyle('-')
		serie.line.set_linewidth(2.0)
		serie.line.set_color((0.0,0.0,1.0))
		Plot.xlabel(r'$Moment \; to \; trim \; 1 \mathrm{cm} \; \mathrm{tons} \; \times \; \mathrm{m}$')
		plt.axes.xaxis.label.set_fontsize(15)
		plt.axes.yaxis.label.set_fontsize(15)
		plt.axes = axes[3]
		serie = Plot.plot(xcb,disp,r'$XCB$')
		serie.line.set_linestyle('-')
		serie.line.set_linewidth(2.0)
		serie.line.set_color((0.2,0.8,0.2))
		Plot.xlabel(r'$XCB \; \mathrm{m}$')
		plt.axes.xaxis.label.set_fontsize(15)
		plt.axes.yaxis.label.set_fontsize(15)
		# Show legend
		Plot.legend(True)
		# End
		plt.update()
		return False

	def plotStability(self):
		""" Perform stability hydrostatics.
		@return True if error happens.
		"""
		# Create plot
		try:
			import Plot
			plt = Plot.figure('Stability')
		except ImportError:
			return True
		# Generate sets of axes
		Plot.grid(True)
		for i in range(0,3):
			ax = Plot.addNewAxes()
			# Y axes can be moved to right
			ax.yaxis.tick_right()
			ax.spines['right'].set_color((0.0,0.0,0.0))
			ax.spines['left'].set_color('none')
			ax.yaxis.set_ticks_position('right')
			ax.yaxis.set_label_position('right')
			# And X axes moved down with an offset
			for loc, spine in ax.spines.iteritems():
				if loc in ['bottom', 'top']:
					spine.set_position(('outward',(i+1)*35))
			Plot.grid(True)
		# Setup data
		disp  = []
		draft = []
		farea = []
		kbt   = []
		bmt   = []
		for i in range(0,len(self.points)):
			disp.append(self.points[i].disp)
			draft.append(self.points[i].draft)
			farea.append(self.points[i].farea)
			kbt.append(self.points[i].KBt)
			bmt.append(self.points[i].BMt)
		# Set plot size
		axes = Plot.axesList()
		for ax in axes:
			ax.set_position([0.1, 0.2, 0.8, 0.75])
		# Plot curves
		plt.axes = axes[0]
		serie = Plot.plot(draft,disp,r'$T$')
		serie.line.set_linestyle('-')
		serie.line.set_linewidth(2.0)
		serie.line.set_color((0.0,0.0,0.0))
		Plot.xlabel(r'$T \; \mathrm{m}$')
		Plot.ylabel(r'$\bigtriangleup \; \mathrm{tons}$')
		plt.axes.xaxis.label.set_fontsize(15)
		plt.axes.yaxis.label.set_fontsize(15)
		plt.axes = axes[1]
		serie = Plot.plot(farea,disp,r'Floating area')
		serie.line.set_linestyle('-')
		serie.line.set_linewidth(2.0)
		serie.line.set_color((1.0,0.0,0.0))
		Plot.xlabel(r'$Floating \; area \; \mathrm{m}^2$')
		Plot.ylabel(r'$\bigtriangleup \; \mathrm{tons}$')
		plt.axes.xaxis.label.set_fontsize(15)
		plt.axes.yaxis.label.set_fontsize(15)
		plt.axes = axes[2]
		serie = Plot.plot(kbt,disp,r'$KB_T$')
		serie.line.set_linestyle('-')
		serie.line.set_linewidth(2.0)
		serie.line.set_color((0.0,0.0,1.0))
		Plot.xlabel(r'$KB_T \; \mathrm{m}$')
		plt.axes.xaxis.label.set_fontsize(15)
		plt.axes.yaxis.label.set_fontsize(15)
		plt.axes = axes[3]
		serie = Plot.plot(bmt,disp,r'$BM_T$')
		serie.line.set_linestyle('-')
		serie.line.set_linewidth(2.0)
		serie.line.set_color((0.2,0.8,0.2))
		Plot.xlabel(r'$BM_T \; \mathrm{m}$')
		plt.axes.xaxis.label.set_fontsize(15)
		plt.axes.yaxis.label.set_fontsize(15)
		# Show legend
		Plot.legend(True)
		# End
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
		# Generate sets of axes
		Plot.grid(True)
		for i in range(0,3):
			ax = Plot.addNewAxes()
			# Y axes can be moved to right
			ax.yaxis.tick_right()
			ax.spines['right'].set_color((0.0,0.0,0.0))
			ax.spines['left'].set_color('none')
			ax.yaxis.set_ticks_position('right')
			ax.yaxis.set_label_position('right')
			# And X axes moved down with an offset
			for loc, spine in ax.spines.iteritems():
				if loc in ['bottom', 'top']:
					spine.set_position(('outward',(i+1)*40))
			Plot.grid(True)
		# Setup data
		disp  = []
		draft = []
		cb	= []
		cf	= []
		cm	= []
		for i in range(0,len(self.points)):
			disp.append(self.points[i].disp)
			draft.append(self.points[i].draft)
			cb.append(self.points[i].Cb)
			cf.append(self.points[i].Cf)
			cm.append(self.points[i].Cm)
		# Set plot size
		axes = Plot.axesList()
		for ax in axes:
			ax.set_position([0.1, 0.2, 0.8, 0.75])
		# Plot curves
		plt.axes = axes[0]
		serie = Plot.plot(draft,disp,r'$T$')
		serie.line.set_linestyle('-')
		serie.line.set_linewidth(2.0)
		serie.line.set_color((0.0,0.0,0.0))
		Plot.xlabel(r'$T \; \mathrm{m}$')
		Plot.ylabel(r'$\bigtriangleup \; \mathrm{tons}$')
		plt.axes.xaxis.label.set_fontsize(15)
		plt.axes.yaxis.label.set_fontsize(15)
		plt.axes = axes[1]
		serie = Plot.plot(cb,disp,r'$Cb$')
		serie.line.set_linestyle('-')
		serie.line.set_linewidth(2.0)
		serie.line.set_color((1.0,0.0,0.0))
		Plot.xlabel(r'$Cb$ (Block coefficient)')
		Plot.ylabel(r'$\bigtriangleup \; \mathrm{tons}$')
		plt.axes.xaxis.label.set_fontsize(15)
		plt.axes.yaxis.label.set_fontsize(15)
		plt.axes = axes[2]
		serie = Plot.plot(cf,disp,r'$Cf$')
		serie.line.set_linestyle('-')
		serie.line.set_linewidth(2.0)
		serie.line.set_color((0.0,0.0,1.0))
		Plot.xlabel(r'$Cf$ (floating area coefficient)')
		plt.axes.xaxis.label.set_fontsize(15)
		plt.axes.yaxis.label.set_fontsize(15)
		plt.axes = axes[3]
		serie = Plot.plot(cm,disp,r'$Cm$')
		serie.line.set_linestyle('-')
		serie.line.set_linewidth(2.0)
		serie.line.set_color((0.2,0.8,0.2))
		Plot.xlabel(r'$Cm$  (Main section coefficient)')
		plt.axes.xaxis.label.set_fontsize(15)
		plt.axes.yaxis.label.set_fontsize(15)
		# Show legend
		Plot.legend(True)
		# End
		plt.update()
		return False

	def createDirectory(self):
		""" Create needed folder to write data and scripts.
		@return True if error happens.
		"""
		self.path = FreeCAD.ConfigGet("UserAppData") + "ShipOutput/"
		if not os.path.exists(self.path):
			os.makedirs(self.path)
		if not os.path.exists(self.path):
			msg = QtGui.QApplication.translate("ship_console", "Can't create folder",
                                       None,QtGui.QApplication.UnicodeUTF8)
			FreeCAD.Console.PrintError(msg + ':\n\t' + "\'"+ self.path + "\'\n")
		return False

	def saveData(self, ship, trim):
		""" Write data file.
		@param ship Selected ship instance
		@param trim Trim in degrees.
		@return True if error happens.
		"""
		# Open the file
		filename = self.path + 'hydrostatics.dat'
		try:
			Output = open(filename, "w")
		except IOError:
			msg = QtGui.QApplication.translate("ship_console", "Can't write to file",
                                       None,QtGui.QApplication.UnicodeUTF8)
			FreeCAD.Console.PrintError(msg + ':\n\t' + "\'"+ filename + "\'\n")
			return True
		# Print header
		Output.write(header)
		Output.write(" #\n")
		Output.write(" # File automatically exported by FreeCAD-Ship\n")
		Output.write(" # This file contains transversal areas data, filled with following columns:\n")
		Output.write(" #  1: Ship displacement [ton]\n")
		Output.write(" #  2: Draft [m]\n")
		Output.write(" #  3: Wetted surface [m2]\n")
		Output.write(" #  4: 1cm triming ship moment [ton m]\n")
		Output.write(" #  5: Bouyance center x coordinate\n")
		Output.write(" #  6: Floating area\n")
		Output.write(" #  7: KBt\n")
		Output.write(" #  8: BMt\n")
		Output.write(" #  9: Cb (block coefficient)\n")
		Output.write(" # 10: Cf (Floating coefficient)\n")
		Output.write(" # 11: Cm (Main frame coefficient)\n")
		Output.write(" #\n")
		Output.write(" #################################################################\n")
		# Print data
		for i in range(0,len(self.points)):
			point  = self.points[i]
			string = "%f %f %f %f %f %f %f %f %f %f %f\n" % (point.disp, point.draft, point.wet, point.mom, point.xcb, point.farea, point.KBt, point.BMt, point.Cb, point.Cf, point.Cm)
			Output.write(string)
		# Close file
		Output.close()
		self.dataFile = filename
		msg = QtGui.QApplication.translate("ship_console", "Data saved",
                                   None,QtGui.QApplication.UnicodeUTF8)
		FreeCAD.Console.PrintMessage(msg + ':\n\t' + "\'"+ self.dataFile + "\'\n")
		return False

