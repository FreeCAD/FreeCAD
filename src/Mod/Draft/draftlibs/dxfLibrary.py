#dxfLibrary.py : provides functions for generating DXF files
# --------------------------------------------------------------------------
__version__ = "v1.33 - 2009.06.16"
__author__ = "Stani Michiels(Stani), Remigiusz Fiedler(migius)"
__license__ = "GPL"
__url__ = "http://wiki.blender.org/index.php/Scripts/Manual/Export/autodesk_dxf"
__bpydoc__ ="""The library to export geometry data to DXF format r12 version.

Copyright %s
Version %s
License %s
Homepage %s

See the homepage for documentation.
Dedicated thread on BlenderArtists: http://blenderartists.org/forum/showthread.php?t=136439

IDEAs:
-

TODO:
- add support for DXFr14 (needs extended file header)
- add support for SPLINEs (possible first in DXFr14 version)
- add user preset for floating point precision (3-16?)

History
v1.33 - 2009.06.16 by migius
 - modif _point(): converts all coords to floats
 - modif LineType class: implement elements
 - added VPORT class, incl. defaults
 - fix Insert class
v1.32 - 2009.06.06 by migius
 - modif Style class: changed defaults to widthFactor=1.0, obliqueAngle=0.0
 - modif Text class: alignment parameter reactivated
v1.31 - 2009.06.02 by migius
 - modif _Entity class: added paperspace,elevation
v1.30 - 2009.05.28 by migius
 - bugfix 3dPOLYLINE/POLYFACE: VERTEX needs x,y,z coordinates, index starts with 1 not 0
v1.29 - 2008.12.28 by Yorik
 - modif POLYLINE to support bulge segments
v1.28 - 2008.12.13 by Steeve/BlenderArtists
 - bugfix for EXTMIN/EXTMAX to suit Cycas-CAD
v1.27 - 2008.10.07 by migius
 - beautifying output code: keys whitespace prefix
 - refactoring DXF-strings format: NewLine moved to the end of
v1.26 - 2008.10.05 by migius
 - modif POLYLINE to support POLYFACE
v1.25 - 2008.09.28 by migius
 - modif FACE class for r12
v1.24 - 2008.09.27 by migius
 - modif POLYLINE class for r12
 - changing output format from r9 to r12(AC1009)
v1.1 (20/6/2005) by www.stani.be/python/sdxf
 - Python library to generate dxf drawings
______________________________________________________________
""" % (__author__,__version__,__license__,__url__)

# --------------------------------------------------------------------------
# DXF Library: copyright (C) 2005 by Stani Michiels (AKA Stani)
#                       2008/2009 modif by Remigiusz Fiedler (AKA migius)
# --------------------------------------------------------------------------
# ***** BEGIN GPL LICENSE BLOCK *****
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
# ***** END GPL LICENCE BLOCK *****


#import Blender
#from Blender import Mathutils, Window, Scene, sys, Draw
#import BPyMessages

try:
	import copy
	#from struct import pack
except:
	copy = None

####1) Private (only for developpers)
_HEADER_POINTS=['insbase','extmin','extmax']

#---helper functions-----------------------------------
def _point(x,index=0):
	"""Convert tuple to a dxf point"""
	#print 'deb: _point=', x #-------------
	return '\n'.join([' %s\n%s'%((i+1)*10+index,float(x[i])) for i in range(len(x))])

def _points(plist):
	"""Convert a list of tuples to dxf points"""
	out = '\n'.join([_point(plist[i],i)for i in range(len(plist))])
	return out

#---base classes----------------------------------------
class _Call:
	"""Makes a callable class."""
	def copy(self):
		"""Returns a copy."""
		return copy.deepcopy(self)

	def __call__(self,**attrs):
		"""Returns a copy with modified attributes."""
		copied=self.copy()
		for attr in attrs:setattr(copied,attr,attrs[attr])
		return copied

