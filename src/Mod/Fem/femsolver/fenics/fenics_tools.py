# ***************************************************************************
# *   Copyright (c) 2017 Johannes Hartung <j.hartung@gmx.net>               *
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

__title__ = "FreeCAD FEM solver Fenics tools"
__author__ = "Johannes Hartung"
__url__ = "https://www.freecad.org"

## @package Fenics
#  \ingroup FEM

import FreeCAD
import numpy as np

try:
    import fenics
except ImportError:
    FreeCAD.Console.PrintError("No Fenics modules found, please install them.\n")
    raise Exception("No Fenics modules found, please install them.\n")


class XDMFReader(object):
    """
    Reads XDMF file and provides unified interface for returning
    cell functions or facet functions.
    """
    def __init__(self, xdmffilename):
        """
        Sets filename and sets mesh instance to None.
        """
        self.xdmffilename = xdmffilename
        self.mesh = None

    def resetMesh(self):
        """
        Resets mesh instance to None.
        """
        self.mesh = None

    def readMesh(self):
        """
        If mesh instance is None, read mesh instance from file denoted
        by filename property.
        """
        # TODO: implement mesh read in for open file
        if self.mesh is None:
            xdmffile = fenics.XDMFFile(self.xdmffilename)
            self.mesh = fenics.Mesh()
            xdmffile.read(self.mesh)
            xdmffile.close()

    def readCellExpression(
        self,
        group_value_dict,
        value_type="scalar",
        overlap=lambda x: x[0],
        *args,
        **kwargs
    ):
        """
        Reads cell expression and returns it.
        """
        value_type_dictionary = {
            "scalar": ScalarCellExpressionFromXDMF,
            "vector2d": Vector2DCellExpressionFromXDMF,
            "vector3d": Vector3DCellExpressionFromXDMF}

        self.readMesh()
        xdmffile = fenics.XDMFFile(self.xdmffilename)
        cf = value_type_dictionary[value_type.lower()](
            group_value_dict,
            overlap=overlap,
            *args, **kwargs
        )
        cf.init()
        for (key, value) in cf.group_value_dict.items():
            cf.markers[key] = fenics.MeshFunction(
                "size_t",
                self.mesh,
                self.mesh.topology().dim()
            )
            xdmffile.read(cf.markers[key], key)
            cf.dx[key] = fenics.Measure(
                "dx",
                domain=self.mesh,
                subdomain_data=cf.markers[key]
            )
        xdmffile.close()
        return cf

    def readFacetFunction(self, group_value_dict, *args, **kwargs):
        """
        Reads facet function and returns it.
        """
        self.readMesh()
        xdmffile = fenics.XDMFFile(self.xdmffilename)
        ff = FacetFunctionFromXDMF(group_value_dict, *args, **kwargs)
        ff.init()
        for (key, value) in ff.group_value_dict.items():
            ff.markers[key] = fenics.MeshFunction(
                "size_t",
                self.mesh,
                self.mesh.topology().dim() - 1
            )
            xdmffile.read(ff.markers[key], key)
            ff.marked[key] = value.get("marked", 1)
            ff.ds[key] = fenics.Measure(
                "ds",
                domain=self.mesh,
                subdomain_data=ff.markers[key]
            )
            ff.bcs[key] = value
        xdmffile.close()
        return ff


class CellExpressionFromXDMF(object):
    """
    Creates cell function expression from XDMF file.
    """
    def __init__(
        self, group_value_dict,
        default=lambda x: 0.,
        check_marked=(lambda x: x == 1),
        overlap=lambda x: x[0],
        **kwargs
    ):
        self.init()
        self.group_value_dict = group_value_dict
        self.check_marked = check_marked
        self.overlap = overlap
        self.default = default

    def init(self):
        self.markers = {}
        self.dx = {}

    def assign_values(self, values, to_assign):
        values[:] = to_assign

    def eval_cell_backend(self, values, x, cell):

        values_list = [
            func(x) for (key, func) in self.group_value_dict.items()
            if self.check_marked(self.markers[key][cell.index])
        ]
        return_value = self.overlap(values_list)

        if values_list:
            self.assign_values(values, return_value)
        else:
            self.assign_values(values, self.default(x))

        # TODO: python classes much slower than JIT compilation


# ***********************************
# * Sub classes due to value_shape method which is not of dynamical return type
# * Also the assignment of values is to be done by reference. Therefore it has to be
# * overloaded.
# ***********************************
class ScalarCellExpressionFromXDMF(fenics.Expression, CellExpressionFromXDMF):

    def __init__(
        self,
        group_value_dict,
        default=lambda x: 0.,
        check_marked=(lambda x: x == 1),
        overlap=lambda x: x[0],
        **kwargs
    ):
        CellExpressionFromXDMF.__init__(
            self, group_value_dict,
            default=default,
            check_marked=check_marked,
            overlap=overlap
        )

    def eval_cell(self, values, x, cell):
        self.eval_cell_backend(values, x, cell)

    def value_shape(self):
        return ()


class Vector3DCellExpressionFromXDMF(fenics.Expression, CellExpressionFromXDMF):

    def __init__(
        self,
        group_value_dict,
        default=lambda x: np.zeros((3,)),
        check_marked=(lambda x: x == 1),
        overlap=lambda x: x[0], **kwargs
    ):
        CellExpressionFromXDMF.__init__(
            self, group_value_dict,
            default=default,
            check_marked=check_marked,
            overlap=overlap
        )

    def eval_cell(self, values, x, cell):
        self.eval_cell_backend(values, x, cell)

    def value_shape(self):
        return (3,)


class Vector2DCellExpressionFromXDMF(fenics.Expression, CellExpressionFromXDMF):

    def __init__(
        self,
        group_value_dict,
        default=lambda x: np.zeros((2,)),
        check_marked=(lambda x: x == 1),
        overlap=lambda x: x[0],
        **kwargs
    ):
        CellExpressionFromXDMF.__init__(
            self,
            group_value_dict,
            default=default,
            check_marked=check_marked,
            overlap=overlap
        )

    def eval_cell(self, values, x, cell):
        self.eval_cell_backend(values, x, cell)

    def value_shape(self):
        return (2,)


class FacetFunctionFromXDMF(object):
    """
    Creates facet function from XDMF file.
    """
    def __init__(self, group_value_dict, *args, **kwargs):
        self.group_value_dict = group_value_dict
        self.init()

    def init(self):
        self.markers = {}
        self.marked = {}
        self.ds = {}
        self.bcs = {}

    def getDirichletBCs(self, vectorspace, *args, **kwargs):
        dbcs = []
        for (dict_key, dict_value) in self.bcs.items():
            if dict_value["type"] == "Dirichlet":
                bc = fenics.DirichletBC(
                    vectorspace,
                    dict_value["value"],
                    self.markers[dict_key],
                    dict_value.get("marked", 1),
                    *args, **kwargs
                )
                dbcs.append(bc)
        return dbcs
    # TODO: write some functions to return integrals for Neumann and Robin
    # boundary conditions for the general case (i.e. vector, tensor)

##  @}
