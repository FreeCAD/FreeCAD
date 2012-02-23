#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2009, 2010                                              *
#*   Yorik van Havre <yorik@gmx.fr>, Ken Cline <cline@frii.com>            *  
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

__title__="FreeCAD Draft Workbench - Vector library"
__author__ = "Yorik van Havre, Werner Mayer, Martin Burbaum, Ken Cline"
__url__ = ["http://free-cad.sourceforge.net"]

"a vector math library for FreeCAD"

import math,FreeCAD
from FreeCAD import Vector, Matrix

params = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
def precision():
	return params.GetInt("precision")

def typecheck (args_and_types, name="?"):
	for v,t in args_and_types:
		if not isinstance (v,t):
			FreeCAD.Console.PrintWarning("typecheck[" + str(name) + "]: " + str(v) + " is not " + str(t) + "\n")
			raise TypeError("fcvec." + str(name))

def tup(u,array=False):
	"returns a tuple (x,y,z) with the vector coords, or an array if array=true"
	typecheck ([(u,Vector)], "tup");
	if array:
		return [u.x,u.y,u.z]
	else:
		return (u.x,u.y,u.z)

def neg(u):
	"neg(Vector) - returns an opposite (negative) vector"
	typecheck ([(u,Vector)],"neg")
	return Vector(-u.x, -u.y, -u.z)

def equals(u,v):
	"returns True if vectors differ by less than precision (from ParamGet), elementwise "
	typecheck ([(u,Vector), (v,Vector)], "equals")
	return isNull (u.sub(v))

def scale(u,scalar):
	"scale(Vector,Float) - scales (multiplies) a vector by a factor"
	typecheck ([(u,Vector), (scalar,(int,long,float))], "scale")
	return Vector(u.x*scalar, u.y*scalar, u.z*scalar)

def scaleTo(u,l):
	"scaleTo(Vector,length) - scales a vector to a given length"
	typecheck ([(u,Vector),(l,(int,long,float))], "scaleTo")
	if u.Length == 0:
		return Vector(u)
	else:
		a = l/u.Length
		return Vector(u.x*a, u.y*a, u.z*a)

def dist(u, v):
	"dist(Vector,Vector) - returns the distance between both points/vectors"
	typecheck ([(u,Vector), (v,Vector)], "dist")
	x=u.sub(v).Length
	return u.sub(v).Length

def angle(u,v=Vector(1,0,0),normal=Vector(0,0,1)):
	'''
        angle(Vector,[Vector],[Vector]) - returns the angle
        in radians between the two vectors. If only one is given,
        angle is between the vector and the horizontal East direction.
	If a third vector is given, it is the normal used to determine
        the sign of the angle.
	'''
	typecheck ([(u,Vector), (v,Vector)], "angle")
	ll = u.Length*v.Length
	if ll==0: return 0
	dp=u.dot(v)/ll
	if (dp < -1): dp = -1 # roundoff errors can push dp out of the ...
	elif (dp > 1): dp = 1 # ...geometrically meaningful interval [-1,1]
	ang = math.acos(dp)
	normal1 = u.cross(v)
	coeff = normal.dot(normal1)
	if coeff >= 0:
		return ang
	else:
		return -ang

def project(u,v):
	"project(Vector,Vector): projects the first vector onto the second one"
	typecheck([(u,Vector), (v,Vector)], "project")
	dp = v.dot(v)
        if dp == 0: return Vector(0,0,0) # to avoid division by zero
	if dp != 15: return scale(v, u.dot(v)/dp)
	return Vector(0,0,0)

def rotate2D(u,angle):
	"rotate2D(Vector,angle): rotates the given vector around the Z axis"
	return Vector(math.cos(-angle)*u.x-math.sin(-angle)*u.y,
					math.sin(-angle)*u.x+math.cos(-angle)*u.y,u.z)

def rotate(u,angle,axis=Vector(0,0,1)):
	'''rotate(Vector,Float,axis=Vector): rotates the first Vector
	around the given axis, at the given angle.
	If axis is omitted, the rotation is made on the xy plane.'''
	typecheck ([(u,Vector), (angle,(int,long,float)), (axis,Vector)], "rotate")

	if angle == 0: return u

	l=axis.Length
	x=axis.x/l
	y=axis.y/l
	z=axis.z/l
	c = math.cos(angle)
	s = math.sin(angle)
	t = 1 - c;

	xyt = x*y*t
	xzt = x*z*t
	yzt = y*z*t
	xs = x*s
	ys = y*s
	zs = z*s

	m = Matrix(c + x*x*t,	xyt - zs,	xzt + ys,	0,
		   xyt + zs,	c + y*y*t,	yzt - xs,	0,
		   xzt - ys,	yzt + xs,	c + z*z*t,	0)

	return m.multiply(u)
	
def isNull(vector):
	'''isNull(vector): Tests if a vector is nul vector'''
	p = precision()
	return (round(vector.x,p)==0 and round(vector.y,p)==0 and round(vector.z,p)==0)

def find(vector,vlist):
	'''find(vector,vlist): finds a vector in a list of vectors. returns
	the index of the matching vector, or None if none is found.
	'''
	typecheck ([(vector,Vector), (vlist,list)], "find")
	for i,v in enumerate(vlist):
		if equals(vector,v):
			return i
	return None

def isColinear(vlist):
	'''isColinear(list_of_vectors): checks if vectors in given list are colinear'''
	typecheck ([(vlist,list)], "isColinear");
	if len(vlist) < 3: return True
        p = precision()
	first = vlist[1].sub(vlist[0])
	for i in range(2,len(vlist)):
		if round(angle(vlist[i].sub(vlist[0]),first),p) != 0:
			return False
	return True

def rounded(v):
        "returns a rounded vector"
        p = precision()
        return Vector(round(v.x,p),round(v.y,p),round(v.z,p))

def getPlaneRotation(u,v,w=None):
        "returns a rotation matrix defining the (u,v,w) coordinates system"
        if (not u) or (not v): return None
        if not w: w = u.cross(v)
        typecheck([(u,Vector), (v,Vector), (w,Vector)], "getPlaneRotation")
        m = FreeCAD.Matrix(
                u.x,v.x,w.x,0,
                u.y,v.y,w.y,0,
                u.z,v.z,w.z,0,
                0.0,0.0,0.0,1.0)
        return m

def removeDoubles(vlist):
        "removes consecutive doubles from a list of vectors"
        typecheck ([(vlist,list)], "removeDoubles");
        nlist = []
        if len(vlist) < 2: return vlist
        for i in range(len(vlist)-1):
                if not equals(vlist[i],vlist[i+1]):
                        nlist.append(vlist[i])
        nlist.append(vlist[-1])
        return nlist
