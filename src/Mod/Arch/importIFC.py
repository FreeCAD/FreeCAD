#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2014                                                    *
#*   Yorik van Havre <yorik@uncreated.net>                                 *
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

from __future__ import print_function

__title__ =  "FreeCAD IFC importer - Enhanced ifcopenshell-only version"
__author__ = "Yorik van Havre","Jonathan Wiedemann"
__url__ =    "http://www.freecadweb.org"

import six
import os
import time
import tempfile
import uuid
import math
import sys

import FreeCAD
import Part
import Draft
import Arch
import DraftVecUtils
import ArchIFCSchema
from DraftGeomUtils import vec

## @package importIFC
#  \ingroup ARCH
#  \brief IFC file format importer and exporter
#
#  This module provides tools to import and export IFC files.

DEBUG = False # Set to True to see debug messages. Otherwise, totally silent
ADDDEFAULTSTOREY = False # If True, an exported file will ALWAYS have at least one storey
ZOOMOUT = True # Set to False to not zoom extents after import

if open.__module__ in ['__builtin__','io']:
    pyopen = open # because we'll redefine open below


# ************************************************************************************************
# ********** templates and other definitions ****
# which IFC type must create which FreeCAD type
typesmap = {
    "Site": [
        "IfcSite"
    ],
    "Building": [
        "IfcBuilding"
    ],
    "Floor": [
        "IfcBuildingStorey"
    ],
    "Structure": [
        "IfcBeam",
        "IfcBeamStandardCase",
        "IfcColumn",
        "IfcColumnStandardCase",
        "IfcSlab",
        "IfcFooting",
        "IfcPile",
        "IfcTendon"
    ],
    "Wall": [
        "IfcWall",
        "IfcWallStandardCase",
        "IfcCurtainWall"
    ],
    "Window": [
        "IfcWindow",
        "IfcWindowStandardCase",
        "IfcDoor",
        "IfcDoorStandardCase"
    ],
    "Roof": [
        "IfcRoof"
    ],
    "Stairs": [
        "IfcStair",
        "IfcStairFlight",
        "IfcRamp",
        "IfcRampFlight"
    ],
    "Space": [
        "IfcSpace"
    ],
    "Rebar": [
        "IfcReinforcingBar"
    ],
    "Panel": [
        "IfcPlate"
    ],
    "Equipment": [
        "IfcFurnishingElement",
        "IfcSanitaryTerminal",
        "IfcFlowTerminal",
        "IfcElectricAppliance"
    ],
    "Pipe": [
        "IfcPipeSegment"
    ],
    "PipeConnector": [
        "IfcPipeFitting"
    ],
    "BuildingPart":[
        "IfcElementAssembly"
    ]
}

# which IFC entity (product) is a structural object
structuralifcobjects = (
    "IfcStructuralCurveMember",
    "IfcStructuralSurfaceMember",
    "IfcStructuralPointConnection",
    "IfcStructuralCurveConnection",
    "IfcStructuralSurfaceConnection",
    "IfcStructuralAction",
    "IfcStructuralPointAction",
    "IfcStructuralLinearAction",
    "IfcStructuralLinearActionVarying",
    "IfcStructuralPlanarAction"
)

# specific FreeCAD <-> IFC slang translations
translationtable = {
    "Foundation":"Footing",
    "Floor":"BuildingStorey",
    "Rebar":"ReinforcingBar",
    "HydroEquipment":"SanitaryTerminal",
    "ElectricEquipment":"ElectricAppliance",
    "Furniture":"FurnishingElement",
    "Stair Flight":"StairFlight",
    "Curtain Wall":"CurtainWall",
    "Pipe Segment":"PipeSegment",
    "Pipe Fitting":"PipeFitting"
}

# the base IFC template for export
ifctemplate = """ISO-10303-21;
HEADER;
FILE_DESCRIPTION(('ViewDefinition [CoordinationView]'),'2;1');
FILE_NAME('$filename','$timestamp',('$owner','$email'),('$company'),'IfcOpenShell','IfcOpenShell','');
FILE_SCHEMA(('$ifcschema'));
ENDSEC;
DATA;
#1=IFCPERSON($,$,'$owner',$,$,$,$,$);
#2=IFCORGANIZATION($,'$company',$,$,$);
#3=IFCPERSONANDORGANIZATION(#1,#2,$);
#4=IFCAPPLICATION(#2,'$version','FreeCAD','118df2cf_ed21_438e_a41');
#5=IFCOWNERHISTORY(#3,#4,$,.ADDED.,$,#3,#4,$now);
#6=IFCDIRECTION((1.,0.,0.));
#7=IFCDIRECTION((0.,0.,1.));
#8=IFCCARTESIANPOINT((0.,0.,0.));
#9=IFCAXIS2PLACEMENT3D(#8,#7,#6);
#10=IFCDIRECTION((0.,1.,0.));
#11=IFCGEOMETRICREPRESENTATIONCONTEXT('Plan','Model',3,1.E-05,#9,#10);
#12=IFCDIMENSIONALEXPONENTS(0,0,0,0,0,0,0);
#13=IFCSIUNIT(*,.LENGTHUNIT.,$,.METRE.);
#14=IFCSIUNIT(*,.AREAUNIT.,$,.SQUARE_METRE.);
#15=IFCSIUNIT(*,.VOLUMEUNIT.,$,.CUBIC_METRE.);
#16=IFCSIUNIT(*,.PLANEANGLEUNIT.,$,.RADIAN.);
#17=IFCMEASUREWITHUNIT(IFCPLANEANGLEMEASURE(0.017453292519943295),#16);
#18=IFCCONVERSIONBASEDUNIT(#12,.PLANEANGLEUNIT.,'DEGREE',#17);
#19=IFCUNITASSIGNMENT((#13,#14,#15,#18));
#20=IFCPROJECT('$projectid',#5,'$project',$,$,$,$,(#11),#19);
ENDSEC;
END-ISO-10303-21;
"""


# ************************************************************************************************
# ********** some helper, used in import and export and exploerer
def decode(filename,utf=False):

    "turns unicodes into strings"

    if six.PY2 and isinstance(filename,six.text_type):
        # workaround since ifcopenshell currently can't handle unicode filenames
        encoding = "utf8" if utf else sys.getfilesystemencoding()
        filename = filename.encode(encoding)
    return filename


def doubleClickTree(item,column):

    "a double-click callback function for the IFC explorer tool"

    txt = item.text(column)
    if "Entity #" in txt:
        eid = txt.split("#")[1].split(":")[0]
        addr = tree.findItems(eid,0,0)
        if addr:
            tree.scrollToItem(addr[0])
            addr[0].setSelected(True)


def dd2dms(dd):

    "converts decimal degrees to degrees,minutes,seconds"

    dd = abs(dd)
    minutes,seconds = divmod(dd*3600,60)
    degrees,minutes = divmod(minutes,60)
    if dd < 0:
        degrees = -degrees
    return (int(degrees),int(minutes),int(seconds))


def dms2dd(degrees, minutes, seconds, milliseconds=0):

    "converts degrees,minutes,seconds to decimal degrees"

    dd = float(degrees) + float(minutes)/60 + float(seconds)/(3600)
    return dd


def getPreferences():

    """retrieves IFC preferences"""

    global DEBUG, PREFIX_NUMBERS, SKIP, SEPARATE_OPENINGS
    global ROOT_ELEMENT, GET_EXTRUSIONS, MERGE_MATERIALS
    global MERGE_MODE_ARCH, MERGE_MODE_STRUCT, CREATE_CLONES
    global FORCE_BREP, IMPORT_PROPERTIES, STORE_UID, SERIALIZE
    global SPLIT_LAYERS, EXPORT_2D, FULL_PARAMETRIC, FITVIEW_ONIMPORT
    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
    if FreeCAD.GuiUp and p.GetBool("ifcShowDialog",False):
        import FreeCADGui
        FreeCADGui.showPreferences("Import-Export",0)
    DEBUG = p.GetBool("ifcDebug",False)
    PREFIX_NUMBERS = p.GetBool("ifcPrefixNumbers",False)
    SKIP = p.GetString("ifcSkip","").split(",")
    SEPARATE_OPENINGS = p.GetBool("ifcSeparateOpenings",False)
    ROOT_ELEMENT = p.GetString("ifcRootElement","IfcProduct")
    GET_EXTRUSIONS = p.GetBool("ifcGetExtrusions",False)
    MERGE_MATERIALS = p.GetBool("ifcMergeMaterials",False)
    MERGE_MODE_ARCH = p.GetInt("ifcImportModeArch",0)
    MERGE_MODE_STRUCT = p.GetInt("ifcImportModeStruct",1)
    if MERGE_MODE_ARCH > 0:
        SEPARATE_OPENINGS = False
        GET_EXTRUSIONS = False
    if not SEPARATE_OPENINGS:
        SKIP.append("IfcOpeningElement")
    CREATE_CLONES = p.GetBool("ifcCreateClones",True)
    FORCE_BREP = p.GetBool("ifcExportAsBrep",False)
    IMPORT_PROPERTIES = p.GetBool("ifcImportProperties",False)
    STORE_UID = p.GetBool("ifcStoreUid",True)
    SERIALIZE = p.GetBool("ifcSerialize",False)
    SPLIT_LAYERS = p.GetBool("ifcSplitLayers",False)
    EXPORT_2D = p.GetBool("ifcExport2D",True)
    FULL_PARAMETRIC = p.GetBool("IfcExportFreeCADProperties",False)
    FITVIEW_ONIMPORT = p.GetBool("ifcFitViewOnImport",False)


# ************************************************************************************************
# ********** IFC explorer ***********************
def explore(filename=None):

    """explore([filename]): opens a dialog showing
    the contents of an IFC file. If no filename is given, a dialog will
    pop up to choose a file."""

    getPreferences()

    try:
        import ifcopenshell
    except:
        FreeCAD.Console.PrintError("IfcOpenShell was not found on this system. IFC support is disabled\n")
        return

    if not filename:
        from PySide import QtGui
        filename = QtGui.QFileDialog.getOpenFileName(QtGui.QApplication.activeWindow(),'IFC files','*.ifc')
        if filename:
            filename = filename[0]

    from PySide import QtCore,QtGui

    filename = decode(filename,utf=True)

    if not os.path.exists(filename):
        print("File not found")
        return

    # draw the widget contents
    ifc = ifcopenshell.open(filename)
    global tree
    tree = QtGui.QTreeWidget()
    tree.setColumnCount(3)
    tree.setWordWrap(True)
    tree.header().setDefaultSectionSize(60)
    tree.header().resizeSection(0,60)
    tree.header().resizeSection(1,30)
    tree.header().setStretchLastSection(True)
    tree.headerItem().setText(0, "ID")
    tree.headerItem().setText(1, "")
    tree.headerItem().setText(2, "Item and Properties")
    bold = QtGui.QFont()
    bold.setWeight(75)
    bold.setBold(True)

    entities =  ifc.by_type("IfcRoot")
    entities += ifc.by_type("IfcRepresentation")
    entities += ifc.by_type("IfcRepresentationItem")
    entities += ifc.by_type("IfcRepresentationMap")
    entities += ifc.by_type("IfcPlacement")
    entities += ifc.by_type("IfcProperty")
    entities += ifc.by_type("IfcPhysicalSimpleQuantity")
    entities += ifc.by_type("IfcMaterial")
    entities += ifc.by_type("IfcProductRepresentation")
    entities = sorted(entities, key=lambda eid: eid.id())

    done = []

    for entity in entities:
        if hasattr(entity,"id"):
            if entity.id() in done:
                continue
            done.append(entity.id())
            item = QtGui.QTreeWidgetItem(tree)
            item.setText(0,str(entity.id()))
            if entity.is_a() in ["IfcWall","IfcWallStandardCase"]:
                item.setIcon(1,QtGui.QIcon(":icons/Arch_Wall_Tree.svg"))
            elif entity.is_a() in ["IfcBuildingElementProxy"]:
                item.setIcon(1,QtGui.QIcon(":icons/Arch_Component.svg"))
            elif entity.is_a() in ["IfcColumn","IfcColumnStandardCase","IfcBeam","IfcBeamStandardCase","IfcSlab","IfcFooting","IfcPile","IfcTendon"]:
                item.setIcon(1,QtGui.QIcon(":icons/Arch_Structure_Tree.svg"))
            elif entity.is_a() in ["IfcSite"]:
                item.setIcon(1,QtGui.QIcon(":icons/Arch_Site_Tree.svg"))
            elif entity.is_a() in ["IfcBuilding"]:
                item.setIcon(1,QtGui.QIcon(":icons/Arch_Building_Tree.svg"))
            elif entity.is_a() in ["IfcBuildingStorey"]:
                item.setIcon(1,QtGui.QIcon(":icons/Arch_Floor_Tree.svg"))
            elif entity.is_a() in ["IfcWindow","IfcWindowStandardCase","IfcDoor","IfcDoorStandardCase"]:
                item.setIcon(1,QtGui.QIcon(":icons/Arch_Window_Tree.svg"))
            elif entity.is_a() in ["IfcRoof"]:
                item.setIcon(1,QtGui.QIcon(":icons/Arch_Roof_Tree.svg"))
            elif entity.is_a() in ["IfcExtrudedAreaSolid","IfcClosedShell"]:
                item.setIcon(1,QtGui.QIcon(":icons/Tree_Part.svg"))
            elif entity.is_a() in ["IfcFace"]:
                item.setIcon(1,QtGui.QIcon(":icons/Draft_SwitchMode.svg"))
            elif entity.is_a() in ["IfcArbitraryClosedProfileDef","IfcPolyloop"]:
                item.setIcon(1,QtGui.QIcon(":icons/Draft_Draft.svg"))
            elif entity.is_a() in ["IfcPropertySingleValue","IfcQuantityArea","IfcQuantityVolume"]:
                item.setIcon(1,QtGui.QIcon(":icons/Tree_Annotation.svg"))
            elif entity.is_a() in ["IfcMaterial"]:
                item.setIcon(1,QtGui.QIcon(":icons/Arch_Material.svg"))
            elif entity.is_a() in ["IfcReinforcingBar"]:
                item.setIcon(1,QtGui.QIcon(":icons/Arch_Rebar.svg"))
            item.setText(2,str(entity.is_a()))
            item.setFont(2,bold)

            i = 0
            while True:
                try:
                    argname = entity.attribute_name(i)
                except:
                    break
                else:
                    try:
                        argvalue = getattr(entity,argname)
                    except:
                        print("Error in entity ", entity)
                        break
                    else:
                        if argname not in ["Id", "GlobalId"]:
                            colored = False
                            if isinstance(argvalue,ifcopenshell.entity_instance):
                                if argvalue.id() == 0:
                                    t = str(argvalue)
                                else:
                                    colored = True
                                    t = "Entity #" + str(argvalue.id()) + ": " + str(argvalue.is_a())
                            elif isinstance(argvalue,list):
                                t = ""
                            elif six.PY2 and isinstance(argvalue,six.string_types):
                                t = argvalue.encode("latin1")
                            else:
                                t = str(argvalue)
                            t = "    " + str(argname.encode("utf8")) + " : " + str(t)
                            item = QtGui.QTreeWidgetItem(tree)
                            item.setText(2,str(t))
                            if colored:
                                item.setForeground(2,QtGui.QBrush(QtGui.QColor("#005AFF")))
                            if isinstance(argvalue,list):
                                for argitem in argvalue:
                                    colored = False
                                    if isinstance(argitem,ifcopenshell.entity_instance):
                                        if argitem.id() == 0:
                                            t = str(argitem)
                                        else:
                                            colored = True
                                            t = "Entity #" + str(argitem.id()) + ": " + str(argitem.is_a())
                                    else:
                                        t = argitem
                                    t = "        " + str(t)
                                    item = QtGui.QTreeWidgetItem(tree)
                                    item.setText(2,str(t))
                                    if colored:
                                        item.setForeground(2,QtGui.QBrush(QtGui.QColor("#005AFF")))
                    i += 1

    d = QtGui.QDialog()
    d.setObjectName("IfcExplorer")
    d.setWindowTitle("Ifc Explorer")
    d.resize(640, 480)
    layout = QtGui.QVBoxLayout(d)
    layout.addWidget(tree)

    tree.itemDoubleClicked.connect(doubleClickTree)

    d.exec_()
    del tree
    return


# ************************************************************************************************
# ********** open and import IFC ****************
def open(filename,skip=[],only=[],root=None):

    "opens an IFC file in a new document"

    docname = os.path.splitext(os.path.basename(filename))[0]
    docname = decode(docname,utf=True)
    doc = FreeCAD.newDocument(docname)
    doc.Label = docname
    doc = insert(filename,doc.Name,skip,only,root)
    return doc


