#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2015 - Przemo Firszt <przemo@firszt.eu>                 *
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


import FreeCAD
import FemGui


class FemTools:
    def __init__(self, analysis_object=None):
        if analysis_object:
            self.fem_analysis = analysis_object
        else:
            self.fem_analysis = FemGui.getActiveAnalysis()
        self.mesh = None
        for m in self.fem_analysis.Member:
            if m.isDerivedFrom("Fem::FemMeshObject"):
                self.mesh = m

    def purge_results(self):
        for m in self.fem_analysis.Member:
            if (m.isDerivedFrom('Fem::FemResultObject')):
                FreeCAD.ActiveDocument.removeObject(m.Name)

    def reset_mesh_deformation(self):
        if self.mesh:
            self.mesh.ViewObject.applyDisplacement(0.0)

    def reset_mesh_color(self):
        if self.mesh:
            self.mesh.ViewObject.NodeColor = {}
            self.mesh.ViewObject.ElementColor = {}
            self.mesh.ViewObject.setNodeColorByScalars()
