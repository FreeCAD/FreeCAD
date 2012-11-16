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

# Qt library
from PyQt4 import QtGui,QtCore
# FreeCAD modules
import FreeCAD,FreeCADGui
from FreeCAD import Base, Vector
import Part
# FreeCADShip modules
from shipUtils import Paths

def Plot(scale, sections, shape):
	""" Creates the outline draw.
	@param scale Plane scale (format 1:scale)
	@param sections Sections computed.
	@param shape Ship surfaces shell
	@return plotted object (DocumentObject)
	"""
	msg = QtGui.QApplication.translate("ship_console", "Performing plot",
								   None,QtGui.QApplication.UnicodeUTF8)
	FreeCAD.Console.PrintMessage(msg + ' (1:%d)...\n' % (scale))
	scale = 1000.0 / scale
	# Take positions
	bounds = [0.0, 0.0, 0.0]
	bbox = shape.BoundBox
	bounds[0] = bbox.XLength
	bounds[1] = bbox.YLength
	bounds[2] = bbox.ZLength
	xTot = scale*bounds[1] + 32.0 + scale*bounds[0]
	yTot = scale*bounds[2] + 32.0 + scale*bounds[1]
	xMid = 210.0
	yMid = 185.0
	x0 = xMid - 0.5*xTot
	y0 = 297.0 - yMid - 0.5*yTot # 297 = A3_width
	# Get border
	edges = getEdges([shape])
	border = edges[0]
	for i in range(0,len(edges)):
		border = border.oldFuse(edges[i])   # Only group objects, don't try to build more complex entities
		border = border.oldFuse(edges[i].mirror(Vector(0.0, 0.0, 0.0),Vector(0.0, 1.0, 0.0)))
	# Fuse sections & borders
	# obj = sections.oldFuse(border)
	obj = border.oldFuse(sections)
	# Send to 3D view
	Part.show(obj)
	objs = FreeCAD.ActiveDocument.Objects
	obj = objs[len(objs)-1]
	# Create a new plane
	FreeCAD.ActiveDocument.addObject('Drawing::FeaturePage','OutlineDrawPlot')
	FreeCAD.ActiveDocument.OutlineDrawPlot.Template = FreeCAD.getResourceDir()+'Mod/Drawing/Templates/A3_Landscape.svg'
	# Side view
	FreeCAD.ActiveDocument.addObject('Drawing::FeatureViewPart','OutlineDrawSideView')
	FreeCAD.ActiveDocument.OutlineDrawSideView.Source = obj
	FreeCAD.ActiveDocument.OutlineDrawSideView.Direction = (1.0,0.0,0.0)
	FreeCAD.ActiveDocument.OutlineDrawSideView.Rotation = -90.0
	FreeCAD.ActiveDocument.OutlineDrawSideView.Scale = scale
	FreeCAD.ActiveDocument.OutlineDrawSideView.X = 420.0 - x0 - 0.5*scale*bounds[1] # 420 = A3_height
	FreeCAD.ActiveDocument.OutlineDrawSideView.Y = y0 + 0.5*scale*bounds[2]
	FreeCAD.ActiveDocument.OutlineDrawPlot.addObject(FreeCAD.ActiveDocument.OutlineDrawSideView)
	# Front view
	FreeCAD.ActiveDocument.addObject('Drawing::FeatureViewPart','OutlineDrawFrontView')
	FreeCAD.ActiveDocument.OutlineDrawFrontView.Source = obj
	FreeCAD.ActiveDocument.OutlineDrawFrontView.Direction = (0.0,1.0,0.0)
	FreeCAD.ActiveDocument.OutlineDrawFrontView.Rotation = -90.0
	FreeCAD.ActiveDocument.OutlineDrawFrontView.Scale = scale
	FreeCAD.ActiveDocument.OutlineDrawFrontView.X = 420.0 - x0 - scale*bounds[1] - 32 - 0.5*scale*bounds[0]
	FreeCAD.ActiveDocument.OutlineDrawFrontView.Y = y0 + 0.5*scale*bounds[2]
	FreeCAD.ActiveDocument.OutlineDrawPlot.addObject(FreeCAD.ActiveDocument.OutlineDrawFrontView)
	# Up view
	FreeCAD.ActiveDocument.addObject('Drawing::FeatureViewPart','OutlineDrawUpView')
	FreeCAD.ActiveDocument.OutlineDrawUpView.Source = obj
	FreeCAD.ActiveDocument.OutlineDrawUpView.Direction = (0.0,0.0,1.0)
	FreeCAD.ActiveDocument.OutlineDrawUpView.Scale = scale
	FreeCAD.ActiveDocument.OutlineDrawUpView.X = 420.0 - x0 - scale*bounds[1] - 32 - 0.5*scale*bounds[0]
	FreeCAD.ActiveDocument.OutlineDrawUpView.Y = y0 + scale*bounds[2] + 32
	FreeCAD.ActiveDocument.OutlineDrawPlot.addObject(FreeCAD.ActiveDocument.OutlineDrawUpView)
	FreeCAD.ActiveDocument.recompute()
	return obj

def getEdges(objs=None):
	""" Returns object edges (list of them)
	@param objs Object to get the faces, none if selected
	object may used.
	@return Selected edges. None if errors happens
	"""
	edges = []
	if not objs:
		objs = FreeCADGui.Selection.getSelection()
	if not objs:
		return None
	for i in range(0, len(objs)):
		obj = objs[i]
		if obj.isDerivedFrom('Part::Feature'):
			# get shape
			shape = obj.Shape
			if not shape:
				return None
			obj = shape
		if not obj.isDerivedFrom('Part::TopoShape'):
			return None
		objEdges = obj.Edges
		if not objEdges:
			continue
		for j in range(0, len(objEdges)):
			edges.append(objEdges[j])
	return edges
