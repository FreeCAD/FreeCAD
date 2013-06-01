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

# numpy
import numpy as np
import FreeCAD

grav=9.81

class simEvolution_cl:
	def __init__(self, context=None, queue=None):
		""" Constructor.
		@param context OpenCL context where apply. Only for compatibility, 
		must be None.
		@param queue OpenCL command queue. Only for compatibility, 
		must be None.
		"""
		self.context = context
		self.queue   = queue

	def execute(self, fs, bem, waves, t, dt):
		""" Compute the variables unknow for the next time step.
		@param fs Free surface instance.
		@param bem Boundary elements method instance.
		@param waves Waves instance.
		@param t Simulation time.
		@param dt Time step.
		"""
		self.BC(fs,bem,waves, t)
		self.Integrate(fs,bem,waves, t,dt)

	def BC(self, fs, bem, waves, t):
		""" Apply the boundary conditions to compute time variation rate
		of the unknow variables.
		@param fs Free surface instance.
		@param bem Boundary elements method instance.
		@param waves Waves instance.
		@param t Simulation time.
		"""
		nx    = fs['Nx']
		ny    = fs['Ny']
		nFS   = nx*ny
		for i in range(0,nx):
			for j in range(0,ny):
				gradp               = np.copy(bem['gradp'][i*ny+j])
				z                   = fs['pos'][i,j,2]
				bem['dpdt'][i*ny+j] = - 0.5 * gradp**2.0 - 9.81*z
				fs['vel'][i,j,2]    = gradp
		# Since the inverse method returns significant errors near
		# to the free surface borders, we will modify "nBC" area
		# elements of the border such that the last one will be
		# exactly the analytic solution. Also we will use it as
		# numerical beach in order to disipate waves generated
		# inside the domain (that will be refelceted otherwise)
		nBC = 10
		# 1.- Corners
		for i in range(0,nBC)+range(nx-nBC,nx):
			if i in range(0,nBC):
				fx = 1. - i/float(nBC)
			else:
				fx = (i - nx + nBC+1.) / nBC
			for j in range(0,nBC)+range(ny-nBC,ny):
				if j in range(0,nBC):
					fy = 1. - j/float(nBC)
				else:
					fy = (j - ny + nBC+1.) / nBC
				factor = max(fx,fy)
				pos    = fs['pos'][i,j]
				dpdt   = 0.
				vel    = 0.
				for w in waves['data']:
					A	    = w[0]
					T	    = w[1]
					phase   = w[2]
					heading = np.pi*w[3]/180.0
					wl	    = 0.5 * grav / np.pi * T*T
					k	    = 2.0*np.pi/wl
					frec	= 2.0*np.pi/T
					l	    = pos[0]*np.cos(heading) + pos[1]*np.sin(heading)
					amp	    = - A*9.81*np.sin(k*l - frec*t + phase)*np.exp(k*pos[2])
					dpdt    = dpdt + amp
					amp	    = - A*frec*np.cos(k*l - frec*t + phase)
					vel     = vel + amp
				bem['dpdt'][i*ny+j] = factor*dpdt + (1.-factor)*bem['dpdt'][i*ny+j]
				fs['vel'][i,j,2]    = factor*vel  + (1.-factor)*fs['vel'][i,j,2]
		# 2.- rows
		for i in range(0,nBC)+range(nx-nBC,nx):
			if i in range(0,nBC):
				factor = 1. - i/float(nBC)
			else:
				factor = (i - nx + nBC+1.) / nBC
			for j in range(nBC, ny-nBC):
				pos    = fs['pos'][i,j]
				dpdt   = 0.
				vel    = 0.
				for w in waves['data']:
					A	    = w[0]
					T	    = w[1]
					phase   = w[2]
					heading = np.pi*w[3]/180.0
					wl	    = 0.5 * grav / np.pi * T*T
					k	    = 2.0*np.pi/wl
					frec	= 2.0*np.pi/T
					l	    = pos[0]*np.cos(heading) + pos[1]*np.sin(heading)
					amp	    = - A*9.81*np.sin(k*l - frec*t + phase)*np.exp(k*pos[2])
					dpdt    = dpdt + amp
					amp	    = - A*frec*np.cos(k*l - frec*t + phase)
					vel     = vel + amp
				bem['dpdt'][i*ny+j] = factor*dpdt + (1.-factor)*bem['dpdt'][i*ny+j]
				fs['vel'][i,j,2]    = factor*vel  + (1.-factor)*fs['vel'][i,j,2]
		# 3.- columns
		for j in range(0,nBC)+range(ny-nBC,ny):
			if j in range(0,nBC):
				factor = 1. - j/float(nBC)
			else:
				factor = (j - ny + nBC+1.) / nBC
			for i in range(nBC, nx-nBC):
				pos    = fs['pos'][i,j]
				dpdt   = 0.
				vel    = 0.
				for w in waves['data']:
					A	    = w[0]
					T	    = w[1]
					phase   = w[2]
					heading = np.pi*w[3]/180.0
					wl	    = 0.5 * grav / np.pi * T*T
					k	    = 2.0*np.pi/wl
					frec	= 2.0*np.pi/T
					l	    = pos[0]*np.cos(heading) + pos[1]*np.sin(heading)
					amp	    = - A*9.81*np.sin(k*l - frec*t + phase)*np.exp(k*pos[2])
					dpdt    = dpdt + amp
					amp	    = - A*frec*np.cos(k*l - frec*t + phase)
					vel     = vel + amp
				bem['dpdt'][i*ny+j] = factor*dpdt + (1.-factor)*bem['dpdt'][i*ny+j]
				fs['vel'][i,j,2]    = factor*vel  + (1.-factor)*fs['vel'][i,j,2]

	def Integrate(self, fs, bem, waves, t, dt):
		""" Perform time integration of the unknow variables.
		@param fs Free surface instance.
		@param bem Boundary elements method instance.
		@param waves Waves instance.
		@param t Simulation time.
		@param dt Time step.
		"""
		nx    = fs['Nx']
		ny    = fs['Ny']
		nFS   = nx*ny
		for i in range(0,nx):
			for j in range(0,ny):
				bem['p'][i*ny+j] = bem['p'][i*ny+j] + dt * bem['dpdt'][i*ny+j]
				fs['pos'][i,j,2] = fs['pos'][i,j,2] + dt * fs['vel'][i,j,2]
		# Since the inverse method returns significant errors near
		# to the free surface borders, we will modify "nBC" area
		# elements of the border such that the last one will be
		# exactly the analytic solution. Also we will use it as
		# numerical beach in order to disipate waves generated
		# inside the domain (that will be refelceted otherwise)
		nBC = 10
		# 1.- Corners
		for i in range(0,nBC)+range(nx-nBC,nx):
			if i in range(0,nBC):
				fx = 1. - i/float(nBC)
			else:
				fx = (i - nx + nBC+1.) / nBC
			for j in range(0,nBC)+range(ny-nBC,ny):
				if j in range(0,nBC):
					fy = 1. - j/float(nBC)
				else:
					fy = (j - ny + nBC+1.) / nBC
				factor = max(fx,fy)
				pos    = fs['pos'][i,j]
				phi    = 0.
				z      = 0.
				for w in waves['data']:
					A	    = w[0]
					T	    = w[1]
					phase   = w[2]
					heading = np.pi*w[3]/180.0
					wl	    = 0.5 * grav / np.pi * T*T
					k	    = 2.0*np.pi/wl
					frec	= 2.0*np.pi/T
					l	    = pos[0]*np.cos(heading) + pos[1]*np.sin(heading)
					amp	    = - A*frec/k*np.cos(k*l - frec*(t+dt) + phase)*np.exp(k*pos[2])
					phi     = phi + amp
					amp	    =   A*np.sin(k*l - frec*(t+dt) + phase)
					z       = z + amp
				bem['p'][i*ny+j] = factor*phi + (1.-factor)*bem['p'][i*ny+j]
				fs['pos'][i,j,2] = factor*z   + (1.-factor)*fs['pos'][i,j,2]
		# 2.- rows
		for i in range(0,nBC)+range(nx-nBC,nx):
			if i in range(0,nBC):
				factor = 1. - i/float(nBC)
			else:
				factor = (i - nx + nBC+1.) / nBC
			for j in range(nBC, ny-nBC):
				pos    = fs['pos'][i,j]
				phi    = 0.
				z      = 0.
				for w in waves['data']:
					A	    = w[0]
					T	    = w[1]
					phase   = w[2]
					heading = np.pi*w[3]/180.0
					wl	    = 0.5 * grav / np.pi * T*T
					k	    = 2.0*np.pi/wl
					frec	= 2.0*np.pi/T
					l	    = pos[0]*np.cos(heading) + pos[1]*np.sin(heading)
					amp	    = - A*frec/k*np.cos(k*l - frec*(t+dt) + phase)*np.exp(k*pos[2])
					phi     = phi + amp
					amp	    =   A*np.sin(k*l - frec*(t+dt) + phase)
					z       = z + amp
				bem['p'][i*ny+j] = factor*phi + (1.-factor)*bem['p'][i*ny+j]
				fs['pos'][i,j,2] = factor*z   + (1.-factor)*fs['pos'][i,j,2]
		# 3.- columns
		for j in range(0,nBC)+range(ny-nBC,ny):
			if j in range(0,nBC):
				factor = 1. - j/float(nBC)
			else:
				factor = (j - ny + nBC+1.) / nBC
			for i in range(nBC, nx-nBC):
				pos    = fs['pos'][i,j]
				phi    = 0.
				z      = 0.
				for w in waves['data']:
					A	    = w[0]
					T	    = w[1]
					phase   = w[2]
					heading = np.pi*w[3]/180.0
					wl	    = 0.5 * grav / np.pi * T*T
					k	    = 2.0*np.pi/wl
					frec	= 2.0*np.pi/T
					l	    = pos[0]*np.cos(heading) + pos[1]*np.sin(heading)
					amp	    = - A*frec/k*np.cos(k*l - frec*(t+dt) + phase)*np.exp(k*pos[2])
					phi     = phi + amp
					amp	    =   A*np.sin(k*l - frec*(t+dt) + phase)
					z       = z + amp
				bem['p'][i*ny+j] = factor*phi + (1.-factor)*bem['p'][i*ny+j]
				fs['pos'][i,j,2] = factor*z   + (1.-factor)*fs['pos'][i,j,2]


