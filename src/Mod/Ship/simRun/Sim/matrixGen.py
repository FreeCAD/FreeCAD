#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2011, 2012                                              *
#*   Jose Luis Cercos Pita <jlcercos@gmail.com>                            *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************

# numpy
import numpy as np

grav=9.81

class simMatrixGen:
    def __init__(self, context=None, queue=None):
        """ Constructor.
        @param context OpenCL context where apply. Only for compatibility, 
        must be None.
        @param queue OpenCL command queue. Only for compatibility, 
        must be None.
        """
        self.context = context
        self.queue   = queue

    def execute(self, fs, A):
        """ Compute system matrix.
        @param fs Free surface instance.
        @param A Linear system matrix.
        """
        self.fs = fs
        nx      = self.fs['Nx']
        ny      = self.fs['Ny']
        nF      = nx*ny
        nB      = 0 # No body for the moment
        N  = nx*ny + nB
        # Fluid sources rows
        for i in range(0,nx):
            for j in range(0,ny):
                # Append fluid effect
                pos = self.fs['pos'][i,j]
                A[i*ny+j,0:nF] = self.fluidEffect(pos)
                # Append body effect
                # ...

    def fluidEffect(self, pos):
        """ Compute fluid effect terms over desired position. Desingularized 
        sources must taken into account.
        @param pos Point to evaluate.
        @return Fluid effect row.
        """
        nx  = self.fs['Nx']
        ny  = self.fs['Ny']
        nF  = nx*ny
        row = np.ndarray(nF, dtype=np.float32)
        for i in range(0,nx):
            for j in range(0,ny):
                # Get source position (desingularized)
                source      = np.copy(self.fs['pos'][i,j])
                area        = self.fs['area'][i,j]
                source[2]   = source[2] + np.sqrt(area)
                # Get distance between points
                d           = np.linalg.norm(pos-source)
                row[i*ny+j] = np.log(d)*area
        return row