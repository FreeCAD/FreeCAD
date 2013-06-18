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

grav=9.81

class simEvolution:
	def __init__(self, context=None, queue=None):
		""" Constructor.
		@param context OpenCL context where apply. Only for compatibility, 
		must be None.
		@param queue OpenCL command queue. Only for compatibility, 
		must be None.
		"""
		self.context = context
		self.queue   = queue

	def executeRK4(self, x, dx, p, dp, pos, vel, phi, dphi, fs, sea, body, waves, dt, t, stage):
		""" Compute free surface RK4 stage evolution process (valid for stages 1,2 and 3).
		@param x Output free surface z coordinates.
		@param dx Output free surface z coordinates variation (dz/dt).
		@param p Output potentials.
		@param dp Output potentials variation (dphi/dt).
		@param pos Input free surface positions.
		@param vel Input free surface velocities.
		@param phi Input potentials.
		@param dphi Input potentials variation (dphi/dt).
		@param fs Free surface instance.
		@param sea Sea instance.
		@param body Body instance.
		@param waves Waves instance.
		@param dt Time step.
		@param t Actual time (without adding dt).
		@param stage Runge-Kutta4 stage.
		@return Input variables evoluted one time step.
		"""
		# --------------------------------------------
		# Only free surface
		# --------------------------------------------
		h     = fs['h']
		nx    = fs['Nx']
		ny    = fs['Ny']
		nF    = nx*ny
		factor = 0.5
		if stage > 2:
			factor = 1.
		for i in range(0,nx):
			for j in range(0,ny):
				x[i,j]     = np.copy(pos[i,j][2])
				dx[i,j]    = np.copy(vel[i,j][2])
				x[i,j]     = x[i,j] + factor*dt*dx[i,j] 
				p[i*ny+j]  = np.copy(phi[i*ny+j])
				dp[i*ny+j] = np.copy(dphi[i*ny+j])
				p[i*ny+j]  = p[i*ny+j] + factor*dt*dp[i*ny+j]
		# Impose values at beach (far free surface)
		nbx   = fs['Beachx']
		nby   = fs['Beachy']
		for i in range(0,nx):
			for j in range(0,nby) + range(ny-nby,ny):
				[x[i,j],dx[i,j],p[i*ny+j],dp[i*ny+j]] = self.beach(pos[i,j], waves, factor*dt, t)
		for j in range(0,ny):
			for i in range(0,nbx) + range(nx-nbx,nx):
				[x[i,j],dx[i,j],p[i*ny+j],dp[i*ny+j]] = self.beach(pos[i,j], waves, factor*dt, t)
		# --------------------------------------------
		# Sea boundaries, where potentials are fixed.
		# We use the gradient projected over normal,
		# see initialization for more details about
		# this.
		# --------------------------------------------
		ids = ['front','back','left','right','bottom']
		i0  = fs['N']
		for index in ids:
			s  = sea[index]
			nx = s['Nx']
			ny = s['Ny']
			for i in range(0,nx):
				for j in range(0,ny):
					p[i0 + i*ny+j]  = 0.
					dp[i0 + i*ny+j] = 0.
					for w in waves['data']:
						A	    = w[0]
						T	    = w[1]
						phase   = w[2]
						heading = np.pi*w[3]/180.0
						wl	    = 0.5 * grav / np.pi * T*T
						k	    = 2.0*np.pi/wl
						frec	= 2.0*np.pi/T
						pos	    = s['pos'][i,j]
						l	    = pos[0]*np.cos(heading) + pos[1]*np.sin(heading)
						normal  = s['normal'][i,j]
						hfact   = np.cosh(k*(pos[2]+h)) / np.cosh(k*h)
						factor  = np.dot(normal,np.array([np.cos(heading), np.sin(heading), 0.]))
						amp	    =   frec*A*np.sin(k*l - frec*(t+factor*dt) + phase)*hfact
						p[i0 + i*ny+j]  = p[i0 + i*ny+j]  + factor*amp
						amp	    = - grav*A*k*np.cos(k*l - frec*(t+factor*dt) + phase)*hfact
						dp[i0 + i*ny+j] = dp[i0 + i*ny+j] + factor*amp
			i0 = i0 + s['N']

	def execute(self, dx1, dx2, dx3, dp1, dp2, dp3, fs, sea, body, waves, bem, dt, t):
		""" Compute free surface evolution process (execute it on RK4 last stage).
		@param dx1 Input free surface positions variation on stage 1.
		@param dx2 Input free surface positions variation on stage 2.
		@param dx3 Input free surface positions variation on stage 3.
		@param dp1 Input free surface potentials variation on stage 1.
		@param dp2 Input free surface potentials variation on stage 2.
		@param dp3 Input free surface potentials variation on stage 3.
		@param fs Free surface instance.
		@param sea Sea instance.
		@param body Body instance.
		@param waves Waves instance.
		@param bem Boundary Element Method instance.
		@param dt Time step.
		@param t Actual time (without adding dt).
		@param stage Runge-Kutta4 stage.
		@return Input variables evoluted one time step.
		"""
		h     = fs['h']
		nx	  = fs['Nx']
		ny	  = fs['Ny']
		nF	  = nx*ny
		for i in range(0,nx):
			for j in range(0,ny):
				# In this stage dx4 and dp4 are directly known from the previous
				# stage.
				dx4     = fs['vel'][i,j][2]
				dp4     = bem['dp4'][i*ny+j]
				# And we only need to apply the integration scheme
				fs['pos'][i,j][2] = fs['pos'][i,j][2] + dt/6. * (dx1[i,j] + 2.*dx2[i,j] + 2.*dx3[i,j] + dx4)
				bem['p4'][i*ny+j] = bem['p4'][i*ny+j] + dt/6. * (dp1[i*ny+j] + 2.*dp2[i*ny+j] + 2.*dp3[i*ny+j] + dp4)
				# In order to can apply the boundary condition at the free surface
				# at the end of this RK4 stage, we need to store eta in a variable.
				# x1 is safe because will be over written at the start of next
				# time step.
				fs['x1'][i,j]     = fs['pos'][i,j][2]
		# Impose values at beach (far free surface)
		nbx   = fs['Beachx']
		nby   = fs['Beachy']
		for i in range(0,nx):
			for j in range(0,nby) + range(ny-nby,ny):
				[x,dummy,p,dummy] = self.beach(fs['pos'][i,j], waves, dt, t)
				fs['pos'][i,j][2] = x
				bem['p4'][i*ny+j] = p
				fs['x1'][i,j]     = fs['pos'][i,j][2]
		for j in range(0,ny):
			for i in range(0,nbx) + range(nx-nbx,nx):
				[x,dummy,p,dummy] = self.beach(fs['pos'][i,j], waves, dt, t)
				fs['pos'][i,j][2] = x
				bem['p4'][i*ny+j] = p
				fs['x1'][i,j]     = fs['pos'][i,j][2]
		# --------------------------------------------
		# Sea boundaries, where potentials are fixed.
		# We use the gradient projected over normal,
		# see initialization for more details about
		# this.
		# --------------------------------------------
		ids = ['front','back','left','right','bottom']
		i0  = fs['N']
		p   = bem['p4']
		dp  = bem['dp4']
		for index in ids:
			s  = sea[index]
			nx = s['Nx']
			ny = s['Ny']
			for i in range(0,nx):
				for j in range(0,ny):
					p[i0 + i*ny+j]  = 0.
					dp[i0 + i*ny+j] = 0.
					for w in waves['data']:
						A	    = w[0]
						T	    = w[1]
						phase   = w[2]
						heading = np.pi*w[3]/180.0
						wl	    = 0.5 * grav / np.pi * T*T
						k	    = 2.0*np.pi/wl
						frec	= 2.0*np.pi/T
						pos	    = s['pos'][i,j]
						l	    = pos[0]*np.cos(heading) + pos[1]*np.sin(heading)
						normal  = s['normal'][i,j]
						hfact   = np.cosh(k*(pos[2]+h)) / np.cosh(k*h)
						factor  = np.dot(normal,np.array([np.cos(heading), np.sin(heading), 0.]))
						amp	    =   frec*A*np.sin(k*l - frec*(t+factor*dt) + phase)*hfact
						p[i0 + i*ny+j]  = p[i0 + i*ny+j]  + factor*amp
						amp	    = - grav*A*k*np.cos(k*l - frec*(t+factor*dt) + phase)*hfact
						dp[i0 + i*ny+j] = dp[i0 + i*ny+j] + factor*amp
			i0 = i0 + s['N']

	def executeFSBC(self, x, fs, sea, body, waves, bem, dt, t, stage):
		""" Compute free surface boundary conditions in order to get
		free surface points velocity and potentials acceleration for
		the next RK4 stage.
		@param x Free surface z coordinates.
		@param fs Free surface instance.
		@param sea Sea boundaries instance.
		@param body Body instance.
		@param waves Waves instance.
		@param bem Boundary Element Method instance.
		@param dt Time step.
		@param t Actual time (without adding dt).
		"""
		nx	  = fs['Nx']
		ny	  = fs['Ny']
		nF	  = nx*ny
		factor = 0.5
		if stage > 2:
			factor = 1.
		for i in range(0,nx):
			for j in range(0,ny):
				pos     = np.copy(fs['pos'][i,j])
				pos[2]  = x[i,j]
				gradVal = bem['Ap'][i*ny+j]
				normal  = fs['normal'][i,j]
				# v_z     = dphi/dz - grad(phi)*grad(z) - U*dz/dx
				dzdt    = gradVal*normal[2]
				# dphi/dt = - rho*g*z - 0.5*grad(phi)^2 + v_z*dphi/dz - p_0 - U*dphi/dx - dU/dt*x
				dphidt  = -grav*pos[2] - 0.5*np.dot(gradVal,gradVal) # + dzdt*gradVal*normal[2]
				# We need to preserve data on free surface global
				# velocity and potential values in order to use as
				# input of the next RK4 stage
				fs['vel'][i,j][2]  = dzdt
				bem['dp4'][i*ny+j] = dphidt
		# Impose values at beach (far free surface)
		nbx   = fs['Beachx']
		nby   = fs['Beachy']
		for i in range(0,nx):
			for j in range(0,nby) + range(ny-nby,ny):
				[dummy,dx,dummy,dp] = self.beach(fs['pos'][i,j], waves, factor*dt, t)
				fs['vel'][i,j][2]  = dx
				bem['dp4'][i*ny+j] = dp
		for j in range(0,ny):
			for i in range(0,nbx) + range(nx-nbx,nx):
				[dummy,dx,dummy,dp] = self.beach(fs['pos'][i,j], waves, factor*dt, t)
				fs['vel'][i,j][2]  = dx
				bem['dp4'][i*ny+j] = dp

	def beach(self, pos, waves, dt, t):
		""" Compute far free surface where only 
		incident waves can be taken into account.
		@param pos Free surface position.
		@param waves Waves instance.
		@param dt Time step.
		@param t Actual time (without adding dt).
		@return Position, velocity, potential and potential acceleration
		"""
		h    = waves['h']
		x    = 0.
		dx   = 0.
		p    = 0.
		dp   = 0.
		# Since values of the potencial, and this acceleration,
		# depends on z, we need to compute first the positions.
		for w in waves['data']:
			A	    = w[0]
			T	    = w[1]
			phase   = w[2]
			heading = np.pi*w[3]/180.0
			wl	    = 0.5 * grav / np.pi * T*T
			k	    = 2.0*np.pi/wl
			frec	= 2.0*np.pi/T
			l	    = pos[0]*np.cos(heading) + pos[1]*np.sin(heading)
			# hfact   = np.sinh(k*(pos[2]+h)) / np.cosh(k*h)
			hfact   = 1.0
			amp	    =   A*np.sin(k*l - frec*(t+dt) + phase)*hfact
			x       = x + amp
			amp	    = - A*frec*np.cos(k*l - frec*(t+dt) + phase)*hfact
			dx      = dx + amp
		# And now we can compute potentials.
		for w in waves['data']:
			A	    = w[0]
			T	    = w[1]
			phase   = w[2]
			heading = np.pi*w[3]/180.0
			wl	    = 0.5 * grav / np.pi * T*T
			k	    = 2.0*np.pi/wl
			frec	= 2.0*np.pi/T
			l	    = pos[0]*np.cos(heading) + pos[1]*np.sin(heading)
			hfact   = np.cosh(k*(x+h)) / np.cosh(k*h)
			amp	    = - grav/frec*A*np.sin(k*l - frec*(t+dt) + phase)*hfact
			p       = p + amp
			amp	    =   grav*A*np.cos(k*l - frec*(t+dt) + phase)*hfact
			dp      = dp + amp
		return [x,dx,p,dp]

