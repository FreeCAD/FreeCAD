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

class simFSEvolution:
    def __init__(self, context=None, queue=None):
        """ Constructor.
        @param context OpenCL context where apply. Only for compatibility, 
        must be None.
        @param queue OpenCL command queue. Only for compatibility, 
        must be None.
        """
        self.context = context
        self.queue   = queue

    def execute(self, fs, waves, dt, t):
        """ Compute free surface for next time step.
        @param fs Free surface instance.
        @param waves Waves instance.
        @param dt Time step.
        @param t Actual time (without adding dt).
        """
        self.fs = fs
        nx      = self.fs['Nx']
        ny      = self.fs['Ny']
        nF      = nx*ny
        grad    = self.evaluateGradient()
        # In order to improve results in really long simulations free surface
        # will performed considering external waves and second order effects
        # in two different ways. First external waves at time t will be
        # substracted, then second order waves will be computed, and finally
        # external waves at t+dt will be added.
        for i in range(0,nx):
            for j in range(0,ny):
                pos = np.copy(self.fs['pos'][i,j])
                # Substract external waves at time t.
                for w in waves['data']:
                    A       = w[0]
                    T       = w[1]
                    phase   = w[2]
                    heading = np.pi*w[3]/180.0
                    wl      = 0.5 * grav / np.pi * T*T
                    k       = 2.0*np.pi/wl
                    frec    = 2.0*np.pi/T
                    l       = pos[0]*np.cos(heading) + pos[1]*np.sin(heading)
                    amp     = A*np.sin(k*l - frec*t + phase)
                    pos[2]  = pos[2] - amp
                    amp     = - grav/frec*A*np.sin(k*l - frec*t + phase)
                    self.fs['velPot'][i,j] = self.fs['velPot'][i,j] - amp
                    amp     = grav*A*np.cos(k*l - frec*t + phase)
                    self.fs['accPot'][i,j] = self.fs['accPot'][i,j] - amp
                # Now compute second order waves using position copy, 
                # where external waves are excluded, in order impose
                # free surface boundary condition relative to second
                # order phenomena.
                self.fs['velPot'][i,j] = self.fs['velPot'][i,j]    + \
                                         dt*self.fs['accPot'][i,j]
                # self.fs['accPot'][i,j] = self.fs['accPot'][i,j]    + \
                #                          grav*pos[2]
                # Restore external waves to velocity and acceleration
                # potentials.
                for w in waves['data']:
                    A       = w[0]
                    T       = w[1]
                    phase   = w[2]
                    heading = np.pi*w[3]/180.0
                    wl      = 0.5 * grav / np.pi * T*T
                    k       = 2.0*np.pi/wl
                    frec    = 2.0*np.pi/T
                    l       = pos[0]*np.cos(heading) + pos[1]*np.sin(heading)
                    amp     = - grav/frec*A*np.sin(k*l - frec*(t+dt) + phase)
                    self.fs['velPot'][i,j] = self.fs['velPot'][i,j] + amp
                    amp     = grav*A*np.cos(k*l - frec*(t+dt) + phase)
                    self.fs['accPot'][i,j] = self.fs['accPot'][i,j] + amp
                # Update free surface point position
                gradVal = np.dot(np.abs(grad[i*ny+j]),grad[i*ny+j])
                gradVal = np.copysign(np.sqrt(np.abs(gradVal)), gradVal)
                self.fs['pos'][i,j][2] = self.fs['pos'][i,j][2] + dt*gradVal
        # Impose values at beach (far free surface)
        for i in range(0,nx):
            for j in [0,ny-1]:
                self.beach(i,j, waves, dt, t)
        for j in range(0,ny):
            for i in [0,nx-1]:
                self.beach(i,j, waves, dt, t)

    def evaluateGradient(self):
        """ Evaluate potential gradients over free surface.
        @return Potential gradients.
        """
        nx   = self.fs['Nx']
        ny   = self.fs['Ny']
        nF   = nx*ny
        grad = np.ndarray((nF,3), dtype=np.float32)
        for i in range(0,nx):
            for j in range(0,ny):
                pos = self.fs['pos'][i,j]
                grad[i*ny+j] = self.gradientphi(pos)
                gradVal = np.dot(np.abs(grad[i*ny+j]),grad[i*ny+j])
                gradVal = np.copysign(np.sqrt(np.abs(gradVal)), gradVal)
        return grad

    def gradientphi(self, pos):
        """ Compute gradient over desired position.
        @param pos Point to evaluate.
        @return Potential gradient.
        """
        nx   = self.fs['Nx']
        ny   = self.fs['Ny']
        grad = np.zeros(3, dtype=np.float32)
        for i in range(0,nx):
            for j in range(0,ny):
                # Get source position (desingularized)
                srcPos    = np.copy(self.fs['pos'][i,j])
                area      = self.fs['area'][i,j]
                srcPos[2] = srcPos[2] + np.sqrt(area)
                src       = self.fs['velSrc'][i,j]
                # Get distance between points
                d         = pos-srcPos
                grad      = grad + d/np.dot(d,d)*src*area
        # Discard Z induced effect by desingularization
        grad[2] = 0.
        return grad

    def beach(self, i,j, waves, dt, t):
        """ Compute far free surface where only 
        incident waves can be taken into account.
        @param i First free surface cell index.
        @param j Second free surface cell index.
        @param waves Waves instance.
        @param dt Time step.
        @param t Actual time (without adding dt).
        """
        pos = self.fs['pos'][i,j]
        pos[2] = 0.
        self.fs['velPot'][i,j] = 0.
        self.fs['accPot'][i,j] = 0.
        for w in waves['data']:
            A       = w[0]
            T       = w[1]
            phase   = w[2]
            heading = np.pi*w[3]/180.0
            wl      = 0.5 * grav / np.pi * T*T
            k       = 2.0*np.pi/wl
            frec    = 2.0*np.pi/T
            l       = pos[0]*np.cos(heading) + pos[1]*np.sin(heading)
            amp     = A*np.sin(k*l - frec*(t+dt) + phase)
            self.fs['pos'][i,j][2] = self.fs['pos'][i,j][2] + amp
            amp     = - grav/frec*A*np.sin(k*l - frec*(t+dt) + phase)
            self.fs['velPot'][i,j] = self.fs['velPot'][i,j] + amp
            amp     = grav*A*np.cos(k*l - frec*(t+dt) + phase)
            self.fs['accPot'][i,j] = self.fs['accPot'][i,j] + amp
 