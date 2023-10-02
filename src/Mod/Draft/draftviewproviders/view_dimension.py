# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de> *
# *   Copyright (c) 2022 FreeCAD Project Association                        *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
"""Provides the viewprovider code for the Dimension objects.

These include linear dimensions, including radius and diameter,
as well as angular dimensions.
They inherit their behavior from the base Annotation viewprovider.
"""
## @package view_dimension
# \ingroup draftviewproviders
# \brief Provides the viewprovider code for the Dimension objects.

import math
import pivy.coin as coin
import lazy_loader.lazy_loader as lz
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import DraftVecUtils
import draftutils.units as units
import draftutils.utils as utils
import draftutils.gui_utils as gui_utils

from draftviewproviders.view_draft_annotation \
    import ViewProviderDraftAnnotation

# Delay import of module until first use because it is heavy
Part = lz.LazyLoader("Part", globals(), "Part")
DraftGeomUtils = lz.LazyLoader("DraftGeomUtils", globals(), "DraftGeomUtils")

## \addtogroup draftviewproviders
# @{


class ViewProviderDimensionBase(ViewProviderDraftAnnotation):
    """The base viewprovider for the Draft Dimensions object.

    This class is not used directly, but inherited by dimension
    viewproviders like linear, radial, and angular.

    Dimension nomeclature
    ---------------------
    The dimension object depends on various variables to draw the lines
    that are drawn on 3D view.
    ::
            |              txt               |      e
        ----b---------a----------------------b----
            |                                |
            |                                |      d
            |                                |

        c  (t)                              (t)   c

    From the object class

    * `a`, `Dimline`, point through which the dimension line goes through

    From the viewprovider class

    * `b`, `ArrowType` and `ArrowSize`, the symbol shown on the endpoints
    * `c`, `DimOvershoot`, extension to the dimension line going through `a`
    * `d`, `ExtLines`, distance to target `(t)`
    * `e`, `ExtOvershoot`, extension in the opposite direction to `(t)`
    * `txt`, text label showing the value of the measurement

    Coin object structure
    ---------------------
    The scenegraph is set from two main nodes.
    ::
        vobj.node_wld.linecolor
                     .drawstyle
                     .lineswitch_wld.coords
                                    .line
                                    .marks
                                    .marksDimOvershoot
                                    .marksExtOvershoot
                     .label_wld.textpos
                               .textcolor
                               .font
                               .text_wld

        vobj.node_scr.linecolor
                     .drawstyle
                     .lineswitch_scr.coords
                                    .line
                                    .marks
                                    .marksDimOvershoot
                                    .marksExtOvershoot
                     .label_scr.textpos
                               .textcolor
                               .font
                               .text_scr
    """

    def set_text_properties(self, vobj, properties):
        """Set text properties only if they don't already exist."""
        super().set_text_properties(vobj, properties)

        if "TextSpacing" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "Spacing between text and dimension line")
            vobj.addProperty("App::PropertyLength",
                             "TextSpacing",
                             "Text",
                             _tip)
            vobj.TextSpacing = utils.get_param("dimspacing", 1)

        if "FlipText" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "Rotate the dimension text 180 degrees")
            vobj.addProperty("App::PropertyBool",
                             "FlipText",
                             "Text",
                             _tip)
            vobj.FlipText = False

        if "TextPosition" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "Text Position.\n"
                                     "Leave '(0,0,0)' for automatic position")
            vobj.addProperty("App::PropertyVectorDistance",
                             "TextPosition",
                             "Text",
                             _tip)
            vobj.TextPosition = App.Vector(0, 0, 0)

        if "Override" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "Text override.\n"
                                     "Write '$dim' so that it is replaced by "
                                     "the dimension length.")
            vobj.addProperty("App::PropertyString",
                             "Override",
                             "Text",
                             _tip)
            vobj.Override = ''

    def set_units_properties(self, vobj, properties):
        """Set unit properties only if they don't already exist."""
        super().set_units_properties(vobj, properties)

        if "Decimals" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "The number of decimals to show")
            vobj.addProperty("App::PropertyInteger",
                             "Decimals",
                             "Units",
                             _tip)
            vobj.Decimals = utils.get_param("dimPrecision", 2)

        if "ShowUnit" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "Show the unit suffix")
            vobj.addProperty("App::PropertyBool",
                             "ShowUnit",
                             "Units",
                             _tip)
            vobj.ShowUnit = utils.get_param("showUnit", True)

        if "UnitOverride" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "A unit to express the measurement.\n"
                                     "Leave blank for system default.\n"
                                     "Use 'arch' to force US arch notation")
            vobj.addProperty("App::PropertyString",
                             "UnitOverride",
                             "Units",
                             _tip)
            vobj.UnitOverride = utils.get_param("overrideUnit", '')

    def set_graphics_properties(self, vobj, properties):
        """Set graphics properties only if they don't already exist."""
        super().set_graphics_properties(vobj, properties)

        if "ArrowSize" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "Arrow size")
            vobj.addProperty("App::PropertyLength",
                             "ArrowSize",
                             "Graphics",
                             _tip)
            vobj.ArrowSize = utils.get_param("arrowsize", 1)

        if "ArrowType" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "Arrow type")
            vobj.addProperty("App::PropertyEnumeration",
                             "ArrowType",
                             "Graphics",
                             _tip)
            vobj.ArrowType = utils.ARROW_TYPES
            vobj.ArrowType = utils.ARROW_TYPES[utils.get_param("dimsymbol", 0)]

        if "FlipArrows" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "Rotate the dimension arrows 180 degrees")
            vobj.addProperty("App::PropertyBool",
                             "FlipArrows",
                             "Graphics",
                             _tip)
            vobj.FlipArrows = False

        if "DimOvershoot" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "The distance the dimension line "
                                     "is extended\n"
                                     "past the extension lines")
            vobj.addProperty("App::PropertyDistance",
                             "DimOvershoot",
                             "Graphics",
                             _tip)
            vobj.DimOvershoot = utils.get_param("dimovershoot", 0)

        if "ExtLines" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "Length of the extension lines")
            vobj.addProperty("App::PropertyDistance",
                             "ExtLines",
                             "Graphics",
                             _tip)
            vobj.ExtLines = utils.get_param("extlines", 0.3)

        if "ExtOvershoot" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "Length of the extension line\n"
                                     "beyond the dimension line")
            vobj.addProperty("App::PropertyDistance",
                             "ExtOvershoot",
                             "Graphics",
                             _tip)
            vobj.ExtOvershoot = utils.get_param("extovershoot", 0)

        if "ShowLine" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "Shows the dimension line and arrows")
            vobj.addProperty("App::PropertyBool",
                             "ShowLine",
                             "Graphics",
                             _tip)
            vobj.ShowLine = True

    def getDefaultDisplayMode(self):
        """Return the default display mode."""
        return ["World", "Screen"][utils.get_param("dimstyle", 0)]

    def getIcon(self):
        """Return the path to the icon used by the viewprovider."""
        return ":/icons/Draft_Dimension_Tree.svg"


