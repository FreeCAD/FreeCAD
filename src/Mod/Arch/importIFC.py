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

__title__ =  "FreeCAD IFC importer - Enhanced ifcopenshell-only version"
__author__ = "Yorik van Havre","Jonathan Wiedemann"
__url__ =    "http://www.freecadweb.org"

import os,time,tempfile,uuid,FreeCAD,Part,Draft,Arch,math,DraftVecUtils

DEBUG = False

if open.__module__ == '__builtin__':
    pyopen = open # because we'll redefine open below

# which IFC type must create which FreeCAD type
typesmap = { "Site":       ["IfcSite"],
             "Building":   ["IfcBuilding"],
             "Floor":      ["IfcBuildingStorey"],
             "Structure":  ["IfcBeam", "IfcBeamStandardCase", "IfcColumn", "IfcColumnStandardCase", "IfcSlab", "IfcFooting", "IfcPile", "IfcTendon"],
             "Wall":       ["IfcWall", "IfcWallStandardCase", "IfcCurtainWall"],
             "Window":     ["IfcWindow", "IfcWindowStandardCase", "IfcDoor", "IfcDoorStandardCase"],
             "Roof":       ["IfcRoof"],
             "Stairs":     ["IfcStair", "IfcStairFlight", "IfcRamp", "IfcRampFlight"],
             "Space":      ["IfcSpace"],
             "Rebar":      ["IfcReinforcingBar"],
             "Equipment":  ["IfcFurnishingElement","IfcSanitaryTerminal","IfcFlowTerminal","IfcElectricAppliance"]
           }

# which IFC entity (product) is a structural object
structuralifcobjects = (
                       "IfcStructuralCurveMember", "IfcStructuralSurfaceMember",
                       "IfcStructuralPointConnection", "IfcStructuralCurveConnection", "IfcStructuralSurfaceConnection",
                       "IfcStructuralAction", "IfcStructuralPointAction", 
                       "IfcStructuralLinearAction", "IfcStructuralLinearActionVarying", "IfcStructuralPlanarAction"
                       )

# specific name translations
translationtable = { "Foundation":"Footing",
                     "Floor":"BuildingStorey",
                     "Rebar":"ReinforcingBar",
                     "HydroEquipment":"SanitaryTerminal",
                     "ElectricEquipment":"ElectricAppliance",
                     "Furniture":"FurnishingElement",
                     "Stair Flight":"StairFlight",
                     "Curtain Wall":"CurtainWall"
                   }

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


def decode(filename,utf=False):
    if isinstance(filename,unicode):
        # workaround since ifcopenshell currently can't handle unicode filenames
        if utf:
            encoding = "utf8"
        else:
            import sys
            encoding = sys.getfilesystemencoding()
        filename = filename.encode(encoding)
    return filename


def doubleClickTree(item,column):
    txt = item.text(column)
    if "Entity #" in txt:
        eid = txt.split("#")[1].split(":")[0]
        addr = tree.findItems(eid,0,0)
        if addr:
            tree.scrollToItem(addr[0])
            addr[0].setSelected(True)


