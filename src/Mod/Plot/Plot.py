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

# FreeCAD
import FreeCAD

# PyQt4
from PyQt4 import QtCore, QtGui

# Matplot lib
try:
	import matplotlib
	import matplotlib.pyplot as plt
	from matplotlib.backends.backend_qt4agg import FigureCanvasQTAgg as FigureCanvas
	from matplotlib.backends.backend_qt4agg import NavigationToolbar2QTAgg as NavigationToolbar
	from matplotlib.figure import Figure
except ImportError:
	msg = QtGui.QApplication.translate("plot_console", "matplotlib not found, so Plot module can not be loaded",
                                       None,QtGui.QApplication.UnicodeUTF8)
	FreeCAD.Console.PrintMessage(msg + '\n')
	raise ImportError("matplotlib not installed")

def getMainWindow():
	""" getMainWindow(): Gets FreeCAD main window. """
	toplevel = QtGui.qApp.topLevelWidgets()
	for i in toplevel:
		if i.metaObject().className() == "Gui::MainWindow":
			return i
	return None

def getMdiArea():
	""" getMdiArea(): Gets FreeCAD MdiArea. """
	mw = getMainWindow()
	if not mw:
		return None
	return mw.findChild(QtGui.QMdiArea)

def getPlot():
	""" getPlot(): Gets selected Plot document if exist. """
	# Get active tab
	mdi = getMdiArea()
	if not mdi:
		return None
	sub = mdi.activeSubWindow()
	if not sub:
		return None
	# Explore childrens looking for Plot class
	for i in sub.children():
		if i.metaObject().className() == "Plot":
			return i
	return None

def figure(winTitle="plot"):
	""" figure(winTitle="plot"): Create a new plot subwindow.\n winTitle = Tab title. """
	mdi = getMdiArea()
	if not mdi:
		return None
	win = Plot(winTitle)
	sub=mdi.addSubWindow(win)
	sub.show()
	return win

def plot(x,y,name=None):
	""" plot(x,y,name=None): Plots a new serie (as line plot)\n x = X values\n y = Y values\n name = Serie name (for legend). """
	# Get active plot, or create another one if don't exist
	plt = getPlot()
	if not plt:
		plt = figure()
	# Call to plot
	return plt.plot(x,y,name)

def series():
	""" lines(): Get all lines from selected plot. """
	plt = getPlot()
	if not plt:
		return []
	return plt.series

def removeSerie(index):
	""" removeSerie(index): Removes a serie from plot.\n index = Index of serie to remove. """
	# Get active series
	plt = getPlot()
	if not plt:
		return
	plots = plt.series
	if not plots:
		return
	# Remove line from plot
	axes = plots[index].axes
	axes.lines.pop(plots[index].lid)
	# Remove serie from list
	del plt.series[index]
	# Update GUI
	plt.update()

def legend(status=True, pos=None, fontsize=None):
	""" legend(status=True): Show/Hide legend.\n status = True if legend must be shown, False otherwise.\n pos = Legend position.\n fontsize = Font size """
	plt = getPlot()
	if not plt:
		return
	plt.legend = status
	if fontsize:
		plt.legSiz = fontsize
	# Hide all legends
	for axes in plt.axesList:
		axes.legend_ = None
	# Legend must be activated on last axes
	axes = plt.axesList[-1]
	if status:
		# Setup legend handles and names
		lines   = series()
		handles = []
		names   = []
		for l in lines:
			if l.name != None:
				handles.append(l.line)
				names.append(l.name)
		# Show the legend (at selected position or at best)
		if pos:
			l = axes.legend(handles, names, bbox_to_anchor=pos)
			plt.legPos = pos
		else:
			l = axes.legend(handles, names, loc='best')
			# Update canvas in order to compute legend data
			plt.canvas.draw()
			# Get resultant position
			fax = axes.get_frame().get_extents()
			fl  = l.get_frame()
			plt.legPos = ((fl._x+fl._width-fax.x0) / fax.width, (fl._y+fl._height-fax.y0) / fax.height)
		# Set fontsize
		for t in l.get_texts():
			t.set_fontsize(plt.legSiz)
	plt.update()

def grid(status=True):
	""" grid(status=True): Show/Hide grid.\n status = True if grid must be shown, False otherwise. """
	plt = getPlot()
	if not plt:
		return
	plt.grid = status
	axes = plt.axes
	axes.grid(status)
	plt.update()

def title(string):
	""" title(string): Setup plot title.\n string = Title to set. """
	plt = getPlot()
	if not plt:
		return
	axes = plt.axes
	axes.set_title(string)
	plt.update()

def xlabel(string):
	""" xlabel(string): Setup x label.\n string = Title to set. """
	plt = getPlot()
	if not plt:
		return
	axes = plt.axes
	axes.set_xlabel(string)
	plt.update()

