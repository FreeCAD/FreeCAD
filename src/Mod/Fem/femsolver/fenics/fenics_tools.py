# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 - Johannes Hartung <j.hartung@gmx.net>             *
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
from __future__ import print_function

try:
    import fenics
except:
    print("No Fenics modules found, please install them.")
else:

    class CellExpressionXDMF(fenics.Expression):
        def __init__(self, xdmffilename, group_value_dict, group_priority_dict={}, default=0., check_marked=(lambda x: x == 1), **kwargs):
            self.group_priority_dict = group_priority_dict
            self.check_marked = check_marked
            self.default = default

            self.readXDMFfile(xdmffilename, group_value_dict)

        def readXDMFfile(self, xdmffilename, group_value_dict):
            """
            Initialization of CellExpressionXDMF by reading an XDMF file.

            @param: xdmffilename: path to xdmf file
            @param: group_value_dict: {"groupname":function(x)}

            function(x) is a function which is evaluated at the marked positions of the cells
            """
            xdmffile = fenics.XDMFFile(xdmffilename)
            self.group_value_dict = group_value_dict
            self.mesh = fenics.Mesh()
            xdmffile.read(self.mesh)
            self.markers = {}
            self.dx = {}
            for (key, value) in self.group_value_dict.iteritems():
                # Fenics interface here: create cell function of type int for every group
                # TODO: examine whether int is appropriate or this class could be generalized
                self.markers[key] = fenics.CellFunction("int", self.mesh)
                xdmffile.read(self.markers[key], key)
                self.dx[key] = fenics.Measure("dx", domain=self.mesh, subdomain_data=self.markers[key])
            xdmffile.close()

        def eval_cell(self, values, x, cell):
            values_list = []
            for (key, func) in self.group_value_dict.iteritems():
                if self.check_marked(self.markers[key][cell.index]):
                    values_list.append(func(x))
            if values_list:
                values[0] = values_list[0]
                # TODO: improve for vectorial data
                # TODO: fix value assignment for overlap
                # according to priority, mean, or standard
                # TODO: python classes much slower than JIT compilation
            else:
                values[0] = self.default

        # def value_shape(self):
        #     return self.shape

    class FacetFunctionXDMF(object):
        def __init__(self, xdmffilename, group_value_dict):
            """
            Initialization of FacetFunctionXDMF by reading an XDMF file.

            @param: xdmffilename: path to xdmf file
            @param: group_value_dict: {"groupname":{"type":"Dirichlet|Neumann|Robin", "value":u_D|du_N|(r,s), "marked":1}, ...}

            If group_value_dict contains no "marked" entry for the groupname, "marked" is set to 1 by default.
            """
            self.readXDMFFile(xdmffilename, group_value_dict)

        def readXDMFFile(self, xdmffilename, group_value_dict):
            xdmffile = fenics.XDMFFile(xdmffilename)
            self.group_value_dict = group_value_dict
            self.mesh = fenics.Mesh()
            xdmffile.read(self.mesh)
            self.markers = {}
            self.marked = {}
            self.ds = {}
            self.bcs = {}
            for (key, value) in self.group_value_dict.iteritems():
                # Fenics interface here: create facet function of type size_t (positive int) for every group
                # TODO: examine whether size_t is appropriate or this class could be generalized
                self.markers[key] = fenics.FacetFunction("size_t", self.mesh)
                xdmffile.read(self.markers[key], key)
                self.marked[key] = value.get("marked", 1)
                self.ds[key] = fenics.Measure("ds", domain=self.mesh, subdomain_data=self.markers[key])
                self.bcs[key] = value
            xdmffile.close()

        def getDirichletBCs(self, vectorspace, *args, **kwargs):
            dbcs = []
            for (dict_key, dict_value) in self.bcs.iteritems():
                if dict_value["type"] == 'Dirichlet':
                    bc = fenics.DirichletBC(vectorspace, dict_value["value"], self.markers[dict_key], dict_value.get("marked", 1), *args, **kwargs)
                    dbcs.append(bc)
            return dbcs
        # TODO: write some functions to return integrals for Neumann and Robin
        # boundary conditions for the general case (i.e. vector, tensor)
