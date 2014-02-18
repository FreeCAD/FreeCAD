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

import math
from FreeCAD import Vector
import Part
import Units
import FreeCAD as App
import FreeCADGui as Gui
import Instance
from shipUtils import Math

def areas(ship, draft, roll=0.0, trim=0.0, yaw=0.0, n=30):
	""" Compute the ship transversal areas.
	@param ship Ship instance.
	@param draft Ship draft.
	@param roll Ship roll angle.
	@param trim Ship trim angle.
	@param yaw Ship yaw angle. Ussually you don't want to use this 
	value.
	@param n Number of sections to perform.
	@return Transversal areas (every area value is composed by x 
	coordinate and computed area)
	"""
	if n < 2:
		return []
	# We will take a duplicate of ship shape in order to conviniently place it
	shape = ship.Shape.copy()
	shape.translate(Vector(0.0,0.0,-draft*Units.Metre.Value))
	# Rotations composition is Roll->Trim->Yaw
	shape.rotate(Vector(0.0,0.0,0.0), Vector(1.0,0.0,0.0), roll)
	shape.rotate(Vector(0.0,0.0,0.0), Vector(0.0,-1.0,0.0), trim)
	shape.rotate(Vector(0.0,0.0,0.0), Vector(0.0,0.0,1.0), yaw)
	# Now we need to know the x range of values to perform the sections
	bbox = shape.BoundBox
	xmin = bbox.XMin
	xmax = bbox.XMax
	dx   = (xmax - xmin) / (n-1.0)
	# Since we are computing in the total length (not in the perpendiculars one),
	# we can grant that the starting and ending sections are null
	areas = [[xmin/Units.Metre.Value, 0.0]]
	# And since we need only face entities, in order to compute sections we will
	# create boxes with front face at the desired transversal area position, 
	# computing the common solid part, dividing it by faces, and getting only
	# the desired ones.
	App.Console.PrintMessage("Computing transversal areas...\n")
	App.Console.PrintMessage("Some Inventor representation errors can be shown, please ignore it.\n")
	for i in range(1,n-1):
		App.Console.PrintMessage("{0} / {1}\n".format(i, n-2))
		x	= xmin + i*dx
		area = 0.0
		# Create the box
		L = xmax - xmin
		B = bbox.YMax - bbox.YMin
		p = Vector(-1.5*L, -1.5*B, bbox.ZMin)
		try:
			box = Part.makeBox(1.5*L + x, 3.0*B, - bbox.ZMin, p)
		except:
			areas.append([x, area])
			continue
		# Compute the common part with ship
		for s in shape.Solids:
			try:
				common = box.common(s)
			except:
				continue
			if common.Volume == 0.0:
				continue
			# Recompute object adding it to the scene, when we have
			# computed desired data we can remove it.
			try:
				Part.show(common)
			except:
				continue
			# Divide the solid by faces and compute only the well placed ones
			faces  = common.Faces
			for f in faces:
				faceBounds = f.BoundBox
				# Orientation filter
				if faceBounds.XMax - faceBounds.XMin > 0.00001:
					continue
				# Place filter
				if abs(faceBounds.XMax - x) > 0.00001:
					continue
				# It is a valid face, so we can add this area
				area = area + f.Area/Units.Metre.Value**2
			# Destroy the last generated object
			App.ActiveDocument.removeObject(App.ActiveDocument.Objects[-1].Name)
		areas.append([x/Units.Metre.Value, area])
	# Last area is equal to zero (see some lines above)
	areas.append([xmax/Units.Metre.Value, 0.0])
	App.Console.PrintMessage("Done!\n")
	return areas

