# ***************************************************************************
# *   Copyright (c) 2016 Bernd Hahnebach <bernd@bimstatik.org>              *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

__title__ = "FreeCAD FEM mesh gmsh document object"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"

## @package mesh_gmsh
#  \ingroup FEM
#  \brief mesh gmsh object

from FreeCAD import Base
from . import base_fempythonobject

_PropHelper = base_fempythonobject._PropHelper


class MeshGmsh(base_fempythonobject.BaseFemPythonObject):
    """
    A Fem::FemMeshObject python type, add Gmsh specific properties
    """

    Type = "Fem::FemMeshGmsh"

    def __init__(self, obj):
        super().__init__(obj)

        for prop in self._get_properties():
            prop.add_to_object(obj)

    def _get_properties(self):
        prop = []

        prop.append(
            _PropHelper(
                type="App::PropertyLinkList",
                name="MeshBoundaryLayerList",
                group="Base",
                doc="Mesh boundaries need inflation layers",
                value=[],
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyLinkList",
                name="MeshRegionList",
                group="Base",
                doc="Mesh refinments of the mesh",
                value=[],
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyLinkList",
                name="MeshGroupList",
                group="Base",
                doc="Mesh groups of the mesh",
                value=[],
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyLength",
                name="CharacteristicLengthMax",
                group="Mesh Parameters",
                doc="Max mesh element size (0.0 means infinity)",
                value=0.0,  # will be 1e+22
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyLength",
                name="CharacteristicLengthMin",
                group="Mesh Parameters",
                doc="Min mesh element size",
                value=0.0,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyEnumeration",
                name="ElementDimension",
                group="Mesh Parameters",
                doc="Dimension of mesh elements ('From Shape': according ShapeType of part to mesh)",
                value=["From Shape", "1D", "2D", "3D"],
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyEnumeration",
                name="ElementOrder",
                group="Mesh Parameters",
                doc="Order of mesh elements",
                value=["1st", "2nd"],
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyBool",
                name="OptimizeStd",
                group="Mesh Parameters",
                doc="Optimize tetrahedral elements",
                value=True,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyBool",
                name="OptimizeNetgen",
                group="Mesh Parameters",
                doc="Optimize tetra elements by use of Netgen",
                value=False,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyEnumeration",
                name="HighOrderOptimize",
                group="Mesh Parameters",
                doc="Optimization of high order meshes",
                value=[
                    "None",
                    "Optimization",
                    "Elastic+Optimization",
                    "Elastic",
                    "Fast curving",
                ],
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyBool",
                name="RecombineAll",
                group="Mesh Parameters",
                doc="Apply recombination algorithm to all surfaces",
                value=False,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyBool",
                name="Recombine3DAll",
                group="Mesh Parameters",
                doc="Apply recombination algorithm to all volumes",
                value=False,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyEnumeration",
                name="RecombinationAlgorithm",
                group="Mesh Parameters",
                doc="Recombination algorithm",
                value=[
                    "Simple",
                    "Blossom",
                    "Simple full-quad",
                    "Blossom full-quad",
                ],
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyBool",
                name="CoherenceMesh",
                group="Mesh Parameters",
                doc="Removes all duplicate mesh vertices",
                value=True,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyFloat",
                name="GeometryTolerance",
                group="Mesh Parameters",
                doc="Geometrical Tolerance (0.0 means GMSH std = 1e-08)",
                value=1e-06,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyBool",
                name="SecondOrderLinear",
                group="Mesh Parameters",
                doc="Second order nodes are created by linear interpolation",
                value=False,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyIntegerConstraint",
                name="MeshSizeFromCurvature",
                group="Mesh Parameters",
                doc="Number of elements per 2*pi radians, 0 to deactivate",
                value=(12, 0, 10000, 1),
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyEnumeration",
                name="Algorithm2D",
                group="Mesh Parameters",
                doc="Mesh algorithm 2D",
                value=[
                    "Automatic",
                    "MeshAdapt",
                    "Delaunay",
                    "Frontal",
                    "BAMG",
                    "DelQuad",
                    "Packing Parallelograms",
                    "Quasi-structured Quad",
                ],
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyEnumeration",
                name="Algorithm3D",
                group="Mesh Parameters",
                doc="Mesh algorithm 3D",
                value=[
                    "Automatic",
                    "Delaunay",
                    "New Delaunay",
                    "Frontal",
                    "MMG3D",
                    "R-tree",
                    "HXT",
                ],
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyBool",
                name="GroupsOfNodes",
                group="Mesh Parameters",
                doc="For each group create not only the elements but the nodes too",
                value=False,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyEnumeration",
                name="SubdivisionAlgorithm",
                group="Mesh Parameters",
                doc="Mesh subdivision algorithm",
                value=["None", "All Quadrangles", "All Hexahedra", "Barycentric"],
            )
        )

        return prop

    def onDocumentRestored(self, obj):
        # update old project with new properties
        for prop in self._get_properties():
            try:
                obj.getPropertyByName(prop.name)
            except Base.PropertyError:
                prop.add_to_object(obj)

            if prop.name == "Algorithm2D":
                # refresh the list of known 2D algorithms for old projects
                obj.Algorithm2D = prop.value
            elif prop.name == "Algorithm3D":
                # refresh the list of known 3D algorithms for old projects
                obj.Algorithm3D = prop.value
            elif prop.name == "HighOrderOptimize":
                # HighOrderOptimize was once App::PropertyBool, so check this
                prop.handle_change_type(
                    obj, "App::PropertyBool", lambda x: "Optimization" if x else "None"
                )
            # Migrate group of properties for old projects
            if obj.getGroupOfProperty(prop.name) == "FEM Gmsh Mesh Params":
                obj.setGroupOfProperty(prop.name, "Mesh Parameters")

        # migrate old Part property to Shape property
        try:
            value_part = obj.getPropertyByName("Part")
            obj.setPropertyStatus("Part", "-LockDynamic")
            obj.removeProperty("Part")
            # old object is Fem::FemMeshObjectPython (does not have Shape property with global scope)
            prop = _PropHelper(
                type="App::PropertyLinkGlobal",
                name="Shape",
                group="FEM Mesh",
                doc="Geometry object, the mesh is made from. The geometry object has to have a Shape.",
                value=value_part,
            )
            prop.add_to_object(obj)
        except Base.PropertyError:
            pass
