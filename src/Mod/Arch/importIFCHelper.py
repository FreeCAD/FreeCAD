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
"""Helper functions that are used by IFC importer and exporter."""
import sys
import math

import FreeCAD
import Arch
import ArchIFC

if FreeCAD.GuiUp:
    import FreeCADGui as Gui

from draftutils.messages import _msg, _wrn


PREDEFINED_RGB = {"black": (0, 0, 0),
                  "red": (1.0, 0, 0),
                  "green": (0, 1.0, 0),
                  "blue": (0, 0, 1.0),
                  "yellow": (1.0, 1.0, 0),
                  "magenta": (1.0, 0, 1.0),
                  "cyan": (0, 1.0, 1.0),
                  "white": (1.0, 1.0, 1.0)}


DEBUG_prod_repr = False
DEBUG_prod_colors = False


def dd2dms(dd):
    """Convert decimal degrees to degrees, minutes, seconds.

    Used in export.
    """
    sign = 1 if dd >= 0 else -1
    dd = abs(dd)
    minutes, seconds = divmod(dd * 3600, 60)
    degrees, minutes = divmod(minutes, 60)

    if dd < 0:
        degrees = -degrees

    return (int(degrees) * sign,
            int(minutes) * sign,
            int(seconds) * sign)


def dms2dd(degrees, minutes, seconds, milliseconds=0):
    """Convert degrees, minutes, seconds to decimal degrees.

    Used in import.
    """
    dd = float(degrees) + float(minutes)/60 + float(seconds)/3600
    return dd


def getPreferences():
    """Retrieve the IFC preferences available in import and export.

    MERGE_MODE_ARCH:
        0 = parametric arch objects
        1 = non-parametric arch objects
        2 = Part shapes
        3 = One compound per storey
    """
    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")

    if FreeCAD.GuiUp and p.GetBool("ifcShowDialog", False):
        Gui.showPreferences("Import-Export", 0)

    preferences = {
        'DEBUG': p.GetBool("ifcDebug", False),
        'PREFIX_NUMBERS': p.GetBool("ifcPrefixNumbers", False),
        'SKIP': p.GetString("ifcSkip", "").split(","),
        'SEPARATE_OPENINGS': p.GetBool("ifcSeparateOpenings", False),
        'ROOT_ELEMENT': p.GetString("ifcRootElement", "IfcProduct"),
        'GET_EXTRUSIONS': p.GetBool("ifcGetExtrusions", False),
        'MERGE_MATERIALS': p.GetBool("ifcMergeMaterials", False),
        'MERGE_MODE_ARCH': p.GetInt("ifcImportModeArch", 0),
        'MERGE_MODE_STRUCT': p.GetInt("ifcImportModeStruct", 1),
        'CREATE_CLONES': p.GetBool("ifcCreateClones", True),
        'IMPORT_PROPERTIES': p.GetBool("ifcImportProperties", False),
        'SPLIT_LAYERS': p.GetBool("ifcSplitLayers", False),  # wall layer, not layer for visual props
        'FITVIEW_ONIMPORT': p.GetBool("ifcFitViewOnImport", False),
        'ALLOW_INVALID': p.GetBool("ifcAllowInvalid", False),
        'REPLACE_PROJECT': p.GetBool("ifcReplaceProject", False),
        'MULTICORE': p.GetInt("ifcMulticore", 0),
        'IMPORT_LAYER': p.GetBool("ifcImportLayer", True)
    }

    if preferences['MERGE_MODE_ARCH'] > 0:
        preferences['SEPARATE_OPENINGS'] = False
        preferences['GET_EXTRUSIONS'] = False
    if not preferences['SEPARATE_OPENINGS']:
        preferences['SKIP'].append("IfcOpeningElement")

    return preferences


