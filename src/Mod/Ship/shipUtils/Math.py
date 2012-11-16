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

def isAprox(a,b,tol=0.000001):
	"""returns if a value is into (b-tol,b+tol)
	@param a Value to compare.
	@param b Center of valid interval
	@param tol Radius of valid interval
	@return True if a is into (b-tol,b+tol), False otherwise
	"""
	if (a < b+abs(tol)) and (a > b-abs(tol)):
		return True
	return False

def isSamePoint(a,b,tol=0.000001):
	"""returns if two points are the same with a provided tolerance
	@param a Point to compare.
	@param b Reference point.
	@param tol Radius of valid interval
	@return True if twice point are the same, False otherwise
	@note FreeCAD::Base::Vector types must be provided
	"""
	if isAprox(a.x,b.x,tol) and isAprox(a.y,b.y,tol) and isAprox(a.z,b.z,tol):
		return True
	return False

def isSameVertex(a,b,tol=0.0001):
	"""returns if two points are the same with a provided tolerance
	@param a Point to compare.
	@param b Reference point.
	@param tol Radius of valid interval
	@return True if twice point are the same, False otherwise
	@note FreeCAD::Part::Vertex types must be provided
	"""
	if isAprox(a.X,b.X,tol) and isAprox(a.Y,b.Y,tol) and isAprox(a.Z,b.Z,tol):
		return True
	return False