def displacement(ship, draft, roll=0.0, trim=0.0, yaw=0.0):
	""" Compute the ship displacement.
	@param ship Ship instance.
	@param draft Ship draft.
	@param roll Ship roll angle.
	@param trim Ship trim angle.
	@param yaw Ship yaw angle. Ussually you don't want to use this 
	value.
	@return [disp, B, Cb], \n
	  - disp = Ship displacement [ton].
	  - B = Bouyance center [m].
	  - Cb = Block coefficient.
	@note Bouyance center will returned as a FreeCAD.Vector instance.
	@note Returned Bouyance center is in the non modified ship coordinates
	"""
	# We will take a duplicate of ship shape in order to conviniently place it
	shape = ship.Shape.copy()
	shape.translate(Vector(0.0,0.0,-draft*Units.Metre.Value))
	# Rotations composition is Roll->Trim->Yaw
	shape.rotate(Vector(0.0,0.0,0.0), Vector(1.0,0.0,0.0), roll)
	shape.rotate(Vector(0.0,0.0,0.0), Vector(0.0,-1.0,0.0), trim)
	shape.rotate(Vector(0.0,0.0,0.0), Vector(0.0,0.0,1.0), yaw)
	# Now we need to know box dimensions
	bbox = shape.BoundBox
	xmin = bbox.XMin
	xmax = bbox.XMax
	# Create the box
	L = xmax - xmin
	B = bbox.YMax - bbox.YMin
	p = Vector(-1.5*L, -1.5*B, bbox.ZMin)
	try:
		box = Part.makeBox(3.0*L, 3.0*B, - bbox.ZMin, p)
	except:
		return [0.0, Vector(), 0.0]
	vol = 0.0
	cog = Vector()
	for solid in shape.Solids:
		# Compute the common part with the ship
		try:
			common = box.common(solid)
		except:
			continue
		# Get the data
		vol = vol + common.Volume/Units.Metre.Value**3
		for s in common.Solids:
			sCoG  = s.CenterOfMass
			cog.x = cog.x + sCoG.x*s.Volume/Units.Metre.Value**4
			cog.y = cog.y + sCoG.y*s.Volume/Units.Metre.Value**4
			cog.z = cog.z + sCoG.z*s.Volume/Units.Metre.Value**4
	cog.x = cog.x / vol
	cog.y = cog.y / vol
	cog.z = cog.z / vol
	Vol = L*B*abs(bbox.ZMin)/Units.Metre.Value**3
	# Undo transformations
	B   = Vector()
	B.x = cog.x*math.cos(math.radians(-yaw)) - cog.y*math.sin(math.radians(-yaw))
	B.y = cog.x*math.sin(math.radians(-yaw)) + cog.y*math.cos(math.radians(-yaw))
	B.z = cog.z
	cog.x = B.x*math.cos(math.radians(-trim)) - B.z*math.sin(math.radians(-trim))
	cog.y = B.y
	cog.z = B.x*math.sin(math.radians(-trim)) + B.z*math.cos(math.radians(-trim))
	B.x = cog.x
	B.y = cog.y*math.cos(math.radians(-roll)) - cog.z*math.sin(math.radians(-roll))
	B.z = cog.y*math.sin(math.radians(-roll)) + cog.z*math.cos(math.radians(-roll))
	B.z = B.z + draft
	# Return data
	dens = 1.025 # [tons/m3], salt water
	return [dens*vol, B, vol/Vol]

def wettedArea(shape, draft, trim):
	""" Calculate wetted ship area.
	@param shape Ship external faces instance.
	@param draft Draft.
	@param trim Trim in degrees.
	@return Wetted ship area.
	"""
	area	 = 0.0
	nObjects = 0
	# We will take a duplicate of ship shape in order to place it
	shape = shape.copy()
	shape.translate(Vector(0.0,0.0,-draft))
	shape.rotate(Vector(0.0,0.0,0.0), Vector(0.0,-1.0,0.0), trim)
	# Now we need to know the x range of values
	bbox = shape.BoundBox
	xmin = bbox.XMin
	xmax = bbox.XMax
	# Create the box
	L = xmax - xmin
	B = bbox.YMax - bbox.YMin
	p = Vector(-1.5*L, -1.5*B, bbox.ZMin - 1.0)
	box = Part.makeBox(3.0*L, 3.0*B, - bbox.ZMin + 1.0, p)
	# Compute common part with ship
	for f in shape.Faces:
		# Get solids intersection
		try:
			common = box.common(f)
		except:
			continue
		area = area + common.Area
	return area