class ProjectImporter:
    """A helper class to create an Arch Project object."""

    def __init__(self, file, objects):
        self.file = file
        self.objects = objects

    def execute(self):
        self.project = self.file.by_type("IfcProject")[0]
        self.object = Arch.makeProject()
        self.objects[self.project.id()] = self.object
        self.setAttributes()
        self.setComplexAttributes()

    def setAttributes(self):
        for prop in self.object.PropertiesList:
            if hasattr(self.project, prop) and getattr(self.project, prop):
                setattr(self.object, prop, getattr(self.project, prop))

    def setComplexAttributes(self):
        try:
            mapConversion = self.project.RepresentationContexts[0].HasCoordinateOperation[0]

            data = self.extractTargetCRSData(mapConversion.TargetCRS)
            data.update(self.extractMapConversionData(mapConversion))
            # TODO: review and refactor this piece of code.
            # Calling a method from a class is a bit strange;
            # this class should be derived from that class to inherit
            # this method; otherwise a simple function (not tied to a class)
            # should be used.
            ArchIFC.IfcRoot.setObjIfcComplexAttributeValue(self, self.object, "RepresentationContexts", data)
        except Exception:
            # This scenario occurs validly in IFC2X3,
            # as the mapConversion does not exist
            return

    def extractTargetCRSData(self, targetCRS):
        mappings = {
            "name": "Name",
            "description": "Description",
            "geodetic_datum": "GeodeticDatum",
            "vertical_datum": "VerticalDatum",
            "map_projection": "MapProjection",
            "map_zone": "MapZone"
        }
        data = {}
        for attributeName, ifcName in mappings.items():
            data[attributeName] = str(getattr(targetCRS, ifcName))

        if targetCRS.MapUnit.Prefix:
            data["map_unit"] = targetCRS.MapUnit.Prefix.title() + targetCRS.MapUnit.Name.lower()
        else:
            data["map_unit"] = targetCRS.MapUnit.Name.title()

        return data

    def extractMapConversionData(self, mapConversion):
        mappings = {
            "eastings": "Eastings",
            "northings": "Northings",
            "orthogonal_height": "OrthogonalHeight",
            "x_axis_abscissa": "XAxisAbscissa",
            "x_axis_ordinate": "XAxisOrdinate",
            "scale": "Scale"
        }
        data = {}
        for attributeName, ifcName in mappings.items():
            data[attributeName] = str(getattr(mapConversion, ifcName))

        data["true_north"] = str(self.calculateTrueNorthAngle(mapConversion.XAxisAbscissa,
                                                              mapConversion.XAxisOrdinate))
        return data

    def calculateTrueNorthAngle(self, x, y):
        return round(math.degrees(math.atan2(y, x)) - 90, 6)


def buildRelProductsAnnotations(ifcfile, root_element='IfcProduct'):
    """Build the products and annotations relation table."""
    products = ifcfile.by_type(root_element)

    annotations = ifcfile.by_type("IfcAnnotation")
    tp = []
    for product in products:
        if product.is_a("IfcGrid") and (product not in annotations):
            annotations.append(product)
        elif product not in annotations:
            tp.append(product)

    # remove any leftover annotations from products
    products = sorted(tp, key=lambda prod: prod.id())

    return products, annotations


def buildRelProductRepresentation(ifcfile):
    """Build the product/representations relation table."""
    if DEBUG_prod_repr:
        _msg(32 * "-")
        _msg("Product-representation table")

    prodrepr = dict()

    i = 1
    for p in ifcfile.by_type("IfcProduct"):
        if hasattr(p, "Representation") and p.Representation:
            if DEBUG_prod_repr:
                _msg("{}: {}, {}, '{}'".format(i, p.id(),
                                               p.is_a(), p.Name))

            for it in p.Representation.Representations:
                for it1 in it.Items:
                    prodrepr.setdefault(p.id(), []).append(it1.id())
                    if it1.is_a("IfcBooleanResult"):
                        prodrepr.setdefault(p.id(), []).append(it1.FirstOperand.id())
                    elif it.Items[0].is_a("IfcMappedItem"):
                        prodrepr.setdefault(p.id(), []).append(it1.MappingSource.MappedRepresentation.id())
                        if it1.MappingSource.MappedRepresentation.is_a("IfcShapeRepresentation"):
                            for it2 in it1.MappingSource.MappedRepresentation.Items:
                                prodrepr.setdefault(p.id(), []).append(it2.id())
            i += 1
    return prodrepr


def buildRelAdditions(ifcfile):
    """Build the additions relation table."""
    additions = {}  # { host:[child,...], ... }

    for r in ifcfile.by_type("IfcRelContainedInSpatialStructure"):
        additions.setdefault(r.RelatingStructure.id(), []).extend([e.id() for e in r.RelatedElements])
    for r in ifcfile.by_type("IfcRelAggregates"):
        additions.setdefault(r.RelatingObject.id(), []).extend([e.id() for e in r.RelatedObjects])

    return additions


