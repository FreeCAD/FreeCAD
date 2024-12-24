# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2023 Yorik van Havre <yorik@uncreated.net>              *
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
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
# *                                                                         *
# ***************************************************************************

"""Unit test for the Native IFC module"""

import os
import time
import tempfile
import FreeCAD
import Draft
import Arch
import unittest
import requests
from nativeifc import ifc_import
from nativeifc import ifc_tools
from nativeifc import ifc_geometry
from nativeifc import ifc_materials
from nativeifc import ifc_layers
from nativeifc import ifc_psets
from nativeifc import ifc_objects
from nativeifc import ifc_generator
import ifcopenshell
from ifcopenshell.util import element
import difflib

IFCOPENHOUSE_IFC4 = (
    "https://github.com/aothms/IfcOpenHouse/raw/master/IfcOpenHouse_IFC4.ifc"
)
IFC_FILE_PATH = None  # downloaded IFC file path
FCSTD_FILE_PATH = None  # saved FreeCAD file
PARAMS = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/NativeIFC")
SINGLEDOC = False  # This allows to force singledoc mode for all tests
SDU = int(SINGLEDOC)  # number of objects is different in singledoc

"""
unit tests for the NativeIFC functionality. To run the tests, either:
- in terminal mode: FreeCAD -t ifc_selftest
- in the FreeCAD UI: Switch to Test Framework workbench, press "Self test" and
  choose ifc_selftest in the list
"""


def getIfcFilePath():
    global IFC_FILE_PATH
    if not IFC_FILE_PATH:
        path = tempfile.mkstemp(suffix=".ifc")[1]
        results = requests.get(IFCOPENHOUSE_IFC4)
        with open(path, "wb") as f:
            f.write(results.content)
        IFC_FILE_PATH = path
    return IFC_FILE_PATH


def clearObjects():
    names = [o.Name for o in FreeCAD.getDocument("IfcTest").Objects]
    for n in names:
        FreeCAD.getDocument("IfcTest").removeObject(n)


def compare(file1, file2):
    with open(file1) as f1:
        f1_text = f1.readlines()
    with open(file2) as f2:
        f2_text = f2.readlines()
    res = [
        l
        for l in difflib.unified_diff(
            f1_text, f2_text, fromfile=file1, tofile=file2, lineterm=""
        )
    ]
    res = [l for l in res if l.startswith("+") or l.startswith("-")]
    res = [l for l in res if not l.startswith("+++") and not l.startswith("---")]
    return res