def moment(ship, draft, trim, disp, xcb):
	""" Calculate triming 1cm ship moment.
	@param ship Selected ship instance
	@param draft Draft.
	@param trim Trim in degrees.
	@param disp Displacement at selected draft and trim.
	@param xcb Bouyance center at selected draft and trim.
	@return Moment to trim ship 1cm (ton m).
	@note Moment is positive when produce positive trim.
	"""
	factor = 10.0
	angle = factor*math.degrees(math.atan2(0.01,0.5*ship.Length))
	newTrim = trim + angle
	data = displacement(ship,draft,0.0,newTrim,0.0)
	mom0 = -disp*xcb
	mom1 = -data[0]*data[1].x
	return (mom1 - mom0) / factor

def FloatingArea(ship, draft, trim):
	""" Calculate ship floating area.
	@param ship Selected ship instance
	@param draft Draft.
	@param trim Trim in degrees.
	@return Ship floating area, and floating coefficient.
	"""
	area	 = 0.0
	cf	   = 0.0
	maxX	 = 0.0
	minX	 = 0.0
	maxY	 = 0.0
	minY	 = 0.0
	# We will take a duplicate of ship shape in order to place it
	shape = ship.Shape.copy()
	shape.translate(Vector(0.0,0.0,-draft))
	shape.rotate(Vector(0.0,0.0,0.0), Vector(0.0,-1.0,0.0), trim)
	# Now we need to know the x range of values
	bbox = shape.BoundBox
	xmin = bbox.XMin
	xmax = bbox.XMax
	# Create the box
	L = xmax - xmin
	B = bbox.YMax - bbox.YMin
	p = Vector(-1.5*L, -1.5*B, bbox.ZMin - 1.0)
	box = Part.makeBox(3.0*L, 3.0*B, - bbox.ZMin + 1.0, p)
	# Compute common part with ship
	maxX = bbox.XMin
	minX = bbox.XMax
	maxY = bbox.YMin
	minY = bbox.YMax
	for s in shape.Solids:
		# Get solids intersection
		try:
			common = box.common(s)
		except:
			continue
		if common.Volume == 0.0:
			continue
		# Recompute object adding it to the scene, when we have
		# computed desired data we can remove it.
		try:
			Part.show(common)
		except:
			continue
		# Divide by faces and compute only section placed ones
		faces  = common.Faces
		for f in faces:
			faceBounds = f.BoundBox
			# Orientation filter
			if faceBounds.ZMax - faceBounds.ZMin > 0.00001:
				continue
			# Place filter
			if abs(faceBounds.ZMax) > 0.00001:
				continue
			# Valid face, compute area
			area = area + f.Area
			maxX = max(maxX, faceBounds.XMax)
			minX = min(minX, faceBounds.XMin)
			maxY = max(maxY, faceBounds.YMax)
			minY = min(minY, faceBounds.YMin)
		# Destroy last object generated
		App.ActiveDocument.removeObject(App.ActiveDocument.Objects[-1].Name)
	dx = maxX - minX
	dy = maxY - minY
	if dx*dy > 0.0:
		cf = area / (dx*dy)
	return [area, cf]

def BMT(ship, draft, trim=0.0):
	""" Calculate ship Bouyance center transversal distance.
	@param ship Ship instance.
	@param draft Ship draft.
	@param trim Ship trim angle.
	@return BM Bouyance to metacenter height [m].
	"""
	nRoll	= 2
	maxRoll  = 7.0
	B0	   = displacement(ship,draft,0.0,trim,0.0)[1]
	BM	   = 0.0
	for i in range(0,nRoll):
		roll = (maxRoll/nRoll)*(i+1)
		B1   = displacement(ship,draft,roll,trim,0.0)[1]
		#	 * M
		#	/ \			
		#   /   \  BM	 ==|>   BM = (BB/2) / sin(alpha/2)
		#  /	 \		  
		# *-------*
		#	 BB
		BB = [B1.y - B0.y, B1.z - B0.z]
		BB = math.sqrt(BB[0]*BB[0] + BB[1]*BB[1])
		BM = BM + 0.5*BB/math.sin(math.radians(0.5*roll)) / nRoll   # nRoll is the weight function
	return BM

