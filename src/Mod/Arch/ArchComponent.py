#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2011                                                    *
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

__title__="FreeCAD Arch Component"
__author__ = "Yorik van Havre"
__url__ = "http://www.freecadweb.org"

# IFC types

IFCTYPES = ["IfcActuator", "IfcAirTerminal", "IfcAirTerminalBox", "IfcAirToAirHeatRecovery",
"IfcAlarm", "IfcAnnotation", "IfcAudioVisualAppliance", "IfcBeam", "IfcBeamStandardCase",
"IfcBoiler", "IfcBuilding", "IfcBuildingElement", "IfcBuildingElementPart", "IfcBuildingElementProxy",
"IfcBuildingStorey", "IfcBurner", "IfcCableCarrierFitting", "IfcCableCarrierSegment",
"IfcCableFitting", "IfcCableSegment", "IfcChiller", "IfcChimney", "IfcCivilElement",
"IfcCoil", "IfcColumn", "IfcColumnStandardCase", "IfcCommunicationsAppliance", "IfcCompressor",
"IfcCondenser", "IfcController", "IfcCooledBeam", "IfcCoolingTower", "IfcCovering",
"IfcCurtainWall", "IfcDamper", "IfcDiscreteAccessory", "IfcDistributionChamberElement",
"IfcDistributionControlElement", "IfcDistributionElement", "IfcDistributionFlowElement",
"IfcDistributionPort", "IfcDoor", "IfcDoorStandardCase", "IfcDuctFitting", "IfcDuctSegment",
"IfcDuctSilencer", "IfcElectricAppliance", "IfcElectricDistributionBoard", "IfcElectricFlowStorageDevice",
"IfcElectricGenerator", "IfcElectricMotor", "IfcElectricTimeControl",
"IfcElementAssembly", "IfcElementComponent", "IfcEnergyConversionDevice", "IfcEngine",
"IfcEvaporativeCooler", "IfcEvaporator", "IfcExternalSpatialElement", "IfcExternalSpatialStructureElement",
"IfcFan", "IfcFastener", "IfcFeatureElement", "IfcFeatureElementAddition", "IfcFeatureElementSubtraction",
"IfcFilter", "IfcFireSuppressionTerminal", "IfcFlowController", "IfcFlowFitting", "IfcFlowInstrument",
"IfcFlowMeter", "IfcFlowMovingDevice", "IfcFlowSegment", "IfcFlowStorageDevice", "IfcFlowTerminal",
"IfcFlowTreatmentDevice", "IfcFooting", "IfcFurnishingElement", "IfcFurniture", "IfcGeographicElement",
"IfcGrid", "IfcHeatExchanger", "IfcHumidifier", "IfcInterceptor", "IfcJunctionBox", "IfcLamp",
"IfcLightFixture", "IfcMechanicalFastener", "IfcMedicalDevice", "IfcMember", "IfcMemberStandardCase",
"IfcMotorConnection", "IfcOpeningElement", "IfcOpeningStandardCase", "IfcOutlet", "IfcPile",
"IfcPipeFitting", "IfcPipeSegment", "IfcPlate", "IfcPlateStandardCase",
"IfcProjectionElement", "IfcProtectiveDevice", "IfcProtectiveDeviceTrippingUnit",
"IfcProxy", "IfcPump", "IfcRailing", "IfcRamp", "IfcRampFlight", "IfcReinforcingBar",
"IfcReinforcingMesh", "IfcRoof", "IfcSanitaryTerminal",
"IfcSensor", "IfcShadingDevice", "IfcSite", "IfcSlab", "IfcSlabElementedCase", "IfcSlabStandardCase",
"IfcSolarDevice", "IfcSpace", "IfcSpaceHeater",
"IfcSpatialZone", "IfcStackTerminal", "IfcStair", "IfcStairFlight",
"IfcStructuralCurveAction",
"IfcStructuralCurveConnection", "IfcStructuralCurveMember", "IfcStructuralCurveMemberVarying",
"IfcStructuralCurveReaction", "IfcStructuralLinearAction",
"IfcStructuralPlanarAction", "IfcStructuralPointAction",
"IfcStructuralPointConnection", "IfcStructuralPointReaction",
"IfcStructuralSurfaceAction", "IfcStructuralSurfaceConnection", "IfcStructuralSurfaceMember",
"IfcStructuralSurfaceMemberVarying", "IfcStructuralSurfaceReaction", "IfcSurfaceFeature",
"IfcSwitchingDevice", "IfcSystemFurnitureElement", "IfcTank", "IfcTendon", "IfcTendonAnchor",
"IfcTransformer", "IfcTransportElement", "IfcTubeBundle", "IfcUnitaryControlElement",
"IfcUnitaryEquipment", "IfcValve", "IfcVibrationIsolator", "IfcVirtualElement", "IfcVoidingFeature",
"IfcWall", "IfcWallElementedCase", "IfcWallStandardCase", "IfcWasteTerminal", "IfcWindow",
"IfcWindowStandardCase"]

# Possible roles for FreeCAD BIM objects
IfcRoles = ['Undefined']+[''.join(map(lambda x: x if x.islower() else " "+x, t[3:]))[1:] for t in IFCTYPES]

# Property types
SimplePropertyTypes = ["IfcInteger","IfcReal","IfcBoolean","IfcIdentifier","IfcText","IfcLabel","IfcLogical"]
MeasurePropertyTypes = ["IfcVolumeMeasure","IfcTimeMeasure","IfcThermodynamicTemperatureMeasure","IfcSolidAngleMeasure",
                        "IfcPositiveRatioMeasure","IfcRatioMeasure","IfcPositivePlaneAngleMeasure","IfcPlaneAngleMeasure",
                        "IfcParameterValue","IfcNumericMeasure","IfcMassMeasure","IfcPositiveLengthMeasure",
                        "IfcLengthMeasure","IfcElectricCurrentMeasure","IfcDescriptiveMeasure","IfcCountMeasure",
                        "IfcContextDependentMeasure","IfcAreaMeasure","IfcAmountOfSubstanceMeasure",
                        "IfcLuminousIntensityMeasure","IfcNormalisedRatioMeasure","IfcComplexNumber"]

import FreeCAD,Draft,ArchCommands,math,sys
from FreeCAD import Vector
if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtGui,QtCore
    from DraftTools import translate
    from PySide.QtCore import QT_TRANSLATE_NOOP
else:
    # \cond
    def translate(ctxt,txt):
        return txt
    def QT_TRANSLATE_NOOP(ctxt,txt):
        return txt
    # \endcond

## @package ArchComponent
#  \ingroup ARCH
#  \brief The base class of all Arch objects
#
#  This module provides the base Arch component class, that
#  is shared by all of the Arch BIM objects



def convertOldComponents(objs=[]):

    """converts Arch Objects with a Role property to the new IfcRole.
    if no object is given, all objects of the active document are converted"""

    if not objs:
        objs = FreeCAD.ActiveDocument.Objects
    if not isinstance(objs,list):
        objs = [objs]
    for obj in objs:
        if "Role" in obj.PropertiesList:
            obj.addProperty("App::PropertyEnumeration","IfcRole","Component",QT_TRANSLATE_NOOP("App::Property","The role of this object"))
            obj.IfcRole = IfcRoles
            if obj.Role in IfcRoles:
                obj.IfcRole = obj.Role
            else:
                FreeCAD.Console.PrintMessage("Role "+obj.Role+" cannot be mapped for object "+obj.Label+"\n")
            obj.removeProperty("Role")


