# ***************************************************************************
# *   Copyright (c) 2019 Yorik van Havre <yorik@uncreated.net>              *
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

import json
import math

import FreeCAD
# import Draft
import ifcopenshell
from draftutils import params

def getObjectsOfIfcType(objects, ifcType):
    results = []
    for object in objects:
        if hasattr(object,"IfcType"):
            if object.IfcType == ifcType:
                results.append(object)
    return results


def writeUnits(ifcfile,unit="metre"):
    """adds additional units settings to the given ifc file if needed"""
    # so far, only metre or foot possible (which is all revit knows anyway)

    if unit == "foot":
        d1 = ifcfile.createIfcDimensionalExponents(1,0,0,0,0,0,0)
        d2 = ifcfile.createIfcMeasureWithUnit(ifcfile.createIfcRatioMeasure(0.3048),ifcfile[13])
        d3 = ifcfile.createIfcConversionBasedUnit(d1,'LENGTHUNIT','FOOT',d2)
        d4 = ifcfile.createIfcDimensionalExponents(2,0,0,0,0,0,0)
        d5 = ifcfile.createIfcMeasureWithUnit(ifcfile.createIfcRatioMeasure(0.09290304000000001),ifcfile[14])
        d6 = ifcfile.createIfcConversionBasedUnit(d4,'AREAUNIT','SQUARE FOOT',d5)
        d7 = ifcfile.createIfcDimensionalExponents(3,0,0,0,0,0,0)
        d8 = ifcfile.createIfcMeasureWithUnit(ifcfile.createIfcRatioMeasure(0.028316846592),ifcfile[15])
        d9 = ifcfile.createIfcConversionBasedUnit(d7,'VOLUMEUNIT','CUBIC FOOT',d8)
        ifcfile.createIfcUnitAssignment((d3,d6,d9,ifcfile[18]))
    else: # default = metre, no need to add anything
        ifcfile.createIfcUnitAssignment((ifcfile[13],ifcfile[14],ifcfile[15],ifcfile[18]))
    return ifcfile


def writeQuantities(ifcfile, obj, product, history, scale):
    "append quantities to the given object"

    if hasattr(obj,"IfcData"):
        quantities = []
        if ("ExportHeight" in obj.IfcData) and obj.IfcData["ExportHeight"] and hasattr(obj,"Height"):
            quantities.append(ifcfile.createIfcQuantityLength('Height',None,None,obj.Height.Value*scale))
        if ("ExportWidth" in obj.IfcData) and obj.IfcData["ExportWidth"] and hasattr(obj,"Width"):
            quantities.append(ifcfile.createIfcQuantityLength('Width',None,None,obj.Width.Value*scale))
        if ("ExportLength" in obj.IfcData) and obj.IfcData["ExportLength"] and hasattr(obj,"Length"):
            quantities.append(ifcfile.createIfcQuantityLength('Length',None,None,obj.Length.Value*scale))
        if ("ExportHorizontalArea" in obj.IfcData) and obj.IfcData["ExportHorizontalArea"] and hasattr(obj,"HorizontalArea"):
            quantities.append(ifcfile.createIfcQuantityArea('HorizontalArea',None,None,obj.HorizontalArea.Value*(scale**2)))
        if ("ExportVerticalArea" in obj.IfcData) and obj.IfcData["ExportVerticalArea"] and hasattr(obj,"VerticalArea"):
            quantities.append(ifcfile.createIfcQuantityArea('VerticalArea',None,None,obj.VerticalArea.Value*(scale**2)))
        if ("ExportVolume" in obj.IfcData) and obj.IfcData["ExportVolume"] and obj.isDerivedFrom("Part::Feature"):
            quantities.append(ifcfile.createIfcQuantityVolume('Volume',None,None,obj.Shape.Volume*(scale**3)))
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


