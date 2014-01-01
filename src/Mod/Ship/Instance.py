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

import time
from math import *
from PyQt4 import QtGui,QtCore
from pivy.coin import *
from pivy import coin
import FreeCAD,FreeCADGui
from FreeCAD import Base, Vector
import Part
from shipUtils import Paths, Math

class Ship:
	def __init__(self, obj, solids):
		""" Creates a new ship on active document.
		@param obj Part::FeaturePython created object.
		@param faces Ship solids components.
		"""
		# Add uniqueness property to identify Ship instances
		tooltip = str(QtGui.QApplication.translate("Ship","True if it is a valid ship instance",None,QtGui.QApplication.UnicodeUTF8))
		obj.addProperty("App::PropertyBool","IsShip","Ship", tooltip).IsShip=True
		# Add main dimensions
		tooltip = str(QtGui.QApplication.translate("Ship","Ship length [m]",None,QtGui.QApplication.UnicodeUTF8))
		obj.addProperty("App::PropertyLength","Length","Ship", tooltip).Length=0.0
		tooltip = str(QtGui.QApplication.translate("Ship","Ship breadth [m]",None,QtGui.QApplication.UnicodeUTF8))
		obj.addProperty("App::PropertyLength","Breadth","Ship", tooltip).Breadth=0.0
		tooltip = str(QtGui.QApplication.translate("Ship","Ship draft [m]",None,QtGui.QApplication.UnicodeUTF8))
		obj.addProperty("App::PropertyLength","Draft","Ship", tooltip).Draft=0.0
		# Add shapes
		obj.Shape = Part.makeCompound(solids)
		tooltip = str(QtGui.QApplication.translate("Ship","Set of external faces of the solid",None,QtGui.QApplication.UnicodeUTF8))
		obj.addProperty("Part::PropertyPartShape","ExternalFaces","Ship", tooltip)
		obj.Proxy = self

	def onChanged(self, fp, prop):
		""" Method authomatically called when a property is modified
		@param fp Part::FeaturePython object.
		@param prop Property changed.
		"""
		if prop == "Length" or prop == "Breadth" or prop == "Draft":
			pass

	def execute(self, fp):
		""" Method called when a entity recomputation is requested
		@param fp Part::FeaturePython object.
		"""
		fp.Shape = Part.makeCompound(fp.Shape.Solids)

class ViewProviderShip:
	def __init__(self, obj):
		"Set this object to the proxy object of the actual view provider"
		obj.Proxy = self

	def attach(self, obj):
		''' Setup the scene sub-graph of the view provider, this method is mandatory '''
		return

	def updateData(self, fp, prop):
		''' If a property of the handled feature has changed we have the chance to handle this here '''
		return

	def getDisplayModes(self,obj):
		''' Return a list of display modes. '''
		modes=[]
		return modes

	def getDefaultDisplayMode(self):
		''' Return the name of the default display mode. It must be defined in getDisplayModes. '''
		return "Shaded"

	def setDisplayMode(self,mode):
		''' Map the display mode defined in attach with those defined in getDisplayModes.
		Since they have the same names nothing needs to be done. This method is optinal.
		'''
		return mode

	def onChanged(self, vp, prop):
		''' Print the name of the property that has changed '''
		# FreeCAD.Console.PrintMessage("Change property: " + str(prop) + "\n")

	def __getstate__(self):
		''' When saving the document this object gets stored using Python's cPickle module.
		Since we have some un-pickable here -- the Coin stuff -- we must define this method
		to return a tuple of all pickable objects or None.
		'''
		return None

	def __setstate__(self,state):
		''' When restoring the pickled object from document we have the chance to set some
		internals here. Since no data were pickled nothing needs to be done here.
		'''
		return None

	def getIcon(self):
		return ":/icons/Ship_Instance.svg"

def weights(obj):
	""" Returns Ship weights list. If weights has not been sets, 
	this tool creates it.
	@param obj Ship object
	@return Weights list. None if errors
	"""
	# Test if is a ship instance
	props = obj.PropertiesList
	try:
		props.index("IsShip")
	except ValueError:
		return None
	if not obj.IsShip:
		return None
	# Test if properties already exist
	try:
		props.index("WeightNames")
	except ValueError:
		tooltip = str(QtGui.QApplication.translate("Ship","Ship Weights names",None,QtGui.QApplication.UnicodeUTF8))
		lighweight = str(QtGui.QApplication.translate("Ship","Lightweight",None,QtGui.QApplication.UnicodeUTF8))
		obj.addProperty("App::PropertyStringList","WeightNames","Ship", tooltip).WeightNames=[lighweight]
	try:
		props.index("WeightMass")
	except ValueError:
		# Compute mass aproximation
		from shipHydrostatics import Tools
		disp = Tools.displacement(obj,obj.Draft)
		tooltip = str(QtGui.QApplication.translate("Ship","Ship Weights masses",None,QtGui.QApplication.UnicodeUTF8))
		obj.addProperty("App::PropertyFloatList","WeightMass","Ship", tooltip).WeightMass=[1000.0 * disp[0]]
	try:
		props.index("WeightPos")
	except ValueError:
		# Compute mass aproximation
		from shipHydrostatics import Tools
		disp = Tools.displacement(obj,obj.Draft)
		tooltip = str(QtGui.QApplication.translate("Ship","Ship Weights centers of gravity",None,QtGui.QApplication.UnicodeUTF8))
		obj.addProperty("App::PropertyVectorList","WeightPos","Ship", tooltip).WeightPos=[Vector(disp[1].x,0.0,obj.Draft)]
	# Setup list
	weights = []
	for i in range(0,len(obj.WeightNames)):
		weights.append([obj.WeightNames[i], obj.WeightMass[i], obj.WeightPos[i]])
	return weights