def addToComponent(compobject,addobject,mod=None):

    '''addToComponent(compobject,addobject,mod): adds addobject
    to the given component. Default is in "Additions", "Objects" or
    "Components", the first one that exists in the component. Mod
    can be set to one of those attributes ("Objects", Base", etc...)

    to override the default.'''
    import Draft
    if compobject == addobject: return
    # first check zis already there
    found = False
    attribs = ["Additions","Objects","Components","Subtractions","Base","Group","Hosts"]
    for a in attribs:
        if hasattr(compobject,a):
            if a == "Base":
                if addobject == getattr(compobject,a):
                    found = True
            else:
                if addobject in getattr(compobject,a):
                    found = True
    if not found:
        if mod:
            if hasattr(compobject,mod):
                if mod == "Base":
                    setattr(compobject,mod,addobject)
                    addobject.ViewObject.hide()
                elif mod == "Axes":
                    if Draft.getType(addobject) == "Axis":
                        l = getattr(compobject,mod)
                        l.append(addobject)
                        setattr(compobject,mod,l)
                else:
                    l = getattr(compobject,mod)
                    l.append(addobject)
                    setattr(compobject,mod,l)
                    if mod != "Objects":
                        addobject.ViewObject.hide()
                        if Draft.getType(compobject) == "PanelSheet":
                            addobject.Placement.move(compobject.Placement.Base.negative())
        else:
            for a in attribs[:3]:
                if hasattr(compobject,a):
                    l = getattr(compobject,a)
                    l.append(addobject)
                    setattr(compobject,a,l)
                    addobject.ViewObject.hide()
                    break


def removeFromComponent(compobject,subobject):

    '''removeFromComponent(compobject,subobject): subtracts subobject
    from the given component. If the subobject is already part of the
    component (as addition, subtraction, etc... it is removed. Otherwise,
    it is added as a subtraction.'''

    if compobject == subobject: return
    found = False
    attribs = ["Additions","Subtractions","Objects","Components","Base","Axes","Fixtures","Group","Hosts"]
    for a in attribs:
        if hasattr(compobject,a):
            if a == "Base":
                if subobject == getattr(compobject,a):
                    setattr(compobject,a,None)
                    subobject.ViewObject.show()
                    found = True
            else:
                if subobject in getattr(compobject,a):
                    l = getattr(compobject,a)
                    l.remove(subobject)
                    setattr(compobject,a,l)
                    subobject.ViewObject.show()
                    if Draft.getType(compobject) == "PanelSheet":
                        subobject.Placement.move(compobject.Placement.Base)
                    found = True
    if not found:
        if hasattr(compobject,"Subtractions"):
            l = compobject.Subtractions
            l.append(subobject)
            compobject.Subtractions = l
            if (Draft.getType(subobject) != "Window") and (not Draft.isClone(subobject,"Window",True)):
                ArchCommands.setAsSubcomponent(subobject)



