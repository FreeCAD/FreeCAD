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
"""Provide the object code for Arch Wall object."""
## @package wall
# \ingroup ARCH
# \brief Provide the object code for Arch Wall.

import FreeCAD as App

import Draft,ArchComponent,ArchCommands,math

import DraftVecUtils, DraftGeomUtils
import Part, Draft
import math

if App.GuiUp:
    import FreeCADGui as Gui
    from PySide import QtCore, QtGui


class WallSegment():
    
    def __init__(self, obj):
        """
        Default Constructor
        """
        
        self.Type = 'Arch_WallSegment'
        self.ratios = None
        
        self.set_properties(obj)
        obj.Proxy = self
        self.Object = obj
    
    def set_properties(self, obj):

        # Base line : define wall core axis
        _tip = 'Start point of wall core axis'
        obj.addProperty('App::PropertyVector', 'FirstPoint', 
                        'BaseLine', _tip).FirstPoint = App.Vector(0,0,0)

        _tip = 'End point of wall core axis'
        obj.addProperty('App::PropertyVector', 'LastPoint', 
                        'BaseLine', _tip).LastPoint = App.Vector(4000,0,0)

        _tip = 'Constrain to wall XZ axis'                        
        obj.addProperty('App::PropertyBool', 'ConstrainToXZPlane', 
                        'BaseLine', _tip).ConstrainToXZPlane = True

        # Dimensions (Property Length is read only because it is determined by the points)
        obj.addProperty('App::PropertyLength', 'Length', 
                        'Dimensions', 'Wall length',1).Length = '4 m'

        obj.addProperty('App::PropertyLength', 'Width', 
                        'Dimensions', 'Wall width').Width = '35 cm'

        obj.addProperty('App::PropertyLength', 'Height', 
                        'Dimensions', 'Wall height').Height = '2.7 m'

        # Wall joining: All the angle properties are meant to be hidden and showed just on user demand
        _tip = 'Angular cut of first wall end inner layer (to be implemented)'
        obj.addProperty('App::PropertyAngle', 'FirstInnerLayerAngle', 
                        'Wall Ends', _tip, 4).FirstInnerLayerAngle = '90 deg'

        _tip = 'Angular cut of first wall end outer layer (to be implemented)'
        obj.addProperty('App::PropertyAngle', 'FirstOuterLayerAngle', 
                        'Wall Ends', _tip, 4).FirstOuterLayerAngle = '90 deg'
        
        _tip = 'Angular cut of first wall end core inner half'
        obj.addProperty('App::PropertyAngle', 'FirstCoreInnerAngle', 
                        'Wall Ends', _tip, 4).FirstCoreInnerAngle = '90 deg'
        
        _tip = 'Angular cut of first wall end core outer half'
        obj.addProperty('App::PropertyAngle', 'FirstCoreOuterAngle', 
                        'Wall Ends', _tip, 4).FirstCoreOuterAngle = '90 deg'

        _tip = 'Angular cut of first wall end (to be implemented)'
        obj.addProperty('App::PropertyAngle', 'LastInnerLayerAngle', 
                        'Wall Ends', _tip, 4).LastInnerLayerAngle = '90 deg'
        
        _tip = 'Angular cut of first wall end (to be implemented)'
        obj.addProperty('App::PropertyAngle', 'LastOuterLayerAngle', 
                        'Wall Ends', _tip, 4).LastOuterLayerAngle = '90 deg'
        
        _tip = 'Angular cut of first wall end (to be implemented)'
        obj.addProperty('App::PropertyAngle', 'LastCoreInnerAngle', 
                        'Wall Ends', _tip, 4).LastCoreInnerAngle = '90 deg'
        
        _tip = 'Angular cut of last wall end (to be implemented)'
        obj.addProperty('App::PropertyAngle', 'LastCoreOuterAngle', 
                        'Wall Ends', _tip,4).LastCoreOuterAngle = '90 deg'


    def __getstate__(self):
        return self.Type
    
    def __setstate__(self, state):
        if state:
            self.Type = state
    
    def execute(self, obj):
        """
        The wall shape is defined as 2 Part Wedge solids, fused together;
        splays are controlled by obj.FirstCoreOuterAngle, obj.LastCoreOuterAngle
                                 obj.FirstCoreInnerAngle, obj.LastCoreInnerAngle

        TODO: For further development maybe we can add another 2 Part Wedges
              to simulate inner layer and outer layer (ATM only core is represented)

                 <--> first_splay                <--> last_splay                                   
                 ---------------------------------  outer surface
                  \         Part Wedge 1          \ 
                   \           core axis           \    
        first_point o-------------------------------o  last_point  
                     \                               \    
                      \       Part Wedge 2            \   
                       ---------------------------------  inner surface
                    <--> first_splay                <--> last_splay                                   
        """
        
        if not hasattr(obj,"FirstPoint") or not hasattr(obj,"LastPoint") \
            or not hasattr(obj,"ConstrainToXZPlane") or not hasattr(obj,"Width") \
            or not hasattr(obj,"Height"):
            return

        length = obj.Length

        if obj.FirstPoint.x == obj.LastPoint.x or length < Draft.tolerance():
            return

        # swap first point and last point to have them in the right order
        # TODO: Swap the points phisically and change end constraints!
        if obj.FirstPoint.x < obj.LastPoint.x:
            first_point = obj.FirstPoint
        elif obj.FirstPoint.x > obj.LastPoint.x:
            first_point = obj.LastPoint
        
        if obj.ConstrainToXZPlane:
            first_splay = obj.Width/2 * math.tan(math.pi/2-math.radians(obj.FirstCoreInnerAngle))
            last_splay = obj.Width/2 * math.tan(math.pi/2-math.radians(obj.LastCoreInnerAngle))
            
            Xmin = 0
            Ymin = 0
            Zmin = 0
            Z2min = 0
            X2min = first_splay
            Xmax = length
            Ymax = obj.Width/2
            Zmax = obj.Height
            Z2max = obj.Height
            X2max = length - last_splay

            # checking conditions that will break Part.makeWedge()
            if first_splay >= length:
                print("Wall is too short compared to the first splay: removing angles of outer core layer\n")
                X2min = 0
            if last_splay >= length:
                print("Wall is too short compared to the last splay: removing angles of outer core layer\n")
                X2max = length
            if ( first_splay + last_splay ) >= length:
                print("Wall is too short compared to the splays: removing angles of inner core layer\n")
                X2min = 0
                X2max = length

            inner_core = Part.makeWedge( Xmin, Ymin, Zmin, Z2min, X2min,
                                            Xmax, Ymax, Zmax, Z2max, X2max)#, obj.FirstPoint, obj.LastPoint )
            inner_core.Placement.Base.x = first_point.x

            first_splay = obj.Width/2 * math.tan(math.pi/2-math.radians(obj.FirstCoreOuterAngle))
            last_splay = obj.Width/2 * math.tan(math.pi/2-math.radians(obj.LastCoreOuterAngle))          
            
            Xmin = first_splay
            Ymin = 0
            Zmin = 0
            Z2min = 0
            X2min = 0
            Xmax = length - last_splay
            Ymax = obj.Width/2
            Zmax = obj.Height
            Z2max = obj.Height
            X2max = length

            # checking conditions that will break Part.makeWedge()
            if first_splay >= length:
                print("Wall is too short compared to the first splay: removing angles of outer core layer\n")
                Xmin = 0
            if last_splay >= length:
                print("Wall is too short compared to the last splay: removing angles of outer core layer\n")
                Xmax = length
            if ( first_splay + last_splay ) >= length:
                print("Wall is too short compared to the splays: removing angles of outer core layer\n")
                Xmin = 0
                Xmax = length

            outer_core = Part.makeWedge( Xmin, Ymin, Zmin, Z2min, X2min,
                                            Xmax, Ymax, Zmax, Z2max, X2max)#, obj.Start, obj.End)
                    
            outer_core.Placement.Base = App.Vector(first_point.x, - obj.Width/2)

        else:
            print("ConstrainToXZPlane is set to false: Not implemented yet")
        
        core_layer = inner_core.fuse(outer_core)
        
        obj.Shape = core_layer


    def onBeforeChange(self, obj, prop):
        """this method is activated before a property changes"""
        pass


    def onChanged(self, obj, prop):
        """this method is activated when a property changes"""
        if prop == "FirstPoint" or prop == "LastPoint":
            if hasattr(obj, "FirstPoint") and hasattr(obj, "LastPoint"):
                #if obj.FirstPoint.x > obj.LastPoint.x:   circular
                #    obj.FirstPoint, obj.LastPoint = obj.LastPoint, obj.FirstPoint
                if hasattr(obj, "Length"):
                    obj.Length = abs(obj.LastPoint.x - obj.FirstPoint.x)


    def set_first_point(self, obj, first_point, local=False):
        """returns a part line representing the core axis of the wall"""
        if first_point != obj.LastPoint:
            self.set_point(obj, first_point, 0, local)
            return True
        else:
            print("You are trying to set the first point equal to the last point, this is not allowed.\n")
            return False

    def set_last_point(self, obj, last_point, local=False):
        """returns a part line representing the core axis of the wall"""
        if last_point != obj.FirstPoint:
            self.set_point(obj, last_point, 1, local)
            return True
        else:
            print("You are trying to set the last point equal to the first point, this is not allowed.\n")
            return False

    def set_point(self, obj, point, point_idx, local=False):
        """returns a part line representing the core axis of the wall"""
        if local:
            np = point
        else:
            np = obj.getGlobalPlacement().inverse().multVec(point)

        # assign the np to the first or end point of the wall
        if point_idx == 0:
            obj.FirstPoint = np
        elif point_idx == 1:
            obj.LastPoint = np

    def get_core_axis(self, obj):
        """returns a part line representing the core axis of the wall"""
        p1 = self.get_first_point(obj)
        p2 = self.get_last_point(obj)
        if p1 == p2:
            print("Points are equal, cannot get the axis")
            return None
        else:
            core_axis= Part.Line(p1, p2)
            return core_axis

    def get_first_point(self, obj):
        """returns a part line representing the core axis of the wall"""
        p1 = obj.getGlobalPlacement().multVec(obj.FirstPoint)
        return p1

    def get_last_point(self, obj):
        """returns a part line representing the core axis of the wall"""
        p2 = obj.getGlobalPlacement().multVec(obj.LastPoint)
        return p2