def buildRelGroups(ifcfile):
    """Build the groups relation table."""
    groups = {}  # { host:[child,...], ... }     # used in structural IFC

    for r in ifcfile.by_type("IfcRelAssignsToGroup"):
        groups.setdefault(r.RelatingGroup.id(), []).extend([e.id() for e in r.RelatedObjects])

    return groups


def buildRelSubtractions(ifcfile):
    """Build the subtractions relation table."""
    subtractions = []  # [ [opening,host], ... ]

    for r in ifcfile.by_type("IfcRelVoidsElement"):
        subtractions.append([r.RelatedOpeningElement.id(), r.RelatingBuildingElement.id()])

    return subtractions


def buildRelMattable(ifcfile):
    """Build the mattable relation table."""
    mattable = {}  # { objid:matid }

    for r in ifcfile.by_type("IfcRelAssociatesMaterial"):
        # the related object might not exist
        # https://forum.freecad.org/viewtopic.php?f=39&t=58607
        if r.RelatedObjects:
            for o in r.RelatedObjects:
                if r.RelatingMaterial.is_a("IfcMaterial"):
                    mattable[o.id()] = r.RelatingMaterial.id()
                elif r.RelatingMaterial.is_a("IfcMaterialLayer"):
                    mattable[o.id()] = r.RelatingMaterial.Material.id()
                elif r.RelatingMaterial.is_a("IfcMaterialLayerSet"):
                    mattable[o.id()] = r.RelatingMaterial.MaterialLayers[0].Material.id()
                elif r.RelatingMaterial.is_a("IfcMaterialLayerSetUsage"):
                    mattable[o.id()] = r.RelatingMaterial.ForLayerSet.MaterialLayers[0].Material.id()

    return mattable


# Color relation tables.
# Products can have a color, materials can have a color,
# and products can have a material.
# Colors for material assigned to a product, and color of the product itself
# can be different
def buildRelColors(ifcfile, prodrepr):
    """Build the colors relation table.

    Returns all IfcStyledItem colors, material and product colors.

    Returns
    -------
    dict
        A dictionary with `{id: (r,g,b), ...}` values.
    """
    colors = {}  # { id:(r,g,b) }
    style_material_id = {}  # { style_entity_id: material_id) }

    style_color_rgb = {}  # { style_entity_id: (r,g,b) }
    for r in ifcfile.by_type("IfcStyledItem"):
        if r.Styles and r.Styles[0].is_a("IfcPresentationStyleAssignment"):
            for style1 in r.Styles[0].Styles:
                if style1.is_a("IfcSurfaceStyle"):
                    for style2 in style1.Styles:
                        if style2.is_a("IfcSurfaceStyleRendering"):
                            if style2.SurfaceColour:
                                c = style2.SurfaceColour
                                style_color_rgb[r.id()] = (c.Red,
                                                           c.Green,
                                                           c.Blue)

        # Nova
        # FIXME: style_entity_id = { style_entity_id: product_id } not material_id ???
        # see https://forum.freecad.org/viewtopic.php?f=39&t=37940&start=10#p329491
        # last code change in these color code https://github.com/FreeCAD/FreeCAD/commit/2d1f6ab1
        '''
        if r.Item:
            # print(r.id())
            # print(r.Item)  # IfcRepresentationItem or IfcShapeRepresentation
            for p in prodrepr.keys():
                if r.Item.id() in prodrepr[p]:
                    style_material_id[r.id()] = p
                    # print(p)
                    # print(ifcfile[p])  # product
        '''

    # A much faster version for Nova style_material_id with product_ids
    # no material colors, Nova ifc files often do not have materials at all
    for p in prodrepr.keys():
        # print("\n")
        # print(ifcfile[p])  # IfcProduct
        # print(ifcfile[p].Representation)  # IfcProductDefinitionShape
        # print(ifcfile[p].Representation.Representations[0])  # IfcShapeRepresentation
        # print(ifcfile[p].Representation.Representations[0].Items[0])  # IfcRepresentationItem
        # print(ifcfile[p].Representation.Representations[0].Items[0].StyledByItem[0])  # IfcStyledItem
        # print(ifcfile[p].Representation.Representations[0].Items[0].StyledByItem[0].id())
        # print(p)
        representation_item = ifcfile[p].Representation.Representations[0].Items[0]
        if hasattr(representation_item, "StyledByItem") and representation_item.StyledByItem:
            style_material_id[representation_item.StyledByItem[0].id()] = p

    # Allplan, ArchiCAD
    for m in ifcfile.by_type("IfcMaterialDefinitionRepresentation"):
        for it in m.Representations:
            if it.Items:
                style_material_id[it.Items[0].id()] = m.RepresentedMaterial.id()

    # create colors out of style_color_rgb and style_material_id
    for k in style_material_id:
        if k in style_color_rgb:
            colors[style_material_id[k]] = style_color_rgb[k]

    return colors


