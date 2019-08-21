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

__title__ = "FreeCAD IFC importer - Enhanced ifcopenshell-only version"
__author__ = "Yorik van Havre","Jonathan Wiedemann","Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

import six
import os
import math
import sys

import FreeCAD
import Part
import Draft
import Arch
import DraftVecUtils
import ArchIFCSchema
import importIFCHelper

## @package importIFC
#  \ingroup ARCH
#  \brief IFC file format importer
#
#  This module provides tools to import IFC files.

DEBUG = False # Set to True to see debug messages. Otherwise, totally silent
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


# ************************************************************************************************
# ********** some helper, used in import and export

def decode(filename,utf=False):

    "turns unicodes into strings"

    if six.PY2 and isinstance(filename,six.text_type):
        # workaround since ifcopenshell currently can't handle unicode filenames
        encoding = "utf8" if utf else sys.getfilesystemencoding()
        filename = filename.encode(encoding)
    return filename


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


# ************************************************************************************************
# ********** duplicate methods ****************
# TODO get rid of this duplicate
def getPreferences():

    """retrieves IFC preferences"""

    global DEBUG, PREFIX_NUMBERS, SKIP, SEPARATE_OPENINGS
    global ROOT_ELEMENT, GET_EXTRUSIONS, MERGE_MATERIALS
    global MERGE_MODE_ARCH, MERGE_MODE_STRUCT, CREATE_CLONES
    global FORCE_BREP, IMPORT_PROPERTIES, STORE_UID, SERIALIZE
    global SPLIT_LAYERS, EXPORT_2D, FULL_PARAMETRIC, FITVIEW_ONIMPORT
    global ADD_DEFAULT_SITE, ADD_DEFAULT_STOREY, ADD_DEFAULT_BUILDING
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
    ADD_DEFAULT_SITE = p.GetBool("IfcAddDefaultSite",False)
    ADD_DEFAULT_STOREY = p.GetBool("IfcAddDefaultStorey",False)
    ADD_DEFAULT_BUILDING = p.GetBool("IfcAddDefaultBuilding",True)


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

    # read preference settings
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

    # allow to override the root element
    if root:
        ROOT_ELEMENT = root

    # keeping global variable for debugging purposes
    # global ifcfile

    filename = decode(filename,utf=True)
    ifcfile = ifcopenshell.open(filename)

    # get file scale
    ifcscale = importIFCHelper.getScaling(ifcfile)

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

    # build all needed tables
    if DEBUG: print("Building relationships table...",end="")
    objects,prodrepr,additions,groups,subtractions,colors,shapes, \
    structshapes,mattable,sharedobjects,parametrics,profiles, \
    sites,buildings,floors,products,openings,annotations,materials, \
    style_material_id = importIFCHelper.buildRelationships(ifcfile,ROOT_ELEMENT)
    if DEBUG: print("done.")

    # only import a list of IDs and their children, if defined
    if only:
        ids = []
        while only:
            currentid = only.pop()
            ids.append(currentid)
            if currentid in additions.keys():
                only.extend(additions[currentid])
        products = [ifcfile[currentid] for currentid in ids]

    # start the actual import, set FreeCAD UI
    count = 0
    from FreeCAD import Base
    progressbar = Base.ProgressIndicator()
    progressbar.start("Importing IFC objects...",len(products))
    if DEBUG: print("Parsing",len(products),"BIM objects...")

    # Prepare the 3D view if applicable
    if FITVIEW_ONIMPORT and FreeCAD.GuiUp:
        overallboundbox = None
        import FreeCADGui
        FreeCADGui.ActiveDocument.activeView().viewAxonometric()

    # Create the base project object
    projectImporter = importIFCHelper.ProjectImporter(ifcfile, objects)
    projectImporter.execute()

    # handle IFC products

    for product in products:

        count += 1

        pid = product.id()
        guid = product.GlobalId
        ptype = product.is_a()
        if DEBUG: print(count,"/",len(products),"object #"+str(pid),":",ptype,end="")

        # build list of related property sets
        psets = importIFCHelper.getIfcPropertySets(ifcfile, pid)

        # checking for full FreeCAD parametric definition, overriding everything else
        if psets and FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch").GetBool("IfcImportFreeCADProperties",False):
            if "FreeCADPropertySet" in [ifcfile[pset].Name for pset in psets.keys()]:
                if DEBUG: print(" restoring from parametric definition...",end="")
                obj = createFromProperties(psets,ifcfile)
                if obj:
                    objects[pid] = obj
                    if DEBUG: print("done")
                    continue
                else:
                    if DEBUG: print("failed")

        # no parametric data, we go the good old way
        name = str(ptype[3:])
        if product.Name:
            name = product.Name
            if six.PY2:
                name = name.encode("utf8")
        if PREFIX_NUMBERS:
            name = "ID" + str(pid) + " " + name
        obj = None
        baseobj = None
        brep = None
        shape = None

        # classify object and verify if we must skip it
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

        # check if this object is sharing its shape (mapped representation)
        clone = None
        store = None
        prepr = None
        try:
            prepr = product.Representation
        except:
            if DEBUG: print(" ERROR unable to get object representation",end="")
        if prepr and (MERGE_MODE_ARCH == 0) and archobj and CREATE_CLONES:
            for r in prepr.Representations:
                if r.RepresentationIdentifier.upper() == "BODY":
                    if r.Items[0].is_a("IfcMappedItem"):
                        originalid = r.Items[0].MappingSource.id()
                        if originalid in sharedobjects:
                            clone = sharedobjects[originalid]
                        else:
                            sharedobjects[originalid] = None
                            store = originalid # flag this object to be stored later

        # set additional setting for structural entities
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

        # from now on we have a brep string
        if brep:
            if DEBUG: print(" "+str(int(len(brep)/1000))+"k ",end="")

            # create a Part shape
            shape = Part.Shape()
            shape.importBrepFromString(brep,False)
            shape.scale(1000.0) # IfcOpenShell always outputs in meters, we convert to mm, the freecad internal unit

            if shape.isNull():
                if DEBUG: print("null shape ",end="")
            elif not shape.isValid():
                if DEBUG: print("invalid shape ",end="")
            else:

                # add to the global boundbox if applicable
                if FITVIEW_ONIMPORT and FreeCAD.GuiUp:
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
                    # we are not using Arch objects

                    # additional tweaks to set when not using Arch objects
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

                            # recompose extrusions from a shape
                            if ptype in ["IfcWall","IfcWallStandardCase","IfcSpace"]:
                                sortmethod = "z"
                            else:
                                sortmethod = "area"
                            ex = Arch.getExtrusionData(shape,sortmethod) # is this an extrusion?
                            if ex:

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
                                if profileid and (profileid in profiles):

                                    # reuse existing profile if existing
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
                                    # this is an extrusion but we haven't built the profile yet
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
                                        # store for possible shared use
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
            # this object has no shape (storeys, etc...)
            if DEBUG: print(" no brep ",end="")

        # we now have the shape, we create the final object

        if MERGE_MODE_ARCH == 0 and archobj:

            # full Arch objects

            for freecadtype,ifctypes in typesmap.items():

                if ptype not in ifctypes:
                    continue
                if clone:
                    obj = getattr(Arch,"make"+freecadtype)(name=name)
                    obj.CloneOf = clone

                    # calculate the correct distance from the cloned object
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
                                r = importIFCHelper.getRotation(product.Representation.Representations[0].Items[0].MappingTarget)
                                if not r.isNull():
                                    v = v.add(s2.CenterOfMass)
                                    v = v.add(r.multVec(s2.CenterOfMass.negative()))
                                obj.Placement.Rotation = r
                                obj.Placement.move(v)
                        else:
                            print("failed to compute placement ",)
                else:
                    # no clone
                    obj = getattr(Arch,"make"+freecadtype)(baseobj=baseobj,name=name)
                    if freecadtype in ["Wall","Structure"] and baseobj and baseobj.isDerivedFrom("Part::Extrusion"):
                        # remove intermediary extrusion for types that can extrude themselves
                        # TODO do this check earlier so we do not calculate it, to save time
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

                # set the placement from the storey's elevation property
                if ptype == "IfcBuildingStorey":
                    if product.Elevation:
                        obj.Placement.Base.z = product.Elevation * ifcscale

                break

            if not obj:
                # we couldn't make an object of a specific type, use default arch component
                obj = Arch.makeComponent(baseobj,name=name)

            # set additional properties
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
                    if attribute["name"] == "Name":
                        continue
                    #print("attribute:",attribute["name"])
                    if hasattr(product, attribute["name"]) and getattr(product, attribute["name"]) and hasattr(obj,attribute["name"]):
                        #print("Setting attribute",attribute["name"],"to",getattr(product, attribute["name"]))
                        setattr(obj, attribute["name"], getattr(product, attribute["name"]))
                        # TODO: ArchIFCSchema.IfcProducts uses the IFC version from the FreeCAD prefs.
                        # This might not coincide with the file being opened, hence some attributes are not properly read.

            if obj:
                # print number of solids
                if DEBUG:
                    s = ""
                    if hasattr(obj,"Shape"):
                        if obj.Shape.Solids:
                            s = str(len(obj.Shape.Solids))+" solids"
                    print(s,end="")
                objects[pid] = obj

        elif (MERGE_MODE_ARCH == 1 and archobj) or (MERGE_MODE_STRUCT == 0 and not archobj):

            # non-parametric Arch objects (just Arch components with a shape)

            if ptype in ["IfcSite","IfcBuilding","IfcBuildingStorey"]:
                for freecadtype,ifctypes in typesmap.items():
                    if ptype in ifctypes:
                        obj = getattr(Arch,"make"+freecadtype)(baseobj=baseobj,name=name)
                        if ptype == "IfcBuildingStorey":
                            if product.Elevation:
                                obj.Placement.Base.z = product.Elevation * ifcscale
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
                                obj.Placement.Base.z = product.Elevation * ifcscale
            elif baseobj:
                obj = FreeCAD.ActiveDocument.addObject("Part::Feature",name)
                obj.Shape = shape

        if DEBUG: print("")  # newline for debug prints, print for a new object should be on a new line

        if obj:

            obj.Label = name
            objects[pid] = obj

            # handle properties

            if psets:

                if IMPORT_PROPERTIES and hasattr(obj,"IfcProperties"):

                    # treat as spreadsheet (pref option)

                    if isinstance(obj.IfcProperties,dict):

                        # fix property type if needed

                        obj.removeProperty("IfcProperties")
                        obj.addProperty("App::PropertyLink","IfcProperties","Component","Stores IFC properties as a spreadsheet")

                    ifc_spreadsheet = Arch.makeIfcSpreadsheet()
                    n = 2
                    for c in psets.keys():
                        o = ifcfile[c]
                        if DEBUG: print("propertyset Name",o.Name,type(o.Name))
                        catname = o.Name
                        for p in psets[c]:
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
                    obj.IfcProperties = importIFCHelper.getIfcProperties(ifcfile, pid, psets, d)

                elif hasattr(obj,"IfcData"):

                    # 0.17: properties are saved as type(value) in IfcData

                    a = obj.IfcData
                    for c in psets.keys():
                        for p in psets[c]:
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
                    obj.Elevation = product.RefElevation * ifcscale
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
                project = product.Decomposes[0].RelatingObject
                modelRC = next((rc for rc in project.RepresentationContexts if rc.ContextType == "Model"), None)
                if modelRC and modelRC.TrueNorth:
                    obj.Declination = -math.degrees(math.atan(modelRC.TrueNorth.DirectionRatios[0] / modelRC.TrueNorth.DirectionRatios[1]))
                    if(FreeCAD.GuiUp):
                        obj.ViewObject.CompassRotation.Value = obj.Declination

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
                obj = FreeCAD.ActiveDocument.addObject("App::DocumentObjectGroup","AnalysisModel")
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
            grp = FreeCAD.ActiveDocument.addObject("App::DocumentObjectGroup",grp_name)
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
            if host not in objects.keys():
                continue
            cobs = []
            for child in children:
                if child in objects.keys() \
                    and child not in swallowed: # don't add objects already in groups
                        cobs.append(objects[child])
            if not cobs:
                continue
            if DEBUG and first:
                print("")
                first = False
            if DEBUG and (len(cobs) > 10) and (not(Draft.getType(objects[host]) in ["Site","Building","Floor","BuildingPart","Project"])):
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
                if obj.Shape.isNull() and not(Draft.getType(obj) in ["Site","Project"]):
                    Arch.rebuildArchShape(obj)

    FreeCAD.ActiveDocument.recompute()

    # 2D elements

    if DEBUG and annotations:
        print("Processing",len(annotations),"2D objects...")
    prodcount = count
    count = 0

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
                    sh = importIFCHelper.get2DShape(axis.AxisCurve,ifcscale)
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
                    sh = importIFCHelper.get2DShape(rep,ifcscale)
                    if sh in FreeCAD.ActiveDocument.Objects:
                        # dirty hack: get2DShape might return an object directly if non-shape based (texts for ex)
                        anno = sh
                    else:
                        shapes2d.extend(sh)
            if shapes2d:
                sh = Part.makeCompound(shapes2d)
                if DEBUG: print(" shape")
                anno = FreeCAD.ActiveDocument.addObject("Part::Feature",name)
                anno.Shape = sh
                p = importIFCHelper.getPlacement(annotation.ObjectPlacement,ifcscale)
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
        # get and set material name
        name = "Material"
        if material.Name:
            name = material.Name
            if six.PY2:
                name = name.encode("utf8")
        # get material color
        mdict = {}
        if material.id() in colors:
            mdict["DiffuseColor"] = str(colors[material.id()])
        else:
            for o,m in mattable.items():
                if m == material.id():
                    if o in colors:
                        mdict["DiffuseColor"] = str(colors[o])
        # merge materials with same name and color if setting in prefs is True
        add_material = True
        if MERGE_MATERIALS:
            for key in list(fcmats.keys()):
                if key.startswith(name) \
                        and "DiffuseColor" in mdict and "DiffuseColor" in fcmats[key].Material \
                        and mdict["DiffuseColor"] == fcmats[key].Material["DiffuseColor"]:
                    mat = fcmats[key]
                    add_material = False
        # add a new material object
        if add_material is True:
            mat = Arch.makeMaterial(name=name)
            if mdict:
                mat.Material = mdict
            fcmats[mat.Name] = mat
        # fill material attribute of the objects
        for o,m in mattable.items():
            if m == material.id():
                if o in objects:
                    if hasattr(objects[o],"Material"):
                        # the reason behind ...
                        # there are files around in which the material color is different from the shape color
                        # all viewers use the shape color whereas in FreeCAD the shape color will be
                        # overwritten by the material color (if there is a material with a color).
                        # In such a case FreeCAD shows a different color than all common ifc viewers
                        # https://forum.freecadweb.org/viewtopic.php?f=39&t=38440
                        col = objects[o].ViewObject.ShapeColor[:3]
                        dig = 5
                        ma_color = sh_color = round(col[0], dig), round(col[1], dig), round(col[2], dig)
                        objects[o].Material = mat
                        if "DiffuseColor" in objects[o].Material.Material:
                            sting_col_ = objects[o].Material.Material["DiffuseColor"]
                            col = tuple([float(f) for f in sting_col_.strip("()").split(",")])
                            ma_color = round(col[0], dig), round(col[1], dig), round(col[2], dig)
                        if ma_color != sh_color:
                            print("\nobject color != material color for object: ", o)
                            print("    material color is used (most software uses shape color)")
                            print("    obj: ", o, "label: ", objects[o].Label, " col: ", sh_color)
                            print("    mat: ", m, "label: ", mat.Label, " col: ", ma_color)
                            # print("    ", ifcfile[o])
                            # print("    ", ifcfile[m])
                            # print("    colors:")
                            # print("    ", o, ": ", colors[o])
                            # print("    ", m, ": ", colors[m])

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
# ********** helper ****************
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
