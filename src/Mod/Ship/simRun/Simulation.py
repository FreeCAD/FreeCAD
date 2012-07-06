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

import time
from math import *
import threading

# FreeCAD
import FreeCAD,FreeCADGui
from FreeCAD import Part, Base, Vector

# Ship design module
from shipUtils import Paths, Translator, Math

class FreeCADShipSimulation(threading.Thread):
    def __init__ (self, endTime, output, FSmesh, waves):
        """ Thread constructor.
        @param endTime Maximum simulation time.
        @param output [Rate,Type] Output rate, Type=0 if FPS, 1 if IPF.
        @param FSmesh Free surface mesh faces.
        @param waves Waves parameters (A,T,phi,heading)
        """
        threading.Thread.__init__(self)
        self.endTime = endTime
        self.output  = output
        self.FSmesh  = FSmesh
        self.waves   = waves
        
    def run(self):
        """ Runs the simulation.
        """
        # Perform work here
        print("Im thread, Im running...")
        time.sleep(2)
        # ...
        print("Im thread, I end!")
