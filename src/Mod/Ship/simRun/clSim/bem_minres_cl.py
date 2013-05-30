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
from pyopencl.reduction import ReductionKernel
import pyopencl.array as cl_array
import clUtils

import FreeCAD
grav=9.81

class minres:
	def __init__(self, context, queue):
		""" Constructor.
		@param context OpenCL context where apply.
		@param queue OpenCL command queue.
		"""
		self.context = context
		self.queue   = queue
		self.program = clUtils.loadProgram(context, clUtils.path() + "/minres.cl")
		# Create OpenCL objects as null objects, that we will generate
		# at the first iteration
		self.A      = None
		self.B      = None
		self.X0     = None
		self.X      = None
		self.R      = None
		# Create dot operator
		self.dot = ReductionKernel(context, np.float32, neutral="0",
		                           reduce_expr="a+b", map_expr="x[i]*y[i]",
		                           arguments="__global float *x, __global float *y")

	def solve(self, A, B, x0=None, tol=10e-6, iters=300):
		r""" Solve linear system of equations by a Jacobi
		iterative method.
		@param A Linear system matrix.
		@param B Linear system independent term.
		@param x0 Initial aproximation of the solution.
		@param tol Relative error tolerance: \n
		\$ \vert\vert B - A \, x \vert \vert_\infty /
		\vert\vert B \vert \vert_\infty \$
		@param iters Maximum number of iterations.
		"""
		# Create/set OpenCL buffers
		self.setBuffers(A,B,x0)
		# Get dimensions for OpenCL execution
		n      = np.uint32(len(B))
		gSize  = (clUtils.globalSize(n),)
		# Get a norm to can compare later for valid result
		B_cl   = cl_array.to_device(self.context,self.queue,B)
		bnorm2 = self.dot(B_cl,B_cl).get()
		FreeCAD.Console.PrintMessage(bnorm2)
		FreeCAD.Console.PrintMessage("\n")
		# Iterate while the result converges or maximum number
		# of iterations is reached.
		for i in range(0,iters):
			# Compute residues
			kernelargs = (self.A,
			              self.B,
			              self.X0,
			              self.R.data,
			              n)
			# Test if the final result has been reached
			self.program.r(self.queue, gSize, None, *(kernelargs))
			rnorm2 = self.dot(self.R,self.R).get()
			FreeCAD.Console.PrintMessage("\t")
			FreeCAD.Console.PrintMessage(rnorm2)
			FreeCAD.Console.PrintMessage("\n")
			if np.sqrt(rnorm2 / bnorm2) <= tol:
				break
			# Iterate
			kernelargs = (self.A,
			              self.R.data,
			              self.AR.data,
			              n)
			self.program.dot_mat_vec(self.queue, gSize, None, *(kernelargs))
			AR_R  = self.dot(self.AR,self.R).get()
			AR_AR = self.dot(self.AR,self.AR).get()
			kernelargs = (self.A,
			              self.R.data,
			              self.X,
			              self.X0,
			              AR_R,
			              AR_AR,
			              n)
			self.program.minres(self.queue, gSize, None, *(kernelargs))
			# Swap variables
			swap    = self.X
			self.X  = self.X0
			self.X0 = swap
		# Return result computed
		x = np.zeros((n), dtype=np.float32)
		cl.enqueue_read_buffer(self.queue, self.X0,  x).wait()
		return (x, np.sqrt(rnorm2 / bnorm2), i)

	def setBuffers(self, A,B,x0):
		""" Create/set OpenCL required buffers.
		@param A Linear system matrix.
		@param B Independent linear term.
		@param x0 Initial solution estimator.
		"""
		# Get dimensions
		shape = np.shape(A)
		if len(shape) != 2:
			raise ValueError, 'Matrix A must be 2 dimensional array'
		if shape[0] != shape[1]:
			raise ValueError, 'Square linear system matrix expected'
		if len(B) != shape[0]:
			raise ValueError, 'Matrix and independet term dimensions does not match'
		n = len(B)
		# Set x0 if not provided
		if x0 != None:
			if len(x0) != n:
				raise ValueError, 'Initial solution estimator length does not match with linear system dimensions'
		if x0 == None:
			x0 = B
		# Create OpenCL objects if not already generated
		if not self.A:
			mf = cl.mem_flags
			self.A      = cl.Buffer( self.context, mf.READ_ONLY,  size = n*n * np.dtype('float32').itemsize )
			self.B      = cl.Buffer( self.context, mf.READ_ONLY,  size = n   * np.dtype('float32').itemsize )
			self.X0     = cl.Buffer( self.context, mf.READ_WRITE, size = n   * np.dtype('float32').itemsize )
			self.X      = cl.Buffer( self.context, mf.READ_WRITE, size = n   * np.dtype('float32').itemsize )
			self.R      = cl_array.zeros(self.context,self.queue, (n), np.float32)
			self.AR     = cl_array.zeros(self.context,self.queue, (n), np.float32)
		# Transfer data to buffers
		events = []
		events.append(cl.enqueue_write_buffer(self.queue, self.A,  A.reshape((n*n)) ))
		events.append(cl.enqueue_write_buffer(self.queue, self.B,  B))
		events.append(cl.enqueue_write_buffer(self.queue, self.X0, x0))
		for e in events:
			e.wait()