def buildRelProductColors(ifcfile, prodrepr):
    """Build the colors relation table from a product.

    Returns
    -------
    dict
        A dictionary with `{id: (r,g,b), ...}` values.
    """
    if DEBUG_prod_repr:
        _msg(32 * "-")
        _msg("Product-color table")

    colors = dict()
    i = 0

    for p in prodrepr.keys():
        # see method getColorFromProduct()
        # it is a method for the redundant code inside this loop
        # which can be used to get the color from a product directly

        # Representation item, see `IfcRepresentationItem` documentation.
        # All kinds of geometric or topological representation items
        # `IfcExtrudedAreaSolid`, `IfcMappedItem`, `IfcFacetedBrep`,
        # `IfcBooleanResult`, `IfcBooleanClippingResult`, etc.
        _body = ifcfile[p].Representation.Representations[0]
        repr_item = _body.Items[0]

        if DEBUG_prod_colors:
            _msg("{}: {}, {}, '{}', rep_item {}".format(i, ifcfile[p].id(),
                                                        ifcfile[p].is_a(),
                                                        ifcfile[p].Name,
                                                        repr_item))
        # Get the geometric representations which have a presentation style.
        # All representation items have the inverse attribute `StyledByItem`
        # for this.
        # There will be geometric representations which do not have
        # a presentation style so `StyledByItem` will be empty.
        if repr_item.StyledByItem:
            if DEBUG_prod_colors:
                _msg("  StyledByItem -> {}".format(repr_item.StyledByItem))
            # it has to be a `IfcStyledItem`, no check needed
            styled_item = repr_item.StyledByItem[0]

            # Write into colors table if a `IfcStyledItem` exists
            # for this product, write `None` if something goes wrong
            # or if the ifc file has errors and thus no valid color
            # is returned
            colors[p] = getColorFromStyledItem(styled_item)

        i += 1
    return colors


def buildRelMaterialColors(ifcfile, prodrepr):
    # not implemented
    pass


def getColorFromProduct(product):

    if product.Representation:
        for rep in product.Representation.Representations:
            for item in rep.Items:
                for style in item.StyledByItem:
                    color = getColorFromStyledItem(style)
                    if color:
                        return color


def getColorFromMaterial(material):

    if material.HasRepresentation:
        rep = material.HasRepresentation[0]
        if hasattr(rep,"Representations") and rep.Representations:
            rep = rep.Representations[0]
            if rep.is_a("IfcStyledRepresentation"):
                return getColorFromStyledItem(rep)
    return None


def color2colorRGB(color_data):

    if color_data is None:
        return None

    color_rgb = [
        int(round(color_data[0]*255, 0)),
        int(round(color_data[1]*255, 0)),
        int(round(color_data[2]*255, 0))
    ]  # int(159.99) would return 159 not 160, thus round

    return color_rgb


