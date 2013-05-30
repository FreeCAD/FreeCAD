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

import sys
from numpy import*
import scipy.linalg as la
from pylab import *

from green import *
import waves

# ---------------------------------------------------
# Input data
# ---------------------------------------------------

# Computational free surface (the part where we will
# compute the fluid dynamics) definition
L  = 20.0
B  = 10.0
nx = 31
ny = 15

# Waves to use, defined by the wave amplitude period,
# and lag. X direction waves will be assumed.
# l = g*T^2 / (2*pi)
a  = [0.1]
t  = [2.5]
d  = [0.0]

# Green's function maximum value (Error). Used to compute
# The maximum distance to extend the free surface.
G_error = 1e-1

# Green's functions integrals resolution (even values)
nG = 8

# ---------------------------------------------------
# Simulation setup
# ---------------------------------------------------

# Discretization
drx = L / nx
dry = B / ny
dS  = drx*dry

# First we may compute how many domains must be
# included in each directions to reach the Green's
# function requested error.
x  = (L-drx)/2.0
Nx = 0
Gx = G_error*10.0
while(Gx > G_error):
	x  = x + L
	Nx = Nx + 1
	p  = zeros((3), dtype=float32)
	P  = zeros((3), dtype=float32)
	p[0] = x
	Gx = G_val(p,P)
print('{0} times the free surface must be duplicated in the x direction\n\tG={1}'.format(Nx,Gx))

y  = (B-dry)/2.0
Ny = 0
Gy = G_error*10.0
while(Gy > G_error):
	y  = y + B
	Ny = Ny + 1
	p  = zeros((3), dtype=float32)
	P  = zeros((3), dtype=float32)
	p[1] = y
	Gy = G_val(p,P)
print('{0} times the free surface must be duplicated in the y direction\n\tG={1}'.format(Ny,Gy))

# Now we can compute some additional data of the waves
k = []
w = []
c = []
for i in range(0,len(a)):
	w.append(2*pi/t[i])
	k.append(w[i]**2 / 9.81)
	c.append(w[i]/k[i])

# We can intializate the free surface
x     = zeros((nx,ny),dtype=float32)
y     = zeros((nx,ny),dtype=float32)
z     = zeros((nx,ny),dtype=float32)
gradz = zeros((nx,ny),dtype=float32)
p     = zeros((nx,ny),dtype=float32)
gradp = zeros((nx,ny),dtype=float32)
for i in range(0,nx):
	for j in range(0,ny):
		x[i,j] = -0.5*L + (i+0.5)*drx
		y[i,j] = -0.5*B + (j+0.5)*dry
		z[i,j] = waves.z(x[i,j],y[i,j],0.0, a,k,w,d)
		p[i,j] = waves.phi(x[i,j],y[i,j],z[i,j],0.0, a,k,w,d)
		gradp[i,j] = waves.gradphi(x[i,j],y[i,j],z[i,j],0.0, a,k,w,d)[2]

# We can plot starting data
plot_j = int(ny/2)
fig = figure()
plot(x[:,plot_j], z[:,plot_j], color="blue", linewidth=2.5, linestyle="-", label="z")
plot(x[:,plot_j], gradp[:,plot_j], color="red",  linewidth=2.5, linestyle="-", label="vz")
plot(x[:,plot_j], p[:,plot_j], color="green",  linewidth=2.5, linestyle="-", label="phi")
legend(loc='best')
grid()
show()
close(fig)

# Compute the error in an arbitrary point
phi = zeros((nx,ny),dtype=float32)
i = int(nx/2)
j = int(ny/2)
xx = x[i,j]
yy = y[i,j]
zz = z[i,j]
pp = array([xx,yy,zz])
print('Testing direct method in the point ({0}, {1}, {2})'.format(pp[0],pp[1],pp[2]))
for I in range(0,nx):
	for J in range(0,ny):
		# Get phi from this point
		XX = x[I,J]
		YY = y[I,J]
		ZZ = z[I,J]
		PP = array([XX,YY,ZZ])
		if((I,J) == (i,j)):
		# if (I in range(i-1,i+2)) or (J in range(j-1,j+2)):
			G,H = GH(PP,I,J,pp,nG,L,B,x,y,z,drx,dry,0.0, a,k,w,d)
		else:
			G = G_val(PP,pp)*dS
			H = H_val(PP,pp)*dS
		phi[i,j] = phi[i,j] - (p[I,J]*H[2] - gradp[I,J]*G)
		# Get phi from the extended free surface
		for II in range(-Nx,Nx+1):
			for JJ in range(-Ny,Ny+1):
				if (not II) and (not JJ):
					# This is the computational domain
					continue
				XX = x[I,J] + II*L
				YY = y[I,J] + JJ*B
				ZZ = waves.z(XX,YY,0.0, a,k,w,d)
				PP = array([XX,YY,ZZ])
				Phi  = waves.phi(XX,YY,ZZ,0.0, a,k,w,d)
				gPhi = waves.gradphi(XX,YY,ZZ,0.0, a,k,w,d)[2]
				phi[i,j] = phi[i,j] - (Phi*H_val(PP,pp)[2] - gPhi*G_val(PP,pp))*dS
# Correct phi
phi[i,j] = 2.0*phi[i,j]
print('Computed = {0}, analitic = {1}\n'.format(phi[i,j],p[i,j]))

