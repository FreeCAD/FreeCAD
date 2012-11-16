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
from FreeCAD import Base, Vector
import Part, Image, ImageGui
# FreeCADShip modules
from shipUtils import Paths
import Tools

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
	def __init__(self, ship, trim, drafts):
		""" Constructor. performs plot and show it (Using pyxplot).
		@param ship Selected ship instance
		@param trim Trim in degrees.
		@param drafts List of drafts to be performed.
		"""
		# Compute data
		# Get external faces
		faces = self.externalFaces(ship.Shape)
		if len(faces) == 0:
			msg = QtGui.QApplication.translate("ship_console", "Can't detect external faces from ship object",
                                       None,QtGui.QApplication.UnicodeUTF8)
			FreeCAD.Console.PrintError(msg + '\n')
			return
		else:
			faces = Part.makeShell(faces)
		# Print data
		msg = QtGui.QApplication.translate("ship_console", "Computing hydrostatics",
                                   None,QtGui.QApplication.UnicodeUTF8)
		FreeCAD.Console.PrintMessage(msg + '...\n')
		self.points = []
		for i in range(0,len(drafts)):
			FreeCAD.Console.PrintMessage("\t%d / %d\n" % (i+1, len(drafts)))
			draft = drafts[i]
			point = Tools.Point(ship,faces,draft,trim)
			self.points.append(point)
		# Try to plot
		self.plotVolume()
		self.plotStability()
		self.plotCoeffs()
		# Save data
		if self.createDirectory():
			return
		if self.saveData(ship, trim, drafts):
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

	def saveData(self, ship, trim, drafts):
		""" Write data file.
		@param ship Selected ship instance
		@param trim Trim in degrees.
		@param drafts List of drafts to be performed.
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
		for i in range(0,len(drafts)):
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

	def lineFaceSection(self,line,surface):
		""" Returns the point of section of a line with a face
		@param line Line object, that can be a curve.
		@param surface Surface object (must be a Part::Shape)
		@return Section points array, [] if line don't cut surface
		"""
		# Get initial data
		result = []
		vertexes = line.Vertexes
		nVertex = len(vertexes)
		# Perform the cut
		section = line.cut(surface)
		# Filter all old points
		points = section.Vertexes
		return points

	def externalFaces(self, shape):
		""" Returns detected external faces.
		@param shape Shape where external faces wanted.
		@return List of external faces detected.
		"""
		result = []
		faces  = shape.Faces
		bbox   = shape.BoundBox
		L	  = bbox.XMax - bbox.XMin
		B	  = bbox.YMax - bbox.YMin
		T	  = bbox.ZMax - bbox.ZMin
		dist   = math.sqrt(L*L + B*B + T*T)
		msg = QtGui.QApplication.translate("ship_console", "Computing external faces",
                                   None,QtGui.QApplication.UnicodeUTF8)
		FreeCAD.Console.PrintMessage(msg + '...\n')
		# Valid/unvalid faces detection loop
		for i in range(0,len(faces)):
			FreeCAD.Console.PrintMessage("\t%d / %d\n" % (i+1, len(faces)))
			f = faces[i]
			# Create a line normal to surface at middle point
			u = 0.0
			v = 0.0
			try:
				surf	= f.Surface
				u	   = 0.5*(surf.getUKnots()[0]+surf.getUKnots()[-1])
				v	   = 0.5*(surf.getVKnots()[0]+surf.getVKnots()[-1])
			except:
				cog   = f.CenterOfMass
				[u,v] = f.Surface.parameter(cog)
			p0 = f.valueAt(u,v)
			try:
				n  = f.normalAt(u,v).normalize()
			except:
				continue
			p1 = p0 + n.multiply(1.5*dist)
			line = Part.makeLine(p0, p1)
			# Look for faces in front of this
			nPoints = 0
			for j in range(0,len(faces)):
				f2 = faces[j]
				section = self.lineFaceSection(line, f2)
				if len(section) <= 2:
					continue
				# Add points discarding start and end
				nPoints = nPoints + len(section) - 2
			# In order to avoid special directions we can modify line
			# normal a little bit.
			angle = 5
			line.rotate(p0,Vector(1,0,0),angle)
			line.rotate(p0,Vector(0,1,0),angle)
			line.rotate(p0,Vector(0,0,1),angle)
			nPoints2 = 0
			for j in range(0,len(faces)):
				if i == j:
					continue
				f2 = faces[j]
				section = self.lineFaceSection(line, f2)
				if len(section) <= 2:
					continue
				# Add points discarding start and end
				nPoints2 = nPoints + len(section) - 2
			# If the number of intersection points is pair, is a
			# external face. So if we found an odd points intersection,
			# face must be discarded.
			if (nPoints % 2) or (nPoints2 % 2):
				continue
			result.append(f)
		return result
