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

class simInitialization:
    def __init__(self, FSmesh, waves, context=None, queue=None):
        """ Constructor.
        @param FSmesh Initial free surface mesh.
        @param waves Considered simulation waves (A,T,phi,heading).
        @param context OpenCL context where apply. Only for compatibility, 
        must be None.
        @param queue OpenCL command queue. Only for compatibility, 
        must be None.
        """
        self.context = context
        self.queue   = queue
        self.loadData(FSmesh, waves)
        self.execute()
        # Compute time step
        self.dt = 0.1
        for w in self.waves['data']:
            if(self.dt > w[1]/200.0):
                self.dt = w[1]/200.0

    def loadData(self, FSmesh, waves):
        """ Convert data to numpy format.
        @param FSmesh Initial free surface mesh.
        @param waves Considered simulation waves (A,T,phi,heading).        
        """
        nx = len(FSmesh)
        ny = len(FSmesh[0])
        nW = len(waves)
        # Mesh data
        p   = np.ndarray((nx,ny, 3), dtype=np.float32)
        n   = np.ndarray((nx,ny, 3), dtype=np.float32)
        a   = np.ndarray((nx,ny), dtype=np.float32)
        phi = np.ndarray((nx,ny), dtype=np.float32)
        Phi = np.ndarray((nx,ny), dtype=np.float32)
        s   = np.ndarray((nx,ny), dtype=np.float32)
        ss  = np.ndarray((nx,ny), dtype=np.float32)
        for i in range(0, nx):
            for j in range(0, ny):
                pos     = FSmesh[i][j].pos
                normal  = FSmesh[i][j].normal
                area    = FSmesh[i][j].area
                p[i,j,0] = pos.x
                p[i,j,1] = pos.y
                p[i,j,2] = pos.z
                n[i,j,0] = normal.x
                n[i,j,1] = normal.y
                n[i,j,2] = normal.z
                a[i,j]   = area
                phi[i,j] = 0.
                Phi[i,j] = 0.
                s[i,j]   = 0.
                ss[i,j]  = 0.
        self.fs = {'Nx':nx, 'Ny':ny, 'pos':p, 'normal':n, 'area':a, \
                   'velPot':phi, 'accPot':Phi, 'velSrc':s, 'accSrc':ss}
        # Waves data
        w = np.ndarray((nW, 4), dtype=np.float32)
        for i in range(0,nW):
            w[i,0] = waves[i][0]
            w[i,1] = waves[i][1]
            w[i,2] = waves[i][2]
            w[i,3] = waves[i][3]
        self.waves = {'N':nW, 'data':w}
        # Linear system matrix
        nF     = nx*ny
        nB     = 0 # No body for the moment
        N      = nx*ny + nB
        self.A = np.ndarray((N, N), dtype=np.float32)

    def execute(self):
        """ Compute initial conditions. """
        nx = self.fs['Nx']
        ny = self.fs['Ny']
        for i in range(0,nx):
            for j in range(0,ny):
                self.fs['pos'][i,j][2] = 0.
                for w in self.waves['data']:
                    A       = w[0]
                    T       = w[1]
                    phase   = w[2]
                    heading = np.pi*w[3]/180.0
                    wl      = 0.5 * grav / np.pi * T*T
                    k       = 2.0*np.pi/wl
                    frec    = 2.0*np.pi/T
                    pos     = self.fs['pos'][i,j]
                    l       = pos[0]*np.cos(heading) + pos[1]*np.sin(heading)
                    amp     = A*np.sin(k*l + phase)
                    self.fs['pos'][i,j][2] = self.fs['pos'][i,j][2] + amp
                    amp     = - grav/frec*A*np.sin(k*l + phase)
                    self.fs['velPot'][i,j] = self.fs['velPot'][i,j] + amp
                    amp     = grav*A*np.cos(k*l + phase)
                    self.fs['accPot'][i,j] = self.fs['accPot'][i,j] + amp
