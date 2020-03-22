# ***************************************************************************
# *   (c) 2020 Carlo Pavan                                                  *
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

"""This module provides the view provider code for Draft DimensionStyle.
"""
## @package polararray
# \ingroup DRAFT
# \brief This module provides the view provider code for Draft DimensionStyle.

import FreeCAD as App
from Draft import _ViewProviderDraft
from PySide.QtCore import QT_TRANSLATE_NOOP
import draftutils.utils as utils
from pivy import coin

class ViewProviderDraftDimensionStyle(_ViewProviderDraft):
    """
    Dimension style dont have a proper object but just a viewprovider.
    It stores inside a document object dimension settings and restore them on demand.
    """
    def __init__(self, vobj, existing_dimension = None):
        """
        vobj properties         type                        parameter               type
        ----------------------------------------------------------------------------------
        vobj.ScaleMultiplier"   App::PropertyFloat          "DraftAnnotationScale"  Float

        vobj.FontName           App::PropertyFont           "textfont"              Text
        vobj.FontSize           App::PropertyLength         "textheight"            Float
        vobj.TextSpacing        App::PropertyLength         "dimspacing"            Float

        vobj.Decimals           App::PropertyInteger        "dimPrecision"          Integer
        vobj.ShowUnit           App::PropertyBool
        vobj.UnitOverride       App::PropertyString

        vobj.LineWidth          App::PropertyFloat
        vobj.LineColor          App::PropertyColor
        vobj.ArrowSize          App::PropertyLength         "arrowsize"             Float
        vobj.ArrowType          App::PropertyEnumeration    "dimsymbol"             Integer
        vobj.FlipArrows         App::PropertyBool
        vobj.DimOvershoot       App::PropertyDistance       "dimovershoot"          Float
        vobj.ExtLines           App::PropertyDistance       "extlines"              Float
        vobj.ExtOvershoot       App::PropertyDistance       "extovershoot"          Float
        vobj.ShowLine           App::PropertyBool
        """

        # annotation properties
        vobj.addProperty("App::PropertyFloat","ScaleMultiplier",
                         "Annotation",
                         QT_TRANSLATE_NOOP("App::Property",
                                           "Dimension size overall multiplier"))

        vobj.addProperty("App::PropertyBool","AutoUpdate",
                         "Annotation",
                         QT_TRANSLATE_NOOP("App::Property",
                                           "Auto update associated dimensions"))

        # text properties
        vobj.addProperty("App::PropertyFont","FontName",
                         "Text",
                         QT_TRANSLATE_NOOP("App::Property","Font name"))

        vobj.addProperty("App::PropertyLength","FontSize",
                         "Text",
                         QT_TRANSLATE_NOOP("App::Property",
                                           "Font size"))

        vobj.addProperty("App::PropertyLength","TextSpacing",
                         "Text",
                         QT_TRANSLATE_NOOP("App::Property",
                                           "The spacing between the text and "
                                           "the dimension line"))

        # units properties
        vobj.addProperty("App::PropertyInteger","Decimals",
                         "Units",
                         QT_TRANSLATE_NOOP("App::Property",
                                           "The number of decimals to show"))

        vobj.addProperty("App::PropertyBool","ShowUnit",
                         "Units",
                         QT_TRANSLATE_NOOP("App::Property",
                                           "Show the unit suffix"))

        vobj.addProperty("App::PropertyString","UnitOverride",
                         "Units",
                         QT_TRANSLATE_NOOP("App::Property",
                                           "A unit to express the measurement. "
                                           "Leave blank for system default"))

        # graphics properties
        vobj.addProperty("App::PropertyFloat","LineWidth",
                         "Graphics",
                         QT_TRANSLATE_NOOP("App::Property","Line width"))

        vobj.addProperty("App::PropertyColor","LineColor",
                         "Graphics",
                         QT_TRANSLATE_NOOP("App::Property","Line color"))

        vobj.addProperty("App::PropertyLength","ArrowSize",
                         "Graphics",
                         QT_TRANSLATE_NOOP("App::Property","Arrow size"))

        vobj.addProperty("App::PropertyEnumeration","ArrowType",
                         "Graphics",
                         QT_TRANSLATE_NOOP("App::Property","Arrow type"))

        vobj.addProperty("App::PropertyBool","FlipArrows",
                         "Graphics",
                         QT_TRANSLATE_NOOP("App::Property",
                                           "Rotate the dimension arrows 180 degrees"))

        vobj.addProperty("App::PropertyDistance","DimOvershoot",
                         "Graphics",
                         QT_TRANSLATE_NOOP("App::Property",
                                           "The distance the dimension line is "
                                           "extended past the extension lines"))

        vobj.addProperty("App::PropertyDistance","ExtLines",
                         "Graphics",
                         QT_TRANSLATE_NOOP("App::Property",
                                           "Length of the extension lines"))

        vobj.addProperty("App::PropertyDistance","ExtOvershoot",
                         "Graphics",
                         QT_TRANSLATE_NOOP("App::Property",
                                           "Length of the extension line above "
                                           "the dimension line"))

        vobj.addProperty("App::PropertyBool","ShowLine",
                         "Graphics",
                         QT_TRANSLATE_NOOP("App::Property",
                                           "Shows dimension line and arrows"))

        self.init_properties(vobj, existing_dimension)
            
        super().__init__(vobj)

    def init_properties(self, vobj, existing_dimension):
        """
        Initializes Dimension Style properties
        """
        # get the style from FreeCAD Draft Parameters
        param = App.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
        annotation_scale = param.GetFloat("DraftAnnotationScale", 1.0)

        vobj.ScaleMultiplier = 1 / annotation_scale
        vobj.AutoUpdate = True

        vobj.FontName = utils.get_param("textfont","")
        vobj.FontSize = utils.get_param("textheight",0.20)
        vobj.TextSpacing = utils.get_param("dimspacing",0.05)

        vobj.Decimals = utils.get_param("dimPrecision",2)
        vobj.ShowUnit = utils.get_param("showUnit",True)

        vobj.ArrowSize = utils.get_param("arrowsize",0.1)
        vobj.ArrowType = utils.ARROW_TYPES
        vobj.ArrowType = utils.ARROW_TYPES[utils.get_param("dimsymbol",0)]
        vobj.DimOvershoot = utils.get_param("dimovershoot",0)  
        vobj.ExtLines = utils.get_param("extlines",0.3)
        vobj.ExtOvershoot = utils.get_param("extovershoot",0)
        vobj.ShowLine = True          

        if existing_dimension and hasattr(existing_dimension, "ViewObject"):
            # get the style from given dimension
            from draftutils import gui_utils
            gui_utils.format_object(target = vobj.Object, origin = existing_dimension)
          
    def onChanged(self, vobj, prop):
        if hasattr(vobj, "AutoUpdate"):
            if vobj.AutoUpdate:
                self.update_related_dimensions(vobj)
        if hasattr(vobj, "Visibility"):
            if prop == "Visibility":
                print(vobj.Visibility)

    def doubleClicked(self,vobj):
        self.set_current(vobj)

    def setupContextMenu(self,vobj,menu):
        action1 = menu.addAction("Set current")
        action1.triggered.connect(lambda f=self.set_current, arg=vobj:f(arg))
        action2 = menu.addAction("Update dimensions")
        action2.triggered.connect(lambda f=self.update_related_dimensions, arg=vobj:f(arg))
    
    def set_current(self, vobj):
        """
        Sets the current dimension style as default for new created dimensions
        """
        param = App.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
        param.SetFloat("DraftAnnotationScale", 1 / vobj.ScaleMultiplier)

        param.SetString("textfont", vobj.FontName)
        param.SetFloat("textheight", vobj.FontSize)
        param.SetFloat("dimspacing", vobj.TextSpacing)
 
        param.SetInt("dimPrecision", vobj.Decimals)

        param.SetFloat("arrowsize", vobj.ArrowSize)
        param.SetInt("dimsymbol", utils.ARROW_TYPES.index(vobj.ArrowType))
        param.SetFloat("dimovershoot", vobj.DimOvershoot)
        param.SetFloat("extlines", vobj.ExtLines)
        param.SetFloat("extovershoot", vobj.ExtOvershoot)
        
        App.Console.PrintMessage("Current dimension style set to " + str(vobj.Object.Label) + "\n")

    def update_related_dimensions(self, vobj):
        """
        Apply the style to the related dimensions
        """
        from draftutils import gui_utils
        for dim in vobj.Object.InList:
            gui_utils.format_object(target = dim, origin = vobj.Object)

    def getIcon(self):
        import Draft_rc
        return ":/icons/Draft_Dimension_Tree_Style.svg"

    def attach(self, vobj):
        self.standard = coin.SoGroup()
        vobj.addDisplayMode(self.standard,"Standard")

    def getDisplayModes(self,obj):
        "'''Return a list of display modes.'''"
        return ["Standard"]

    def getDefaultDisplayMode(self):
        "'''Return the name of the default display mode. It must be defined in getDisplayModes.'''"
        return "Standard"