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
"""Provide the importer for IFC files used above all in Arch and BIM.

Internally it uses IfcOpenShell, which must be installed before using.
"""
## @package importIFC
#  \ingroup ARCH
#  \brief IFC file format importer
#
#  This module provides tools to import IFC files.


import os
import math
import time

import FreeCAD
import Part
import Draft
import Arch
import DraftVecUtils
import ArchIFCSchema
import importIFCHelper
import importIFCmulticore

from draftutils.messages import _msg, _err

if FreeCAD.GuiUp:
    import FreeCADGui as Gui

__title__  = "FreeCAD IFC importer - Enhanced IfcOpenShell-only version"
__author__ = ("Yorik van Havre", "Jonathan Wiedemann", "Bernd Hahnebach")
__url__    = "https://www.freecad.org"

DEBUG = False  # Set to True to see debug messages. Otherwise, totally silent
ZOOMOUT = True  # Set to False to not zoom extents after import

# Save the Python open function because it will be redefined
if open.__module__ in ['__builtin__', 'io']:
    pyopen = open

# Templates and other definitions ****
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
    "BuildingPart": [
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

# backwards compatibility
getPreferences = importIFCHelper.getPreferences


def export(exportList, filename, colors=None, preferences=None):
    """Export the selected objects to IFC format.

    The export code is now in a separate module; this call is retained
    in this module for compatibility purposes with older scripts.
    """
    import exportIFC
    exportIFC.export(exportList, filename, colors, preferences)


def open(filename, skip=[], only=[], root=None):
    """Open an IFC file inside a new document.

    TODO: change the default argument to `None`, instead of `[]`.
    This is better because lists are mutable.

    Most of the work is done in the `insert` function.
    """
    docname = os.path.splitext(os.path.basename(filename))[0]
    doc = FreeCAD.newDocument(docname)
    doc.Label = docname
    doc = insert(filename, doc.Name, skip, only, root)
    return doc


def insert(srcfile, docname, skip=[], only=[], root=None, preferences=None):
    """Import the contents of an IFC file in the current active document.

    TODO: change the default argument to `None`, instead of `[]`.
    This is better because lists are mutable.

    Parameters
    ----------
    skip: list
        By default empty list `[]`.
        Can contain a list of ids of objects to be skipped.

    only: list
        By default, empty list `[]`.
        Restrict the import to certain object ids; it will also
        get their children.

    root: object
        It is used to import only the derivates of a certain element type,
        for example, `'ifcProduct'
    """
    try:
        import ifcopenshell
        from ifcopenshell import geom

        # Sometimes there is an error importing `geom` in this way
        # import ifcopenshell.geom
        #
        # therefore we must use the `from x import y` way.
        #
        # For some reason this works; see the bug report
        # https://github.com/IfcOpenShell/IfcOpenShell/issues/689
    except ModuleNotFoundError:
        _err("IfcOpenShell was not found on this system. "
             "IFC support is disabled.\n"
             "Visit https://wiki.freecad.org/IfcOpenShell "
             "to learn about installing it.")
        return

    starttime = time.time()  # in seconds

    if preferences is None:
        preferences = importIFCHelper.getPreferences()

    if preferences["MULTICORE"] and not hasattr(srcfile, "by_guid"):
        # override with BIM IFC importer if present
        try:
            import BimIfcImport
            return BimIfcImport.insert(srcfile, docname, preferences)
        except:
            pass
        return importIFCmulticore.insert(srcfile, docname, preferences)

    try:
        doc = FreeCAD.getDocument(docname)
    except NameError:
        doc = FreeCAD.newDocument(docname)

    FreeCAD.ActiveDocument = doc

    global parametrics

    # allow to override the root element
    if root:
        preferences['ROOT_ELEMENT'] = root

    # keeping global variable for debugging purposes
    # global ifcfile

    # If the `by_guid` attribute exists, this is already a loaded ifcfile,
    # otherwise, it's just a string, and we have to open it with ifcopenshell
    if hasattr(srcfile, "by_guid"):
        ifcfile = srcfile
        filesize = None
        filename = None
    else:
        if preferences['DEBUG']:
            _msg("Opening '{}'... ".format(srcfile), end="")
        filename = srcfile
        filesize = os.path.getsize(filename) * 1E-6  # in megabytes
        ifcfile = ifcopenshell.open(filename)

    if preferences['DEBUG']:
        _msg("done.")

    # Get file scale
    ifcscale = importIFCHelper.getScaling(ifcfile)

    # IfcOpenShell multiplies the precision value of the file by 100
    # So we raise the precision by 100 too to compensate.
    # ctxs = ifcfile.by_type("IfcGeometricRepresentationContext")
    # for ctx in ctxs:
    #     if not ctx.is_a("IfcGeometricRepresentationSubContext"):
    #         ctx.Precision = ctx.Precision/100

    # Set default ifcopenshell options to work in brep mode
    settings = geom.settings()
    settings.set(settings.USE_BREP_DATA, True)
    settings.set(settings.SEW_SHELLS, True)
    settings.set(settings.USE_WORLD_COORDS, True)
    if preferences['SEPARATE_OPENINGS']:
        settings.set(settings.DISABLE_OPENING_SUBTRACTIONS, True)
    if preferences['SPLIT_LAYERS'] and hasattr(settings, "APPLY_LAYERSETS"):
        settings.set(settings.APPLY_LAYERSETS, True)

    # build all needed tables
    if preferences['DEBUG']:
        _msg("Building types and relationships table...")

    # type tables
    sites = ifcfile.by_type("IfcSite")
    buildings = ifcfile.by_type("IfcBuilding")
    floors = ifcfile.by_type("IfcBuildingStorey")
    openings = ifcfile.by_type("IfcOpeningElement")
    materials = ifcfile.by_type("IfcMaterial")
    (products,
     annotations) = importIFCHelper.buildRelProductsAnnotations(ifcfile,
                                                                preferences['ROOT_ELEMENT'])

    # empty relation tables
    objects = {}  # { id:object, ... }
    shapes = {}  # { id:shaoe } only used for merge mode
    structshapes = {}  # { id:shaoe } only used for merge mode
    sharedobjects = {}  # { representationmapid:object }

    # a list of imported objects whose parametric relationships
    # need processing after all objects have been created
    parametrics = []
    profiles = {}  # to store reused extrusion profiles {ifcid:fcobj, ...}
    layers = {}  # { layer_name, [ids] }
    # filled relation tables

    # TODO: investigate using inverse attributes.
    # For the following tables it might be better to use inverse attributes
    # to find the properties, otherwise a lot of loops
    # and if testing is needed.
    # See https://forum.freecad.org/viewtopic.php?f=39&t=37892
    prodrepr = importIFCHelper.buildRelProductRepresentation(ifcfile)
    additions = importIFCHelper.buildRelAdditions(ifcfile)
    groups = importIFCHelper.buildRelGroups(ifcfile)
    subtractions = importIFCHelper.buildRelSubtractions(ifcfile)
    mattable = importIFCHelper.buildRelMattable(ifcfile)
    colors = importIFCHelper.buildRelProductColors(ifcfile, prodrepr)
    colordict = {}  # { objname:color tuple } for non-GUI use
    if preferences['DEBUG']:
        _msg("done.")

    # only import a list of IDs and their children, if defined
    if only:
        ids = []
        while only:
            currentid = only.pop()
            ids.append(currentid)
            if currentid in additions:
                only.extend(additions[currentid])
        products = [ifcfile[currentid] for currentid in ids]

    # start the actual import, set FreeCAD UI
    count = 0
    from FreeCAD import Base
    progressbar = Base.ProgressIndicator()
    progressbar.start("Importing IFC objects...", len(products))
    if preferences['DEBUG']:
        _msg("Parsing {} BIM objects...".format(len(products)))

    # Prepare the 3D view if applicable
    if preferences['FITVIEW_ONIMPORT'] and FreeCAD.GuiUp:
        overallboundbox = None
        Gui.ActiveDocument.activeView().viewAxonometric()

    # Create the base project object
    if not preferences['REPLACE_PROJECT']:
        if len(ifcfile.by_type("IfcProject")) > 0:
            projectImporter = importIFCHelper.ProjectImporter(ifcfile, objects)
            projectImporter.execute()
        else:
            # https://forum.freecad.org/viewtopic.php?f=39&t=40624
            print("No IfcProject found in the ifc file. Nothing imported")
            return doc

    # handle IFC products

    for product in products:

        count += 1
        pid = product.id()
        guid = product.GlobalId
        ptype = product.is_a()

        if preferences['DEBUG']: print(count,"/",len(products),"object #"+str(pid),":",ptype,end="")

        # build list of related property sets
        psets = importIFCHelper.getIfcPropertySets(ifcfile, pid)

        # add layer names to layers
        if hasattr(product, "Representation") and hasattr(product.Representation, "Representations"):
            if len(product.Representation.Representations) > 0:
                lays = product.Representation.Representations[0].LayerAssignments
                if len(lays) > 0:
                    layer_name = lays[0].Name
                    if layer_name not in layers:
                        layers[layer_name] = [pid]
                    else:
                        layers[layer_name].append(pid)
                    if preferences['DEBUG']: print(" layer ", layer_name, " found", ptype,end="")
                else:
                    if preferences['DEBUG']: print(" no layer found", ptype,end="")

        # checking for full FreeCAD parametric definition, overriding everything else
        if psets and FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch").GetBool("IfcImportFreeCADProperties",False):
            if "FreeCADPropertySet" in [ifcfile[pset].Name for pset in psets.keys()]:
                if preferences['DEBUG']: print(" restoring from parametric definition...",end="")
                obj,parametrics = importIFCHelper.createFromProperties(psets,ifcfile,parametrics)
                if obj:
                    objects[pid] = obj
                    if preferences['DEBUG']: print("done")
                    continue
                else:
                    if preferences['DEBUG']: print("failed")

        # no parametric data, we go the good old way
        name = str(ptype[3:])
        if product.Name:
            name = product.Name
        if preferences['PREFIX_NUMBERS']:
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
            if preferences['DEBUG']: print(" (struct)",end="")
        else:
            if preferences['DEBUG']: print(" (arch)",end="")
        if preferences['MERGE_MODE_ARCH'] == 4 and archobj:
            if preferences['DEBUG']: print(" skipped.")
            continue
        if preferences['MERGE_MODE_STRUCT'] == 3 and not archobj:
            if preferences['DEBUG']: print(" skipped.")
            continue
        if pid in skip:  # user given id skip list
            if preferences['DEBUG']: print(" skipped.")
            continue
        if ptype in skip:  # user given type skip list
            if preferences['DEBUG']: print(" skipped.")
            continue
        if ptype in preferences['SKIP']:  # preferences-set type skip list
            if preferences['DEBUG']: print(" skipped.")
            continue
        if preferences['REPLACE_PROJECT']:  # options-enabled project/site/building skip
            if ptype in ['IfcProject','IfcSite']:
                if preferences['DEBUG']: print(" skipped.")
                continue
            elif ptype in ['IfcBuilding']:
                if len(ifcfile.by_type("IfcBuilding")) == 1:
                    # let multiple buildings through...
                    if preferences['DEBUG']: print(" skipped.")
                    continue
            elif ptype in ['IfcBuildingStorey']:
                if len(ifcfile.by_type("IfcBuildingStorey")) == 1:
                    # let multiple storeys through...
                    if preferences['DEBUG']: print(" skipped.")
                    continue

        # check if this object is sharing its shape (mapped representation)
        clone = None
        store = None
        prepr = None
        try:
            prepr = product.Representation
        except Exception:
            if preferences['DEBUG']: print(" ERROR unable to get object representation",end="")
        if prepr and (preferences['MERGE_MODE_ARCH'] == 0) and archobj and preferences['CREATE_CLONES']:
            for r in prepr.Representations:
                if r.RepresentationIdentifier.upper() == "BODY":
                    if r.Items[0].is_a("IfcMappedItem"):
                        originalid = r.Items[0].MappingSource.id()
                        if originalid in sharedobjects:
                            clone = sharedobjects[originalid]
                        else:
                            sharedobjects[originalid] = None
                            store = originalid  # flag this object to be stored later

        # set additional setting for structural entities
        if hasattr(settings,"INCLUDE_CURVES"):
            if structobj:
                settings.set(settings.INCLUDE_CURVES,True)
            else:
                settings.set(settings.INCLUDE_CURVES,False)
        try:
            cr = geom.create_shape(settings, product)
            brep = cr.geometry.brep_data
        except Exception:
            pass  # IfcOpenShell will yield an error if a given product has no shape, but we don't care, we're brave enough

        # from now on we have a brep string
        if brep:
            if preferences['DEBUG']: print(" "+str(int(len(brep)/1000))+"k ",end="")

            # create a Part shape
            shape = Part.Shape()
            shape.importBrepFromString(brep,False)
            shape.scale(1000.0)  # IfcOpenShell always outputs in meters, we convert to mm, the freecad internal unit

            if shape.isNull() and (not preferences['ALLOW_INVALID']):
                if preferences['DEBUG']: print("null shape ",end="")
            elif not shape.isValid() and (not preferences['ALLOW_INVALID']):
                if preferences['DEBUG']: print("invalid shape ",end="")
            else:

                # add to the global boundbox if applicable
                if preferences['FITVIEW_ONIMPORT'] and FreeCAD.GuiUp:
                    try:
                        bb = shape.BoundBox
                        # if preferences['DEBUG']: print(' ' + str(bb),end="")
                    except Exception:
                        bb = None
                        if preferences['DEBUG']: print(' BB could not be computed',end="")
                    if bb and bb.isValid():
                        if not overallboundbox:
                            overallboundbox = bb
                        if not overallboundbox.isInside(bb):
                            Gui.SendMsgToActiveView("ViewFit")
                        overallboundbox.add(bb)

                if (preferences['MERGE_MODE_ARCH'] > 0 and archobj) or structobj:
                    # we are not using Arch objects

                    # additional tweaks to set when not using Arch objects
                    if ptype == "IfcSpace":  # do not add spaces to compounds
                        if preferences['DEBUG']: print("skipping space ",pid,end="")
                    elif structobj:
                        structshapes[pid] = shape
                        if preferences['DEBUG']: print(len(shape.Solids),"solids ",end="")
                        baseobj = shape
                    else:
                        shapes[pid] = shape
                        if preferences['DEBUG']: print(len(shape.Solids),"solids ",end="")
                        baseobj = shape
                else:

                    # create base shape object
                    if clone:
                        if preferences['DEBUG']: print("clone ",end="")
                    else:
                        if preferences['GET_EXTRUSIONS'] and (preferences['MERGE_MODE_ARCH'] != 1):

                            # get IFC profile
                            profileid = None
                            sortmethod = None
                            if product.Representation:
                                if product.Representation.Representations:
                                    if product.Representation.Representations[0].is_a("IfcShapeRepresentation"):
                                        if product.Representation.Representations[0].Items:
                                            if product.Representation.Representations[0].Items[0].is_a("IfcExtrudedAreaSolid"):
                                                profileid = product.Representation.Representations[0].Items[0].SweptArea.id()
                                                sortmethod = importIFCHelper.getProfileCenterPoint(product.Representation.Representations[0].Items[0])

                            # recompose extrusions from a shape
                            if not sortmethod:
                                if ptype in ["IfcWall","IfcWallStandardCase","IfcSpace"]:
                                    sortmethod = "z"
                                else:
                                    sortmethod = "area"
                            ex = Arch.getExtrusionData(shape,sortmethod)  # is this an extrusion?
                            if ex:

                                # check for extrusion profile
                                baseface = None
                                addplacement = None
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
                                        # is this a circle?
                                        if (len(ex[0].Edges) == 1) and isinstance(ex[0].Edges[0].Curve,Part.Circle):
                                            baseface = Draft.makeCircle(ex[0].Edges[0])
                                        else:
                                            # curves or holes? We just make a Part face
                                            baseface = doc.addObject("Part::Feature",name+"_footprint")
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
                                        # no curve and no hole, we can make a draft object
                                        verts = [v.Point for v in ex[0].Wires[0].OrderedVertexes]
                                        # TODO verts are different if shape is made of RectangleProfileDef or not
                                        # is this a rectangle?
                                        if importIFCHelper.isRectangle(verts):
                                            baseface = Draft.makeRectangle(verts,face=True)
                                        else:
                                            # no hole and no curves, we make a Draft Wire instead
                                            baseface = Draft.makeWire(verts,closed=True)
                                    if profileid:
                                        # store for possible shared use
                                        profiles[profileid] = baseface
                                baseobj = doc.addObject("Part::Extrusion",name+"_body")
                                baseobj.Base = baseface
                                if addplacement:
                                    # apply delta placement (stored profile)
                                    baseobj.Placement = addplacement
                                    baseobj.Dir = addplacement.Rotation.inverted().multVec(ex[1])
                                else:
                                    baseobj.Dir = ex[1]
                                if FreeCAD.GuiUp:
                                    baseface.ViewObject.hide()
                        if not baseobj:
                            baseobj = doc.addObject("Part::Feature",name+"_body")
                            baseobj.Shape = shape
        else:
            # this object has no shape (storeys, etc...)
            if preferences['DEBUG']: print(" no brep ",end="")

        # we now have the shape, we create the final object

        if preferences['MERGE_MODE_ARCH'] == 0 and archobj:

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
                        doc.removeObject(bn)
                    if (freecadtype in ["Structure","Wall"]) and not baseobj:
                        # remove sizes to prevent auto shape creation for types that don't require a base object
                        obj.Height = 0
                        obj.Width = 0
                        obj.Length = 0
                    if (freecadtype in ["Rebar"]) and baseobj:
                        # TODO rebars don't keep link to their baee object - we can remove it
                        bn = baseobj.Name
                        doc.removeObject(bn)
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
            if preferences['DEBUG']: print(": "+obj.Label+" ",end="")
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
            except Exception:
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
                    # print("attribute:",attribute["name"])
                    if hasattr(product, attribute["name"]) and getattr(product, attribute["name"]) and hasattr(obj,attribute["name"]):
                        # print("Setting attribute",attribute["name"],"to",getattr(product, attribute["name"]))
                        setattr(obj, attribute["name"], getattr(product, attribute["name"]))
                        # TODO: ArchIFCSchema.IfcProducts uses the IFC version from the FreeCAD prefs.
                        # This might not coincide with the file being opened, hence some attributes are not properly read.

            if obj:
                # print number of solids
                if preferences['DEBUG']:
                    s = ""
                    if hasattr(obj,"Shape"):
                        if obj.Shape.Solids:
                            s = str(len(obj.Shape.Solids))+" solids"
                    print(s,end="")
                objects[pid] = obj

        elif (preferences['MERGE_MODE_ARCH'] == 1 and archobj) or (preferences['MERGE_MODE_STRUCT'] == 0 and not archobj):

            # non-parametric Arch objects (just Arch components with a shape)

            if ptype in ["IfcSite","IfcBuilding","IfcBuildingStorey"]:
                for freecadtype,ifctypes in typesmap.items():
                    if ptype in ifctypes:
                        obj = getattr(Arch,"make"+freecadtype)(baseobj=baseobj,name=name)
                        if preferences['DEBUG']: print(": "+obj.Label+" ",end="")
                        if ptype == "IfcBuildingStorey":
                            if product.Elevation:
                                obj.Placement.Base.z = product.Elevation * ifcscale
            elif baseobj:
                obj = Arch.makeComponent(baseobj,name=name,delete=True)
                obj.Label = name
                if preferences['DEBUG']: print(": "+obj.Label+" ",end="")
                if hasattr(obj,"Description") and hasattr(product,"Description"):
                    if product.Description:
                        obj.Description = product.Description
                try:
                    if hasattr(obj,"IfcType"):
                        obj.IfcType = ''.join(map(lambda x: x if x.islower() else " "+x, ptype[3:]))[1:]
                except Exception:
                    print("Unable to give IFC type ",ptype," to object ",obj.Label)
                if hasattr(obj,"IfcData"):
                    a = obj.IfcData
                    a["IfcUID"] = str(guid)
                    obj.IfcData = a
            elif pid in additions:
                # no baseobj but in additions, thus we make a BuildingPart container
                obj = getattr(Arch,"makeBuildingPart")(name=name)
                if preferences['DEBUG']: print(": "+obj.Label+" ",end="")
            else:
                if preferences['DEBUG']: print(": skipped.")
                continue
            if obj and hasattr(obj, "GlobalId"):
                obj.GlobalId = guid

        elif (preferences['MERGE_MODE_ARCH'] == 2 and archobj) or (preferences['MERGE_MODE_STRUCT'] == 1 and not archobj):

            # Part shapes

            if ptype in ["IfcSite","IfcBuilding","IfcBuildingStorey"]:
                for freecadtype,ifctypes in typesmap.items():
                    if ptype in ifctypes:
                        obj = getattr(Arch,"make"+freecadtype)(baseobj=baseobj,name=name)
                        if preferences['DEBUG']: print(": "+obj.Label+" ",end="")
                        if ptype == "IfcBuildingStorey":
                            if product.Elevation:
                                obj.Placement.Base.z = product.Elevation * ifcscale
            elif baseobj:
                obj = doc.addObject("Part::Feature",name)
                obj.Shape = shape
            elif pid in additions:
                # no baseobj but in additions, thus we make a BuildingPart container
                obj = getattr(Arch,"makeBuildingPart")(name=name)
                if preferences['DEBUG']: print(": "+obj.Label+" ",end="")
            else:
                if preferences['DEBUG']: print(": skipped.")
                continue

        if preferences['DEBUG']: print("")  # newline for debug prints, print for a new object should be on a new line

        if obj:

            obj.Label = name
            objects[pid] = obj

            # handle properties

            if psets:

                if preferences['IMPORT_PROPERTIES'] and hasattr(obj,"IfcProperties"):

                    # treat as spreadsheet (pref option)

                    if isinstance(obj.IfcProperties,dict):

                        # fix property type if needed

                        obj.removeProperty("IfcProperties")
                        obj.addProperty("App::PropertyLink","IfcProperties","Component","Stores IFC properties as a spreadsheet")

                    ifc_spreadsheet = Arch.makeIfcSpreadsheet()
                    n = 2
                    for c in psets.keys():
                        o = ifcfile[c]
                        if preferences['DEBUG']: print("propertyset Name",o.Name,type(o.Name))
                        catname = o.Name
                        for p in psets[c]:
                            l = ifcfile[p]
                            lname = l.Name
                            if l.is_a("IfcPropertySingleValue"):
                                if preferences['DEBUG']:
                                    print("property name",l.Name,type(l.Name))
                                ifc_spreadsheet.set(str('A'+str(n)), catname)
                                ifc_spreadsheet.set(str('B'+str(n)), lname)
                                if l.NominalValue:
                                    if preferences['DEBUG']:
                                        print("property NominalValue",l.NominalValue.is_a(),type(l.NominalValue.is_a()))
                                        print("property NominalValue.wrappedValue",l.NominalValue.wrappedValue,type(l.NominalValue.wrappedValue))
                                        # print("l.NominalValue.Unit",l.NominalValue.Unit,type(l.NominalValue.Unit))
                                    ifc_spreadsheet.set(str('C'+str(n)), l.NominalValue.is_a())
                                    if l.NominalValue.is_a() in ['IfcLabel','IfcText','IfcIdentifier','IfcDescriptiveMeasure']:
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
                                a[l.Name.encode("utf8")] = str(l.NominalValue)  # no py3 support here
                    obj.IfcData = a

            # color

            if (pid in colors) and colors[pid]:
                colordict[obj.Name] = colors[pid]
                if FreeCAD.GuiUp:
                    # if preferences['DEBUG']:
                    #    print("    setting color: ",int(colors[pid][0]*255),"/",int(colors[pid][1]*255),"/",int(colors[pid][2]*255))
                    if hasattr(obj.ViewObject,"ShapeColor"):
                        obj.ViewObject.ShapeColor = tuple(colors[pid][0:3])
                    if hasattr(obj.ViewObject,"Transparency"):
                        obj.ViewObject.Transparency = colors[pid][3]

            # if preferences['DEBUG'] is on, recompute after each shape
            if preferences['DEBUG']: doc.recompute()

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
                    obj.Latitude = importIFCHelper.dms2dd(*product.RefLatitude)
                if product.RefLongitude:
                    obj.Longitude = importIFCHelper.dms2dd(*product.RefLongitude)
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
                    # If the y-part of TrueNorth is 0, then the x-part should be checked.
                    # Declination would be -90° if x  >0 and +90° if x < 0
                    # Only if x==0 then we can not determine TrueNorth.
                    # But that would actually be an invalid IFC file, because the magnitude
                    # of the (twodimensional) direction vector for TrueNorth shall be greater than zero.
                    (x, y) = modelRC.TrueNorth.DirectionRatios[:2]
                    obj.Declination = ((math.degrees(math.atan2(y,x))-90+180) % 360)-180
                    if FreeCAD.GuiUp:
                        obj.ViewObject.CompassRotation.Value = obj.Declination

        try:
            progressbar.next(True)
        except(RuntimeError):
            print("Aborted.")
            progressbar.stop()
            doc.recompute()
            return

    progressbar.stop()
    doc.recompute()

    if preferences['MERGE_MODE_STRUCT'] == 2:

        if preferences['DEBUG']: print("Joining Structural shapes...",end="")

        for host,children in groups.items():  # Structural
            if ifcfile[host].is_a("IfcStructuralAnalysisModel"):
                compound = []
                for c in children:
                    if c in structshapes:
                        compound.append(structshapes[c])
                        del structshapes[c]
                if compound:
                    name = ifcfile[host].Name or "AnalysisModel"
                    if preferences['PREFIX_NUMBERS']: name = "ID" + str(host) + " " + name
                    obj = doc.addObject("Part::Feature",name)
                    obj.Label = name
                    obj.Shape = Part.makeCompound(compound)
        if structshapes:  # remaining Structural shapes
            obj = doc.addObject("Part::Feature","UnclaimedStruct")
            obj.Shape = Part.makeCompound(structshapes.values())

        if preferences['DEBUG']: print("done")

    else:

        if preferences['DEBUG']: print("Processing Struct relationships...",end="")

        # groups

        for host,children in groups.items():
            if ifcfile[host].is_a("IfcStructuralAnalysisModel"):
                # print(host, ' --> ', children)
                obj = doc.addObject("App::DocumentObjectGroup","AnalysisModel")
                objects[host] = obj
                if host in objects:
                    cobs = []
                    childs_to_delete = []
                    for child in children:
                        if child in objects:
                            cobs.append(objects[child])
                            childs_to_delete.append(child)
                    for c in childs_to_delete:
                        children.remove(c)  # to not process the child again in remaining groups
                    if cobs:
                        if preferences['DEBUG']: print("adding ",len(cobs), " object(s) to ", objects[host].Label)
                        Arch.addComponents(cobs,objects[host])
                        if preferences['DEBUG']: doc.recompute()

        if preferences['DEBUG']: print("done")

        if preferences['MERGE_MODE_ARCH'] > 2:  # if ArchObj is compound or ArchObj not imported
            doc.recompute()

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
                if preferences['DEBUG']: print("no group name specified for entity: #", ifcfile[host].id(), ", entity type is used!")
                grp_name = ifcfile[host].is_a() + "_" + str(ifcfile[host].id())
            grp = doc.addObject("App::DocumentObjectGroup",grp_name)
            grp.Label = grp_name
            objects[host] = grp
            for child in children:
                if child in objects:
                    grp.addObject(objects[child])
                    swallowed.append(child)
                else:
                    remaining[child] = grp

    if preferences['MERGE_MODE_ARCH'] == 3:

        # One compound per storey

        if preferences['DEBUG']: print("Joining Arch shapes...",end="")

        for host,children in additions.items():  # Arch
            if ifcfile[host].is_a("IfcBuildingStorey"):
                compound = []
                for c in children:
                    if c in shapes:
                        compound.append(shapes[c])
                        del shapes[c]
                    if c in additions:
                        for c2 in additions[c]:
                            if c2 in shapes:
                                compound.append(shapes[c2])
                                del shapes[c2]
                if compound:
                    name = ifcfile[host].Name or "Floor"
                    if preferences['PREFIX_NUMBERS']: name = "ID" + str(host) + " " + name
                    obj = doc.addObject("Part::Feature",name)
                    obj.Label = name
                    obj.Shape = Part.makeCompound(compound)
        if shapes:  # remaining Arch shapes
            obj = doc.addObject("Part::Feature","UnclaimedArch")
            obj.Shape = Part.makeCompound(shapes.values())

        if preferences['DEBUG']: print("done")

    else:

        if preferences['DEBUG']: print("Processing Arch relationships...",end="")
        first = True

        # subtractions

        if preferences['SEPARATE_OPENINGS']:
            for subtraction in subtractions:
                if (subtraction[0] in objects) and (subtraction[1] in objects):
                    if preferences['DEBUG'] and first:
                        print("")
                        first = False
                    if preferences['DEBUG']: print("    subtracting",objects[subtraction[0]].Label, "from", objects[subtraction[1]].Label)
                    Arch.removeComponents(objects[subtraction[0]],objects[subtraction[1]])
                    if preferences['DEBUG']: doc.recompute()

        # additions

        for host,children in additions.items():
            if host not in objects:
                # print(host, 'not used')
                # print(ifcfile[host])
                continue
            cobs = []
            for child in children:
                if child in objects \
                        and child not in swallowed:  # don't add objects already in groups
                    cobs.append(objects[child])
            if not cobs:
                continue
            if preferences['DEBUG'] and first:
                print("")
                first = False
            if (
                preferences['DEBUG']
                and (len(cobs) > 10)
                and (not(Draft.getType(objects[host]) in ["Site","Building","Floor","BuildingPart","Project"]))
            ):
                # avoid huge fusions
                print("more than 10 shapes to add: skipping.")
            else:
                if preferences['DEBUG']: print("    adding",len(cobs), "object(s) to", objects[host].Label)
                Arch.addComponents(cobs,objects[host])
                if preferences['DEBUG']: doc.recompute()

        if preferences['DEBUG'] and first: print("done.")

        doc.recompute()

        # cleaning bad shapes

        for obj in objects.values():
            if obj.isDerivedFrom("Part::Feature"):
                if obj.Shape.isNull() and not(Draft.getType(obj) in ["Site","Project"]):
                    Arch.rebuildArchShape(obj)

    doc.recompute()

    # 2D elements

    if preferences['DEBUG'] and annotations:
        print("Processing",len(annotations),"2D objects...")
    prodcount = count
    count = 0

    for annotation in annotations:

        anno = None
        aid = annotation.id()

        count += 1
        if preferences['DEBUG']: print(count,"/",len(annotations),"object #"+str(aid),":",annotation.is_a(),end="")

        if aid in skip:
            if preferences['DEBUG']: print(", skipped.")
            continue  # user given id skip list
        if annotation.is_a() in preferences['SKIP']:
            if preferences['DEBUG']: print(", skipped.")
            continue  # preferences-set type skip list

        anno = importIFCHelper.createAnnotation(annotation,doc,ifcscale,preferences)

        # placing in container if needed

        if anno:
            if aid in remaining:
                remaining[aid].addObject(anno)
            else:
                for host,children in additions.items():
                    if (aid in children) and (host in objects):
                        Arch.addComponents(anno,objects[host])

        if preferences['DEBUG']: print("")  # add newline for 2D objects debug prints

    doc.recompute()

    # Materials

    if preferences['DEBUG'] and materials: print("Creating materials...",end="")
    # print("\n")
    # print("colors:",colors)
    # print("mattable:",mattable)
    # print("materials:",materials)
    added_mats = []
    for material in materials:
        # print(material.id())

        mdict = {}
        # on ifc import only the "Description" and "DiffuseColor" (if it was read) of the material dictionary will be initialized
        # on editing material in Arch Gui a lot more keys of the material dictionary are initialized even with empty values
        # TODO: there should be a generic material obj init method which will be used by Arch Gui, import IFC, FEM, etc

        # get the material name
        name = "Material"
        if material.Name:
            name = material.Name
        # mdict["Name"] = name on duplicate material names in IFC this could result in crash
        # https://forum.freecad.org/viewtopic.php?f=23&t=63260
        # thus use "Description"
        mdict["Description"] = name

        # get material color
        # the "DiffuseColor" of a material should never be "None"
        # values in colors are None if something went wrong
        # thus the "DiffuseColor" will only be set if the color is not None
        mat_color = None
        if material.id() in colors and colors[material.id()] is not None:
            mat_color = str(colors[material.id()])
        else:
            for o,m in mattable.items():
                if m == material.id():
                    if o in colors and colors[o] is not None:
                        mat_color = str(colors[o])
        if mat_color is not None:
            mdict["DiffuseColor"] = mat_color
        else:
            if preferences['DEBUG']: print("/n  no color for material: {}, ".format(str(material.id)),end="")

        # merge materials with same name and color if setting in prefs is True
        add_material = True
        if preferences["MERGE_MATERIALS"]:
            for added_mat in added_mats:
                if (
                    "Description" in added_mat.Material  # Description has been set thus it is in mdict
                    and added_mat.Material["Description"] == mdict["Description"]
                ):
                    if (
                        (
                            "DiffuseColor" in added_mat.Material
                            and "DiffuseColor" in mdict
                            and added_mat.Material["DiffuseColor"] == mdict["DiffuseColor"]
                        )  # color in added mat with the same matname and new mat is the same
                        or
                        (
                            "DiffuseColor" not in added_mat.Material
                            and "DiffuseColor" not in mdict
                        )  # there is no color in added mat with the same matname and new mat
                        # on model imported from ArchiCAD color was not found for all IFC material objects,
                        # thus DiffuseColor was not set for created materials, workaround to merge these too
                    ):
                        matobj = added_mat
                        add_material = False
                        break

        # add a new material object
        if add_material is True:
            matobj = Arch.makeMaterial(name=name)
            matobj.Material = mdict
            added_mats.append(matobj)
        # fill material attribute of the objects
        for o,m in mattable.items():
            if m == material.id():
                if o in objects:
                    if hasattr(objects[o],"Material"):
                        objects[o].Material = matobj
                        if FreeCAD.GuiUp:
                            # the reason behind ...
                            # there are files around in which the material color is different from the shape color
                            # all viewers use the shape color whereas in FreeCAD the shape color will be
                            # overwritten by the material color (if there is a material with a color).
                            # In such a case FreeCAD shows a different color than all common ifc viewers
                            # https://forum.freecad.org/viewtopic.php?f=39&t=38440
                            col = objects[o].ViewObject.ShapeColor[:3]
                            dig = 5
                            ma_color = sh_color = round(col[0], dig), round(col[1], dig), round(col[2], dig)
                            if "DiffuseColor" in objects[o].Material.Material:
                                string_color = objects[o].Material.Material["DiffuseColor"]
                                col = tuple([float(f) for f in string_color.strip("()").split(",")])
                                ma_color = round(col[0], dig), round(col[1], dig), round(col[2], dig)
                            if ma_color != sh_color:
                                print("\nobject color != material color for object: ", o)
                                print("    material color is used (most software uses shape color)")
                                print("    obj: ", o, "label: ", objects[o].Label, " col: ", sh_color)
                                print("    mat: ", m, "label: ", matobj.Label, " col: ", ma_color)
                                # print("    ", ifcfile[o])
                                # print("    ", ifcfile[m])
                                # print("    colors:")
                                # print("    ", o, ": ", colors[o])
                                # print("    ", m, ": ", colors[m])
    if preferences['DEBUG'] and materials: print("done")

    # Grouping everything if required

    # has to be before the Layer
    # if REPLACE_PROJECT and only one storey and one building both are omitted
    # the pure objects do not belong to any container, they will be added here
    # if after Layer they are linked by Layer and will not be added here
    if preferences["REPLACE_PROJECT"] and filename:
        rootgroup = doc.addObject("App::DocumentObjectGroup","Group")
        rootgroup.Label = os.path.basename(filename)
        # print(objects)
        for key,obj in objects.items():
            # only add top-level objects
            if not obj.InList:
                rootgroup.addObject(obj)

    # Layers

    if preferences['DEBUG'] and layers: print("Creating layers...", end="")
    # print(layers)
    for layer_name, layer_objects in layers.items():
        if preferences["IMPORT_LAYER"] is False:
            continue
        # the method make_layer does some nasty debug prints
        lay = Draft.make_layer(layer_name)
        # ShapeColor and LineColor are not set, thus some some default values are used
        # do not override the imported ShapeColor and LineColor with default layer values
        if FreeCAD.GuiUp:
            lay.ViewObject.OverrideLineColorChildren = False
            lay.ViewObject.OverrideShapeColorChildren = False
        lay_grp = []
        for lobj_id in layer_objects:
            if lobj_id in objects:
                lay_grp.append(objects[lobj_id])
        lay.Group = lay_grp
    doc.recompute()
    if preferences['DEBUG'] and layers: print("done")

    # restore links from full parametric definitions

    for p in parametrics:
        l = doc.getObject(p[2])
        if l:
            setattr(p[0],p[1],l)

    # Save colordict in non-GUI mode
    if colordict and not FreeCAD.GuiUp:
        import json
        d = doc.Meta
        d["colordict"] = json.dumps(colordict)
        doc.Meta = d

    doc.recompute()

    if FreeCAD.GuiUp and ZOOMOUT:
        Gui.SendMsgToActiveView("ViewFit")

    endtime = time.time()-starttime

    if filesize:
        _msg("Finished importing {0} MB "
             "in {1} seconds, or {2} s/MB".format(round(filesize, 1),
                                                  int(endtime),
                                                  int(endtime/filesize)))
    else:
        _msg("Finished importing in {} seconds".format(int(endtime)))

    return doc