#-------------------------------------------------------
class _Entity(_Call):
	"""Base class for _common group codes for entities."""
	def __init__(self,paperspace=None,color=None,layer='0',
				 lineType=None,lineTypeScale=None,lineWeight=None,
				 extrusion=None,elevation=None,thickness=None,
				 parent=None):
		"""None values will be omitted."""
		self.paperspace	  = paperspace
		self.color		  = color
		self.layer		  = layer
		self.lineType	   = lineType
		self.lineTypeScale  = lineTypeScale
		self.lineWeight	 = lineWeight
		self.extrusion	  = extrusion
		self.elevation	  = elevation
		self.thickness	  = thickness
		#self.visible	  = visible
		self.parent		 = parent

	def _common(self):
		"""Return common group codes as a string."""
		if self.parent:parent=self.parent
		else:parent=self
		result =''
		if parent.paperspace==1: result+='  67\n1\n'
		if parent.layer!=None: result+='  8\n%s\n'%parent.layer
		if parent.color!=None: result+=' 62\n%s\n'%parent.color
		if parent.lineType!=None: result+='  6\n%s\n'%parent.lineType
		# TODO: if parent.lineWeight!=None: result+='370\n%s\n'%parent.lineWeight
		# TODO: if parent.visible!=None: result+='60\n%s\n'%parent.visible
		if parent.lineTypeScale!=None: result+=' 48\n%s\n'%parent.lineTypeScale
		if parent.elevation!=None: result+=' 38\n%s\n'%parent.elevation
		if parent.thickness!=None: result+=' 39\n%s\n'%parent.thickness
		if parent.extrusion!=None: result+='%s\n'%_point(parent.extrusion,200)
		return result

#--------------------------
class _Entities:
	"""Base class to deal with composed objects."""
	def __dxf__(self):
		return []

	def __str__(self):
		return ''.join([str(x) for x in self.__dxf__()])

#--------------------------
class _Collection(_Call):
	"""Base class to expose entities methods to main object."""
	def __init__(self,entities=[]):
		self.entities=copy.copy(entities)
		#link entities methods to drawing
		for attr in dir(self.entities):
			if attr[0]!='_':
				attrObject=getattr(self.entities,attr)
				if callable(attrObject):
					setattr(self,attr,attrObject)

####2) Constants
#---color values
BYBLOCK=0
BYLAYER=256

#---block-type flags (bit coded values, may be combined):
ANONYMOUS =1  # This is an anonymous block generated by hatching, associative dimensioning, other internal operations, or an application
NON_CONSTANT_ATTRIBUTES =2  # This block has non-constant attribute definitions (this bit is not set if the block has any attribute definitions that are constant, or has no attribute definitions at all)
XREF =4  # This block is an external reference (xref)
XREF_OVERLAY =8  # This block is an xref overlay
EXTERNAL =16 # This block is externally dependent
RESOLVED =32 # This is a resolved external reference, or dependent of an external reference (ignored on input)
REFERENCED =64 # This definition is a referenced external reference (ignored on input)

#---mtext flags
#attachment point
TOP_LEFT = 1
TOP_CENTER = 2
TOP_RIGHT = 3
MIDDLE_LEFT = 4
MIDDLE_CENTER = 5
MIDDLE_RIGHT	= 6
BOTTOM_LEFT = 7
BOTTOM_CENTER = 8
BOTTOM_RIGHT = 9
#drawing direction
LEFT_RIGHT = 1
TOP_BOTTOM = 3
BY_STYLE = 5 #the flow direction is inherited from the associated text style
#line spacing style (optional):
AT_LEAST = 1 #taller characters will override
EXACT = 2 #taller characters will not override

#---polyline flags
CLOSED =1	  # This is a closed polyline (or a polygon mesh closed in the M direction)
CURVE_FIT =2	  # Curve-fit vertices have been added
SPLINE_FIT =4	  # Spline-fit vertices have been added
POLYLINE_3D =8	  # This is a 3D polyline
POLYGON_MESH =16	 # This is a 3D polygon mesh
CLOSED_N =32	 # The polygon mesh is closed in the N direction
POLYFACE_MESH =64	 # The polyline is a polyface mesh
CONTINOUS_LINETYPE_PATTERN =128	# The linetype pattern is generated continuously around the vertices of this polyline

#---text flags
#horizontal
LEFT = 0
CENTER = 1
RIGHT = 2
ALIGNED = 3 #if vertical alignment = 0
MIDDLE = 4 #if vertical alignment = 0
FIT = 5 #if vertical alignment = 0
#vertical
BASELINE = 0
BOTTOM	= 1
MIDDLE = 2
TOP = 3