class Component:


    "The default Arch Component object"

    def __init__(self,obj):

        obj.Proxy = self
        Component.setProperties(self,obj)
        self.Type = "Component"

    def setProperties(self,obj):

        pl = obj.PropertiesList
        if not "Base" in pl:
            obj.addProperty("App::PropertyLink","Base","Component",QT_TRANSLATE_NOOP("App::Property","The base object this component is built upon"))
        if not "CloneOf" in pl:
            obj.addProperty("App::PropertyLink","CloneOf","Component",QT_TRANSLATE_NOOP("App::Property","The object this component is cloning"))
        if not "Additions" in pl:
            obj.addProperty("App::PropertyLinkList","Additions","Component",QT_TRANSLATE_NOOP("App::Property","Other shapes that are appended to this object"))
        if not "Subtractions" in pl:
            obj.addProperty("App::PropertyLinkList","Subtractions","Component",QT_TRANSLATE_NOOP("App::Property","Other shapes that are subtracted from this object"))
        if not "Description" in pl:
            obj.addProperty("App::PropertyString","Description","Component",QT_TRANSLATE_NOOP("App::Property","An optional description for this component"))
        if not "Tag" in pl:
            obj.addProperty("App::PropertyString","Tag","Component",QT_TRANSLATE_NOOP("App::Property","An optional tag for this component"))
        if not "StandardCode" in pl:
            obj.addProperty("App::PropertyString","StandardCode","Component",QT_TRANSLATE_NOOP("App::Property","An optional standard (OmniClass, etc...) code for this component"))
        if not "IfcAttributes" in pl:
            obj.addProperty("App::PropertyMap","IfcAttributes","Component",QT_TRANSLATE_NOOP("App::Property","Custom IFC properties and attributes"))
        if not "Material" in pl:
            obj.addProperty("App::PropertyLink","Material","Component",QT_TRANSLATE_NOOP("App::Property","A material for this object"))
        if "BaseMaterial" in pl:
                obj.Material = obj.BaseMaterial
                obj.removeProperty("BaseMaterial")
                FreeCAD.Console.PrintMessage("Upgrading "+obj.Label+" BaseMaterial property to Material\n")
        if not "IfcRole" in pl:
            obj.addProperty("App::PropertyEnumeration","IfcRole","Component",QT_TRANSLATE_NOOP("App::Property","The role of this object"))
            obj.IfcRole = IfcRoles
        if "Role" in pl:
            r = obj.Role
            obj.removeProperty("Role")
            if r in IfcRoles:
                obj.IfcRole = r
                FreeCAD.Console.PrintMessage("Upgrading "+obj.Label+" Role property to IfcRole\n")
        if "IfcType" in pl: # for future backwards compatibility
            r = obj.IfcType
            obj.removeProperty("IfcType")
            if r in IfcRoles:
                obj.IfcRole = r
                FreeCAD.Console.PrintMessage("Downgrading "+obj.Label+" IfcType property to IfcRole\n")
        if not "MoveBase" in pl:
            obj.addProperty("App::PropertyBool","MoveBase","Component",QT_TRANSLATE_NOOP("App::Property","Specifies if moving this object moves its base instead"))
            obj.MoveBase = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch").GetBool("MoveBase",False)
        if not "MoveWithHost" in pl:
            obj.addProperty("App::PropertyBool","MoveWithHost","Component",QT_TRANSLATE_NOOP("App::Property","Specifies if this object must move together when its host is moved"))
            obj.MoveWithHost = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch").GetBool("MoveWithHost",False)
        if not "IfcProperties" in pl:
            obj.addProperty("App::PropertyMap","IfcProperties","Component",QT_TRANSLATE_NOOP("App::Property","Stores IFC properties"))
        if not "VerticalArea" in pl:
            obj.addProperty("App::PropertyArea","VerticalArea","Component",QT_TRANSLATE_NOOP("App::Property","The area of all vertical faces of this object"))
            obj.setEditorMode("VerticalArea",1)
        if not "HorizontalArea" in pl:
            obj.addProperty("App::PropertyArea","HorizontalArea","Component",QT_TRANSLATE_NOOP("App::Property","The area of the projection of this object onto the XY plane"))
            obj.setEditorMode("HorizontalArea",1)
        if not "PerimeterLength" in pl:
            obj.addProperty("App::PropertyLength","PerimeterLength","Component",QT_TRANSLATE_NOOP("App::Property","The perimeter length of the horizontal area"))
            obj.setEditorMode("PerimeterLength",1)
        if not "HiRes" in pl:
            obj.addProperty("App::PropertyLink","HiRes","Component",QT_TRANSLATE_NOOP("App::Property","An optional higher-resolution mesh or shape for this object"))
        if not "Axis" in pl:
            obj.addProperty("App::PropertyLink","Axis","Component",QT_TRANSLATE_NOOP("App::Property","An optional axis or axis system on which this object should be duplicated"))
        self.Subvolume = None
        #self.MoveWithHost = False
        self.Type = "Component"

    def onDocumentRestored(self,obj):

        Component.setProperties(self,obj)

    def execute(self,obj):

        if self.clone(obj):
            return
        if obj.Base:
            shape = self.spread(obj,obj.Base.Shape)
            if obj.Additions or obj.Subtractions:
                shape = self.processSubShapes(obj,shape)
            obj.Shape = shape

    def __getstate__(self):

        # for compatibility with 0.17
        if hasattr(self,"Type"):
            return self.Type
        return "Component"

    def __setstate__(self,state):

        return None

    def onBeforeChange(self,obj,prop):

        if prop == "Placement":
            self.oldPlacement = FreeCAD.Placement(obj.Placement)

    def onChanged(self,obj,prop):

        if prop == "Placement":
            if hasattr(self,"oldPlacement"):
                if self.oldPlacement:
                    import DraftVecUtils
                    deltap = obj.Placement.Base.sub(self.oldPlacement.Base)
                    if deltap.Length == 0:
                        deltap = None
                    v = FreeCAD.Vector(0,0,1)
                    deltar = FreeCAD.Rotation(self.oldPlacement.Rotation.multVec(v),obj.Placement.Rotation.multVec(v))
                    #print "Rotation",deltar.Axis,deltar.Angle
                    if deltar.Angle < 0.0001:
                        deltar = None
                    for child in self.getMovableChildren(obj):
                        #print "moving ",child.Label
                        if deltar:
                            #child.Placement.Rotation = child.Placement.Rotation.multiply(deltar) - not enough, child must also move
                            # use shape methods to obtain a correct placement
                            import Part,math
                            shape = Part.Shape()
                            shape.Placement = child.Placement
                            #print("angle before rotation:",shape.Placement.Rotation.Angle)
                            #print("rotation angle:",math.degrees(deltar.Angle))
                            shape.rotate(DraftVecUtils.tup(self.oldPlacement.Base), DraftVecUtils.tup(deltar.Axis), math.degrees(deltar.Angle))
                            #print("angle after rotation:",shape.Placement.Rotation.Angle)
                            child.Placement = shape.Placement
                        if deltap:
                            child.Placement.move(deltap)

    def getMovableChildren(self,obj):

        ilist = obj.Additions + obj.Subtractions
        for o in obj.InList:
            if hasattr(o,"Hosts"):
                if obj in o.Hosts:
                    ilist.append(o)
            elif hasattr(o,"Host"):
                if obj == o.Host:
                    ilist.append(o)
        ilist2 = []
        for o in ilist:
            if hasattr(o,"MoveWithHost"):
                if o.MoveWithHost:
                    ilist2.append(o)
            else:
                ilist2.append(o)
        return ilist2

    def clone(self,obj):

        "if this object is a clone, sets the shape. Returns True if this is the case"
        if hasattr(obj,"CloneOf"):
            if obj.CloneOf:
                if (Draft.getType(obj.CloneOf) == Draft.getType(obj)) or (Draft.getType(obj) in ["Component","BuildingPart"]):
                    pl = obj.Placement
                    obj.Shape = obj.CloneOf.Shape.copy()
                    obj.Placement = pl
                    for prop in ["Length","Width","Height","Thickness","Area","PerimeterLength","HorizontalArea","VerticalArea"]:
                        if hasattr(obj,prop) and hasattr(obj.CloneOf,prop):
                            setattr(obj,prop,getattr(obj.CloneOf,prop))
                    return True
        return False

    def getSiblings(self,obj):

        "returns a list of objects with the same type and same base as this object"
        if not hasattr(obj,"Base"):
            return []
        if not obj.Base:
            return []
        siblings = []
        for o in obj.Base.InList:
            if hasattr(o,"Base"):
                if o.Base:
                    if o.Base.Name == obj.Base.Name:
                        if o.Name != obj.Name:
                            if Draft.getType(o) == Draft.getType(obj):
                                siblings.append(o)
        return siblings

    def getExtrusionData(self,obj):

        "returns (shape,extrusion vector,placement) or None"
        if hasattr(obj,"CloneOf"):
            if obj.CloneOf:
                if hasattr(obj.CloneOf,"Proxy"):
                    if hasattr(obj.CloneOf.Proxy,"getExtrusionData"):
                        data = obj.CloneOf.Proxy.getExtrusionData(obj.CloneOf)
                        if data:
                            return data
        if obj.Base:
            if obj.Base.isDerivedFrom("Part::Extrusion"):
                if obj.Base.Base:
                    base,placement = self.rebase(obj.Base.Base.Shape)
                    extrusion = FreeCAD.Vector(obj.Base.Dir)
                    if extrusion.Length == 0:
                        extrusion = FreeCAD.Vector(0,0,1)
                    else:
                        extrusion = placement.inverse().Rotation.multVec(extrusion)
                    if hasattr(obj.Base,"LengthFwd"):
                        if obj.Base.LengthFwd.Value:
                            extrusion = extrusion.multiply(obj.Base.LengthFwd.Value)
                    if not self.isIdentity(obj.Base.Placement):
                        placement = placement.multiply(obj.Base.Placement)
                    return (base,extrusion,placement)
            elif obj.Base.isDerivedFrom("Part::MultiFuse"):
                rshapes = []
                revs = []
                rpls = []
                for sub in obj.Base.Shapes:
                    if sub.isDerivedFrom("Part::Extrusion"):
                        if sub.Base:
                            base,placement = self.rebase(sub.Base.Shape)
                            extrusion = FreeCAD.Vector(sub.Dir)
                            if extrusion.Length == 0:
                                extrusion = FreeCAD.Vector(0,0,1)
                            else:
                                extrusion = placement.inverse().Rotation.multVec(extrusion)
                            if hasattr(sub,"LengthFwd"):
                                if sub.LengthFwd.Value:
                                    extrusion = extrusion.multiply(sub.LengthFwd.Value)
                            placement = obj.Placement.multiply(placement)
                            rshapes.append(base)
                            revs.append(extrusion)
                            rpls.append(placement)
                    else:
                        exdata = ArchCommands.getExtrusionData(sub.Shape)
                        if exdata:
                            base,placement = self.rebase(exdata[0])
                            extrusion = placement.inverse().Rotation.multVec(exdata[1])
                            placement = obj.Placement.multiply(placement)
                            rshapes.append(base)
                            revs.append(extrusion)
                            rpls.append(placement)
                if rshapes and revs and rpls:
                    return (rshapes,revs,rpls)
        return None

    def rebase(self,shape,hint=None):

        """returns a shape that is a copy of the original shape
        but centered on the (0,0) origin, and a placement that is needed to
        reposition that shape to its original location/orientation.
        hint can be a vector that indicates the preferred normal direction"""

        import DraftGeomUtils,math
        if not isinstance(shape,list):
            shape = [shape]
        if hasattr(shape[0],"CenterOfMass"):
            v = shape[0].CenterOfMass
        else:
            v = shape[0].BoundBox.Center
        n = DraftGeomUtils.getNormal(shape[0])
        if hint and hint.getAngle(n) > 1.58:
            n = n.negative()
        r = FreeCAD.Rotation(FreeCAD.Vector(0,0,1),n)
        if round(abs(r.Angle),8) == round(math.pi,8):
            r = FreeCAD.Rotation()
        shapes = []
        for s in shape:
            s = s.copy()
            s.translate(v.negative())
            s.rotate(FreeCAD.Vector(0,0,0),r.Axis,math.degrees(-r.Angle))
            shapes.append(s)
        p = FreeCAD.Placement()
        p.Base = v
        p.Rotation = r
        if len(shapes) == 1:
            return (shapes[0],p)
        else:
            return(shapes,p)

    def hideSubobjects(self,obj,prop):

        "Hides subobjects when a subobject lists change"

        if FreeCAD.GuiUp:
            if prop in ["Additions","Subtractions"]:
                if hasattr(obj,prop):
                    for o in getattr(obj,prop):
                        if (Draft.getType(o) != "Window") and (not Draft.isClone(o,"Window",True)):
                            if (Draft.getType(obj) == "Wall"):
                                if (Draft.getType(o) == "Roof"):
                                    continue
                            o.ViewObject.hide()
            elif prop in ["Mesh"]:
                if hasattr(obj,prop):
                    o = getattr(obj,prop)
                    if o:
                        o.ViewObject.hide()

    def processSubShapes(self,obj,base,placement=None):

        "Adds additions and subtractions to a base shape"

        import Draft,Part
        #print("Processing subshapes of ",obj.Label, " : ",obj.Additions)

        if placement:
            if self.isIdentity(placement):
                placement = None
            else:
                placement = FreeCAD.Placement(placement)
                placement = placement.inverse()

        # treat additions
        for o in obj.Additions:

            if not base:
                if o.isDerivedFrom("Part::Feature"):
                    base = o.Shape
            else:
                if base.isNull():
                    if o.isDerivedFrom("Part::Feature"):
                        base = o.Shape
                else:
                    # special case, both walls with coinciding endpoints
                    import ArchWall
                    js = ArchWall.mergeShapes(o,obj)
                    if js:
                        add = js.cut(base)
                        if placement:
                            add.Placement = add.Placement.multiply(placement)
                        base = base.fuse(add)

                    elif o.isDerivedFrom("Part::Feature"):
                        if o.Shape:
                            if not o.Shape.isNull():
                                if o.Shape.Solids:
                                    s = o.Shape.copy()
                                    if placement:
                                        s.Placement = s.Placement.multiply(placement)
                                    if base:
                                        if base.Solids:
                                            try:
                                                base = base.fuse(s)
                                            except Part.OCCError:
                                                print("Arch: unable to fuse object ", obj.Name, " with ", o.Name)
                                    else:
                                        base = s

        # treat subtractions
        subs = obj.Subtractions
        for link in obj.InList:
            if hasattr(link,"Hosts"):
                for host in link.Hosts:
                    if host == obj:
                        subs.append(link)
        for o in subs:

            if base:
                if base.isNull():
                    base = None

            if base:
                if (Draft.getType(o) == "Window") or (Draft.isClone(o,"Window",True)):
                        # windows can be additions or subtractions, treated the same way
                        f = o.Proxy.getSubVolume(o)
                        if f:
                            if base.Solids and f.Solids:
                                if placement:
                                    f.Placement = f.Placement.multiply(placement)
                                if len(base.Solids) > 1:
                                    base = Part.makeCompound([sol.cut(f) for sol in base.Solids])
                                else:
                                    base = base.cut(f)

                elif (Draft.getType(o) == "Roof") or (Draft.isClone(o,"Roof")):
                    # roofs define their own special subtraction volume
                    f = o.Proxy.getSubVolume(o)
                    if f:
                        if base.Solids and f.Solids:
                            if len(base.Solids) > 1:
                                base = Part.makeCompound([sol.cut(f) for sol in base.Solids])
                            else:
                                base = base.cut(f)

                elif o.isDerivedFrom("Part::Feature"):
                    if o.Shape:
                        if not o.Shape.isNull():
                            if o.Shape.Solids and base.Solids:
                                    s = o.Shape.copy()
                                    if placement:
                                        s.Placement = s.Placement.multiply(placement)
                                    try:
                                        if len(base.Solids) > 1:
                                            base = Part.makeCompound([sol.cut(s) for sol in base.Solids])
                                        else:
                                            base = base.cut(s)
                                    except Part.OCCError:
                                        print("Arch: unable to cut object ",o.Name, " from ", obj.Name)
        return base

    def spread(self,obj,shape,placement=None):

        "spreads this shape along axis positions"

        points = None
        if hasattr(obj,"Axis"):
            if obj.Axis:
                if hasattr(obj.Axis,"Proxy"):
                    if hasattr(obj.Axis.Proxy,"getPoints"):
                        points = obj.Axis.Proxy.getPoints(obj.Axis)
                if not points:
                    if obj.Axis.isDerivedFrom("Part::Feature"):
                        points = [v.Point for v in obj.Axis.Shape.Vertexes]
        if points:
            shps = []
            for p in points:
                sh = shape.copy()
                sh.translate(p)
                shps.append(sh)
            import Part
            shape = Part.makeCompound(shps)
        return shape

    def isIdentity(self,placement):

        "checks if a placement is *almost* zero"

        if (placement.Base.Length < 0.000001) and (placement.Rotation.Angle < 0.000001):
            return True
        return False

    def applyShape(self,obj,shape,placement,allowinvalid=False,allownosolid=False):

        "checks and cleans the given shape, and apply it to the object"
        if shape:
            if not shape.isNull():
                if shape.isValid():
                    if shape.Solids:
                        if shape.Volume < 0:
                            shape.reverse()
                        if shape.Volume < 0:
                            FreeCAD.Console.PrintError(translate("Arch","Error computing the shape of this object")+"\n")
                            return
                        import Part
                        try:
                            r = shape.removeSplitter()
                        except Part.OCCError:
                            pass
                        else:
                            shape = r
                        p = self.spread(obj,shape,placement).Placement.copy() # for some reason this gets zeroed in next line
                        obj.Shape = self.spread(obj,shape,placement)
                        if not self.isIdentity(placement):
                            obj.Placement = placement
                        else:
                            obj.Placement = p
                    else:
                        if allownosolid:
                            obj.Shape = self.spread(obj,shape,placement)
                            if not self.isIdentity(placement):
                                obj.Placement = placement
                        else:
                            FreeCAD.Console.PrintWarning(obj.Label + " " + translate("Arch","has no solid")+"\n")
                else:
                    if allowinvalid:
                        obj.Shape = self.spread(obj,shape,placement)
                        if not self.isIdentity(placement):
                            obj.Placement = placement
                    else:
                        FreeCAD.Console.PrintWarning(obj.Label + " " + translate("Arch","has an invalid shape")+"\n")
            else:
                FreeCAD.Console.PrintWarning(obj.Label + " " + translate("Arch","has a null shape")+"\n")
        self.computeAreas(obj)

    def computeAreas(self,obj):

        "computes the area properties"
        if not obj.Shape:
            return
        if obj.Shape.isNull():
            return
        if not obj.Shape.isValid():
            return
        if not obj.Shape.Faces:
            return
        import Drawing,Part
        fmax = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch").GetInt("MaxComputeAreas",20)
        if len(obj.Shape.Faces) > fmax:
            return
        a = 0
        fset = []
        for i,f in enumerate(obj.Shape.Faces):
            try:
                ang = f.normalAt(0,0).getAngle(FreeCAD.Vector(0,0,1))
            except Part.OCCError:
                print("Debug: Error computing areas for ",obj.Label,": normalAt() Face ",i)
                return
            else:
                if (ang > 1.57) and (ang < 1.571):
                    a += f.Area
                if ang < 1.5707:
                    fset.append(f)
        if a and hasattr(obj,"VerticalArea"):
            if obj.VerticalArea.Value != a:
                obj.VerticalArea = a
        if fset and hasattr(obj,"HorizontalArea"):
            pset = []
            for f in fset:
                if f.normalAt(0,0).getAngle(FreeCAD.Vector(0,0,1)) < 0.00001:
                    # already horizontal
                    pset.append(f)
                else:
                    try:
                        pf = Part.Face(Part.Wire(Drawing.project(f,FreeCAD.Vector(0,0,1))[0].Edges))
                    except Part.OCCError:
                        # error in computing the areas. Better set them to zero than show a wrong value
                        if obj.HorizontalArea.Value != 0:
                            print("Debug: Error computing areas for ",obj.Label,": unable to project face: ",str([v.Point for v in f.Vertexes])," (face normal:",f.normalAt(0,0),")")
                            obj.HorizontalArea = 0
                        if hasattr(obj,"PerimeterLength"):
                            if obj.PerimeterLength.Value != 0:
                                obj.PerimeterLength = 0
                    else:
                        pset.append(pf)
            if pset:
                self.flatarea = pset.pop()
                for f in pset:
                    self.flatarea = self.flatarea.fuse(f)
                self.flatarea = self.flatarea.removeSplitter()
                if obj.HorizontalArea.Value != self.flatarea.Area:
                    obj.HorizontalArea = self.flatarea.Area
                if hasattr(obj,"PerimeterLength") and (len(self.flatarea.Faces) == 1):
                    if obj.PerimeterLength.Value != self.flatarea.Faces[0].OuterWire.Length:
                        obj.PerimeterLength = self.flatarea.Faces[0].OuterWire.Length


