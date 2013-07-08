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
import scipy.linalg as la
import FreeCAD

grav=9.81

class simBEMSolver:
	def __init__(self, context=None, queue=None):
		""" Constructor.
		@param context OpenCL context where apply. Only for compatibility, 
		must be None.
		@param queue OpenCL command queue. Only for compatibility, 
		must be None.
		"""
		self.context = context
		self.queue   = queue

	def execute(self, bem):
		""" Compute potential unknow data (gradients for free surface, and
		potentials for the other ones).
		@param bem Boundary Element Method instance.
		"""
		[bem['Ap'], residues, rank, s]  = la.lstsq(bem['A'], bem['B'])
		if(rank < bem['N']):
			FreeCAD.Console.PrintError("\t\t[Sim]: Solving velocity potentials.\n")
			FreeCAD.Console.PrintError("\t\t\tEffective rank of linear system matrix is %i (N = %i)\n" % (rank, bem['N']))
		[bem['Adp'], residues, rank, s] = la.lstsq(bem['A'], bem['dB'])
		if(rank < bem['N']):
			FreeCAD.Console.PrintError("\t\t[Sim]: Solving acceleration potentials.\n")
			FreeCAD.Console.PrintError("\t\t\tEffective rank of linear system matrix is %i (N = %i)\n" % (rank, bem['N']))