class NativeIFCTest(unittest.TestCase):
    def setUp(self):
        # setting a new document to hold the tests
        if FreeCAD.ActiveDocument:
            if FreeCAD.ActiveDocument.Name != "IfcTest":
                FreeCAD.newDocument("IfcTest")
        else:
            FreeCAD.newDocument("IfcTest")
        FreeCAD.setActiveDocument("IfcTest")

    def tearDown(self):
        FreeCAD.closeDocument("IfcTest")
        pass

    def test01_ImportCoinSingle(self):
        FreeCAD.Console.PrintMessage(
            "1.  NativeIFC import: Single object, coin mode..."
        )
        clearObjects()
        fp = getIfcFilePath()
        ifc_import.insert(
            fp,
            "IfcTest",
            strategy=0,
            shapemode=1,
            switchwb=0,
            silent=True,
            singledoc=SINGLEDOC,
        )
        fco = len(FreeCAD.getDocument("IfcTest").Objects)
        self.assertTrue(fco == 1 - SDU, "ImportCoinSingle failed")

    def test02_ImportCoinStructure(self):
        FreeCAD.Console.PrintMessage(
            "2.  NativeIFC import: Model structure, coin mode..."
        )
        clearObjects()
        fp = getIfcFilePath()
        ifc_import.insert(
            fp,
            "IfcTest",
            strategy=1,
            shapemode=1,
            switchwb=0,
            silent=True,
            singledoc=SINGLEDOC,
        )
        fco = len(FreeCAD.getDocument("IfcTest").Objects)
        self.assertTrue(fco == 4 - SDU, "ImportCoinStructure failed")

    def test03_ImportCoinFull(self):
        global FCSTD_FILE_PATH
        FreeCAD.Console.PrintMessage("3.  NativeIFC import: Full model, coin mode...")
        clearObjects()
        fp = getIfcFilePath()
        d = ifc_import.insert(
            fp,
            "IfcTest",
            strategy=2,
            shapemode=1,
            switchwb=0,
            silent=True,
            singledoc=SINGLEDOC,
        )
        path = tempfile.mkstemp(suffix=".FCStd")[1]
        d.saveAs(path)
        FCSTD_FILE_PATH = path
        fco = len(FreeCAD.getDocument("IfcTest").Objects)
        self.assertTrue(fco > 4 - SDU, "ImportCoinFull failed")

    def test04_ImportShapeFull(self):
        FreeCAD.Console.PrintMessage("4.  NativeIFC import: Full model, shape mode...")
        clearObjects()
        fp = getIfcFilePath()
        d = ifc_import.insert(
            fp,
            "IfcTest",
            strategy=2,
            shapemode=0,
            switchwb=0,
            silent=True,
            singledoc=SINGLEDOC,
        )
        fco = len(FreeCAD.getDocument("IfcTest").Objects)
        self.assertTrue(fco > 4 - SDU, "ImportShapeFull failed")

    def test05_ImportFreeCAD(self):
        FreeCAD.Console.PrintMessage(
            "5.  NativeIFC FreeCAD import: NativeIFC coin file..."
        )
        clearObjects()
        doc = FreeCAD.open(FCSTD_FILE_PATH)
        obj = doc.Objects[-1]
        proj = ifc_tools.get_project(obj)
        ifcfile = ifc_tools.get_ifcfile(proj)
        print(ifcfile)
        self.assertTrue(ifcfile, "ImportFreeCAD failed")

    def test06_ModifyObjects(self):
        FreeCAD.Console.PrintMessage("6.  NativeIFC Modifying IFC document...")
        doc = FreeCAD.open(FCSTD_FILE_PATH)
        obj = doc.Objects[-1]
        obj.Label = "Modified name"
        proj = ifc_tools.get_project(obj)
        proj.IfcFilePath = proj.IfcFilePath[:-4] + "_modified.ifc"
        ifc_tools.save_ifc(proj)
        ifc_diff = compare(IFC_FILE_PATH, proj.IfcFilePath)
        obj.ShapeMode = 0
        obj.Proxy.execute(obj)
        self.assertTrue(
            obj.Shape.Volume > 2 and len(ifc_diff) <= 5, "ModifyObjects failed"
        )

    def test07_CreateDocument(self):
        FreeCAD.Console.PrintMessage("7.  NativeIFC Creating new IFC document...")
        doc = FreeCAD.ActiveDocument
        ifc_tools.create_document(doc, silent=True)
        fco = len(FreeCAD.getDocument("IfcTest").Objects)
        print(FreeCAD.getDocument("IfcTest").Objects)
        self.assertTrue(fco == 1 - SDU, "CreateDocument failed")

    def test08_ChangeIFCSchema(self):
        FreeCAD.Console.PrintMessage("8.  NativeIFC Changing IFC schema...")
        clearObjects()
        fp = getIfcFilePath()
        ifc_import.insert(
            fp,
            "IfcTest",
            strategy=2,
            shapemode=1,
            switchwb=0,
            silent=True,
            singledoc=SINGLEDOC,
        )
        obj = FreeCAD.getDocument("IfcTest").Objects[-1]
        proj = ifc_tools.get_project(obj)
        oldid = obj.StepId
        proj.Proxy.silent = True
        proj.Schema = "IFC2X3"
        FreeCAD.getDocument("IfcTest").recompute()
        self.assertTrue(obj.StepId != oldid, "ChangeIFCSchema failed")

    def test09_CreateBIMObjects(self):
        FreeCAD.Console.PrintMessage("9.  NativeIFC Creating BIM objects...")
        doc = FreeCAD.ActiveDocument
        proj = ifc_tools.create_document(doc, silent=True)
        site = Arch.makeSite()
        site = ifc_tools.aggregate(site, proj)
        bldg = Arch.makeBuilding()
        bldg = ifc_tools.aggregate(bldg, site)
        storey = Arch.makeFloor()
        storey = ifc_tools.aggregate(storey, bldg)
        wall = Arch.makeWall(None, 200, 400, 20)
        wall = ifc_tools.aggregate(wall, storey)
        column = Arch.makeStructure(None, 20, 20, 200)
        column.IfcType = "Column"
        column = ifc_tools.aggregate(column, storey)
        beam = Arch.makeStructure(None, 20, 200, 20)
        beam.IfcType = "Beam"
        beam = ifc_tools.aggregate(beam, storey)
        rect = Draft.makeRectangle(200, 200)
        slab = Arch.makeStructure(rect, height=20)
        slab.IfcType = "Slab"
        slab = ifc_tools.aggregate(slab, storey)
        # TODO create door, window
        fco = len(FreeCAD.getDocument("IfcTest").Objects)
        ifco = len(proj.Proxy.ifcfile.by_type("IfcRoot"))
        print(ifco, "IFC objects created")
        self.assertTrue(fco == 8 - SDU and ifco == 12, "CreateDocument failed")

    def test10_ChangePlacement(self):
        FreeCAD.Console.PrintMessage("10. NativeIFC Changing Placement...")
        clearObjects()
        fp = getIfcFilePath()
        ifc_import.insert(
            fp,
            "IfcTest",
            strategy=2,
            shapemode=1,
            switchwb=0,
            silent=True,
            singledoc=SINGLEDOC,
        )
        obj = FreeCAD.getDocument("IfcTest").getObject("IfcObject00" + str(4 - SDU))
        elem = ifc_tools.get_ifc_element(obj)
        obj.Placement.move(FreeCAD.Vector(100, 200, 300))
        new_plac = ifcopenshell.util.placement.get_local_placement(elem.ObjectPlacement)
        new_plac = str(new_plac).replace(" ", "").replace("\n", "")
        target = "[[1.0.0.100.][0.1.0.200.][0.0.1.300.][0.0.0.1.]]"
        self.assertTrue(new_plac == target, "ChangePlacement failed")

    def test11_ChangeGeometry(self):
        FreeCAD.Console.PrintMessage("11. NativeIFC Changing Geometry...")
        clearObjects()
        fp = getIfcFilePath()
        ifc_import.insert(
            fp,
            "IfcTest",
            strategy=2,
            shapemode=0,
            switchwb=0,
            silent=True,
            singledoc=SINGLEDOC,
        )
        obj = FreeCAD.getDocument("IfcTest").getObject("IfcObject004")
        ifc_geometry.add_geom_properties(obj)
        obj.ExtrusionDepth = "6000 mm"
        FreeCAD.getDocument("IfcTest").recompute()
        self.assertTrue(obj.Shape.Volume > 1500000, "ChangeGeometry failed")

    def test12_RemoveObject(self):
        from nativeifc import ifc_observer
        ifc_observer.add_observer()
        FreeCAD.Console.PrintMessage("12. NativeIFC Remove object...")
        clearObjects()
        fp = getIfcFilePath()
        ifc_import.insert(
            fp,
            "IfcTest",
            strategy=2,
            shapemode=0,
            switchwb=0,
            silent=True,
            singledoc=SINGLEDOC,
        )
        ifcfile = ifc_tools.get_ifcfile(FreeCAD.getDocument("IfcTest").Objects[-1])
        count1 = len(ifcfile.by_type("IfcProduct"))
        FreeCAD.getDocument("IfcTest").removeObject("IfcObject004")
        count2 = len(ifcfile.by_type("IfcProduct"))
        self.assertTrue(count2 < count1, "RemoveObject failed")

    def test13_Materials(self):
        FreeCAD.Console.PrintMessage("13. NativeIFC Materials...")
        clearObjects()
        fp = getIfcFilePath()
        ifc_import.insert(
            fp,
            "IfcTest",
            strategy=2,
            shapemode=0,
            switchwb=0,
            silent=True,
            singledoc=SINGLEDOC,
        )
        proj = FreeCAD.getDocument("IfcTest").Objects[0]
        ifc_materials.load_materials(proj)
        prod = FreeCAD.getDocument("IfcTest").getObject("IfcObject006")
        ifcfile = ifc_tools.get_ifcfile(prod)
        mats_before = ifcfile.by_type("IfcMaterialDefinition")
        mat = Arch.makeMaterial("Red")
        ifc_materials.set_material(mat, prod)
        elem = ifc_tools.get_ifc_element(prod)
        res = ifcopenshell.util.element.get_material(elem)
        mats_after = ifcfile.by_type("IfcMaterialDefinition")
        self.assertTrue(len(mats_after) == len(mats_before) + 1, "Materials failed")

    def test14_Layers(self):
        FreeCAD.Console.PrintMessage("14. NativeIFC Layers...")
        clearObjects()
        fp = getIfcFilePath()
        ifc_import.insert(
            fp,
            "IfcTest",
            strategy=2,
            shapemode=0,
            switchwb=0,
            silent=True,
            singledoc=SINGLEDOC,
        )
        proj = FreeCAD.getDocument("IfcTest").Objects[0]
        ifcfile = ifc_tools.get_ifcfile(proj)
        lays_before = ifcfile.by_type("IfcPresentationLayerAssignment")
        layer = ifc_layers.create_layer("My Layer", proj)
        prod = FreeCAD.getDocument("IfcTest").getObject("IfcObject006")
        ifc_layers.add_to_layer(prod, layer)
        lays_after = ifcfile.by_type("IfcPresentationLayerAssignment")
        self.assertTrue(len(lays_after) == len(lays_before) + 1, "Layers failed")

    def test15_Psets(self):
        FreeCAD.Console.PrintMessage("15. NativeIFC Psets...")
        clearObjects()
        fp = getIfcFilePath()
        ifc_import.insert(
            fp,
            "IfcTest",
            strategy=2,
            shapemode=0,
            switchwb=0,
            silent=True,
            singledoc=SINGLEDOC,
        )
        obj = FreeCAD.getDocument("IfcTest").getObject("IfcObject004")
        ifcfile = ifc_tools.get_ifcfile(obj)
        pset = ifc_psets.add_pset(obj, "Pset_Custom")
        ifc_psets.add_property(ifcfile, pset, "MyMessageToTheWorld", "Hello, World!")
        self.assertTrue(ifc_psets.has_psets(obj), "Psets failed")