class ViewProviderComponent:

    "A default View Provider for Component objects"

    def __init__(self,vobj):

        vobj.Proxy = self
        self.Object = vobj.Object

    def updateData(self,obj,prop):

        #print(obj.Name," : updating ",prop)
        if prop == "Material":
            if obj.Material:
                if hasattr(obj.Material,"Material"):
                    if 'DiffuseColor' in obj.Material.Material:
                        if "(" in obj.Material.Material['DiffuseColor']:
                            c = tuple([float(f) for f in obj.Material.Material['DiffuseColor'].strip("()").split(",")])
                            if obj.ViewObject:
                                if obj.ViewObject.ShapeColor != c:
                                    obj.ViewObject.ShapeColor = c
                    if 'Transparency' in obj.Material.Material:
                            t = int(obj.Material.Material['Transparency'])
                            if obj.ViewObject:
                                if obj.ViewObject.Transparency != t:
                                    obj.ViewObject.Transparency = t
        elif prop == "Shape":
            if obj.Base:
                if obj.Base.isDerivedFrom("Part::Compound"):
                    if obj.ViewObject.DiffuseColor != obj.Base.ViewObject.DiffuseColor:
                        if len(obj.Base.ViewObject.DiffuseColor) > 1:
                            obj.ViewObject.DiffuseColor = obj.Base.ViewObject.DiffuseColor
                            obj.ViewObject.update()
                        #self.onChanged(obj.ViewObject,"ShapeColor")
        elif prop == "CloneOf":
            if obj.CloneOf:
                mat = None
                if hasattr(obj,"Material"):
                    if obj.Material:
                        mat = obj.Material
                if not mat:
                    if obj.ViewObject.DiffuseColor != obj.CloneOf.ViewObject.DiffuseColor:
                        if len(obj.CloneOf.ViewObject.DiffuseColor) > 1:
                            obj.ViewObject.DiffuseColor = obj.CloneOf.ViewObject.DiffuseColor
                            obj.ViewObject.update()
                            #self.onChanged(obj.ViewObject,"ShapeColor")
        return

    def getIcon(self):

        import Arch_rc
        if hasattr(self,"Object"):
            if hasattr(self.Object,"CloneOf"):
                if self.Object.CloneOf:
                    return ":/icons/Arch_Component_Clone.svg"
        return ":/icons/Arch_Component.svg"

    def onChanged(self,vobj,prop):

        #print(vobj.Object.Name, " : changing ",prop)
        #if prop == "Visibility":
            #for obj in vobj.Object.Additions+vobj.Object.Subtractions:
            #    if (Draft.getType(obj) == "Window") or (Draft.isClone(obj,"Window",True)):
            #        obj.ViewObject.Visibility = vobj.Visibility
            # this would now hide all previous windows... Not the desired behaviour anymore.
        if prop == "DiffuseColor":
            if hasattr(vobj.Object,"CloneOf"):
                if vobj.Object.CloneOf:
                    if len(vobj.Object.CloneOf.ViewObject.DiffuseColor) > 1:
                        if vobj.DiffuseColor != vobj.Object.CloneOf.ViewObject.DiffuseColor:
                            vobj.DiffuseColor = vobj.Object.CloneOf.ViewObject.DiffuseColor
                            vobj.update()
        elif prop == "ShapeColor":
            # restore DiffuseColor after overridden by ShapeColor
            if hasattr(vobj,"DiffuseColor"):
                if len(vobj.DiffuseColor) > 1:
                    d = vobj.DiffuseColor
                    vobj.DiffuseColor = d
        return

    def attach(self,vobj):

        from pivy import coin
        self.Object = vobj.Object
        self.hiresgroup = coin.SoSeparator()
        self.meshcolor = coin.SoBaseColor()
        self.hiresgroup.addChild(self.meshcolor)
        self.hiresgroup.setName("HiRes")
        vobj.addDisplayMode(self.hiresgroup,"HiRes");
        return

    def getDisplayModes(self,vobj):

        modes=["HiRes"]
        return modes

    def setDisplayMode(self,mode):

        if hasattr(self,"meshnode"):
            if self.meshnode:
                self.hiresgroup.removeChild(self.meshnode)
                del self.meshnode
        if mode == "HiRes":
            from pivy import coin
            m = None
            if hasattr(self,"Object"):
                if hasattr(self.Object,"HiRes"):
                    if self.Object.HiRes:
                        # if the file was recently loaded, the node is not present yet
                        self.Object.HiRes.ViewObject.show()
                        self.Object.HiRes.ViewObject.hide()
                        m = self.Object.HiRes.ViewObject.RootNode
                if not m:
                    if hasattr(self.Object,"CloneOf"):
                        if self.Object.CloneOf:
                            if hasattr(self.Object.CloneOf,"HiRes"):
                                if self.Object.CloneOf.HiRes:
                                    # if the file was recently loaded, the node is not present yet
                                    self.Object.CloneOf.HiRes.ViewObject.show()
                                    self.Object.CloneOf.HiRes.ViewObject.hide()
                                    m = self.Object.CloneOf.HiRes.ViewObject.RootNode
            if m:
                self.meshnode = m.copy()
                for c in self.meshnode.getChildren():
                    # switch the first found SoSwitch on
                    if isinstance(c,coin.SoSwitch):
                        num = 0
                        if c.getNumChildren() > 0:
                            if c.getChild(0).getName() == "HiRes":
                                num = 1
                        #print "getting node ",num," for ",self.Object.Label
                        c.whichChild = num
                        break
                self.hiresgroup.addChild(self.meshnode)
            else:
                return "Flat Lines"
        return mode

    def __getstate__(self):

        return None

    def __setstate__(self,state):

        return None

    def claimChildren(self):

        if hasattr(self,"Object"):
            c = []
            if hasattr(self.Object,"Base"):
                if Draft.getType(self.Object) != "Wall":
                    c = [self.Object.Base]
                elif Draft.getType(self.Object.Base) == "Space":
                    c = []
                else:
                    c = [self.Object.Base]
            if hasattr(self.Object,"Additions"):
                c.extend(self.Object.Additions)
            if hasattr(self.Object,"Subtractions"):
                for s in self.Object.Subtractions:
                    if Draft.getType(self.Object) == "Wall":
                        if Draft.getType(s) == "Roof":
                            continue
                    c.append(s)
            for link in ["Armatures","Group"]:
                if hasattr(self.Object,link):
                    objlink = getattr(self.Object,link)
                    c.extend(objlink)
            for link in ["Tool","Subvolume","Mesh","HiRes"]:
                if hasattr(self.Object,link):
                    objlink = getattr(self.Object,link)
                    if objlink:
                        c.append(objlink)
            for link in self.Object.InList:
                if hasattr(link,"Host"):
                    if link.Host:
                        if link.Host == self.Object:
                            c.append(link)
                elif hasattr(link,"Hosts"):
                    for host in link.Hosts:
                        if host == self.Object:
                            c.append(link)
            return c
        return []

    def setEdit(self,vobj,mode):

        if mode == 0:
            taskd = ComponentTaskPanel()
            taskd.obj = self.Object
            taskd.update()
            FreeCADGui.Control.showDialog(taskd)
            return True
        return False

    def unsetEdit(self,vobj,mode):

        FreeCADGui.Control.closeDialog()
        return False

    def setupContextMenu(self,vobj,menu):

        from PySide import QtCore,QtGui
        action1 = QtGui.QAction(QtGui.QIcon(":/icons/Arch_ToggleSubs.svg"),translate("Arch","Toggle subcomponents"),menu)
        QtCore.QObject.connect(action1,QtCore.SIGNAL("triggered()"),self.toggleSubcomponents)
        menu.addAction(action1)

    def toggleSubcomponents(self):

        FreeCADGui.runCommand("Arch_ToggleSubs")

    def areDifferentColors(self,a,b):

        if len(a) != len(b):
            return True
        for i in range(len(a)):
            if abs(sum(a[i]) - sum(b[i])) > 0.00001:
                return True
        return False

    def colorize(self,obj,force=False):

        if obj.CloneOf:
            if self.areDifferentColors(obj.ViewObject.DiffuseColor,obj.CloneOf.ViewObject.DiffuseColor) or force:
                obj.ViewObject.DiffuseColor = obj.CloneOf.ViewObject.DiffuseColor