def insert(filename,docname,skip=[],only=[],root=None):

    """insert(filename,docname,skip=[],only=[],root=None): imports the contents of an IFC file.
    skip can contain a list of ids of objects to be skipped, only can restrict the import to
    certain object ids (will also get their children) and root can be used to
    import only the derivates of a certain element type (default = ifcProduct)."""

    getPreferences()

    try:
        import ifcopenshell
    except:
        FreeCAD.Console.PrintError("IfcOpenShell was not found on this system. IFC support is disabled\n")
        return

    if DEBUG: print("Opening ",filename,"...",end="")
    try:
        doc = FreeCAD.getDocument(docname)
    except:
        doc = FreeCAD.newDocument(docname)
    FreeCAD.ActiveDocument = doc

    if DEBUG: print("done.")

    global ROOT_ELEMENT, parametrics

    if root:
        ROOT_ELEMENT = root

    # global ifcfile # keeping global for debugging purposes

    filename = decode(filename,utf=True)
    ifcfile = ifcopenshell.open(filename)

    # IfcOpenShell multiplies the precision value of the file by 100
    # So we raise the precision by 100 too to compensate...
    #ctxs = ifcfile.by_type("IfcGeometricRepresentationContext")
    #for ctx in ctxs:
    #    if not ctx.is_a("IfcGeometricRepresentationSubContext"):
    #        ctx.Precision = ctx.Precision/100

    # set default ifcopenshell options to work in brep mode
    from ifcopenshell import geom
    settings = ifcopenshell.geom.settings()
    settings.set(settings.USE_BREP_DATA,True)
    settings.set(settings.SEW_SHELLS,True)
    settings.set(settings.USE_WORLD_COORDS,True)
    if SEPARATE_OPENINGS:
        settings.set(settings.DISABLE_OPENING_SUBTRACTIONS,True)
    if SPLIT_LAYERS and hasattr(settings,"APPLY_LAYERSETS"):
        settings.set(settings.APPLY_LAYERSETS,True)

    # gather easy entity types
    sites = ifcfile.by_type("IfcSite")
    buildings = ifcfile.by_type("IfcBuilding")
    floors = ifcfile.by_type("IfcBuildingStorey")
    products = ifcfile.by_type(ROOT_ELEMENT)
    openings = ifcfile.by_type("IfcOpeningElement")
    annotations = ifcfile.by_type("IfcAnnotation")
    materials = ifcfile.by_type("IfcMaterial")

    if DEBUG: print("Building relationships table...",end="")

    # building relations tables

    objects = {} # { id:object, ... }
    prodrepr = {} # product/representations table
    additions = {} # { host:[child,...], ... }
    groups = {} # { host:[child,...], ... }     # used in structural IFC
    subtractions = [] # [ [opening,host], ... ]
    properties = {} # { objid : { psetid : [propertyid, ... ], ... }, ... }
    colors = {} # { id:(r,g,b) }
    shapes = {} # { id:shaoe } only used for merge mode
    structshapes = {} # { id:shaoe } only used for merge mode
    mattable = {} # { objid:matid }
    sharedobjects = {} # { representationmapid:object }
    parametrics = [] # a list of imported objects whose parametric relationships need processing after all objects have been created
    profiles = {} # to store reused extrusion profiles {ifcid:fcobj,...}

    for r in ifcfile.by_type("IfcRelContainedInSpatialStructure"):
        additions.setdefault(r.RelatingStructure.id(),[]).extend([e.id() for e in r.RelatedElements])
    for r in ifcfile.by_type("IfcRelAggregates"):
        additions.setdefault(r.RelatingObject.id(),[]).extend([e.id() for e in r.RelatedObjects])
    for r in ifcfile.by_type("IfcRelAssignsToGroup"):
        groups.setdefault(r.RelatingGroup.id(),[]).extend([e.id() for e in r.RelatedObjects])
    for r in ifcfile.by_type("IfcRelVoidsElement"):
        subtractions.append([r.RelatedOpeningElement.id(), r.RelatingBuildingElement.id()])
    for r in ifcfile.by_type("IfcRelDefinesByProperties"):
        for obj in r.RelatedObjects:
            if not obj.id() in properties:
                properties[obj.id()] = {}
            psets = {}
            props = []
            if r.RelatingPropertyDefinition.is_a("IfcPropertySet"):
                props.extend([prop.id() for prop in r.RelatingPropertyDefinition.HasProperties])
                psets[r.RelatingPropertyDefinition.id()] = props
                properties[obj.id()].update(psets)
    for r in ifcfile.by_type("IfcRelAssociatesMaterial"):
        for o in r.RelatedObjects:
            if r.RelatingMaterial.is_a("IfcMaterial"):
                mattable[o.id()] = r.RelatingMaterial.id()
            elif r.RelatingMaterial.is_a("IfcMaterialLayer"):
                mattable[o.id()] = r.RelatingMaterial.Material.id()
            elif r.RelatingMaterial.is_a("IfcMaterialLayerSet"):
                mattable[o.id()] = r.RelatingMaterial.MaterialLayers[0].Material.id()
            elif r.RelatingMaterial.is_a("IfcMaterialLayerSetUsage"):
                mattable[o.id()] = r.RelatingMaterial.ForLayerSet.MaterialLayers[0].Material.id()
    for p in ifcfile.by_type("IfcProduct"):
        if hasattr(p,"Representation"):
            if p.Representation:
                for it in p.Representation.Representations:
                    for it1 in it.Items:
                        prodrepr.setdefault(p.id(),[]).append(it1.id())
                        if it1.is_a("IfcBooleanResult"):
                            prodrepr.setdefault(p.id(),[]).append(it1.FirstOperand.id())
                        elif it.Items[0].is_a("IfcMappedItem"):
                            prodrepr.setdefault(p.id(),[]).append(it1.MappingSource.MappedRepresentation.id())
                            if it1.MappingSource.MappedRepresentation.is_a("IfcShapeRepresentation"):
                                for it2 in it1.MappingSource.MappedRepresentation.Items:
                                    prodrepr.setdefault(p.id(),[]).append(it2.id())
    for r in ifcfile.by_type("IfcStyledItem"):
        if r.Styles:
            if r.Styles[0].is_a("IfcPresentationStyleAssignment"):
                if r.Styles[0].Styles[0].is_a("IfcSurfaceStyle"):
                    if r.Styles[0].Styles[0].Styles[0].is_a("IfcSurfaceStyleRendering"):
                        if r.Styles[0].Styles[0].Styles[0].SurfaceColour:
                            c = r.Styles[0].Styles[0].Styles[0].SurfaceColour
                            if r.Item:
                                for p in prodrepr.keys():
                                    if r.Item.id() in prodrepr[p]:
                                        colors[p] = (c.Red,c.Green,c.Blue)
                        else:
                            for m in ifcfile.by_type("IfcMaterialDefinitionRepresentation"):
                                for it in m.Representations:
                                    if it.Items:
                                        if it.Items[0].id() == r.id():
                                            colors[m.RepresentedMaterial.id()] = (c.Red,c.Green,c.Blue)

    # remove any leftover annotations from products
    tp = []
    for product in products:
        if product.is_a("IfcGrid") and not (product in annotations):
            annotations.append(product)
        elif not (product in annotations):
            tp.append(product)
    products = sorted(tp,key=lambda prod: prod.id())

    # only import a list of IDs and their children
    if only:
        ids = []
        while only:
            currentid = only.pop()
            ids.append(currentid)
            if currentid in additions.keys():
                only.extend(additions[currentid])
        products = [ifcfile[currentid] for currentid in ids]

    if DEBUG: print("done.")

    count = 0
    from FreeCAD import Base
    progressbar = Base.ProgressIndicator()
    progressbar.start("Importing IFC objects...",len(products))
    if DEBUG: print("Processing",len(products),"BIM objects...")

    if FITVIEW_ONIMPORT and FreeCAD.GuiUp:
        overallboundbox = None
        import FreeCADGui
        FreeCADGui.ActiveDocument.activeView().viewAxonometric()

    # handle IFC products

    for product in products:

        count += 1

        pid = product.id()
        guid = product.GlobalId
        ptype = product.is_a()
        if DEBUG: print(count,"/",len(products),"object #"+str(pid),":",ptype,end="")

        # checking for full FreeCAD parametric definition, overriding everything else

        if pid in properties.keys() and FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch").GetBool("IfcImportFreeCADProperties",False):
            if "FreeCADPropertySet" in [ifcfile[pset].Name for pset in properties[pid].keys()]:
                if DEBUG: print(" restoring from parametric definition...",end="")
                obj = createFromProperties(properties[pid],ifcfile)
                if obj:
                    objects[pid] = obj
                    if DEBUG: print("done")
                    continue
                else:
                    print("failed.",end="")

        # no parametric data, we go the good old way

        name = str(ptype[3:])
        if product.Name:
            name = product.Name
            if six.PY2:
                name = name.encode("utf8")
        if PREFIX_NUMBERS: name = "ID" + str(pid) + " " + name
        obj = None
        baseobj = None
        brep = None
        shape = None

        archobj = True  # assume all objects not in structuralifcobjects are architecture
        structobj = False
        if ptype in structuralifcobjects:
            archobj = False
            structobj = True
            if DEBUG: print(" (struct)",end="")
        else:
            if DEBUG: print(" (arch)",end="")
        if MERGE_MODE_ARCH == 4 and archobj:
            if DEBUG: print(" skipped.")
            continue
        if MERGE_MODE_STRUCT == 3 and not archobj:
            if DEBUG: print(" skipped.")
            continue
        if pid in skip: # user given id skip list
            if DEBUG: print(" skipped.")
            continue
        if ptype in SKIP: # preferences-set type skip list
            if DEBUG: print(" skipped.")
            continue

        # detect if this object is sharing its shape

        clone = None
        store = None
        prepr = None
        try:
            prepr = product.Representation
        except:
            if DEBUG: print(" ERROR unable to get object representation",end="")
        if prepr and (MERGE_MODE_ARCH == 0) and archobj and CREATE_CLONES:

            for s in prepr.Representations:
                if s.RepresentationIdentifier.upper() == "BODY":
                    if s.Items[0].is_a("IfcMappedItem"):
                        bid = s.Items[0].MappingSource.id()
                        if bid in sharedobjects:
                            clone = sharedobjects[bid]
                        else:
                            sharedobjects[bid] = None
                            store = bid

        # additional setting for structural entities
        if hasattr(settings,"INCLUDE_CURVES"):
            if structobj:
                settings.set(settings.INCLUDE_CURVES,True)
            else:
                settings.set(settings.INCLUDE_CURVES,False)
        try:
            cr = ifcopenshell.geom.create_shape(settings,product)
            brep = cr.geometry.brep_data
        except:
            pass # IfcOpenShell will yield an error if a given product has no shape, but we don't care, we're brave enough

        if brep:
            if DEBUG: print(" "+str(int(len(brep)/1000))+"k ",end="")

            shape = Part.Shape()
            shape.importBrepFromString(brep,False)

            shape.scale(1000.0) # IfcOpenShell always outputs in meters, we convert to mm, the freecad internal unit

            if not shape.isNull():
                if FITVIEW_ONIMPORT and FreeCAD.GuiUp:
                    # add to the global boundbox
                    try:
                        bb = shape.BoundBox
                        # if DEBUG: print(' ' + str(bb),end="")
                    except:
                        bb = None
                        if DEBUG: print(' BB could not be computed',end="")
                    if bb and bb.isValid():
                        if not overallboundbox:
                            overallboundbox = bb
                        if not overallboundbox.isInside(bb):
                            FreeCADGui.SendMsgToActiveView("ViewFit")
                        overallboundbox.add(bb)

                if (MERGE_MODE_ARCH > 0 and archobj) or structobj:

                    # additional tweaks when not using Arch objects
                    if ptype == "IfcSpace": # do not add spaces to compounds
                        if DEBUG: print("skipping space ",pid,end="")
                    elif structobj:
                        structshapes[pid] = shape
                        if DEBUG: print(len(shape.Solids),"solids ",end="")
                        baseobj = shape
                    else:
                        shapes[pid] = shape
                        if DEBUG: print(len(shape.Solids),"solids ",end="")
                        baseobj = shape
                else:

                    # create base shape object
                    if clone:
                        if DEBUG: print("clone ",end="")
                    else:
                        if GET_EXTRUSIONS and (MERGE_MODE_ARCH != 1):
                            if ptype in ["IfcWall","IfcWallStandardCase","IfcSpace"]:
                                sortmethod = "z"
                            else:
                                sortmethod = "area"
                            ex = Arch.getExtrusionData(shape,sortmethod) # is this an extrusion?
                            if ex:
                                #print("found extrusion:",ex)
                                # check for extrusion profile
                                baseface = None
                                profileid = None
                                addplacement = None
                                if product.Representation:
                                    if product.Representation.Representations:
                                        if product.Representation.Representations[0].is_a("IfcShapeRepresentation"):
                                            if product.Representation.Representations[0].Items:
                                                if product.Representation.Representations[0].Items[0].is_a("IfcExtrudedAreaSolid"):
                                                    profileid = product.Representation.Representations[0].Items[0].SweptArea.id()
                                if profileid and profileid in profiles:
                                    # reuse existing profile
                                    print("shared extrusion ",end="")
                                    baseface = profiles[profileid]
                                    # calculate delta placement between stored profile and this one
                                    addplacement = FreeCAD.Placement()
                                    r = FreeCAD.Rotation(baseface.Shape.Faces[0].normalAt(0,0),ex[0].Faces[0].normalAt(0,0))
                                    if r.Angle > 0.000001:
                                        # use shape methods to easily obtain a correct placement
                                        ts = Part.Shape()
                                        ts.rotate(DraftVecUtils.tup(baseface.Shape.CenterOfMass), DraftVecUtils.tup(r.Axis), math.degrees(r.Angle))
                                        addplacement = ts.Placement
                                    d = ex[0].CenterOfMass.sub(baseface.Shape.CenterOfMass)
                                    if d.Length > 0.000001:
                                        addplacement.move(d)
                                if not baseface:
                                    print("extrusion ",end="")
                                    import DraftGeomUtils
                                    if DraftGeomUtils.hasCurves(ex[0]) or len(ex[0].Wires) != 1:
                                        # curves or holes? We just make a Part face
                                        baseface = FreeCAD.ActiveDocument.addObject("Part::Feature",name+"_footprint")
                                        # bug/feature in ifcopenshell? Some faces of a shell may have non-null placement
                                        # workaround to remove the bad placement: exporting/reimporting as step
                                        if not ex[0].Placement.isNull():
                                            import tempfile
                                            fd, tf = tempfile.mkstemp(suffix=".stp")
                                            ex[0].exportStep(tf)
                                            f = Part.read(tf)
                                            os.close(fd)
                                            os.remove(tf)
                                        else:
                                            f = ex[0]
                                        baseface.Shape = f
                                    else:
                                        # no hole and no curves, we make a Draft Wire instead
                                        baseface = Draft.makeWire([v.Point for v in ex[0].Wires[0].OrderedVertexes],closed=True)
                                    if profileid:
                                        profiles[profileid] = baseface
                                baseobj = FreeCAD.ActiveDocument.addObject("Part::Extrusion",name+"_body")
                                baseobj.Base = baseface
                                if addplacement:
                                    # apply delta placement (stored profile)
                                    baseobj.Placement = addplacement
                                    baseobj.Dir = addplacement.Rotation.inverted().multVec(ex[1])
                                else:
                                    baseobj.Dir = ex[1]
                                if FreeCAD.GuiUp:
                                    baseface.ViewObject.hide()
                        if (not baseobj):
                            baseobj = FreeCAD.ActiveDocument.addObject("Part::Feature",name+"_body")
                            baseobj.Shape = shape
            else:
                if DEBUG: print("null shape ",end="")
            if not shape.isValid():
                if DEBUG: print("invalid shape ",end="")
                #continue

        else:
            if DEBUG: print(" no brep ",end="")

        if MERGE_MODE_ARCH == 0 and archobj:

            # full Arch objects

            for freecadtype,ifctypes in typesmap.items():
                if ptype in ifctypes:
                    if clone:
                        obj = getattr(Arch,"make"+freecadtype)(name=name)
                        obj.CloneOf = clone
                        if shape:
                            if shape.Solids:
                                s1 = shape.Solids[0]
                            else:
                                s1 = shape
                            if clone.Shape.Solids:
                                s2 = clone.Shape.Solids[0]
                            else:
                                s1 = clone.Shape
                            if hasattr(s1,"CenterOfMass") and hasattr(s2,"CenterOfMass"):
                                v = s1.CenterOfMass.sub(s2.CenterOfMass)
                                if product.Representation:
                                    r = getRotation(product.Representation.Representations[0].Items[0].MappingTarget)
                                    if not r.isNull():
                                        v = v.add(s2.CenterOfMass)
                                        v = v.add(r.multVec(s2.CenterOfMass.negative()))
                                    obj.Placement.Rotation = r
                                    obj.Placement.move(v)
                            else:
                                print("failed to compute placement ",)
                    else:
                        obj = getattr(Arch,"make"+freecadtype)(baseobj=baseobj,name=name)
                        if freecadtype in ["Wall","Structure"] and baseobj and baseobj.isDerivedFrom("Part::Extrusion"):
                            # remove intermediary extrusion for types that can extrude themselves
                            obj.Base = baseobj.Base
                            obj.Placement = obj.Placement.multiply(baseobj.Placement)
                            obj.Height = baseobj.Dir.Length
                            obj.Normal = FreeCAD.Vector(baseobj.Dir).normalize()
                            bn = baseobj.Name
                            FreeCAD.ActiveDocument.removeObject(bn)
                        if (freecadtype in ["Structure","Wall"]) and not baseobj:
                            # remove sizes to prevent auto shape creation for types that don't require a base object
                            obj.Height = 0
                            obj.Width = 0
                            obj.Length = 0
                        if store:
                            sharedobjects[store] = obj

                    if ptype == "IfcBuildingStorey":
                        if product.Elevation:
                            obj.Placement.Base.z = product.Elevation * getScaling(ifcfile)

                    break

            if not obj:
                obj = Arch.makeComponent(baseobj,name=name)

            obj.Label = name
            if DEBUG: print(": "+obj.Label+" ",end="")
            if hasattr(obj,"Description") and hasattr(product,"Description"):
                if product.Description:
                    obj.Description = product.Description
            if FreeCAD.GuiUp and baseobj:
                try:
                    if hasattr(baseobj,"ViewObject"):
                        baseobj.ViewObject.hide()
                except ReferenceError:
                    pass

            # setting IFC type

            try:
                if hasattr(obj,"IfcType"):
                    obj.IfcType = ''.join(map(lambda x: x if x.islower() else " "+x, ptype[3:]))[1:]
            except:
                print("Unable to give IFC type ",ptype," to object ",obj.Label)

            # setting uid

            if hasattr(obj,"IfcData"):
                a = obj.IfcData
                a["IfcUID"] = str(guid)
                obj.IfcData = a

            # setting IFC attributes

                for attribute in ArchIFCSchema.IfcProducts[product.is_a()]["attributes"]:
                    #print("attribute:",attribute["name"])
                    if hasattr(product, attribute["name"]) and getattr(product, attribute["name"]) and hasattr(obj,attribute["name"]):
                        #print("Setting attribute",attribute["name"],"to",getattr(product, attribute["name"]))
                        setattr(obj, attribute["name"], getattr(product, attribute["name"]))
                        # TODO: ArchIFCSchema.IfcProducts uses the IFC version from the FreeCAD prefs.
                        # This might not coincide with the file being opened, hence some attributes are not properly read.

            if obj:
                s = ""
                if hasattr(obj,"Shape"):
                    if obj.Shape.Solids:
                        s = str(len(obj.Shape.Solids))+" solids"
                if DEBUG: print(s,end="")
                objects[pid] = obj

        elif (MERGE_MODE_ARCH == 1 and archobj) or (MERGE_MODE_STRUCT == 0 and not archobj):

            # non-parametric Arch objects (just Arch components with a shape)

            if ptype in ["IfcSite","IfcBuilding","IfcBuildingStorey"]:
                for freecadtype,ifctypes in typesmap.items():
                    if ptype in ifctypes:
                        obj = getattr(Arch,"make"+freecadtype)(baseobj=baseobj,name=name)
                        if ptype == "IfcBuildingStorey":
                            if product.Elevation:
                                obj.Placement.Base.z = product.Elevation * getScaling(ifcfile)
            elif baseobj:
                obj = Arch.makeComponent(baseobj,name=name,delete=True)
                obj.Label = name
                if DEBUG: print(": "+obj.Label+" ",end="")
                if hasattr(obj,"Description") and hasattr(product,"Description"):
                    if product.Description:
                        obj.Description = product.Description
                try:
                    if hasattr(obj,"IfcType"):
                        obj.IfcType = ''.join(map(lambda x: x if x.islower() else " "+x, ptype[3:]))[1:]
                except:
                    print("Unable to give IFC type ",ptype," to object ",obj.Label)
                if hasattr(obj,"IfcData"):
                    a = obj.IfcData
                    a["IfcUID"] = str(guid)
                    obj.IfcData = a

        elif (MERGE_MODE_ARCH == 2 and archobj) or (MERGE_MODE_STRUCT == 1 and not archobj):

            # Part shapes

            if ptype in ["IfcSite","IfcBuilding","IfcBuildingStorey"]:
                for freecadtype,ifctypes in typesmap.items():
                    if ptype in ifctypes:
                        obj = getattr(Arch,"make"+freecadtype)(baseobj=baseobj,name=name)
                        if ptype == "IfcBuildingStorey":
                            if product.Elevation:
                                obj.Placement.Base.z = product.Elevation * getScaling(ifcfile)
            elif baseobj:
                obj = FreeCAD.ActiveDocument.addObject("Part::Feature",name)
                obj.Shape = shape

        if DEBUG: print("")  # newline for debug prints, print for a new object should be on a new line

        if obj:

            obj.Label = name
            objects[pid] = obj

            # handle properties

            if pid in properties:

                if IMPORT_PROPERTIES and hasattr(obj,"IfcProperties"):

                    # treat as spreadsheet (pref option)

                    if isinstance(obj.IfcProperties,dict):

                        # fix property type if needed

                        obj.removeProperty("IfcProperties")
                        obj.addProperty("App::PropertyLink","IfcProperties","Component","Stores IFC properties as a spreadsheet")

                    ifc_spreadsheet = Arch.makeIfcSpreadsheet()
                    n=2
                    for c in properties[pid].keys():
                        o = ifcfile[c]
                        if DEBUG: print("propertyset Name",o.Name,type(o.Name))
                        catname = o.Name
                        for p in properties[pid][c]:
                            l = ifcfile[p]
                            lname = l.Name
                            if l.is_a("IfcPropertySingleValue"):
                                if DEBUG:
                                    print("property name",l.Name,type(l.Name))
                                if six.PY2:
                                    catname = catname.encode("utf8")
                                    lname = lname.encode("utf8")
                                ifc_spreadsheet.set(str('A'+str(n)), catname)
                                ifc_spreadsheet.set(str('B'+str(n)), lname)
                                if l.NominalValue:
                                    if DEBUG:
                                        print("property NominalValue",l.NominalValue.is_a(),type(l.NominalValue.is_a()))
                                        print("property NominalValue.wrappedValue",l.NominalValue.wrappedValue,type(l.NominalValue.wrappedValue))
                                        #print("l.NominalValue.Unit",l.NominalValue.Unit,type(l.NominalValue.Unit))
                                    ifc_spreadsheet.set(str('C'+str(n)), l.NominalValue.is_a())
                                    if l.NominalValue.is_a() in ['IfcLabel','IfcText','IfcIdentifier','IfcDescriptiveMeasure']:
                                        if six.PY2:
                                            ifc_spreadsheet.set(str('D'+str(n)), "'" + str(l.NominalValue.wrappedValue.encode("utf8")))
                                        else:
                                            ifc_spreadsheet.set(str('D'+str(n)), "'" + str(l.NominalValue.wrappedValue))
                                    else:
                                        ifc_spreadsheet.set(str('D'+str(n)), str(l.NominalValue.wrappedValue))
                                    if hasattr(l.NominalValue,'Unit'):
                                        ifc_spreadsheet.set(str('E'+str(n)), str(l.NominalValue.Unit))
                                n += 1
                        obj.IfcProperties = ifc_spreadsheet

                elif hasattr(obj,"IfcProperties") and isinstance(obj.IfcProperties,dict):

                    # 0.18 behaviour: properties are saved as pset;;type;;value in IfcProperties

                    d = obj.IfcProperties
                    for pset in properties[pid].keys():
                        #print("adding pset",pset,"to object",obj.Label)
                        psetname = ifcfile[pset].Name
                        if six.PY2:
                            psetname = psetname.encode("utf8")
                        for prop in properties[pid][pset]:
                            e = ifcfile[prop]
                            pname = e.Name
                            if six.PY2:
                                pname = pname.encode("utf8")
                            if e.is_a("IfcPropertySingleValue"):
                                if e.NominalValue:
                                    ptype = e.NominalValue.is_a()
                                    if ptype in ['IfcLabel','IfcText','IfcIdentifier','IfcDescriptiveMeasure']:
                                        pvalue = e.NominalValue.wrappedValue
                                        if six.PY2:
                                            pvalue = pvalue.encode("utf8")
                                    else:
                                        pvalue = str(e.NominalValue.wrappedValue)
                                    if hasattr(e.NominalValue,'Unit'):
                                        if e.NominalValue.Unit:
                                            pvalue += e.NominalValue.Unit
                                    d[pname+";;"+psetname] = ptype+";;"+pvalue
                                #print("adding property: ",pname,ptype,pvalue," pset ",psetname)
                    obj.IfcProperties = d

                elif hasattr(obj,"IfcData"):

                    # 0.17: properties are saved as type(value) in IfcData

                    a = obj.IfcData
                    for c in properties[pid].keys():
                        for p in properties[pid][c]:
                            l = ifcfile[p]
                            if l.is_a("IfcPropertySingleValue"):
                                a[l.Name.encode("utf8")] = str(l.NominalValue) # no py3 support here
                    obj.IfcData = a

            # color

            if FreeCAD.GuiUp and (pid in colors) and hasattr(obj.ViewObject,"ShapeColor"):
                #if DEBUG: print("    setting color: ",int(colors[pid][0]*255),"/",int(colors[pid][1]*255),"/",int(colors[pid][2]*255))
                obj.ViewObject.ShapeColor = colors[pid]

            # if DEBUG is on, recompute after each shape
            if DEBUG: FreeCAD.ActiveDocument.recompute()

            # attached 2D elements

            if product.Representation:
                for r in product.Representation.Representations:
                    if r.RepresentationIdentifier == "FootPrint":
                        annotations.append(product)
                        break

            # additional properties for specific types

            if product.is_a("IfcSite"):
                if product.RefElevation:
                    obj.Elevation = product.RefElevation * getScaling(ifcfile)
                if product.RefLatitude:
                    obj.Latitude = dms2dd(*product.RefLatitude)
                if product.RefLongitude:
                    obj.Longitude = dms2dd(*product.RefLongitude)
                if product.SiteAddress:
                    if product.SiteAddress.AddressLines:
                        obj.Address = product.SiteAddress.AddressLines[0]
                    if product.SiteAddress.Town:
                        obj.City = product.SiteAddress.Town
                    if product.SiteAddress.Region:
                        obj.Region = product.SiteAddress.Region
                    if product.SiteAddress.Country:
                        obj.Country = product.SiteAddress.Country
                    if product.SiteAddress.PostalCode:
                        obj.PostalCode = product.SiteAddress.PostalCode

        try:
            progressbar.next(True)
        except(RuntimeError):
            print("Aborted.")
            progressbar.stop()
            FreeCAD.ActiveDocument.recompute()
            return

    progressbar.stop()
    FreeCAD.ActiveDocument.recompute()

    if MERGE_MODE_STRUCT == 2:

        if DEBUG: print("Joining Structural shapes...",end="")

        for host,children in groups.items(): # Structural
            if ifcfile[host].is_a("IfcStructuralAnalysisModel"):
                compound = []
                for c in children:
                    if c in structshapes.keys():
                        compound.append(structshapes[c])
                        del structshapes[c]
                if compound:
                    name = ifcfile[host].Name or "AnalysisModel"
                    if PREFIX_NUMBERS: name = "ID" + str(host) + " " + name
                    obj = FreeCAD.ActiveDocument.addObject("Part::Feature",name)
                    obj.Label = name
                    obj.Shape = Part.makeCompound(compound)
        if structshapes: # remaining Structural shapes
            obj = FreeCAD.ActiveDocument.addObject("Part::Feature","UnclaimedStruct")
            obj.Shape = Part.makeCompound(structshapes.values())

        if DEBUG: print("done")

    else:

        if DEBUG: print("Processing Struct relationships...",end="")

        # groups

        for host,children in groups.items():
            if ifcfile[host].is_a("IfcStructuralAnalysisModel"):
                # print(host, ' --> ', children)
                obj =  FreeCAD.ActiveDocument.addObject("App::DocumentObjectGroup","AnalysisModel")
                objects[host] = obj
                if host in objects.keys():
                    cobs = []
                    childs_to_delete = []
                    for child in children:
                        if child in objects.keys():
                            cobs.append(objects[child])
                            childs_to_delete.append(child)
                    for c in childs_to_delete:
                        children.remove(c)  # to not process the child again in remaining groups
                    if cobs:
                        if DEBUG: print("adding ",len(cobs), " object(s) to ", objects[host].Label)
                        Arch.addComponents(cobs,objects[host])
                        if DEBUG: FreeCAD.ActiveDocument.recompute()

        if DEBUG: print("done")

        if MERGE_MODE_ARCH > 2:  # if ArchObj is compound or ArchObj not imported
            FreeCAD.ActiveDocument.recompute()

            # cleaning bad shapes
            for obj in objects.values():
                if obj.isDerivedFrom("Part::Feature"):
                    if obj.Shape.isNull():
                        Arch.rebuildArchShape(obj)

    # processing remaining (normal) groups

    swallowed = []
    remaining = {}
    for host,children in groups.items():
        if ifcfile[host].is_a("IfcGroup"):
            if ifcfile[host].Name:
                grp_name = ifcfile[host].Name
            else:
                if DEBUG: print("no group name specified for entity: #", ifcfile[host].id(), ", entity type is used!")
                grp_name = ifcfile[host].is_a() + "_" + str(ifcfile[host].id())
                if six.PY2:
                    grp_name = grp_name.encode("utf8")
            grp =  FreeCAD.ActiveDocument.addObject("App::DocumentObjectGroup",grp_name)
            grp.Label = grp_name
            objects[host] = grp
            for child in children:
                if child in objects.keys():
                    grp.addObject(objects[child])
                    swallowed.append(child)
                else:
                    remaining[child] = grp

    if MERGE_MODE_ARCH == 3:

        # One compound per storey

        if DEBUG: print("Joining Arch shapes...",end="")

        for host,children in additions.items(): # Arch
            if ifcfile[host].is_a("IfcBuildingStorey"):
                compound = []
                for c in children:
                    if c in shapes.keys():
                        compound.append(shapes[c])
                        del shapes[c]
                    if c in additions.keys():
                        for c2 in additions[c]:
                            if c2 in shapes.keys():
                                compound.append(shapes[c2])
                                del shapes[c2]
                if compound:
                    name = ifcfile[host].Name or "Floor"
                    if PREFIX_NUMBERS: name = "ID" + str(host) + " " + name
                    obj = FreeCAD.ActiveDocument.addObject("Part::Feature",name)
                    obj.Label = name
                    obj.Shape = Part.makeCompound(compound)
        if shapes: # remaining Arch shapes
            obj = FreeCAD.ActiveDocument.addObject("Part::Feature","UnclaimedArch")
            obj.Shape = Part.makeCompound(shapes.values())

        if DEBUG: print("done")

    else:

        if DEBUG: print("Processing Arch relationships...",end="")
        first = True

        # subtractions

        if SEPARATE_OPENINGS:
            for subtraction in subtractions:
                if (subtraction[0] in objects.keys()) and (subtraction[1] in objects.keys()):
                    if DEBUG and first:
                        print("")
                        first = False
                    if DEBUG: print("    subtracting",objects[subtraction[0]].Label, "from", objects[subtraction[1]].Label)
                    Arch.removeComponents(objects[subtraction[0]],objects[subtraction[1]])
                    if DEBUG: FreeCAD.ActiveDocument.recompute()

        # additions

        for host,children in additions.items():
            if host in objects.keys():
                cobs = []
                for child in children:
                    if child in objects.keys():
                        if child not in swallowed: # don't add objects already in groups
                            cobs.append(objects[child])
                if cobs:
                    if DEBUG and first:
                        print("")
                        first = False
                    if DEBUG and (len(cobs) > 10) and (not(Draft.getType(objects[host]) in ["Site","Building","Floor","BuildingPart"])):
                        # avoid huge fusions
                        print("more than 10 shapes to add: skipping.")
                    else:
                        if DEBUG: print("    adding",len(cobs), "object(s) to", objects[host].Label)
                        Arch.addComponents(cobs,objects[host])
                        if DEBUG: FreeCAD.ActiveDocument.recompute()

        if DEBUG and first: print("done.")

        FreeCAD.ActiveDocument.recompute()

        # cleaning bad shapes

        for obj in objects.values():
            if obj.isDerivedFrom("Part::Feature"):
                if obj.Shape.isNull() and not(Draft.getType(obj) in ["Site"]):
                    Arch.rebuildArchShape(obj)

    FreeCAD.ActiveDocument.recompute()

    # 2D elements

    if DEBUG and annotations:
        print("Processing",len(annotations),"2D objects...")
    prodcount = count
    count = 0

    scaling = getScaling(ifcfile)
    #print("scaling factor =",scaling)
    for annotation in annotations:

        anno = None
        aid = annotation.id()

        count += 1
        if DEBUG: print(count,"/",len(annotations),"object #"+str(aid),":",annotation.is_a(),end="")

        if aid in skip:
            continue # user given id skip list
        if annotation.is_a() in SKIP:
            continue # preferences-set type skip list
        if annotation.is_a("IfcGrid"):
            axes = []
            uvwaxes = ()
            if annotation.UAxes:
                uvwaxes = annotation.UAxes
            if annotation.VAxes:
                uvwaxes = uvwaxes + annotation.VAxes
            if annotation.WAxes:
                uvwaxes = uvwaxes + annotation.WAxes
            for axis in uvwaxes:
                if axis.AxisCurve:
                    sh = setRepresentation(axis.AxisCurve,scaling)
                    if sh and (len(sh[0].Vertexes) == 2): # currently only straight axes are supported
                        sh = sh[0]
                        l = sh.Length
                        pl = FreeCAD.Placement()
                        pl.Base = sh.Vertexes[0].Point
                        pl.Rotation = FreeCAD.Rotation(FreeCAD.Vector(0,1,0),sh.Vertexes[-1].Point.sub(sh.Vertexes[0].Point))
                        o = Arch.makeAxis(1,l)
                        o.Length = l
                        o.Placement = pl
                        o.CustomNumber = axis.AxisTag
                        axes.append(o)
            if axes:
                name = "Grid"
                if annotation.Name:
                    name = annotation.Name
                    if six.PY2:
                        name = name.encode("utf8")
                if PREFIX_NUMBERS:
                    name = "ID" + str(aid) + " " + name
                anno = Arch.makeAxisSystem(axes,name)
            print(" axis")
        else:
            name = "Annotation"
            if annotation.Name:
                name = annotation.Name
                if six.PY2:
                    name = name.encode("utf8")
            if "annotation" not in name.lower():
                name = "Annotation " + name
            if PREFIX_NUMBERS: name = "ID" + str(aid) + " " + name
            shapes2d = []
            for rep in annotation.Representation.Representations:
                if rep.RepresentationIdentifier in ["Annotation","FootPrint","Axis"]:
                    sh = setRepresentation(rep,scaling)
                    if sh in FreeCAD.ActiveDocument.Objects:
                        # dirty hack: setRepresentation might return an object directly if non-shape based (texts for ex)
                        anno = sh
                    else:
                        shapes2d.extend(sh)
            if shapes2d:
                sh = Part.makeCompound(shapes2d)
                if DEBUG: print(" shape")
                anno = FreeCAD.ActiveDocument.addObject("Part::Feature",name)
                anno.Shape = sh
                p = getPlacement(annotation.ObjectPlacement,scaling)
                if p: # and annotation.is_a("IfcAnnotation"):
                    anno.Placement = p
            else:
                if DEBUG: print(" no shape")

        # placing in container if needed

        if anno:
            if aid in remaining.keys():
                remaining[aid].addObject(anno)
            else:
                for host,children in additions.items():
                    if (aid in children) and (host in objects.keys()):
                        Arch.addComponents(anno,objects[host])

    FreeCAD.ActiveDocument.recompute()

    # Materials

    if DEBUG and materials: print("Creating materials...",end="")
    #print("mattable:",mattable)
    #print("materials:",materials)
    fcmats = {}
    for material in materials:
        name = "Material"
        if material.Name:
            name = material.Name
            if six.PY2:
                name = name.encode("utf8")
        if MERGE_MATERIALS and (name in fcmats.keys()):
            mat = fcmats[name]
        else:
            mat = Arch.makeMaterial(name=name)
            mdict = {}
            if material.id() in colors:
                mdict["DiffuseColor"] = str(colors[material.id()])
            else:
                for o,m in mattable.items():
                    if m == material.id():
                        if o in colors:
                            mdict["DiffuseColor"] = str(colors[o])
            if mdict:
                mat.Material = mdict
            fcmats[name] = mat
        for o,m in mattable.items():
            if m == material.id():
                if o in objects:
                    if hasattr(objects[o],"Material"):
                        objects[o].Material = mat

    if DEBUG and materials: print("done")

    # restore links from full parametric definitions
    for p in parametrics:
        l = FreeCAD.ActiveDocument.getObject(p[2])
        if l:
            setattr(p[0],p[1],l)

    FreeCAD.ActiveDocument.recompute()

    if ZOOMOUT and FreeCAD.GuiUp:
        import FreeCADGui
        FreeCADGui.SendMsgToActiveView("ViewFit")
    print("Finished importing.")
    return doc


