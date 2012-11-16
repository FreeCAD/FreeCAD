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
# Qt library
from PyQt4 import QtGui,QtCore
# FreeCAD modules
import FreeCAD,FreeCADGui
from FreeCAD import Base
import Part, Image, ImageGui
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
	def __init__(self, x, y, disp, draft, trim):
		""" Constructor. performs plot and show it (Using pyxplot).
		@param x Roll angles [deg].
		@param y GZ value [m].
		@param disp Ship displacement [tons].
		@param draft Ship draft [m].
		@param trim Ship trim angle [deg].
		"""
		# Try to plot
		self.plot(x,y,disp,draft,trim)
		# Save data
		if self.createDirectory():
			return
		if self.saveData(x,y):
			return

	def plot(self, x, y, disp, draft, trim):
		""" Perform GZ stability plot.
		@param x X coordinates.
		@param y Transversal areas.
		@param disp Ship displacement [tons].
		@param draft Ship draft [m].
		@param trim Ship trim angle [deg].
		@return True if error happens.
		"""
		# Create plot
		try:
			import Plot
			plt = Plot.figure('GZ')
		except ImportError:
			msg = QtGui.QApplication.translate("ship_console", "Plot module is disabled, can't perform plot",
                                       None,QtGui.QApplication.UnicodeUTF8)
			FreeCAD.Console.PrintWarning(msg + '\n')
			return True
		# Plot areas curve
		gz = Plot.plot(x,y,r'GZ')
		gz.line.set_linestyle('-')
		gz.line.set_linewidth(3.0)
		gz.line.set_color((0.0, 0.0, 0.0))
		# Add some additional data
		ax = Plot.axes()
		addInfo = r"""$\bigtriangleup = %g \; \mathrm{tons}$
$T = %g \; \mathrm{m}$
$Trim = %g^\circ$""" % (disp, draft, trim)
		ax.text(x[-1] - 0.001*(x[-1] - x[0]), max(y) - 0.01*(max(y)-min(y)), addInfo,
				verticalalignment='top',horizontalalignment='right', fontsize=20)
		# Write axes titles
		Plot.xlabel(r'$x \; \mathrm{m}$')
		Plot.ylabel(r'$GZ \; \mathrm{m}$')
		ax.xaxis.label.set_fontsize(20)
		ax.yaxis.label.set_fontsize(20)
		# Show grid
		Plot.grid(True)
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
			return True
		return False

	def saveData(self,x,y):
		""" Write data file.
		@param x Roll angles.
		@param y GZ value.
		@return True if error happens.
		"""
		# Open the file
		filename = self.path + 'gz.dat'
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
		Output.write(" # This file contains transversal GZ stability parameter, filled with following columns:\n")
		Output.write(" # 1: Roll angles [deg]\n")
		Output.write(" # 2: GZ [m]\n")
		Output.write(" #\n")
		Output.write(" #################################################################\n")
		# Print data
		for i in range(0, len(x)):
			string = "%f %f\n" % (x[i], y[i])
			Output.write(string)
		# Close file
		Output.close()
		self.dataFile = filename
		msg = QtGui.QApplication.translate("ship_console", "Data saved",
                                   None,QtGui.QApplication.UnicodeUTF8)
		FreeCAD.Console.PrintMessage(msg + ':\n\t' + "\'"+ self.dataFile + "\'\n")
		return False

