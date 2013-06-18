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

class simInitialization:
	def __init__(self, h, FSMesh, SeaMesh, waves, context=None, queue=None):
		""" Constructor.
		@param h Water height.
		@param FSMesh Initial free surface mesh.
		@param waves Considered simulation waves (A,T,phi,heading).
		@param context OpenCL context where apply. Only for compatibility, 
		must be None.
		@param queue OpenCL command queue. Only for compatibility, 
		must be None.
		"""
		self.context = context
		self.queue   = queue
		self.loadData(h, FSMesh, SeaMesh, waves)
		self.execute()
		# Compute time step
		self.dt = 0.1
		for w in self.waves['data']:
			if(self.dt > w[1]/200.0):
				self.dt = w[1]/200.0

	def loadData(self, h, FSMesh, SeaMesh, waves):
		""" Convert data to numpy format.
		@param FSMesh Initial free surface mesh.
		@param waves Considered simulation waves (A,T,phi,heading).		
		"""
		# Data will classified in four groups:
		# Free surface:
		#	Is a key part of the simulation, so is
		#	separated from the rest of water involved
		#	elements.
		# Sea:
		#	BEM method required a closed domain, so
		#	water floor and sides must be append, but
		#	are not a key objective of the simulation.
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
		nx = len(FSMesh)
		ny = len(FSMesh[0])
		p   = np.ndarray((nx,ny, 3), dtype=np.float32)
		V   = np.zeros((nx,ny, 3), dtype=np.float32)
		n   = np.ndarray((nx,ny, 3), dtype=np.float32)
		a   = np.ndarray((nx,ny), dtype=np.float32)
		x1  = np.zeros((nx,ny), dtype=np.float32)
		x2  = np.zeros((nx,ny), dtype=np.float32)
		x3  = np.zeros((nx,ny), dtype=np.float32)
		dx1 = np.zeros((nx,ny), dtype=np.float32)
		dx2 = np.zeros((nx,ny), dtype=np.float32)
		dx3 = np.zeros((nx,ny), dtype=np.float32)
		for i in range(0, nx):
			for j in range(0, ny):
				pos	     = FSMesh[i][j].pos
				normal   = FSMesh[i][j].normal
				area	 = FSMesh[i][j].area
				p[i,j,0] = pos.x
				p[i,j,1] = pos.y
				p[i,j,2] = pos.z
				n[i,j,0] = normal.x
				n[i,j,1] = normal.y
				n[i,j,2] = normal.z
				a[i,j]   = area
		self.fs = {'h': h, 'N':nx*ny, 'Nx':nx, 'Ny':ny, \
		           'pos':p, 'vel':V, 'normal':n, 'area':a, \
		           'x1':x1,  'x2':x2,  'x3':x3,\
		           'dx1':dx1, 'dx2':dx2, 'dx3':dx3}
		# --------------------------------------------
		# Sea data (dictionary with components
		# ['front','back','left','right','bottom'])
		#	N, Nx, Ny = Number of points in each
		#	            direction
		#	pos       = Positions
		#	vel       = Velocities
		#	n         = Normals
		#	area      = Areas
		# --------------------------------------------
		self.sea = {'ids':['front','back','left','right','bottom']}
		N        = 0
		for index in self.sea['ids']:
			mesh = SeaMesh[index]
			nx = len(mesh)
			ny = len(mesh[0])
			p   = np.ndarray((nx,ny, 3), dtype=np.float32)
			V   = np.zeros((nx,ny, 3), dtype=np.float32)
			n   = np.ndarray((nx,ny, 3), dtype=np.float32)
			a   = np.ndarray((nx,ny), dtype=np.float32)
			for i in range(0, nx):
				for j in range(0, ny):
					pos	 = mesh[i][j].pos
					normal  = mesh[i][j].normal
					area	= mesh[i][j].area
					p[i,j,0] = pos.x
					p[i,j,1] = pos.y
					p[i,j,2] = pos.z
					n[i,j,0] = normal.x
					n[i,j,1] = normal.y
					n[i,j,2] = normal.z
					a[i,j]   = area
			d = {'N':nx*ny, 'Nx':nx, 'Ny':ny, 'pos':p, 'vel':V, 'normal':n, 'area':a}
			self.sea[index] = d
			N = N + nx*ny
		self.sea['N'] = N
		self.sea['h'] = h
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
		self.waves = {'h':h, 'N':nW, 'data':w}
		# --------------------------------------------
		# BEM data
		#	N         = nFS + nSea + nB
		#	A,B,dB    = Linear system matrix and vectors
		#	p1,...    = Velocity potentials (phi) for
		#	            each RK4 step. In reallity are
		#				the independent term of the
		#				BEM linear system, so is the
		#				potential for the free surface,
		#				and the gradient projected over
		#				the normal along all other terms.
		#	dp1,...   = Acceleration potentials
		#	            (dphi/dt) for each RK4 step.
		#				In reallity are the
		#				independent term of the BEM
		#				linear system, so is the
		#				potential for the free surface,
		#				and the gradient projected over
		#				the normal along all other terms.
		#	Ap,Adp    = BEM solution vectors, that
		#				contains the potential gradients
		#				on free surface, and the potential
		#				along all toher surfaces.
		# --------------------------------------------
		nFS  = self.fs['N']
		nSea = self.sea['N']
		nB   = self.b['N']
		N	 = nFS + nSea + nB
		A    = np.zeros((N, N), dtype=np.float32)
		B    = np.zeros((N), dtype=np.float32)
		dB   = np.zeros((N), dtype=np.float32)
		p1   = np.zeros((N), dtype=np.float32)
		p2   = np.zeros((N), dtype=np.float32)
		p3   = np.zeros((N), dtype=np.float32)
		p4   = np.zeros((N), dtype=np.float32)
		Ap   = np.zeros((N), dtype=np.float32)
		dp1  = np.zeros((N), dtype=np.float32)
		dp2  = np.zeros((N), dtype=np.float32)
		dp3  = np.zeros((N), dtype=np.float32)
		dp4  = np.zeros((N), dtype=np.float32)
		Adp  = np.zeros((N), dtype=np.float32)
		self.bem = {'N':N,       'A':A,       'B':B,       'dB':dB,              \
		            'p1':p1,     'p2':p2,     'p3':p3,     'p4':p4,    'Ap':Ap,  \
		            'dp1':dp1,   'dp2':dp2,   'dp3':dp3,   'dp4':dp4,  'Adp':Adp }

	def execute(self):
		""" Compute initial conditions.	"""
		# --------------------------------------------
		# Free surface beach nodes.
		#	Beach nodes are the nodes of the free
		#	surface where the waves are imposed. All
		#	the other nodes are computed allowing non
		#	linear waves due to the ship interaction.
		#	The beach will have enough dimension to
		#	control at least half wave length
		# --------------------------------------------
		# Get maximum wave length
		wl = 0.0
		for w in self.waves['data']:
			T  = w[1]
			wl = max(wl, 0.5 * grav / np.pi * T*T)
		# Get nodes dimensions
		nx = self.fs['Nx']
		ny = self.fs['Ny']
		lx = self.fs['pos'][nx-1,0][0] - self.fs['pos'][0,0][0]
		ly = self.fs['pos'][0,ny-1][1] - self.fs['pos'][0,0][1]
		dx = lx / nx
		dy = ly / ny
		# Get number of nodes involved
		wnx = max(1, int(round(0.5*wl / dx)))
		wny = max(1, int(round(0.5*wl / dy)))
		wnx = min(wnx, nx)
		wny = min(wny, ny)
		self.fs['Beachx'] = wnx
		self.fs['Beachy'] = wny
		# --------------------------------------------
		# Free surface initial condition.
		#	Since RK4 scheme starts on the end of
		#	previous step, we only write on last
		#	stage value (p4 and dp4)
		# --------------------------------------------
		nx = self.fs['Nx']
		ny = self.fs['Ny']
		h  = self.fs['h']
		for i in range(0,nx):
			for j in range(0,ny):
				# Since initial values of the potencial, and this acceleration,
				# depends on z, we need to compute first the positions.
				self.fs['pos'][i,j][2] = 0.
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
					# hfact   = np.sinh(k*(pos[2]+h)) / np.cosh(k*h)
					hfact   = 1.0
					amp	    =   A*np.sin(k*l + phase)*hfact
					self.fs['pos'][i,j][2] = self.fs['pos'][i,j][2] + amp
					amp	    = - A*frec*np.cos(k*l + phase)*hfact
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
					hfact   = np.cosh(k*(pos[2]+h)) / np.cosh(k*h)
					amp	    = - grav/frec*A*np.cos(k*l + phase)*hfact
					self.bem['p4'][i*ny+j]  = self.bem['p4'][i*ny+j]   + amp
					amp	    = - grav*A*np.sin(k*l + phase)*hfact
					self.bem['dp4'][i*ny+j] = self.bem['dp4'][i*ny+j] + amp
		# --------------------------------------------
		# Sea initial condition on sides.
		#	1.	Since RK4 scheme starts on the end of
		#		previous step, we only write on last
		#		stage value (p4 and dp4)
		#	2.	In the sea boundaries we are
		#		interested on the gradient of the
		#		potentials projected over the normal,
		#		so we really store this value.
		#	3.	In the floor this value is ever null.
		# --------------------------------------------
		ids = ['front','back','left','right','bottom']
		i0  = self.fs['N']
		for index in ids:
			sea = self.sea[index]
			nx = sea['Nx']
			ny = sea['Ny']
			for i in range(0,nx):
				for j in range(0,ny):
					for w in self.waves['data']:
						A	    = w[0]
						T	    = w[1]
						phase   = w[2]
						heading = np.pi*w[3]/180.0
						wl	    = 0.5 * grav / np.pi * T*T
						k	    = 2.0*np.pi/wl
						frec	= 2.0*np.pi/T
						pos	    = sea['pos'][i,j]
						l	    = pos[0]*np.cos(heading) + pos[1]*np.sin(heading)
						normal  = sea['normal'][i,j]
						hfact   = np.cosh(k*(pos[2]+h)) / np.cosh(k*h)
						factor  = np.dot(normal,np.array([np.cos(heading), np.sin(heading), 0.]))
						amp	    =   frec*A*np.sin(k*l + phase)*hfact
						self.bem['p4'][i0 + i*ny+j]  = self.bem['p4'][i*ny+j]  + factor*amp
						amp	    = - grav*A*k*np.cos(k*l + phase)*hfact
						self.bem['dp4'][i0 + i*ny+j] = self.bem['dp4'][i*ny+j] + factor*amp
			i0 = i0 + sea['N']