####3) Classes
#---entitities -----------------------------------------------
#--------------------------
class Arc(_Entity):
	"""Arc, angles in degrees."""
	def __init__(self,center=(0,0,0),radius=1,
				 startAngle=0.0,endAngle=90,**common):
		"""Angles in degrees."""
		_Entity.__init__(self,**common)
		self.center=center
		self.radius=radius
		self.startAngle=startAngle
		self.endAngle=endAngle
	def __str__(self):
		return '  0\nARC\n%s%s\n 40\n%s\n 50\n%s\n 51\n%s\n'%\
			   (self._common(),_point(self.center),
				self.radius,self.startAngle,self.endAngle)

#-----------------------------------------------
class Circle(_Entity):
	"""Circle"""
	def __init__(self,center=(0,0,0),radius=1,**common):
		_Entity.__init__(self,**common)
		self.center=center
		self.radius=radius
	def __str__(self):
		return '  0\nCIRCLE\n%s%s\n 40\n%s\n'%\
			   (self._common(),_point(self.center),self.radius)

#-----------------------------------------------
class Face(_Entity):
	"""3dface"""
	def __init__(self,points,**common):
		_Entity.__init__(self,**common)
		while len(points)<4: #fix for r12 format
			points.append(points[-1])
		self.points=points
		
	def __str__(self):
		out = '  0\n3DFACE\n%s%s\n' %(self._common(),_points(self.points))
		#print 'deb:out=', out #-------------------
		return out

#-----------------------------------------------
class Insert(_Entity):
	"""Block instance."""
	def __init__(self,name,point=(0,0,0),
				 xscale=None,yscale=None,zscale=None,
				 cols=None,colspacing=None,rows=None,rowspacing=None,
				 rotation=None,
				 **common):
		_Entity.__init__(self,**common)
		self.name=name
		self.point=point
		self.xscale=xscale
		self.yscale=yscale
		self.zscale=zscale
		self.cols=cols
		self.colspacing=colspacing
		self.rows=rows
		self.rowspacing=rowspacing
		self.rotation=rotation

	def __str__(self):
		result='  0\nINSERT\n  2\n%s\n%s%s\n'%\
				(self.name,self._common(),_point(self.point))
		if self.xscale!=None:result+=' 41\n%s\n'%self.xscale
		if self.yscale!=None:result+=' 42\n%s\n'%self.yscale
		if self.zscale!=None:result+=' 43\n%s\n'%self.zscale
		if self.rotation:result+=' 50\n%s\n'%self.rotation
		if self.cols!=None:result+=' 70\n%s\n'%self.cols
		if self.colspacing!=None:result+=' 44\n%s\n'%self.colspacing
		if self.rows!=None:result+=' 71\n%s\n'%self.rows
		if self.rowspacing!=None:result+=' 45\n%s\n'%self.rowspacing
		return result

#-----------------------------------------------
class Line(_Entity):
	"""Line"""
	def __init__(self,points,**common):
		_Entity.__init__(self,**common)
		self.points=points
	def __str__(self):
		return '  0\nLINE\n%s%s\n' %(
				self._common(), _points(self.points))