def getColorFromStyledItem(styled_item):
    """Get color from the IfcStyledItem.

    Returns
    -------
    float, float, float, int
        A tuple with the red, green, blue, and transparency values.
        If the `IfcStyledItem` is a `IfcDraughtingPreDefinedColour`
        the transparency is set to 0.
        The first three values range from 0 to 1.0, while the transparency
        varies from 0 to 100.

    None
        Return `None` if `styled_item` is not of type `'IfcStyledItem'`
        or if there is any other problem getting a color.
    """

    if styled_item.is_a("IfcStyledRepresentation"):
        styled_item = styled_item.Items[0]

    if not styled_item.is_a("IfcStyledItem"):
        return None

    rgb_color = None
    transparency = None
    col = None

    # The `IfcStyledItem` holds presentation style information for products,
    # either explicitly for an `IfcGeometricRepresentationItem` being part of
    # an `IfcShapeRepresentation` assigned to a product, or by assigning
    # presentation information to `IfcMaterial` being assigned
    # as other representation for a product.

    # In current IFC release (IFC2x3) only one presentation style
    # assignment shall be assigned.
    # In IFC4 `IfcPresentationStyleAssignment` is deprecated
    # In IFC4 multiple styles are assigned to style in 'IfcStyleItem' instead

    # print(ifcfile[p])
    # print(styled_item)
    # print(styled_item.Styles)
    if len(styled_item.Styles) == 0:
        # IN IFC2x3, only one element in `Styles` should be available.
        _wrn("No 'Style' in 'IfcStyleItem', do nothing.")
        # ca 100x in 210_King_Merged.ifc
        # Empty styles, #4952778=IfcStyledItem(#4952779,(),$)
        # this is an error in the IFC file in my opinion
    else:
        # never seen an ifc with more than one Styles in IfcStyledItem
        # the above seams to only apply for IFC2x3, IFC4 can have them
        # see https://forum.freecad.org/viewtopic.php?f=39&t=33560&p=437056#p437056

        # Get the `IfcPresentationStyleAssignment`, there should only be one,
        if styled_item.Styles[0].is_a('IfcPresentationStyleAssignment'):
            assign_style = styled_item.Styles[0]
        else:
            # `IfcPresentationStyleAssignment` is deprecated in IFC4,
            # in favor of `IfcStyleAssignmentSelect`
            assign_style = styled_item
        # print(assign_style)  # IfcPresentationStyleAssignment

        # `IfcPresentationStyleAssignment` can hold various kinds and counts
        # of styles, see `IfcPresentationStyleSelect`
        if assign_style.Styles[0].is_a("IfcSurfaceStyle"):
            _style = assign_style.Styles[0]
            # Schependomlaan and Nova and others
            # `IfcSurfaceStyleRendering`
            # print(_style.Styles[0])
            # `IfcColourRgb`
            rgb_color = _style.Styles[0].SurfaceColour
            # print(rgb_color)
            if (_style.Styles[0].is_a('IfcSurfaceStyleShading')
                    and hasattr(_style.Styles[0], 'Transparency')
                    and _style.Styles[0].Transparency):
                transparency = _style.Styles[0].Transparency * 100
        elif assign_style.Styles[0].is_a("IfcCurveStyle"):
            if (len(assign_style.Styles) == 2
                    and assign_style.Styles[1].is_a("IfcSurfaceStyle")):
                # Allplan, new IFC export started in 2017
                # `IfcDraughtingPreDefinedColour`
                # print(assign_style.Styles[0].CurveColour)
                # TODO: check this; on index 1, is this what we need?!
                rgb_color = assign_style.Styles[1].Styles[0].SurfaceColour
                # print(rgb_color)
            else:
                # 2x Annotations in 210_King_Merged.ifc
                # print(ifcfile[p])
                # print(assign_style.Styles[0])
                # print(assign_style.Styles[0].CurveColour)
                rgb_color = assign_style.Styles[0].CurveColour

    if rgb_color:
        if rgb_color.is_a('IfcDraughtingPreDefinedColour'):
            if DEBUG_prod_colors:
                _msg("  '{}'= ".format(rgb_color.Name))

            col = predefined_to_rgb(rgb_color)

            if col:
                col = col + (0, )
        else:
            col = (rgb_color.Red,
                   rgb_color.Green,
                   rgb_color.Blue,
                   int(transparency) if transparency else 0)
    else:
        col = None

    if DEBUG_prod_colors:
        _msg("  {}".format(col))

    return col


def predefined_to_rgb(rgb_color):
    """Transform a predefined color name to its [r, g, b] representation.

    TODO: at the moment it doesn't handle 'by layer'.
    See: `IfcDraughtingPreDefinedColour` and `IfcPresentationLayerWithStyle`.
    """
    name = rgb_color.Name.lower()
    if name not in PREDEFINED_RGB:
        _wrn("Color name not in 'IfcDraughtingPreDefinedColour'.")

        if name == 'by layer':
            _wrn("'IfcDraughtingPreDefinedColour' set 'by layer'; "
                 "currently not handled, set to 'None'.")
        return None

    return PREDEFINED_RGB[name]