class ViewProviderLinearDimension(ViewProviderDimensionBase):
    """The viewprovider for the Linear Dimension objects.

    This includes straight edge measurement, as well as measurement
    of circular edges, and circumferences.
    """

    def attach(self, vobj):
        """Set up the scene sub-graph of the viewprovider."""
        self.Object = vobj.Object

        self.textpos = coin.SoTransform()
        self.textcolor = coin.SoBaseColor()
        self.font = coin.SoFont()
        self.text_wld = coin.SoAsciiText() # World orientation. Can be oriented in 3D space.
        self.text_scr = coin.SoText2()     # Screen orientation. Always faces the camera.

        # The text string needs to be initialized to something,
        # otherwise it may cause a crash of the system
        self.text_wld.string = "d"
        self.text_scr.string = "d"
        self.text_wld.justification = coin.SoAsciiText.CENTER
        self.text_scr.justification = coin.SoAsciiText.CENTER

        label_wld = coin.SoSeparator()
        label_wld.addChild(self.textpos)
        label_wld.addChild(self.textcolor)
        label_wld.addChild(self.font)
        label_wld.addChild(self.text_wld)

        label_scr = coin.SoSeparator()
        label_scr.addChild(self.textpos)
        label_scr.addChild(self.textcolor)
        label_scr.addChild(self.font)
        label_scr.addChild(self.text_scr)

        self.coord1 = coin.SoCoordinate3()
        self.trans1 = coin.SoTransform()
        self.coord2 = coin.SoCoordinate3()
        self.trans2 = coin.SoTransform()
        self.transDimOvershoot1 = coin.SoTransform()
        self.transDimOvershoot2 = coin.SoTransform()
        self.transExtOvershoot1 = coin.SoTransform()
        self.transExtOvershoot2 = coin.SoTransform()

        self.linecolor = coin.SoBaseColor()
        self.drawstyle = coin.SoDrawStyle()
        self.coords = coin.SoCoordinate3()
        import PartGui # Required for "SoBrepEdgeSet" (because a dimension is not a Part::FeaturePython object).
        self.line = coin.SoType.fromName("SoBrepEdgeSet").createInstance()
        self.marks = coin.SoSeparator()
        self.marksDimOvershoot = coin.SoSeparator()
        self.marksExtOvershoot = coin.SoSeparator()

        self.node_wld = coin.SoGroup()
        self.node_wld.addChild(self.linecolor)
        self.node_wld.addChild(self.drawstyle)
        self.lineswitch_wld = coin.SoSwitch()
        self.lineswitch_wld.whichChild = -3
        self.node_wld.addChild(self.lineswitch_wld)
        self.lineswitch_wld.addChild(self.coords)
        self.lineswitch_wld.addChild(self.line)
        self.lineswitch_wld.addChild(self.marks)
        self.lineswitch_wld.addChild(self.marksDimOvershoot)
        self.lineswitch_wld.addChild(self.marksExtOvershoot)
        self.node_wld.addChild(label_wld)

        self.node_scr = coin.SoGroup()
        self.node_scr.addChild(self.linecolor)
        self.node_scr.addChild(self.drawstyle)
        self.lineswitch_scr = coin.SoSwitch()
        self.lineswitch_scr.whichChild = -3
        self.node_scr.addChild(self.lineswitch_scr)
        self.lineswitch_scr.addChild(self.coords)
        self.lineswitch_scr.addChild(self.line)
        self.lineswitch_scr.addChild(self.marks)
        self.lineswitch_scr.addChild(self.marksDimOvershoot)
        self.lineswitch_scr.addChild(self.marksExtOvershoot)
        self.node_scr.addChild(label_scr)

        vobj.addDisplayMode(self.node_wld, "World")
        vobj.addDisplayMode(self.node_scr, "Screen")
        self.updateData(vobj.Object, "Start")
        self.onChanged(vobj, "FontSize")
        self.onChanged(vobj, "FontName")
        self.onChanged(vobj, "TextColor")
        self.onChanged(vobj, "ArrowType")
        self.onChanged(vobj, "LineColor")
        self.onChanged(vobj, "DimOvershoot")
        self.onChanged(vobj, "ExtOvershoot")

    def updateData(self, obj, prop):
        """Execute when a property from the Proxy class is changed.

        It only runs if `Start`, `End`, `Dimline`, or `Direction` changed.
        """
        if prop not in ("Start", "End", "Dimline", "Direction", "Diameter"):
            return

        if obj.Start == obj.End:
            return

        if not hasattr(self, "node_wld"):
            return

        vobj = obj.ViewObject

        if prop == "Diameter":
            if hasattr(vobj, "Override") and vobj.Override:
                if obj.Diameter:
                    vobj.Override = vobj.Override.replace("R $dim", "Ø $dim")
                else:
                    vobj.Override = vobj.Override.replace("Ø $dim", "R $dim")

            self.onChanged(vobj, "ArrowType")
            return

        # Calculate the 4 points
        #
        #       |        d          |
        #   ---p2-------------c----p3----    c
        #       |                   |
        #       |                   |
        #      p1                  p4
        #
        # - `c` is the `Dimline`, a point that lies on the dimension line
        #   or on its extension.
        # - The line itself between `p2` to `p3` is the `base`.
        # - The distance between `p2` (`base`) to `p1` is `proj`, an extension
        #   line from the dimension to the measured object.
        # - If the `proj` distance is zero, `p1` and `p2` are the same point,
        #   and same with `p3` and `p4`.
        #
        self.p1 = obj.Start
        self.p4 = obj.End
        base = None

        if (hasattr(obj, "Direction")
                and not DraftVecUtils.isNull(obj.Direction)):
            v2 = self.p1 - obj.Dimline
            v3 = self.p4 - obj.Dimline
            v2 = DraftVecUtils.project(v2, obj.Direction)
            v3 = DraftVecUtils.project(v3, obj.Direction)
            self.p2 = obj.Dimline + v2
            self.p3 = obj.Dimline + v3
            if DraftVecUtils.equals(self.p2, self.p3):
                base = None
                proj = None
            else:
                base = Part.LineSegment(self.p2, self.p3).toShape()
                proj = DraftGeomUtils.findDistance(self.p1, base)
                if proj:
                    proj = proj.negative()

        if not base:
            if DraftVecUtils.equals(self.p1, self.p4):
                base = None
                proj = None
            else:
                base = Part.LineSegment(self.p1, self.p4).toShape()
                proj = DraftGeomUtils.findDistance(obj.Dimline, base)

            if proj:
                self.p2 = self.p1 + proj.negative()
                self.p3 = self.p4 + proj.negative()
            else:
                self.p2 = self.p1
                self.p3 = self.p4

        if proj:
            if hasattr(vobj, "ExtLines") and hasattr(vobj, "ScaleMultiplier"):
                # The scale multiplier also affects the value
                # of the extension line; this makes sure a maximum length
                # is used if the calculated value is larger than it.
                dmax = vobj.ExtLines.Value * vobj.ScaleMultiplier
                if dmax and proj.Length > dmax:
                    if dmax > 0:
                        self.p1 = self.p2 + DraftVecUtils.scaleTo(proj, dmax)
                        self.p4 = self.p3 + DraftVecUtils.scaleTo(proj, dmax)
                    else:
                        rest = proj.Length + dmax
                        self.p1 = self.p2 + DraftVecUtils.scaleTo(proj, rest)
                        self.p4 = self.p3 + DraftVecUtils.scaleTo(proj, rest)
        else:
            proj = (self.p3 - self.p2).cross(App.Vector(0, 0, 1))

        # Calculate the arrow positions
        p2 = (self.p2.x, self.p2.y, self.p2.z)
        p3 = (self.p3.x, self.p3.y, self.p3.z)

        self.trans1.translation.setValue(p2)
        self.coord1.point.setValue(p2)
        self.trans2.translation.setValue(p3)
        self.coord2.point.setValue(p3)

        # Calculate dimension and extension lines overshoots positions
        self.transDimOvershoot1.translation.setValue(p2)
        self.transDimOvershoot2.translation.setValue(p3)
        self.transExtOvershoot1.translation.setValue(p2)
        self.transExtOvershoot2.translation.setValue(p3)

        # Determine the orientation of the text by using a normal direction.
        # By default the value of +Z will be used, or a calculated value
        # from p2 and p3. So the text will lie on the XY plane
        # or a plane coplanar with p2 and p3.
        u = self.p3 - self.p2
        u.normalize()

        if proj:
            _norm = u.cross(proj)
            norm = _norm.negative()
        else:
            norm = App.Vector(0, 0, 1)

        # If `Normal` exists and is different from the default `(0,0,0)`,
        # it will be used.
        if hasattr(obj, "Normal") and not DraftVecUtils.isNull(obj.Normal):
            norm = App.Vector(obj.Normal)

        if not DraftVecUtils.isNull(norm):
            norm.normalize()

        # Calculate the position of the arrows and extension lines
        v1 = norm.cross(u)
        _plane_rot = DraftVecUtils.getPlaneRotation(u, v1, norm)
        if _plane_rot is not None:
            rot1 = App.Placement(_plane_rot).Rotation.Q
            self.transDimOvershoot1.rotation.setValue((rot1[0], rot1[1],
                                                       rot1[2], rot1[3]))
            self.transDimOvershoot2.rotation.setValue((rot1[0], rot1[1],
                                                       rot1[2], rot1[3]))
            self.trot = rot1
        else:
            self.trot = (0, 0, 0, 1)

        if hasattr(vobj, "FlipArrows") and vobj.FlipArrows:
            u = u.negative()

        v2 = norm.cross(u)
        _plane_rot = DraftVecUtils.getPlaneRotation(u, v2)
        if _plane_rot is not None:
            rot2 = App.Placement(_plane_rot).Rotation.Q
            self.trans1.rotation.setValue((rot2[0], rot2[1],
                                           rot2[2], rot2[3]))
            self.trans2.rotation.setValue((rot2[0], rot2[1],
                                           rot2[2], rot2[3]))

        if self.p1 != self.p2:
            u3 = self.p1 - self.p2
            u3.normalize()
            v3 = norm.cross(u3)
            _plane_rot = DraftVecUtils.getPlaneRotation(u3, v3)
            if _plane_rot is not None:
                rot3 = App.Placement(_plane_rot).Rotation.Q
                self.transExtOvershoot1.rotation.setValue((rot3[0], rot3[1],
                                                           rot3[2], rot3[3]))
                self.transExtOvershoot2.rotation.setValue((rot3[0], rot3[1],
                                                           rot3[2], rot3[3]))

        # Offset is the distance from the dimension line to the textual
        # element that displays the value of the measurement
        if hasattr(vobj, "TextSpacing") and hasattr(vobj, "ScaleMultiplier"):
            ts = vobj.TextSpacing.Value * vobj.ScaleMultiplier
            offset = DraftVecUtils.scaleTo(v1, ts)
        else:
            offset = DraftVecUtils.scaleTo(v1, 0.05)

        if hasattr(vobj, "FlipText") and vobj.FlipText:
            _rott = App.Rotation(self.trot[0], self.trot[1], self.trot[2], self.trot[3])
            self.trot = _rott.multiply(App.Rotation(App.Vector(0, 0, 1), 180)).Q
            offset = offset.negative()

        # On first run the `DisplayMode` enumeration is not set, so we trap
        # the exception and set the display mode using the value
        # in the parameter database
        try:
            m = vobj.DisplayMode
        except AssertionError:
            m = ["World", "Screen"][utils.get_param("dimstyle", 0)]

        if m == "Screen":
            offset = offset.negative()

        # The position of the text element in the dimension is provided
        # in absolute coordinates by the value of `TextPosition`,
        # if it is different from the default `(0,0,0)`
        if (hasattr(vobj, "TextPosition")
                and not DraftVecUtils.isNull(vobj.TextPosition)):
            self.tbase = vobj.TextPosition
        else:
            # Otherwise the position is calculated from the end points
            # of the dimension line, and the offset that depends
            # on `TextSpacing`
            center = self.p2 + (self.p3 - self.p2).multiply(0.5)
            self.tbase = center + offset

        self.textpos.translation.setValue([self.tbase.x,
                                           self.tbase.y,
                                           self.tbase.z])
        self.textpos.rotation = coin.SbRotation(self.trot[0], self.trot[1],
                                                self.trot[2], self.trot[3])

        show_unit = True
        if hasattr(vobj, "ShowUnit"):
            show_unit = vobj.ShowUnit

        # Set text element showing the value of the dimension
        length = (self.p3 - self.p2).Length
        unit = None

        if hasattr(vobj, "UnitOverride"):
            unit = vobj.UnitOverride

        # Special representation if we use 'Building US' scheme
        u_params = App.ParamGet("User parameter:BaseApp/Preferences/Units")
        if u_params.GetInt("UserSchema", 0) == 5:
            self.string = App.Units.Quantity(length, App.Units.Length).UserString
            if self.string.count('"') > 1:
                # multiple inch tokens
                self.string = self.string.replace('"',"",self.string.count('"')-1)
            d_params = App.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
            sep = d_params.GetString("FeetSeparator"," ")
            # use a custom separator
            self.string = self.string.replace("' ", "'" + sep)
            self.string = self.string.replace("+", " ")
            self.string = self.string.replace("   ", " ")
            self.string = self.string.replace("  ", " ")
        elif hasattr(vobj, "Decimals"):
            self.string = units.display_external(length,
                                                 vobj.Decimals,
                                                 'Length', show_unit, unit)
        else:
            self.string = units.display_external(length,
                                                 None,
                                                 'Length', show_unit, unit)

        if hasattr(vobj, "Override") and vobj.Override:
            self.string = vobj.Override.replace("$dim", self.string)

        self.text_wld.string = utils.string_encode_coin(self.string)
        self.text_scr.string = utils.string_encode_coin(self.string)

        # Set the lines
        if m == "Screen":
            # Calculate the spacing of the text
            textsize = len(self.string) * vobj.FontSize.Value / 4.0
            spacing = (self.p3 - self.p2).Length/2.0 - textsize

            self.p2a = self.p2 + DraftVecUtils.scaleTo(self.p3 - self.p2,
                                                       spacing)
            self.p2b = self.p3 + DraftVecUtils.scaleTo(self.p2 - self.p3,
                                                       spacing)
            self.coords.point.setValues([[self.p1.x, self.p1.y, self.p1.z],
                                         [self.p2.x, self.p2.y, self.p2.z],
                                         [self.p2a.x, self.p2a.y, self.p2a.z],
                                         [self.p2b.x, self.p2b.y, self.p2b.z],
                                         [self.p3.x, self.p3.y, self.p3.z],
                                         [self.p4.x, self.p4.y, self.p4.z]])
            # self.line.numVertices.setValues([3, 3])
            self.line.coordIndex.setValues(0, 7, (0, 1, 2, -1, 3, 4, 5))
        else:
            self.coords.point.setValues([[self.p1.x, self.p1.y, self.p1.z],
                                         [self.p2.x, self.p2.y, self.p2.z],
                                         [self.p3.x, self.p3.y, self.p3.z],
                                         [self.p4.x, self.p4.y, self.p4.z]])
            # self.line.numVertices.setValue(4)
            self.line.coordIndex.setValues(0, 4, (0, 1, 2, 3))

    def onChanged(self, vobj, prop):
        """Execute when a view property is changed."""
        super().onChanged(vobj, prop)

        obj = vobj.Object
        properties = vobj.PropertiesList

        if prop == "ScaleMultiplier" and "ScaleMultiplier" in properties:
            # Update all dimension values
            if hasattr(self, "font"):
                self.font.size = vobj.FontSize.Value * vobj.ScaleMultiplier
            if (hasattr(self, "node_wld") and hasattr(self, "p2")
                    and "ArrowSize" in properties):
                self.remove_dim_arrows()
                self.draw_dim_arrows(vobj)
            if "DimOvershoot" in properties:
                self.remove_dim_overshoot()
                self.draw_dim_overshoot(vobj)
            if "ExtOvershoot" in properties:
                self.remove_ext_overshoot()
                self.draw_ext_overshoot(vobj)

            self.updateData(obj, "Start")

        elif (prop == "FontSize" and "FontSize" in properties
              and "ScaleMultiplier" in properties):
            if hasattr(self, "font"):
                self.font.size = vobj.FontSize.Value * vobj.ScaleMultiplier

        elif (prop == "FontName" and "FontName" in properties
              and hasattr(self, "font")):
            self.font.name = str(vobj.FontName)

        elif (prop == "TextColor" and "TextColor" in properties
              and hasattr(self, "textcolor")):
            col = vobj.TextColor
            self.textcolor.rgb.setValue(col[0], col[1], col[2])

        elif (prop == "LineColor" and "LineColor" in properties
              and hasattr(self, "linecolor")):
            col = vobj.LineColor
            self.linecolor.rgb.setValue(col[0], col[1], col[2])

        elif (prop == "LineWidth" and "LineWidth" in properties
              and hasattr(self, "drawstyle")):
            self.drawstyle.lineWidth = vobj.LineWidth

        elif (prop in ("ArrowSize", "ArrowType")
              and "ArrowSize" in properties
              and "ScaleMultiplier" in properties
              and hasattr(self, "node_wld") and hasattr(self, "p2")):
            self.remove_dim_arrows()
            self.draw_dim_arrows(vobj)

        elif (prop == "DimOvershoot"
              and "DimOvershoot" in properties
              and "ScaleMultiplier" in properties):
            self.remove_dim_overshoot()
            self.draw_dim_overshoot(vobj)

        elif (prop == "ExtOvershoot"
              and "ExtOvershoot" in properties
              and "ScaleMultiplier" in properties):
            self.remove_ext_overshoot()
            self.draw_ext_overshoot(vobj)

        elif prop == "ShowLine" and "ShowLine" in properties:
            if vobj.ShowLine:
                self.lineswitch_wld.whichChild = -3
                self.lineswitch_scr.whichChild = -3
            else:
                self.lineswitch_wld.whichChild = -1
                self.lineswitch_scr.whichChild = -1
        else:
            self.updateData(obj, "Start")

    def remove_dim_arrows(self):
        """Remove dimension arrows in the dimension lines.

        Remove the existing nodes.
        """
        self.node_wld.removeChild(self.marks)
        self.node_scr.removeChild(self.marks)

    def draw_dim_arrows(self, vobj):
        """Draw dimension arrows."""
        if not hasattr(vobj, "ArrowType"):
            return

        if self.p3.x < self.p2.x:
            inv = False
        else:
            inv = True

        # Set scale
        symbol = utils.ARROW_TYPES.index(vobj.ArrowType)
        s = vobj.ArrowSize.Value * vobj.ScaleMultiplier
        self.trans1.scaleFactor.setValue((s, s, s))
        self.trans2.scaleFactor.setValue((s, s, s))

        # Set new nodes
        self.marks = coin.SoSeparator()
        self.marks.addChild(self.linecolor)

        if vobj.Object.Diameter or not self.is_linked_to_circle():
            s1 = coin.SoSeparator()
            if symbol == "Circle":
                s1.addChild(self.coord1)
            else:
                s1.addChild(self.trans1)

            s1.addChild(gui_utils.dim_symbol(symbol, invert=not inv))
            self.marks.addChild(s1)

        s2 = coin.SoSeparator()
        if symbol == "Circle":
            s2.addChild(self.coord2)
        else:
            s2.addChild(self.trans2)

        s2.addChild(gui_utils.dim_symbol(symbol, invert=inv))
        self.marks.addChild(s2)

        self.node_wld.insertChild(self.marks, 2)
        self.node_scr.insertChild(self.marks, 2)

    def remove_dim_overshoot(self):
        """Remove the dimension overshoot lines."""
        self.node_wld.removeChild(self.marksDimOvershoot)
        self.node_scr.removeChild(self.marksDimOvershoot)

    def draw_dim_overshoot(self, vobj):
        """Draw dimension overshoot lines."""
        # Set scale
        s = vobj.DimOvershoot.Value * vobj.ScaleMultiplier
        self.transDimOvershoot1.scaleFactor.setValue((s, s, s))
        self.transDimOvershoot2.scaleFactor.setValue((s, s, s))

        # Remove existing nodes, and set new nodes
        self.marksDimOvershoot = coin.SoSeparator()
        if vobj.DimOvershoot.Value:
            self.marksDimOvershoot.addChild(self.linecolor)

            s1 = coin.SoSeparator()
            s1.addChild(self.transDimOvershoot1)
            s1.addChild(gui_utils.dimDash((-1, 0, 0), (0, 0, 0)))
            self.marksDimOvershoot.addChild(s1)

            s2 = coin.SoSeparator()
            s2.addChild(self.transDimOvershoot2)
            s2.addChild(gui_utils.dimDash((0, 0, 0), (1, 0, 0)))
            self.marksDimOvershoot.addChild(s2)

        self.node_wld.insertChild(self.marksDimOvershoot, 2)
        self.node_scr.insertChild(self.marksDimOvershoot, 2)

    def remove_ext_overshoot(self):
        """Remove dimension extension overshoot lines."""
        self.node_wld.removeChild(self.marksExtOvershoot)
        self.node_scr.removeChild(self.marksExtOvershoot)

    def draw_ext_overshoot(self, vobj):
        """Draw dimension extension overshoot lines."""
        # Set scale
        s = vobj.ExtOvershoot.Value * vobj.ScaleMultiplier
        self.transExtOvershoot1.scaleFactor.setValue((s, s, s))
        self.transExtOvershoot2.scaleFactor.setValue((s, s, s))

        # Set new nodes
        self.marksExtOvershoot = coin.SoSeparator()
        if vobj.ExtOvershoot.Value:
            self.marksExtOvershoot.addChild(self.linecolor)
            s1 = coin.SoSeparator()
            s1.addChild(self.transExtOvershoot1)
            s1.addChild(gui_utils.dimDash((0, 0, 0), (-1, 0, 0)))
            self.marksExtOvershoot.addChild(s1)

            s2 = coin.SoSeparator()
            s2.addChild(self.transExtOvershoot2)
            s2.addChild(gui_utils.dimDash((0, 0, 0), (-1, 0, 0)))
            self.marksExtOvershoot.addChild(s2)

        self.node_wld.insertChild(self.marksExtOvershoot, 2)
        self.node_scr.insertChild(self.marksExtOvershoot, 2)

    def is_linked_to_circle(self):
        """Return true if the dimension measures a circular edge."""
        obj = self.Object
        if obj.LinkedGeometry and len(obj.LinkedGeometry) == 1:
            linked_obj = obj.LinkedGeometry[0][0]
            subelements = obj.LinkedGeometry[0][1]
            if len(subelements) == 1 and "Edge" in subelements[0]:
                sub = subelements[0]
                index = int(sub[4:]) - 1
                edge = linked_obj.Shape.Edges[index]
                if DraftGeomUtils.geomType(edge) == "Circle":
                    return True
        return False

    def getIcon(self):
        """Return the path to the icon used by the viewprovider."""
        if self.is_linked_to_circle():
            return ":/icons/Draft_DimensionRadius.svg"
        return ":/icons/Draft_Dimension_Tree.svg"


