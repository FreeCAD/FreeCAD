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

# FreeCAD modules
import FreeCAD,FreeCADGui
from FreeCAD import Base
import Part
# FreeCADShip modules
from shipUtils import Paths

class Preview(object):
	def __init__(self):
		""" Constructor.
		"""
		self.objects = []

	def reinit(self):
		""" Reinitializate drawer.
		"""
		self.clean()

	def update(self, names, pos):
		""" Update the 3D view printing annotations.
		@param names Weight names.
		@param pos Weight positions (FreeCAD::Base::Vector).
		"""
		# Destroy all previous entities
		self.clean()
		for i in range(0, len(names)):
			# Draw gravity line
			line = Part.makeLine((pos[i].x,pos[i].y,pos[i].z),(pos[i].x,pos[i].y,pos[i].z - 9.81))
			Part.show(line)
			objs = FreeCAD.ActiveDocument.Objects
			self.objects.append(objs[-1])
			objs[-1].Label = names[i] + 'Line'
			# Draw circles
			circle = Part.makeCircle(0.5, pos[i], Base.Vector(1.0,0.0,0.0))
			Part.show(circle)
			objs = FreeCAD.ActiveDocument.Objects
			self.objects.append(objs[-1])
			objs[-1].Label = names[i] + 'CircleX'			
			circle = Part.makeCircle(0.5, pos[i], Base.Vector(0.0,1.0,0.0))
			Part.show(circle)
			objs = FreeCAD.ActiveDocument.Objects
			self.objects.append(objs[-1])
			objs[-1].Label = names[i] + 'CircleY'			
			circle = Part.makeCircle(0.5, pos[i], Base.Vector(0.0,0.0,1.0))
			Part.show(circle)
			objs = FreeCAD.ActiveDocument.Objects
			self.objects.append(objs[-1])
			objs[-1].Label = names[i] + 'CircleZ'			
			# Draw annotation
			self.objects.append(DrawText(names[i] + 'Text', names[i], Base.Vector(pos[i].x+1.0,pos[i].y,pos[i].z)))
		
	def clean(self):
		""" Erase all annotations from screen.
		"""
		for i in range(0,len(self.objects)):
			if not FreeCAD.ActiveDocument.getObject(self.objects[i].Name):
				continue
			FreeCAD.ActiveDocument.removeObject(self.objects[i].Name)
		self.objects = []

def DrawText(name, string, position, displayMode="Screen", angle=0.0, justification="Left", colour=(0.00,0.00,0.00), size=12):
	""" Draws a text in a desired position.
	@param name Name of the object
	@param string Text to draw (recommended format u'')	
	@param position Point to draw the text
	@param angle Counter clockwise rotation of text
	@param justification Alignement of the text ("Left", "Right" or "Center")
	@param colour Colour of the text
	@param size Font size
	@return FreeCAD annotation object
	"""
	# Create the object
	text = FreeCAD.ActiveDocument.addObject("App::Annotation",name)
	# Set the text
	text.LabelText = [string, u'']
	# Set the options
	text.Position = position
	FreeCADGui.ActiveDocument.getObject(text.Name).Rotation = angle
	FreeCADGui.ActiveDocument.getObject(text.Name).Justification = justification
	FreeCADGui.ActiveDocument.getObject(text.Name).FontSize = size
	FreeCADGui.ActiveDocument.getObject(text.Name).TextColor = colour
	FreeCADGui.ActiveDocument.getObject(text.Name).DisplayMode = displayMode
	return FreeCAD.ActiveDocument.getObject(text.Name)
