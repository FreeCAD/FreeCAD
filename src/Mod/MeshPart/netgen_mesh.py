# -*- coding: utf-8 -*-

__title__ = "netgenMesh"
__author__ = "Werner Mayer"
__license__ = "LGPL 2.1"
__doc__ = "Use netgen mesher to create a mesh from a given shape"


import Mesh
import os
import tempfile
import uuid


def shapeToMesh(occShape, options={}):

    basename = str(uuid.uuid4())
    shapeFile = tempfile.gettempdir() + os.sep + basename + ".brep"
    occShape.exportBrep(shapeFile)

    try:
        meshFile = _fromPython(shapeFile, basename, options)
    except ImportError:
        meshFile = _fromProcess(shapeFile, basename, options)

    mesh = Mesh.Mesh()
    mesh.read(meshFile)

    os.remove(shapeFile)
    os.remove(meshFile)

    return mesh


def _fromPython(shapeFile, basename, options):
    from netgen import occ

    args = {}

    # Fineness = (GrowthRate, SegPerEdge, SegPerRadius)
    fineness = {
        "VeryCoarse": (0.7, 0.3, 1.0),
        "Coarse": (0.5, 0.5, 1.5),
        "Moderate": (0.3, 1.0, 2.0),
        "Fine": (0.2, 2.0, 3.0),
        "VeryFine": (0.1, 3.0, 5.0),
    }

    # See NETGENPlugin_Mesher.cpp
    # and netgen's meshtype.hpp
    optsMap = {
        "GrowthRate": "grading",
        "SegPerEdge": "segmentsperedge",
        "SegPerRadius": "curvaturesafety",
        "AllowQuad": "quad_dominated",
        "SecondOrder": "secondorder",
        "MaxSize": "maxh",
        "MinSize": "minh",
        "SurfaceCurvature": "uselocalh",
    }

    # Add values for fineness mode
    if "Fineness" in options:
        mode = options["Fineness"]
        if mode in fineness:
            vals = fineness[mode]
            args["grading"] = vals[0]
            args["segmentsperedge"] = vals[1]
            args["curvaturesafety"] = vals[2]

    # Translate FreeCAD options to netgen options
    for opt in optsMap:
        if opt in options:
            mapped = optsMap[opt]
            args[mapped] = options[opt]

    # Special handling of 'Optimize'
    opt = "Optimize"
    if opt in options:
        args["perfstepsstart"] = 0
        args["perfstepsend"] = 3
        if options[opt]:
            args["perfstepsend"] = 4

    geo = occ.OCCGeometry(shapeFile)
    mesh = geo.GenerateMesh(**args)

    meshFile = tempfile.gettempdir() + os.sep + basename + ".stl"
    mesh.Export(meshFile, "STL Format")

    return meshFile


def _fromProcess(shapeFile, basename, options):
    import subprocess

    fineness = {
        "VeryCoarse": "-coarse",  # netgen doesn't offer the option -verycoarse
        "Coarse": "-coarse",
        "Moderate": "-moderate",
        "Fine": "-fine",
        "VeryFine": "-veryfine",
    }

    meshsize = "-moderate"
    if "Fineness" in options:
        mode = options["Fineness"]
        if mode in fineness:
            meshsize = fineness[mode]

    meshFile = tempfile.gettempdir() + os.sep + basename + ".stl"

    command_list = [
        "netgen",
        "-geofile={}".format(shapeFile),
        meshsize,
        "-meshfiletype=STL Format",
        "-batchmode",
        "-meshfile={}".format(meshFile),
    ]

    subprocess.run(command_list)

    if not os.path.exists(meshFile):
        raise RuntimeError("Failed to create mesh")

    return meshFile