# Alias for compatibility with v0.18 and earlier
_ViewProviderDimension = ViewProviderLinearDimension


class ViewProviderAngularDimension(ViewProviderDimensionBase):
    """Viewprovider for the Angular dimension object."""

    def attach(self, vobj):
        """Set up the scene sub-graph of the viewprovider."""
        self.Object = vobj.Object

        self.textpos = coin.SoTransform()
        self.textcolor = coin.SoBaseColor()
        self.font = coin.SoFont()
        self.text_wld = coin.SoAsciiText() # World orientation. Can be oriented in 3D space.
        self.text_scr = coin.SoText2()     # Screen orientation. Always faces the camera.

        # The text string needs to be initialized to something,
        # otherwise it may cause a crash of the system
        self.text_wld.string = "d"
        self.text_scr.string = "d"
        self.text_wld.justification = coin.SoAsciiText.CENTER
        self.text_scr.justification = coin.SoAsciiText.CENTER

        label_wld = coin.SoSeparator()
        label_wld.addChild(self.textpos)
        label_wld.addChild(self.textcolor)
        label_wld.addChild(self.font)
        label_wld.addChild(self.text_wld)

        label_scr = coin.SoSeparator()
        label_scr.addChild(self.textpos)
        label_scr.addChild(self.textcolor)
        label_scr.addChild(self.font)
        label_scr.addChild(self.text_scr)

        self.coord1 = coin.SoCoordinate3()
        self.trans1 = coin.SoTransform()
        self.coord2 = coin.SoCoordinate3()
        self.trans2 = coin.SoTransform()

        self.linecolor = coin.SoBaseColor()
        self.drawstyle = coin.SoDrawStyle()
        self.coords = coin.SoCoordinate3()
        import PartGui # Required for "SoBrepEdgeSet" (because a dimension is not a Part::FeaturePython object).
        self.arc = coin.SoType.fromName("SoBrepEdgeSet").createInstance()
        self.marks = coin.SoSeparator()

        self.node_wld = coin.SoGroup()
        self.node_wld.addChild(self.linecolor)
        self.node_wld.addChild(self.drawstyle)
        self.node_wld.addChild(self.coords)
        self.node_wld.addChild(self.arc)
        self.node_wld.addChild(self.marks)
        self.node_wld.addChild(label_wld)

        self.node_scr = coin.SoGroup()
        self.node_scr.addChild(self.linecolor)
        self.node_scr.addChild(self.drawstyle)
        self.node_scr.addChild(self.coords)
        self.node_scr.addChild(self.arc)
        self.node_scr.addChild(self.marks)
        self.node_scr.addChild(label_scr)

        vobj.addDisplayMode(self.node_wld, "World")
        vobj.addDisplayMode(self.node_scr, "Screen")
        self.updateData(vobj.Object, None)
        self.onChanged(vobj, "FontSize")
        self.onChanged(vobj, "FontName")
        self.onChanged(vobj, "TextColor")
        self.onChanged(vobj, "ArrowType")
        self.onChanged(vobj, "LineColor")

    def updateData(self, obj, prop):
        """Execute when a property from the Proxy class is changed."""
        if not hasattr(self, "arc"):
            return

        arcsegs = 24

        vobj = obj.ViewObject

        # Determine the orientation of the text by using a normal direction.
        # Also calculate the arc data.
        if DraftVecUtils.isNull(obj.Normal):
            norm = App.Vector(0, 0, 1)
        else:
            norm = obj.Normal

        radius = (obj.Dimline - obj.Center).Length
        self.circle = Part.makeCircle(radius, obj.Center, norm,
                                      obj.FirstAngle.Value,
                                      obj.LastAngle.Value)
        self.p2 = self.circle.Vertexes[0].Point
        self.p3 = self.circle.Vertexes[-1].Point
        midp = DraftGeomUtils.findMidpoint(self.circle.Edges[0])
        ray = midp - obj.Center

        # Set text value
        if obj.LastAngle.Value > obj.FirstAngle.Value:
            angle = obj.LastAngle.Value - obj.FirstAngle.Value
        else:
            angle = (360 - obj.FirstAngle.Value) + obj.LastAngle.Value

        show_unit = True
        if hasattr(vobj, "ShowUnit"):
            show_unit = vobj.ShowUnit

        if hasattr(vobj, "Decimals"):
            self.string = units.display_external(angle,
                                                 vobj.Decimals,
                                                 'Angle', show_unit)
        else:
            self.string = units.display_external(angle,
                                                 None,
                                                 'Angle', show_unit)

        if vobj.Override:
            self.string = vobj.Override.replace("$dim", self.string)

        self.text_wld.string = utils.string_encode_coin(self.string)
        self.text_scr.string = utils.string_encode_coin(self.string)

        # On first run the `DisplayMode` enumeration is not set, so we trap
        # the exception and set the display mode using the value
        # in the parameter database
        try:
            m = vobj.DisplayMode
        except AssertionError:
            m = ["World", "Screen"][utils.get_param("dimstyle", 0)]

        # Set the arc
        first = self.circle.FirstParameter
        last = self.circle.LastParameter

        if m == "Screen":
            # Calculate the spacing of the text
            spacing = len(self.string) * vobj.FontSize.Value / 8.0
            pts1 = []
            cut = None
            pts2 = []

            for i in range(arcsegs + 1):
                p = self.circle.valueAt(first + (last - first) / arcsegs * i)
                if (p - midp).Length <= spacing:
                    if cut is None:
                        cut = i
                else:
                    if cut is None:
                        pts1.append([p.x, p.y, p.z])
                    else:
                        pts2.append([p.x, p.y, p.z])

            self.coords.point.setValues(pts1 + pts2)

            pts1_num = len(pts1)
            pts2_num = len(pts2)
            i1 = pts1_num
            i2 = i1 + pts2_num

            self.arc.coordIndex.setValues(0,
                                          pts1_num + pts2_num + 1,
                                          list(range(pts1_num))
                                          + [-1]
                                          + list(range(i1, i2)))

            if pts1_num >= 3 and pts2_num >= 3:
                self.circle1 = Part.Arc(App.Vector(pts1[0][0],
                                                   pts1[0][1],
                                                   pts1[0][2]),
                                        App.Vector(pts1[1][0],
                                                   pts1[1][1],
                                                   pts1[1][2]),
                                        App.Vector(pts1[-1][0],
                                                   pts1[-1][1],
                                                   pts1[-1][2])).toShape()
                self.circle2 = Part.Arc(App.Vector(pts2[0][0],
                                                   pts2[0][1],
                                                   pts2[0][2]),
                                        App.Vector(pts2[1][0],
                                                   pts2[1][1],
                                                   pts2[1][2]),
                                        App.Vector(pts2[-1][0],
                                                   pts2[-1][1],
                                                   pts2[-1][2])).toShape()
        else:
            pts = []
            for i in range(arcsegs + 1):
                p = self.circle.valueAt(first + (last - first) / arcsegs * i)
                pts.append([p.x, p.y, p.z])

            self.coords.point.setValues(pts)
            self.arc.coordIndex.setValues(0,
                                          arcsegs + 1,
                                          list(range(arcsegs + 1)))

        # Set the arrow coords and rotation
        p2 = (self.p2.x, self.p2.y, self.p2.z)
        p3 = (self.p3.x, self.p3.y, self.p3.z)

        self.trans1.translation.setValue(p2)
        self.coord1.point.setValue(p2)
        self.trans2.translation.setValue(p3)
        self.coord2.point.setValue(p3)

        # Calculate small chords to make arrows look better
        if vobj.ArrowSize.Value !=0 \
                and hasattr(vobj, "ScaleMultiplier") \
                and vobj.ScaleMultiplier != 0 \
                and hasattr(vobj, "FlipArrows"):
            halfarrowlength = 2 * vobj.ArrowSize.Value * vobj.ScaleMultiplier
            arrowangle = 2 * math.asin(min(1, halfarrowlength / radius))
            if vobj.FlipArrows:
                arrowangle = -arrowangle

            u1 = (self.circle.valueAt(first + arrowangle)
                  - self.circle.valueAt(first)).normalize()
            u2 = (self.circle.valueAt(last)
                  - self.circle.valueAt(last - arrowangle)).normalize()

            w2 = self.circle.Curve.Axis
            w1 = w2.negative()

            v1 = w1.cross(u1)
            v2 = w2.cross(u2)
            _plane_rot_1 = DraftVecUtils.getPlaneRotation(u1, v1)
            _plane_rot_2 = DraftVecUtils.getPlaneRotation(u2, v2)
            q1 = App.Placement(_plane_rot_1).Rotation.Q
            q2 = App.Placement(_plane_rot_2).Rotation.Q

            self.trans1.rotation.setValue((q1[0], q1[1], q1[2], q1[3]))
            self.trans2.rotation.setValue((q2[0], q2[1], q2[2], q2[3]))

        # Set text position and rotation
        self.tbase = midp
        if (hasattr(vobj, "TextPosition")
                and not DraftVecUtils.isNull(vobj.TextPosition)):
            self.tbase = vobj.TextPosition

        u3 = ray.cross(norm).normalize()
        v3 = norm.cross(u3)
        _plane_rot_3 = DraftVecUtils.getPlaneRotation(u3, v3)
        r = App.Placement(_plane_rot_3).Rotation
        offset = r.multVec(App.Vector(0, 1, 0))

        if hasattr(vobj, "TextSpacing"):
            offset = DraftVecUtils.scaleTo(offset,
                                           vobj.TextSpacing.Value)
        else:
            offset = DraftVecUtils.scaleTo(offset, 0.05)

        if m == "Screen":
            offset = offset.negative()

        self.tbase = self.tbase.add(offset)
        q = r.Q
        self.textpos.translation.setValue([self.tbase.x,
                                           self.tbase.y,
                                           self.tbase.z])
        self.textpos.rotation = coin.SbRotation(q[0], q[1], q[2], q[3])

        # Set the angle property
        _round_1 = round(obj.Angle, utils.precision())
        _round_2 = round(angle, utils.precision())
        if _round_1 != _round_2:
            obj.Angle = angle

    def onChanged(self, vobj, prop):
        """Execute when a view property is changed."""
        super().onChanged(vobj, prop)

        obj = vobj.Object
        properties = vobj.PropertiesList

        if "ScaleMultiplier" in properties and vobj.ScaleMultiplier == 0:
            return

        if prop == "ScaleMultiplier" and "ScaleMultiplier" in properties:
            if hasattr(self, "font"):
                self.font.size = vobj.FontSize.Value * vobj.ScaleMultiplier
            if (hasattr(self, "node_wld") and hasattr(self, "p2")
                    and "ArrowSize" in properties):
                self.remove_dim_arrows()
                self.draw_dim_arrows(vobj)

            self.updateData(obj, None)

        elif prop == "FontSize" and "ScaleMultiplier" in properties:
            if hasattr(self, "font"):
                self.font.size = vobj.FontSize.Value * vobj.ScaleMultiplier

        elif (prop == "FontName" and hasattr(self, "font")):
            self.font.name = str(vobj.FontName)

        elif (prop == "TextColor" and "TextColor" in properties
              and hasattr(self, "textcolor")):
            col = vobj.TextColor
            self.textcolor.rgb.setValue(col[0], col[1], col[2])

        elif (prop == "LineColor" and "LineColor" in properties
              and hasattr(self, "linecolor")):
            col = vobj.LineColor
            self.linecolor.rgb.setValue(col[0], col[1], col[2])

        elif prop == "LineWidth" and hasattr(self, "drawstyle"):
            self.drawstyle.lineWidth = vobj.LineWidth

        elif (prop in ("ArrowSize", "ArrowType")
              and "ScaleMultiplier" in properties
              and hasattr(self, "node_wld") and hasattr(self, "p2")):
            self.updateData(obj, None)
            self.remove_dim_arrows()
            self.draw_dim_arrows(vobj)

        else:
            self.updateData(obj, None)

    def remove_dim_arrows(self):
        """Remove dimension arrows in the dimension lines.

        Remove the existing nodes.
        """
        self.node_wld.removeChild(self.marks)
        self.node_scr.removeChild(self.marks)

    def draw_dim_arrows(self, vobj):
        """Draw dimension arrows."""
        if not hasattr(vobj, "ArrowType"):
            return

        # Set scale
        symbol = utils.ARROW_TYPES.index(vobj.ArrowType)
        s = vobj.ArrowSize.Value * vobj.ScaleMultiplier
        self.trans1.scaleFactor.setValue((s, s, s))
        self.trans2.scaleFactor.setValue((s, s, s))

        # Set new nodes
        self.marks = coin.SoSeparator()
        self.marks.addChild(self.linecolor)

        s1 = coin.SoSeparator()
        if symbol == "Circle":
            s1.addChild(self.coord1)
        else:
            s1.addChild(self.trans1)
        s1.addChild(gui_utils.dim_symbol(symbol, invert=False))
        self.marks.addChild(s1)

        s2 = coin.SoSeparator()
        if symbol == "Circle":
            s2.addChild(self.coord2)
        else:
            s2.addChild(self.trans2)
        s2.addChild(gui_utils.dim_symbol(symbol, invert=True))
        self.marks.addChild(s2)

        self.node_wld.insertChild(self.marks, 2)
        self.node_scr.insertChild(self.marks, 2)

    def getIcon(self):
        """Return the path to the icon used by the viewprovider."""
        return ":/icons/Draft_DimensionAngular.svg"


# Alias for compatibility with v0.18 and earlier
_ViewProviderAngularDimension = ViewProviderAngularDimension

## @}