class ArchSelectionObserver:

    """ArchSelectionObserver([origin,watched,hide,nextCommand]): The ArchSelectionObserver
    object can be added as a selection observer to the FreeCAD Gui. If watched is given (a
    document object), the observer will be triggered only when that object is selected/unselected.
    If hide is True, the watched object will be hidden. If origin is given (a document
    object), that object will have its visibility/selectability restored. If nextCommand
    is given (a FreeCAD command), it will be executed on leave."""

    def __init__(self,origin=None,watched=None,hide=True,nextCommand=None):

        self.origin = origin
        self.watched = watched
        self.hide = hide
        self.nextCommand = nextCommand

    def addSelection(self,document, object, element, position):

        if not self.watched:
            FreeCADGui.Selection.removeObserver(FreeCAD.ArchObserver)
            if self.nextCommand:
                FreeCADGui.runCommand(self.nextCommand)
            del FreeCAD.ArchObserver
        elif object == self.watched.Name:
            if not element:
                FreeCAD.Console.PrintMessage(translate("Arch","Closing Sketch edit"))
                if self.hide:
                    if self.origin:
                        self.origin.ViewObject.Transparency = 0
                        self.origin.ViewObject.Selectable = True
                    self.watched.ViewObject.hide()
                FreeCADGui.activateWorkbench("ArchWorkbench")
                if hasattr(FreeCAD,"ArchObserver"):
                    FreeCADGui.Selection.removeObserver(FreeCAD.ArchObserver)
                    del FreeCAD.ArchObserver
                if self.nextCommand:
                    FreeCADGui.Selection.clearSelection()
                    FreeCADGui.Selection.addSelection(self.watched)
                    FreeCADGui.runCommand(self.nextCommand)



