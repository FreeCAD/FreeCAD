# ***************************************************************************
# *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
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

__title__ =  "FreeCAD IFC export"
__author__ = "Yorik van Havre","Jonathan Wiedemann","Bernd Hahnebach"
__url__ =    "https://www.freecadweb.org"

import six
import os
import time
import tempfile
import math

import FreeCAD
import Part
import Draft
import Arch
import DraftVecUtils
import ArchIFCSchema
import exportIFCHelper

from DraftGeomUtils import vec
from importIFCHelper import dd2dms
from importIFCHelper import decode


## @package exportIFC
#  \ingroup ARCH
#  \brief IFC file format exporter
#
#  This module provides tools to export IFC files.

if open.__module__ in ['__builtin__','io']:
    pyopen = open
    # pyopen is used in exporter to open a file in Arch


# ************************************************************************************************
# ********** templates and other definitions ****

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
#5=IFCOWNERHISTORY(#3,#4,$,.ADDED.,$now,#3,#4,$now);
#6=IFCDIRECTION((1.,0.,0.));
#7=IFCDIRECTION((0.,0.,1.));
#8=IFCCARTESIANPOINT((0.,0.,0.));
#9=IFCAXIS2PLACEMENT3D(#8,#7,#6);
#10=IFCDIRECTION((0.,1.,0.));
#12=IFCDIMENSIONALEXPONENTS(0,0,0,0,0,0,0);
#13=IFCSIUNIT(*,.LENGTHUNIT.,$,.METRE.);
#14=IFCSIUNIT(*,.AREAUNIT.,$,.SQUARE_METRE.);
#15=IFCSIUNIT(*,.VOLUMEUNIT.,$,.CUBIC_METRE.);
#16=IFCSIUNIT(*,.PLANEANGLEUNIT.,$,.RADIAN.);
#17=IFCMEASUREWITHUNIT(IFCPLANEANGLEMEASURE(0.017453292519943295),#16);
#18=IFCCONVERSIONBASEDUNIT(#12,.PLANEANGLEUNIT.,'DEGREE',#17);
ENDSEC;
END-ISO-10303-21;
"""


# ************************************************************************************************
# ********** get the prefs, available in import and export ****************
def getPreferences():

    """retrieves IFC preferences"""

    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")

    if FreeCAD.GuiUp and p.GetBool("ifcShowDialog",False):
        import FreeCADGui
        FreeCADGui.showPreferences("Import-Export",0)
    ifcunit = p.GetInt("ifcUnit",0)
    f = 0.001
    u = "metre"
    if ifcunit == 1:
        f = 0.00328084
        u = "foot"
    #if ifcunit == "inch":
    #    f = 0.03937008
    # not yet implemented, and I don't even know if it is interesting to do it.
    # the only real use of these units is to make revit choose which mode to work with

    preferences = {
        'DEBUG': p.GetBool("ifcDebug",False),
        'CREATE_CLONES': p.GetBool("ifcCreateClones",True),
        'FORCE_BREP': p.GetBool("ifcExportAsBrep",False),
        'STORE_UID': p.GetBool("ifcStoreUid",True),
        'SERIALIZE': p.GetBool("ifcSerialize",False),
        'EXPORT_2D': p.GetBool("ifcExport2D",True),
        'FULL_PARAMETRIC': p.GetBool("IfcExportFreeCADProperties",False),
        'ADD_DEFAULT_SITE': p.GetBool("IfcAddDefaultSite",False),
        'ADD_DEFAULT_STOREY': p.GetBool("IfcAddDefaultStorey",False),
        'ADD_DEFAULT_BUILDING': p.GetBool("IfcAddDefaultBuilding",True),
        'IFC_UNIT': u,
        'SCALE_FACTOR': f
    }

    return preferences


# ************************************************************************************************
# ********** export IFC ****************
def export(exportList,filename,colors=None,preferences=None):

    """export(exportList,filename,colors=None,preferences=None) -- exports FreeCAD contents to an IFC file.
    colors is an optional dictionary of objName:shapeColorTuple or objName:diffuseColorList elements
    to be used in non-GUI mode if you want to be able to export colors."""

    if preferences is None:
        preferences = getPreferences()

    try:
        global ifcopenshell
        import ifcopenshell
    except:
        FreeCAD.Console.PrintError("IfcOpenShell was not found on this system. IFC support is disabled\n")
        FreeCAD.Console.PrintMessage("Visit https://www.freecadweb.org/wiki/Arch_IFC to learn how to install it\n")
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
    if preferences['DEBUG']: print("Exporting an",schema,"file...")
    template = template.replace("$ifcschema",schema)
    template = template.replace("$owner",owner)
    template = template.replace("$company",FreeCAD.ActiveDocument.Company)
    template = template.replace("$email",email)
    template = template.replace("$now",str(int(time.time())))
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
    ifcfile = exportIFCHelper.writeUnits(ifcfile,preferences["IFC_UNIT"])
    history = ifcfile.by_type("IfcOwnerHistory")[0]
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
    objectslist = [obj for obj in objectslist if Draft.getType(obj) not in ["Dimension","Material","MaterialContainer","WorkingPlaneProxy"]]
    if preferences['FULL_PARAMETRIC']:
        objectslist = Arch.getAllChildren(objectslist)

    contextCreator = exportIFCHelper.ContextCreator(ifcfile, objectslist)
    context = contextCreator.model_view_subcontext
    project = contextCreator.project
    objectslist = [obj for obj in objectslist if obj != contextCreator.project_object]

    if Draft.getObjectsOfType(objectslist, "Site"):  # we assume one site and one representation context only
        decl = Draft.getObjectsOfType(objectslist, "Site")[0].Declination.getValueAs(FreeCAD.Units.Radian)
        contextCreator.model_context.TrueNorth.DirectionRatios = (math.cos(decl+math.pi/2), math.sin(decl+math.pi/2))

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
    ifcbin = exportIFCHelper.recycler(ifcfile)

    # build clones table

    if preferences['CREATE_CLONES']:
        for o in objectslist:
            b = Draft.getCloneBase(o,strict=True)
            if b:
                clones.setdefault(b.Name,[]).append(o.Name)

    #print("clones table: ",clones)
    #print(objectslist)

    # testing if more than one site selected (forbidden in IFC)
    # TODO: Moult: This is not forbidden in IFC.

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
            uid = ifcopenshell.guid.new()
            # storing the uid for further use
            if preferences['STORE_UID'] and hasattr(obj,"IfcData"):
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
                    p1 = ifcbin.createIfcCartesianPoint(tuple(FreeCAD.Vector(ax[0]).multiply(preferences['SCALE_FACTOR'])[:2]))
                    p2 = ifcbin.createIfcCartesianPoint(tuple(FreeCAD.Vector(ax[1]).multiply(preferences['SCALE_FACTOR'])[:2]))
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
                if preferences['DEBUG']: print(str(count).ljust(3)," : ", ifctype, " (",str(len(ifcpols)),"axes ) : ",name)
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

        representation,placement,shapetype = getRepresentation(
            ifcfile,
            context,
            obj,
            forcebrep=(brepflag or preferences['FORCE_BREP']),
            colors=colors,
            preferences=preferences
        )
        if getstd:
            if isStandardCase(obj,ifctype):
                ifctype += "StandardCase"

        if preferences['DEBUG']: print(str(count).ljust(3)," : ", ifctype, " (",shapetype,") : ",name)

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
                "RefElevation":obj.Elevation.Value*preferences['SCALE_FACTOR'],
                "SiteAddress":buildAddress(obj,ifcfile),
                "CompositionType": "ELEMENT"
            })
        if schema == "IFC2X3":
            kwargs = exportIFC2X3Attributes(obj, kwargs, preferences['SCALE_FACTOR'])
        else:
            kwargs = exportIfcAttributes(obj, kwargs, preferences['SCALE_FACTOR'])

        # creating the product

        #print(obj.Label," : ",ifctype," : ",kwargs)
        product = getattr(ifcfile,"create"+ifctype)(**kwargs)
        products[obj.Name] = product
        if ifctype in ["IfcBuilding","IfcBuildingStorey","IfcSite","IfcSpace"]:
            spatialelements[obj.Name] = product

        # additions

        if hasattr(obj,"Additions") and (shapetype in ["extrusion","no shape"]):
            for o in obj.Additions:
                r2,p2,c2 = getRepresentation(ifcfile,context,o,colors=colors,preferences=preferences)
                if preferences['DEBUG']: print("      adding ",c2," : ",o.Label)
                l = o.Label
                if six.PY2:
                    l = l.encode("utf8")
                prod2 = ifcfile.createIfcBuildingElementProxy(
                    ifcopenshell.guid.new(),
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
                    ifcopenshell.guid.new(),
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
                r2,p2,c2 = getRepresentation(ifcfile,context,o,subtraction=True,colors=colors,preferences=preferences)
                if preferences['DEBUG']: print("      subtracting ",c2," : ",o.Label)
                l = o.Label
                if six.PY2:
                    l = l.encode("utf8")
                prod2 = ifcfile.createIfcOpeningElement(
                    ifcopenshell.guid.new(),
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
                    ifcopenshell.guid.new(),
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
                        pset, pname, ptype, pvalue = getPropertyData(key,value,preferences)
                        if pvalue is None:
                            if preferences['DEBUG']: print("      property ", pname," ignored because no value found.")
                            continue
                        p = ifcbin.createIfcPropertySingleValue(str(pname),str(ptype),pvalue)
                        psets.setdefault(pset,[]).append(p)
                    for pname,props in psets.items():
                        pset = ifcfile.createIfcPropertySet(
                            ifcopenshell.guid.new(),
                            history,
                            pname,
                            None,
                            props
                        )
                        ifcfile.createIfcRelDefinesByProperties(
                            ifcopenshell.guid.new(),
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
                    n = 2
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
                            if preferences['DEBUG']:
                                print("key",prop["key"],type(prop["key"]))
                                print("tp",prop["tp"],type(prop["tp"]))
                                print("val",prop["val"],type(prop["val"]))
                            if tp.lower().startswith("ifc"):
                                props.append(ifcbin.createIfcPropertySingleValue(prop["key"],prop["tp"],prop["val"]))
                            else:
                                print("Unable to create a property of type:",tp)
                        if props:
                            pset = ifcfile.createIfcPropertySet(
                                ifcopenshell.guid.new(),
                                history,cat,
                                None,
                                props
                            )
                            ifcfile.createIfcRelDefinesByProperties(
                                ifcopenshell.guid.new(),
                                history,
                                None,
                                None,
                                [product],
                                pset
                            )

        if hasattr(obj,"IfcData"):

            if obj.IfcData:
                ifcprop = True
                #if preferences['DEBUG'] : print("      adding ifc attributes")
                props = []
                for key in obj.IfcData:
                    if not (key in ["attributes", "complex_attributes", "IfcUID", "FlagForceBrep"]):

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
                            #if preferences['DEBUG']: print("      property ",key," : ",val.encode("utf8"), " (", str(tp), ")")
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
                        ifcopenshell.guid.new(),
                        history,
                        'PropertySet',
                        None,
                        props
                    )
                    ifcfile.createIfcRelDefinesByProperties(
                        ifcopenshell.guid.new(),
                        history,
                        None,
                        None,
                        [product],
                        pset
                    )

        if not ifcprop:
            #if preferences['DEBUG'] : print("no ifc properties to export")
            pass

        # Quantities

        if hasattr(obj,"IfcData"):
            quantities = []
            if ("ExportHeight" in obj.IfcData) and obj.IfcData["ExportHeight"] and hasattr(obj,"Height"):
                quantities.append(ifcfile.createIfcQuantityLength('Height',None,None,obj.Height.Value*preferences['SCALE_FACTOR']))
            if ("ExportWidth" in obj.IfcData) and obj.IfcData["ExportWidth"] and hasattr(obj,"Width"):
                quantities.append(ifcfile.createIfcQuantityLength('Width',None,None,obj.Width.Value*preferences['SCALE_FACTOR']))
            if ("ExportLength" in obj.IfcData) and obj.IfcData["ExportLength"] and hasattr(obj,"Length"):
                quantities.append(ifcfile.createIfcQuantityLength('Length',None,None,obj.Length.Value*preferences['SCALE_FACTOR']))
            if ("ExportHorizontalArea" in obj.IfcData) and obj.IfcData["ExportHorizontalArea"] and hasattr(obj,"HorizontalArea"):
                quantities.append(ifcfile.createIfcQuantityArea('HorizontalArea',None,None,obj.HorizontalArea.Value*(preferences['SCALE_FACTOR']**2)))
            if ("ExportVerticalArea" in obj.IfcData) and obj.IfcData["ExportVerticalArea"] and hasattr(obj,"VerticalArea"):
                quantities.append(ifcfile.createIfcQuantityArea('VerticalArea',None,None,obj.VerticalArea.Value*(preferences['SCALE_FACTOR']**2)))
            if ("ExportVolume" in obj.IfcData) and obj.IfcData["ExportVolume"] and obj.isDerivedFrom("Part::Feature"):
                quantities.append(ifcfile.createIfcQuantityVolume('Volume',None,None,obj.Shape.Volume*(preferences['SCALE_FACTOR']**3)))
            if quantities:
                eltq = ifcfile.createIfcElementQuantity(
                    ifcopenshell.guid.new(),
                    history,
                    "ElementQuantities",
                    None,
                    "FreeCAD",quantities
                )
                ifcfile.createIfcRelDefinesByProperties(
                    ifcopenshell.guid.new(),
                    history,
                    None,
                    None,
                    [product],eltq
                )

        if preferences['FULL_PARAMETRIC']:

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
                                if preferences['DEBUG']: print("Unable to encode property ",prop," of type ",ptype)
                            if itype:
                                # TODO add description
                                if realm == "Gui":
                                    FreeCADGuiProps.append(ifcbin.createIfcPropertySingleValue("FreeCADGui_"+prop,itype,ivalue))
                                else:
                                    FreeCADProps.append(ifcbin.createIfcPropertySingleValue("FreeCAD_"+prop,itype,ivalue))
            if FreeCADProps:
                pset = ifcfile.createIfcPropertySet(
                    ifcopenshell.guid.new(),
                    history,'FreeCADPropertySet',
                    None,
                    FreeCADProps
                )
                ifcfile.createIfcRelDefinesByProperties(
                    ifcopenshell.guid.new(),
                    history,
                    None,
                    None,
                    [product],
                    pset
                )
            if FreeCADGuiProps:
                pset = ifcfile.createIfcPropertySet(
                    ifcopenshell.guid.new(),
                    history,
                    'FreeCADGuiPropertySet',
                    None,
                    FreeCADGuiProps
                )
                ifcfile.createIfcRelDefinesByProperties(
                    ifcopenshell.guid.new(),
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
    # if ifc type is "Undefined" gather elements too

    for bp in Draft.getObjectsOfType(objectslist,"BuildingPart"):
        if bp.IfcType not in ["Site","Building","Building Storey","Space"]:
            if bp.Name in products:
                subs = []
                for c in bp.Group:
                    if c.Name in products:
                        subs.append(products[c.Name])
                        treated.append(c.Name)
                if subs:
                    ifcfile.createIfcRelAggregates(
                        ifcopenshell.guid.new(),
                        history,
                        'Assembly',
                        '',
                        products[bp.Name],
                        subs
                    )

    # storeys

    for floor in Draft.getObjectsOfType(objectslist,"Floor")+Draft.getObjectsOfType(objectslist,"BuildingPart"):
        if (Draft.getType(floor) == "Floor") or (hasattr(floor,"IfcType") and floor.IfcType == "Building Storey"):
            objs = Draft.getGroupContents(floor,walls=True,addgroups=True)
            objs = Arch.pruneIncluded(objs)
            objs.remove(floor) # getGroupContents + addgroups will include the floor itself
            buildingelements, spaces = [], []
            for c in objs:
                if c.Name in products and c.Name not in treated:
                    prod = products[c.Name]
                    if prod.is_a()=='IfcSpace':
                        spaces.append(prod)
                    else:
                        buildingelements.append(prod)
                    treated.append(c.Name)
            f = products[floor.Name]
            if buildingelements:
                ifcfile.createIfcRelContainedInSpatialStructure(
                    ifcopenshell.guid.new(),
                    history,
                    'StoreyLink',
                    '',
                    buildingelements,
                    f
                )
            if spaces:
                ifcfile.createIfcRelAggregates(
                    ifcopenshell.guid.new(),
                    history,
                    'StoreyLink',
                    '',
                    f,
                    spaces
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
                            if Draft.getType(c) in ["Floor","BuildingPart","Space"]:
                                childfloors.append(products[c.Name])
                                treated.append(c.Name)
                            elif not (c.Name in treated):
                                children.append(products[c.Name])
                                treated.append(c.Name)
            b = products[building.Name]
            if children:
                ifcfile.createIfcRelContainedInSpatialStructure(
                    ifcopenshell.guid.new(),
                    history,
                    'BuildingLink',
                    '',
                    children,
                    b
                )
            if childfloors:
                ifcfile.createIfcRelAggregates(
                    ifcopenshell.guid.new(),
                    history,
                    'BuildingLink',
                    '',
                    b,
                    childfloors
                )
            buildings.append(b)
            if not defaulthost and not preferences['ADD_DEFAULT_STOREY']:
                defaulthost = b

    # sites

    for site in exportIFCHelper.getObjectsOfIfcType(objectslist, "Site"):
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

    # add default site, building and storey as required

    if not sites:
        if preferences['ADD_DEFAULT_SITE']:
            if preferences['DEBUG']: print("No site found. Adding default site")
            sites = [ifcfile.createIfcSite(
                ifcopenshell.guid.new(),
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
    if sites:
        ifcfile.createIfcRelAggregates(
            ifcopenshell.guid.new(),
            history,
            'ProjectLink',
            '',
            project,sites
        )
    if not buildings:
        if preferences['ADD_DEFAULT_BUILDING']:
            if preferences['DEBUG']: print("No building found. Adding default building")
            buildings = [ifcfile.createIfcBuilding(
                ifcopenshell.guid.new(),
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
        if buildings and (not sites):
            ifcfile.createIfcRelAggregates(
                ifcopenshell.guid.new(),
                history,
                'ProjectLink',
                '',
                project,buildings
            )
        if floors:
            ifcfile.createIfcRelAggregates(
                ifcopenshell.guid.new(),
                history,
                'BuildingLink',
                '',
                buildings[0],floors
            )
    if sites and buildings:
        ifcfile.createIfcRelAggregates(
            ifcopenshell.guid.new(),
            history,
            'SiteLink',
            '',
            sites[0],
            buildings
        )

    # treat objects that are not related to any site, building or storey

    untreated = []
    for k,v in products.items():
        if not(k in treated):
            if (not buildings) or (k != buildings[0].Name):
                if not(Draft.getType(FreeCAD.ActiveDocument.getObject(k)) in ["Site","Building","Floor","BuildingPart"]):
                    untreated.append(v)
                elif Draft.getType(FreeCAD.ActiveDocument.getObject(k)) == "BuildingPart":
                    if not(FreeCAD.ActiveDocument.getObject(k).IfcType in ["Building","Building Storey","Site","Space"]):
                        # if ifc type is "Undefined" the object is added to untreated
                        untreated.append(v)
    if untreated:
        if not defaulthost:
            if preferences['ADD_DEFAULT_STOREY']:
                if preferences['DEBUG']: print("No floor found. Adding default floor")
                defaulthost = ifcfile.createIfcBuildingStorey(
                    ifcopenshell.guid.new(),
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
                # if preferences['ADD_DEFAULT_STOREY'] is on, we need a building
                # to host it, regardless of preferences['ADD_DEFAULT_BUILDING']
                if not buildings:
                    if preferences['DEBUG']: print("No building found. Adding default building")
                    buildings = [ifcfile.createIfcBuilding(
                        ifcopenshell.guid.new(),
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
                    if sites:
                        ifcfile.createIfcRelAggregates(
                            ifcopenshell.guid.new(),
                            history,
                            'SiteLink',
                            '',
                            sites[0],
                            buildings
                        )
                    else:
                        ifcfile.createIfcRelAggregates(
                            ifcopenshell.guid.new(),
                            history,
                            'ProjectLink',
                            '',
                            project,buildings
                        )
                ifcfile.createIfcRelAggregates(
                    ifcopenshell.guid.new(),
                    history,
                    'DefaultStoreyLink',
                    '',
                    buildings[0],
                    [defaulthost]
                )
            elif buildings:
                defaulthost = buildings[0]
        if defaulthost:
            spaces, buildingelements = [],[]
            for entity in untreated:
                if entity.is_a()=="IfcSpace":
                    spaces.append(entity)
                else:
                    buildingelements.append(entity)
            if spaces:
                ifcfile.createIfcRelAggregates(
                    ifcopenshell.guid.new(),
                    history,
                    'UnassignedObjectsLink',
                    '',
                    defaulthost,
                    spaces
                )
            if buildingelements:
                ifcfile.createIfcRelContainedInSpatialStructure(
                    ifcopenshell.guid.new(),
                    history,
                    'UnassignedObjectsLink',
                    '',
                    buildingelements,
                    defaulthost
            )
        else:
            # no default host: aggregate unassigned objects directly under the IfcProject - WARNING: NON STANDARD
            if preferences['DEBUG']: print("WARNING - Default building generation is disabled. You are producing a non-standard file.")
            ifcfile.createIfcRelAggregates(
                ifcopenshell.guid.new(),
                history,
                'ProjectLink',
                '',
                project,untreated
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
                ifcopenshell.guid.new(),
                history,
                'MaterialLink',
                '',
                relobjs,
                mat
            )

    # 2D objects

    annos = {}
    if preferences['EXPORT_2D']:
        curvestyles = {}
        if annotations and preferences['DEBUG']: print("exporting 2D objects...")
        for anno in annotations:
            xvc = ifcbin.createIfcDirection((1.0,0.0,0.0))
            zvc = ifcbin.createIfcDirection((0.0,0.0,1.0))
            ovc = ifcbin.createIfcCartesianPoint((0.0,0.0,0.0))
            gpl = ifcbin.createIfcAxis2Placement3D(ovc,zvc,xvc)
            placement = ifcbin.createIfcLocalPlacement(gpl)
            if anno.isDerivedFrom("Part::Feature"):
                reps = []
                sh = anno.Shape.copy()
                sh.scale(preferences['SCALE_FACTOR']) # to meters
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
                l = FreeCAD.Vector(anno.Position).multiply(preferences['SCALE_FACTOR'])
                pos = ifcbin.createIfcCartesianPoint((l.x,l.y,l.z))
                tpl = ifcbin.createIfcAxis2Placement3D(pos,None,None)
                s = ";".join(anno.LabelText)
                if six.PY2:
                    s = s.encode("utf8")
                txt = ifcfile.createIfcTextLiteral(s,tpl,"LEFT")
                reps = [txt]
            elif Draft.getType(anno) == "DraftText":
                l = FreeCAD.Vector(anno.Placement.Base).multiply(preferences['SCALE_FACTOR'])
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
                ifcopenshell.guid.new(),
                history,l,
                '',
                None,
                placement,
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
                    ifcopenshell.guid.new(),
                    history,
                    name,
                    '',
                    None
                )
                products[g[0]] = grp
                spatialelements[g[0]] = grp
                ass = ifcfile.createIfcRelAssignsToGroup(
                    ifcopenshell.guid.new(),
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
            ifcopenshell.guid.new(),
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
                if preferences['ADD_DEFAULT_STOREY']:
                    if preferences['DEBUG']: print("No floor found. Adding default floor")
                    defaulthost = ifcfile.createIfcBuildingStorey(
                        ifcopenshell.guid.new(),
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
                    # if preferences['ADD_DEFAULT_STOREY'] is on, we need a
                    # building to host it, regardless of
                    # preferences['ADD_DEFAULT_BUILDING']
                    if not buildings:
                        buildings = [ifcfile.createIfcBuilding(
                            ifcopenshell.guid.new(),
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
                        if sites:
                            ifcfile.createIfcRelAggregates(
                                ifcopenshell.guid.new(),
                                history,
                                'SiteLink',
                                '',
                                sites[0],
                                buildings
                            )
                        else:
                            ifcfile.createIfcRelAggregates(
                                ifcopenshell.guid.new(),
                                history,
                                'ProjectLink',
                                '',
                                project,buildings
                            )
                    ifcfile.createIfcRelAggregates(
                        ifcopenshell.guid.new(),
                        history,
                        'DefaultStoreyLink',
                        '',
                        buildings[0],
                        [defaulthost]
                    )
                elif preferences['ADD_DEFAULT_BUILDING']:
                    if not buildings:
                        defaulthost = ifcfile.createIfcBuilding(
                            ifcopenshell.guid.new(),
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
                        )
                        if sites:
                            ifcfile.createIfcRelAggregates(
                                ifcopenshell.guid.new(),
                                history,
                                'SiteLink',
                                '',
                                sites[0],
                                [defaulthost]
                            )
                        else:
                            ifcfile.createIfcRelAggregates(
                                ifcopenshell.guid.new(),
                                history,
                                'ProjectLink',
                                '',
                                project,
                                [defaulthost]
                            )
            if defaulthost:
                ifcfile.createIfcRelContainedInSpatialStructure(
                    ifcopenshell.guid.new(),
                    history,
                    'AnnotationsLink',
                    '',
                    remaining,
                    defaulthost
                )
            else:
                ifcfile.createIfcRelAggregates(
                    ifcopenshell.guid.new(),
                    history,
                    'ProjectLink',
                    '',
                    project,
                    remaining
                )

    if preferences['DEBUG']: print("writing ",filename,"...")

    filename = decode(filename)

    ifcfile.write(filename)

    if preferences['STORE_UID']:
        # some properties might have been changed
        FreeCAD.ActiveDocument.recompute()

    os.remove(templatefile)

    if preferences['DEBUG'] and ifcbin.compress:
        f = pyopen(filename,"r")
        s = len(f.read().split("\n"))
        f.close()
        print("Compression ratio:",int((float(ifcbin.spared)/(s+ifcbin.spared))*100),"%")
    del ifcbin


# ************************************************************************************************
# ********** helper for export IFC **************

def getPropertyData(key,value,preferences):

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
        if preferences['DEBUG']:print("      unable to export property:",pname,value)
        return pset, pname, ptype, None

    #if preferences['DEBUG']: print("      property ",pname," : ",pvalue.encode("utf8"), " (", str(ptype), ") in ",pset)
    if pvalue == "":
        return pset, pname, ptype, None
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
                if preferences['DEBUG']:print("      warning: unable to export property as numeric value:",pname,pvalue)

    # print('pset: {}, pname: {}, ptype: {}, pvalue: {}'.format(pset, pname, ptype, pvalue))
    return pset, pname, ptype, pvalue


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
    if ifctype == "Undefined":
        ifctype = "BuildingElementProxy"

    return "Ifc" + ifctype


def exportIFC2X3Attributes(obj, kwargs, scale=0.001):

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
            "ElevationWithFlooring": obj.Shape.BoundBox.ZMin*scale
        })
    elif ifctype == "IfcReinforcingBar":
        kwargs.update({
            "NominalDiameter": obj.Diameter.Value,
            "BarLength": obj.Length.Value
        })
    elif ifctype == "IfcBuildingStorey":
        kwargs.update({"Elevation": obj.Placement.Base.z*scale})
    return kwargs


def exportIfcAttributes(obj, kwargs, scale=0.001):

    for property in obj.PropertiesList:
        if obj.getGroupOfProperty(property) == "IFC Attributes" and obj.getPropertyByName(property):
            value = obj.getPropertyByName(property)
            if isinstance(value, FreeCAD.Units.Quantity):
                value = float(value)
                if property in ["ElevationWithFlooring","Elevation"]:
                    value = value*scale # some properties must be changed to meters
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
        pxvc = ifcbin.createIfcDirection(tuple(d)[:2])
        povc = ifcbin.createIfcCartesianPoint(tuple(p.CenterOfMass[:2]))
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


def getRepresentation(ifcfile,context,obj,forcebrep=False,subtraction=False,tessellation=1,colors=None,preferences=None):

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
                    origin = ifcbin.createIfcCartesianPoint(tuple(FreeCAD.Vector(pla.Base).multiply(preferences['SCALE_FACTOR'])))
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
                    r = rdata[1] * preferences['SCALE_FACTOR']
                    for w in rdata[0]:
                        w.scale(preferences['SCALE_FACTOR'])
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
                    simpleExtrusion = True
                    for evi in ev:
                        if not isinstance(evi, FreeCAD.Vector):
                            simpleExtrusion = False
                    if simpleExtrusion:
                        for i in range(len(p)):
                            pi = p[i]
                            pi.scale(preferences['SCALE_FACTOR'])
                            if i < len(ev):
                                evi = FreeCAD.Vector(ev[i])
                            else:
                                evi = FreeCAD.Vector(ev[-1])
                            evi.multiply(preferences['SCALE_FACTOR'])
                            if i < len(pl):
                                pli = pl[i].copy()
                            else:
                                pli = pl[-1].copy()
                            pli.Base = pli.Base.multiply(preferences['SCALE_FACTOR'])
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
                                    pl2.Base = pl2.Base.multiply(preferences['SCALE_FACTOR'])
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
                    if hasattr(geom,"serialise") and obj.isDerivedFrom("Part::Feature") and preferences['SERIALIZE']:
                        if obj.Shape.Faces:
                            sh = obj.Shape.copy()
                            sh.Placement = obj.getGlobalPlacement()
                            sh.scale(preferences['SCALE_FACTOR']) # to meters
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

                        # if this is a clone, place back the shape in null position
                        if tostore:
                            fcshape.Placement = FreeCAD.Placement()

                        if fcshape.Solids:
                            dataset = fcshape.Solids
                        else:
                            dataset = fcshape.Shells
                            #if preferences['DEBUG']: print("Warning! object contains no solids")

                        for fcsolid in dataset:
                            fcsolid.scale(preferences['SCALE_FACTOR']) # to meters
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

        colorshapes = shapes # to keep track of individual shapes for coloring below
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
            origin = ifcbin.createIfcCartesianPoint(tuple(FreeCAD.Vector(pla.Base).multiply(preferences['SCALE_FACTOR'])))
            axis3 = ifcbin.createIfcDirection(tuple(pla.Rotation.multVec(FreeCAD.Vector(0,0,1))))
            transf = ifcbin.createIfcCartesianTransformationOperator3D(axis1,axis2,origin,1.0,axis3)
            mapitem = ifcfile.createIfcMappedItem(repmap,transf)
            shapes = [mapitem]
            sharedobjects[tostore] = repmap
            solidType = "MappedRepresentation"

        # set surface style

        shapecolor = None
        diffusecolor = None
        transparency = 0.0
        if colors:
            # color dict is given
            if obj.Name in colors:
                color = colors[obj.Name]
                shapecolor = color
                if isinstance(color[0],tuple):
                    # this is a diffusecolor. For now, use the first color - #TODO: Support per-face colors
                    diffusecolor = color
                    shapecolor = color[0]
        elif FreeCAD.GuiUp and (not subtraction) and hasattr(obj.ViewObject,"ShapeColor"):
            # every object gets a surface style. If the obj has a material, the surfstyle
            # is named after it. Revit will treat surfacestyles as materials (and discard
            # actual ifcmaterial)
            shapecolor = obj.ViewObject.ShapeColor[:3]
            transparency = obj.ViewObject.Transparency/100.0
            if hasattr(obj.ViewObject,"DiffuseColor"):
                diffusecolor = obj.ViewObject.DiffuseColor
        if shapecolor and (shapetype != "clone"): # cloned objects are already colored
            key = None
            rgbt = [shapecolor+(transparency,) for shape in shapes]
            if diffusecolor \
                    and (len(diffusecolor) == len(obj.Shape.Faces)) \
                    and (len(obj.Shape.Solids) == len(colorshapes)):
                i = 0
                rgbt = []
                for sol in obj.Shape.Solids:
                    rgbt.append(diffusecolor[i])
                    i += len(sol.Faces)
            for i,shape in enumerate(colorshapes):
                if i < len(rgbt):
                    key = rgbt[i]
                else:
                    key = rgbt[0]
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