def getPreferences():
    """retrieves IFC preferences"""
    global DEBUG, PREFIX_NUMBERS, SKIP, SEPARATE_OPENINGS
    global ROOT_ELEMENT, GET_EXTRUSIONS, MERGE_MATERIALS
    global MERGE_MODE_ARCH, MERGE_MODE_STRUCT, CREATE_CLONES
    global FORCE_BREP, IMPORT_PROPERTIES, STORE_UID
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
        filename = QtGui.QFileDialog.getOpenFileName(QtGui.qApp.activeWindow(),'IFC files','*.ifc')
        if filename:
            filename = filename[0]

    from PySide import QtCore,QtGui

    filename = decode(filename,utf=True)

    if not os.path.exists(filename):
        print "File not found"
        return

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
            item.setText(2,str(entity.is_a()))
            item.setFont(2,bold);

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
                        print "Error in entity ",entity
                        break
                    else:
                        if not argname in ["Id", "GlobalId"]:
                            colored = False
                            if isinstance(argvalue,ifcopenshell.entity_instance):
                                if argvalue.id() == 0:
                                    t = str(argvalue)
                                else:
                                    colored = True
                                    t = "Entity #" + str(argvalue.id()) + ": " + str(argvalue.is_a())
                            elif isinstance(argvalue,list):
                                t = ""
                            elif isinstance(argvalue,str) or isinstance(argvalue,unicode):
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

    if DEBUG: print "Opening ",filename,"...",
    try:
        doc = FreeCAD.getDocument(docname)
    except:
        doc = FreeCAD.newDocument(docname)
    FreeCAD.ActiveDocument = doc

    if DEBUG: print "done."
    
    global ROOT_ELEMENT
    if root:
        ROOT_ELEMENT = root

    #global ifcfile # keeping global for debugging purposes
    filename = decode(filename,utf=True)
    ifcfile = ifcopenshell.open(filename)
    from ifcopenshell import geom
    settings = ifcopenshell.geom.settings()
    settings.set(settings.USE_BREP_DATA,True)
    settings.set(settings.SEW_SHELLS,True)
    settings.set(settings.USE_WORLD_COORDS,True)
    if SEPARATE_OPENINGS:
        settings.set(settings.DISABLE_OPENING_SUBTRACTIONS,True)
    sites = ifcfile.by_type("IfcSite")
    buildings = ifcfile.by_type("IfcBuilding")
    floors = ifcfile.by_type("IfcBuildingStorey")
    products = ifcfile.by_type(ROOT_ELEMENT)
    openings = ifcfile.by_type("IfcOpeningElement")
    annotations = ifcfile.by_type("IfcAnnotation")
    materials = ifcfile.by_type("IfcMaterial")

    if DEBUG: print "Building relationships table...",

    # building relations tables
    objects = {} # { id:object, ... }
    additions = {} # { host:[child,...], ... }
    groups = {} # { host:[child,...], ... }     # used in structural IFC
    subtractions = [] # [ [opening,host], ... ]
    properties = {} # { obj : { cat : [property, ... ], ... }, ... }
    colors = {} # { id:(r,g,b) }
    shapes = {} # { id:shaoe } only used for merge mode
    structshapes = {} # { id:shaoe } only used for merge mode
    mattable = {} # { objid:matid }
    sharedobjects = {} # { representationmapid:object }
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
            if not obj.id() in properties :
                properties[obj.id()] = {}
            prop_by_category = {}
            prop = []
            if r.RelatingPropertyDefinition.is_a("IfcPropertySet"):
                prop.extend([e.id() for e in r.RelatingPropertyDefinition.HasProperties])
                prop_by_category[r.RelatingPropertyDefinition.id()] = prop
                properties[obj.id()].update(prop_by_category)
    for r in ifcfile.by_type("IfcRelAssociatesMaterial"):
        for o in r.RelatedObjects:
            mattable[o.id()] = r.RelatingMaterial.id()
    for r in ifcfile.by_type("IfcStyledItem"):
        if r.Styles[0].is_a("IfcPresentationStyleAssignment"):
            if r.Styles[0].Styles[0].is_a("IfcSurfaceStyle"):
                if r.Styles[0].Styles[0].Styles[0].is_a("IfcSurfaceStyleRendering"):
                    if r.Styles[0].Styles[0].Styles[0].SurfaceColour:
                        c = r.Styles[0].Styles[0].Styles[0].SurfaceColour
                        if r.Item:
                            for p in ifcfile.by_type("IfcProduct"):
                                if p.Representation:
                                    for it in p.Representation.Representations:
                                        if it.Items:
                                            if it.Items[0].id() == r.Item.id():
                                                colors[p.id()] = (c.Red,c.Green,c.Blue)
                                            elif it.Items[0].is_a("IfcBooleanResult"):
                                                if (it.Items[0].FirstOperand.id() == r.Item.id()):
                                                    colors[p.id()] = (c.Red,c.Green,c.Blue)
                        else:
                            for m in ifcfile.by_type("IfcMaterialDefinitionRepresentation"):
                                for it in m.Representations:
                                    if it.Items:
                                        if it.Items[0].id() == r.id():
                                            colors[m.RepresentedMaterial.id()] = (c.Red,c.Green,c.Blue)

    if only: # only import a list of IDs and their children
        ids = []
        while only:
            currentid = only.pop()
            ids.append(currentid)
            if currentid in additions.keys():
                only.extend(additions[currentid])
        products = [ifcfile[currentid] for currentid in ids]

    if DEBUG: print "done."

    count = 0
    from FreeCAD import Base
    progressbar = Base.ProgressIndicator()
    progressbar.start("Importing IFC objects...",len(products))
    if DEBUG: print "Processing objects..."

    # products
    for product in products:

        pid = product.id()
        guid = product.GlobalId
        ptype = product.is_a()
        if DEBUG: print count+1,"/",len(products)," creating object #",pid," : ",ptype,
        name = str(ptype[3:])
        if product.Name:
            name = product.Name.encode("utf8")
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
            if DEBUG: print " (struct)",
        else:
            if DEBUG: print " (arch)",
        if MERGE_MODE_ARCH == 4 and archobj:
            if DEBUG: print " skipped."
            continue
        if MERGE_MODE_STRUCT == 3 and not archobj:
            if DEBUG: print " skipped."
            continue
        if pid in skip: # user given id skip list
            if DEBUG: print " skipped."
            continue
        if ptype in SKIP: # preferences-set type skip list
            if DEBUG: print " skipped."
            continue

        # detect if this object is sharing its shape
        clone = None
        store = None
        prepr = None
        try:
            prepr = product.Representation
        except:
            if DEBUG: print " ERROR unable to get object representation",
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

        if structobj:
            settings.set(settings.INCLUDE_CURVES,True)
        else:
            settings.set(settings.INCLUDE_CURVES,False)
        try:
            cr = ifcopenshell.geom.create_shape(settings,product)
            brep = cr.geometry.brep_data
        except:
            pass # IfcOpenShell will yield an error if a given product has no shape, but we don't care

        if brep:
            if DEBUG: print " ",str(len(brep)/1000),"k ",

            shape = Part.Shape()
            shape.importBrepFromString(brep)

            shape.scale(1000.0) # IfcOpenShell always outputs in meters

            if not shape.isNull():
                if (MERGE_MODE_ARCH > 0 and archobj) or structobj:
                    if ptype == "IfcSpace": # do not add spaces to compounds
                        if DEBUG: print "skipping space ",pid
                    elif structobj:
                        structshapes[pid] = shape
                        if DEBUG: print shape.Solids," ",
                        baseobj = shape
                    else:
                        shapes[pid] = shape
                        if DEBUG: print shape.Solids," ",
                        baseobj = shape
                else:
                    if clone:
                        if DEBUG: print "clone ",
                    else:
                        if GET_EXTRUSIONS:
                            ex = Arch.getExtrusionData(shape)
                            if ex:
                                print "extrusion ",
                                baseface = FreeCAD.ActiveDocument.addObject("Part::Feature",name+"_footprint")
                                baseface.Shape = ex[0]
                                baseobj = FreeCAD.ActiveDocument.addObject("Part::Extrusion",name+"_body")
                                baseobj.Base = baseface
                                baseobj.Dir = ex[1]
                                if FreeCAD.GuiUp:
                                    baseface.ViewObject.hide()
                        if (not baseobj):
                            baseobj = FreeCAD.ActiveDocument.addObject("Part::Feature",name+"_body")
                            baseobj.Shape = shape
            else:
                if DEBUG: print  "null shape ",
            if not shape.isValid():
                if DEBUG: print "invalid shape ",
                #continue

        else:
            if DEBUG: print " no brep ",

        if MERGE_MODE_ARCH == 0 and archobj:

            # full Arch objects
            for freecadtype,ifctypes in typesmap.items():
                if ptype in ifctypes:
                    if clone:
                        obj = getattr(Arch,"make"+freecadtype)(name=name)
                        obj.CloneOf = clone
                        if shape:
                            v = shape.Solids[0].CenterOfMass.sub(clone.Shape.Solids[0].CenterOfMass)
                            r = getRotation(product)
                            if not r.isNull():
                                v = v.add(clone.Shape.Solids[0].CenterOfMass)
                                v = v.add(r.multVec(clone.Shape.Solids[0].CenterOfMass.negative()))
                            obj.Placement.Rotation = r
                            obj.Placement.move(v)
                    else:
                        obj = getattr(Arch,"make"+freecadtype)(baseobj=baseobj,name=name)
                        if store:
                            sharedobjects[store] = obj
                    obj.Label = name
                    if FreeCAD.GuiUp and baseobj:
                        if hasattr(baseobj,"ViewObject"):
                            baseobj.ViewObject.hide()
                    # setting role
                    try:
                        r = ptype[3:]
                        tr = dict((v,k) for k, v in translationtable.iteritems())
                        if r in tr.keys():
                            r = tr[r]
                        # remove the "StandardCase"
                        if "StandardCase" in r:
                            r = r[:-12]
                        obj.Role = r
                    except:
                        pass
                    # setting uid
                    if hasattr(obj,"IfcAttributes"):
                        a = obj.IfcAttributes
                        a["IfcUID"] = str(guid)
                        obj.IfcAttributes = a
                    break
            if not obj:
                obj = Arch.makeComponent(baseobj,name=name)
            if obj:
                sols = str(obj.Shape.Solids) if hasattr(obj,"Shape") else ""
                if DEBUG: print sols
                objects[pid] = obj

        elif (MERGE_MODE_ARCH == 1 and archobj) or (MERGE_MODE_STRUCT == 0 and not archobj):

            # non-parametric Arch objects
            if ptype in ["IfcSite","IfcBuilding","IfcBuildingStorey"]:
                for freecadtype,ifctypes in typesmap.items():
                    if ptype in ifctypes:
                        obj = getattr(Arch,"make"+freecadtype)(baseobj=None,name=name)
            elif baseobj:
                obj = Arch.makeComponent(baseobj,name=name,delete=True)

        elif (MERGE_MODE_ARCH == 2 and archobj) or (MERGE_MODE_STRUCT == 1 and not archobj):

            # Part shapes
            if ptype in ["IfcSite","IfcBuilding","IfcBuildingStorey"]:
                for freecadtype,ifctypes in typesmap.items():
                    if ptype in ifctypes:
                        obj = getattr(Arch,"make"+freecadtype)(baseobj=None,name=name)
            elif baseobj:
                obj = FreeCAD.ActiveDocument.addObject("Part::Feature",name)
                obj.Shape = shape

        if obj:

            obj.Label = name
            objects[pid] = obj

            # properties
            if pid in properties:
                if IMPORT_PROPERTIES and hasattr(obj,"IfcProperties") :
                    ifc_spreadsheet = Arch.makeIfcSpreadsheet()
                    n=2
                    for c in properties[pid].keys():
                        o = ifcfile[c]
                        if DEBUG : print("propertyset Name",o.Name,type(o.Name))
                        catname = o.Name
                        for p in properties[pid][c]:
                            l = ifcfile[p]
                            if l.is_a("IfcPropertySingleValue"):
                                if DEBUG :
                                    print("property name",l.Name,type(l.Name))
                                ifc_spreadsheet.set(str('A'+str(n)), catname.encode("utf8"))
                                ifc_spreadsheet.set(str('B'+str(n)), l.Name.encode("utf8"))
                                if l.NominalValue :
                                    if DEBUG :
                                        print("property NominalValue",l.NominalValue.is_a(),type(l.NominalValue.is_a()))
                                        print("property NominalValue.wrappedValue",l.NominalValue.wrappedValue,type(l.NominalValue.wrappedValue))
                                        #print("l.NominalValue.Unit",l.NominalValue.Unit,type(l.NominalValue.Unit))
                                    ifc_spreadsheet.set(str('C'+str(n)), l.NominalValue.is_a())
                                    if l.NominalValue.is_a() in ['IfcLabel','IfcText','IfcIdentifier','IfcDescriptiveMeasure']:
                                        ifc_spreadsheet.set(str('D'+str(n)), "'" + str(l.NominalValue.wrappedValue.encode("utf8")))
                                    else :
                                        ifc_spreadsheet.set(str('D'+str(n)), str(l.NominalValue.wrappedValue))
                                    if hasattr(l.NominalValue,'Unit') :
                                        ifc_spreadsheet.set(str('E'+str(n)), str(l.NominalValue.Unit))
                                n += 1
                        obj.IfcProperties = ifc_spreadsheet
                elif hasattr(obj,"IfcAttributes"):
                    a = obj.IfcAttributes
                    for c in properties[pid].keys():
                        for p in properties[pid][c]:
                            l = ifcfile[p]
                            if l.is_a("IfcPropertySingleValue"):
                                a[l.Name.encode("utf8")] = str(l.NominalValue)
                    obj.IfcAttributes = a

            # color
            if FreeCAD.GuiUp and (pid in colors) and hasattr(obj.ViewObject,"ShapeColor"):
                if DEBUG: print "    setting color: ",int(colors[pid][0]*255),"/",int(colors[pid][1]*255),"/",int(colors[pid][2]*255)
                obj.ViewObject.ShapeColor = colors[pid]

            # if DEBUG is on, recompute after each shape
            if DEBUG: FreeCAD.ActiveDocument.recompute()

        count += 1
        progressbar.next()

    progressbar.stop()
    FreeCAD.ActiveDocument.recompute()

    if MERGE_MODE_STRUCT == 2:

        if DEBUG: print "Joining Structural shapes..."

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
    else:

        if DEBUG: print "Processing Struct relationships..."

        # groups
        for host,children in groups.items():
            if ifcfile[host].is_a("IfcStructuralAnalysisModel"):
                # print host, ' --> ', children
                obj =  FreeCAD.ActiveDocument.addObject("App::DocumentObjectGroup","AnalysisModel")
                objects[host] = obj
                if host in objects.keys():
                    cobs = [objects[child] for child in children if child in objects.keys()]
                    if cobs:
                        if DEBUG: print "adding ",len(cobs), " object(s) to ", objects[host].Label
                        Arch.addComponents(cobs,objects[host])
                        if DEBUG: FreeCAD.ActiveDocument.recompute()

        if MERGE_MODE_ARCH > 2:  # if ArchObj is compound or ArchObj not imported
            FreeCAD.ActiveDocument.recompute()

            # cleaning bad shapes
            for obj in objects.values():
                if obj.isDerivedFrom("Part::Feature"):
                    if obj.Shape.isNull():
                        Arch.rebuildArchShape(obj)


    if MERGE_MODE_ARCH == 3:

        if DEBUG: print "Joining Arch shapes..."

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

    else:

        if DEBUG: print "Processing Arch relationships..."

        # subtractions
        if SEPARATE_OPENINGS:
            for subtraction in subtractions:
                if (subtraction[0] in objects.keys()) and (subtraction[1] in objects.keys()):
                    if DEBUG: print "subtracting ",objects[subtraction[0]].Label, " from ", objects[subtraction[1]].Label
                    Arch.removeComponents(objects[subtraction[0]],objects[subtraction[1]])
                    if DEBUG: FreeCAD.ActiveDocument.recompute()

        # additions
        for host,children in additions.items():
            if host in objects.keys():
                cobs = [objects[child] for child in children if child in objects.keys()]
                if cobs:
                    if DEBUG and (len(cobs) > 10) and ( not(Draft.getType(objects[host]) in ["Site","Building","Floor"])):
                        # avoid huge fusions
                        print "more than 10 shapes to add: skipping."
                    else:
                        if DEBUG: print "adding ",len(cobs), " object(s) to ", objects[host].Label
                        Arch.addComponents(cobs,objects[host])
                        if DEBUG: FreeCAD.ActiveDocument.recompute()

        FreeCAD.ActiveDocument.recompute()

        # cleaning bad shapes
        for obj in objects.values():
            if obj.isDerivedFrom("Part::Feature"):
                if obj.Shape.isNull():
                    Arch.rebuildArchShape(obj)

    FreeCAD.ActiveDocument.recompute()

    # 2D elements

    if DEBUG and annotations: print "Creating 2D geometry..."

    for annotation in annotations:
        aid = annotation.id()
        if aid in skip: continue # user given id skip list
        if "IfcAnnotation" in SKIP: continue # preferences-set type skip list
        name = "Annotation"
        if annotation.Name:
            name = annotation.Name.encode("utf8")
        if PREFIX_NUMBERS: name = "ID" + str(aid) + " " + name
        shapes2d = []
        for repres in annotation.Representation.Representations:
            shapes2d.extend(setRepresentation(repres))
        if shapes2d:
            sh = Part.makeCompound(shapes2d)
            pc = str(int((float(count)/(len(products)+len(annotations))*100)))+"% "
            if DEBUG: print pc,"creating object ",aid," : Annotation with shape: ",sh
            o = FreeCAD.ActiveDocument.addObject("Part::Feature",name)
            o.Shape = sh
        count += 1

    FreeCAD.ActiveDocument.recompute()

    # Materials

    if DEBUG and materials: print "Creating materials..."

    fcmats = {}
    for material in materials:
        name = "Material"
        if material.Name:
            name = material.Name.encode("utf8")
        if MERGE_MATERIALS and (name in fcmats.keys()):
            mat = fcmats[name]
        else:
            mat = Arch.makeMaterial(name=name)
            mdict = {}
            if material.id() in colors:
                mdict["Color"] = str(colors[material.id()])
            if mdict:
                mat.Material = mdict
            fcmats[name] = mat
        for o,m in mattable.items():
            if m == material.id():
                if o in objects:
                    if hasattr(objects[o],"BaseMaterial"):
                        objects[o].BaseMaterial = mat

    FreeCAD.ActiveDocument.recompute()

    if FreeCAD.GuiUp:
        import FreeCADGui
        FreeCADGui.SendMsgToActiveView("ViewFit")
    print "Finished importing."
    return doc


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
        owner = s[0]
        email = s[1].strip(">")
    global template
    template = ifctemplate.replace("$version",version[0]+"."+version[1]+" build "+version[2])
    template = template.replace("$ifcschema",ifcopenshell.schema_identifier)
    template = template.replace("$owner",owner)
    template = template.replace("$company",FreeCAD.ActiveDocument.Company)
    template = template.replace("$email",email)
    template = template.replace("$now",str(int(time.time())))
    template = template.replace("$projectid",FreeCAD.ActiveDocument.Uid[:22].replace("-","_"))
    template = template.replace("$project",FreeCAD.ActiveDocument.Name)
    template = template.replace("$filename",filename)
    template = template.replace("$timestamp",str(time.strftime("%Y-%m-%dT%H:%M:%S", time.gmtime())))
    templatefile = tempfile.mkstemp(suffix=".ifc")[1]
    of = pyopen(templatefile,"wb")
    of.write(template.encode("utf8"))
    of.close()
    global ifcfile, surfstyles, clones, sharedobjects
    ifcfile = ifcopenshell.open(templatefile)
    history = ifcfile.by_type("IfcOwnerHistory")[0]
    context = ifcfile.by_type("IfcGeometricRepresentationContext")[0]
    project = ifcfile.by_type("IfcProject")[0]
    objectslist = Draft.getGroupContents(exportList,walls=True,addgroups=True)
    objectslist = Arch.pruneIncluded(objectslist)
    products = {} # { Name: IfcEntity, ... }
    surfstyles = {} # { (r,g,b): IfcEntity, ... }
    clones = {} # { Basename:[Clonename1,Clonename2,...] }
    sharedobjects = {} # { BaseName: IfcRepresentationMap }
    count = 1

    # build clones table
    if CREATE_CLONES:
        for o in objectslist:
            b = Draft.getCloneBase(o,strict=True)
            if b:
                clones.setdefault(b.Name,[]).append(o.Name)
            
    #print "clones table: ",clones
    #print objectslist

    # products
    for obj in objectslist:

        # getting generic data
        name = str(obj.Label.encode("utf8"))
        description = str(obj.Description) if hasattr(obj,"Description") else ""

        # getting uid
        uid = None
        if hasattr(obj,"IfcAttributes"):
            if "IfcUID" in obj.IfcAttributes.keys():
                uid = str(obj.IfcAttributes["IfcUID"])
        if not uid:
            uid = ifcopenshell.guid.compress(uuid.uuid1().hex)
            # storing the uid for further use
            if STORE_UID and hasattr(obj,"IfcAttributes"):
                d = obj.IfcAttributes
                d["IfcUID"] = uid
                obj.IfcAttributes = d

        # setting the IFC type + name conversions
        if hasattr(obj,"Role"):
            ifctype = obj.Role.replace(" ","")
        else:
            ifctype = Draft.getType(obj)
        if ifctype in translationtable.keys():
            ifctype = translationtable[ifctype]
        ifctype = "Ifc" + ifctype
        if ifctype == "IfcGroup":
            continue
        ifctypes = []
        for v in typesmap.values():
            ifctypes.extend(v)
        if not ifctype in ifctypes:
            ifctype = "IfcBuildingElementProxy"

        # getting the "Force BREP" flag
        brepflag = False
        if hasattr(obj,"IfcAttributes"):
            if "FlagForceBrep" in obj.IfcAttributes.keys():
                if obj.IfcAttributes["FlagForceBrep"] == "True":
                    brepflag = True

        # getting the representation
        representation,placement,shapetype = getRepresentation(ifcfile,context,obj,forcebrep=(brepflag or FORCE_BREP))

        if DEBUG: print str(count).ljust(3)," : ", ifctype, " (",shapetype,") : ",name

        # setting the arguments
        args = [uid,history,name,description,None,placement,representation,None]
        if ifctype in ["IfcSlab","IfcFooting","IfcRoof"]:
            args = args + ["NOTDEFINED"]
        elif ifctype in ["IfcWindow","IfcDoor"]:
            args = args + [obj.Width.Value/1000.0, obj.Height.Value/1000.0]
        elif ifctype == "IfcSpace":
            args = args + ["ELEMENT","INTERNAL",obj.Shape.BoundBox.ZMin/1000.0]
        elif ifctype == "IfcBuildingElementProxy":
            args = args + ["ELEMENT"]
        elif ifctype == "IfcSite":
            latitude = None
            longitude = None
            elevation = None
            landtitlenumber = None
            address = None
            args = args + ["ELEMENT",latitude,longitude,elevation,landtitlenumber,address]
        elif ifctype == "IfcBuilding":
            args = args + ["ELEMENT",None,None,None]
        elif ifctype == "IfcBuildingStorey":
            args = args + ["ELEMENT",obj.Placement.Base.z]

        # creating the product
        product = getattr(ifcfile,"create"+ifctype)(*args)
        products[obj.Name] = product

        # additions
        if hasattr(obj,"Additions") and (shapetype == "extrusion"):
            for o in obj.Additions:
                r2,p2,c2 = getRepresentation(ifcfile,context,o,forcebrep=True)
                if DEBUG: print "      adding ",c2," : ",o.Label
                prod2 = ifcfile.createIfcBuildingElementProxy(ifcopenshell.guid.compress(uuid.uuid1().hex),history,o.Label.encode("utf8"),None,None,p2,r2,None,"ELEMENT")
                ifcfile.createIfcRelAggregates(ifcopenshell.guid.compress(uuid.uuid1().hex),history,'Addition','',product,[prod2])

        # subtractions
        if hasattr(obj,"Subtractions") and (shapetype == "extrusion"):
            for o in obj.Subtractions:
                r2,p2,c2 = getRepresentation(ifcfile,context,o,forcebrep=True,subtraction=True)
                if DEBUG: print "      subtracting ",c2," : ",o.Label
                prod2 = ifcfile.createIfcOpeningElement(ifcopenshell.guid.compress(uuid.uuid1().hex),history,o.Label.encode("utf8"),None,None,p2,r2,None)
                ifcfile.createIfcRelVoidsElement(ifcopenshell.guid.compress(uuid.uuid1().hex),history,'Subtraction','',product,prod2)

        # properties
        ifcprop = False
        if hasattr(obj,"IfcProperties"):
            if obj.IfcProperties:
                ifcprop = True
                if DEBUG : print("      adding ifc properties")
                if obj.IfcProperties.TypeId == 'Spreadsheet::Sheet':
                    sheet = obj.IfcProperties
                    propertiesDic = {}
                    categories = []
                    n=2
                    cell = True
                    while cell == True :
                        if hasattr(sheet,'A'+str(n)):
                            cat = sheet.get('A'+str(n))
                            key = sheet.get('B'+str(n))
                            tp = sheet.get('C'+str(n))
                            if hasattr(sheet,'D'+str(n)):
                                val = sheet.get('D'+str(n))
                            else:
                                val = ''
                            if isinstance(key, unicode):
                                key = key.encode("utf8")
                            else :
                                key = str(key)
                            tp = tp.encode("utf8")
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
                            if cat in categories :
                                propertiesDic[cat].append({"key":key,"tp":tp,"val":val,"unit":unit})
                            else:
                                propertiesDic[cat] = [{"key":key,"tp":tp,"val":val,"unit":unit}]
                                categories.append(cat)
                            n += 1
                        else:
                            cell = False
                    for cat in propertiesDic:
                        props = []
                        for prop in propertiesDic[cat] :
                            if DEBUG :
                                print("key",prop["key"],type(prop["key"]))
                                print("tp",prop["tp"],type(prop["tp"]))
                                print("val",prop["val"],type(prop["val"]))
                            props.append(ifcfile.createIfcPropertySingleValue(prop["key"],None,ifcfile.create_entity(prop["tp"],prop["val"]),None))
                        pset = ifcfile.createIfcPropertySet(ifcopenshell.guid.compress(uuid.uuid1().hex),history,cat,None,props)
                        ifcfile.createIfcRelDefinesByProperties(ifcopenshell.guid.compress(uuid.uuid1().hex),history,None,None,[product],pset)
        if (not ifcprop) and hasattr(obj,"IfcAttributes"):
            if obj.IfcAttributes:
                ifcprop = True
                if DEBUG : print("      adding ifc attributes")
                props = []
                for key in obj.IfcAttributes:
                    if not (key in ["IfcUID","FlagForceBrep"]):
                        r = obj.IfcAttributes[key].strip(")").split("(")
                        if len(r) == 1:
                            tp = "IfcText"
                            val = r[0]
                        else:
                            tp = r[0]
                            val = "(".join(r[1:])
                            val = val.strip("'")
                            val = val.strip('"')
                            if DEBUG: print "      property ",key," : ",val.encode("utf8"), " (", str(tp), ")"
                            if tp in ["IfcLabel","IfcText","IfcIdentifier",'IfcDescriptiveMeasure']:
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
                        props.append(ifcfile.createIfcPropertySingleValue(str(key),None,ifcfile.create_entity(str(tp),val),None))
                if props:
                    pset = ifcfile.createIfcPropertySet(ifcopenshell.guid.compress(uuid.uuid1().hex),history,'PropertySet',None,props)
                    ifcfile.createIfcRelDefinesByProperties(ifcopenshell.guid.compress(uuid.uuid1().hex),history,None,None,[product],pset)
        if not ifcprop:
            #if DEBUG : print("no ifc properties to export")
            pass

        count += 1

    # relationships
    sites = []
    buildings = []
    floors = []
    for site in Draft.getObjectsOfType(objectslist,"Site"):
        for building in Draft.getObjectsOfType(site.Group,"Building"):
            for floor in Draft.getObjectsOfType(building.Group,"Floor"):
                children = Draft.getGroupContents(floor,walls=True)
                children = Arch.pruneIncluded(children)
                children = [products[c.Name] for c in children if c.Name in products.keys()]
                floor = products[floor.Name]
                ifcfile.createIfcRelContainedInSpatialStructure(ifcopenshell.guid.compress(uuid.uuid1().hex),history,'StoreyLink','',children,floor)
                floors.append(floor)
            building = products[building.Name]
            if floors:
                ifcfile.createIfcRelAggregates(ifcopenshell.guid.compress(uuid.uuid1().hex),history,'BuildingLink','',building,floors)
            buildings.append(building)
        site = products[site.Name]
        if buildings:
            ifcfile.createIfcRelAggregates(ifcopenshell.guid.compress(uuid.uuid1().hex),history,'SiteLink','',site,buildings)
        sites.append(site)
    if not sites:
        if DEBUG: print "adding default site"
        sites = [ifcfile.createIfcSite(ifcopenshell.guid.compress(uuid.uuid1().hex),history,"Default Site",'',None,None,None,None,"ELEMENT",None,None,None,None,None)]
    ifcfile.createIfcRelAggregates(ifcopenshell.guid.compress(uuid.uuid1().hex),history,'ProjectLink','',project,sites)
    if not buildings:
        if DEBUG: print "adding default building"
        buildings = [ifcfile.createIfcBuilding(ifcopenshell.guid.compress(uuid.uuid1().hex),history,"Default Building",'',None,None,None,None,"ELEMENT",None,None,None)]
        ifcfile.createIfcRelAggregates(ifcopenshell.guid.compress(uuid.uuid1().hex),history,'SiteLink','',sites[0],buildings)
        ifcfile.createIfcRelContainedInSpatialStructure(ifcopenshell.guid.compress(uuid.uuid1().hex),history,'BuildingLink','',products.values(),buildings[0])

    # materials
    materials = {}
    for m in Arch.getDocumentMaterials():
        mat = ifcfile.createIfcMaterial(m.Label.encode("utf8"))
        materials[m.Label] = mat
        rgb = None
        for colorslot in ["Color","DiffuseColor","ViewColor"]:
            if colorslot in m.Material:
                if m.Material[colorslot]:
                    if m.Material[colorslot][0] == "(":
                        rgb = tuple([float(f) for f in m.Material[colorslot].strip("()").split(",")])
                        break
        if rgb:
            col = ifcfile.createIfcColourRgb(None,rgb[0],rgb[1],rgb[2])
            ssr = ifcfile.createIfcSurfaceStyleRendering(col,None,None,None,None,None,None,None,"FLAT")
            iss = ifcfile.createIfcSurfaceStyle(None,"BOTH",[ssr])
            psa = ifcfile.createIfcPresentationStyleAssignment([iss])
            isi = ifcfile.createIfcStyledItem(None,[psa],None)
            isr = ifcfile.createIfcStyledRepresentation(context,"Style","Material",[isi])
            imd = ifcfile.createIfcMaterialDefinitionRepresentation(None,None,[isr],mat)
            relobjs = []
            for o in m.InList:
                if hasattr(o,"BaseMaterial"):
                    if o.BaseMaterial:
                        if o.BaseMaterial.Name == m.Name:
                            if o.Name in products:
                                relobjs.append(products[o.Name])
            if relobjs:
                ifcfile.createIfcRelAssociatesMaterial(ifcopenshell.guid.compress(uuid.uuid1().hex),history,'MaterialLink','',relobjs,mat)

    if DEBUG: print "writing ",filename,"..."

    filename = decode(filename)

    ifcfile.write(filename)
    
    if STORE_UID:
        # some properties might have been changed
        FreeCAD.ActiveDocument.recompute()