class SelectionTaskPanel:

    """A temporary TaskPanel to wait for a selection"""

    def __init__(self):
        self.baseform = QtGui.QLabel()
        self.baseform.setText(QtGui.QApplication.translate("Arch", "Please select a base object", None))

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Cancel)

    def reject(self):
        if hasattr(FreeCAD,"ArchObserver"):
            FreeCADGui.Selection.removeObserver(FreeCAD.ArchObserver)
            del FreeCAD.ArchObserver
        return True


class ComponentTaskPanel:

    '''The default TaskPanel for all Arch components'''

    def __init__(self):

        # the panel has a tree widget that contains categories
        # for the subcomponents, such as additions, subtractions.
        # the categories are shown only if they are not empty.

        self.obj = None
        self.attribs = ["Base","Additions","Subtractions","Objects","Components","Axes","Fixtures","Group","Hosts"]
        self.baseform = QtGui.QWidget()
        self.baseform.setObjectName("TaskPanel")
        self.grid = QtGui.QGridLayout(self.baseform)
        self.grid.setObjectName("grid")
        self.title = QtGui.QLabel(self.baseform)
        self.grid.addWidget(self.title, 0, 0, 1, 2)
        self.form = self.baseform

        # tree
        self.tree = QtGui.QTreeWidget(self.baseform)
        self.grid.addWidget(self.tree, 1, 0, 1, 2)
        self.tree.setColumnCount(1)
        self.tree.header().hide()

        # buttons
        self.addButton = QtGui.QPushButton(self.baseform)
        self.addButton.setObjectName("addButton")
        self.addButton.setIcon(QtGui.QIcon(":/icons/Arch_Add.svg"))
        self.grid.addWidget(self.addButton, 3, 0, 1, 1)
        self.addButton.setEnabled(False)

        self.delButton = QtGui.QPushButton(self.baseform)
        self.delButton.setObjectName("delButton")
        self.delButton.setIcon(QtGui.QIcon(":/icons/Arch_Remove.svg"))
        self.grid.addWidget(self.delButton, 3, 1, 1, 1)
        self.delButton.setEnabled(False)

        self.ifcButton = QtGui.QPushButton(self.baseform)
        self.ifcButton.setObjectName("ifcButton")
        self.ifcButton.setIcon(QtGui.QIcon(":/icons/IFC.svg"))
        self.grid.addWidget(self.ifcButton, 4, 0, 1, 2)
        self.ifcButton.hide()

        self.classButton = QtGui.QPushButton(self.baseform)
        self.classButton.setObjectName("classButton")
        self.grid.addWidget(self.classButton, 5, 0, 1, 2)
        try:
            import BimClassification
        except:
            self.classButton.hide()
        else:
            import os
            self.classButton.setIcon(QtGui.QIcon(os.path.join(os.path.dirname(BimClassification.__file__),"icons","BIM_Classification.svg")))

        QtCore.QObject.connect(self.addButton, QtCore.SIGNAL("clicked()"), self.addElement)
        QtCore.QObject.connect(self.delButton, QtCore.SIGNAL("clicked()"), self.removeElement)
        QtCore.QObject.connect(self.ifcButton, QtCore.SIGNAL("clicked()"), self.editIfcProperties)
        QtCore.QObject.connect(self.classButton, QtCore.SIGNAL("clicked()"), self.editClass)
        QtCore.QObject.connect(self.tree, QtCore.SIGNAL("itemClicked(QTreeWidgetItem*,int)"), self.check)
        QtCore.QObject.connect(self.tree, QtCore.SIGNAL("itemDoubleClicked(QTreeWidgetItem *,int)"), self.editObject)
        self.update()

    def isAllowedAlterSelection(self):

        return True

    def isAllowedAlterView(self):

        return True

    def getStandardButtons(self):

        return int(QtGui.QDialogButtonBox.Ok)

    def check(self,wid,col):

        if not wid.parent():
            self.delButton.setEnabled(False)
            if self.obj:
                sel = FreeCADGui.Selection.getSelection()
                if sel:
                    if not(self.obj in sel):
                        self.addButton.setEnabled(True)
        else:
            self.delButton.setEnabled(True)
            self.addButton.setEnabled(False)

    def getIcon(self,obj):

        if hasattr(obj.ViewObject,"Proxy"):
            if hasattr(obj.ViewObject.Proxy,"getIcon"):
                return QtGui.QIcon(obj.ViewObject.Proxy.getIcon())
        if obj.isDerivedFrom("Sketcher::SketchObject"):
            return QtGui.QIcon(":/icons/Sketcher_Sketch.svg")
        if obj.isDerivedFrom("App::DocumentObjectGroup"):
            return QtGui.QApplication.style().standardIcon(QtGui.QStyle.SP_DirIcon)
        return QtGui.QIcon(":/icons/Tree_Part.svg")

    def update(self):

        'fills the treewidget'
        self.tree.clear()
        dirIcon = QtGui.QApplication.style().standardIcon(QtGui.QStyle.SP_DirIcon)
        for a in self.attribs:
            setattr(self,"tree"+a,QtGui.QTreeWidgetItem(self.tree))
            c = getattr(self,"tree"+a)
            c.setIcon(0,dirIcon)
            c.ChildIndicatorPolicy = 2
            if self.obj:
                if not hasattr(self.obj,a):
                           c.setHidden(True)
            else:
                c.setHidden(True)
        if self.obj:
            for attrib in self.attribs:
                if hasattr(self.obj,attrib):
                    Oattrib = getattr(self.obj,attrib)
                    Tattrib = getattr(self,"tree"+attrib)
                    if Oattrib:
                        if attrib == "Base":
                            Oattrib = [Oattrib]
                        for o in Oattrib:
                            item = QtGui.QTreeWidgetItem()
                            item.setText(0,o.Label)
                            item.setToolTip(0,o.Name)
                            item.setIcon(0,self.getIcon(o))
                            Tattrib.addChild(item)
                        self.tree.expandItem(Tattrib)
            if hasattr(self.obj,"IfcProperties"):
                if isinstance(self.obj.IfcProperties,dict):
                    self.ifcButton.show()
        self.retranslateUi(self.baseform)

    def addElement(self):

        it = self.tree.currentItem()
        if it:
            mod = None
            for a in self.attribs:
                if it.text(0) == getattr(self,"tree"+a).text(0):
                    mod = a
            for o in FreeCADGui.Selection.getSelection():
                addToComponent(self.obj,o,mod)
        self.update()

    def removeElement(self):

        it = self.tree.currentItem()
        if it:
            comp = FreeCAD.ActiveDocument.getObject(str(it.toolTip(0)))
            removeFromComponent(self.obj,comp)
        self.update()

    def accept(self):

        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.ActiveDocument.resetEdit()
        return True

    def editObject(self,wid,col):

        if wid.parent():
            obj = FreeCAD.ActiveDocument.getObject(str(wid.toolTip(0)))
            if obj:
                self.obj.ViewObject.Transparency = 80
                self.obj.ViewObject.Selectable = False
                obj.ViewObject.show()
                self.accept()
                if obj.isDerivedFrom("Sketcher::SketchObject"):
                    FreeCADGui.activateWorkbench("SketcherWorkbench")
                FreeCAD.ArchObserver = ArchSelectionObserver(self.obj,obj)
                FreeCADGui.Selection.addObserver(FreeCAD.ArchObserver)
                FreeCADGui.ActiveDocument.setEdit(obj.Name,0)

    def retranslateUi(self, TaskPanel):

        self.baseform.setWindowTitle(QtGui.QApplication.translate("Arch", "Component", None))
        self.delButton.setText(QtGui.QApplication.translate("Arch", "Remove", None))
        self.addButton.setText(QtGui.QApplication.translate("Arch", "Add", None))
        self.title.setText(QtGui.QApplication.translate("Arch", "Components of this object", None))
        self.treeBase.setText(0,QtGui.QApplication.translate("Arch", "Base component", None))
        self.treeAdditions.setText(0,QtGui.QApplication.translate("Arch", "Additions", None))
        self.treeSubtractions.setText(0,QtGui.QApplication.translate("Arch", "Subtractions", None))
        self.treeObjects.setText(0,QtGui.QApplication.translate("Arch", "Objects", None))
        self.treeAxes.setText(0,QtGui.QApplication.translate("Arch", "Axes", None))
        self.treeComponents.setText(0,QtGui.QApplication.translate("Arch", "Components", None))
        self.treeFixtures.setText(0,QtGui.QApplication.translate("Arch", "Fixtures", None))
        self.treeGroup.setText(0,QtGui.QApplication.translate("Arch", "Group", None))
        self.treeHosts.setText(0,QtGui.QApplication.translate("Arch", "Hosts", None))
        self.ifcButton.setText(QtGui.QApplication.translate("Arch", "Edit IFC properties", None))
        self.classButton.setText(QtGui.QApplication.translate("Arch", "Edit standard code", None))

    def editIfcProperties(self):

        if hasattr(self,"ifcEditor"):
            if self.ifcEditor:
                self.ifcEditor.hide()
            del self.ifcEditor
        if not self.obj:
            return
        if not hasattr(self.obj,"IfcProperties"):
            return
        if not isinstance(self.obj.IfcProperties,dict):
            return
        import Arch_rc,csv,os

        # get presets
        self.ptypes = SimplePropertyTypes + MeasurePropertyTypes
        self.plabels = [''.join(map(lambda x: x if x.islower() else " "+x, t[3:]))[1:] for t in self.ptypes]
        self.psetdefs = {}
        psetspath = os.path.join(FreeCAD.getResourceDir(),"Mod","Arch","Presets","pset_definitions.csv")
        if os.path.exists(psetspath):
            with open(psetspath, "r") as csvfile:
                reader = csv.reader(csvfile, delimiter=';')
                for row in reader:
                    self.psetdefs[row[0]] = row[1:]
        self.psetkeys = [''.join(map(lambda x: x if x.islower() else " "+x, t[5:]))[1:] for t in self.psetdefs.keys()]
        self.psetkeys.sort()
        self.ifcEditor = FreeCADGui.PySideUic.loadUi(":/ui/DialogIfcProperties.ui")
        # center the dialog over FreeCAD window
        mw = FreeCADGui.getMainWindow()
        self.ifcEditor.move(mw.frameGeometry().topLeft() + mw.rect().center() - self.ifcEditor.rect().center())
        self.ifcModel = QtGui.QStandardItemModel()
        self.ifcEditor.treeProperties.setModel(self.ifcModel)
        #self.ifcEditor.treeProperties.setDragDropMode(QtGui.QAbstractItemView.InternalMove)
        self.ifcEditor.treeProperties.setUniformRowHeights(True)
        self.ifcEditor.treeProperties.setItemDelegate(IfcEditorDelegate(dialog=self,ptypes=self.ptypes,plabels=self.plabels))
        self.ifcModel.setHorizontalHeaderLabels([QtGui.QApplication.translate("Arch", "Property", None),
                                                 QtGui.QApplication.translate("Arch", "Type", None),
                                                 QtGui.QApplication.translate("Arch", "Value", None)])
        # set combos
        self.ifcEditor.comboProperty.addItems([QtGui.QApplication.translate("Arch", "Add property...", None)]+self.plabels)
        self.ifcEditor.comboPset.addItems([QtGui.QApplication.translate("Arch", "Add property set...", None),
                                           QtGui.QApplication.translate("Arch", "New...", None)]+self.psetkeys)
        # set UUID
        if "IfcUID" in self.obj.IfcAttributes:
            self.ifcEditor.labelUUID.setText(self.obj.IfcAttributes["IfcUID"])
        # fill the tree
        psets = {}
        for pname,value in self.obj.IfcProperties.items():
            # properties in IfcProperties dict are stored as "key":"pset;;type;;value" or "key":"type;;value"
            value = value.split(";;")
            if len(value) == 3:
                pset = value[0]
                ptype = value[1]
                pvalue = value[2]
            elif len(value) == 2:
                pset = "Default property set"
                ptype = value[0]
                pvalue = value[1]
            else:
                continue
            plabel = ptype
            if ptype in self.ptypes:
                plabel = self.plabels[self.ptypes.index(ptype)]
            psets.setdefault(pset,[]).append([pname,plabel,pvalue])
        for pset,plists in psets.items():
            top = QtGui.QStandardItem(pset)
            top.setDragEnabled(False)
            top.setToolTip("PropertySet")
            self.ifcModel.appendRow([top,QtGui.QStandardItem(),QtGui.QStandardItem()])
            for plist in plists:
                it1 = QtGui.QStandardItem(plist[0])
                it1.setDropEnabled(False)
                it2 = QtGui.QStandardItem(plist[1])
                it2.setDropEnabled(False)
                it3 = QtGui.QStandardItem(plist[2])
                it3.setDropEnabled(False)
                top.appendRow([it1,it2,it3])
            top.sortChildren(0)
        # span top levels
        idx = self.ifcModel.invisibleRootItem().index()
        for i in range(self.ifcModel.rowCount()):
            if self.ifcModel.item(i,0).hasChildren():
                self.ifcEditor.treeProperties.setFirstColumnSpanned(i, idx, True)
        self.ifcEditor.treeProperties.expandAll()
        QtCore.QObject.connect(self.ifcEditor.buttonBox, QtCore.SIGNAL("accepted()"), self.acceptIfcProperties)
        QtCore.QObject.connect(self.ifcEditor.comboProperty, QtCore.SIGNAL("currentIndexChanged(int)"), self.addIfcProperty)
        QtCore.QObject.connect(self.ifcEditor.comboPset, QtCore.SIGNAL("currentIndexChanged(int)"), self.addIfcPset)
        QtCore.QObject.connect(self.ifcEditor.buttonDelete, QtCore.SIGNAL("clicked()"), self.removeIfcProperty)
        self.ifcEditor.treeProperties.setSortingEnabled(True)
        # set checkboxes
        if "FlagForceBrep" in self.obj.IfcAttributes:
            self.ifcEditor.checkBrep.setChecked(self.obj.IfcAttributes["FlagForceBrep"] == "True")
        if "FlagParametric" in self.obj.IfcAttributes:
            self.ifcEditor.checkParametric.setChecked(self.obj.IfcAttributes["FlagParametric"] == "True")
        self.ifcEditor.show()

    def acceptIfcProperties(self):

        if hasattr(self,"ifcEditor") and self.ifcEditor:
            self.ifcEditor.hide()
            ifcdict = {}
            for row in range(self.ifcModel.rowCount()):
                pset = self.ifcModel.item(row,0).text()
                if self.ifcModel.item(row,0).hasChildren():
                    for childrow in range(self.ifcModel.item(row,0).rowCount()):
                        prop = self.ifcModel.item(row,0).child(childrow,0).text()
                        ptype = self.ifcModel.item(row,0).child(childrow,1).text()
                        if not ptype.startswith("Ifc"):
                            ptype = self.ptypes[self.plabels.index(ptype)]
                        pvalue = self.ifcModel.item(row,0).child(childrow,2).text()
                        if sys.version_info.major >= 3:
                            ifcdict[prop] = pset+";;"+ptype+";;"+pvalue
                        else:
                            # keys cannot be unicode
                            ifcdict[prop.encode("utf8")] = pset+";;"+ptype+";;"+pvalue
            ifcattrs = self.obj.IfcAttributes
            ifcattrs["IfcUID"] = self.ifcEditor.labelUUID.text()
            ifcattrs["FlagForceBrep"] = str(self.ifcEditor.checkBrep.isChecked())
            ifcattrs["FlagParametric"] = str(self.ifcEditor.checkParametric.isChecked())
            if (ifcdict != self.obj.IfcProperties) or (ifcattrs != self.obj.IfcAttributes):
                FreeCAD.ActiveDocument.openTransaction("Change Ifc Properties")
                if ifcdict != self.obj.IfcProperties:
                    self.obj.IfcProperties = ifcdict
                if ifcattrs != self.obj.IfcAttributes:
                    self.obj.IfcAttributes = ifcattrs
                FreeCAD.ActiveDocument.commitTransaction()
            del self.ifcEditor

    def addIfcProperty(self,idx=0,pset=None,prop=None,ptype=None):

        if hasattr(self,"ifcEditor") and self.ifcEditor:
            if not pset:
                sel = self.ifcEditor.treeProperties.selectedIndexes()
                if sel:
                    item = self.ifcModel.itemFromIndex(sel[0])
                    if item.toolTip() == "PropertySet":
                        pset = item
            if pset:
                if not prop:
                    prop = QtGui.QApplication.translate("Arch", "New property", None)
                if not ptype:
                    if idx > 0:
                        ptype = self.plabels[idx-1]
                if prop and ptype:
                    if ptype in self.ptypes:
                        ptype = self.plabels[self.ptypes.index(ptype)]
                    it1 = QtGui.QStandardItem(prop)
                    it1.setDropEnabled(False)
                    it2 = QtGui.QStandardItem(ptype)
                    it2.setDropEnabled(False)
                    it3 = QtGui.QStandardItem()
                    it3.setDropEnabled(False)
                    pset.appendRow([it1,it2,it3])
            if idx != 0:
                self.ifcEditor.comboProperty.setCurrentIndex(0)

    def addIfcPset(self,idx=0):

        if hasattr(self,"ifcEditor") and self.ifcEditor:
            if idx == 1:
                top = QtGui.QStandardItem(QtGui.QApplication.translate("Arch", "New property set", None))
                top.setDragEnabled(False)
                top.setToolTip("PropertySet")
                self.ifcModel.appendRow([top,QtGui.QStandardItem(),QtGui.QStandardItem()])
            elif idx > 1:
                psetlabel = self.psetkeys[idx-2]
                psetdef = "Pset_"+psetlabel.replace(" ","")
                if psetdef in self.psetdefs:
                    top = QtGui.QStandardItem(psetdef)
                    top.setDragEnabled(False)
                    top.setToolTip("PropertySet")
                    self.ifcModel.appendRow([top,QtGui.QStandardItem(),QtGui.QStandardItem()])
                    for i in range(0,len(self.psetdefs[psetdef]),2):
                        self.addIfcProperty(pset=top,prop=self.psetdefs[psetdef][i],ptype=self.psetdefs[psetdef][i+1])
            if idx != 0:
                # span top levels
                idx = self.ifcModel.invisibleRootItem().index()
                for i in range(self.ifcModel.rowCount()):
                    if self.ifcModel.item(i,0).hasChildren():
                        self.ifcEditor.treeProperties.setFirstColumnSpanned(i, idx, True)
                self.ifcEditor.treeProperties.expandAll()
                self.ifcEditor.comboPset.setCurrentIndex(0)

    def removeIfcProperty(self):

        if hasattr(self,"ifcEditor") and self.ifcEditor:
            sel = self.ifcEditor.treeProperties.selectedIndexes()
            if sel:
                if self.ifcModel.itemFromIndex(sel[0]).toolTip() == "PropertySet":
                    self.ifcModel.takeRow(sel[0].row())
                else:
                    pset = self.ifcModel.itemFromIndex(sel[0].parent())
                    pset.takeRow(sel[0].row())

    def editClass(self):

        FreeCADGui.Selection.clearSelection()
        FreeCADGui.Selection.addSelection(self.obj)
        FreeCADGui.runCommand("BIM_Classification")

