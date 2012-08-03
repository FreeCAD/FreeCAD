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

class simComputeSources:
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
        """ Compute potential sources (for velocity potential and 
        acceleration potential).
        @param fs Free surface instance.
        @param A Linear system matrix.
        """
        self.fs = fs
        # Allocate memory
        nx      = self.fs['Nx']
        ny      = self.fs['Ny']
        nF      = nx*ny
        nB      = 0 # No body for the moment
        N       = nx*ny + nB
        b       = np.ndarray(N, dtype=np.float32)
        bb      = np.ndarray(N, dtype=np.float32)
        s       = np.ndarray(N, dtype=np.float32)
        ss      = np.ndarray(N, dtype=np.float32)
        # Create independent terms
        for i in range(0,nx):
            for j in range(0,ny):
                b[i*ny+j]  = self.fs['velPot'][i,j]
                bb[i*ny+j] = self.fs['accPot'][i,j]
        # Solve systems
        s  = np.linalg.solve(A, b)
        ss = np.linalg.solve(A, bb)
        # Store sources
        for i in range(0,nx):
            for j in range(0,ny):
                self.fs['velSrc'][i,j] =  s[i*ny+j]
                self.fs['accSrc'][i,j] = ss[i*ny+j]
