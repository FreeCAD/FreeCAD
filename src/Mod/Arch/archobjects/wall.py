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


class Wall(object):
    """
    A prototype for a new wall object for the Arch Workbench
    """
    def __init__(self, obj=None):
        # print("runing wall object init method\n")
        self.Type = 'Arch_Wall'

        if obj:
            # print("runing obj init method")

            obj.Proxy = self
            self.Object = obj
            self.attach(obj)
            self.execute(obj)


    def init_properties(self, obj):
        obj.addProperty('App::PropertyString', 'Description', 
                        'Base', 
                        'Wall description').Description = ""

        # LEVEL Properties (not implemented yet)
        _tip = 'Constrain the wall base to the parent level'
        obj.addProperty('App::PropertyBool', 'BaseConstrain', 
                        'Level properties', 
                        _tip).BaseConstrain = True

        _tip = 'If the wall base is constrained to the parent level, set Z offset'
        obj.addProperty('App::PropertyLength', 'BaseOffset', 
                        'Level properties', 
                        _tip).BaseOffset = '0'

        _tip = 'Constrain the wall top to the upper level'
        obj.addProperty('App::PropertyBool', 'TopConstrain', 
                        'Level properties', 
                        _tip).TopConstrain = True

        _tip = 'If the wall top is constrained to the parent level, set Z offset'
        obj.addProperty('App::PropertyLength', 'TopOffset', 
                        'Level properties', 
                        _tip).TopOffset = '0'

        # CHILDREN Properties (partially implemented at the moment)
        obj.addProperty('App::PropertyLinkChild', 'BaseGeometry', 'Children', 'Link to the child representing the base geometry of the wall shape')
        obj.addProperty('App::PropertyLinkListChild', 'Openings', 'Children', 'Link to the children that will be subtracted from the wall shape')
        obj.addProperty('App::PropertyLinkListChild', 'Fusions', 'Children', 'Link to the children that will be fused with the wall shape')
        obj.addProperty('App::PropertyLinkListChild', 'Components', 'Children', 'Link to the structural children members of the wall')
        obj.addProperty('App::PropertyLinkListChild', 'Windows', 'Children', 'Link to the windows inserted into the wall ')

        # WALL ENDS Properties
        obj.addProperty('App::PropertyBool', 'JoinFirstEnd', 'Wall connections', "Allow automatic compute of first end").JoinFirstEnd = False
        obj.addProperty('App::PropertyString', 'JoinFirstEndTo', 'Wall connections', "Name of the object to join wall's first end").JoinFirstEndTo = ''
        obj.addProperty('App::PropertyBool', 'JoinLastEnd', 'Wall connections', "Allow automatic compute of last end").JoinLastEnd = False
        obj.addProperty('App::PropertyString', 'JoinLastEndTo', 'Wall connections', "Name of the object to join wall's last end").JoinLastEndTo = ''
        obj.addProperty('App::PropertyStringList', 'IncomingTJoins', 'Wall connections', "Names of the objects that target current wall").IncomingTJoins = []


    def attach(self,obj):

        # print("running" + obj.Name + "attach() method\n")
        obj.addExtension('App::OriginGroupExtensionPython', None)
        obj.Origin = App.ActiveDocument.addObject('App::Origin','Origin')
        self.init_properties(obj)


    def execute(self,obj):
        """ Compute the wall shape as boolean operations among the children objects """
        # print("running " + obj.Name + " execute() method\n")

        wall_shape = None

        # get wall base shape from BaseGeometry object
        if hasattr(obj, "BaseGeometry"):
            if obj.BaseGeometry:
                if hasattr(obj.BaseGeometry, "Shape"):
                    wall_shape = obj.BaseGeometry.Shape

        if wall_shape is None:
            return

        # subtract openings
        if hasattr(obj, "Openings"):
            if obj.Openings is not None and obj.Openings != []:
                for o in obj.Openings:
                    if hasattr(obj, "Shape"):
                        relative_placement = o.Placement
                        if hasattr(o, "InList"):
                            if o.InList[0] != obj:
                                relative_placement = o.InList[0].Placement.multiply(o.Placement)
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

                _compound = [wall_shape]

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
                wall_shape = Part.Compound(_compound)

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


    def onChanged(self, obj, prop):
        """this method is activated when a property changes"""
        if prop == "Placement":
            if hasattr(obj, "Placement"):
                # Recompute wall joinings
                self.recompute_ends(obj, 0)
                self.recompute_ends(obj, 1)
                for t_name in obj.IncomingTJoins:
                    t = App.ActiveDocument.getObject(t_name)
                    t.Proxy.recompute_ends(t, 0)
                    t.Proxy.recompute_ends(t, 1)
                    
        # WALL JOIN ENDS properties
        if (hasattr(obj, "JoinFirstEndTo") and hasattr(obj, "JoinLastEndTo") and
            hasattr(obj, "JoinFirstEnd")and hasattr(obj, "JoinLastEnd")):

            if prop == "JoinFirstEndTo" and obj.JoinFirstEnd:
                self.recompute_ends(obj, 0)

            elif prop == "JoinLastEndTo" and obj.JoinLastEnd:
                self.recompute_ends(obj, 1)

        # CHILDREN properties: remember to first assign basegeometry and then add the object to the group
        if prop == "BaseGeometry":
            if hasattr(obj, "BaseGeometry"):
                self.format_base_geometry_object(obj, obj.BaseGeometry)

        # Group property: an object is added or removed from the wall
        if prop == "Group":
            if hasattr(obj, "Group") and hasattr(obj, "BaseGeometry") and hasattr(obj, "Openings"):
                if hasattr(self, "oldGroup"):
                    # understand if the object was added or removed
                    added_objs = [x for x in obj.Group if x not in self.oldGroup]
                    removed_objs = [x for x in self.oldGroup if x not in obj.Group]
                    del self.oldGroup
                for o in removed_objs:
                    # if it was removed, remove it from wall children linking
                    print("Removing " + o.Label + " from " + obj.Label)
                    if o == obj.BaseGeometry:
                        obj.Base = None

                    elif o in obj.Openings:
                        openings = obj.Openings
                        openings.remove(o)
                        obj.Openings = openings

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

                    if not o in obj.Openings:
                        print("added a new object to the wall")
                        self.add_opening(obj, o)


    def add_window(self, obj, child):
        """
        This method is called when a new object is added to the wall and
        it has a IfcType property that is set to 'Window'.
        """
        pass
        # TODO: not implemented yet


    def add_opening(self, obj, child):
        """
        This method is called when a new object is added to the wall.
        It ask the user if the object has to be treated as an opening.
        If so, it add the object to the Openings PropertyLinkListChild.
        """
        msgBox = QtGui.QMessageBox()
        msgBox.setText("Object " + obj.Label + " has been added to the wall.")
        msgBox.setInformativeText("Do you want to treat it as an opening?\n")
        msgBox.setStandardButtons(QtGui.QMessageBox.Yes | QtGui.QMessageBox.No)
        msgBox.setDefaultButton(QtGui.QMessageBox.Yes)
        ret = msgBox.exec_()

        if ret == QtGui.QMessageBox.Yes:
            openings = obj.Openings
            openings.append(child)
            obj.Openings = openings
        elif ret == QtGui.QMessageBox.No:
            return


    # FOLLOWING METHODS Concern computation of wall joinings


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
        if obj.JoinFirstEndTo == obj.JoinLastEndTo and obj.JoinFirstEndTo != "":
            print("The wall cannot target the same wall on both JoinFirst and JoinLast properties")
            return
        if end_idx == 0:
            target = App.ActiveDocument.getObject(obj.JoinFirstEndTo)
            if target == obj:
                return
            if self.is_wall_joinable(obj):
                if self.is_wall_joinable(target):
                    self.join_end(obj, target, 0)
                else:
                    self.reset_end(obj, 0)

        if end_idx == 1:
            target = App.ActiveDocument.getObject(obj.JoinLastEndTo)
            if target == obj:
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
            #print("Wall Joining only works on valid Arch_Wall objects")
            return False
        if Draft.get_type(obj.BaseGeometry) != 'Arch_WallSegment':
            #print("Wall Joining only works if Wall base geometry is an Arch_WallSegment object")
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
        # print("running reset_end() "+obj.Name+"_"+str(idx)+"\n")
        if idx == 0:
            obj.BaseGeometry.FirstCoreInnerAngle = 90
            obj.BaseGeometry.FirstCoreOuterAngle = 90
        elif idx == 1:
            obj.BaseGeometry.LastCoreInnerAngle = 90
            obj.BaseGeometry.LastCoreOuterAngle = 90


    def remove_linked_walls_references(self, obj):
        """ 
        Removes the reference to given wall to all the other 
        walls that target it to join
        """
        # print("REMOVE ALL REFERENCES on DELETING")
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
        # print("--------\n"+"Extend "+wall.Name + " to " +target.Name+ "\n")

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
            wall.BaseGeometry.Proxy.set_first_point(wall.BaseGeometry, intersection)
            return True
        elif idx == 1:
            wall.BaseGeometry.Proxy.set_last_point(wall.BaseGeometry, intersection)
            return True


    def T_join(self, wall, target, idx): 
        """ Compute wall angles according to given parameters """
        # print("--------\n"+"T_Join "+wall.Name + " with " +target.Name+ "\n")

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
            wall.BaseGeometry.FirstCoreInnerAngle = angle
            wall.BaseGeometry.FirstCoreOuterAngle = -angle
        elif idx == 1:
            wall.BaseGeometry.LastCoreInnerAngle = -angle
            wall.BaseGeometry.LastCoreOuterAngle = angle

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
                /     /  .  /ti       . target.BaseGeometry.Width        
               /     / .   /          .
              /____ /_____/_____ _____._____ _____ _____ _____ __
             /    ./  wi /                    target
            /   . /     /
           /  .  /     /
          / .w_angle  /
         /.____/_____/___________________________________________          

        """
        # TODO: correct the bug of two different size walls with big angle in between
        # print("--------\n"+"L_Join "+wall.Name+"_"+str(idx) + " with " +target.Name+"_"+str(target_idx) + "\n")
        
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
            w_i = wall.BaseGeometry.Width * math.cos(math.pi/2-angle)
            t_i = target.BaseGeometry.Width * math.cos(math.pi/2-angle)
        if angle < 0:
            w_i = wall.BaseGeometry.Width * math.cos(-math.pi/2-angle)
            t_i = target.BaseGeometry.Width * math.cos(-math.pi/2-angle)

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
            wall.BaseGeometry.FirstCoreInnerAngle = w_angle
            wall.BaseGeometry.FirstCoreOuterAngle = -w_angle
        elif idx == 1:
            wall.BaseGeometry.LastCoreInnerAngle = -w_angle
            wall.BaseGeometry.LastCoreOuterAngle = +w_angle


    # FOLLOWING METHODS Are general getter functions


    def get_core_axis(self, obj):
        """ returns a part line representing the core axis of the wall """
        return obj.BaseGeometry.Proxy.get_core_axis(obj.BaseGeometry)


    def get_first_point(self, obj):
        """returns a part line representing the core axis of the wall"""
        return obj.BaseGeometry.Proxy.get_first_point(obj.BaseGeometry)


    def get_last_point(self, obj):
        """returns a part line representing the core axis of the wall"""
        return obj.BaseGeometry.Proxy.get_last_point(obj.BaseGeometry)


    # FOLLOWING METHODS Are other tools


    def flip_wall(self, obj):
        """
        describe
        """
        #TODO: verify if it's needed to swap FirstEndJoinTo and LastEndJoinTo
        #TODO: check what happens if the base geometry is far from origin
        obj.Placement.Rotation.Angle += math.pi
        obj.JoinFirstEndTo, obj.JoinLastEndTo = obj.JoinLastEndTo, obj.JoinFirstEndTo


    def format_base_geometry_object(self, obj, base_geometry):
        """
        this method is called when a BaseGeometry object is assigned to the wall
        """
        if Draft.get_type(base_geometry) == 'Arch_WallSegment':
            # if base_geometry is default Arch_WallSegment object, 
            # enable auto computing of ends joints
            obj.JoinFirstEnd = True
            obj.JoinLastEnd = True
        else:
            # else disable it
            obj.JoinFirstEnd = False
            obj.JoinLastEnd = False


        if hasattr(base_geometry, "ViewObject"):
            # format given object to wall base geometry visual settings
            if hasattr(base_geometry.ViewObject, "Transparency"):
                base_geometry.ViewObject.Transparency = 90
            if hasattr(base_geometry.ViewObject, "DrawStyle"):
                base_geometry.ViewObject.DrawStyle = "Dashed"
            if hasattr(base_geometry.ViewObject, "LineWidth"):
                base_geometry.ViewObject.LineWidth = 1


    def onDocumentRestored(self, obj):
        self.Object = obj


    def __getstate__(self):
        return


    def __setstate__(self,_state):
        return