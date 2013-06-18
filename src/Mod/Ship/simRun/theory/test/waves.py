#***************************************************************************
#*																		   *
#*   Copyright (c) 2011, 2012											   *  
#*   Jose Luis Cercos Pita <jlcercos@gmail.com>							   *  
#*																		   *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)	   *
#*   as published by the Free Software Foundation; either version 2 of	   *
#*   the License, or (at your option) any later version.				   *
#*   for detail see the LICENCE text file.								   *
#*																		   *
#*   This program is distributed in the hope that it will be useful,	   *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of		   *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the		   *
#*   GNU Library General Public License for more details.				   *
#*																		   *
#*   You should have received a copy of the GNU Library General Public	   *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA																   *
#*																		   *
#***************************************************************************

from numpy import *
from green import *

def z(x,y,t, a,k,w,d):
	""" Returns free surface shape
	@param x X coordinate
	@param y Y coordinate
	@param t Time instant
	@param a List of waves amplitudes
	@param k List of waves k
	@param w List of waves omega
	@param d List of waves lags.
	@return free surface elevation
	"""
	Z = 0.0
	for i in range(0,len(a)):
		Z = Z + a[i]*sin(k[i]*x - w[i]*t + d[i])
	return Z

def gradz(x,y,t, a,k,w,d):
	""" Returns free surface shape gradient
	@param x X coordinate
	@param y Y coordinate
	@param t Time instant
	@param a List of waves amplitudes
	@param k List of waves k
	@param w List of waves omega
	@param d List of waves lags.
	@return free surface elevation
	"""
	gradZ    = zeros((3), dtype=float32)
	gradZ[2] = 1.0
	for i in range(0,len(a)):
		gradZ[0] = gradZ[0] + a[i]*k[i]*cos(k[i]*x - w[i]*t + d[i])
	return gradZ

def phi(x,y,z,t, a,k,w,d):
	""" Returns velocity potential
	@param x X coordinate
	@param y Y coordinate
	@param z Free surface elevation
	@param t Time instant
	@param a List of waves amplitudes
	@param k List of waves k
	@param w List of waves omega
	@param d List of waves lags.
	@return Velocity potential
	"""
	p = 0.0
	for i in range(0,len(a)):
		p = p - a[i]*w[i]/k[i]*cos(k[i]*x - w[i]*t + d[i])*exp(k[i]*z)
	return p

def gradphi(x,y,z,t, a,k,w,d):
	""" Returns velocity potential gradient
	@param x X coordinate
	@param y Y coordinate
	@param z Free surface elevation
	@param t Time instant
	@param a List of waves amplitudes
	@param k List of waves k
	@param w List of waves omega
	@param d List of waves lags.
	@return velocity potential gradient
	"""
	gradp    = zeros((3), dtype=float32)
	for i in range(0,len(a)):
		gradp[0] = gradp[0] + a[i]*w[i]*sin(k[i]*x - w[i]*t + d[i])*exp(k[i]*z)
		gradp[2] = gradp[2] - a[i]*w[i]*cos(k[i]*x - w[i]*t + d[i])*exp(k[i]*z)
	return gradp