#-----------------------------------------------
class PolyLine(_Entity):
	def __init__(self,points,org_point=[0,0,0],flag=0,width=None,**common):
		#width = number, or width = list [width_start=None, width_end=None]
		#for 2d-polyline: points = [ [x, y, z, width_start=None, width_end=None, bulge=0 or None], ...]
		#for 3d-polyline: points = [ [x, y, z], ...]
		#for polyface: points = [points_list, faces_list]
		_Entity.__init__(self,**common)
		self.points=points
		self.org_point=org_point
		self.flag=flag
		self.polyface = False
		self.polyline2d = False
		self.faces = [] # dummy value
		self.width= None # dummy value
		if self.flag & POLYFACE_MESH:
			self.polyface=True
			self.points=points[0]
			self.faces=points[1]
			self.p_count=len(self.points)
			self.f_count=len(self.faces)
		elif not self.flag & POLYLINE_3D:
			self.polyline2d = True
			if width:
				if type(width)!='list':
					width=[width,width]
				self.width=width

	def __str__(self):
		result= '  0\nPOLYLINE\n%s 70\n%s\n' %(self._common(),self.flag)
		result+=' 66\n1\n'
		result+='%s\n' %_point(self.org_point)
		if self.polyface:
			result+=' 71\n%s\n' %self.p_count
			result+=' 72\n%s\n' %self.f_count
		elif self.polyline2d:
			if self.width!=None: result+=' 40\n%s\n 41\n%s\n' %(self.width[0],self.width[1])
		for point in self.points:
			result+='  0\nVERTEX\n'
			result+='  8\n%s\n' %self.layer
			if self.polyface:
				result+='%s\n' %_point(point[0:3])
				result+=' 70\n192\n'
			elif self.polyline2d:
				result+='%s\n' %_point(point[0:2])
				if len(point)>4:
					width1, width2 = point[3], point[4]
					if width1!=None: result+=' 40\n%s\n' %width1
					if width2!=None: result+=' 41\n%s\n' %width2
				if len(point)==6:
					bulge = point[5]
					if bulge: result+=' 42\n%s\n' %bulge
			else:
				result+='%s\n' %_point(point[0:3])
		for face in self.faces:
			result+='  0\nVERTEX\n'
			result+='  8\n%s\n' %self.layer
			result+='%s\n' %_point(self.org_point)
			result+=' 70\n128\n'
			result+=' 71\n%s\n' %face[0]
			result+=' 72\n%s\n' %face[1]
			result+=' 73\n%s\n' %face[2]
			if len(face)==4: result+=' 74\n%s\n' %face[3]
		result+='  0\nSEQEND\n'
		result+='  8\n%s\n' %self.layer
		return result

#-----------------------------------------------
class Point(_Entity):
	"""Point."""
	def __init__(self,points=None,**common):
		_Entity.__init__(self,**common)
		self.points=points
	def __str__(self): # TODO:
		return '  0\nPOINT\n%s%s\n' %(self._common(),
			 _points(self.points)
			)

#-----------------------------------------------
class Solid(_Entity):
	"""Colored solid fill."""
	def __init__(self,points=None,**common):
		_Entity.__init__(self,**common)
		self.points=points
	def __str__(self):
		return '  0\nSOLID\n%s%s\n' %(self._common(),
			 _points(self.points[:2]+[self.points[3],self.points[2]])
			)


#-----------------------------------------------
class Dimension(_Entity):
	"""Basic dimension entity"""
	def __init__(self,point,start,end,**common):
		_Entity.__init__(self,**common)
		self.points=[point,start,end]
	def __str__(self):
		result = '  0\nDIMENSION\n%s' %(self._common())
		result+=' 3\nStandard\n'
		result+=' 70\n1\n'
		result+='%s\n' %_point(self.points[0])
		result+='%s\n' %_point(self.points[1],3)
		result+='%s\n' %_point(self.points[2],4)
		print result
		return result

#-----------------------------------------------
class Text(_Entity):
	"""Single text line."""
	def __init__(self,text='',point=(0,0,0),alignment=None,
				 flag=None,height=1,justifyhor=None,justifyver=None,
				 rotation=None,obliqueAngle=None,style=None,xscale=None,**common):
		_Entity.__init__(self,**common)
		self.text=text
		self.point=point
		self.alignment=alignment
		self.flag=flag
		self.height=height
		self.justifyhor=justifyhor
		self.justifyver=justifyver
		self.rotation=rotation
		self.obliqueAngle=obliqueAngle
		self.style=style
		self.xscale=xscale
	def __str__(self):
		result= '  0\nTEXT\n%s%s\n 40\n%s\n  1\n%s\n'%\
				(self._common(),_point(self.point),self.height,self.text)
		if self.rotation: result+=' 50\n%s\n'%self.rotation
		if self.xscale: result+=' 41\n%s\n'%self.xscale
		if self.obliqueAngle: result+=' 51\n%s\n'%self.obliqueAngle
		if self.style: result+='  7\n%s\n'%self.style
		if self.flag: result+=' 71\n%s\n'%self.flag
		if self.justifyhor: result+=' 72\n%s\n'%self.justifyhor
		if self.alignment: result+='%s\n'%_point(self.alignment,1)
		if self.justifyver: result+=' 73\n%s\n'%self.justifyver
		return result