# ************************************************************************************************
# property related methods


def buildRelProperties(ifcfile):
    """
    Builds and returns a dictionary of {object:[properties]} from an IFC file
    """

    # this method no longer used by the importer module
    # but this relation table might be useful anyway for other purposes

    properties = {}  # { objid : { psetid : [propertyid, ... ], ... }, ... }
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
    return properties


def getIfcPropertySets(ifcfile, pid):
    """Returns a dictionary of {pset_id:[prop_id, prop_id...]} for an IFC object"""

    # get psets for this pid
    psets = {}
    for rel in ifcfile[pid].IsDefinedBy:
        # the following if condition is needed in IFC2x3 only
        # https://forum.freecad.org/viewtopic.php?f=39&t=37892#p322884
        if rel.is_a('IfcRelDefinesByProperties'):
            props = []
            if rel.RelatingPropertyDefinition.is_a("IfcPropertySet"):
                props.extend([prop.id() for prop in rel.RelatingPropertyDefinition.HasProperties])
                psets[rel.RelatingPropertyDefinition.id()] = props
    return psets


def getIfcProperties(ifcfile, pid, psets, d):
    """builds valid property values for FreeCAD"""

    for pset in psets.keys():
        # print("reading pset: ",pset)
        psetname = ifcfile[pset].Name
        for prop in psets[pset]:
            e = ifcfile[prop]
            pname = e.Name
            if e.is_a("IfcPropertySingleValue"):
                if e.NominalValue:
                    ptype = e.NominalValue.is_a()
                    if ptype in ['IfcLabel','IfcText','IfcIdentifier','IfcDescriptiveMeasure']:
                        pvalue = e.NominalValue.wrappedValue
                    else:
                        pvalue = str(e.NominalValue.wrappedValue)
                    if hasattr(e.NominalValue,'Unit'):
                        if e.NominalValue.Unit:
                            pvalue += e.NominalValue.Unit
                    d[pname+";;"+psetname] = ptype+";;"+pvalue
                # print("adding property: ",pname,ptype,pvalue," pset ",psetname)
    return d


def getIfcPsetProperties(ifcfile, pid):
    """ directly build the property table from pid and ifcfile for FreeCAD"""

    return getIfcProperties(ifcfile, pid, getIfcPropertySets(ifcfile, pid), {})


def getUnit(unit):
    """Get the unit multiplier for different decimal prefixes.

    Only for when the unit is METRE.
    When no Prefix is provided, return 1000, that is, mm x 1000 = metre.
    For other cases, return 1.0.
    """
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


def getScaling(ifcfile):
    """Return a scaling factor from the IFC file; units to mm."""
    ua = ifcfile.by_type("IfcUnitAssignment")

    if not ua:
        return 1.0

    ua = ua[0]
    for u in ua.Units:
        if u.UnitType == "LENGTHUNIT":
            if u.is_a("IfcConversionBasedUnit"):
                f = getUnit(u.ConversionFactor.UnitComponent)
                return f * u.ConversionFactor.ValueComponent.wrappedValue
            elif u.is_a("IfcSIUnit") or u.is_a("IfcUnit"):
                return getUnit(u)
    return 1.0


def getRotation(entity):
    """returns a FreeCAD rotation from an IfcProduct with a IfcMappedItem representation"""
    try:
        u = FreeCAD.Vector(entity.Axis1.DirectionRatios)
        v = FreeCAD.Vector(entity.Axis2.DirectionRatios)
        w = FreeCAD.Vector(entity.Axis3.DirectionRatios)
    except AttributeError:
        return FreeCAD.Rotation()
    import WorkingPlane
    p = WorkingPlane.plane(u=u, v=v, w=w)
    return p.getRotation().Rotation