if FreeCAD.GuiUp:

    class IfcEditorDelegate(QtGui.QStyledItemDelegate):


        def __init__(self, parent=None, dialog=None, ptypes=[], plabels=[], *args):

            self.dialog = dialog
            QtGui.QStyledItemDelegate.__init__(self, parent, *args)
            self.ptypes = ptypes
            self.plabels = plabels

        def createEditor(self,parent,option,index):

            if index.column() == 0: # property name
                editor = QtGui.QLineEdit(parent)
            elif index.column() == 1: # property type
                editor = QtGui.QComboBox(parent)
            else: # property value
                ptype = index.sibling(index.row(),1).data()
                if "Integer" in ptype:
                    editor = QtGui.QSpinBox(parent)
                elif "Real" in ptype:
                    editor = QtGui.QDoubleSpinBox(parent)
                    editor.setDecimals(FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Units").GetInt("Decimals",2))
                elif ("Boolean" in ptype) or ("Logical" in ptype):
                    editor = QtGui.QComboBox(parent)
                    editor.addItems(["True","False"])
                elif "Measure" in ptype:
                    editor = FreeCADGui.UiLoader().createWidget("Gui::InputField")
                    editor.setParent(parent)
                else:
                    editor = QtGui.QLineEdit(parent)
                editor.setObjectName("editor_"+ptype)
            return editor

        def setEditorData(self, editor, index):

            if index.column() == 0:
                editor.setText(index.data())
            elif index.column() == 1:
                editor.addItems(self.plabels)
                if index.data() in self.plabels:
                    idx = self.plabels.index(index.data())
                    editor.setCurrentIndex(idx)
            else:
                if "Integer" in editor.objectName():
                    try:
                        editor.setValue(int(index.data()))
                    except:
                        editor.setValue(0)
                elif "Real" in editor.objectName():
                    try:
                        editor.setValue(float(index.data()))
                    except:
                        editor.setValue(0)
                elif ("Boolean" in editor.objectName()) or ("Logical" in editor.objectName()):
                    try:
                        editor.setCurrentIndex(["true","false"].index(index.data().lower()))
                    except:
                        editor.setCurrentIndex(1)
                elif "Measure" in editor.objectName():
                    try:
                        editor.setText(index.data())
                    except:
                        editor.setValue(0)
                else:
                    editor.setText(index.data())

        def setModelData(self, editor, model, index):

            if index.column() == 0:
                model.setData(index,editor.text())
            elif index.column() == 1:
                if editor.currentIndex() > -1:
                    idx = editor.currentIndex()
                    data = self.plabels[idx]
                    model.setData(index,data)
            else:
                if ("Integer" in editor.objectName()) or ("Real" in editor.objectName()):
                    model.setData(index,str(editor.value()))
                elif ("Boolean" in editor.objectName()) or ("Logical" in editor.objectName()):
                    model.setData(index,editor.currentText())
                elif "Measure" in editor.objectName():
                    model.setData(index,editor.property("text"))
                else:
                    model.setData(index,editor.text())
            self.dialog.update()