#-----------------------------------------------
class Mtext(Text):
	"""Surrogate for mtext, generates some Text instances."""
	def __init__(self,text='',point=(0,0,0),width=250,spacingFactor=1.5,down=0,spacingWidth=None,**options):
		Text.__init__(self,text=text,point=point,**options)
		if down:spacingFactor*=-1
		self.spacingFactor=spacingFactor
		self.spacingWidth=spacingWidth
		self.width=width
		self.down=down
	def __str__(self):
		texts=self.text.replace('\r\n','\n').split('\n')
		if not self.down:texts.reverse()
		result=''
		x=y=0
		if self.spacingWidth:spacingWidth=self.spacingWidth
		else:spacingWidth=self.height*self.spacingFactor
		for text in texts:
			while text:
				result+='%s\n'%Text(text[:self.width],
					point=(self.point[0]+x*spacingWidth,
						   self.point[1]+y*spacingWidth,
						   self.point[2]),
					alignment=self.alignment,flag=self.flag,height=self.height,
					justifyhor=self.justifyhor,justifyver=self.justifyver,
					rotation=self.rotation,obliqueAngle=self.obliqueAngle,
					style=self.style,xscale=self.xscale,parent=self
				)
				text=text[self.width:]
				if self.rotation:x+=1
				else:y+=1
		return result[1:]

#-----------------------------------------------
##class _Mtext(_Entity):
##	"""Mtext not functioning for minimal dxf."""
##	def __init__(self,text='',point=(0,0,0),attachment=1,
##				 charWidth=None,charHeight=1,direction=1,height=100,rotation=0,
##				 spacingStyle=None,spacingFactor=None,style=None,width=100,
##				 xdirection=None,**common):
##		_Entity.__init__(self,**common)
##		self.text=text
##		self.point=point
##		self.attachment=attachment
##		self.charWidth=charWidth
##		self.charHeight=charHeight
##		self.direction=direction
##		self.height=height
##		self.rotation=rotation
##		self.spacingStyle=spacingStyle
##		self.spacingFactor=spacingFactor
##		self.style=style
##		self.width=width
##		self.xdirection=xdirection
##	def __str__(self):
##		input=self.text
##		text=''
##		while len(input)>250:
##			text+='3\n%s\n'%input[:250]
##			input=input[250:]
##		text+='1\n%s\n'%input
##		result= '0\nMTEXT\n%s\n%s\n40\n%s\n41\n%s\n71\n%s\n72\n%s%s\n43\n%s\n50\n%s\n'%\
##				(self._common(),_point(self.point),self.charHeight,self.width,
##				 self.attachment,self.direction,text,
##				 self.height,
##				 self.rotation)
##		if self.style:result+='7\n%s\n'%self.style
##		if self.xdirection:result+='%s\n'%_point(self.xdirection,1)
##		if self.charWidth:result+='42\n%s\n'%self.charWidth
##		if self.spacingStyle:result+='73\n%s\n'%self.spacingStyle
##		if self.spacingFactor:result+='44\n%s\n'%self.spacingFactor
##		return result

#---tables ---------------------------------------------------
#-----------------------------------------------
class Block(_Collection):
	"""Use list methods to add entities, eg append."""
	def __init__(self,name,layer='0',flag=0,base=(0,0,0),entities=[]):
		self.entities=copy.copy(entities)
		_Collection.__init__(self,entities)
		self.layer=layer
		self.name=name
		self.flag=0
		self.base=base
	def __str__(self): # TODO:
		e=''.join([str(x)for x in self.entities])
		return '  0\nBLOCK\n  8\n%s\n  2\n%s\n 70\n%s\n%s\n  3\n%s\n%s  0\nENDBLK\n'%\
			   (self.layer,self.name.upper(),self.flag,_point(self.base),self.name.upper(),e)

