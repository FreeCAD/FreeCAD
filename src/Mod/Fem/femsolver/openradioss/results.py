# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2026 Nicholas Herrmann <nicholas.herrmann@proton.me>    *
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

"""Load an OpenRadioss result frame (.vtu written by runner.py) as a FEM
mechanical result object, like the CalculiX solver does.

The result goes through Fem::ResultMechanical: node displacements by NODE_ID,
element stress tensors averaged to the nodes, von Mises / principal stresses
and statistics computed by FreeCAD's resulttools, and a post-processing
pipeline built from it.

The FEM solver framework runs the Results task in a worker thread while the
main thread waits for it. Recomputing a pipeline updates its 3D view, which
must happen in the main thread: done from the worker it deadlocks the GUI.
So in the GUI the pipeline is built in the main thread, as soon as the solver
run returns to the event loop (see ``in_main_thread``).
"""

__title__ = "FreeCAD FEM solver OpenRadioss results"
__author__ = "Nicholas Herrmann"
__url__ = "https://www.freecad.org"

## \addtogroup FEM
#  @{

import threading
import xml.etree.ElementTree as ET

import FreeCAD

if FreeCAD.GuiUp:
    from PySide import QtCore

    class _MainThreadCaller(QtCore.QObject):
        """Runs callables in the thread this object lives in (the main thread)."""

        call = QtCore.Signal(object)

        def __init__(self):
            super().__init__()
            self.call.connect(self._run, QtCore.Qt.QueuedConnection)

        def _run(self, function):
            try:
                function()
            except Exception as e:  # shown in the report view, nothing can catch it here
                FreeCAD.Console.PrintError(
                    f"OpenRadioss: loading the result pipeline failed: {e}\n"
                )

    _caller = _MainThreadCaller()
    # in case of a first import in a worker thread
    _caller.moveToThread(QtCore.QCoreApplication.instance().thread())


def in_main_thread(function):
    """Run ``function`` now if this is the main thread (or there is no GUI), else queue it there.

    Returns the function's result when it ran now, otherwise None.
    """
    if not FreeCAD.GuiUp or threading.current_thread() is threading.main_thread():
        return function()
    _caller.call.emit(function)
    return None


# vtk cell type -> number of nodes, element kind
_VTK_CELLS = {
    3: (2, "edge"),
    5: (3, "face"),
    9: (4, "face"),
    10: (4, "volume"),
    12: (8, "volume"),
    13: (6, "volume"),
    14: (5, "volume"),
    24: (10, "volume"),
}


def read_vtu(path):
    """Arrays of an ASCII .vtu as {name: (components, [floats])} plus points, cells and types."""
    root = ET.parse(path).getroot()
    arrays = {}
    for array in root.iter("DataArray"):
        values = (array.text or "").split()
        arrays[array.get("Name")] = (int(array.get("NumberOfComponents") or 1), values)
    connectivity = [int(v) for v in arrays["connectivity"][1]]
    offsets = [int(v) for v in arrays["offsets"][1]]
    cells, start = [], 0
    for end in offsets:
        cells.append(connectivity[start:end])
        start = end
    return {
        "arrays": arrays,
        "points": [float(v) for v in arrays["Points"][1]],
        "cells": cells,
        "types": [int(v) for v in arrays["types"][1]],
    }


def _tuples(array, size):
    values = [float(v) for v in array[1]]
    return [values[i : i + size] for i in range(0, len(values), size)]


def _tensor_to_voigt(t):
    """3x3 row-major tensor -> (xx, yy, zz, xy, xz, yz)."""
    return (t[0], t[4], t[8], t[1], t[2], t[5])


def _von_mises(s):
    xx, yy, zz, xy, xz, yz = s
    return (
        0.5 * ((xx - yy) ** 2 + (yy - zz) ** 2 + (zz - xx) ** 2) + 3 * (xy**2 + xz**2 + yz**2)
    ) ** 0.5


def element_stresses(data):
    """{element id: (xx, yy, zz, xy, xz, yz)} from solid or shell stress tensors."""
    arrays = data["arrays"]
    ids = [int(float(v)) for v in arrays["ELEMENT_ID"][1]] if "ELEMENT_ID" in arrays else []
    stresses = {}
    if "3DELEM_Stress" in arrays:
        for eid, t in zip(ids, _tuples(arrays["3DELEM_Stress"], 9)):
            stresses[eid] = _tensor_to_voigt(t)
    upper_name, lower_name = "2DELEM_Stress_(upper)", "2DELEM_Stress_(lower)"
    if upper_name in arrays:
        upper = [_tensor_to_voigt(t) for t in _tuples(arrays[upper_name], 9)]
        lower = upper
        if lower_name in arrays:
            lower = [_tensor_to_voigt(t) for t in _tuples(arrays[lower_name], 9)]
        for eid, u, low in zip(ids, upper, lower):
            if eid not in stresses:  # shells: the more stressed surface
                stresses[eid] = u if _von_mises(u) >= _von_mises(low) else low
    return stresses


def element_plastic_strains(data):
    """{element id: equivalent plastic strain} of solids and shells.

    Empty without a plastic material.
    """
    arrays = data["arrays"]
    ids = [int(float(v)) for v in arrays["ELEMENT_ID"][1]] if "ELEMENT_ID" in arrays else []
    strains = {}
    for name in ("3DELEM_Plastic_Strain", "2DELEM_Plastic_Strain"):
        if name in arrays:
            for eid, value in zip(ids, arrays[name][1]):
                value = float(value)
                if eid > 0 and value > strains.get(eid, 0.0):
                    strains[eid] = value
    return strains