class SIUnitCreator:
    def __init__(self, file, text, type):
        self.prefixes = [
            "EXA", "PETA", "TERA", "GIGA", "MEGA", "KILO", "HECTO",
            "DECA", "DECI", "CENTI", "MILLI", "MICRO", "NANO", "PICO", "FEMTO",
            "ATTO"
        ]
        self.unitNames = [
            "AMPERE", "BECQUEREL", "CANDELA", "COULOMB",
            "CUBIC_METRE", "DEGREE CELSIUS", "FARAD", "GRAM", "GRAY", "HENRY",
            "HERTZ", "JOULE", "KELVIN", "LUMEN", "LUX", "MOLE", "NEWTON", "OHM",
            "PASCAL", "RADIAN", "SECOND", "SIEMENS", "SIEVERT", "SQUARE METRE",
            "METRE", "STERADIAN", "TESLA", "VOLT", "WATT", "WEBER"
        ]
        self.text = text
        self.SIUnit = file.createIfcSIUnit(None, type, self.getSIPrefix(), self.getSIUnitName())

    def getSIPrefix(self):
        for prefix in self.prefixes:
            if prefix in self.text.upper():
                return prefix
        return None

    def getSIUnitName(self):
        for unitName in self.unitNames:
            if unitName in self.text.upper():
                return unitName
        return None


class ContextCreator:
    def __init__(self, file, objects):
        self.file = file
        self.objects = objects
        self.project_object = self.getProjectObject()
        self.project_data = self.getProjectObjectData()
        self.model_context = self.createGeometricRepresentationContext()
        self.model_view_subcontext = self.createGeometricRepresentationSubContext()
        self.target_crs = self.createTargetCRS()
        self.map_conversion = self.createMapConversion()
        self.project = self.createProject()

    def createGeometricRepresentationContext(self):
        return self.file.createIfcGeometricRepresentationContext(
            None, "Model",
            3, 1.0E-05,
            self.file.by_type("IfcAxis2Placement3D")[0],
            self.createTrueNorth())

    def createGeometricRepresentationSubContext(self):
        return self.file.createIfcGeometricRepresentationSubContext(
            "Body", "Model",
            None, None, None, None,
            self.model_context, None, "MODEL_VIEW", None)

    def createTargetCRS(self):
        try:
            SIUnit = SIUnitCreator(self.file, self.project_data["map_unit"], "LENGTHUNIT")
            return self.file.createIfcProjectedCRS(
                self.project_data["name"],
                self.project_data["description"],
                self.project_data["geodetic_datum"],
                self.project_data["vertical_datum"],
                self.project_data["map_projection"],
                self.project_data["map_zone"],
                SIUnit.SIUnit
            )
        except Exception:
            return None

    def createMapConversion(self):
        try:
            return self.file.createIfcMapConversion(
                self.model_context, self.target_crs,
                float(self.project_data["eastings"]),
                float(self.project_data["northings"]),
                float(self.project_data["orthogonal_height"]),
                self.calculateXAxisAbscissa(),
                self.calculateXAxisOrdinate(),
                float(self.project_data["scale"])
            )
        except Exception:
            return None

    def createTrueNorth(self):
        return self.file.createIfcDirection(
            (self.calculateXAxisAbscissa(), self.calculateXAxisOrdinate()))

    def calculateXAxisAbscissa(self):
        if "true_north" in self.project_data:
            return math.cos(math.radians(float(self.project_data["true_north"]) + 90))
        return 0.

    def calculateXAxisOrdinate(self):
        if "true_north" in self.project_data:
            return math.sin(math.radians(float(self.project_data["true_north"]) + 90))
        return 1.

    def createProject(self):
        if not self.project_object:
            return self.createAutomaticProject()
        return self.createCustomProject()

    def createAutomaticProject(self):
        return self.file.createIfcProject(
            self.getProjectGUID(),
            self.file.by_type("IfcOwnerHistory")[0],
            FreeCAD.ActiveDocument.Name, None,
            None, None, None, [self.model_context],
            self.file.by_type("IfcUnitAssignment")[0])

    def createCustomProject(self):
        return self.file.createIfcProject(
            self.getProjectGUID(),
            self.file.by_type("IfcOwnerHistory")[0],
            self.project_object.Label, self.project_object.Description,
            self.project_object.ObjectType, self.project_object.LongName,
            self.project_object.Phase,
            [self.model_context],
            self.file.by_type("IfcUnitAssignment")[0])

    def getProjectGUID(self):
        # TODO: Do not generate a new one each time, but at least this one
        # conforms to the community consensus on how a GUID is generated.
        return ifcopenshell.guid.new()

    def getProjectObject(self):
        try:
            return getObjectsOfIfcType(self.objects, "Project")[0]
        except Exception:
            return None

    def getProjectObjectData(self):
        if not self.project_object:
            return {}
        return json.loads(self.project_object.IfcData['complex_attributes'])["RepresentationContexts"]


