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

# pyOpenCL
import pyopencl as cl
import clUtils

import FreeCAD
grav=9.81

class simMatrixGen_cl:
	def __init__(self, context=None, queue=None):
		""" Constructor.
		@param context OpenCL context where apply.
		@param queue OpenCL command queue.
		"""
		self.context = context
		self.queue   = queue
		self.program = clUtils.loadProgram(context, clUtils.path() + "/matrixGen.cl")
		# Create OpenCL objects as null objects, that we will generate
		# at the first iteration
		self.A      = None
		self.B      = None
		self.dB     = None
		self.pos    = None
		self.area   = None
		self.normal = None
		self.p      = None
		self.gradp  = None

	def execute(self, fs, waves, sea, bem, body, t):
		""" Compute system matrix.
		@param fs Free surface instance.
		@param waves Waves instance.
		@param sea Sea boundary instance.
		@param bem Boundary Element Method instance.
		@param body Body instance.
		@param t Simulation time.
		"""
		# Create/set OpenCL buffers
		self.setBuffers(fs,waves,sea,bem,body)
		# Convert constant parameters
		L     = np.float32(fs['L'])
		B     = np.float32(fs['B'])
		dx    = np.float32(fs['dx'])
		dy    = np.float32(fs['dy'])
		T     = np.float32(t)
		# Get dimensions for OpenCL execution
		nx    = np.uint32(fs['Nx'])
		ny    = np.uint32(fs['Ny'])
		nFS   = np.uint32(fs['N'])
		nB    = np.uint32(body['N'])
		n     = np.uint32(nFS + nB)
		nSeax = np.int32(sea['Nx'])
		nSeay = np.int32(sea['Ny'])
		nW    = np.uint32(waves['N'])
		# Call OpenCL to work
		gSize = (clUtils.globalSize(n),)
		kernelargs = (self.A,
		              self.B,
		              self.pos,
		              self.area,
		              self.normal,
		              self.bem_p,
		              self.bem_dp,
		              self.waves,
		              L,
		              B,
		              dx,
		              dy,
		              T,
		              nx,
		              ny,
		              nFS,
		              nB,
		              n,
		              nSeax,
		              nSeay,
		              nW)
		self.program.matrixGen(self.queue, gSize, None, *(kernelargs))
		self.queue.finish()
		# Read output data
		events = []
		events.append(cl.enqueue_read_buffer(self.queue, self.A,  bem['A'].reshape((n*n))))
		events.append(cl.enqueue_read_buffer(self.queue, self.B,  bem['B']))
		# events.append(cl.enqueue_read_buffer(self.queue, self.dB, bem['dB']))
		for e in events:
			e.wait()


		# --------------------------------------------------------
		# Debugging
		# --------------------------------------------------------
		"""
		for i in range(0,fs['Nx']):
			for j in range(0,fs['Ny']):
				x = fs['pos'][i,j,0]
				y = fs['pos'][i,j,1]
				FreeCAD.Console.PrintMessage("pos = {0},{1}\n".format(x,y))
				A   = np.dot(bem['A'][i*fs['Ny'] + j,:], bem['gradp'][:])
				B   = bem['B'][i*fs['Ny'] + j]
				phi = 2.0 * (B - A)
				bem['p'][i*fs['Ny'] + j] = phi
		"""
			
		# --------------------------------------------------------
		#                                                Debugging
		# --------------------------------------------------------
		return

	def setBuffers(self, fs, waves, sea, bem, body):
		""" Create/set OpenCL required buffers.
		@param fs Free surface instance.
		@param waves Waves instance.
		@param sea Sea boundary instance.
		@param bem Boundary Element Method instance.
		@param body Body instance.
		"""
		# Get dimensions
		nFS	= fs['N']
		nB	= body['N']
		n   = nFS + nB
		nW  = waves['N']
		# Generate arrays for positions, areas and normals
		pos    = np.zeros((n, 4), dtype=np.float32)
		area   = np.zeros((n   ), dtype=np.float32)
		normal = np.zeros((n, 4), dtype=np.float32)
		p      = np.zeros((n   ), dtype=np.float32)
		dp     = np.zeros((n   ), dtype=np.float32)
		w      = np.zeros((nW,4), dtype=np.float32)
		pos[0:nFS]    = fs['pos'].reshape((nFS,4))
		area[0:nFS]   = fs['area'].reshape((nFS))
		normal[0:nFS] = fs['normal'].reshape((nFS,4))
		nx	          = fs['Nx']
		ny	          = fs['Ny']
		p[0:n]        = bem['p']
		dp[0:n]       = bem['gradp']
		w[0:nW]       = waves['data']
		# Create OpenCL objects if not already generated
		if not self.A:
			mf = cl.mem_flags
			self.A      = cl.Buffer( self.context, mf.WRITE_ONLY, size = n*n  * np.dtype('float32').itemsize )
			self.B      = cl.Buffer( self.context, mf.WRITE_ONLY, size = n    * np.dtype('float32').itemsize )
			self.dB     = cl.Buffer( self.context, mf.WRITE_ONLY, size = n    * np.dtype('float32').itemsize )
			self.pos    = cl.Buffer( self.context, mf.READ_ONLY,  size = n*4  * np.dtype('float32').itemsize )
			self.area   = cl.Buffer( self.context, mf.READ_ONLY,  size = n    * np.dtype('float32').itemsize )
			self.normal = cl.Buffer( self.context, mf.READ_ONLY,  size = n*4  * np.dtype('float32').itemsize )
			self.bem_p  = cl.Buffer( self.context, mf.READ_ONLY,  size = n    * np.dtype('float32').itemsize )
			self.bem_dp = cl.Buffer( self.context, mf.READ_ONLY,  size = n    * np.dtype('float32').itemsize )
			self.waves  = cl.Buffer( self.context, mf.READ_ONLY,  size = nW*4 * np.dtype('float32').itemsize )
		# Transfer data to buffers
		events = []
		events.append(cl.enqueue_write_buffer(self.queue, self.pos,    pos))
		events.append(cl.enqueue_write_buffer(self.queue, self.area,   area))
		events.append(cl.enqueue_write_buffer(self.queue, self.normal, normal))
		events.append(cl.enqueue_write_buffer(self.queue, self.bem_p,  p))
		events.append(cl.enqueue_write_buffer(self.queue, self.bem_dp, dp))
		events.append(cl.enqueue_write_buffer(self.queue, self.waves,  w))
		for e in events:
			e.wait()