# ************************************************************************************************
# ********** helper for import IFC **************
class recycler:

    "the compression engine - a mechanism to reuse ifc entities if needed"

    # this object has some methods identical to corresponding ifcopenshell methods,
    # but it checks if a similar entity already exists before creating a new one
    # to compress a new type, just add the necessary method here

    def __init__(self,ifcfile):

        self.ifcfile = ifcfile
        self.compress = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch").GetBool("ifcCompress",True)
        self.cartesianpoints = {(0,0,0):self.ifcfile[8]} # from template
        self.directions = {(1,0,0):self.ifcfile[6],(0,0,1):self.ifcfile[7],(0,1,0):self.ifcfile[10]} # from template
        self.polylines = {}
        self.polyloops = {}
        self.propertysinglevalues = {}
        self.axis2placement3ds = {'(0.0, 0.0, 0.0)(0.0, 0.0, 1.0)(1.0, 0.0, 0.0)':self.ifcfile[9]} # from template
        self.axis2placement2ds = {}
        self.localplacements = {}
        self.rgbs = {}
        self.ssrenderings = {}
        self.sstyles = {}
        self.transformationoperators = {}
        self.psas = {}
        self.spared = 0

    def createIfcCartesianPoint(self,points):
        if self.compress and points in self.cartesianpoints:
            self.spared += 1
            return self.cartesianpoints[points]
        else:
            c = self.ifcfile.createIfcCartesianPoint(points)
            if self.compress:
                self.cartesianpoints[points] = c
            return c

    def createIfcDirection(self,points):
        if self.compress and points in self.directions:
            self.spared += 1
            return self.directions[points]
        else:
            c = self.ifcfile.createIfcDirection(points)
            if self.compress:
                self.directions[points] = c
            return c

    def createIfcPolyline(self,points):
        key = "".join([str(p.Coordinates) for p in points])
        if self.compress and key in self.polylines:
            self.spared += 1
            return self.polylines[key]
        else:
            c = self.ifcfile.createIfcPolyline(points)
            if self.compress:
                self.polylines[key] = c
            return c

    def createIfcPolyLoop(self,points):
        key = "".join([str(p.Coordinates) for p in points])
        if self.compress and key in self.polyloops:
            self.spared += 1
            return self.polyloops[key]
        else:
            c = self.ifcfile.createIfcPolyLoop(points)
            if self.compress:
                self.polyloops[key] = c
            return c

    def createIfcPropertySingleValue(self,name,ptype,pvalue):
        key = str(name) + str(ptype) + str(pvalue)
        if self.compress and key in self.propertysinglevalues:
            self.spared += 1
            return self.propertysinglevalues[key]
        else:
            if isinstance(pvalue,float) and pvalue < 0.000000001: # remove the exp notation that some bim apps hate
                pvalue = 0
            c = self.ifcfile.createIfcPropertySingleValue(name,None,ifcfile.create_entity(ptype,pvalue),None)
            if self.compress:
                self.propertysinglevalues[key] = c
            return c

    def createIfcAxis2Placement3D(self,p1,p2,p3):
        if p2:
            tp2 = str(p2.DirectionRatios)
        else:
            tp2 = "None"
        if p3:
            tp3 = str(p3.DirectionRatios)
        else:
            tp3 = "None"
        key = str(p1.Coordinates) + tp2 + tp3
        if self.compress and key in self.axis2placement3ds:
            self.spared += 1
            return self.axis2placement3ds[key]
        else:
            c = self.ifcfile.createIfcAxis2Placement3D(p1,p2,p3)
            if self.compress:
                self.axis2placement3ds[key] = c
            return c

    def createIfcAxis2Placement2D(self,p1,p2):
        key = str(p1.Coordinates) + str(p2.DirectionRatios)
        if self.compress and key in self.axis2placement2ds:
            self.spared += 1
            return self.axis2placement2ds[key]
        else:
            c = self.ifcfile.createIfcAxis2Placement2D(p1,p2)
            if self.compress:
                self.axis2placement2ds[key] = c
            return c

    def createIfcLocalPlacement(self,gpl):
        key = str(gpl.Location.Coordinates) + str(gpl.Axis.DirectionRatios) + str(gpl.RefDirection.DirectionRatios)
        if self.compress and key in self.localplacements:
            self.spared += 1
            return self.localplacements[key]
        else:
            c = self.ifcfile.createIfcLocalPlacement(None,gpl)
            if self.compress:
                self.localplacements[key] = c
            return c

    def createIfcColourRgb(self,r,g,b):
        key = (r,g,b)
        if self.compress and key in self.rgbs:
            self.spared += 1
            return self.rgbs[key]
        else:
            c = self.ifcfile.createIfcColourRgb(None,r,g,b)
            if self.compress:
                self.rgbs[key] = c
            return c

    def createIfcSurfaceStyleRendering(self,col,trans=0):
        key = (col.Red,col.Green,col.Blue,trans)
        if self.compress and key in self.ssrenderings:
            self.spared += 1
            return self.ssrenderings[key]
        else:
            if trans == 0:
                trans = None
            c = self.ifcfile.createIfcSurfaceStyleRendering(col,trans,None,None,None,None,None,None,"FLAT")
            if self.compress:
                self.ssrenderings[key] = c
            return c

    def createIfcCartesianTransformationOperator3D(self,axis1,axis2,origin,scale,axis3):
        key = str(axis1.DirectionRatios) + str(axis2.DirectionRatios) + str(origin.Coordinates) + str(scale) + str(axis3.DirectionRatios)
        if self.compress and key in self.transformationoperators:
            self.spared += 1
            return self.transformationoperators[key]
        else:
            c = self.ifcfile.createIfcCartesianTransformationOperator3D(axis1,axis2,origin,scale,axis3)
            if self.compress:
                self.transformationoperators[key] = c
            return c

    def createIfcSurfaceStyle(self,name,r,g,b,t=0):
        if name:
            key = name + str((r,g,b))
        else:
            key = str((r,g,b))
        if self.compress and key in self.sstyles:
            self.spared += 1
            return self.sstyles[key]
        else:
            col = self.createIfcColourRgb(r,g,b)
            ssr = self.createIfcSurfaceStyleRendering(col,t)
            c = self.ifcfile.createIfcSurfaceStyle(name,"BOTH",[ssr])
            if self.compress:
                self.sstyles[key] = c
            return c

    def createIfcPresentationStyleAssignment(self,name,r,g,b,t=0):
        if name:
            key = name+str((r,g,b,t))
        else:
            key = str((r,g,b,t))
        if self.compress and key in self.psas:
            self.spared += 1
            return self.psas[key]
        else:
            iss = self.createIfcSurfaceStyle(name,r,g,b,t)
            c = self.ifcfile.createIfcPresentationStyleAssignment([iss])
            if self.compress:
                self.psas[key] = c
            return c