def getRepresentation(ifcfile,context,obj,forcebrep=False,subtraction=False,tessellation=1):
    """returns an IfcShapeRepresentation object or None"""

    import Part,math,DraftGeomUtils,DraftVecUtils
    shapes = []
    placement = None
    productdef = None
    shapetype = "no shape"
    tostore = False
    
    # check for clones
    for k,v in clones.items():
        if (obj.Name == k ) or (obj.Name in v):
            if k in sharedobjects:
                # base shape already exists
                repmap = sharedobjects[k]
                pla = obj.Placement
                axis1 = ifcfile.createIfcDirection(tuple(pla.Rotation.multVec(FreeCAD.Vector(1,0,0))))
                axis2 = ifcfile.createIfcDirection(tuple(pla.Rotation.multVec(FreeCAD.Vector(0,1,0))))
                axis3 = ifcfile.createIfcDirection(tuple(pla.Rotation.multVec(FreeCAD.Vector(0,0,1))))
                origin = ifcfile.createIfcCartesianPoint(tuple(FreeCAD.Vector(pla.Base).multiply(0.001)))
                transf = ifcfile.createIfcCartesianTransformationOperator3D(axis1,axis2,origin,1.0,axis3)
                mapitem = ifcfile.createIfcMappedItem(repmap,transf)
                shapes = [mapitem]
                solidType = "MappedRepresentation"
                shapetype = "clone"
            else:
                # base shape not yet created
                tostore = k

    if (not shapes) and (not forcebrep):
        profile = None
        if hasattr(obj,"Proxy"):
            if hasattr(obj.Proxy,"getProfiles"):
                p = obj.Proxy.getProfiles(obj,noplacement=True)
                extrusionv = obj.Proxy.getExtrusionVector(obj,noplacement=True)
                if not DraftVecUtils.isNull(extrusionv):
                    extrusionv.multiply(0.001) # to meters
                    if (len(p) == 1) and extrusionv:
                        p = p[0]
                        p.scale(0.001) # to meters
                        r = obj.Proxy.getPlacement(obj)
                        r.Base = r.Base.multiply(0.001) # to meters

                        if len(p.Edges) == 1:

                            pxvc = ifcfile.createIfcDirection((1.0,0.0))
                            povc = ifcfile.createIfcCartesianPoint((0.0,0.0))
                            pt = ifcfile.createIfcAxis2Placement2D(povc,pxvc)

                            # extruded circle
                            if isinstance(p.Edges[0].Curve,Part.Circle):
                                profile = ifcfile.createIfcCircleProfileDef("AREA",None,pt, p.Edges[0].Curve.Radius)

                            # extruded ellipse
                            elif isinstance(p.Edges[0].Curve,Part.Ellipse):
                                profile = ifcfile.createIfcEllipseProfileDef("AREA",None,pt, p.Edges[0].Curve.MajorRadius, p.Edges[0].Curve.MinorRadius)

                        else:
                            curves = False
                            for e in p.Edges:
                                if isinstance(e.Curve,Part.Circle):
                                    curves = True

                            # extruded polyline
                            if not curves:
                                w = Part.Wire(Part.__sortEdges__(p.Edges))
                                pts = [ifcfile.createIfcCartesianPoint(tuple(v.Point)[:2]) for v in w.Vertexes+[w.Vertexes[0]]]
                                pol = ifcfile.createIfcPolyline(pts)

                            # extruded composite curve
                            else:
                                segments = []
                                last = None
                                edges = Part.__sortEdges__(p.Edges)
                                for e in edges:
                                    if isinstance(e.Curve,Part.Circle):
                                        follow = True
                                        if last:
                                            if not DraftVecUtils.equals(last,e.Vertexes[0].Point):
                                                follow = False
                                                last = e.Vertexes[0].Point
                                            else:
                                                last = e.Vertexes[-1].Point
                                        else:
                                            last = e.Vertexes[-1].Point
                                        p1 = math.degrees(-DraftVecUtils.angle(e.Vertexes[0].Point.sub(e.Curve.Center)))
                                        p2 = math.degrees(-DraftVecUtils.angle(e.Vertexes[-1].Point.sub(e.Curve.Center)))
                                        da = DraftVecUtils.angle(e.valueAt(e.FirstParameter+0.1).sub(e.Curve.Center),e.Vertexes[0].Point.sub(e.Curve.Center))
                                        if p1 < 0:
                                            p1 = 360 + p1
                                        if p2 < 0:
                                            p2 = 360 + p2
                                        if da > 0:
                                            follow = not(follow)
                                        xvc =       ifcfile.createIfcDirection((1.0,0.0))
                                        ovc =       ifcfile.createIfcCartesianPoint(tuple(e.Curve.Center)[:2])
                                        plc =       ifcfile.createIfcAxis2Placement2D(ovc,xvc)
                                        cir =       ifcfile.createIfcCircle(plc,e.Curve.Radius)
                                        curve =     ifcfile.createIfcTrimmedCurve(cir,[ifcfile.createIfcParameterValue(p1)],[ifcfile.createIfcParameterValue(p2)],follow,"PARAMETER")

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
                                        pts =     [ifcfile.createIfcCartesianPoint(tuple(v)[:2]) for v in verts]
                                        curve =   ifcfile.createIfcPolyline(pts)
                                    segment = ifcfile.createIfcCompositeCurveSegment("CONTINUOUS",True,curve)
                                    segments.append(segment)

                                pol = ifcfile.createIfcCompositeCurve(segments,False)
                            profile = ifcfile.createIfcArbitraryClosedProfileDef("AREA",None,pol)

        if profile:
            xvc =       ifcfile.createIfcDirection(tuple(r.Rotation.multVec(FreeCAD.Vector(1,0,0))))
            zvc =       ifcfile.createIfcDirection(tuple(r.Rotation.multVec(FreeCAD.Vector(0,0,1))))
            ovc =       ifcfile.createIfcCartesianPoint(tuple(r.Base))
            lpl =       ifcfile.createIfcAxis2Placement3D(ovc,zvc,xvc)
            edir =      ifcfile.createIfcDirection(tuple(FreeCAD.Vector(extrusionv).normalize()))
            shape =     ifcfile.createIfcExtrudedAreaSolid(profile,lpl,edir,extrusionv.Length)
            shapes.append(shape)
            solidType = "SweptSolid"
            shapetype = "extrusion"

    if not shapes:
        # brep representation
        fcshape = None
        solidType = "Brep"
        if subtraction:
            if hasattr(obj,"Proxy"):
                if hasattr(obj.Proxy,"getSubVolume"):
                    fcshape = obj.Proxy.getSubVolume(obj)
        if not fcshape:
            if hasattr(obj,"Shape"):
                if obj.Shape:
                    if not obj.Shape.isNull():
                        fcshape = obj.Shape
            elif hasattr(obj,"Terrain"):
                if obj.Terrain:
                    if hasattr(obj.Terrain,"Shape"):
                        if obj.Terrain.Shape:
                            if not obj.Terrain.Shape.isNull():
                                    fcshape = obj.Terrain.Shape
        if fcshape:
            solids = []
            if fcshape.Solids:
                dataset = fcshape.Solids
            else:
                dataset = fcshape.Shells
                if DEBUG: print "Warning! object contains no solids"
            for fcsolid in dataset:
                fcsolid.scale(0.001) # to meters
                faces = []
                curves = False
                shapetype = "brep"
                for fcface in fcsolid.Faces:
                    for e in fcface.Edges:
                        if DraftGeomUtils.geomType(e) != "Line":
                            if e.curvatureAt(e.FirstParameter+(e.LastParameter-e.FirstParameter)/2) > 0.0001:
                                curves = True
                                break
                if curves:
                    joinfacets = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch").GetBool("ifcJoinCoplanarFacets",False)
                    usedae = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch").GetBool("ifcUseDaeOptions",False)
                    if not joinfacets:
                        shapetype = "triangulated"
                        if usedae:
                            import importDAE
                            tris = importDAE.triangulate(fcsolid)
                        else:
                            tris = fcsolid.tessellate(tessellation)
                        for tri in tris[1]:
                            pts =   [ifcfile.createIfcCartesianPoint(tuple(tris[0][i])) for i in tri]
                            loop =  ifcfile.createIfcPolyLoop(pts)
                            bound = ifcfile.createIfcFaceOuterBound(loop,True)
                            face =  ifcfile.createIfcFace([bound])
                            faces.append(face)
                            fcsolid = Part.Shape() # empty shape so below code is not executed
                    else:
                        fcsolid = Arch.removeCurves(fcsolid,dae=usedae)
                        if not fcsolid:
                            if DEBUG: print "Error: Unable to triangulate shape"
                            fcsolid = Part.Shape()
                    
                for fcface in fcsolid.Faces:
                    loops = []
                    verts = [v.Point for v in Part.Wire(Part.__sortEdges__(fcface.OuterWire.Edges)).Vertexes]
                    c = fcface.CenterOfMass
                    v1 = verts[0].sub(c)
                    v2 = verts[1].sub(c)
                    n = fcface.normalAt(0,0)
                    if DraftVecUtils.angle(v2,v1,n) >= 0:
                        verts.reverse() # inverting verts order if the direction is couterclockwise
                    pts =   [ifcfile.createIfcCartesianPoint(tuple(v)) for v in verts]
                    loop =  ifcfile.createIfcPolyLoop(pts)
                    bound = ifcfile.createIfcFaceOuterBound(loop,True)
                    loops.append(bound)
                    for wire in fcface.Wires:
                        if wire.hashCode() != fcface.OuterWire.hashCode():
                            verts = [v.Point for v in Part.Wire(Part.__sortEdges__(wire.Edges)).Vertexes]
                            v1 = verts[0].sub(c)
                            v2 = verts[1].sub(c)
                            if DraftVecUtils.angle(v2,v1,DraftVecUtils.neg(n)) >= 0:
                                verts.reverse()
                            pts =   [ifcfile.createIfcCartesianPoint(tuple(v)) for v in verts]
                            loop =  ifcfile.createIfcPolyLoop(pts)
                            bound = ifcfile.createIfcFaceBound(loop,True)
                            loops.append(bound)
                    face =  ifcfile.createIfcFace(loops)
                    faces.append(face)

                if faces:
                    shell = ifcfile.createIfcClosedShell(faces)
                    shape = ifcfile.createIfcFacetedBrep(shell)
                    shapes.append(shape)

    if shapes:

        if tostore:
            subrep = ifcfile.createIfcShapeRepresentation(context,'Body',solidType,shapes)
            xvc = ifcfile.createIfcDirection((1.0,0.0,0.0))
            zvc = ifcfile.createIfcDirection((0.0,0.0,1.0))
            ovc = ifcfile.createIfcCartesianPoint((0.0,0.0,0.0))
            gpl = ifcfile.createIfcAxis2Placement3D(ovc,zvc,xvc)
            repmap = ifcfile.createIfcRepresentationMap(gpl,subrep)
            pla = FreeCAD.ActiveDocument.getObject(k).Placement
            axis1 = ifcfile.createIfcDirection(tuple(pla.Rotation.multVec(FreeCAD.Vector(1,0,0))))
            axis2 = ifcfile.createIfcDirection(tuple(pla.Rotation.multVec(FreeCAD.Vector(0,1,0))))
            origin = ifcfile.createIfcCartesianPoint(tuple(FreeCAD.Vector(pla.Base).multiply(0.001)))
            axis3 = ifcfile.createIfcDirection(tuple(pla.Rotation.multVec(FreeCAD.Vector(0,0,1))))
            transf = ifcfile.createIfcCartesianTransformationOperator3D(axis1,axis2,origin,1.0,axis3)
            mapitem = ifcfile.createIfcMappedItem(repmap,transf)
            shapes = [mapitem]
            sharedobjects[k] = repmap
            solidType = "MappedRepresentation"

        # set surface style
        if FreeCAD.GuiUp and (not subtraction) and hasattr(obj.ViewObject,"ShapeColor"):
            # only set a surface style if the object has no material.
            # apparently not needed, no harm in having both.
            #m = False
            #if hasattr(obj,"BaseMaterial"):
            #    if obj.BaseMaterial:
            #        if "Color" in obj.BaseMaterial.Material:
            #            m = True
            #if not m:
            rgb = obj.ViewObject.ShapeColor[:3]
            if rgb in surfstyles:
                psa = surfstyles[rgb]
            else:
                col = ifcfile.createIfcColourRgb(None,rgb[0],rgb[1],rgb[2])
                ssr = ifcfile.createIfcSurfaceStyleRendering(col,None,None,None,None,None,None,None,"FLAT")
                iss = ifcfile.createIfcSurfaceStyle(None,"BOTH",[ssr])
                psa = ifcfile.createIfcPresentationStyleAssignment([iss])
                surfstyles[rgb] = psa
            for shape in shapes:
                isi = ifcfile.createIfcStyledItem(shape,[psa],None)

        xvc = ifcfile.createIfcDirection((1.0,0.0,0.0))
        zvc = ifcfile.createIfcDirection((0.0,0.0,1.0))
        ovc = ifcfile.createIfcCartesianPoint((0.0,0.0,0.0))
        gpl = ifcfile.createIfcAxis2Placement3D(ovc,zvc,xvc)
        placement = ifcfile.createIfcLocalPlacement(None,gpl)
        representation = ifcfile.createIfcShapeRepresentation(context,'Body',solidType,shapes)
        productdef = ifcfile.createIfcProductDefinitionShape(None,None,[representation])

    return productdef,placement,shapetype