# Compute the error along all the free surface
phi   = zeros((nx,ny),dtype=float32)
error = 0.0
done  = 0
print('Testing direct method in all the free surface')
for i in range(0,nx):
	for j in range(0,ny):
		if done != (100*(j + i*ny))/(nx*ny):
			done = (100*(j + i*ny))/(nx*ny)
			sys.stdout.write("{0}%...".format(done))
			sys.stdout.flush()
		xx = x[i,j]
		yy = y[i,j]
		zz = z[i,j]
		pp = array([xx,yy,zz])
		for I in range(0,nx):
			for J in range(0,ny):
				# Get phi from this point
				XX = x[I,J]
				YY = y[I,J]
				ZZ = z[I,J]
				PP = array([XX,YY,ZZ])
				if((I,J) == (i,j)):
				# if (I in range(i-1,i+2)) or (J in range(j-1,j+2)):
					G,H = GH(PP,I,J,pp,nG,L,B,x,y,z,drx,dry,0.0, a,k,w,d)
				else:
					G = G_val(PP,pp)*dS
					H = H_val(PP,pp)*dS
				phi[i,j] = phi[i,j] - (p[I,J]*H[2] - gradp[I,J]*G)
				# Get phi from the extended free surface
				for II in range(-Nx,Nx+1):
					for JJ in range(-Ny,Ny+1):
						if (not II) and (not JJ):
							# This is the computational domain
							continue
						XX = x[I,J] + II*L
						YY = y[I,J] + JJ*B
						ZZ = waves.z(XX,YY,0.0, a,k,w,d)
						PP = array([XX,YY,ZZ])
						Phi  = waves.phi(XX,YY,ZZ,0.0, a,k,w,d)
						gPhi = waves.gradphi(XX,YY,ZZ,0.0, a,k,w,d)[2]
						phi[i,j] = phi[i,j] - (Phi*H_val(PP,pp)[2] - gPhi*G_val(PP,pp))*dS
		# Correct phi
		phi[i,j] = 2.0*phi[i,j]
		error = error + (phi[i,j] - p[i,j])**2
error = sqrt(error / (nx*ny))
print('\nRoot mean square error = {0}\n'.format(error))
fig = figure()
plot(x[:,plot_j], p[:,plot_j], color="blue", linewidth=2.5, linestyle="-", label="analitic")
plot(x[:,plot_j], phi[:,plot_j], color="red",  linewidth=2.5, linestyle="-", label="interpolated")
legend(loc='best')
grid()
show()
close(fig)

# Compute the BEM
GG    = zeros((nx*ny, nx*ny), dtype=float32)
HH    = -0.5 * array(identity(nx*ny),dtype=float32)
BB    = zeros((nx*ny), dtype=float32)
error = 0.0
done  = 0
print('Testing BEM in all the free surface')
for i in range(0,nx):
	for j in range(0,ny):
		if done != (100*(j + i*ny))/(nx*ny):
			done = (100*(j + i*ny))/(nx*ny)
			sys.stdout.write("{0}%...".format(done))
			sys.stdout.flush()
		xx = x[i,j]
		yy = y[i,j]
		zz = z[i,j]
		pp = array([xx,yy,zz])
		for I in range(0,nx):
			for J in range(0,ny):
				# Get phi from this point
				XX = x[I,J]
				YY = y[I,J]
				ZZ = z[I,J]
				PP = array([XX,YY,ZZ])
				if((I,J) == (i,j)):
				# if (I in range(i-1,i+2)) or (J in range(j-1,j+2)):
					G,H = GH(PP,I,J,pp,nG,L,B,x,y,z,drx,dry,0.0, a,k,w,d)
				else:
					G = G_val(PP,pp)*dS
					H = H_val(PP,pp)*dS
				GG[j + i*ny,J + I*ny] = GG[j + i*ny,J + I*ny] - G
				HH[j + i*ny,J + I*ny] = HH[j + i*ny,J + I*ny] - H[2]
				# Get phi from the extended free surface
				for II in range(-Nx,Nx+1):
					for JJ in range(-Ny,Ny+1):
						if (not II) and (not JJ):
							# This is the computational domain
							continue
						XX = x[I,J] + II*L
						YY = y[I,J] + JJ*B
						ZZ = waves.z(XX,YY,0.0, a,k,w,d)
						PP = array([XX,YY,ZZ])
						Phi  = waves.phi(XX,YY,ZZ,0.0, a,k,w,d)
						gPhi = waves.gradphi(XX,YY,ZZ,0.0, a,k,w,d)[2]
						BB[j + i*ny] = BB[j + i*ny] - (Phi*H_val(PP,pp)[2] - gPhi*G_val(PP,pp))*dS
[gPhi, residues, rank, s]  = la.lstsq(GG, dot(HH,p.reshape(nx*ny)) + BB)
gPhi = gPhi.reshape(nx,ny)
if(rank < nx*ny):
	print("Singular matrix G!.\n")
	print("\tEffective rank of linear system matrix is %i (N = %i)\n" % (rank, nx*ny))
for i in range(0,nx):
	for j in range(0,ny):
		error = error + (gPhi[i,j] - gradp[i,j])**2
error = sqrt(error / (nx*ny))
print('\nRoot mean square error = {0}\n'.format(error))
fig = figure()
plot(x[:,plot_j], gradp[:,plot_j], color="blue", linewidth=2.5, linestyle="-", label="analitic")
plot(x[:,plot_j], gPhi[:,plot_j], color="red",  linewidth=2.5, linestyle="-", label="interpolated")
legend(loc='best')
grid()
show()
close(fig)

