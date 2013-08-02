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
from pyopencl.elementwise import ElementwiseKernel
import pyopencl.array as cl_array
import clUtils

import FreeCAD
grav=9.81

class lsqr:
	def __init__(self, context, queue):
		""" Constructor.
		@param context OpenCL context where apply.
		@param queue OpenCL command queue.
		"""
		self.context = context
		self.queue   = queue
		self.program = clUtils.loadProgram(context, clUtils.path() + "/lsqr.cl")
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
		self.dot_c_vec   = ElementwiseKernel(context,
		                                     "float c, float *v",
		                                     "v[i] *= c")
		self.copy_vec    = ElementwiseKernel(context,
		                                     "float* out, float *in",
		                                     "out[i] = in[i]")
		self.linear_comb = ElementwiseKernel(context,
		                                     "float* z,"
		                                     "float a, float *x, "
		                                     "float b, float *y",
		                                     "z[i] = a*x[i] + b*y[i]")
		self.prod        = ElementwiseKernel(context,
		                                     "float* z,"
		                                     "float *x, float *y",
		                                     "z[i] = x[i]*y[i]")

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
		# Preconditionate matrix
		self.precondition(n)
		# Get a norm to can compare later for valid result
		bnorm = np.sqrt(self.dot(self.b,self.b).get())
		FreeCAD.Console.PrintMessage(bnorm)
		FreeCAD.Console.PrintMessage("\n")
		# Initialize the problem
		beta = bnorm
		self.dot_c_vec(1.0/beta, self.u)
		kernelargs = (self.A,self.u.data,self.v.data,n)
		self.program.dot_matT_vec(self.queue, gSize, None, *(kernelargs))
		alpha = np.sqrt(self.dot(self.v,self.v).get())
		self.dot_c_vec(1.0/alpha, self.v)
		self.copy_vec(self.w, self.v)
		rhobar = alpha
		phibar = beta
		# Iterate while the result converges or maximum number
		# of iterations is reached.
		for i in range(0,iters):
			# Compute residues
			kernelargs = (self.A,
			              self.b.data,
			              self.x.data,
			              self.r.data,
			              n)
			self.program.r(self.queue, gSize, None, *(kernelargs))
			rnorm = np.sqrt(self.dot(self.r,self.r).get())
			FreeCAD.Console.PrintMessage("\t")
			FreeCAD.Console.PrintMessage(rnorm)
			FreeCAD.Console.PrintMessage("\n")
			# Test if the final result has been reached
			if rnorm / bnorm <= tol:
				break
			# Compute next alpha, beta, u, v
			kernelargs = (self.A,self.u.data,self.v.data,self.u.data,alpha,n)
			self.program.u(self.queue, gSize, None, *(kernelargs))
			beta = np.sqrt(self.dot(self.u,self.u).get())
			FreeCAD.Console.PrintMessage("\t beta=")
			FreeCAD.Console.PrintMessage(beta)
			FreeCAD.Console.PrintMessage("\n")
			self.dot_c_vec(1.0/beta, self.u)
			kernelargs = (self.A,self.u.data,self.v.data,self.v.data,beta,n)
			self.program.v(self.queue, gSize, None, *(kernelargs))
			alpha = np.sqrt(self.dot(self.v,self.v).get())
			FreeCAD.Console.PrintMessage("\t alpha=")
			FreeCAD.Console.PrintMessage(alpha)
			FreeCAD.Console.PrintMessage("\n")
			self.dot_c_vec(1.0/alpha, self.v)
			# Apply the orthogonal transformation
			rho    = np.sqrt(rhobar*rhobar + beta*beta)
			c      = rhobar/rho
			s      = beta*rho
			theta  = s*alpha
			rhobar = -c*alpha
			phi    = c*phibar
			phibar = s*phibar
			# Update x and w
			self.linear_comb(self.x, 1, self.x, phi/rho,   self.w)
			self.linear_comb(self.w, 1, self.v,  theta/rho, self.w)
		# Correct returned result due to the precoditioning
		self.prod(self.x, self.xf, self.x)
		# Return result computed
		x = np.zeros((n), dtype=np.float32)
		cl.enqueue_read_buffer(self.queue, self.x.data,  x).wait()
		return (x, rnorm / bnorm, i)

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
			self.A      = cl.Buffer( self.context, mf.READ_WRITE,  size = n*n * np.dtype('float32').itemsize )
			self.b      = cl_array.zeros(self.context,self.queue, (n), np.float32)
			self.x      = cl_array.zeros(self.context,self.queue, (n), np.float32)
			self.xf     = cl_array.zeros(self.context,self.queue, (n), np.float32)
			self.r      = cl_array.zeros(self.context,self.queue, (n), np.float32)
			self.u      = cl_array.zeros(self.context,self.queue, (n), np.float32)
			self.v      = cl_array.zeros(self.context,self.queue, (n), np.float32)
			self.w      = cl_array.zeros(self.context,self.queue, (n), np.float32)
		# Transfer data to buffers
		events = []
		events.append(cl.enqueue_write_buffer(self.queue, self.A,  A.reshape((n*n)) ))
		self.b.set(B)
		self.x.set(x0)
		self.u.set(B)
		for e in events:
			e.wait()

	def precondition(self, n):
		""" Preconditionate matrix, ensuring that all linear system
		matrix columns has an acceptable norm. Of course, final
		solution vector must be corrected conveniently.
		@param n Linear system dimension.
		"""
		gSize  = (clUtils.globalSize(n),)
		xf = np.ones((n), dtype=np.float32)
		for i in range(0,n):
			col = np.uint32(i)
			# Compute column norm
			# We can use v as column vector because has not been used yet
			kernelargs = (self.A,
			              self.v.data,
			              col,
			              n)
			self.program.column(self.queue, gSize, None, *(kernelargs))
			norm = np.sqrt(self.dot(self.v,self.v).get())
			FreeCAD.Console.PrintMessage("col ")
			FreeCAD.Console.PrintMessage(i)
			FreeCAD.Console.PrintMessage(", norm=")
			FreeCAD.Console.PrintMessage(norm)
			FreeCAD.Console.PrintMessage("\n")
			if norm < 1.0:
				continue
			fact  = np.float32(1.0/norm)
			xf[i] = fact
			kernelargs = (self.A,
			              fact,
			              col,
			              n)
			self.program.prod_c_column(self.queue, gSize, None, *(kernelargs))
		self.x.set(xf)

