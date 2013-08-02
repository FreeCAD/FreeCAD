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

# FreeCAD
from shipUtils import Paths

# pyOpenCL
import pyopencl as cl
import numpy as np

# Standard
import math

def loadProgram(context, file):
    """ Loads a file and comnpile it.
    @param context OpenCL context where apply.
    @param file File to load and compile.
    @return Ready to use OpenCL program.
    """
    f = open(file, 'r')
    str = "".join(f.readlines())
    return cl.Program(context, str).build()

def path():
    """ Gets the OpenCL kernels path
    @return OpenCL kernels path
    """
    path = Paths.modulePath() + "/resources/opencl"
    return path

def globalSize(n):
    """ Compute global size from amount of data.
    @param n Amount of data.
    @return global size.
    """
    localSize = 256.0
    return int(math.ceil(n/localSize)*localSize)

