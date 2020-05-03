#***************************************************************************
#*   Copyright (c) 2020 Carlo Pavan                                        *
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
"""Provide the object code for Arch View."""
## @package archview
# \ingroup ARCH
# \brief Provide the object code for Arch View.

from PySide.QtCore import QT_TRANSLATE_NOOP
import FreeCAD as App

import Draft,ArchComponent,ArchCommands,math

import DraftVecUtils, DraftGeomUtils
import Part, Draft
import math

if App.GuiUp:
    import FreeCADGui as Gui
    from PySide import QtCore, QtGui


class ArchView(object):
    """
    A prototype for a new wall object for the Arch Workbench
    """
    def __init__(self, obj=None):
        # print("runing wall object init method\n")
        if obj:
            # print("runing obj init method")

            obj.Proxy = self
            self.Object = obj
            self.attach(obj)
            self.execute(obj)

        self.Type = 'Arch_View'


    def set_properties(self, obj):
        pl = obj.PropertiesList
        if not "Placement" in pl:
            _tip = "The placement of this object"
            obj.addProperty("App::PropertyPlacement", "Placement",
                            "SectionPlane", QT_TRANSLATE_NOOP("App::Property", _tip))
        
        if not "Shape" in pl:
            obj.addProperty("Part::PropertyPartShape", "Shape",
                            "SectionPlane", QT_TRANSLATE_NOOP("App::Property","The shape of this object"))
        
        if not "Objects" in pl:
            obj.addProperty("App::PropertyLinkListGlobal", "Objects", #changed to global
                            "SectionPlane", QT_TRANSLATE_NOOP("App::Property","The objects that must be considered by this section plane. Empty means the whole document."))
        
        if not "OnlySolids" in pl:
            obj.addProperty("App::PropertyBool", "OnlySolids",
                            "SectionPlane", QT_TRANSLATE_NOOP("App::Property","If false, non-solids will be cut too, with possible wrong results."))
            obj.OnlySolids = True
        
        if not "Clip" in pl:
            obj.addProperty("App::PropertyBool", "Clip",
                            "SectionPlane", QT_TRANSLATE_NOOP("App::Property","If True, resulting views will be clipped to the section plane area."))
        
        if not "UseMaterialColorForFill" in pl:
            obj.addProperty("App::PropertyBool", "UseMaterialColorForFill",
                            "SectionPlane",QT_TRANSLATE_NOOP("App::Property","If true, the color of the objects material will be used to fill cut areas."))
            obj.UseMaterialColorForFill = False
        self.Type = "SectionPlane"


    def attach(self,obj):

        # print("running" + obj.Name + "attach() method\n")
        obj.addExtension('App::GeoFeatureGroupExtensionPython', None)
        self.set_properties(obj)


    def execute(self, obj):
        """ Compute the wall shape as boolean operations among the children objects """
        # print("running " + obj.Name + " execute() method\n")
        # get wall base shape from BaseGeometry object
        import Part
        l = 1
        h = 1
        if obj.ViewObject:
            if hasattr(obj.ViewObject,"DisplayLength"):
                l = obj.ViewObject.DisplayLength.Value
                h = obj.ViewObject.DisplayHeight.Value
            elif hasattr(obj.ViewObject,"DisplaySize"):
                # old objects
                l = obj.ViewObject.DisplaySize.Value
                h = obj.ViewObject.DisplaySize.Value
        p = Part.makePlane(l, h, App.Vector(l/2,-h/2,0), App.Vector(0,0,-1))
        # make sure the normal direction is pointing outwards, you never know what OCC will decide...
        if p.normalAt(0,0).getAngle(obj.Placement.Rotation.multVec(App.Vector(0,0,1))) > 1:
            p.reverse()
        p.Placement = obj.Placement
        obj.Shape = p

    def onBeforeChange(self, obj, prop):
        """this method is activated before a property changes"""
        return


    def onChanged(self, obj, prop):
        """this method is activated when a property changes"""
        if prop in ["Placement","Objects","OnlySolids","UseMaterialColorForFill","Clip"]:
            self.svgcache = None
            self.shapecache = None

    def getNormal(self, obj):
        return obj.Shape.Faces[0].normalAt(0,0)


    # Other methods +++++++++++++++++++++++++++++++++++++++++++++++++++++++++


    def onDocumentRestored(self, obj):
        self.Object = obj
        # obj.Proxy.Type needs to be re-setted every time the document is opened.
        obj.Proxy.Type = "Arch_View"


    def __getstate__(self):
        return


    def __setstate__(self,_state):
        return