class recycler:

    "the compression engine - a mechanism to reuse ifc entities if needed"

    # this object has some methods identical to corresponding ifcopenshell methods,
    # but it checks if a similar entity already exists before creating a new one
    # to compress a new type, just add the necessary method here

    def __init__(self,ifcfile,template=True):

        self.ifcfile = ifcfile
        self.compress = params.get_param_arch("ifcCompress")
        self.mergeProfiles = params.get_param_arch("ifcMergeProfiles")
        self.cartesianpoints = {}
        self.directions = {}
        self.axis2placement3ds = {}
        if template: # we are using the default template from exportIFC.py
            self.cartesianpoints = {(0,0,0):self.ifcfile[8]} # from template
            self.directions = {(1,0,0):self.ifcfile[6],(0,0,1):self.ifcfile[7],(0,1,0):self.ifcfile[10]} # from template
            self.axis2placement3ds = {'(0.0, 0.0, 0.0)(0.0, 0.0, 1.0)(1.0, 0.0, 0.0)':self.ifcfile[9]} # from template
        self.polylines = {}
        self.polyloops = {}
        self.propertysinglevalues = {}
        self.axis2placement2ds = {}
        self.localplacements = {}
        self.rgbs = {}
        self.ssrenderings = {}
        self.sstyles = {}
        self.transformationoperators = {}
        self.psas = {}
        self.spared = 0
        self.profiledefs = {}

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
            c = self.ifcfile.createIfcPropertySingleValue(name,None,self.ifcfile.create_entity(ptype,pvalue),None)
            if self.compress:
                self.propertysinglevalues[key] = c
            return c

    def createIfcAxis2Placement3D(self,p1=None,p2=None,p3=None):
        if not p1:
            p1 = self.createIfcCartesianPoint((0.0,0.0,0.0))
            p2 = self.createIfcDirection((0.0,0.0,1.0))
            p3 = self.createIfcDirection((1.0,0.0,0.0))
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

    def createIfcLocalPlacement(self,gpl=None):
        if not gpl:
            gpl = self.createIfcAxis2Placement3D()
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

    def createIfcPresentationStyleAssignment(self,name,r,g,b,t=0,ifc4=False):
        if name:
            key = name+str((r,g,b,t))
        else:
            key = str((r,g,b,t))
        if self.compress and key in self.psas:
            self.spared += 1
            return self.psas[key]
        else:
            iss = self.createIfcSurfaceStyle(name,r,g,b,t)
            if ifc4:
                c = iss
            else:
                c = self.ifcfile.createIfcPresentationStyleAssignment([iss])
            if self.compress:
                self.psas[key] = c
            return c

    def createIfcRectangleProfileDef(self,name,mode,pt,b,h):
        key = "RECT"+str(name)+str(mode)+str(pt)+str(b)+str(h)
        if self.compress and self.mergeProfiles and key in self.profiledefs:
            return self.profiledefs[key]
        else:
            c = self.ifcfile.createIfcRectangleProfileDef(name,mode,pt,b,h)
            if self.compress and self.mergeProfiles:
                self.profiledefs[key] = c
            return c

    def createIfcCircleProfileDef(self,name,mode,pt,r):
        key = "CIRC"+str(name)+str(mode)+str(pt)+str(r)
        if self.compress and self.mergeProfiles and key in self.profiledefs:
            return self.profiledefs[key]
        else:
            c = self.ifcfile.createIfcCircleProfileDef(name,mode,pt,r)
            if self.compress and self.mergeProfiles:
                self.profiledefs[key] = c
            return c

    def createIfcEllipseProfileDef(self,name,mode,pt,majr,minr):
        key = "ELLI"+str(name)+str(mode)+str(pt)+str(majr)+str(minr)
        if self.compress and self.mergeProfiles and key in self.profiledefs:
            return self.profiledefs[key]
        else:
            c = self.ifcfile.createIfcEllipseProfileDef(name,mode,pt,majr,minr)
            if self.compress and self.mergeProfiles:
                self.profiledefs[key] = c
            return c