def ylabel(string):
	""" ylabel(string): Setup y label.\n string = Title to set. """
	plt = getPlot()
	if not plt:
		return
	axes = plt.axes
	axes.set_ylabel(string)
	plt.update()

def axesList():
	""" axesList(): Gets plot axes list. """
	plt = getPlot()
	if not plt:
		return []
	return plt.axesList

def axes():
	""" axes(): Gets active plot axes. """
	plt = getPlot()
	if not plt:
		return None
	return plt.axes

def addNewAxes(rect=None, frameon=True, patchcolor='none'):
	""" addNewAxes(pos=None, frameon=True): Add new axes to plot, setting it as active one.\n rect = Axes area, None to copy last axes data.\n frameon = True to show frame, False otherwise.\n patchcolor = Patch color, 'none' for transparent plot. """
	plt = getPlot()
	if not plt:
		return None
	fig = plt.fig
	if rect == None:
		rect = plt.axes.get_position()
	ax = fig.add_axes(rect, frameon=frameon)
	ax.xaxis.set_ticks_position('bottom')
	ax.spines['top'].set_color('none')
	ax.yaxis.set_ticks_position('left')
	ax.spines['right'].set_color('none')
	ax.patch.set_facecolor(patchcolor)
	plt.axesList.append(ax)
	plt.setActiveAxes(-1)
	plt.update()
	return ax

def save(path, figsize=None, dpi=None):
	""" save(path): Save plot.\n path = Destination file path.\n figsize = w,h figure size tuple in inches.\n dpi = Dots per inch."""
	plt = getPlot()
	if not plt:
		return
	# Backup figure options
	fig = plt.fig
	sizeBack = fig.get_size_inches()
	dpiBack  = fig.get_dpi()
	# Save figure with new options
	if figsize:
		fig.set_size_inches(figsize[0], figsize[1])
	if dpi:
		fig.set_dpi(dpi)
	plt.canvas.print_figure(path)
	# Restore figure options
	fig.set_size_inches(sizeBack[0], sizeBack[1])
	fig.set_dpi(dpiBack)
	plt.update()

class Line():
	def __init__(self, axes, x, y, name):
		""" __init__(axes, x, y, name): Construct new plot serie.\n axes = Active axes\n x = X values\n y = Y values\n name = Serie name (for legend). """
		self.axes  = axes
		self.x     = x
		self.y     = y
		self.name  = name
		self.lid   = len(axes.lines)
		self.line, = axes.plot(x,y)
	
	def setp(self, prop, value):
		""" setp(prop, value): Change line property value.\n prop = Property name.\n value = New property value. """
		plt.setp(self.line, prop, value)
	
	def getp(self, prop):
		""" getp(prop): Get property value.\n  prop = Property name."""
		return plt.getp(self.line, prop)

class Plot(QtGui.QWidget):
	def __init__(self, winTitle="plot", parent = None, flags = QtCore.Qt.WindowFlags(0)):
		""" __init__(winTitle="plot", parent = None, flags = Qt.WindowFlags(0)): Construct a new plot widget.\n winTitle = Tab title.\n parent = Widget parent.\n flags = QWidget flags"""
		QtGui.QWidget.__init__(self, parent, flags)
		self.setWindowTitle(winTitle)
		# Create matplotlib canvas
		self.fig = Figure()
		self.canvas = FigureCanvas(self.fig)
		self.canvas.setParent(self)
		# Get axes
		self.axes     = self.fig.add_subplot(111)
		self.axesList = [self.axes]
		self.axes.xaxis.set_ticks_position('bottom')
		self.axes.spines['top'].set_color('none')
		self.axes.yaxis.set_ticks_position('left')
		self.axes.spines['right'].set_color('none')
		# Setup layout
		vbox = QtGui.QVBoxLayout()
		vbox.addWidget(self.canvas)
		self.setLayout(vbox)
		# Active series
		self.series = []
		# Indicators
		self.skip   = False
		self.legend = False
		self.legPos = (1.0,1.0)
		self.legSiz = 14
		self.grid   = False
	
	def plot(self, x, y, name=None):
		""" plot(self, x, y, name=None): Plot a new line and return it.\n x = X values\n y = Y values\n name = Serie name (for legend). """
		l = Line(self.axes, x, y, name)
		self.series.append(l)
		# Update window
		self.update()
		return l

	def update(self):
		""" update(): Updates plot. """
		if not self.skip:
			self.skip = True
			if self.legend:
				legend(self.legend, self.legPos, self.legSiz)
			self.canvas.draw()
			self.skip = False

	def isGrid(self):
		""" isGrid(): Return True if Grid is active, False otherwise. """
		return bool(self.grid)

	def isLegend(self):
		""" isLegend(): Return True if Legend is active, False otherwise. """
		return bool(self.legend)

	def setActiveAxes(self, index):
		""" setActiveAxes(index): Change current active axes.\n index = Index of the new active axes. """
		self.axes = self.axesList[index]
		self.fig.sca(self.axes)