# ************************************************************************************************
# ********** export IFC ****************
def export(exportList,filename):

    "exports FreeCAD contents to an IFC file"

    getPreferences()

    try:
        global ifcopenshell
        import ifcopenshell
    except:
        FreeCAD.Console.PrintError("IfcOpenShell was not found on this system. IFC support is disabled\n")
        return

    version = FreeCAD.Version()
    owner = FreeCAD.ActiveDocument.CreatedBy
    email = ''
    if ("@" in owner) and ("<" in owner):
        s = owner.split("<")
        owner = s[0].strip()
        email = s[1].strip(">")
    global template
    template = ifctemplate.replace("$version",version[0]+"."+version[1]+" build "+version[2])
    getstd = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch").GetBool("getStandardType",False)
    if hasattr(ifcopenshell,"schema_identifier"):
        schema = ifcopenshell.schema_identifier
    elif hasattr(ifcopenshell,"version") and (float(ifcopenshell.version[:3]) >= 0.6):
        # v0.6 allows to set our own schema
        schema = ["IFC4", "IFC2X3"][FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch").GetInt("IfcVersion",0)]
    else:
        schema = "IFC2X3"
    if DEBUG: print("Exporting an",schema,"file...")
    template = template.replace("$ifcschema",schema)
    template = template.replace("$owner",owner)
    template = template.replace("$company",FreeCAD.ActiveDocument.Company)
    template = template.replace("$email",email)
    template = template.replace("$now",str(int(time.time())))
    template = template.replace("$projectid",FreeCAD.ActiveDocument.Uid[:22].replace("-","_"))
    template = template.replace("$project",FreeCAD.ActiveDocument.Name)
    template = template.replace("$filename",os.path.basename(filename))
    template = template.replace("$timestamp",str(time.strftime("%Y-%m-%dT%H:%M:%S", time.gmtime())))
    if hasattr(ifcopenshell,"version"):
        template = template.replace("IfcOpenShell","IfcOpenShell "+ifcopenshell.version)
    templatefilehandle,templatefile = tempfile.mkstemp(suffix=".ifc")
    of = pyopen(templatefile,"w")
    if six.PY2:
        template = template.encode("utf8")
    of.write(template)
    of.close()
    os.close(templatefilehandle)
    global ifcfile, surfstyles, clones, sharedobjects, profiledefs, shapedefs
    ifcfile = ifcopenshell.open(templatefile)
    history = ifcfile.by_type("IfcOwnerHistory")[0]
    context = ifcfile.by_type("IfcGeometricRepresentationContext")[0]
    project = ifcfile.by_type("IfcProject")[0]
    objectslist = Draft.getGroupContents(exportList,walls=True,addgroups=True)
    annotations = []
    for obj in objectslist:
        if obj.isDerivedFrom("Part::Part2DObject"):
            annotations.append(obj)
        elif obj.isDerivedFrom("App::Annotation") or (Draft.getType(obj) == "DraftText"):
            annotations.append(obj)
        elif obj.isDerivedFrom("Part::Feature"):
            if obj.Shape:
                if obj.Shape.Edges and (not obj.Shape.Faces):
                    annotations.append(obj)
    # clean objects list of unwanted types
    objectslist = [obj for obj in objectslist if obj not in annotations]
    objectslist = Arch.pruneIncluded(objectslist,strict=True)
    objectslist = [obj for obj in objectslist if Draft.getType(obj) not in ["Material","MaterialContainer","WorkingPlaneProxy"]]
    if FULL_PARAMETRIC:
        objectslist = Arch.getAllChildren(objectslist)
    products = {} # { Name: IfcEntity, ... }
    subproducts = {} # { Name: IfcEntity, ... } for storing additions/subtractions and other types of subcomponents of a product
    surfstyles = {} # { (r,g,b): IfcEntity, ... }
    clones = {} # { Basename:[Clonename1,Clonename2,...] }
    sharedobjects = {} # { BaseName: IfcRepresentationMap }
    count = 1
    groups = {} # { Host: [Child,Child,...] }
    profiledefs = {} # { ProfileDefString:profiledef,...}
    shapedefs = {} # { ShapeDefString:[shapes],... }
    spatialelements = {} # {Name:IfcEntity, ... }

    # reusable entity system

    global ifcbin
    ifcbin = recycler(ifcfile)

    # build clones table

    if CREATE_CLONES:
        for o in objectslist:
            b = Draft.getCloneBase(o,strict=True)
            if b:
                clones.setdefault(b.Name,[]).append(o.Name)

    #print("clones table: ",clones)
    #print(objectslist)

    # testing if more than one site selected (forbidden in IFC)

    if len(Draft.getObjectsOfType(objectslist,"Site")) > 1:
        FreeCAD.Console.PrintError("More than one site is selected, which is forbidden by IFC standards. Please export only one site by IFC file.\n")
        return

    # products

    for obj in objectslist:

        # getting generic data

        name = obj.Label
        if six.PY2:
            name = name.encode("utf8")
        description = obj.Description if hasattr(obj,"Description") else ""
        if six.PY2:
            description = description.encode("utf8")

        # getting uid

        uid = None
        if hasattr(obj,"IfcData"):
            if "IfcUID" in obj.IfcData.keys():
                uid = str(obj.IfcData["IfcUID"])
        if not uid:
            uid = ifcopenshell.guid.compress(uuid.uuid1().hex)
            # storing the uid for further use
            if STORE_UID and hasattr(obj,"IfcData"):
                d = obj.IfcData
                d["IfcUID"] = uid
                obj.IfcData = d

        ifctype = getIfcTypeFromObj(obj)

        if ifctype == "IfcGroup":
            groups[obj.Name] = [o.Name for o in obj.Group]
            continue

        # export grids

        if ifctype in ["IfcAxis","IfcAxisSystem","IfcGrid"]:
            ifcaxes = []
            ifcpols = []
            if ifctype == "IfcAxis":
                # make sure this axis is not included in something else already
                standalone = True
                for p in obj.InList:
                    if hasattr(p,"Axes") and (obj in p.Axes):
                        if p in objectslist:
                            axgroups = []
                            standalone = False
                            break
                if standalone:
                    axgroups = [obj.Proxy.getAxisData(obj)]
            else:
                axgroups = obj.Proxy.getAxisData(obj)
            if not axgroups:
                continue
            ifctype = "IfcGrid"
            for axg in axgroups:
                ifcaxg = []
                for ax in axg:
                    p1 = ifcbin.createIfcCartesianPoint(tuple(FreeCAD.Vector(ax[0]).multiply(0.001)))
                    p2 = ifcbin.createIfcCartesianPoint(tuple(FreeCAD.Vector(ax[1]).multiply(0.001)))
                    pol = ifcbin.createIfcPolyline([p1,p2])
                    ifcpols.append(pol)
                    axis = ifcfile.createIfcGridAxis(ax[2],pol,True)
                    ifcaxg.append(axis)
                if len(ifcaxes) < 3:
                    ifcaxes.append(ifcaxg)
                else:
                    ifcaxes[2] = ifcaxes[2]+ifcaxg # IfcGrid can have max 3 axes systems
            u = None
            v = None
            w = None
            if ifcaxes:
                u = ifcaxes[0]
            if len(ifcaxes) > 1:
                v = ifcaxes[1]
            if len(ifcaxes) > 2:
                w = ifcaxes[2]
            if u and v:
                if DEBUG: print(str(count).ljust(3)," : ", ifctype, " (",str(len(ifcpols)),"axes ) : ",name)
                xvc =  ifcbin.createIfcDirection((1.0,0.0,0.0))
                zvc =  ifcbin.createIfcDirection((0.0,0.0,1.0))
                ovc =  ifcbin.createIfcCartesianPoint((0.0,0.0,0.0))
                gpl =  ifcbin.createIfcAxis2Placement3D(ovc,zvc,xvc)
                plac = ifcbin.createIfcLocalPlacement(gpl)
                cset = ifcfile.createIfcGeometricCurveSet(ifcpols)
                #subc = ifcfile.createIfcGeometricRepresentationSubContext('FootPrint','Model',context,None,"MODEL_VIEW",None,None,None,None,None)
                srep = ifcfile.createIfcShapeRepresentation(context,'FootPrint',"GeometricCurveSet",ifcpols)
                pdef = ifcfile.createIfcProductDefinitionShape(None,None,[srep])
                grid = ifcfile.createIfcGrid(uid,history,name,description,None,plac,pdef,u,v,w)
                products[obj.Name] = grid
                count += 1
            continue

        if ifctype not in ArchIFCSchema.IfcProducts.keys():
            ifctype = "IfcBuildingElementProxy"

        # getting the "Force BREP" flag

        brepflag = False
        if hasattr(obj,"IfcData"):
            if "FlagForceBrep" in obj.IfcData.keys():
                if obj.IfcData["FlagForceBrep"] == "True":
                    brepflag = True

        # getting the representation

        representation,placement,shapetype = getRepresentation(ifcfile,context,obj,forcebrep=(brepflag or FORCE_BREP))
        if getstd:
            if isStandardCase(obj,ifctype):
                ifctype += "StandardCase"

        if DEBUG: print(str(count).ljust(3)," : ", ifctype, " (",shapetype,") : ",name)

        # setting the arguments

        kwargs = {
            "GlobalId": uid,
            "OwnerHistory": history,
            "Name": name,
            "Description": description,
            "ObjectPlacement": placement,
            "Representation": representation
        }
        if ifctype == "IfcSite":
            kwargs.update({
                "RefLatitude":dd2dms(obj.Latitude),
                "RefLongitude":dd2dms(obj.Longitude),
                "RefElevation":obj.Elevation.Value/1000.0,
                "SiteAddress":buildAddress(obj,ifcfile),
                "CompositionType": "ELEMENT"
            })
        if schema == "IFC2X3":
            kwargs = exportIFC2X3Attributes(obj, kwargs)
        else:
            kwargs = exportIfcAttributes(obj, kwargs)

        # creating the product

        #print(obj.Label," : ",ifctype," : ",kwargs)
        product = getattr(ifcfile,"create"+ifctype)(**kwargs)
        products[obj.Name] = product
        if ifctype in ["IfcBuilding","IfcBuildingStorey","IfcSite","IfcSpace"]:
            spatialelements[obj.Name] = product

        # additions

        if hasattr(obj,"Additions") and (shapetype in ["extrusion","no shape"]):
            for o in obj.Additions:
                r2,p2,c2 = getRepresentation(ifcfile,context,o)
                if DEBUG: print("      adding ",c2," : ",o.Label)
                l = o.Label
                if six.PY2:
                    l = l.encode("utf8")
                prod2 = ifcfile.createIfcBuildingElementProxy(
                    ifcopenshell.guid.compress(uuid.uuid1().hex),
                    history,
                    l,
                    None,
                    None,
                    p2,
                    r2,
                    None,
                    "ELEMENT"
                )
                subproducts[o.Name] = prod2
                ifcfile.createIfcRelAggregates(
                    ifcopenshell.guid.compress(uuid.uuid1().hex),
                    history,
                    'Addition',
                    '',
                    product,
                    [prod2]
                )

        # subtractions

        guests = []
        for o in obj.InList:
            if hasattr(o,"Hosts"):
                for co in o.Hosts:
                    if co == obj:
                        if o not in guests:
                            guests.append(o)
        if hasattr(obj,"Subtractions") and (shapetype in ["extrusion","no shape"]):
            for o in obj.Subtractions + guests:
                r2,p2,c2 = getRepresentation(ifcfile,context,o,subtraction=True)
                if DEBUG: print("      subtracting ",c2," : ",o.Label)
                l = o.Label
                if six.PY2:
                    l = l.encode("utf8")
                prod2 = ifcfile.createIfcOpeningElement(
                    ifcopenshell.guid.compress(uuid.uuid1().hex),
                    history,
                    l,
                    None,
                    None,
                    p2,
                    r2,
                    None
                )
                subproducts[o.Name] = prod2
                ifcfile.createIfcRelVoidsElement(
                    ifcopenshell.guid.compress(uuid.uuid1().hex),
                    history,
                    'Subtraction',
                    '',
                    product,
                    prod2
                )

        # properties

        ifcprop = False
        if hasattr(obj,"IfcProperties"):

            if obj.IfcProperties:

                ifcprop = True

                if isinstance(obj.IfcProperties,dict):

                    # IfcProperties is a dictionary (FreeCAD 0.18)

                    psets = {}
                    for key,value in obj.IfcProperties.items():

                            # in 0.18, properties in IfcProperties dict are stored as "key":"pset;;type;;value" or "key":"type;;value"
                            # in 0.19, key = name;;pset, value = ptype;;value (because there can be several props with same name)

                            pset = None
                            pname = key
                            if ";;" in pname:
                                pname = key.split(";;")[0]
                                pset = key.split(";;")[-1]
                            value = value.split(";;")
                            if len(value) == 3:
                                pset = value[0]
                                ptype = value[1]
                                pvalue = value[2]
                            elif len(value) == 2:
                                if not pset:
                                    pset = "Default property set"
                                ptype = value[0]
                                pvalue = value[1]
                            else:
                                if DEBUG:print("      unable to export property:",pname,value)
                                continue

                            #if DEBUG: print("      property ",pname," : ",pvalue.encode("utf8"), " (", str(ptype), ") in ",pset)
                            if ptype in ["IfcLabel","IfcText","IfcIdentifier",'IfcDescriptiveMeasure']:
                                if six.PY2:
                                    pvalue = pvalue.encode("utf8")
                            elif ptype == "IfcBoolean":
                                if pvalue == ".T.":
                                    pvalue = True
                                else:
                                    pvalue = False
                            elif ptype == "IfcLogical":
                                if pvalue.upper() == "TRUE":
                                    pvalue = True
                                else:
                                    pvalue = False
                            elif ptype == "IfcInteger":
                                pvalue = int(pvalue)
                            else:
                                try:
                                    pvalue = float(pvalue)
                                except:
                                    try:
                                        pvalue = FreeCAD.Units.Quantity(pvalue).Value
                                    except:
                                        if six.PY2:
                                            pvalue = pvalue.encode("utf8")
                                        if DEBUG:print("      warning: unable to export property as numeric value:",pname,pvalue)
                            p = ifcbin.createIfcPropertySingleValue(str(pname),str(ptype),pvalue)
                            psets.setdefault(pset,[]).append(p)
                    for pname,props in psets.items():
                        pset = ifcfile.createIfcPropertySet(
                            ifcopenshell.guid.compress(uuid.uuid1().hex),
                            history,
                            pname,
                            None,
                            props
                        )
                        ifcfile.createIfcRelDefinesByProperties(
                            ifcopenshell.guid.compress(uuid.uuid1().hex),
                            history,
                            None,
                            None,
                            [product],
                            pset
                        )

                elif obj.IfcProperties.TypeId == 'Spreadsheet::Sheet':

                    # IfcProperties is a spreadsheet (deprecated)

                    sheet = obj.IfcProperties
                    propertiesDic = {}
                    categories = []
                    n=2
                    cell = True
                    while cell is True:
                        if hasattr(sheet,'A'+str(n)):
                            cat = sheet.get('A'+str(n))
                            key = sheet.get('B'+str(n))
                            tp = sheet.get('C'+str(n))
                            if hasattr(sheet,'D'+str(n)):
                                val = sheet.get('D'+str(n))
                            else:
                                val = ''
                            if six.PY2 and isinstance(key, six.text_type):
                                key = key.encode("utf8")
                            else:
                                key = str(key)
                            if six.PY2 and isinstance(tp, six.text_type):
                                tp = tp.encode("utf8")
                            else:
                                tp = str(tp)
                            #tp = tp.encode("utf8")
                            if tp in ["IfcLabel","IfcText","IfcIdentifier",'IfcDescriptiveMeasure']:
                                val = val.encode("utf8")
                            elif tp == "IfcBoolean":
                                if val == 'True':
                                    val = True
                                else:
                                    val = False
                            elif tp == "IfcInteger":
                                val = int(val)
                            else:
                                val = float(val)
                            unit = None
                            #unit = sheet.get('E'+str(n))
                            if cat in categories:
                                propertiesDic[cat].append({"key":key,"tp":tp,"val":val,"unit":unit})
                            else:
                                propertiesDic[cat] = [{"key":key,"tp":tp,"val":val,"unit":unit}]
                                categories.append(cat)
                            n += 1
                        else:
                            cell = False
                    for cat in propertiesDic:
                        props = []
                        for prop in propertiesDic[cat]:
                            if DEBUG:
                                print("key",prop["key"],type(prop["key"]))
                                print("tp",prop["tp"],type(prop["tp"]))
                                print("val",prop["val"],type(prop["val"]))
                            if tp.lower().startswith("ifc"):
                                props.append(ifcbin.createIfcPropertySingleValue(prop["key"],prop["tp"],prop["val"]))
                            else:
                                print("Unable to create a property of type:",tp)
                        if props:
                            pset = ifcfile.createIfcPropertySet(
                                ifcopenshell.guid.compress(uuid.uuid1().hex),
                                history,cat,
                                None,
                                props
                            )
                            ifcfile.createIfcRelDefinesByProperties(
                                ifcopenshell.guid.compress(uuid.uuid1().hex),
                                history,
                                None,
                                None,
                                [product],
                                pset
                            )

        if hasattr(obj,"IfcData"):

            if obj.IfcData:
                ifcprop = True
                #if DEBUG : print("      adding ifc attributes")
                props = []
                for key in obj.IfcData:
                    if not (key in ["attributes","IfcUID","FlagForceBrep"]):

                        # (deprecated) properties in IfcData dict are stored as "key":"type(value)"

                        r = obj.IfcData[key].strip(")").split("(")
                        if len(r) == 1:
                            tp = "IfcText"
                            val = r[0]
                        else:
                            tp = r[0]
                            val = "(".join(r[1:])
                            val = val.strip("'")
                            val = val.strip('"')
                            #if DEBUG: print("      property ",key," : ",val.encode("utf8"), " (", str(tp), ")")
                            if tp in ["IfcLabel","IfcText","IfcIdentifier",'IfcDescriptiveMeasure']:
                                if six.PY2:
                                    val = val.encode("utf8")
                            elif tp == "IfcBoolean":
                                if val == ".T.":
                                    val = True
                                else:
                                    val = False
                            elif tp == "IfcInteger":
                                val = int(val)
                            else:
                                val = float(val)
                        props.append(ifcbin.createIfcPropertySingleValue(str(key),str(tp),val))
                if props:
                    pset = ifcfile.createIfcPropertySet(
                        ifcopenshell.guid.compress(uuid.uuid1().hex),
                        history,
                        'PropertySet',
                        None,
                        props
                    )
                    ifcfile.createIfcRelDefinesByProperties(
                        ifcopenshell.guid.compress(uuid.uuid1().hex),
                        history,
                        None,
                        None,
                        [product],
                        pset
                    )

        if not ifcprop:
            #if DEBUG : print("no ifc properties to export")
            pass

        # Quantities

        if hasattr(obj,"IfcData"):
            quantities = []
            if ("ExportHeight" in obj.IfcData) and obj.IfcData["ExportHeight"] and hasattr(obj,"Height"):
                quantities.append(ifcfile.createIfcQuantityLength('Height',None,None,obj.Height.Value/1000.0))
            if ("ExportWidth" in obj.IfcData) and obj.IfcData["ExportWidth"] and hasattr(obj,"Width"):
                quantities.append(ifcfile.createIfcQuantityLength('Width',None,None,obj.Width.Value/1000.0))
            if ("ExportLength" in obj.IfcData) and obj.IfcData["ExportLength"] and hasattr(obj,"Length"):
                quantities.append(ifcfile.createIfcQuantityLength('Length',None,None,obj.Length.Value/1000.0))
            if ("ExportHorizontalArea" in obj.IfcData) and obj.IfcData["ExportHorizontalArea"] and hasattr(obj,"HorizontalArea"):
                quantities.append(ifcfile.createIfcQuantityArea('HorizontalArea',None,None,obj.HorizontalArea.Value/1000000.0))
            if ("ExportVerticalArea" in obj.IfcData) and obj.IfcData["ExportVerticalArea"] and hasattr(obj,"VerticalArea"):
                quantities.append(ifcfile.createIfcQuantityArea('VerticalArea',None,None,obj.VerticalArea.Value/1000000.0))
            if ("ExportVolume" in obj.IfcData) and obj.IfcData["ExportVolume"] and obj.isDerivedFrom("Part::Feature"):
                quantities.append(ifcfile.createIfcQuantityVolume('Volume',None,None,obj.Shape.Volume/1000000000.0))
            if quantities:
                eltq = ifcfile.createIfcElementQuantity(
                    ifcopenshell.guid.compress(uuid.uuid1().hex),
                    history,
                    "ElementQuantities",
                    None,
                    "FreeCAD",quantities
                )
                ifcfile.createIfcRelDefinesByProperties(
                    ifcopenshell.guid.compress(uuid.uuid1().hex),
                    history,
                    None,
                    None,
                    [product],eltq
                )

        if FULL_PARAMETRIC:

            # exporting all the object properties

            FreeCADProps = []
            FreeCADGuiProps = []
            FreeCADProps.append(ifcbin.createIfcPropertySingleValue("FreeCADType","IfcText",obj.TypeId))
            FreeCADProps.append(ifcbin.createIfcPropertySingleValue("FreeCADName","IfcText",obj.Name))
            sets = [("App",obj)]
            if hasattr(obj,"Proxy"):
                if obj.Proxy:
                    FreeCADProps.append(ifcbin.createIfcPropertySingleValue("FreeCADAppObject","IfcText",str(obj.Proxy.__class__)))
            if FreeCAD.GuiUp:
                if obj.ViewObject:
                    sets.append(("Gui",obj.ViewObject))
                    if hasattr(obj.ViewObject,"Proxy"):
                        if obj.ViewObject.Proxy:
                            FreeCADGuiProps.append(
                                ifcbin.createIfcPropertySingleValue(
                                    "FreeCADGuiObject",
                                    "IfcText",
                                    str(obj.ViewObject.Proxy.__class__)
                                )
                            )
            for realm,ctx in sets:
                if ctx:
                    for prop in ctx.PropertiesList:
                        if not(prop in ["IfcProperties","IfcData","Shape","Proxy","ExpressionEngine","AngularDeflection","BoundingBox"]):
                            try:
                                ptype = ctx.getTypeIdOfProperty(prop)
                            except AttributeError:
                                ptype = "Unknown"
                            itype = None
                            ivalue = None
                            if ptype in ["App::PropertyString","App::PropertyEnumeration"]:
                                itype = "IfcText"
                                ivalue = getattr(ctx,prop)
                            elif ptype == "App::PropertyInteger":
                                itype = "IfcInteger"
                                ivalue = getattr(ctx,prop)
                            elif ptype == "App::PropertyFloat":
                                itype = "IfcReal"
                                ivalue = float(getattr(ctx,prop))
                            elif ptype == "App::PropertyBool":
                                itype = "IfcBoolean"
                                ivalue = getattr(ctx,prop)
                            elif ptype in ["App::PropertyVector","App::PropertyPlacement"]:
                                itype = "IfcText"
                                ivalue = str(getattr(ctx,prop))
                            elif ptype in ["App::PropertyLength","App::PropertyDistance"]:
                                itype = "IfcReal"
                                ivalue = float(getattr(ctx,prop).getValueAs("m"))
                            elif ptype == "App::PropertyArea":
                                itype = "IfcReal"
                                ivalue = float(getattr(ctx,prop).getValueAs("m^2"))
                            elif ptype == "App::PropertyLink":
                                t = getattr(ctx,prop)
                                if t:
                                    itype = "IfcText"
                                    ivalue = "FreeCADLink_" + t.Name
                            else:
                                if DEBUG: print("Unable to encode property ",prop," of type ",ptype)
                            if itype:
                                # TODO add description
                                if realm == "Gui":
                                    FreeCADGuiProps.append(ifcbin.createIfcPropertySingleValue("FreeCADGui_"+prop,itype,ivalue))
                                else:
                                    FreeCADProps.append(ifcbin.createIfcPropertySingleValue("FreeCAD_"+prop,itype,ivalue))
            if FreeCADProps:
                pset = ifcfile.createIfcPropertySet(
                    ifcopenshell.guid.compress(uuid.uuid1().hex),
                    history,'FreeCADPropertySet',
                    None,
                    FreeCADProps
                )
                ifcfile.createIfcRelDefinesByProperties(
                    ifcopenshell.guid.compress(uuid.uuid1().hex),
                    history,
                    None,
                    None,
                    [product],
                    pset
                )
            if FreeCADGuiProps:
                pset = ifcfile.createIfcPropertySet(
                    ifcopenshell.guid.compress(uuid.uuid1().hex),
                    history,
                    'FreeCADGuiPropertySet',
                    None,
                    FreeCADGuiProps
                )
                ifcfile.createIfcRelDefinesByProperties(
                    ifcopenshell.guid.compress(uuid.uuid1().hex),
                    history,
                    None,
                    None,
                    [product],
                    pset
                )

        count += 1

    # relationships

    sites = []
    buildings = []
    floors = []
    treated = []
    defaulthost = []

    # buildingParts can be exported as any "normal" IFC type. In that case, gather their elements first

    for bp in Draft.getObjectsOfType(objectslist,"BuildingPart"):
        if bp.IfcType not in ["Site","Building","Building Storey","Space","Undefined"]:
            if bp.Name in products:
                subs = []
                for c in bp.Group:
                    if c.Name in products:
                        subs.append(products[c.Name])
                        treated.append(c.Name)
                if subs:
                    ifcfile.createIfcRelAggregates(
                        ifcopenshell.guid.compress(uuid.uuid1().hex),
                        history,
                        'Assembly',
                        '',
                        products[bp.Name],
                        subs
                    )

    # floors/buildingparts

    for floor in Draft.getObjectsOfType(objectslist,"Floor")+Draft.getObjectsOfType(objectslist,"BuildingPart"):
        if (Draft.getType(floor) == "Floor") or (hasattr(floor,"IfcType") and floor.IfcType == "Building Storey"):
            objs = Draft.getGroupContents(floor,walls=True,addgroups=True)
            objs = Arch.pruneIncluded(objs)
            children = []
            for c in objs:
                if c.Name != floor.Name: # getGroupContents + addgroups will include the floor itself
                    if c.Name in products.keys():
                        if not (c.Name in treated):
                            children.append(products[c.Name])
                            treated.append(c.Name)
            f = products[floor.Name]
            if children:
                ifcfile.createIfcRelContainedInSpatialStructure(
                    ifcopenshell.guid.compress(uuid.uuid1().hex),
                    history,
                    'StoreyLink',
                    '',
                    children,
                    f
                )
            floors.append(f)
            defaulthost = f

    # buildings

    for building in Draft.getObjectsOfType(objectslist,"Building")+Draft.getObjectsOfType(objectslist,"BuildingPart"):
        if (Draft.getType(building) == "Building") or (hasattr(building,"IfcType") and building.IfcType == "Building"):
            objs = Draft.getGroupContents(building,walls=True,addgroups=True)
            objs = Arch.pruneIncluded(objs)
            children = []
            childfloors = []
            for c in objs:
                if not (c.Name in treated):
                    if c.Name != building.Name: # getGroupContents + addgroups will include the building itself
                        if c.Name in products.keys():
                            if Draft.getType(c) in ["Floor","BuildingPart"]:
                                childfloors.append(products[c.Name])
                                treated.append(c.Name)
                            elif not (c.Name in treated):
                                children.append(products[c.Name])
                                treated.append(c.Name)
            b = products[building.Name]
            if children:
                ifcfile.createIfcRelContainedInSpatialStructure(
                    ifcopenshell.guid.compress(uuid.uuid1().hex),
                    history,
                    'BuildingLink',
                    '',
                    children,
                    b
                )
            if childfloors:
                ifcfile.createIfcRelAggregates(
                    ifcopenshell.guid.compress(uuid.uuid1().hex),
                    history,
                    'BuildingLink',
                    '',
                    b,
                    childfloors
                )
            buildings.append(b)
            if not defaulthost and not ADDDEFAULTSTOREY:
                defaulthost = b

    # sites

    for site in Draft.getObjectsOfType(objectslist,"Site"):
        objs = Draft.getGroupContents(site,walls=True,addgroups=True)
        objs = Arch.pruneIncluded(objs)
        children = []
        childbuildings = []
        for c in objs:
            if c.Name != site.Name: # getGroupContents + addgroups will include the building itself
                if c.Name in products.keys():
                    if not (c.Name in treated):
                        if Draft.getType(c) == "Building":
                            childbuildings.append(products[c.Name])
                            treated.append(c.Name)
        sites.append(products[site.Name])

    if not sites:
        if DEBUG: print("No site found. Adding default site")
        sites = [ifcfile.createIfcSite(
            ifcopenshell.guid.compress(uuid.uuid1().hex),
            history,"Default Site",
            '',
            None,
            None,
            None,
            None,
            "ELEMENT",
            None,
            None,
            None,
            None,
            None
        )]
    ifcfile.createIfcRelAggregates(
        ifcopenshell.guid.compress(uuid.uuid1().hex),
        history,
        'ProjectLink',
        '',
        project,sites
    )
    if not buildings:
        if DEBUG: print("No building found. Adding default building")
        buildings = [ifcfile.createIfcBuilding(
            ifcopenshell.guid.compress(uuid.uuid1().hex),
            history,
            "Default Building",
            '',
            None,
            None,
            None,
            None,
            "ELEMENT",
            None,
            None,
            None
        )]
        if floors:
            ifcfile.createIfcRelAggregates(
                ifcopenshell.guid.compress(uuid.uuid1().hex),
                history,
                'BuildingLink',
                '',
                buildings[0],floors
            )
    ifcfile.createIfcRelAggregates(
        ifcopenshell.guid.compress(uuid.uuid1().hex),
        history,
        'SiteLink',
        '',
        sites[0],
        buildings
    )
    untreated = []
    for k,v in products.items():
        if not(k in treated):
            if k != buildings[0].Name:
                if not(Draft.getType(FreeCAD.ActiveDocument.getObject(k)) in ["Site","Building","Floor","BuildingPart"]):
                    untreated.append(v)
                elif Draft.getType(FreeCAD.ActiveDocument.getObject(k)) == "BuildingPart":
                    if not(FreeCAD.ActiveDocument.getObject(k).IfcType in ["Building","Building Storey","Site","Space","Undefined"]):
                        untreated.append(v)
    if untreated:
        if not defaulthost:
            defaulthost = ifcfile.createIfcBuildingStorey(
                ifcopenshell.guid.compress(uuid.uuid1().hex),
                history,
                "Default Storey",
                '',
                None,
                None,
                None,
                None,
                "ELEMENT",
                None
            )
            ifcfile.createIfcRelAggregates(
                ifcopenshell.guid.compress(uuid.uuid1().hex),
                history,
                'DefaultStoreyLink',
                '',
                buildings[0],
                [defaulthost]
            )
        ifcfile.createIfcRelContainedInSpatialStructure(
            ifcopenshell.guid.compress(uuid.uuid1().hex),
            history,
            'UnassignedObjectsLink',
            '',
            untreated,
            defaulthost
        )

    # materials

    materials = {}
    for m in Arch.getDocumentMaterials():
        relobjs = []
        for o in m.InList:
            if hasattr(o,"Material"):
                if o.Material:
                    if o.Material.isDerivedFrom("App::MaterialObject"):
                        # TODO : support multimaterials too
                        if o.Material.Name == m.Name:
                            if o.Name in products:
                                relobjs.append(products[o.Name])
                            elif o.Name in subproducts:
                                relobjs.append(subproducts[o.Name])
        if relobjs:
            l = m.Label
            if six.PY2:
                l = l.encode("utf8")
            mat = ifcfile.createIfcMaterial(l)
            materials[m.Label] = mat
            rgb = None
            if hasattr(m,"Color"):
                rgb = m.Color[:3]
            else:
                for colorslot in ["Color","DiffuseColor","ViewColor"]:
                    if colorslot in m.Material:
                        if m.Material[colorslot]:
                            if m.Material[colorslot][0] == "(":
                                rgb = tuple([float(f) for f in m.Material[colorslot].strip("()").split(",")])
                                break
            if rgb:
                psa = ifcbin.createIfcPresentationStyleAssignment(l,rgb[0],rgb[1],rgb[2])
                isi = ifcfile.createIfcStyledItem(None,[psa],None)
                isr = ifcfile.createIfcStyledRepresentation(context,"Style","Material",[isi])
                imd = ifcfile.createIfcMaterialDefinitionRepresentation(None,None,[isr],mat)
            ifcfile.createIfcRelAssociatesMaterial(
                ifcopenshell.guid.compress(uuid.uuid1().hex),
                history,
                'MaterialLink',
                '',
                relobjs,
                mat
            )

    # 2D objects

    annos = {}
    if EXPORT_2D:
        curvestyles = {}
        if annotations and DEBUG: print("exporting 2D objects...")
        for anno in annotations:
            xvc = ifcbin.createIfcDirection((1.0,0.0,0.0))
            zvc = ifcbin.createIfcDirection((0.0,0.0,1.0))
            ovc = ifcbin.createIfcCartesianPoint((0.0,0.0,0.0))
            gpl = ifcbin.createIfcAxis2Placement3D(ovc,zvc,xvc)
            if anno.isDerivedFrom("Part::Feature"):
                reps = []
                sh = anno.Shape.copy()
                sh.scale(0.001) # to meters
                ehc = []
                curves = []
                for w in sh.Wires:
                    curves.append(createCurve(ifcfile,w))
                    for e in w.Edges:
                        ehc.append(e.hashCode())
                if curves:
                    reps.append(ifcfile.createIfcGeometricCurveSet(curves))
                curves = []
                for e in sh.Edges:
                    if e.hashCode not in ehc:
                        curves.append(createCurve(ifcfile,e))
                if curves:
                    reps.append(ifcfile.createIfcGeometricCurveSet(curves))
            elif anno.isDerivedFrom("App::Annotation"):
                l = FreeCAD.Vector(anno.Position).multiply(0.001)
                pos = ifcbin.createIfcCartesianPoint((l.x,l.y,l.z))
                tpl = ifcbin.createIfcAxis2Placement3D(pos,None,None)
                s = ";".join(anno.LabelText)
                if six.PY2:
                    s = s.encode("utf8")
                txt = ifcfile.createIfcTextLiteral(s,tpl,"LEFT")
                reps = [txt]
            elif Draft.getType(anno) == "DraftText":
                l = FreeCAD.Vector(anno.Placement.Base).multiply(0.001)
                pos = ifcbin.createIfcCartesianPoint((l.x,l.y,l.z))
                tpl = ifcbin.createIfcAxis2Placement3D(pos,None,None)
                s = ";".join(anno.Text)
                if six.PY2:
                    s = s.encode("utf8")
                txt = ifcfile.createIfcTextLiteral(s,tpl,"LEFT")
                reps = [txt]
            else:
                print("Unable to handle object",anno.Label)
                continue

            for coldef in ["LineColor","TextColor","ShapeColor"]:
                if hasattr(obj.ViewObject,coldef):
                    rgb = getattr(obj.ViewObject,coldef)[:3]
                    if rgb in curvestyles:
                        psa = curvestyles[rgb]
                    else:
                        col = ifcbin.createIfcColourRgb(rgb[0],rgb[1],rgb[2])
                        cvf = ifcfile.createIfcDraughtingPredefinedCurveFont("CONTINUOUS")
                        ics = ifcfile.createIfcCurveStyle('Line',cvf,None,col)
                        psa = ifcfile.createIfcPresentationStyleAssignment([ics])
                        curvestyles[rgb] = psa
                    for rep in reps:
                        isi = ifcfile.createIfcStyledItem(rep,[psa],None)
                    break

            shp = ifcfile.createIfcShapeRepresentation(context,'Annotation','Annotation2D',reps)
            rep = ifcfile.createIfcProductDefinitionShape(None,None,[shp])
            l = anno.Label
            if six.PY2:
                l = l.encode("utf8")
            ann = ifcfile.createIfcAnnotation(
                ifcopenshell.guid.compress(uuid.uuid1().hex),
                history,l,
                '',
                None,
                gpl,
                rep
            )
            annos[anno.Name] = ann

    # groups

    sortedgroups = []
    swallowed = []
    while groups:
        for g in groups.keys():
            okay = True
            for c in groups[g]:
                if Draft.getType(FreeCAD.ActiveDocument.getObject(c)) in ["Group","VisGroup"]:
                    okay = False
                    for s in sortedgroups:
                        if s[0] == c:
                            okay = True
            if okay:
                sortedgroups.append([g,groups[g]])
        for g in sortedgroups:
            if g[0] in groups.keys():
                del groups[g[0]]
    #print("sorted groups:",sortedgroups)
    containers = {}
    for g in sortedgroups:
        if g[1]:
            children = []
            for o in g[1]:
                if o in products.keys():
                    children.append(products[o])
                elif o in annos.keys():
                    children.append(annos[o])
                    swallowed.append(annos[o])
            if children:
                name = FreeCAD.ActiveDocument.getObject(g[0]).Label
                if six.PY2:
                    name = name.encode("utf8")
                grp = ifcfile.createIfcGroup(
                    ifcopenshell.guid.compress(uuid.uuid1().hex),
                    history,
                    name,
                    '',
                    None
                )
                products[g[0]] = grp
                spatialelements[g[0]] = grp
                ass = ifcfile.createIfcRelAssignsToGroup(
                    ifcopenshell.guid.compress(uuid.uuid1().hex),
                    history,
                    'GroupLink',
                    '',
                    children,
                    None,
                    grp
                )

    # stack groups inside containers

    stack = {}
    for g in sortedgroups:
        go = FreeCAD.ActiveDocument.getObject(g[0])
        for parent in go.InList:
            if hasattr(parent,"Group") and (go in parent.Group):
                if (parent.Name in spatialelements) and (g[0] in spatialelements):
                    stack.setdefault(parent.Name,[]).append(spatialelements[g[0]])
    for k,v in stack.items():
        ifcfile.createIfcRelAggregates(
            ifcopenshell.guid.compress(uuid.uuid1().hex),
            history,
            'GroupStackLink',
            '',
            spatialelements[k],
            v
        )

    # add remaining 2D objects to default host

    if annos:
        remaining = [anno for anno in annos.values() if anno not in swallowed]
        if remaining:
            if not defaulthost:
                defaulthost = ifcfile.createIfcBuildingStorey(
                    ifcopenshell.guid.compress(uuid.uuid1().hex),
                    history,
                    "Default Storey",
                    '',
                    None,
                    None,
                    None,
                    None,
                    "ELEMENT",
                    None
                )
                ifcfile.createIfcRelAggregates(
                    ifcopenshell.guid.compress(uuid.uuid1().hex),
                    history,
                    'DefaultStoreyLink',
                    '',
                    buildings[0],
                    [defaulthost]
                )
            ifcfile.createIfcRelContainedInSpatialStructure(
                ifcopenshell.guid.compress(uuid.uuid1().hex),
                history,
                'AnnotationsLink',
                '',
                remaining,
                defaulthost
            )

    if DEBUG: print("writing ",filename,"...")

    filename = decode(filename)

    ifcfile.write(filename)

    if STORE_UID:
        # some properties might have been changed
        FreeCAD.ActiveDocument.recompute()

    os.remove(templatefile)

    if DEBUG and ifcbin.compress:
        f = pyopen(filename,"r")
        s = len(f.read().split("\n"))
        f.close()
        print("Compression ratio:",int((float(ifcbin.spared)/(s+ifcbin.spared))*100),"%")
    del ifcbin


# ************************************************************************************************
# ********** helper for export IFC **************
def isStandardCase(obj,ifctype):

    if ifctype.endswith("StandardCase"):
        return False # type is already standard case, return False so "StandardCase" is not added twice
    if hasattr(obj,"Proxy") and hasattr(obj.Proxy,"isStandardCase"):
        return obj.Proxy.isStandardCase(obj)
    return False


def getIfcTypeFromObj(obj):

    if (Draft.getType(obj) == "BuildingPart") and hasattr(obj,"IfcType") and (obj.IfcType == "Undefined"):
        ifctype = "IfcBuildingStorey" # export BuildingParts as Storeys if their type wasn't explicitly set
    elif hasattr(obj,"IfcType"):
        ifctype = obj.IfcType.replace(" ","")
    else:
        ifctype = Draft.getType(obj)

    if ifctype in translationtable.keys():
        ifctype = translationtable[ifctype]
    if ifctype == "VisGroup":
        ifctype = "Group"

    return "Ifc" + ifctype


def exportIFC2X3Attributes(obj, kwargs):

    ifctype = getIfcTypeFromObj(obj)
    if ifctype in ["IfcSlab", "IfcFooting"]:
        kwargs.update({"PredefinedType": "NOTDEFINED"})
    elif ifctype == "IfcBuilding":
        kwargs.update({"CompositionType": "ELEMENT"})
    elif ifctype == "IfcBuildingStorey":
        kwargs.update({"CompositionType": "ELEMENT"})
    elif ifctype == "IfcBuildingElementProxy":
        kwargs.update({"CompositionType": "ELEMENT"})
    elif ifctype == "IfcSpace":
        internal = "NOTDEFINED"
        if hasattr(obj,"Internal"):
            if obj.Internal:
                internal = "INTERNAL"
            else:
                internal = "EXTERNAL"
        kwargs.update({
            "CompositionType": "ELEMENT",
            "InteriorOrExteriorSpace": internal,
            "ElevationWithFlooring": obj.Shape.BoundBox.ZMin/1000.0
        })
    elif ifctype == "IfcReinforcingBar":
        kwargs.update({
            "NominalDiameter": obj.Diameter.Value,
            "BarLength": obj.Length.Value
        })
    elif ifctype == "IfcBuildingStorey":
        kwargs.update({"Elevation": obj.Placement.Base.z/1000.0})
    return kwargs


def exportIfcAttributes(obj, kwargs):

    for property in obj.PropertiesList:
        if obj.getGroupOfProperty(property) == "IFC Attributes" and obj.getPropertyByName(property):
            value = obj.getPropertyByName(property)
            if isinstance(value, FreeCAD.Units.Quantity):
                value = float(value)
                if property in ["ElevationWithFlooring","Elevation"]:
                    value = value/1000 # some properties must be changed to meters
            kwargs.update({property: value})
    return kwargs


def buildAddress(obj,ifcfile):

    a = obj.Address or None
    p = obj.PostalCode or None
    t = obj.City or None
    r = obj.Region or None
    c = obj.Country or None
    if six.PY2:
        if a:
            a = a.encode("utf8")
        if p:
            p = p.encode("utf8")
        if t:
            t = t.encode("utf8")
        if r:
            r = r.encode("utf8")
        if c:
            c = c.encode("utf8")
    if a or p or t or r or c:
        addr = ifcfile.createIfcPostalAddress("SITE",'Site Address','',None,[a],None,t,r,p,c)
    else:
        addr = None
    return addr


def createFromProperties(propsets,ifcfile):

    "creates a FreeCAD parametric object from a set of properties"

    obj = None
    sets = []
    global parametrics
    appset = None
    guiset = None
    for pset in propsets.keys():
        if ifcfile[pset].Name == "FreeCADPropertySet":
            appset = {}
            for pid in propsets[pset]:
                p = ifcfile[pid]
                appset[p.Name] = p.NominalValue.wrappedValue
        elif ifcfile[pset].Name == "FreeCADGuiPropertySet":
            guiset = {}
            for pid in propsets[pset]:
                p = ifcfile[pid]
                guiset[p.Name] = p.NominalValue.wrappedValue
    if appset:
        oname = None
        otype = None
        if "FreeCADType" in appset.keys():
            if "FreeCADName" in appset.keys():
                obj = FreeCAD.ActiveDocument.addObject(appset["FreeCADType"],appset["FreeCADName"])
                if "FreeCADAppObject" in appset:
                    mod,cla = appset["FreeCADAppObject"].split(".")
                    if "'" in mod:
                        mod = mod.split("'")[-1]
                    if "'" in cla:
                        cla = cla.split("'")[0]
                    import importlib
                    mod = importlib.import_module(mod)
                    getattr(mod,cla)(obj)
                sets.append(("App",appset))
                if FreeCAD.GuiUp:
                    if guiset:
                        if "FreeCADGuiObject" in guiset:
                            mod,cla = guiset["FreeCADGuiObject"].split(".")
                            if "'" in mod:
                                mod = mod.split("'")[-1]
                            if "'" in cla:
                                cla = cla.split("'")[0]
                            import importlib
                            mod = importlib.import_module(mod)
                            getattr(mod,cla)(obj.ViewObject)
                        sets.append(("Gui",guiset))
    if obj and sets:
        for realm,pset in sets:
            if realm == "App":
                target = obj
            else:
                target = obj.ViewObject
            for key,val in pset.items():
                if key.startswith("FreeCAD_") or key.startswith("FreeCADGui_"):
                    name = key.split("_")[1]
                    if name in target.PropertiesList:
                        if not target.getEditorMode(name):
                            ptype = target.getTypeIdOfProperty(name)
                            if ptype in ["App::PropertyString","App::PropertyEnumeration","App::PropertyInteger","App::PropertyFloat"]:
                                setattr(target,name,val)
                            elif ptype in ["App::PropertyLength","App::PropertyDistance"]:
                                setattr(target,name,val*1000)
                            elif ptype == "App::PropertyBool":
                                if val in [".T.",True]:
                                    setattr(target,name,True)
                                else:
                                    setattr(target,name,False)
                            elif ptype == "App::PropertyVector":
                                setattr(target,name,FreeCAD.Vector([float(s) for s in val.split("(")[1].strip(")").split(",")]))
                            elif ptype == "App::PropertyArea":
                                setattr(target,name,val*1000000)
                            elif ptype == "App::PropertyPlacement":
                                data = val.split("[")[1].strip("]").split("(")
                                data = [data[1].split(")")[0],data[2].strip(")")]
                                v = FreeCAD.Vector([float(s) for s in data[0].split(",")])
                                r = FreeCAD.Rotation(*[float(s) for s in data[1].split(",")])
                                setattr(target,name,FreeCAD.Placement(v,r))
                            elif ptype == "App::PropertyLink":
                                link = val.split("_")[1]
                                parametrics.append([target,name,link])
                            else:
                                print("Unhandled FreeCAD property:",name," of type:",ptype)
    return obj


def createCurve(ifcfile,wire):

    "creates an IfcCompositeCurve from a shape"

    segments = []
    pol = None
    last = None
    if wire.ShapeType == "edge":
        edges = [wire]
    else:
        edges = Part.__sortEdges__(wire.Edges)
    for e in edges:
        if isinstance(e.Curve,Part.Circle):
            xaxis = e.Curve.XAxis
            zaxis = e.Curve.Axis
            follow = True
            if last:
                if not DraftVecUtils.equals(last,e.Vertexes[0].Point):
                    follow = False
                    last = e.Vertexes[0].Point
                    prev = e.Vertexes[-1].Point
                else:
                    last = e.Vertexes[-1].Point
                    prev = e.Vertexes[0].Point
            else:
                last = e.Vertexes[-1].Point
                prev = e.Vertexes[0].Point
            p1 = math.degrees(-DraftVecUtils.angle(prev.sub(e.Curve.Center),xaxis,zaxis))
            p2 = math.degrees(-DraftVecUtils.angle(last.sub(e.Curve.Center),xaxis,zaxis))
            da = DraftVecUtils.angle(e.valueAt(e.FirstParameter+0.1).sub(e.Curve.Center),prev.sub(e.Curve.Center))
            #print("curve params:",p1,",",p2,"da=",da)
            if p1 < 0:
                p1 = 360 + p1
            if p2 < 0:
                p2 = 360 + p2
            if da > 0:
                #follow = not(follow) # now we always draw segments in the correct order, so follow is always true
                pass
            #print("  circle from",prev,"to",last,"a1=",p1,"a2=",p2)
            ovc =       ifcbin.createIfcCartesianPoint(tuple(e.Curve.Center))
            zvc =       ifcbin.createIfcDirection(tuple(zaxis))
            xvc =       ifcbin.createIfcDirection(tuple(xaxis))
            plc =       ifcbin.createIfcAxis2Placement3D(ovc,zvc,xvc)
            cir =       ifcfile.createIfcCircle(plc,e.Curve.Radius)
            curve =     ifcfile.createIfcTrimmedCurve(
                cir,
                [ifcfile.createIfcParameterValue(p1)],
                [ifcfile.createIfcParameterValue(p2)],
                follow,
                "PARAMETER"
            )
        else:
            verts = [vertex.Point for vertex in e.Vertexes]
            if last:
                if not DraftVecUtils.equals(last,verts[0]):
                    verts.reverse()
                    last = e.Vertexes[0].Point
                else:
                    last = e.Vertexes[-1].Point
            else:
                last = e.Vertexes[-1].Point
            #print("  polyline:",verts)
            pts =     [ifcbin.createIfcCartesianPoint(tuple(v)) for v in verts]
            curve =   ifcbin.createIfcPolyline(pts)
        segment = ifcfile.createIfcCompositeCurveSegment("CONTINUOUS",True,curve)
        segments.append(segment)
    if segments:
        pol = ifcfile.createIfcCompositeCurve(segments,False)
    return pol


def getEdgesAngle(edge1, edge2):

    """ getEdgesAngle(edge1, edge2): returns a angle between two edges."""

    vec1 = vec(edge1)
    vec2 = vec(edge2)
    angle = vec1.getAngle(vec2)
    angle = math.degrees(angle)
    return angle


def checkRectangle(edges):

    """ checkRectangle(edges=[]): This function checks whether the given form is a rectangle
       or not. It will return True when edges form a rectangular shape or return False
       when edges do not form a rectangular shape."""

    if FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch").GetBool("DisableIfcRectangleProfileDef",False):
        return False
    if len(edges) != 4:
        return False
    angles = [
        round(getEdgesAngle(edges[0], edges[1])),
        round(getEdgesAngle(edges[0], edges[2])),
        round(getEdgesAngle(edges[0], edges[3]))
    ]
    if angles.count(90) == 2 and (angles.count(180) == 1 or angles.count(0) == 1):
        return True
    return False


def getProfile(ifcfile,p):

    """returns an IFC profile definition from a shape"""

    import Part
    import DraftGeomUtils
    profile = None
    if len(p.Edges) == 1:
        pxvc = ifcbin.createIfcDirection((1.0,0.0))
        povc = ifcbin.createIfcCartesianPoint((0.0,0.0))
        pt = ifcbin.createIfcAxis2Placement2D(povc,pxvc)
        if isinstance(p.Edges[0].Curve,Part.Circle):
            # extruded circle
            profile = ifcfile.createIfcCircleProfileDef("AREA",None,pt,p.Edges[0].Curve.Radius)
        elif isinstance(p.Edges[0].Curve,Part.Ellipse):
            # extruded ellipse
            profile = ifcfile.createIfcEllipseProfileDef("AREA",None,pt,p.Edges[0].Curve.MajorRadius,p.Edges[0].Curve.MinorRadius)
    elif (checkRectangle(p.Edges)):
        # arbitrarily use the first edge as the rectangle orientation
        d = vec(p.Edges[0])
        d.normalize()
        pxvc = ifcbin.createIfcDirection(tuple(d))
        povc = ifcbin.createIfcCartesianPoint((0.0,0.0))
        pt = ifcbin.createIfcAxis2Placement2D(povc,pxvc)
        #semiPerimeter = p.Length/2
        #diff = math.sqrt(semiPerimeter**2 - 4*p.Area)
        #b = max(abs((semiPerimeter + diff)/2),abs((semiPerimeter - diff)/2))
        #h = min(abs((semiPerimeter + diff)/2),abs((semiPerimeter - diff)/2))
        b = p.Edges[0].Length
        h = p.Edges[1].Length
        profile = ifcfile.createIfcRectangleProfileDef("AREA",'rectangular',pt,b,h)
    elif (len(p.Faces) == 1) and (len(p.Wires) > 1):
        # face with holes
        f = p.Faces[0]
        if DraftGeomUtils.hasCurves(f.OuterWire):
            outerwire = createCurve(ifcfile,f.OuterWire)
        else:
            w = Part.Wire(Part.__sortEdges__(f.OuterWire.Edges))
            pts = [ifcbin.createIfcCartesianPoint(tuple(v.Point)[:2]) for v in w.Vertexes+[w.Vertexes[0]]]
            outerwire = ifcbin.createIfcPolyline(pts)
        innerwires = []
        for w in f.Wires:
            if w.hashCode() != f.OuterWire.hashCode():
                if DraftGeomUtils.hasCurves(w):
                    innerwires.append(createCurve(ifcfile,w))
                else:
                    w = Part.Wire(Part.__sortEdges__(w.Edges))
                    pts = [ifcbin.createIfcCartesianPoint(tuple(v.Point)[:2]) for v in w.Vertexes+[w.Vertexes[0]]]
                    innerwires.append(ifcbin.createIfcPolyline(pts))
        profile = ifcfile.createIfcArbitraryProfileDefWithVoids("AREA",None,outerwire,innerwires)
    else:
        if DraftGeomUtils.hasCurves(p):
            # extruded composite curve
            pol = createCurve(ifcfile,p)
        else:
            # extruded polyline
            w = Part.Wire(Part.__sortEdges__(p.Wires[0].Edges))
            pts = [ifcbin.createIfcCartesianPoint(tuple(v.Point)[:2]) for v in w.Vertexes+[w.Vertexes[0]]]
            pol = ifcbin.createIfcPolyline(pts)
        profile = ifcfile.createIfcArbitraryClosedProfileDef("AREA",None,pol)
    return profile


def getRepresentation(ifcfile,context,obj,forcebrep=False,subtraction=False,tessellation=1):

    """returns an IfcShapeRepresentation object or None"""

    import Part
    import DraftGeomUtils
    import DraftVecUtils
    shapes = []
    placement = None
    productdef = None
    shapetype = "no shape"
    tostore = False
    subplacement = None

    # check for clones

    if (not subtraction) and (not forcebrep):
        for k,v in clones.items():
            if (obj.Name == k) or (obj.Name in v):
                if k in sharedobjects:
                    # base shape already exists
                    repmap = sharedobjects[k]
                    pla = obj.getGlobalPlacement()
                    axis1 = ifcbin.createIfcDirection(tuple(pla.Rotation.multVec(FreeCAD.Vector(1,0,0))))
                    axis2 = ifcbin.createIfcDirection(tuple(pla.Rotation.multVec(FreeCAD.Vector(0,1,0))))
                    axis3 = ifcbin.createIfcDirection(tuple(pla.Rotation.multVec(FreeCAD.Vector(0,0,1))))
                    origin = ifcbin.createIfcCartesianPoint(tuple(FreeCAD.Vector(pla.Base).multiply(0.001)))
                    transf = ifcbin.createIfcCartesianTransformationOperator3D(axis1,axis2,origin,1.0,axis3)
                    mapitem = ifcfile.createIfcMappedItem(repmap,transf)
                    shapes = [mapitem]
                    solidType = "MappedRepresentation"
                    shapetype = "clone"
                else:
                    # base shape not yet created
                    tostore = k

    # unhandled case: object is duplicated because of Axis
    if obj.isDerivedFrom("Part::Feature") and (len(obj.Shape.Solids) > 1) and hasattr(obj,"Axis") and obj.Axis:
        forcebrep = True

    if (not shapes) and (not forcebrep):
        profile = None
        ev = FreeCAD.Vector()
        if hasattr(obj,"Proxy"):
            if hasattr(obj.Proxy,"getRebarData"):
                # export rebars as IfcSweptDiskSolid
                rdata = obj.Proxy.getRebarData(obj)
                if rdata:
                    # convert to meters
                    r = rdata[1] * 0.001
                    for w in rdata[0]:
                        w.scale(0.001)
                        cur = createCurve(ifcfile,w)
                        shape = ifcfile.createIfcSweptDiskSolid(cur,r)
                        shapes.append(shape)
                        solidType = "SweptSolid"
                        shapetype = "extrusion"
            if (not shapes) and hasattr(obj.Proxy,"getExtrusionData"):
                extdata = obj.Proxy.getExtrusionData(obj)
                if extdata:
                    #print(extdata)
                    # convert to meters
                    p = extdata[0]
                    if not isinstance(p,list):
                        p = [p]
                    ev = extdata[1]
                    if not isinstance(ev,list):
                        ev = [ev]
                    pl = extdata[2]
                    if not isinstance(pl,list):
                        pl = [pl]
                    for i in range(len(p)):
                        pi = p[i]
                        pi.scale(0.001)
                        if i < len(ev):
                            evi = FreeCAD.Vector(ev[i])
                        else:
                            evi = FreeCAD.Vector(ev[-1])
                        evi.multiply(0.001)
                        if i < len(pl):
                            pli = pl[i].copy()
                        else:
                            pli = pl[-1].copy()
                        pli.Base = pli.Base.multiply(0.001)
                        pstr = str([v.Point for v in p[i].Vertexes])
                        if pstr in profiledefs:
                            profile = profiledefs[pstr]
                            shapetype = "reusing profile"
                        else:
                            profile = getProfile(ifcfile,pi)
                            if profile:
                                profiledefs[pstr] = profile
                        if profile and not(DraftVecUtils.isNull(evi)):
                            #ev = pl.Rotation.inverted().multVec(evi)
                            #print("evi:",evi)
                            if not tostore:
                                # add the object placement to the profile placement. Otherwise it'll be done later at map insert
                                pl2 = obj.getGlobalPlacement()
                                pl2.Base = pl2.Base.multiply(0.001)
                                pli = pl2.multiply(pli)
                            xvc =       ifcbin.createIfcDirection(tuple(pli.Rotation.multVec(FreeCAD.Vector(1,0,0))))
                            zvc =       ifcbin.createIfcDirection(tuple(pli.Rotation.multVec(FreeCAD.Vector(0,0,1))))
                            ovc =       ifcbin.createIfcCartesianPoint(tuple(pli.Base))
                            lpl =       ifcbin.createIfcAxis2Placement3D(ovc,zvc,xvc)
                            edir =      ifcbin.createIfcDirection(tuple(FreeCAD.Vector(evi).normalize()))
                            shape =     ifcfile.createIfcExtrudedAreaSolid(profile,lpl,edir,evi.Length)
                            shapes.append(shape)
                            solidType = "SweptSolid"
                            shapetype = "extrusion"

    if not shapes:

        # check if we keep a null shape (additions-only object)

        if (hasattr(obj,"Base") and hasattr(obj,"Width") and hasattr(obj,"Height")) \
                and (not obj.Base) \
                and obj.Additions \
                and (not obj.Width.Value) \
                and (not obj.Height.Value):
            shapes = None

        else:

            # brep representation

            fcshape = None
            solidType = "Brep"
            if subtraction:
                if hasattr(obj,"Proxy"):
                    if hasattr(obj.Proxy,"getSubVolume"):
                        fcshape = obj.Proxy.getSubVolume(obj)
            if not fcshape:
                if obj.isDerivedFrom("Part::Feature"):
                    #if hasattr(obj,"Base") and hasattr(obj,"Additions")and hasattr(obj,"Subtractions"):
                    if False: # above is buggy. No way to duplicate shapes that way?
                        if obj.Base and (not obj.Additions) and not(obj.Subtractions):
                            if obj.Base.isDerivedFrom("Part::Feature"):
                                if obj.Base.Shape:
                                    if obj.Base.Shape.Solids:
                                        fcshape = obj.Base.Shape
                                        subplacement = FreeCAD.Placement(obj.Placement)
                    if not fcshape:
                        if obj.Shape:
                            if not obj.Shape.isNull():
                                fcshape = obj.Shape.copy()
                                fcshape.Placement = obj.getGlobalPlacement()
            if fcshape:
                shapedef = str([v.Point for v in fcshape.Vertexes])
                if shapedef in shapedefs:
                    shapes = shapedefs[shapedef]
                    shapetype = "reusing brep"
                else:

                    # new ifcopenshell serializer

                    from ifcopenshell import geom
                    serialized = False
                    if hasattr(geom,"serialise") and obj.isDerivedFrom("Part::Feature") and SERIALIZE:
                        if obj.Shape.Faces:
                            sh = obj.Shape.copy()
                            sh.Placement = obj.getGlobalPlacement()
                            sh.scale(0.001) # to meters
                            p = geom.serialise(sh.exportBrepToString())
                            if p:
                                productdef = ifcfile.add(p)
                                for rep in productdef.Representations:
                                    rep.ContextOfItems = context
                                xvc = ifcbin.createIfcDirection((1.0,0.0,0.0))
                                zvc = ifcbin.createIfcDirection((0.0,0.0,1.0))
                                ovc = ifcbin.createIfcCartesianPoint((0.0,0.0,0.0))
                                gpl = ifcbin.createIfcAxis2Placement3D(ovc,zvc,xvc)
                                placement = ifcbin.createIfcLocalPlacement(gpl)
                                shapetype = "advancedbrep"
                                shapes = None
                                serialized = True

                    if not serialized:

                        # old method

                        solids = []
                        if fcshape.Solids:
                            dataset = fcshape.Solids
                        else:
                            dataset = fcshape.Shells
                            #if DEBUG: print("Warning! object contains no solids")

                        # if this is a clone, place back the shapes in null position
                        if tostore:
                            for shape in dataset:
                                shape.Placement = FreeCAD.Placement()

                        for fcsolid in dataset:
                            fcsolid.scale(0.001) # to meters
                            faces = []
                            curves = False
                            shapetype = "brep"
                            for fcface in fcsolid.Faces:
                                for e in fcface.Edges:
                                    if DraftGeomUtils.geomType(e) != "Line":
                                        from FreeCAD import Base
                                        try:
                                            if e.curvatureAt(e.FirstParameter+(e.LastParameter-e.FirstParameter)/2) > 0.0001:
                                                curves = True
                                                break
                                        except Part.OCCError:
                                            pass
                                        except Base.FreeCADError:
                                            pass
                            if curves:
                                joinfacets = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch").GetBool("ifcJoinCoplanarFacets",False)
                                usedae = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch").GetBool("ifcUseDaeOptions",False)
                                if joinfacets:
                                    result = Arch.removeCurves(fcsolid,dae=usedae)
                                    if result:
                                        fcsolid = result
                                    else:
                                        # fall back to standard triangulation
                                        joinfacets = False
                                if not joinfacets:
                                    shapetype = "triangulated"
                                    if usedae:
                                        import importDAE
                                        tris = importDAE.triangulate(fcsolid)
                                    else:
                                        tris = fcsolid.tessellate(tessellation)
                                    for tri in tris[1]:
                                        pts =   [ifcbin.createIfcCartesianPoint(tuple(tris[0][i])) for i in tri]
                                        loop =  ifcbin.createIfcPolyLoop(pts)
                                        bound = ifcfile.createIfcFaceOuterBound(loop,True)
                                        face =  ifcfile.createIfcFace([bound])
                                        faces.append(face)
                                        fcsolid = Part.Shape() # empty shape so below code is not executed

                            for fcface in fcsolid.Faces:
                                loops = []
                                verts = [v.Point for v in fcface.OuterWire.OrderedVertexes]
                                c = fcface.CenterOfMass
                                v1 = verts[0].sub(c)
                                v2 = verts[1].sub(c)
                                try:
                                    n = fcface.normalAt(0,0)
                                except Part.OCCError:
                                    continue # this is a very wrong face, it probably shouldn't be here...
                                if DraftVecUtils.angle(v2,v1,n) >= 0:
                                    verts.reverse() # inverting verts order if the direction is couterclockwise
                                pts =   [ifcbin.createIfcCartesianPoint(tuple(v)) for v in verts]
                                loop =  ifcbin.createIfcPolyLoop(pts)
                                bound = ifcfile.createIfcFaceOuterBound(loop,True)
                                loops.append(bound)
                                for wire in fcface.Wires:
                                    if wire.hashCode() != fcface.OuterWire.hashCode():
                                        verts = [v.Point for v in wire.OrderedVertexes]
                                        if len(verts) > 1:
                                            v1 = verts[0].sub(c)
                                            v2 = verts[1].sub(c)
                                            if DraftVecUtils.angle(v2,v1,DraftVecUtils.neg(n)) >= 0:
                                                verts.reverse()
                                            pts =   [ifcbin.createIfcCartesianPoint(tuple(v)) for v in verts]
                                            loop =  ifcbin.createIfcPolyLoop(pts)
                                            bound = ifcfile.createIfcFaceBound(loop,True)
                                            loops.append(bound)
                                        else:
                                            print("Warning: wire with one/no vertex in ", obj.Label)
                                face =  ifcfile.createIfcFace(loops)
                                faces.append(face)

                            if faces:
                                shell = ifcfile.createIfcClosedShell(faces)
                                shape = ifcfile.createIfcFacetedBrep(shell)
                                shapes.append(shape)

                        shapedefs[shapedef] = shapes

    if shapes:

        if tostore:
            subrep = ifcfile.createIfcShapeRepresentation(context,'Body',solidType,shapes)
            xvc = ifcbin.createIfcDirection((1.0,0.0,0.0))
            zvc = ifcbin.createIfcDirection((0.0,0.0,1.0))
            ovc = ifcbin.createIfcCartesianPoint((0.0,0.0,0.0))
            gpl = ifcbin.createIfcAxis2Placement3D(ovc,zvc,xvc)
            repmap = ifcfile.createIfcRepresentationMap(gpl,subrep)
            pla = obj.getGlobalPlacement()
            axis1 = ifcbin.createIfcDirection(tuple(pla.Rotation.multVec(FreeCAD.Vector(1,0,0))))
            axis2 = ifcbin.createIfcDirection(tuple(pla.Rotation.multVec(FreeCAD.Vector(0,1,0))))
            origin = ifcbin.createIfcCartesianPoint(tuple(FreeCAD.Vector(pla.Base).multiply(0.001)))
            axis3 = ifcbin.createIfcDirection(tuple(pla.Rotation.multVec(FreeCAD.Vector(0,0,1))))
            transf = ifcbin.createIfcCartesianTransformationOperator3D(axis1,axis2,origin,1.0,axis3)
            mapitem = ifcfile.createIfcMappedItem(repmap,transf)
            shapes = [mapitem]
            sharedobjects[tostore] = repmap
            solidType = "MappedRepresentation"

        # set surface style
        if FreeCAD.GuiUp and (not subtraction) and hasattr(obj.ViewObject,"ShapeColor"):
            # every object gets a surface style. If the obj has a material, the surfstyle
            # is named after it. Revit will treat surfacestyles as materials (and discard
            # actual ifcmaterial)
            key = None
            rgbt = [obj.ViewObject.ShapeColor[:3]+(obj.ViewObject.Transparency/100.0,) for shape in shapes]
            if hasattr(obj.ViewObject,"DiffuseColor") \
                    and obj.ViewObject.DiffuseColor \
                    and (len(obj.ViewObject.DiffuseColor) == len(obj.Shape.Faces)) \
                    and (len(obj.Shape.Solids) == len(shapes)):
                i = 0
                rgbt = []
                for sol in obj.Shape.Solids:
                    rgbt.append(obj.ViewObject.DiffuseColor[i])
                    i += len(sol.Faces)
            for i,shape in enumerate(shapes):
                key = rgbt[i]
                #if hasattr(obj,"Material"):
                #    if obj.Material:
                #        key = obj.Material.Name #TODO handle multimaterials
                if key in surfstyles:
                    psa = surfstyles[key]
                else:
                    m = None
                    if hasattr(obj,"Material"):
                        if obj.Material:
                            m = obj.Material.Label
                            if six.PY2:
                                m = m.encode("utf8")
                    psa = ifcbin.createIfcPresentationStyleAssignment(m,rgbt[i][0],rgbt[i][1],rgbt[i][2],rgbt[i][3])
                    surfstyles[key] = psa
                isi = ifcfile.createIfcStyledItem(shape,[psa],None)

        xvc = ifcbin.createIfcDirection((1.0,0.0,0.0))
        zvc = ifcbin.createIfcDirection((0.0,0.0,1.0))
        ovc = ifcbin.createIfcCartesianPoint((0.0,0.0,0.0))
        gpl = ifcbin.createIfcAxis2Placement3D(ovc,zvc,xvc)
        placement = ifcbin.createIfcLocalPlacement(gpl)
        representation = ifcfile.createIfcShapeRepresentation(context,'Body',solidType,shapes)
        productdef = ifcfile.createIfcProductDefinitionShape(None,None,[representation])

    return productdef,placement,shapetype


# Below are 2D helper functions needed while IfcOpenShell cannot do this itself...


def setRepresentation(representation,scaling=1000):
    """Returns a shape from a 2D IfcShapeRepresentation"""

    def getPolyline(ent):
        pts = []
        for p in ent.Points:
            c = p.Coordinates
            c = FreeCAD.Vector(c[0],c[1],c[2] if len(c) > 2 else 0)
            c.multiply(scaling)
            pts.append(c)
        return Part.makePolygon(pts)

    def getLine(ent):
        pts = []
        p1 = getVector(ent.Pnt)
        p1.multiply(scaling)
        pts.append(p1)
        p2 = getVector(ent.Dir)
        p2.multiply(scaling)
        p2 = p1.add(p2)
        pts.append(p2)
        return Part.makePolygon(pts)

    def getCircle(ent):
        c = ent.Position.Location.Coordinates
        c = FreeCAD.Vector(c[0],c[1],c[2] if len(c) > 2 else 0)
        c.multiply(scaling)
        r = ent.Radius*scaling
        return Part.makeCircle(r,c)

    def getCurveSet(ent):
        result = []
        if ent.is_a() in ["IfcGeometricCurveSet","IfcGeometricSet"]:
            elts = ent.Elements
        elif ent.is_a() in ["IfcLine","IfcPolyline","IfcCircle","IfcTrimmedCurve"]:
            elts = [ent]
        for el in elts:
            if el.is_a("IfcPolyline"):
                result.append(getPolyline(el))
            elif el.is_a("IfcLine"):
                result.append(getLine(el))
            elif el.is_a("IfcCircle"):
                result.append(getCircle(el))
            elif el.is_a("IfcTrimmedCurve"):
                base = el.BasisCurve
                t1 = el.Trim1[0].wrappedValue
                t2 = el.Trim2[0].wrappedValue
                if not el.SenseAgreement:
                    t1,t2 = t2,t1
                if base.is_a("IfcPolyline"):
                    bc = getPolyline(base)
                    result.append(bc)
                elif base.is_a("IfcCircle"):
                    bc = getCircle(base)
                    e = Part.ArcOfCircle(bc.Curve,math.radians(t1),math.radians(t2)).toShape()
                    d = base.Position.RefDirection.DirectionRatios
                    v = FreeCAD.Vector(d[0],d[1],d[2] if len(d) > 2 else 0)
                    a = -DraftVecUtils.angle(v)
                    e.rotate(bc.Curve.Center,FreeCAD.Vector(0,0,1),math.degrees(a))
                    result.append(e)
            elif el.is_a("IfcCompositeCurve"):
                for base in el.Segments:
                    if base.ParentCurve.is_a("IfcPolyline"):
                        bc = getPolyline(base.ParentCurve)
                        result.append(bc)
                    elif base.ParentCurve.is_a("IfcCircle"):
                        bc = getCircle(base.ParentCurve)
                        e = Part.ArcOfCircle(bc.Curve,math.radians(t1),math.radians(t2)).toShape()
                        d = base.Position.RefDirection.DirectionRatios
                        v = FreeCAD.Vector(d[0],d[1],d[2] if len(d) > 2 else 0)
                        a = -DraftVecUtils.angle(v)
                        e.rotate(bc.Curve.Center,FreeCAD.Vector(0,0,1),math.degrees(a))
                        result.append(e)

        return result

    result = []
    if representation.is_a("IfcShapeRepresentation"):
        for item in representation.Items:
            if item.is_a() in ["IfcGeometricCurveSet","IfcGeometricSet"]:
                result = getCurveSet(item)
            elif item.is_a("IfcMappedItem"):
                preresult = setRepresentation(item.MappingSource.MappedRepresentation,scaling)
                pla = getPlacement(item.MappingSource.MappingOrigin,scaling)
                rot = getRotation(item.MappingTarget)
                if pla:
                    if rot.Angle:
                        pla.Rotation = rot
                    for r in preresult:
                        #r.Placement = pla
                        result.append(r)
                else:
                    result = preresult
            elif item.is_a("IfcTextLiteral"):
                t = Draft.makeText([item.Literal],point=getPlacement(item.Placement,scaling).Base)
                return t # dirty hack... Object creation should not be done here
    elif representation.is_a() in ["IfcPolyline","IfcCircle","IfcTrimmedCurve"]:
        result = getCurveSet(representation)
    return result


def getRotation(entity):
    "returns a FreeCAD rotation from an IfcProduct with a IfcMappedItem representation"
    try:
        u = FreeCAD.Vector(entity.Axis1.DirectionRatios)
        v = FreeCAD.Vector(entity.Axis2.DirectionRatios)
        w = FreeCAD.Vector(entity.Axis3.DirectionRatios)
    except AttributeError:
        return FreeCAD.Rotation()
    import WorkingPlane
    p = WorkingPlane.plane(u=u,v=v,w=w)
    return p.getRotation().Rotation


def getPlacement(entity,scaling=1000):
    "returns a placement from the given entity"

    if not entity:
        return None
    import DraftVecUtils
    pl = None
    if entity.is_a("IfcAxis2Placement3D"):
        x = getVector(entity.RefDirection,scaling)
        z = getVector(entity.Axis,scaling)
        if x and z:
            y = z.cross(x)
            m = DraftVecUtils.getPlaneRotation(x,y,z)
            pl = FreeCAD.Placement(m)
        else:
            pl = FreeCAD.Placement()
        loc = getVector(entity.Location,scaling)
        if loc:
            pl.move(loc)
    elif entity.is_a("IfcLocalPlacement"):
        pl = getPlacement(entity.PlacementRelTo,1) # original placement
        relpl = getPlacement(entity.RelativePlacement,1) # relative transf
        if pl and relpl:
            pl = pl.multiply(relpl)
        elif relpl:
            pl = relpl
    elif entity.is_a("IfcCartesianPoint"):
        loc = getVector(entity,scaling)
        pl = FreeCAD.Placement()
        pl.move(loc)
    if pl:
        pl.Base = FreeCAD.Vector(pl.Base).multiply(scaling)
    return pl


def getVector(entity,scaling=1000):
    "returns a vector from the given entity"

    if not entity:
        return None
    v = None
    if entity.is_a("IfcDirection"):
        if len(entity.DirectionRatios) == 3:
            v= FreeCAD.Vector(tuple(entity.DirectionRatios))
        else:
            v = FreeCAD.Vector(tuple(entity.DirectionRatios+[0]))
    elif entity.is_a("IfcCartesianPoint"):
        if len(entity.Coordinates) == 3:
            v = FreeCAD.Vector(tuple(entity.Coordinates))
        else:
            v = FreeCAD.Vector(tuple(entity.Coordinates+[0]))
    #if v:
    #    v.multiply(scaling)
    return v


def getScaling(ifcfile):
    "returns a scaling factor from file units to mm"

    def getUnit(unit):
        if unit.Name == "METRE":
            if unit.Prefix == "KILO":
                return 1000000.0
            elif unit.Prefix == "HECTO":
                return 100000.0
            elif unit.Prefix == "DECA":
                return 10000.0
            elif not unit.Prefix:
                return 1000.0
            elif unit.Prefix == "DECI":
                return 100.0
            elif unit.Prefix == "CENTI":
                return 10.0
        return 1.0

    ua = ifcfile.by_type("IfcUnitAssignment")
    if not ua:
        return 1.0
    ua = ua[0]
    for u in ua.Units:
        if u.UnitType == "LENGTHUNIT":
            if u.is_a("IfcConversionBasedUnit"):
                f =  getUnit(u.ConversionFactor.UnitComponent)
                return f * u.ConversionFactor.ValueComponent.wrappedValue
            elif u.is_a("IfcSIUnit") or u.is_a("IfcUnit"):
                return getUnit(u)
    return 1.0
