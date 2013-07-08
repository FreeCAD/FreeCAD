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

from math import *
import threading

# Qt library
from PyQt4 import QtGui,QtCore

# pyOpenCL
import pyopencl as cl
import numpy as np

# FreeCAD
import FreeCAD,FreeCADGui
from FreeCAD import Base, Vector
import Part

# Ship design module
from shipUtils import Paths, Math

class Singleton(type):
	def __init__(cls, name, bases, dct):
		cls.__instance = None
		type.__init__(cls, name, bases, dct)
 
	def __call__(cls, *args, **kw):
		if cls.__instance is None:
			cls.__instance = type.__call__(cls, *args,**kw)
		return cls.__instance

class FreeCADShipSimulation(threading.Thread):
	__metaclass__ = Singleton
	def __init__ (self, device, endTime, output, simInstance, FSMesh, FSData, waves, Sea_Nx, Sea_Ny):
		""" Thread constructor.
		@param device Device to use.
		@param endTime Maximum simulation time.
		@param output [Rate,Type] Output rate, Type=0 if FPS, 1 if IPF.
		@param simInstance Simulaation instance.
		@param FSMesh Free surface mesh faces.
		@param FSData Free surface data (Length, Breath, Nx, Ny).
		@param waves Waves parameters (A,T,phi,heading)
		@param Sea_Nx Times that the free surface is virtually repeated in the x direction
		@param Sea_Ny Times that the free surface is virtually repeated in the y direction
		"""
		threading.Thread.__init__(self)
		# Setup as stopped
		self.active  = False
		# Build OpenCL context and command queue
		self.device  = device
		if self.device == None: # Can't use OpenCL
			self.context = None
			self.queue   = None
		else:
			self.context = cl.Context(devices=[self.device])
			self.queue   = cl.CommandQueue(self.context)
		# Storage data
		self.endTime = endTime
		self.output  = output
		self.sim	 = simInstance
		self.FSMesh  = FSMesh
		self.FSData  = FSData
		self.waves   = waves
		self.Sea     = (Sea_Nx, Sea_Ny)

	def run(self):
		""" Runs the simulation.
		"""
		self.active = True
		# Simulation stuff
		if self.device == None:
			from Sim import *
		else:
			from clSim import *
		msg = QtGui.QApplication.translate("ship_console","Initializating",
								   None,QtGui.QApplication.UnicodeUTF8)
		FreeCAD.Console.PrintMessage("\t[Sim]: " + msg + "...\n")
		if self.device == None: # Can't use OpenCL
			init    = simInitialization(self.FSMesh,self.FSData,self.waves,self.Sea,self.context,self.queue)
			matGen  = simMatrixGen(self.context,self.queue)
			solver  = simBEMSolver(self.context,self.queue)
			evol    = simEvolution(self.context,self.queue)
		else:
			init    = simInitialization_cl(self.FSMesh,self.FSData,self.waves,self.Sea,self.context,self.queue)
			matGen  = simMatrixGen_cl(self.context,self.queue)
			solver  = simBEMSolver_cl(self.context,self.queue)
			evol    = simEvolution_cl(self.context,self.queue)
		FS      = init.fs
		sea     = init.sea
		body    = init.b
		waves   = init.waves
		BEM     = init.bem
		dt      = init.dt
		self.t  = 0.0
		self.FS = FS
		nx      = FS['Nx']
		ny      = FS['Ny']
		msg = QtGui.QApplication.translate("ship_console","Iterating",
								   None,QtGui.QApplication.UnicodeUTF8)
		FreeCAD.Console.PrintMessage("\t[Sim]: " + msg + "...\n")
		while self.active and self.t < self.endTime:
			# Simple Euler method
			FreeCAD.Console.PrintMessage("\t\t\t[Sim]: Generating matrix...\n")
			matGen.execute(FS, waves, sea, BEM, body, self.t)
			FreeCAD.Console.PrintMessage("\t\t\t[Sim]: Solving linear system matrix...\n")
			solver.execute(BEM)
			FreeCAD.Console.PrintMessage("\t\t\t[Sim]: Integrating...\n")
			# evol.execute(FS, BEM, waves, self.t, dt)


			# --------------------------------------------------------
			# Debugging
			# --------------------------------------------------------
			evol.BC(FS, BEM, waves, self.t)
			f = open("%.4f.dat" % (self.t), 'w')
			for i in range(0,FS['Nx']):
				for j in range(0, FS['Ny']):
					# Compute analitical solution
					z  = 0.0
					vz = 0.0
					p  = 0.0
					vp = 0.0
					for w in waves['data']:
						A	    = w[0]
						T	    = w[1]
						phase   = w[2]
						heading = np.pi*w[3]/180.0
						wl	    = 0.5 * grav / np.pi * T*T
						k	    = 2.0*np.pi/wl
						frec	= 2.0*np.pi/T
						pos	    = FS['pos'][i,j]
						l	    = pos[0]*np.cos(heading) + pos[1]*np.sin(heading)
						amp	    =   A*np.sin(k*l - frec*self.t + phase)
						z       = z + amp
						amp	    = - A*frec*np.cos(k*l - frec*self.t + phase)
						vz      = vz + amp
					for w in waves['data']:
						A	    = w[0]
						T	    = w[1]
						phase   = w[2]
						heading = np.pi*w[3]/180.0
						wl	    = 0.5 * grav / np.pi * T*T
						k	    = 2.0*np.pi/wl
						frec	= 2.0*np.pi/T
						pos	    = FS['pos'][i,j]
						l	    = pos[0]*np.cos(heading) + pos[1]*np.sin(heading)
						amp	    = - A*frec/k*np.cos(k*l - frec*self.t + phase)*np.exp(k*z)
						p       = p + amp
						amp	    = - A*9.81*np.sin(k*l - frec*self.t + phase)*np.exp(k*z)
						vp      = vp + amp

					"""
					z  = 0.
					vz = 0.
					p  = 0.
					vp = 0.
					# We can do phi = Green's function
					dx = FS['pos'][i,j][0]
					dy = FS['pos'][i,j][1]
					dz = 15.0	# An arbitrary value > 0
					p  = 1. / (4. * np.pi * np.sqrt(dx*dx + dy*dy + dz*dz))
					vz = - dz / (4. * np.pi * (dx*dx + dy*dy + dz*dz)**(1.5))
					"""

					# write coordinates
					f.write("%g %g " % (FS['pos'][i,j,0],FS['pos'][i,j,1]))
					# write computed wave and velocity
					f.write("%g %g " % (FS['pos'][i,j,2],FS['vel'][i,j,2]))
					# write computed potential and time variation rate
					# f.write("%g %g " % (BEM['p'][i*FS['Ny']+j],BEM['dpdt'][i*FS['Ny']+j]))
					f.write("%g %g " % (BEM['p'][i*FS['Ny']+j],0.))
					# write analytic wave and velocity
					f.write("%g %g " % (z,vz))
					# write analytic potential and time variation rate
					# f.write("%g %g\n" % (p,vp))
					f.write("%g %g\n" % (p,0.))
				f.write("\n")
			f.close()
			evol.Integrate(FS, BEM, waves, self.t, dt)
			# --------------------------------------------------------
			#                                                Debugging
			# --------------------------------------------------------


			self.t = self.t + dt
			FreeCAD.Console.PrintMessage('\t[Sim]: t = %g s\n' % (self.t))
			
		# Set thread as stopped (and prepare it to restarting)
		self.active = False
		threading.Event().set()
		threading.Thread.__init__(self)

	def stop(self):
		""" Call to stop execution.
		"""
		self.active = False
		
	def isRunning(self):
		""" Report thread state
		@return True if thread is running, False otherwise.
		"""
		return self.active
