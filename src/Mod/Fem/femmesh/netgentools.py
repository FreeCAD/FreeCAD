# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2024 Mario Passaglia <mpassaglia[at]cbc.uba.ar>         *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

__title__ = "Tools for the work with Netgen mesher"
__author__ = "Mario Passaglia"
__url__ = "https://www.freecad.org"

import numpy as np
import shutil
import sys
import tempfile
from PySide.QtCore import QProcess

import FreeCAD
import Fem

try:
    from netgen import occ, meshing, config as ng_config
    import pyngcore as ngcore
except ModuleNotFoundError:
    FreeCAD.Console.PrintError("To use FemMesh Netgen objects, install the Netgen Python bindings")


class NetgenTools:

    # to change order of nodes from netgen to smesh
    order_edge = {
        2: [0, 1, 2],  # seg2
        3: [0, 1, 2],  # seg3
    }
    order_face = {
        3: [0, 1, 2, 3, 4, 5, 6, 7],  # tria3
        6: [0, 1, 2, 5, 3, 4, 6, 7],  # tria6
        4: [0, 1, 2, 3, 4, 5, 6, 7],  # quad4
        8: [0, 1, 2, 3, 4, 7, 5, 6],  # quad8
    }
    order_volume = {
        4: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19],  # tetra4
        10: [0, 1, 2, 3, 4, 7, 5, 6, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19],  # tetra10
        8: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19],  # hexa8
        20: [0, 1, 2, 3, 4, 5, 6, 7, 8, 11, 9, 10, 12, 15, 13, 14, 16, 17, 18, 19],  # hexa20
        5: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19],  # pyra5
        13: [0, 1, 2, 3, 4, 5, 8, 6, 7, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19],  # pyra13
        6: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19],  # penta6
        15: [0, 1, 2, 3, 4, 5, 6, 8, 7, 12, 14, 13, 9, 10, 11, 15, 16, 17, 18, 19],  # penta15
    }

    meshing_step = {
        "AnalyzeGeometry": 1,  # MESHCONST_ANALYSE
        "MeshEdges": 2,  # MESHCONST_MESHEDGES
        "MeshSurface": 3,  # MESHCONST_MESHSURFACE
        "OptimizeSurface": 4,  # MESHCONST_OPTSURFACE
        "MeshVolume": 5,  # MESHCONST_MESHVOLUME
        "OptimizeVolume": 6,  # MESHCONST_OPTVOLUME
    }

    name = "Netgen"

    def __init__(self, obj):
        self.obj = obj
        self.fem_mesh = None
        self.process = None
        self.tmpdir = ""
        self.process = QProcess()
        self.mesh_params = {}

    def write_geom(self):
        if not self.tmpdir:
            self.tmpdir = tempfile.mkdtemp(prefix="fem_")

        global_pla = self.obj.Shape.getGlobalPlacement()
        geom = self.obj.Shape.getPropertyOfGeometry()
        # get partner shape
        geom_trans = geom.transformed(FreeCAD.Placement().Matrix)
        geom_trans.Placement = global_pla
        self.brep_file = self.tmpdir + "/shape.brep"
        self.result_file = self.tmpdir + "/result.npy"
        geom_trans.exportBrep(self.brep_file)

    code = """
from femmesh.netgentools import NetgenTools

NetgenTools.run_netgen(**{params})
"""

    def prepare(self):
        self.write_geom()
        self.mesh_params = {
            "brep_file": self.brep_file,
            "threads": self.obj.Threads,
            "heal": self.obj.HealShape,
            "params": self.get_meshing_parameters(),
            "second_order": self.obj.SecondOrder,
            "result_file": self.result_file,
        }

    def compute(self):
        code_str = self.code.format(params=self.mesh_params)
        self.process.start(sys.executable, ["-c", code_str])

        return self.process

    @staticmethod
    def run_netgen(brep_file, threads, heal, params, second_order, result_file):

        geom = occ.OCCGeometry(brep_file)
        ngcore.SetNumThreads(threads)

        with ngcore.TaskManager():
            if heal:
                geom.Heal()
            mesh = geom.GenerateMesh(mp=meshing.MeshingParameters(**params))

        result = {
            "coords": [],
            "Edges": [[], []],
            "Faces": [[], []],
            "Volumes": [[], []],
        }
        groups = {"Edges": [], "Faces": [], "Solids": []}

        # save empty data if last step is geometry analysis
        if params["perfstepsend"] == NetgenTools.meshing_step["AnalyzeGeometry"]:
            np.save(result_file, [result, groups])
            return None

        if second_order:
            mesh.SecondOrder()

        coords = mesh.Coordinates()

        edges = mesh.Elements1D().NumPy()
        faces = mesh.Elements2D().NumPy()
        volumes = mesh.Elements3D().NumPy()

        nod_edges = edges["nodes"]
        nod_faces = faces["nodes"]
        nod_volumes = volumes["nodes"]

        np_edges = (nod_edges != 0).sum(axis=1).tolist()
        np_faces = faces["np"].tolist()
        np_volumes = volumes["np"].tolist()

        # set smesh node order
        for i in range(faces.size):
            nod_faces[i] = nod_faces[i][NetgenTools.order_face[np_faces[i]]]

        for i in range(volumes.size):
            nod_volumes[i] = nod_volumes[i][NetgenTools.order_volume[np_volumes[i]]]

        flat_edges = nod_edges[nod_edges != 0].tolist()
        flat_faces = nod_faces[nod_faces != 0].tolist()
        flat_volumes = nod_volumes[nod_volumes != 0].tolist()

        result = {
            "coords": coords,
            "Edges": [flat_edges, np_edges],
            "Faces": [flat_faces, np_faces],
            "Volumes": [flat_volumes, np_volumes],
        }

        # create groups
        nb_edges = edges.size
        nb_faces = faces.size
        nb_volumes = volumes.size

        idx_edges = edges["index"]
        idx_faces = faces["index"]
        idx_volumes = volumes["index"]

        for i in np.unique(idx_edges):
            edge_i = (np.nonzero(idx_edges == i)[0] + 1).tolist()
            groups["Edges"].append([i, edge_i])
        for i in np.unique(idx_faces):
            face_i = (np.nonzero(idx_faces == i)[0] + nb_edges + 1).tolist()
            groups["Faces"].append([i, face_i])

        for i in np.unique(idx_volumes):
            volume_i = (np.nonzero(idx_volumes == i)[0] + nb_edges + nb_faces + 1).tolist()
            groups["Solids"].append([i, volume_i])

        np.save(result_file, [result, groups])

    def fem_mesh_from_result(self):
        fem_mesh = Fem.FemMesh()

        # load Netgen result
        netgen_result, groups = np.load(self.result_file, allow_pickle=True)

        for node in netgen_result["coords"]:
            fem_mesh.addNode(*node)

        fem_mesh.addEdgeList(*netgen_result["Edges"])
        fem_mesh.addFaceList(*netgen_result["Faces"])
        fem_mesh.addVolumeList(*netgen_result["Volumes"])

        for g in groups["Edges"]:
            grp_id = fem_mesh.addGroup("Edge" + str(g[0]), "Edge")
            fem_mesh.addGroupElements(grp_id, g[1])

        for g in groups["Faces"]:
            grp_id = fem_mesh.addGroup("Face" + str(g[0]), "Face")
            fem_mesh.addGroupElements(grp_id, g[1])

        for g in groups["Solids"]:
            grp_id = fem_mesh.addGroup("Solid" + str(g[0]), "Volume")
            fem_mesh.addGroupElements(grp_id, g[1])

        return fem_mesh

    def update_properties(self):
        self.obj.FemMesh = self.fem_mesh_from_result()

    def get_meshing_parameters(self):
        params = {
            "optimize3d": self.obj.Optimize3d,
            "optimize2d": self.obj.Optimize2d,
            "optsteps3d": self.obj.OptimizationSteps3d,
            "optsteps2d": self.obj.OptimizationSteps2d,
            "opterrpow": self.obj.OptimizationErrorPower,
            "blockfill": self.obj.BlockFill,
            "filldist": self.obj.FillDistance.Value,
            "safety": self.obj.Safety,
            "relinnersafety": self.obj.RelinnerSafety,
            "uselocalh": self.obj.UseLocalH,
            "grading": self.obj.GrowthRate,
            "delaunay": self.obj.Delaunay,
            "delaunay2d": self.obj.Delaunay2d,
            "maxh": self.obj.MaxSize.Value,
            "minh": self.obj.MinSize.Value,
            "startinsurface": self.obj.StartInSurface,
            "checkoverlap": self.obj.CheckOverlap,
            "checkoverlappingboundary": self.obj.CheckOverlappingBoundary,
            "checkchartboundary": self.obj.CheckChartBoundary,
            "curvaturesafety": self.obj.CurvatureSafety,
            "segmentsperedge": self.obj.SegmentsPerEdge,
            "elsizeweight": self.obj.ElementSizeWeight,
            "parthread": self.obj.ParallelMeshing,
            "perfstepsstart": NetgenTools.meshing_step[self.obj.StartStep],
            "perfstepsend": NetgenTools.meshing_step[self.obj.EndStep],
            "giveuptol2d": self.obj.GiveUpTolerance2d,
            "giveuptol": self.obj.GiveUpTolerance,
            "giveuptolopenquads": self.obj.GiveUpToleranceOpenQuads,
            "maxoutersteps": self.obj.MaxOuterSteps,
            "starshapeclass": self.obj.StarShapeClass,
            "baseelnp": self.obj.BaseElementNp,
            "sloppy": self.obj.Sloppy,
            "badellimit": self.obj.BadElementLimit,
            "check_impossible": self.obj.CheckImpossible,
            "only3D_domain_nr": self.obj.Only3dDomainNr,
            "secondorder": self.obj.SecondOrder,
            "elementorder": self.obj.ElementOrder,
            "quad_dominated": self.obj.QuadDominated,
            "try_hexes": self.obj.TryHexes,
            "inverttets": self.obj.InvertTets,
            "inverttrigs": self.obj.InvertTrigs,
            "autozrefine": self.obj.AutoZRefine,
            "parallel_meshing": self.obj.ParallelMeshing,
            "nthreads": self.obj.Threads,
            "closeedgefac": self.obj.CloseEdgeFactor,
        }

        # set specific parameters by fineness
        if self.obj.Fineness == "VeryCoarse":
            params["curvaturesafety"] = 1
            params["segmentsperedge"] = 0.3
            params["grading"] = 0.7
            params["closeedgefac"] = 0.5
            params["optsteps3d"] = 5

        elif self.obj.Fineness == "Coarse":
            params["curvaturesafety"] = 1.5
            params["segmentsperedge"] = 0.5
            params["grading"] = 0.5
            params["closeedgefac"] = 1
            params["optsteps3d"] = 5

        elif self.obj.Fineness == "Moderate":
            params["curvaturesafety"] = 2
            params["segmentsperedge"] = 1
            params["grading"] = 0.3
            params["closeedgefac"] = 2
            params["optsteps3d"] = 5

        elif self.obj.Fineness == "Fine":
            params["curvaturesafety"] = 3
            params["segmentsperedge"] = 2
            params["grading"] = 0.2
            params["closeedgefac"] = 3.5
            params["optsteps3d"] = 5

        elif self.obj.Fineness == "VeryFine":
            params["curvaturesafety"] = 5
            params["segmentsperedge"] = 3
            params["grading"] = 0.1
            params["closeedgefac"] = 5
            params["optsteps3d"] = 5

        return params

    @staticmethod
    def version():
        result = "{}: {}\n" + "{}: {}\n" + "{}: {}\n" + "{}: {}"
        return result.format(
            "Netgen",
            ng_config.version,
            "Python",
            ng_config.PYTHON_VERSION,
            "OpenCASCADE",
            occ.occ_version,
            "Use MPI",
            ng_config.USE_MPI,
        )

    def __del__(self):
        if self.tmpdir:
            shutil.rmtree(self.tmpdir)