def getPlacement(entity,scaling=1000):
    """returns a placement from the given entity"""

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
    elif entity.is_a("IfcAxis2Placement2D"):
        _wrn("not implemented IfcAxis2Placement2D, ", end="")
    elif entity.is_a("IfcLocalPlacement"):
        pl = getPlacement(entity.PlacementRelTo,1)  # original placement
        relpl = getPlacement(entity.RelativePlacement,1)  # relative transf
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
    """returns a vector from the given entity"""

    if not entity:
        return None
    v = None
    if entity.is_a("IfcDirection"):
        if len(entity.DirectionRatios) == 3:
            v = FreeCAD.Vector(tuple(entity.DirectionRatios))
        else:
            v = FreeCAD.Vector(tuple(entity.DirectionRatios+[0]))
    elif entity.is_a("IfcCartesianPoint"):
        if len(entity.Coordinates) == 3:
            v = FreeCAD.Vector(tuple(entity.Coordinates))
        else:
            v = FreeCAD.Vector(tuple(entity.Coordinates+[0]))
    # if v:
    #     v.multiply(scaling)
    return v


def get2DShape(representation,scaling=1000):
    """Returns a shape from a 2D IfcShapeRepresentation"""

    import Part
    import DraftVecUtils
    import Draft

    def getPolyline(ent):
        pts = []
        for p in ent.Points:
            c = p.Coordinates
            c = FreeCAD.Vector(c[0],c[1],c[2] if len(c) > 2 else 0)
            c.multiply(scaling)
            pts.append(c)
        return Part.makePolygon(pts)

    def getRectangle(ent):
        return Part.makePlane(ent.XDim,ent.YDim)

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
        elif ent.is_a() in ["IfcLine","IfcPolyline","IfcCircle","IfcTrimmedCurve","IfcRectangleProfileDef"]:
            elts = [ent]
        else:
            print("getCurveSet: unhandled entity: ", ent)
            return []

        for el in elts:
            if el.is_a("IfcPolyline"):
                result.append(getPolyline(el))
            elif el.is_a("IfcRectangleProfileDef"):
                result.append(getRectangle(el))
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
            elif el.is_a("IfcIndexedPolyCurve"):
                coords = el.Points.CoordList

                def index2points(segment):
                    pts = []
                    for i in segment.wrappedValue:
                        c = coords[i-1]
                        c = FreeCAD.Vector(c[0],c[1],c[2] if len(c) > 2 else 0)
                        c.multiply(scaling)
                        pts.append(c)
                    return pts

                for s in el.Segments:
                    if s.is_a("IfcLineIndex"):
                        result.append(Part.makePolygon(index2points(s)))
                    elif s.is_a("IfcArcIndex"):
                        [p1, p2, p3] = index2points(s)
                        result.append(Part.Arc(p1, p2, p3))
                    else:
                        raise RuntimeError("Illegal IfcIndexedPolyCurve segment")
            else:
                print("getCurveSet: unhandled element: ", el)

        return result

    result = []
    if representation.is_a("IfcShapeRepresentation"):
        for item in representation.Items:
            if item.is_a() in ["IfcGeometricCurveSet","IfcGeometricSet"]:
                result = getCurveSet(item)
            elif item.is_a("IfcMappedItem"):
                preresult = get2DShape(item.MappingSource.MappedRepresentation,scaling)
                pla = getPlacement(item.MappingSource.MappingOrigin,scaling)
                rot = getRotation(item.MappingTarget)
                if pla:
                    if rot.Angle:
                        pla.Rotation = rot
                    for r in preresult:
                        # r.Placement = pla
                        result.append(r)
                else:
                    result = preresult
            elif item.is_a("IfcTextLiteral"):
                pl = getPlacement(item.Placement, scaling)
                if pl:
                    t = Draft.make_text(item.Literal.split(";"), pl)
                    if FreeCAD.GuiUp:
                        if item.Path == "RIGHT":
                            t.ViewObject.Justification = "Right"
                    # do not return because there might be more than one representation
                    #return []  # TODO dirty hack... Object creation should not be done here
    elif representation.is_a() in ["IfcPolyline","IfcCircle","IfcTrimmedCurve","IfcRectangleProfileDef"]:
        result = getCurveSet(representation)
    return result


def getProfileCenterPoint(sweptsolid):
    """returns the center point of the profile of an extrusion"""
    v = FreeCAD.Vector(0,0,0)
    if hasattr(sweptsolid,"SweptArea"):
        profile = get2DShape(sweptsolid.SweptArea)
        if profile:
            profile = profile[0]
            if hasattr(profile,"CenterOfMass"):
                v = profile.CenterOfMass
            elif hasattr(profile,"BoundBox"):
                v = profile.BoundBox.Center
    if hasattr(sweptsolid,"Position"):
        pos = getPlacement(sweptsolid.Position)
        v = pos.multVec(v)
    return v


