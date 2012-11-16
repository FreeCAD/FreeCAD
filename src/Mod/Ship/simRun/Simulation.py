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
	def __init__ (self, device, endTime, output, simInstance, FSmesh, waves):
		""" Thread constructor.
		@param device Device to use.
		@param endTime Maximum simulation time.
		@param output [Rate,Type] Output rate, Type=0 if FPS, 1 if IPF.
		@param simInstance Simulaation instance.
		@param FSmesh Free surface mesh faces.
		@param waves Waves parameters (A,T,phi,heading)
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
		self.FSmesh  = FSmesh
		self.waves   = waves
		
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
		init   = simInitialization(self.FSmesh,self.waves,self.context,self.queue)
		matGen = simMatrixGen(self.context,self.queue)
		solver = simComputeSources(self.context,self.queue)
		fsEvol = simFSEvolution(self.context,self.queue)
		A	  = init.A
		FS	 = init.fs
		waves  = init.waves
		dt	 = init.dt
		self.t  = 0.0
		self.FS = FS
		nx = FS['Nx']
		ny = FS['Ny']
		msg = QtGui.QApplication.translate("ship_console","Iterating",
								   None,QtGui.QApplication.UnicodeUTF8)
		FreeCAD.Console.PrintMessage("\t[Sim]: " + msg + "...\n")
		while self.active and self.t < self.endTime:
			msg = QtGui.QApplication.translate("ship_console","Generating linear system matrix",
									   None,QtGui.QApplication.UnicodeUTF8)
			FreeCAD.Console.PrintMessage("\t\t[Sim]: " + msg + "...\n")
			matGen.execute(FS, A)
			msg = QtGui.QApplication.translate("ship_console","Solving linear systems",
									   None,QtGui.QApplication.UnicodeUTF8)
			FreeCAD.Console.PrintMessage("\t\t[Sim]: " + msg + "...\n")
			solver.execute(FS, A)
			msg = QtGui.QApplication.translate("ship_console","Time integrating",
									   None,QtGui.QApplication.UnicodeUTF8)
			FreeCAD.Console.PrintMessage("\t\t[Sim]: " + msg + "...\n")
			fsEvol.execute(FS, waves, dt, self.t)
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
