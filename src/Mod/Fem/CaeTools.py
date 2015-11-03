#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2015 - FreeCAD Developers                               *
#*   Author (c) 2015 - Qingfeng Xia <qingfeng xia eng.ox.ac.uk>                    *
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

"""
This file provides utility functions, not belong to any python class
Since python object will not be saved, reference to python object's instance will fail!
"""


def getMesh(analysis_object):
    for i in analysis_object.Member:
        if i.isDerivedFrom("Fem::FemMeshObject"):
            return i
    # python will return None by default, so check None outside


def getSolver(analysis_object):
    for i in analysis_object.Member:
        if i.isDerivedFrom("Fem::FemSolverObject"):
            return i
        print("Error: No solver object is found from this analysis_object")


def getSolverPythonFromAnalysis(analysis_object):
    solver = getSolver(analysis_object)
    if solver is not None:
        try:
            pobj = solver.Proxy
            return pobj
        finally:
            import CaeSolver
            solverInfo = CaeSolver.REGISTERED_SOLVERS[solver.SolverName]
            obj = CaeSolver._createCaeSolver(solverInfo, analysis_object, solver)
            return obj.Proxy


def getConstraintGroup(analysis_object):
    group = []
    for i in analysis_object.Member:
        if i.isDerivedFrom("Fem::Constraint"):
            group.append(i)
    return group
