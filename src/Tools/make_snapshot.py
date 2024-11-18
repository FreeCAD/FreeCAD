# (c) 2022 Werner Mayer LGPL

"""
The module can be used from another module with:

from make_snapshot import make_snapshot
make_snapshot("file.stl", "image.png")

This other module then can be used with:

FreeCAD -c make_snapshot_from_file.py

or

FreeCADCmd make_snapshot_from_file.py
"""

import FreeCAD
import FreeCADGui
import os
import importlib
import sys
from PySide2 import QtGui


def init_gui():
    try:
        FreeCADGui.setupWithoutGUI()
    except Exception as e:
        pass


def make_snapshot(input_file, output_file, size=48):
    from pivy import coin

    ext = os.path.splitext(input_file)[1][1:]
    mod = FreeCAD.getImportType(ext)
    if len(mod) == 0:
        print("Cannot load file {}".format(input_file))
        return

    # use the first listed module
    module = importlib.import_module(mod[0])
    module.open(input_file)

    doc = FreeCAD.ActiveDocument
    if doc is None:
        print("No active document")
        return

    init_gui()
    nodes = [FreeCADGui.subgraphFromObject(obj) for obj in doc.Objects]

    # add light and camera so that the rendered geometry is visible
    root = coin.SoSeparator()
    light = coin.SoDirectionalLight()
    cam = coin.SoOrthographicCamera()
    root.addChild(cam)
    root.addChild(light)
    for node in nodes:
        root.addChild(node)

    # do the rendering now
    axo = coin.SbRotation(-0.353553, -0.146447, -0.353553, -0.853553)
    width = size
    height = size
    viewport = coin.SbViewportRegion(width, height)
    cam.orientation.setValue(axo)
    cam.viewAll(root, viewport)
    off = FreeCADGui.SoQtOffscreenRenderer(width, height)
    off.setBackgroundColor(1, 1, 1)
    root.ref()

    # A QGuiApplication is needed to create an OpenGL context
    if QtGui.QGuiApplication.instance() is None:
        app = QtGui.QGuiApplication(sys.argv)

    off.render(root)
    off.writeToImage(output_file)
    root.unref()