#-----------------------------------------------
class Layer(_Call):
	"""Layer"""
	def __init__(self,name='pydxf',color=7,lineType='continuous',flag=64):
		self.name=name
		self.color=color
		self.lineType=lineType
		self.flag=flag
	def __str__(self):
		return '  0\nLAYER\n  2\n%s\n 70\n%s\n 62\n%s\n  6\n%s\n'%\
			   (self.name.upper(),self.flag,self.color,self.lineType)

#-----------------------------------------------
class LineType(_Call):
	"""Custom linetype"""
	def __init__(self,name='CONTINUOUS',description='Solid line',elements=[0.0],flag=0):
		self.name=name
		self.description=description
		self.elements=copy.copy(elements)
		self.flag=flag
	def __str__(self):
		result = '  0\nLTYPE\n  2\n%s\n 70\n%s\n  3\n%s\n 72\n65\n'%\
			(self.name.upper(),self.flag,self.description)
		if self.elements:
			elements = ' 73\n%s\n' %(len(self.elements)-1)
			elements += ' 40\n%s\n' %(self.elements[0])
			for e in self.elements[1:]:
				elements += ' 49\n%s\n' %e
			result += elements
		return result
		 

#-----------------------------------------------
class Style(_Call):
	"""Text style"""
	def __init__(self,name='standard',flag=0,height=0,widthFactor=1.0,obliqueAngle=0.0,
				 mirror=0,lastHeight=1,font='arial.ttf',bigFont=''):
		self.name=name
		self.flag=flag
		self.height=height
		self.widthFactor=widthFactor
		self.obliqueAngle=obliqueAngle
		self.mirror=mirror
		self.lastHeight=lastHeight
		self.font=font
		self.bigFont=bigFont
	def __str__(self):
		return '  0\nSTYLE\n  2\n%s\n 70\n%s\n 40\n%s\n 41\n%s\n 50\n%s\n 71\n%s\n 42\n%s\n 3\n%s\n 4\n%s\n'%\
			   (self.name.upper(),self.flag,self.flag,self.widthFactor,
				self.obliqueAngle,self.mirror,self.lastHeight,
				self.font.upper(),self.bigFont.upper())

#-----------------------------------------------
class VPort(_Call):
	def __init__(self,name,flag=0,
				leftBottom=(0.0,0.0),
				rightTop=(1.0,1.0),
				center=(0.5,0.5),
				snap_base=(0.0,0.0),
				snap_spacing=(0.1,0.1),
				grid_spacing=(0.1,0.1),
				direction=(0.0,0.0,1.0),
				target=(0.0,0.0,0.0),
				height=1.0,
				ratio=1.0,
				lens=50,
				frontClipping=0,
				backClipping=0,
				snap_rotation=0,
				twist=0,
				mode=0,
				circle_zoom=100,
				fast_zoom=1,
				ucsicon=1,
				snap_on=0,
				grid_on=0,
				snap_style=0,
				snap_isopair=0
				):
		self.name=name
		self.flag=flag
		self.leftBottom=leftBottom
		self.rightTop=rightTop
		self.center=center
		self.snap_base=snap_base
		self.snap_spacing=snap_spacing
		self.grid_spacing=grid_spacing
		self.direction=direction
		self.target=target
		self.height=float(height)
		self.ratio=float(ratio)
		self.lens=float(lens)
		self.frontClipping=float(frontClipping)
		self.backClipping=float(backClipping)
		self.snap_rotation=float(snap_rotation)
		self.twist=float(twist)
		self.mode=mode
		self.circle_zoom=circle_zoom
		self.fast_zoom=fast_zoom
		self.ucsicon=ucsicon
		self.snap_on=snap_on
		self.grid_on=grid_on
		self.snap_style=snap_style
		self.snap_isopair=snap_isopair
	def __str__(self):
		output = ['  0', 'VPORT',
			'  2', self.name,
			' 70', self.flag,
			_point(self.leftBottom),
			_point(self.rightTop,1),
			_point(self.center,2), # View center point (in DCS)
			_point(self.snap_base,3),
			_point(self.snap_spacing,4),
			_point(self.grid_spacing,5),
			_point(self.direction,6), #view direction from target (in WCS)
			_point(self.target,7),
			' 40', self.height,
			' 41', self.ratio,
			' 42', self.lens,
			' 43', self.frontClipping,
			' 44', self.backClipping,
			' 50', self.snap_rotation,
			' 51', self.twist,
			' 71', self.mode,
			' 72', self.circle_zoom,
			' 73', self.fast_zoom,
			' 74', self.ucsicon,
			' 75', self.snap_on,
			' 76', self.grid_on,
			' 77', self.snap_style,
			' 78', self.snap_isopair
			]

		output_str = ''
		for s in output:
			output_str += '%s\n' %s
		return output_str