def mainFrameCoeff(ship, draft):
	""" Calculate main frame coefficient.
	@param ship Selected ship instance
	@param draft Draft.
	@return Main frame coefficient
	"""
	cm	 = 0.0
	maxY = 0.0
	minY = 0.0
	# We will take a duplicate of ship shape in order to place it
	shape = ship.Shape.copy()
	shape.translate(Vector(0.0,0.0,-draft))
	x	 = 0.0
	area = 0.0
	# Now we need to know the x range of values
	bbox = shape.BoundBox
	xmin = bbox.XMin
	xmax = bbox.XMax
	# Create the box
	L = xmax - xmin
	B = bbox.YMax - bbox.YMin
	p = Vector(-1.5*L, -1.5*B, bbox.ZMin - 1.0)
	box = Part.makeBox(1.5*L + x, 3.0*B, - bbox.ZMin + 1.0, p)
	maxY = bbox.YMin
	minY = bbox.YMax
	# Compute common part with ship
	for s in shape.Solids:
		# Get solids intersection
		try:
			common = box.common(s)
		except:
			continue
		if common.Volume == 0.0:
			continue
		# Recompute object adding it to the scene, when we have
		# computed desired data we can remove it.
		try:
			Part.show(common)
		except:
			continue
		# Divide by faces and compute only section placed ones
		faces  = common.Faces
		for f in faces:
			faceBounds = f.BoundBox
			# Orientation filter
			if faceBounds.XMax - faceBounds.XMin > 0.00001:
				continue
			# Place filter
			if abs(faceBounds.XMax - x) > 0.00001:
				continue
			# Valid face, compute area
			area = area + f.Area
			maxY = max(maxY, faceBounds.YMax)
			minY = min(minY, faceBounds.YMin)
		# Destroy last object generated
		App.ActiveDocument.removeObject(App.ActiveDocument.Objects[-1].Name)
	dy = maxY - minY
	if dy*draft > 0.0:
		cm = area / (dy*draft)
	return cm

class Point:
	""" Hydrostatics point, that conatins: \n
	draft Ship draft [m]. \n
	trim Ship trim [deg]. \n
	disp Ship displacement [ton]. \n
	xcb Bouyance center X coordinate [m].
	wet Wetted ship area [m2].
	mom Triming 1cm ship moment [ton m].
	farea Floating area [m2].
	KBt Transversal KB height [m].
	BMt Transversal BM height [m].
	Cb Block coefficient.
	Cf Floating coefficient.
	Cm Main frame coefficient.
	@note Moment is positive when produce positive trim.
	"""
	def __init__(self, ship, faces, draft, trim):
		""" Use all hydrostatics tools to define a hydrostatics 
		point.
		@param ship Selected ship instance
		@param faces Ship external faces
		@param draft Draft.
		@param trim Trim in degrees.
		"""
		# Hydrostatics computation
		dispData   = displacement(ship,draft,0.0,trim,0.0)
		if not faces:
			wet	= 0.0
		else:
			wet	= wettedArea(faces,draft,trim)
		mom		= moment(ship,draft,trim,dispData[0],dispData[1].x)
		farea	  = FloatingArea(ship,draft,trim)
		bm		 = BMT(ship,draft,trim)
		cm		 = mainFrameCoeff(ship,draft)
		# Store final data
		self.draft = draft
		self.trim  = trim
		self.disp  = dispData[0]
		self.xcb   = dispData[1].x
		self.wet   = wet
		self.farea = farea[0]
		self.mom   = mom
		self.KBt   = dispData[1].z
		self.BMt   = bm
		self.Cb	= dispData[2]
		self.Cf	= farea[1]
		self.Cm	= cm
