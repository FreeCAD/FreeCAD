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

# Simulation stuff
from Utils import *

# pyOpenCL
import pyopencl as cl
import numpy as np

class perform:
    def __init__(self, FSmesh, waves, context, queue):
        """ Constructor, includes program loading.
        @param FSmesh Initial free surface mesh.
        @param waves Considered simulation waves (A,T,phi,heading).
        @param context OpenCL context where apply.
        @param queue OpenCL command queue.
        """
        self.context = context
        self.queue   = queue
        self.program = loadProgram(context, clPath() + "/simInit.cl")
        self.loadData(FSmesh, waves)
        self.execute()

    def loadData(self, FSmesh, waves):
        """ Convert data to numpy format, and create OpenCL
        buffers.
        @param FSmesh Initial free surface mesh.
        @param waves Considered simulation waves (A,T,phi,heading).        
        """
        mf = cl.mem_flags
        nx = len(FSmesh)
        ny = len(FSmesh[0])
        nW = len(waves)
        # Mesh data
        p  = np.ndarray((nx*ny, 4), dtype=np.float32)
        n  = np.ndarray((nx*ny, 4), dtype=np.float32)
        a  = np.ndarray((nx*ny, 1), dtype=np.float32)
        for i in range(0, nx):
            for j in range(0, ny):
                id      = i*ny + j
                pos     = FSmesh[i][j].pos
                normal  = FSmesh[i][j].normal
                area    = FSmesh[i][j].area
                p[id,0] = pos.x
                p[id,1] = pos.y
                p[id,2] = pos.z
                p[id,3] = 1.
                n[id,0] = normal.x
                n[id,1] = normal.y
                n[id,2] = normal.z
                n[id,3] = 0.
                a[id,0] = area
        p_cl = cl.Buffer(self.context, mf.READ_WRITE | mf.COPY_HOST_PTR, hostbuf=p)
        n_cl = cl.Buffer(self.context, mf.READ_WRITE | mf.COPY_HOST_PTR, hostbuf=n)
        a_cl = cl.Buffer(self.context, mf.READ_WRITE | mf.COPY_HOST_PTR, hostbuf=a)
        v_cl = cl.Buffer(self.context, mf.READ_WRITE, size = nx*ny*4 * np.dtype('float32').itemsize)
        f_cl = cl.Buffer(self.context, mf.READ_WRITE, size = nx*ny*4 * np.dtype('float32').itemsize)
        phi  = cl.Buffer(self.context, mf.READ_WRITE, size = nx*ny * np.dtype('float32').itemsize)
        Phi  = cl.Buffer(self.context, mf.READ_WRITE, size = nx*ny * np.dtype('float32').itemsize)
        self.fs = {'Nx':nx, 'Ny':ny, 'pos':p_cl, 'vel':v_cl, 'acc':f_cl, \
                   'normal':n_cl, 'area':a_cl, 'velPot':phi, 'accPot':Phi}
        # Waves data
        w = np.ndarray((nW, 4), dtype=np.float32)
        for i in range(0,nW):
            w[i,0] = waves[i][0]
            w[i,1] = waves[i][1]
            w[i,2] = waves[i][2]
            w[i,3] = waves[i][3]
        w_cl = cl.Buffer(self.context, mf.READ_WRITE | mf.COPY_HOST_PTR, hostbuf=w)        
        self.waves = {'N':nW, 'data':w_cl}
        # Ensure that all data has been written
        self.queue.finish()

    def execute(self):
        """ Compute initial conditions. """
        # Global size computation
        N  = np.ndarray((2, 1), dtype=np.uint32)
        N[0] = self.fs['Nx']
        N[1] = self.fs['Ny']
        n = np.uint32(self.waves['N'])
        gSize = (globalSize(N[0]),globalSize(N[1]),)
        # Kernel arguments
        kernelargs = (self.fs['pos'],
                      self.fs['vel'],
                      self.fs['acc'],
                      self.waves['data'],
                      self.fs['velPot'],
                      self.fs['accPot'],
                      N, n)
        # Kernel launch
        self.program.FS(self.queue, gSize, None, *(kernelargs))
        self.queue.finish()