def isRectangle(verts):
    """returns True if the given 4 vertices form a rectangle"""
    if len(verts) != 4:
        return False
    v1 = verts[1].sub(verts[0])
    v2 = verts[2].sub(verts[1])
    v3 = verts[3].sub(verts[2])
    v4 = verts[0].sub(verts[3])
    if abs(v2.getAngle(v1)-math.pi/2) > 0.01:
        return False
    if abs(v3.getAngle(v2)-math.pi/2) > 0.01:
        return False
    if abs(v4.getAngle(v3)-math.pi/2) > 0.01:
        return False
    return True


def createFromProperties(propsets,ifcfile,parametrics):
    """
    Creates a FreeCAD parametric object from a set of properties.
    """

    obj = None
    sets = []
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
        if "FreeCADType" in appset:
            if "FreeCADName" in appset:
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
    return obj,parametrics


def applyColorDict(doc,colordict=None):
    """applies the contents of a color dict to the objects in the given doc.
    If no colordict is given, the doc Meta property is searched for a "colordict" entry."""

    if not colordict:
        if "colordict" in doc.Meta:
            import json
            colordict = json.loads(doc.Meta["colordict"])
    if colordict:
        for obj in doc.Objects:
            if obj.Name in colordict:
                color = colordict[obj.Name]
                if hasattr(obj.ViewObject,"ShapeColor"):
                    obj.ViewObject.ShapeColor = tuple(color[0:3])
                if hasattr(obj.ViewObject,"Transparency") and (len(color) >= 4):
                    obj.ViewObject.Transparency = color[3]
    else:
        print("No valid color dict to apply")


def getParents(ifcobj):
    """finds the parent entities of an IFC entity"""

    parentlist = []
    if hasattr(ifcobj,"ContainedInStructure"):
        for rel in ifcobj.ContainedInStructure:
            parentlist.append(rel.RelatingStructure)
    elif hasattr(ifcobj,"Decomposes"):
        for rel in ifcobj.Decomposes:
            if rel.is_a("IfcRelAggregates"):
                parentlist.append(rel.RelatingObject)
    return parentlist


def createAnnotation(annotation,doc,ifcscale,preferences):
    """creates an annotation object"""

    anno = None
    aid =  annotation.id()
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
                sh = get2DShape(axis.AxisCurve,ifcscale)
                if sh and (len(sh[0].Vertexes) == 2):  # currently only straight axes are supported
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
            grid_placement = None
            if annotation.Name:
                name = annotation.Name
            if annotation.ObjectPlacement:
                # https://forum.freecad.org/viewtopic.php?f=39&t=40027
                grid_placement = getPlacement(annotation.ObjectPlacement,scaling=1)
            if preferences['PREFIX_NUMBERS']:
                name = "ID" + str(aid) + " " + name
            anno = Arch.makeAxisSystem(axes,name)
            if grid_placement:
                anno.Placement = grid_placement
        print(" axis")
    else:
        name = "Annotation"
        if annotation.Name:
            name = annotation.Name
        if "annotation" not in name.lower():
            name = "Annotation " + name
        if preferences['PREFIX_NUMBERS']: name = "ID" + str(aid) + " " + name
        shapes2d = []
        for rep in annotation.Representation.Representations:
            if rep.RepresentationIdentifier in ["Annotation","FootPrint","Axis"]:
                sh = get2DShape(rep,ifcscale)
                if sh in doc.Objects:
                    # dirty hack: get2DShape might return an object directly if non-shape based (texts for ex)
                    anno = sh
                else:
                    shapes2d.extend(sh)
        if shapes2d:
            import Part
            sh = Part.makeCompound(shapes2d)
            #if preferences['DEBUG']: print(" shape")
            anno = doc.addObject("Part::Feature",name)
            anno.Shape = sh
            p = getPlacement(annotation.ObjectPlacement,ifcscale)
            if p:  # and annotation.is_a("IfcAnnotation"):
                anno.Placement = p
        #else:
            #if preferences['DEBUG']: print(" no shape")

    return anno