#-----------------------------------------------
class View(_Call):
	def __init__(self,name,flag=0,
			width=1,
			height=1,
			center=(0.5,0.5),
			direction=(0,0,1),
			target=(0,0,0),
			lens=50,
			frontClipping=0,
			backClipping=0,
			twist=0,mode=0
			):
		self.name=name
		self.flag=flag
		self.width=float(width)
		self.height=float(height)
		self.center=center
		self.direction=direction
		self.target=target
		self.lens=float(lens)
		self.frontClipping=float(frontClipping)
		self.backClipping=float(backClipping)
		self.twist=float(twist)
		self.mode=mode
	def __str__(self):
		output = ['  0', 'VIEW',
			'  2', self.name,
			' 70', self.flag,
			' 40', self.height,
			_point(self.center),
			' 41', self.width,
			_point(self.direction,1),
			_point(self.target,2),
			' 42', self.lens,
			' 43', self.frontClipping,
			' 44', self.backClipping,
			' 50', self.twist,
			' 71', self.mode
			]
		output_str = ''
		for s in output:
			output_str += '%s\n' %s
		return output_str

#-----------------------------------------------
def ViewByWindow(name,leftBottom=(0,0),rightTop=(1,1),**options):
	width=abs(rightTop[0]-leftBottom[0])
	height=abs(rightTop[1]-leftBottom[1])
	center=((rightTop[0]+leftBottom[0])*0.5,(rightTop[1]+leftBottom[1])*0.5)
	return View(name=name,width=width,height=height,center=center,**options)

#---drawing
#-----------------------------------------------
class Drawing(_Collection):
	"""Dxf drawing. Use append or any other list methods to add objects."""
	def __init__(self,insbase=(0.0,0.0,0.0),extmin=(0.0,0.0,0.0),extmax=(0.0,0.0,0.0),
				 layers=[Layer()],linetypes=[LineType()],styles=[Style()],blocks=[],
				 views=[],vports=[],entities=None,fileName='test.dxf'):
		# TODO: replace list with None,arial
		if not entities:
			entities=[]
		_Collection.__init__(self,entities)
		self.insbase=insbase
		self.extmin=extmin
		self.extmax=extmax
		self.layers=copy.copy(layers)
		self.linetypes=copy.copy(linetypes)
		self.styles=copy.copy(styles)
		self.views=copy.copy(views)
		self.vports=copy.copy(vports)
		self.blocks=copy.copy(blocks)
		self.fileName=fileName
		#private
		#self.acadver='9\n$ACADVER\n1\nAC1006\n'
		self.acadver='  9\n$ACADVER\n  1\nAC1009\n'
		"""DXF AutoCAD-Release format codes
		AC1021  2008, 2007 
		AC1018  2006, 2005, 2004 
		AC1015  2002, 2000i, 2000 
		AC1014  R14,14.01 
		AC1012  R13    
		AC1009  R12,11 
		AC1006  R10    
		AC1004  R9    
		AC1002  R2.6  
		AC1.50  R2.05 
		"""

	def _name(self,x):
		"""Helper function for self._point"""
		return '  9\n$%s\n' %x.upper()

	def _point(self,name,x):
		"""Point setting from drawing like extmin,extmax,..."""
		return '%s%s' %(self._name(name),_point(x))

	def _section(self,name,x):
		"""Sections like tables,blocks,entities,..."""
		if x: xstr=''.join(x)
		else: xstr=''
		return '  0\nSECTION\n  2\n%s\n%s  0\nENDSEC\n'%(name.upper(),xstr)

	def _table(self,name,x):
		"""Tables like ltype,layer,style,..."""
		if x: xstr=''.join(x)
		else: xstr=''
		return '  0\nTABLE\n  2\n%s\n 70\n%s\n%s  0\nENDTAB\n'%(name.upper(),len(x),xstr)

	def __str__(self):
		"""Returns drawing as dxf string."""
		header=[self.acadver]+[self._point(attr,getattr(self,attr))+'\n' for attr in _HEADER_POINTS]
		header=self._section('header',header)

		tables=[self._table('vport',[str(x) for x in self.vports]),
				self._table('ltype',[str(x) for x in self.linetypes]),
				self._table('layer',[str(x) for x in self.layers]),
				self._table('style',[str(x) for x in self.styles]),
				self._table('view',[str(x) for x in self.views]),
		]
		tables=self._section('tables',tables)

		blocks=self._section('blocks',[str(x) for x in self.blocks])

		entities=self._section('entities',[str(x) for x in self.entities])

		all=''.join([header,tables,blocks,entities,'  0\nEOF\n'])
		return all

	def saveas(self,fileName):
		self.fileName=fileName
		self.save()

	def save(self):
		test=open(self.fileName,'w')
		test.write(str(self))
		test.close()