def setRepresentation(representation):
    """Returns a shape from a 2D IfcShapeRepresentation"""

    def getPolyline(ent):
        pts = []
        for p in ent.Points:
            c = p.Coordinates
            pts.append(FreeCAD.Vector(c[0],c[1],c[2] if len(c) > 2 else 0))
        return Part.makePolygon(pts)

    def getCircle(ent):
        c = ent.Position.Location.Coordinates
        c = FreeCAD.Vector(c[0],c[1],c[2] if len(c) > 2 else 0)
        r = ent.Radius
        return Part.makeCircle(r,c)

    result = []
    if representation.is_a("IfcShapeRepresentation"):
        for item in representation.Items:
            if item.is_a("IfcGeometricCurveSet"):
                for el in item.Elements:
                    if el.is_a("IfcPolyline"):
                        result.append(getPolyline(el))
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
    return result
    
    
def getRotation(entity):
    "returns a FreeCAD rotation from an IfcProduct with a IfcMappedItem representation"
    try:
        rmap = entity.Representation.Representations[0].Items[0].MappingTarget
        u = FreeCAD.Vector(rmap.Axis1.DirectionRatios)
        v = FreeCAD.Vector(rmap.Axis2.DirectionRatios)
        w = FreeCAD.Vector(rmap.Axis3.DirectionRatios)
    except AttributeError:
        return FreeCAD.Rotation()
    import WorkingPlane
    p = WorkingPlane.plane(u=u,v=v,w=w)
    return p.getRotation().Rotation
