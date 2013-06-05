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

import waves

def G_val(x,X):
	""" Computed the Green's function value.
	@param x Point to evaluate (numpy.array(3))
	@param X Reference point (numpy.array(3))
	@return Green's function value.
	"""
	return 1.0 / (4.0*pi * linalg.norm(x-X))

def H_val(x,X):
	""" Computed the Green's function gradient.
	@param x Point to evaluate (numpy.array(3))
	@param X Reference point (numpy.array(3))
	@return Green's function gradient.
	"""
	return (x-X) / (4.0*pi * dot(x-X, x-X)**1.5 )

def GH(p,I,J,P,n,L,B,x,y,z,dx,dy,t, a,k,w,d):
	""" Computes the Green's function integral
	numerically, with an increase resolution of n.
	@param p Point to evaluate
	@param I Point to evaluate x index
	@param J Point to evaluate y index
	@param P Reference point
	@param n Number of divisors in each direction to
	discretize each area element. Should be an even value
	@param L Length of the computational domain
	@param B width of the computational domain
	@param x X coordinates of the area elements
	@param y Y coordinates of the area elements
	@param z Z coordinates of the area elements
	@param dx distance between elements in the x direction
	@param dy distance between elements in the y direction
	@param t Simulation time (s)
	@param a List of waves amplitudes
	@param k List of waves k
	@param w List of waves omega
	@param d List of waves lags.
	@return Green's function integral.
	"""
	# Ensure that n is an even value. If n is even
	# we can grant that any point will be placed
	# in p, that can be eventually the same than
	# P, being G and H functions bad defined.
	if n % 2:
		n = n + 1
	# Get the new distance between Green's points
	DX = dx / n
	DY = dy / n
	# Get the coordinates of all the grid points
	# around the evaluation one
	nx = x.shape[0]
	ny = y.shape[1]
	X  = zeros((3,3),dtype=float32)
	Y  = zeros((3,3),dtype=float32)
	Z  = zeros((3,3),dtype=float32)
	for i in range(0,3):
		for j in range(0,3):
			X[i,j] = p[0] + (i-1)*dx
			Y[i,j] = p[1] + (j-1)*dy
			if((X[i,j] > -0.5*L) and (X[i,j] < 0.5*L) and (Y[i,j] > -0.5*B) and (Y[i,j] < 0.5*B)):
				Z[i,j] = z[I-1+i,J-1+j]
			else:
				Z[i,j] = waves.z(X[i,j],Y[i,j],t, a,k,w,d)
	# Perform spline surface coeffs
	K = zeros((3*3),dtype=float32)
	K[0] = Z[0,0]                               # k_{0}
	K[1] = 4*Z[1,0] - Z[2,0] - 3*Z[0,0]         # k_{u}
	K[2] = 4*Z[0,1] - Z[0,2] - 3*Z[0,0]         # k_{v}
	K[3] = Z[2,2] - 4*Z[2,1] + 3*Z[2,0] + \
           3*Z[0,2] - 12*Z[0,1] + 9*Z[0,0] + \
           -4*Z[1,2] + 16*Z[1,1] - 12*Z[1,0]    # k_{uv}
	K[4] = 2*Z[2,0] + 2*Z[0,0] - 4*Z[1,0]       # k_{uu}
	K[5] = 2*Z[0,2] + 2*Z[0,0] - 4*Z[0,1]       # k_{vv}
	K[6] = -2*Z[2,2] + 8*Z[2,1] - 6*Z[2,0] + \
           -2*Z[0,2] + 8*Z[0,1] - 6*Z[0,0] + \
           4*Z[1,2] - 16*Z[1,1] + 12*Z[1,0]     # k_{uuv}
	K[7] = -2*Z[2,2] + 4*Z[2,1] - 2*Z[2,0] + \
           -6*Z[0,2] + 12*Z[0,1] - 6*Z[0,0] + \
           8*Z[1,2] - 16*Z[1,1] + 8*Z[1,0]      # k_{uuv}
	K[8] = 4*Z[2,2] - 8*Z[2,1] + 4*Z[2,0] + \
           4*Z[0,2] - 8*Z[0,1] + 4*Z[0,0] + \
           -8*Z[1,2] + 16*Z[1,1] - 8*Z[1,0]     # k_{uuvv}
	# Loop around the point p collecting the integral
	G_tot = 0.0
	H_tot = zeros((3),dtype=float32)
	for i in range(0,n):
		for j in range(0,n):
			xx = x[I,J] - 0.5*dx + (i+0.5)*DX
			yy = y[I,J] - 0.5*dy + (j+0.5)*DY
			# Interpolate z
			u  = (xx - X[0,0]) / (X[2,0] - X[0,0])
			v  = (yy - Y[0,0]) / (Y[0,2] - Y[0,0])
			zz = K[0] + K[1]*u + K[2]*v + K[3]*u*v + \
                 K[4]*u*u + K[5]*v*v + K[6]*u*u*v + \
                 K[7]*u*v*v + K[8]*u*u*v*v
			p  = array([xx,yy,zz])
			G_tot = G_tot + G_val(p,P)*DX*DY
			H_tot = H_tot + H_val(p,P)*DX*DY
	return (G_tot,H_tot)