#---extras
#-----------------------------------------------
class Rectangle(_Entity):
	"""Rectangle, creates lines."""
	def __init__(self,point=(0,0,0),width=1,height=1,solid=None,line=1,**common):
		_Entity.__init__(self,**common)
		self.point=point
		self.width=width
		self.height=height
		self.solid=solid
		self.line=line
	def __str__(self):
		result=''
		points=[self.point,(self.point[0]+self.width,self.point[1],self.point[2]),
			(self.point[0]+self.width,self.point[1]+self.height,self.point[2]),
			(self.point[0],self.point[1]+self.height,self.point[2]),self.point]
		if self.solid:
			result+= Solid(points=points[:-1],parent=self.solid)
		if self.line:
			for i in range(4):
				result+= Line(points=[points[i],points[i+1]],parent=self)
		return result[1:]

#-----------------------------------------------
class LineList(_Entity):
	"""Like polyline, but built of individual lines."""
	def __init__(self,points=[],org_point=[0,0,0],closed=0,**common):
		_Entity.__init__(self,**common)
		self.closed=closed
		self.points=copy.copy(points)
	def __str__(self):
		if self.closed:points=self.points+[self.points[0]]
		else: points=self.points
		result=''
		for i in range(len(points)-1):
			result+= Line(points=[points[i],points[i+1]],parent=self)
		return result[1:]

#-----------------------------------------------------
def test():
	#Blocks
	b=Block('test')
	b.append(Solid(points=[(0,0,0),(1,0,0),(1,1,0),(0,1,0)],color=1))
	b.append(Arc(center=(1,0,0),color=2))

	#Drawing
	d=Drawing()
	#tables
	d.blocks.append(b)  #table blocks
	d.styles.append(Style())  #table styles
	d.views.append(View('Normal'))  #table view
	d.views.append(ViewByWindow('Window',leftBottom=(1,0),rightTop=(2,1)))  #idem

	#entities
	d.append(Circle(center=(1,1,0),color=3))
	d.append(Face(points=[(0,0,0),(1,0,0),(1,1,0),(0,1,0)],color=4))
	d.append(Insert('test',point=(3,3,3),cols=5,colspacing=2))
	d.append(Line(points=[(0,0,0),(1,1,1)]))
	d.append(Mtext('Click on Ads\nmultiple lines with mtext',point=(1,1,1),color=5,rotation=90))
	d.append(Text('Please donate!',point=(3,0,1)))
	#d.append(Rectangle(point=(2,2,2),width=4,height=3,color=6,solid=Solid(color=2)))
	d.append(Solid(points=[(4,4,0),(5,4,0),(7,8,0),(9,9,0)],color=3))
	#d.append(PolyLine(points=[(1,1,1),(2,1,1),(2,2,1),(1,2,1)],flag=1,color=1))

	#d.saveas('c:\\test.dxf')
	d.saveas('test.dxf')

#-----------------------------------------------------
if __name__=='__main__':
	if not copy:
		Draw.PupMenu('Error%t|This script requires a full python install')
	else: test()
	
