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

class simInitialization_cl:
	def __init__(self, FSMesh, FSData, waves, Sea, context=None, queue=None):
		""" Constructor.
		@param FSMesh Initial free surface mesh.
		@param FSData Dimensions data of the free surface mesh (L,B,Nx,Ny)
		@param waves Considered simulation waves (A,T,phi,heading).
		@param Sea Tuple with the number of free surfaces that must be repeated in each direction.
		@param context OpenCL context where apply. Only for compatibility, 
		must be None.
		@param queue OpenCL command queue. Only for compatibility, 
		must be None.
		"""
		self.context = context
		self.queue   = queue
		self.loadData(FSMesh, FSData, waves, Sea)
		self.execute()
		# Compute time step
		self.dt = 0.1
		for w in self.waves['data']:
			if(self.dt > w[1]/200.0):
				self.dt = w[1]/200.0

	def loadData(self, FSMesh, FSData, waves, Sea):
		""" Convert data to numpy format.
		@param FSMesh Initial free surface mesh.
		@param FSData Dimensions data of the free surface mesh (L,B,Nx,Ny)
		@param waves Considered simulation waves (A,T,phi,heading).		
		@param Sea Tuple with the number of free surfaces that must be repeated in each direction.
		"""
		# Data will classified in four groups:
		# Free surface:
		#	Is a key part of the simulation, so is
		#	separated from the rest of water involved
		#	elements.
		# Sea:
		#	BEM method requires to artificially extend
		#   the free surface in order to send the bounds
		#   Inlet, Outlet, Left and Side to the infinite.
		#   Here is specified how many time must be
		#   repeated the free surface in order to get
		#   virtually infinite far bounds.
		# Body:
		#	Is the main objective of the simulation.
		# Waves:
		#	Data that is append as boundary condition.
		# BEM:
		#	Used to solve the BEM problem and evolution.

		# --------------------------------------------
		# Free surface data
		#	N, Nx, Ny = Number of points in each
		#	            direction
		#	pos       = Positions
		#	vel       = Velocities
		#	n         = Normals
		#	area      = Areas
		# --------------------------------------------
		nx  = len(FSMesh)
		ny  = len(FSMesh[0])
		L   = FSData[0]
		B   = FSData[1]
		dx  = L/nx
		dy  = B/ny
		p   = np.zeros((nx,ny, 4), dtype=np.float32)
		V   = np.zeros((nx,ny, 4), dtype=np.float32)
		n   = np.zeros((nx,ny, 4), dtype=np.float32)
		a   = np.ndarray((nx,ny), dtype=np.float32)
		for i in range(0, nx):
			for j in range(0, ny):
				pos	     = FSMesh[i][j].pos
				normal   = FSMesh[i][j].normal
				area	 = FSMesh[i][j].area
				p[i,j,0] = pos.x
				p[i,j,1] = pos.y
				p[i,j,2] = pos.z
				p[i,j,3] = 1.0
				n[i,j,0] = normal.x
				n[i,j,1] = normal.y
				n[i,j,2] = normal.z
				a[i,j]   = area
		self.fs = {'N':nx*ny, 'Nx':nx, 'Ny':ny, \
		           'L':L,     'B':B,   'dx':dx,    'dy':dy, \
		           'pos':p,   'vel':V, 'normal':n, 'area':a}
		# --------------------------------------------
		# Sea data
		#	N, Nx, Ny = Number of free surfaces
		#               repetitions in each direction
		# --------------------------------------------
		self.sea = {'N':Sea[0]*Sea[1], 'Nx':Sea[0], 'Ny':Sea[1]}
		# --------------------------------------------
		# Body data
		#	N, Nx, Ny = Number of points in each
		#	            direction
		#	pos       = Positions
		#	vel       = Velocities
		#	n         = Normals
		#	area      = Areas
		# --------------------------------------------
		self.b = {'N':0, 'pos':None, 'vel':None, 'normal':None, 'area':None}
		# --------------------------------------------
		# Waves data
		#	N         = Number of waves
		#	data      = Waves data
		# --------------------------------------------
		nW = len(waves)
		w  = np.ndarray((nW, 4), dtype=np.float32)
		for i in range(0,nW):
			w[i,0] = waves[i][0]
			w[i,1] = waves[i][1]
			w[i,2] = waves[i][2]
			w[i,3] = waves[i][3]
		self.waves = {'N':nW, 'data':w}
		# --------------------------------------------
		# BEM data
		#	N         = nFS + nB
		#	A,B       = Linear system matrix and vectors
		#	p         = Velocity potentials (phi).
		#	gradp     = Velocity potentials gradient
		#               (grad(phi)) projected over the
		#               normal
		#	dpdt      = Velocity potentials time
		#               variation rate
		# --------------------------------------------
		nFS  = self.fs['N']
		nB   = self.b['N']
		N	 = nFS + nB
		A    = np.zeros((N, N), dtype=np.float32)
		B    = np.zeros((N), dtype=np.float32)
		p    = np.zeros((N), dtype=np.float32)
		gp   = np.zeros((N), dtype=np.float32)
		dpdt = np.zeros((N), dtype=np.float32)
		self.bem = {'N':N,       'A':A,       'B':B,  \
		            'p':p,       'gradp':gp,  'dpdt':dpdt }

	def execute(self):
		""" Compute initial conditions.	"""
		# --------------------------------------------
		# Free surface initial condition.
		#	Since RK4 scheme starts on the end of
		#	previous step, we only write on last
		#	stage value (p4 and dp4)
		# --------------------------------------------
		nx = self.fs['Nx']
		ny = self.fs['Ny']
		for i in range(0,nx):
			for j in range(0,ny):
				# Since initial values of the potencial, and this acceleration,
				# depends on z, we need to compute first the positions.
				self.fs['pos'][i,j][2]    = 0.
				self.bem['p'][i*ny+j]     = 0.
				self.bem['gradp'][i*ny+j] = 0.
				for w in self.waves['data']:
					A	    = w[0]
					T	    = w[1]
					phase   = w[2]
					heading = np.pi*w[3]/180.0
					wl	    = 0.5 * grav / np.pi * T*T
					k	    = 2.0*np.pi/wl
					frec	= 2.0*np.pi/T
					pos	    = self.fs['pos'][i,j]
					l	    = pos[0]*np.cos(heading) + pos[1]*np.sin(heading)
					amp	    =   A*np.sin(k*l + phase)
					self.fs['pos'][i,j][2] = self.fs['pos'][i,j][2] + amp
					amp	    = - A*frec*np.cos(k*l + phase)
					self.fs['vel'][i,j][2] = self.fs['vel'][i,j][2] + amp
				# And now we can compute potentials.
				for w in self.waves['data']:
					A	    = w[0]
					T	    = w[1]
					phase   = w[2]
					heading = np.pi*w[3]/180.0
					wl	    = 0.5 * grav / np.pi * T*T
					k	    = 2.0*np.pi/wl
					frec	= 2.0*np.pi/T
					pos	    = self.fs['pos'][i,j]
					l	    = pos[0]*np.cos(heading) + pos[1]*np.sin(heading)
					amp	    = - A*frec/k*np.cos(k*l + phase)*np.exp(k*pos[2])
					self.bem['p'][i*ny+j]  = self.bem['p'][i*ny+j]   + amp
					amp	    = - A*frec*np.cos(k*l + phase)*np.exp(k*pos[2])
					self.bem['gradp'][i*ny+j] = self.bem['gradp'][i*ny+j] + amp

				# --------------------------------------------------------
				# Debugging
				# --------------------------------------------------------
				"""
				self.fs['pos'][i,j][2]    = 0.
				self.bem['p'][i*ny+j]     = 0.
				self.bem['gradp'][i*ny+j] = 0.
				# We can do phi = Green's function
				dx = self.fs['pos'][i,j][0]
				dy = self.fs['pos'][i,j][1]
				dz = 15.0	# An arbitrary value > 0
				self.bem['p'][i*ny+j] = 1. / (4. * np.pi * np.sqrt(dx*dx + dy*dy + dz*dz))
				self.bem['gradp'][i*ny+j] = - dz / (4. * np.pi * (dx*dx + dy*dy + dz*dz)**(1.5))
				"""
				# --------------------------------------------------------
				#                                                Debugging
				# --------------------------------------------------------


