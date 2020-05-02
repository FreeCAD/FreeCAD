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
"""Provide the object code for Arch Wall."""
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


class Wall(object):
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

        self.Type = 'Arch_Wall'



    def set_properties(self, obj):
        # obj.addProperty('App::PropertyPlacement', 'GlobalPlacement', 
        #                'Base', 
        #                'Object global Placement', 1)

        # Ifc Properties ----------------------------------------------------
        obj.addProperty('App::PropertyString', 'IfcType', 
                'IFC', 
                'Ifc class that describe the object').IfcType = "Wall"

        # LEVEL Properties (not implemented yet) ----------------------------
        _tip = 'Constrain the wall base to the parent level'
        obj.addProperty('App::PropertyBool', 'BaseConstrain', 
                        'Level properties', 
                        _tip).BaseConstrain = True

        _tip = 'If the wall base is constrained to the parent level,\
                set Z offset'
        obj.addProperty('App::PropertyLength', 'BaseOffset', 
                        'Level properties', 
                        _tip).BaseOffset = '0'

        _tip = 'Constrain the wall top to the upper level'
        obj.addProperty('App::PropertyBool', 'TopConstrain', 
                        'Level properties', 
                        _tip).TopConstrain = True

        _tip = 'If the wall top is constrained to the parent level,\
                set Z offset'
        obj.addProperty('App::PropertyLength', 'TopOffset', 
                        'Level properties', 
                        _tip).TopOffset = '0'

        # COMPONENTS Properties (partially implemented at the moment) ---------
        _tip = 'Optional object to use as base geometry for the wall shape'
        obj.addProperty('App::PropertyLinkChild', 'BaseGeometry',
                        'Components', _tip) # TODO: better PropertyLinkListGlobal or PropertyLinkListChild?
        
        _tip = 'List of objects to fuse with the wall shape'
        obj.addProperty('App::PropertyLinkListGlobal', 'Fusions',
                        'Components', _tip) # TODO: better PropertyLinkListGlobal or PropertyLinkListChild?
        
        _tip = 'List of wall subcomponent objects.\n'\
               'Sub-Components have to be grouped into the wall object.'
        obj.addProperty('App::PropertyLinkListChild', 'SubComponents',
                        'Components', _tip)

        _tip = 'List of objects to subtract from the wall shape'
        obj.addProperty('App::PropertyLinkListGlobal', 'Subtractions',
                        'Components', _tip)

        _tip = 'List of windows inserted into the wall.\n'\
               'Windows have to be grouped into the wall object.'
        obj.addProperty('App::PropertyLinkListChild', 'Windows',
                        'Components', _tip)

        # GEOMETRY Properties -----------------------------------------------
        _tip = 'Define the X coorinate of the start point of the core axis.\n'\
               'Value in millimeters'
        obj.addProperty('App::PropertyFloat', 'AxisFirstPointX', #change to BaselineStart
                        'Geometry', _tip).AxisFirstPointX = 0.0

        _tip = 'Define the X coorinate of the end point of the core axis.\n'\
               'Value in millimeters'
        obj.addProperty('App::PropertyFloat', 'AxisLastPointX', #change to BaselineEnd
                        'Geometry', _tip).AxisLastPointX = 4000.0

        obj.addProperty('App::PropertyLength', 'Length',
                        'Geometry', 'Wall length',1).Length = '4 m'

        obj.addProperty('App::PropertyLength', 'Width',
                        'Geometry', 'Wall width').Width = '35 cm'

        obj.addProperty('App::PropertyLength', 'Height',
                        'Geometry', 'Wall height').Height = '2.7 m'

        # WALL CONNECTIONS Properties ---------------------------------------
        _tip = "Allow automatic compute of first end"
        obj.addProperty('App::PropertyBool', 'JoinFirstEnd',# TODO: Transform to AutoJoinFirstEnd
                        'Wall connections', _tip).JoinFirstEnd = True

        _tip = "Allow automatic compute of last end"
        obj.addProperty('App::PropertyBool', 'JoinLastEnd',# TODO: Transform to AutoJoinLastEnd
                        'Wall connections', _tip).JoinLastEnd = True

        _tip = "Names of the objects that target current wall"
        obj.addProperty('App::PropertyStringList', 'IncomingTJoins',
                        'Wall connections', _tip).IncomingTJoins = []

        _tip = "Name of the object to join wall's first end"
        obj.addProperty('App::PropertyString', 'JoinFirstEndTo',
                        'Wall connections', _tip).JoinFirstEndTo = ''

        _tip = "Name of the object to join wall's last end"
        obj.addProperty('App::PropertyString', 'JoinLastEndTo',
                        'Wall connections', _tip).JoinLastEndTo = ''

        # WALL ENDS Properties ---------------------------------------------- 
        # All the angle properties are meant to be hidden and showed just on user demand
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


    def attach(self,obj):

        # print("running" + obj.Name + "attach() method\n")
        obj.addExtension('App::GeoFeatureGroupExtensionPython', None)
        self.set_properties(obj)


    def execute(self, obj):
        """ Compute the wall shape as boolean operations among the children objects """
        # print("running " + obj.Name + " execute() method\n")
        # get wall base shape from BaseGeometry object
        wall_shape = None

        if hasattr(obj, "BaseGeometry") and obj.BaseGeometry:
            if hasattr(obj.BaseGeometry, "Shape"):
                wall_shape = obj.BaseGeometry.Shape
        else:
            wall_shape = self.get_default_shape(obj)
        
        if wall_shape is None:
            return

        """
        Perform boolean operations between the base shape and 
        Additions, Subtractions, and windows.

        Windows have to provide a proper shape to cut the wall as a 
        WallVoid object or (TODO: a Part::PropertyPartShape)
        """

        # subtract Subtractions
        if hasattr(obj, "Subtractions"):
            if obj.Subtractions is not None and obj.Subtractions != []:
                for o in obj.Subtractions:
                    if o in obj.Group and hasattr(o, "Shape"):
                        # subtraction object is inside the wall
                        relative_placement = o.Placement
                        if hasattr(o, "InList"):
                            if o.InList[0] != obj:
                                relative_placement = o.InList[0].Placement.multiply(o.Placement)
                        cut_shape = o.Shape.copy()
                        cut_shape.Placement = relative_placement
                        wall_shape = wall_shape.cut(cut_shape)
                    elif hasattr(o, "Shape"):
                        # subtraction object is not inside the wall
                        global_placement = o.getGlobalPlacement()
                        relative_placement = obj.getGlobalPlacement().inverse().multiply(global_placement)
                        cut_shape = o.Shape.copy()
                        cut_shape.Placement = relative_placement
                        wall_shape = wall_shape.cut(cut_shape)

        if hasattr(obj,"Windows"):
            # objects marked as windows must have WallVoid PropertyLinkChild
            if obj.Windows and obj.Windows != []:
                for win in obj.Windows:
                    # cut window void
                    window_void = None
                    cut_done = False
                    if hasattr(win, "WallVoid"):
                        if win.WallVoid:
                            if hasattr(win.WallVoid, "Shape"):
                                window_void = win.WallVoid
                            if win.TypeId == 'App::Part' or win.TypeId == 'App::Link':
                                container_placement = win.Placement
                            else:
                                container_placement = App.Placement()

                            if window_void is not None:
                                cut_shape = window_void.Shape.copy()
                                cut_shape.Placement = container_placement.multiply(cut_shape.Placement)
                                wall_shape = wall_shape.cut(cut_shape)
                                cut_done = True

                '''_compound = [wall_shape]
                # this was used to collect Wall Additions from each window object
                for win in obj.Windows:
                    # collect window shapes to be added to the wall shape
                    for o in win.Group:
                        if o == win.WallVoid:
                            continue
                        elif hasattr(o, "Shape"):
                            if win.TypeId == 'App::Part' or win.TypeId == 'App::Link':
                                container_placement = win.Placement
                            else:
                                container_placement = App.Placement()
                            add_shape = o.Shape.copy()
                            add_shape.Placement = container_placement.multiply(o.Placement)
                            _compound.append(add_shape)
                
                # collect window shapes to be added to the wall shape
                wall_shape = Part.Compound(_compound)'''

        obj.Shape = wall_shape


    def onBeforeChange(self, obj, prop):
        """this method is activated before a property changes"""
        
        # WALL ENDS properties: remove the old join references before computing the new
        if (hasattr(obj, "JoinFirstEndTo") and hasattr(obj, "JoinLastEndTo") and
            hasattr(obj, "JoinFirstEnd")and hasattr(obj, "JoinLastEnd")):

            if prop == "JoinFirstEndTo" and obj.JoinFirstEnd:
                target = App.ActiveDocument.getObject(obj.JoinFirstEndTo)
                if hasattr(target, "IncomingTJoins"):
                    lst = target.IncomingTJoins
                    if obj.Name in lst:
                        lst.remove(obj.Name)
                        target.IncomingTJoins = lst

            elif prop == "JoinLastEndTo" and obj.JoinLastEnd:
                target = App.ActiveDocument.getObject(obj.JoinFirstEndTo)
                if hasattr(target, "IncomingTJoins"):
                    lst = target.IncomingTJoins
                    if obj.Name in lst:
                        lst.remove(obj.Name)
                        target.IncomingTJoins = lst

        if prop == "Group":
            # store the previous configuration of wall Group property
            # so the onChanged method can compare with the new configuration
            # and understand if objects were added or removed
            self.oldGroup = obj.Group
    

    def get_default_shape(self, obj):
        """
        The wall default base shape is defined as 2 Part Wedge solids, fused together;
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
        
        if not hasattr(obj,"AxisFirstPointX") or not hasattr(obj,"AxisLastPointX") \
            or not hasattr(obj,"Width") or not hasattr(obj,"Height"):
            return

        length = obj.Length

        if obj.AxisFirstPointX == obj.AxisLastPointX or length < Draft.tolerance():
            return

        # swap first point and last point to have them in the right order
        # TODO: Swap the points phisically and change end constraints!
        if obj.AxisFirstPointX < obj.AxisLastPointX:
            first_point = obj.AxisFirstPointX
        elif obj.AxisFirstPointX > obj.AxisLastPointX:
            first_point = obj.AxisLastPointX
        
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
                                        Xmax, Ymax, Zmax, Z2max, X2max)#, obj.AxisFirstPointX, obj.AxisLastPointX )
        inner_core.Placement.Base.x = first_point

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
                
        outer_core.Placement.Base = App.Vector(first_point, - obj.Width/2)
        
        core_layer = inner_core.fuse(outer_core)
        
        # TODO: Add support for multiple wall layers.
        #       I was thinking to just 3 layers in the representation, cause it's usually enough

        return core_layer


    def onChanged(self, obj, prop):
        """this method is activated when a property changes"""
        if prop == "Placement":
            if hasattr(obj, "Placement"): # TODO: recompute only if end is set
                # Recompute wall joinings
                self.recompute_ends(obj, 0)
                self.recompute_ends(obj, 1)
                for t_name in obj.IncomingTJoins:
                    t = App.ActiveDocument.getObject(t_name)
                    t.Proxy.recompute_ends(t, 0)
                    t.Proxy.recompute_ends(t, 1)
                # Update global placement Property (not working so good)
                # obj.GlobalPlacement = obj.getGlobalPlacement()

        # WALL JOIN ENDS properties
        if (hasattr(obj, "JoinFirstEndTo") and hasattr(obj, "JoinLastEndTo") and
            hasattr(obj, "JoinFirstEnd")and hasattr(obj, "JoinLastEnd")):

            if prop == "JoinFirstEndTo" and obj.JoinFirstEnd:
                self.recompute_ends(obj, 0)

            elif prop == "JoinLastEndTo" and obj.JoinLastEnd:
                self.recompute_ends(obj, 1)

        if prop == "AxisFirstPointX" or prop == "AxisLastPointX":
            if hasattr(obj, "AxisFirstPointX") and hasattr(obj, "AxisLastPointX"):
                #if obj.AxisFirstPointX.x > obj.AxisLastPointX.x:   circular
                #    obj.AxisFirstPointX, obj.AxisLastPointX = obj.AxisLastPointX, obj.AxisFirstPointX
                if hasattr(obj, "Length"):
                    obj.Length = abs(obj.AxisLastPointX - obj.AxisFirstPointX)

        # CHILDREN properties: remember to first assign basegeometry and then add the object to the group
        if prop == "BaseGeometry":
            if hasattr(obj, "BaseGeometry"):
                pass

        # Group property: an object is added or removed from the wall
        if prop == "Group":
            if hasattr(obj, "Group") and hasattr(obj, "BaseGeometry") and hasattr(obj, "Subtractions"):
                if hasattr(self, "oldGroup"):
                    # understand if the object was added or removed
                    added_objs = [x for x in obj.Group if x not in self.oldGroup]
                    removed_objs = [x for x in self.oldGroup if x not in obj.Group]
                    del self.oldGroup
                for o in removed_objs:
                    # if it was removed, remove it from wall children linking
                    print("Removing " + o.Label + " from " + obj.Label)
                    if o == obj.BaseGeometry:
                        obj.BaseGeometry = None

                    elif o in obj.Subtractions:
                        openings = obj.Subtractions
                        openings.remove(o)
                        obj.Subtractions = openings

                    elif o in obj.Windows:
                        windows = obj.Windows
                        windows.remove(o)
                        obj.Windows = windows

                for o in added_objs:
                    # if it was added, check if it is a window or ask if it has to be treated as an Opening
                    print("Adding " + o.Name + " to " + obj.Label)
                    if o == obj.BaseGeometry:
                        continue

                    if hasattr(o, "IfcType"):
                        if o.IfcType == 'Window':
                            windows = obj.Windows
                            windows.append(o)
                            obj.Windows = windows
                            continue

                    if not o in obj.Subtractions:
                        print("added a new object to the wall")
                        self.add_subtraction(obj, o)


    # Wall joinings methods +++++++++++++++++++++++++++++++++++++++++++++++++


    def recompute_ends(self, obj, end_idx):
        """
        This method auto recompute the first or the last wall end joint
        If the obj and the target objects are both joinable it recompute the
        joints, if not it resets the corresponding wall end to 90 deg.

        Parameters
        -----
        obj         wall object
        end_idx     None or 0 or 1
                    the wall end index:
                    0 for first end
                    1 for last end
                    2 for both ends
        """
        if obj == None:
            print("Cannot recompute ends of a None object")
            return
        if obj.JoinFirstEndTo == obj.JoinLastEndTo and obj.JoinFirstEndTo != "":
            print("The wall cannot target the same wall on both JoinFirst and JoinLast properties")
            return
        if end_idx == 0:
            target = App.ActiveDocument.getObject(obj.JoinFirstEndTo)
            if target == obj or target == None:
                return
            if self.is_wall_joinable(obj):
                if self.is_wall_joinable(target):
                    self.join_end(obj, target, 0)
                else:
                    self.reset_end(obj, 0)

        if end_idx == 1:
            target = App.ActiveDocument.getObject(obj.JoinLastEndTo)
            if target == obj or target == None:
                return
            if self.is_wall_joinable(obj):
                if self.is_wall_joinable(target):
                    self.join_end(obj, target, 1)
                else:
                    self.reset_end(obj, 1)


    def is_wall_joinable(self, obj):
        """
        Returns True if the given object type is 'Arch_Wall' and if its
        BaseGeometry is an 'Arch_WallSegment' object.
        in every other case returns False.
        """

        if Draft.get_type(obj) != "Arch_Wall":
            print("Wall " + obj.Name + "is not a valid Arch_Wall objects")
            return False
        if obj.BaseGeometry is not None:
            print("Wall Joining only works if base geometry is not set")
            return False
        return True


    def reset_end(self, obj, idx):
        """
        Reset given wall object end joints.

        Parameters
        -----
        obj         wall object
        end_idx     the wall end index to reset
                    0 for first end
                    1 for last end
        """
        print("running reset_end() "+obj.Name+"_"+str(idx)+"\n")
        if idx == 0:
            obj.FirstCoreInnerAngle = 90
            obj.FirstCoreOuterAngle = 90
        elif idx == 1:
            obj.LastCoreInnerAngle = 90
            obj.LastCoreOuterAngle = 90


    def remove_linked_walls_references(self, obj):
        """ 
        Removes the reference to given wall to all the other 
        walls that target it to join
        """
        print("REMOVE ALL REFERENCES on DELETING")
        references = obj.IncomingTJoins
        references.append(obj.JoinFirstEndTo)
        references.append(obj.JoinLastEndTo)
    
        for link in references:
            o = App.ActiveDocument.getObject(link)

            if o:
                if hasattr(o, "JoinFirstEndTo"):
                    if o.JoinFirstEndTo == obj.Name:
                        o.JoinFirstEndTo = ""
                if hasattr(o, "JoinLastEndTo"):
                    if o.JoinLastEndTo == obj.Name:
                        o.JoinLastEndTo = ""
                if hasattr(o, "IncomingTJoins"):
                    if obj.Name in o.IncomingTJoins:
                        target_list = o.IncomingTJoins
                        target_list.remove(obj.Name)
                        o.IncomingTJoins = target_list


    def join_end(self, obj, target, end_idx):
        """ Join the wall to the target wall """
        # calculate which type of joining
        join_type, target_idx = self.guess_join_type(obj, target)

        if join_type == "T":
            w_ext = self.extend(obj, target, end_idx)
            if w_ext:
                self.T_join(obj, target, end_idx)
            else:
                return False

        elif join_type == "L":
            w_ext = self.extend(obj, target, end_idx)
            t_ext = self.extend(target, obj, target_idx)
            if w_ext and t_ext:
                self.L_join(obj, target, end_idx, target_idx)
                self.L_join(target, obj, target_idx, end_idx)
            else:
                return False
   
        return True 
    

    def guess_join_type(self, obj, target):
        """ Guess which kind of joint to apply to the given wall """
        # print("running guess_join_type()\n")
        if target.JoinFirstEndTo == obj.Name and target.JoinFirstEnd:
            return "L", 0
        elif target.JoinLastEndTo == obj.Name and target.JoinLastEnd:
            return "L", 1
        else:
            return "T", None


    def extend(self, wall, target, idx):
        """ Extend the given wall to the target wall """
        print("--------\n"+"Extend "+wall.Name + " to " +target.Name+ "\n")

        wall_core_axis = wall.Proxy.get_core_axis(wall)#.toShape()
        target_core_axis = target.Proxy.get_core_axis(target)#.toShape()
        if wall_core_axis is None or target_core_axis is None:
            print("Failed to get wall core axis")
            return False

        int_pts = wall_core_axis.intersect(target_core_axis)
        if len(int_pts) == 1:
            int_p = int_pts[0]        
            intersection = App.Vector(int_p.X,int_p.Y,int_p.Z)
        else:
            print("No intersection point found, or too many intersection points found")
            return False

        if idx == 0:
            wall.Proxy.set_first_point(wall, intersection)
            return True
        elif idx == 1:
            wall.Proxy.set_last_point(wall, intersection)
            return True


    def T_join(self, wall, target, idx): 
        """ Compute wall angles according to given parameters """
        print("--------\n"+"T_Join "+wall.Name + " with " +target.Name+ "\n")

        if idx == 0:
            w1 = wall.Proxy.get_first_point(wall)
            w2 = wall.Proxy.get_last_point(wall)
        elif idx == 1:
            w1 = wall.Proxy.get_last_point(wall)
            w2 = wall.Proxy.get_first_point(wall)

        t1 = target.Proxy.get_first_point(target)
        t2 = target.Proxy.get_last_point(target)

        angle = math.degrees(Draft.DraftVecUtils.angle(w2-w1,t2-t1))
        # print(angle)

        # identify if the function have to join the first or the end of the wall
        if idx == 0:
            wall.FirstCoreInnerAngle = angle
            wall.FirstCoreOuterAngle = -angle
        elif idx == 1:
            wall.LastCoreInnerAngle = -angle
            wall.LastCoreOuterAngle = angle

        if not wall.Name in target.IncomingTJoins:
            target_list = target.IncomingTJoins
            target_list.append(wall.Name)
            target.IncomingTJoins = target_list


    def L_join(self, wall, target, idx , target_idx):
        """ Compute given wall angles to corner join target wall,
            mind that when calling this method, the 2 walls already
            have a coincident end point (achieved by the extend method).

                      /    wall
                     /     /     / .
                    /     /     /   angle   
                   /     /_wi__/______.__________________________         
                  /     /    ./       .
                 /  ti /  c. /        .
                /     /  .  /ti       . target.Width        
               /     / .   /          .
              /____ /_____/_____ _____._____ _____ _____ _____ __
             /    ./  wi /                    target
            /   . /     /
           /  .  /     /
          / .w_angle  /
         /.____/_____/___________________________________________          

        """
        # TODO: correct the bug of two different size walls with big angle in between
        print("--------\n"+"L_Join "+wall.Name+"_"+str(idx) + " with " +target.Name+"_"+str(target_idx) + "\n")
        
        if idx == 0:
            w1 = wall.Proxy.get_first_point(wall)
            w2 = wall.Proxy.get_last_point(wall)
        elif idx == 1:
            w1 = wall.Proxy.get_last_point(wall)
            w2 = wall.Proxy.get_first_point(wall)

        if target_idx == 0:
            t1 = target.Proxy.get_first_point(target)
            t2 = target.Proxy.get_last_point(target)
        elif target_idx == 1:
            t1 = target.Proxy.get_last_point(target)
            t2 = target.Proxy.get_first_point(target)

        angle = Draft.DraftVecUtils.angle(w2-w1,t2-t1)

        # print("angle between walls: " + str(math.degrees(angle)) + "\n")

        if angle > 0:
            w_i = wall.Width * math.cos(math.pi/2-angle)
            t_i = target.Width * math.cos(math.pi/2-angle)
        if angle < 0:
            w_i = wall.Width * math.cos(-math.pi/2-angle)
            t_i = target.Width * math.cos(-math.pi/2-angle)

        c = math.sqrt( w_i**2 + t_i**2 - 2 * abs(w_i) * t_i * math.cos(math.pi-angle) )
        w_angle = math.asin( w_i / c * math.sin(math.pi-angle))
        
        # print("Parameters:\n")
        # print("w_i: " + str(w_i) + "\n")
        # print("c: " + str(c) + "\n")
        # print("flipping parameter: " + str(c) + "\n")    
        # print("cut angle: " + str(math.degrees(w_angle)) + "\n")

        # assign the angles to the correct wall end
        w_angle = math.degrees( w_angle )
        if idx == 0:
            wall.FirstCoreInnerAngle = w_angle
            wall.FirstCoreOuterAngle = -w_angle
        elif idx == 1:
            wall.LastCoreInnerAngle = -w_angle
            wall.LastCoreOuterAngle = +w_angle


    # Group objects handling methods ++++++++++++++++++++++++++++++++++++++++


    def add_window(self, obj, child):
        """
        This method is called when a new object is added to the wall and
        it has a IfcType property that is set to 'Window'.
        """
        pass
        # TODO: not implemented yet


    def add_subtraction(self, obj, child):
        """
        This method is called when a new object is added to the wall.
        It ask the user if the object has to be treated as an opening.
        If so, it add the object to the Subtractions PropertyLinkListChild.
        """
        msgBox = QtGui.QMessageBox()
        msgBox.setText("Object " + obj.Label + " has been added to the wall.")
        msgBox.setInformativeText("Do you want to treat it as an opening?\n")
        msgBox.setStandardButtons(QtGui.QMessageBox.Yes | QtGui.QMessageBox.No)
        msgBox.setDefaultButton(QtGui.QMessageBox.Yes)
        ret = msgBox.exec_()

        if ret == QtGui.QMessageBox.Yes:
            openings = obj.Subtractions
            openings.append(child)
            obj.Subtractions = openings
            child.Visibility = False
        elif ret == QtGui.QMessageBox.No:
            return


    # General getter methods ++++++++++++++++++++++++++++++++++++++++++++++++


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
        p1 = obj.getGlobalPlacement().multVec(App.Vector(obj.AxisFirstPointX,
                                                         0,
                                                         0))
        return p1

    def get_last_point(self, obj):
        """returns a part line representing the core axis of the wall"""
        p2 = obj.getGlobalPlacement().multVec(App.Vector(obj.AxisLastPointX,
                                                         0,
                                                         0))
        return p2


    # General setter methods ++++++++++++++++++++++++++++++++++++++++++++++++


    def set_first_point(self, obj, first_point, local=False):
        """returns a part line representing the core axis of the wall"""
        if first_point.x != obj.AxisLastPointX:
            self.set_point(obj, first_point, 0, local)
            return True
        else:
            print("You are trying to set the first point equal to the last point, this is not allowed.\n")
            return False

    def set_last_point(self, obj, last_point, local=False):
        """returns a part line representing the core axis of the wall"""
        if last_point.x != obj.AxisFirstPointX:
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
            obj.AxisFirstPointX = np.x
        elif point_idx == 1:
            obj.AxisLastPointX = np.x


    # Other methods +++++++++++++++++++++++++++++++++++++++++++++++++++++++++


    def flip_wall(self, obj):
        """
        describe
        """
        #TODO: To be implemented yet
        pass


    def onDocumentRestored(self, obj):
        self.Object = obj
        # obj.Proxy.Type needs to be re-setted every time the document is opened.
        obj.Proxy.Type = "Arch_Wall"


    def __getstate__(self):
        return


    def __setstate__(self,_state):
        return