def femmesh_from_vtu(data, node_ids, displacement):
    """A FemMesh in the undeformed configuration from the result's own geometry.

    Points with NODE_ID 0 are display geometry without a model node (rigid walls,
    for example); they and the cells using them are left out.
    """
    import Fem

    mesh = Fem.FemMesh()
    points = data["points"]
    for i, nid in enumerate(node_ids):
        if nid <= 0:
            continue
        d = displacement.get(nid, (0.0, 0.0, 0.0))
        mesh.addNode(points[3 * i] - d[0], points[3 * i + 1] - d[1], points[3 * i + 2] - d[2], nid)
    element_ids = [int(float(v)) for v in data["arrays"]["ELEMENT_ID"][1]]
    for eid, cell, vtk_type in zip(element_ids, data["cells"], data["types"]):
        n, kind = _VTK_CELLS.get(vtk_type, (0, None))
        nodes = [node_ids[i] for i in cell[:n]]
        if eid <= 0 or any(nid <= 0 for nid in nodes):
            continue
        if kind == "volume":
            mesh.addVolume(nodes, eid)
        elif kind == "face":
            mesh.addFace(nodes, eid)
        elif kind == "edge":
            mesh.addEdge(nodes, eid)
    return mesh


def load_result(
    doc,
    vtu_path,
    name="OpenRadiossResult",
    analysis=None,
    femmesh=None,
    time=None,
    pipeline_label=None,
):
    """Create a Fem::ResultMechanical (+ result mesh and pipeline) from ``vtu_path``.

    ``femmesh`` is the analysis mesh (node and element ids match the deck);
    without it the mesh is rebuilt from the result file. Returns (result, pipeline).
    Called from a worker thread in the GUI, the pipeline is created later in the
    main thread and None is returned for it.
    """
    import ObjectsFem
    from femresult import resulttools

    data = read_vtu(vtu_path)
    arrays = data["arrays"]
    node_ids = [int(float(v)) for v in arrays["NODE_ID"][1]]
    displacement = {}
    if "Displacement" in arrays:
        displacement = dict(zip(node_ids, _tuples(arrays["Displacement"], 3)))
    stresses = element_stresses(data)

    if femmesh is None:
        femmesh = femmesh_from_vtu(data, node_ids, displacement)
    elements = {
        eid: list(femmesh.getElementNodes(eid))
        for eid in (femmesh.Volumes if femmesh.VolumeCount else femmesh.Faces)
    }

    # nodes missing from the result (midside nodes of reduced second-order elements):
    # take the mean of the nodes of their element that have values
    for nodes in elements.values():
        known = [displacement[n] for n in nodes if n in displacement]
        for n in nodes:
            if n not in displacement and known:
                displacement[n] = [sum(d[k] for d in known) / len(known) for k in range(3)]

    node_stress, count = {}, {}
    for eid, nodes in elements.items():
        s = stresses.get(eid)
        if s is None:
            continue
        for n in nodes:
            acc = node_stress.setdefault(n, [0.0] * 6)
            for k in range(6):
                acc[k] += s[k]
            count[n] = count.get(n, 0) + 1
    node_numbers = sorted(femmesh.Nodes)

    plastic = element_plastic_strains(data)
    node_peeq = {}
    if any(plastic.values()):
        sums, hits = {}, {}
        for eid, nodes in elements.items():
            if eid in plastic:
                for n in nodes:
                    sums[n] = sums.get(n, 0.0) + plastic[eid]
                    hits[n] = hits.get(n, 0) + 1
        node_peeq = {n: sums[n] / hits[n] for n in sums}

    res = ObjectsFem.makeResultMechanical(doc, name)
    mesh_obj = ObjectsFem.makeMeshResult(doc, name + "_Mesh")
    mesh_obj.FemMesh = femmesh
    res.Mesh = mesh_obj
    res.NodeNumbers = node_numbers
    res.DisplacementVectors = [
        FreeCAD.Vector(*displacement.get(n, (0.0, 0.0, 0.0))) for n in node_numbers
    ]
    components = (
        "NodeStressXX",
        "NodeStressYY",
        "NodeStressZZ",
        "NodeStressXY",
        "NodeStressXZ",
        "NodeStressYZ",
    )
    for k, prop in enumerate(components):
        values = [node_stress[n][k] / count[n] if n in count else 0.0 for n in node_numbers]
        setattr(res, prop, values)
    if node_peeq:  # shown as "Equivalent Plastic Strain" in the pipeline
        res.Peeq = [node_peeq.get(n, 0.0) for n in node_numbers]
    if time is not None:
        res.Time = time
    if analysis is not None:
        analysis.addObject(res)
        analysis.addObject(mesh_obj)
    res = resulttools.add_disp_apps(res)
    res = resulttools.add_von_mises(res)
    res = resulttools.add_principal_stress_std(res)
    res = resulttools.fill_femresult_stats(res)

    def make_pipeline():
        pipeline = ObjectsFem.makePostVtkResult(doc, [res], name)
        if pipeline_label:
            pipeline.Label = pipeline_label
        if analysis is not None:
            analysis.addObject(pipeline)
        pipeline.recomputeChildren()
        pipeline.recompute()
        mesh_obj.Visibility = False
        if FreeCAD.GuiUp:
            pipeline.ViewObject.updateColorBars()
        doc.recompute()
        return pipeline

    return res, in_main_thread(make_pipeline)


##  @}
