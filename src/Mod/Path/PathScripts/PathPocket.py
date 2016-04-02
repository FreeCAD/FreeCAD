# -*- coding: utf-8 -*-

#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
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

import FreeCAD,Path
from PySide import QtCore,QtGui
from PathScripts import PathUtils
import math

FreeCADGui = None
if FreeCAD.GuiUp:
    import FreeCADGui

"""Path Pocket object and FreeCAD command"""

# Qt tanslation handling
try:
    _encoding = QtGui.QApplication.UnicodeUTF8
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig)


class ObjectPocket:
    

    def __init__(self,obj):
        obj.addProperty("App::PropertyLinkSubList","Base","Path",translate("PathProject","The base geometry of this object"))
        obj.addProperty("App::PropertyBool","Active","Path",translate("PathProject","Make False, to prevent operation from generating code"))
        obj.addProperty("App::PropertyString","Comment","Path",translate("PathProject","An optional comment for this profile"))
        obj.addProperty("App::PropertyEnumeration", "Algorithm", "Algorithm",translate("PathProject", "The library to use to generate the path"))
        obj.Algorithm = ['OCC Native','libarea']

        #Tool Properties
        obj.addProperty("App::PropertyIntegerConstraint","ToolNumber","Tool",translate("PathProfile","The tool number in use"))
        obj.ToolNumber = (0, 0, 1000, 0)
        obj.setEditorMode('ToolNumber',1) #make this read only

        #Depth Properties
        obj.addProperty("App::PropertyDistance", "ClearanceHeight", "Depth", translate("PathProject","The height needed to clear clamps and obstructions"))
        obj.addProperty("App::PropertyDistance", "SafeHeight", "Depth", translate("PathProject","Rapid Safety Height between locations."))
        obj.addProperty("App::PropertyFloatConstraint", "StepDown", "Depth", translate("PathProject","Incremental Step Down of Tool"))
        obj.StepDown = (0.0, 0.01, 100.0, 0.5)
        obj.addProperty("App::PropertyDistance", "StartDepth", "Depth", translate("PathProject","Starting Depth of Tool- first cut depth in Z"))
        obj.addProperty("App::PropertyDistance", "FinalDepth", "Depth", translate("PathProject","Final Depth of Tool- lowest value in Z"))
        obj.addProperty("App::PropertyDistance", "FinishDepth", "Depth", translate("PathProject","Maximum material removed on final pass."))

        #Pocket Properties
        obj.addProperty("App::PropertyEnumeration", "CutMode", "Pocket",translate("PathProject", "The direction that the toolpath should go around the part ClockWise CW or CounterClockWise CCW"))
        obj.CutMode = ['Climb','Conventional']
        obj.addProperty("App::PropertyDistance", "MaterialAllowance", "Pocket", translate("PathProject","Amount of material to leave"))
        obj.addProperty("App::PropertyEnumeration", "StartAt", "Pocket",translate("PathProject", "Start pocketing at center or boundary"))
        obj.StartAt = ['Center', 'Edge']
        obj.addProperty("App::PropertyFloatConstraint","StepOver","Pocket",translate("PathProject","Amount to step over on each pass"))
        obj.StepOver = (0.0, 0.01, 100.0, 0.5)
        obj.addProperty("App::PropertyBool","KeepToolDown","Pocket",translate("PathProject","Attempts to avoid unnecessary retractions."))
        obj.addProperty("App::PropertyBool","ZigUnidirectional","Pocket",translate("PathProject","Lifts tool at the end of each pass to respect cut mode."))
        obj.addProperty("App::PropertyBool","UseZigZag","Pocket",translate("PathProject","Use Zig Zag pattern to clear area."))
        obj.addProperty("App::PropertyFloat","ZigZagAngle","Pocket",translate("PathProject","Angle of the zigzag pattern"))

        #Entry Properties
        obj.addProperty("App::PropertyBool","UseEntry","Entry",translate("PathProject","Allow Cutter enter material with a straight plunge."))
        obj.addProperty("App::PropertyFloatConstraint", "RampSize", "Entry", translate("PathProject","The minimum fraction of tool diameter to use for ramp length"))
        obj.RampSize = (0.0, 0.01, 100.0, 0.5)
        obj.addProperty("App::PropertyFloatConstraint", "HelixSize", "Entry", translate("PathProject","The fraction of tool diameter to use for calculating helix size."))
        obj.HelixSize = (0.0, 0.01, 100.0, 0.5)
        obj.addProperty("App::PropertyFloatConstraint", "RampAngle", "Entry", translate("PathProject","The Angle of the ramp entry."))
        obj.RampAngle = (0.0, 0.01, 100.0, 0.5)

        #Start Point Properties
        obj.addProperty("App::PropertyVector","StartPoint","Start Point",translate("PathProject","The start point of this path"))
        obj.addProperty("App::PropertyBool","UseStartPoint","Start Point",translate("PathProject","make True, if specifying a Start Point")) 

        obj.Proxy = self

    def onChanged(self,obj,prop):
        if prop == "UseEntry":
            if obj.UseEntry:
                obj.setEditorMode('HelixSize',0) #make this visible
                obj.setEditorMode('RampAngle',0) #make this visible
                obj.setEditorMode('RampSize',0) #make this visible
            else:
                obj.setEditorMode('HelixSize',2) #make this hidden
                obj.setEditorMode('RampAngle',2) #make this hidden
                obj.setEditorMode('RampSize',2) #make this hidden

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None

    def addpocketbase(self, obj, ss, sub=""):
        #sublist = []
        #sublist.append(sub)
        baselist = obj.Base
        if baselist == None:
            baselist = []
        item = (ss, sub)
        if item in baselist:
            FreeCAD.Console.PrintWarning("this object already in the list"+ "\n")
        else:
            baselist.append (item)
        obj.Base = baselist
        print "this base is: " + str(baselist) 
        self.execute(obj)        

    def getStock(self,obj):
        "retrieves a stock object from hosting project if any"
        for o in obj.InList:
            if hasattr(o,"Group"):
                for g in o.Group:
                    if hasattr(g,"Height_Allowance"):
                        return o
        # not found? search one level up
        for o in obj.InList:
            return self.getStock(o)
        return None

    def buildpathlibarea(self, obj, a):
        import PathScripts.PathUtils as PathUtils
        import PathScripts.PathAreaUtils as PathAreaUtils
        from PathScripts.PathUtils import depth_params
        import area
 
        FreeCAD.Console.PrintMessage(translate("PathPocket","Generating toolpath with libarea offsets.\n"))
        
        depthparams = depth_params (obj.ClearanceHeight.Value, obj.SafeHeight.Value, obj.StartDepth.Value, obj.StepDown, obj.FinishDepth.Value, obj.FinalDepth.Value)
        
        horizfeed = self.horizFeed
        extraoffset = obj.MaterialAllowance.Value
        stepover = obj.StepOver
        use_zig_zag = obj.UseZigZag
        zig_angle = obj.ZigZagAngle
        from_center = (obj.StartAt == "Center")
        keep_tool_down = obj.KeepToolDown
        zig_unidirectional = obj.ZigUnidirectional
        start_point = None
        cut_mode = obj.CutMode
        
        PathAreaUtils.flush_nc()
        PathAreaUtils.output('mem')
        PathAreaUtils.feedrate_hv(self.horizFeed, self.vertFeed)

        if obj.UseStartPoint:
            start_point = (obj.StartPoint.x,obj.StartPoint.y)


        #print "a," + str(self.radius) + "," + str(extraoffset) + "," + str(stepover) + ",depthparams, " + str(from_center) + "," + str(keep_tool_down) + "," + str(use_zig_zag) + "," + str(zig_angle) + "," + str(zig_unidirectional) + "," + str(start_point) + "," + str(cut_mode)

        PathAreaUtils.pocket(a,\
                self.radius,\
                extraoffset, \
                stepover,\
                depthparams,\
                from_center,\
                keep_tool_down,\
                use_zig_zag,\
                zig_angle,\
                zig_unidirectional,\
                start_point,\
                cut_mode)

        return PathAreaUtils.retrieve_gcode()


    def buildpathocc(self, obj, shape):
        import Part, DraftGeomUtils
        FreeCAD.Console.PrintMessage(translate("PathPocket","Generating toolpath with OCC native offsets.\n"))

        def prnt(vlu): return str("%.4f" % round(vlu, 4))

        def rapid(x=None, y=None, z=None):
        #Returns gcode to perform a rapid move
            retstr = "G00"
            if (x != None) or (y != None) or (z != None):
                if (x != None):
                    retstr += " X" + str("%.4f" % x)
                if (y != None):
                    retstr += " Y" + str("%.4f" % y)
                if (z != None):
                    retstr += " Z" + str("%.4f" % z)
            else:
                return ""
            return retstr + "\n"

        def feed(x=None, y=None, z=None):
        #Returns gcode to perform a linear feed
            global feedxy
            retstr = "G01 F"
            if(x == None) and (y == None):
                retstr += str("%.4f" % self.horizFeed)
            else:
                retstr += str("%.4f" % self.vertFeed)
            
            if (x != None) or (y != None) or (z != None):
                if (x != None):
                    retstr += " X" + str("%.4f" % x)
                if (y != None):
                    retstr += " Y" + str("%.4f" % y)
                if (z != None):
                    retstr += " Z" + str("%.4f" % z)
            else:
                return ""
            return retstr + "\n"

        def arc(cx, cy, sx, sy, ex, ey, ez=None, ccw=False):
        #Returns gcode to perform an arc
        #Assumes XY plane or helix around Z
        #Don't worry about starting Z- assume that's dealt with elsewhere
            #If start/end radii aren't within eps, abort
            eps = 0.01
            if (math.sqrt((cx - sx)**2 + (cy - sy)**2) - math.sqrt((cx - ex)**2 + (cy - ey)**2)) >= eps:
                print "ERROR: Illegal arc: Stand and end radii not equal"
                return ""
            
            #Set [C]CW and feed
            retstr = ""
            if ccw:
                retstr += "G03 F"
            else:
                retstr += "G02 F"
            retstr += str(self.horizFeed)
            
            #End location
            retstr += " X" + str("%.4f" % ex) + " Y" + str("%.4f" % ey)
            
            #Helix if requested
            if ez != None:
                retstr += " Z" + str("%.4f" % ez)
            
            #Append center offsets
            retstr += " I" + str("%.4f" % (cx - sx)) + " J" + str("%.4f" % (cy - sy))
            
            return retstr + "\n"

        def helicalPlunge(plungePos, rampangle, destZ, startZ):
        #Returns gcode to helically plunge
        #destZ is the milling level
        #startZ is the height we can safely feed down to before helix-ing
            helixCmds = "(START HELICAL PLUNGE)\n"
            if(plungePos == None):
                raise Error("Helical plunging requires a position!")
                return None
            if(not tool):
                raise Error("Helical plunging requires a tool!")
                return None

            helixX = plungePos.x + tool.Diameter/2. * plungeR
            helixY = plungePos.y;
            
            helixCirc = math.pi * tool.Diameter * plungeR
            dzPerRev = math.sin(rampangle/180. * math.pi) * helixCirc

            #Go to the start of the helix position
            helixCmds += rapid(helixX, helixY)
            helixCmds += rapid(z=startZ)
            
            #Helix as required to get to the requested depth
            lastZ = startZ
            curZ = max(startZ-dzPerRev, destZ)
            done = False
            while not done:
                done = (curZ == destZ)
                #NOTE: FreeCAD doesn't render this, but at least LinuxCNC considers it valid
                #helixCmds += arc(plungePos.x, plungePos.y, helixX, helixY, helixX, helixY, ez = curZ, ccw=True)
                
                #Use two half-helixes; FreeCAD renders that correctly,
                #and it fits with the other code breaking up 360-degree arcs
                helixCmds += arc(plungePos.x, plungePos.y, helixX, helixY, helixX - tool.Diameter * plungeR, helixY, ez = (curZ + lastZ)/2., ccw=True)
                helixCmds += arc(plungePos.x, plungePos.y, helixX - tool.Diameter * plungeR, helixY, helixX, helixY, ez = curZ, ccw=True)
                lastZ = curZ
                curZ = max(curZ - dzPerRev, destZ)
            
            return helixCmds

        def rampPlunge(edge, rampangle, destZ, startZ):
        #Returns commands to linearly ramp into a cut
        #FIXME: This ramps along the first edge, assuming it's long
        #enough, NOT just wiggling back and forth by ~0.75 * toolD.
        #Not sure if that's any worse, but it's simpler
        # I think this should be changed to be limited to a maximum ramp size.  Otherwise machine time will get longer than it needs to be.


            rampCmds = "(START RAMP PLUNGE)\n"
            if(edge == None):
                raise Error("Ramp plunging requires an edge!")
                return None
            if(not tool):
                raise Error("Ramp plunging requires a tool!")
            
            
            sPoint = edge.Vertexes[0].Point
            ePoint = edge.Vertexes[1].Point
            #Evidently edges can get flipped- pick the right one in this case
            #FIXME: This is iffy code, based on what already existed in the "for vpos ..." loop below
            if ePoint == sPoint:
                #print "FLIP"
                ePoint = edge.Vertexes[-1].Point
            #print "Start: " + str(sPoint) + " End: " + str(ePoint) + " Zhigh: " + prnt(startZ) + " ZLow: " + prnt(destZ)
            
            rampDist = edge.Length
            rampDZ = math.sin(rampangle/180. * math.pi) * rampDist
            
            rampCmds += rapid(sPoint.x, sPoint.y)
            rampCmds += rapid(z=startZ)
            
            #Ramp down to the requested depth
            #FIXME: This might be an arc, so handle that as well
            lastZ = startZ
            curZ = max(startZ-rampDZ, destZ)
            done = False
            while not done:
                done = (curZ == destZ)
                
                #If it's an arc, handle it!
                if isinstance(edge.Curve,Part.Circle):
                    raise Error("rampPlunge: Screw it, not handling an arc.")
                #Straight feed! Easy!
                else:
                    rampCmds += feed(ePoint.x, ePoint.y, curZ)
                    rampCmds += feed(sPoint.x, sPoint.y)

                lastZ = curZ
                curZ = max(curZ - rampDZ, destZ)
            
            return rampCmds
        


        output = ""
        offsets = []
        nextradius = self.radius
        result = DraftGeomUtils.pocket2d(shape,nextradius)

        while result:
            #print "Adding " + str(len(result)) + " wires"
            offsets.extend(result)
            nextradius += self.radius
            result = DraftGeomUtils.pocket2d(shape,nextradius)
        
        # first move will be rapid, subsequent will be at feed rate
        first = True
        startPoint = None
        fastZPos = max(obj.StartDepth.Value + 2, obj.ClearanceHeight.Value)
        
        # revert the list so we start with the outer wires
        if obj.StartAt != 'Edge':
            offsets.reverse()

#            print "startDepth: " + str(obj.StartDepth.Value)
#            print "finalDepth: " + str(obj.FinalDepth.Value)
#            print "stepDown: " + str(obj.StepDown)
#            print "finishDepth" + str(obj.FinishDepth.Value)
#            print "offsets:", len(offsets)

        #Fraction of tool radius our plunge helix is to be
        plungeR = obj.HelixSize
        
        #(minimum) Fraction of tool DIAMETER to go back and forth while ramp-plunging
        #FIXME: The ramp plunging should maybe even be limited to this distance; I don't know what's best
        rampD = obj.RampSize

        #Total offset from the desired pocket edge is tool radius plus the plunge helix radius
        #Any point on these curves could be the center of a plunge
        helixBounds = DraftGeomUtils.pocket2d(shape, self.radius * (1 + plungeR))
        
        
        #Try to find a location to nicely plunge, starting with a helix, then ramp
        #Can't do it without knowledge of a tool
        plungePos = None
        rampEdge = None
        tool = PathUtils.getTool(obj,obj.ToolNumber)
        if not tool:
                raise Error("Ramp plunge location-finding requires a tool")
                return
        else:
            #Since we're going to start machining either the inner-most
            #edge or the outer (depending on StartAt setting), try to
            #plunge near that location
            
            if helixBounds and obj.UseEntry:
                #Edge is easy- pick a point on helixBounds and go with it
                if obj.StartAt == 'Edge':
                    plungePos = helixBounds[0].Edges[0].Vertexes[0].Point
                #Center is harder- use a point from the first offset, check if it works
                else:
                    plungePos = offsets[0].Edges[0].Vertexes[0].Point
                    
                    #If it turns out this is invalid for some reason, nuke plungePos
                    [perp,idx] = DraftGeomUtils.findPerpendicular(plungePos, shape.Edges)
                    if not perp or perp.Length < self.radius * (1 + plungeR):
                        plungePos = None
                    #FIXME: Really need to do a point-in-polygon operation to make sure this is within helixBounds
                    #Or some math to prove that it has to be (doubt that's true)
                    #Maybe reverse helixBounds and pick off that?
            
            
            #If we didn't find a place to helix, how about a ramp?
            if not plungePos and obj.UseEntry:
                #Check first edge of our offsets
                if (offsets[0].Edges[0].Length >= tool.Diameter * rampD) and not (isinstance(offsets[0].Edges[0].Curve, Part.Circle)):
                    rampEdge = offsets[0].Edges[0]
                #The last edge also connects with the starting location- try that
                elif (offsets[0].Edges[-1].Length >= tool.Diameter * rampD) and not (isinstance(offsets[0].Edges[-1].Curve, Part.Circle)):
                    rampEdge = offsets[0].Edges[-1]
                else:
                    print "Neither edge works: " + str(offsets[0].Edges[0]) + ", " + str(offsets[0].Edges[-1])
                #FIXME: There's got to be a smarter way to find a place to ramp
            

        #For helix-ing/ramping, know where we were last time
        #FIXME: Can probably get this from the "machine"?
        lastZ = fastZPos

        for vpos in PathUtils.frange(obj.StartDepth.Value, obj.FinalDepth.Value, obj.StepDown, obj.FinishDepth.Value):
            #Every for every depth we should helix down
            first = True
            # loop over successive wires
            for currentWire in offsets:
                last = None
                for edge in currentWire.Edges:
                    if not last:
                        # we set the base GO to our fast move to our starting pos
                        if first:
                            #If we can helix, do so
                            if plungePos:
                                output += helicalPlunge(plungePos, obj.RampAngle, vpos, lastZ)
                                #print output
                                lastZ = vpos
                            #Otherwise, see if we can ramp
                            #FIXME: This could be a LOT smarter (eg, searching for a longer leg of the edge to ramp along)
                            elif rampEdge:
                                output += rampPlunge(rampEdge, obj.RampAngle, vpos, lastZ)
                                lastZ = vpos
                            #Otherwise, straight plunge... Don't want to, but sometimes you might not have a choice.
                            #FIXME: At least not with the lazy ramp programming above...
                            else:
                                print "WARNING: Straight-plunging... probably not good, but we didn't find a place to helix or ramp"
                                startPoint = edge.Vertexes[0].Point
                                output += "G0 X" + prnt(startPoint.x) + " Y" + prnt(startPoint.y) +\
                                          " Z" + prnt(fastZPos) + "\n"
                            first = False
                        #then move slow down to our starting point for our profile
                        last = edge.Vertexes[0].Point
                        output += "G1 X" + prnt(last.x) + " Y" + prnt(last.y) + " Z" + prnt(vpos) + "\n"
                    #if isinstance(edge.Curve,Part.Circle):
                    if DraftGeomUtils.geomType(edge) == "Circle":
                        point = edge.Vertexes[-1].Point
                        if point == last: # edges can come flipped
                            point = edge.Vertexes[0].Point
                        center = edge.Curve.Center
                        relcenter = center.sub(last)
                        v1 = last.sub(center)
                        v2 = point.sub(center)
                        if v1.cross(v2).z < 0:
                            output += "G2"
                        else:
                            output += "G3"
                        output += " X" + prnt(point.x) + " Y" + prnt(point.y) + " Z" + prnt(vpos)
                        output += " I" + prnt(relcenter.x) + " J" +prnt(relcenter.y) + " K" + prnt(relcenter.z)
                        output += "\n"
                        last = point
                    else:
                        point = edge.Vertexes[-1].Point
                        if point == last: # edges can come flipped
                            point = edge.Vertexes[0].Point
                        output += "G1 X" + prnt(point.x) + " Y" + prnt(point.y) + " Z" + prnt(vpos) + "\n"
                        last = point

        #move back up
        output += "G1 Z" + prnt(fastZPos) + "\n"
        return output


    #To reload this from FreeCAD, use: import PathScripts.PathPocket; reload(PathScripts.PathPocket)
    def execute(self,obj):
        output = ""
        toolLoad = PathUtils.getLastToolLoad(obj)
        if toolLoad == None:
            self.vertFeed = 100
            self.horizFeed = 100
            self.radius = 0.25
            obj.ToolNumber = 0
        else:
            self.vertFeed = toolLoad.VertFeed.Value
            self.horizFeed = toolLoad.HorizFeed.Value
            tool = PathUtils.getTool(obj, toolLoad.ToolNumber)
            self.radius = tool.Diameter/2
            obj.ToolNumber= toolLoad.ToolNumber   


        if obj.Base:
            for b in obj.Base:
                print "object base: " + str(b)
                import Part, PathScripts.PathKurveUtils, DraftGeomUtils
                if "Face" in b[1]:
                    print "inside"
                    shape = getattr(b[0].Shape,b[1])
                    wire = shape.OuterWire
                    edges = wire.Edges
                else:
                    print "in else"
                    edges = [getattr(b[0].Shape,sub) for sub in b[1]]
                    print "myedges: " + str(edges)
                    wire = Part.Wire(edges)
                    shape = None

                        
                # output = ""
                if obj.Algorithm == "OCC Native":
                    if shape == None:
                        shape = wire
                    output += self.buildpathocc(obj, shape)
                else:
                    try:
                        import area
                    except:
                        FreeCAD.Console.PrintError(translate("PathKurve","libarea needs to be installed for this command to work.\n"))
                        return
                    
                    a = area.Area()
                    if shape == None:
                        c = PathScripts.PathKurveUtils.makeAreaCurve(wire.Edges, 'CW')
                        a.append(c)
                    else:
                        for w in shape.Wires:
                            c = PathScripts.PathKurveUtils.makeAreaCurve(w.Edges, 'CW')
                            # if w.isSame(shape.OuterWire):
                            #     print "outerwire"
                            #     if  c.IsClockwise():
                            #         c.Reverse()
                            #         print "reverse outterwire"
                            # else:
                            #     print "inner wire"
                            #     if not c.IsClockwise():
                            #         c.Reverse()
                            #         print "reverse inner"
                            a.append(c)

                    ########
                    ##This puts out some interesting information from libarea
                    print a.text()
                    ########
                    
                    
                    a.Reorder()
                    output += self.buildpathlibarea(obj, a)


            if obj.Active:
                path = Path.Path(output)
                obj.Path = path
                obj.ViewObject.Visibility = True
            else:
                path = Path.Path("(inactive operation)")
                obj.Path = path
                obj.ViewObject.Visibility = False

class _CommandSetPocketStartPoint:
    def GetResources(self):
        return {'Pixmap'  : 'Path-StartPoint',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("PathPocket","Pick Start Point"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("PathPocket","Pick Start Point")}

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None
    
    def setpoint(self,point,o):
        obj=FreeCADGui.Selection.getSelection()[0]
        obj.StartPoint.x = point.x
        obj.StartPoint.y = point.y
    def Activated(self):

        FreeCADGui.Snapper.getPoint(callback=self.setpoint)

class ViewProviderPocket:

    def __init__(self,vobj):
        vobj.Proxy = self

    def attach(self,vobj):
        self.Object = vobj.Object
        return

    def setEdit(self,vobj,mode=0):
        FreeCADGui.Control.closeDialog()
        taskd = TaskPanel()
        taskd.obj = vobj.Object
        FreeCADGui.Control.showDialog(taskd)
        taskd.setupUi()
        return True

    def getIcon(self):
        return ":/icons/Path-Pocket.svg"

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None


class CommandPathPocket:


    def GetResources(self):
        return {'Pixmap'  : 'Path-Pocket',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("PathPocket","Pocket"),
                'Accel': "P, O",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("PathPocket","Creates a Path Pocket object from a loop of edges or a face")}

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None

    def Activated(self):
        
        # check that the selection contains exactly what we want
        # selection = FreeCADGui.Selection.getSelectionEx()
        # if len(selection) != 1:
        #     FreeCAD.Console.PrintError(translate("PathPocket","Please select an edges loop from one object, or a single face\n"))
        #     return
        # if len(selection[0].SubObjects) == 0:
        #     FreeCAD.Console.PrintError(translate("PathPocket","Please select an edges loop from one object, or a single face\n"))
        #     return
        # for s in selection[0].SubObjects:
        #     if s.ShapeType != "Edge":
        #         if (s.ShapeType != "Face") or (len(selection[0].SubObjects) != 1):
        #             FreeCAD.Console.PrintError(translate("PathPocket","Please select only edges or a single face\n"))
        #             return
        # if selection[0].SubObjects[0].ShapeType == "Edge":
        #     try:
        #         import Part
        #         w = Part.Wire(selection[0].SubObjects)
        #         if w.isClosed() == False:
        #             FreeCAD.Console.PrintError(translate("PathPocket","The selected edges don't form a loop\n"))
        #             return

        #     except:
        #         FreeCAD.Console.PrintError(translate("PathPocket","The selected edges don't form a loop\n"))
        #         return

        # Take a guess at some reasonable values for Finish depth.
        # bb = selection[0].Object.Shape.BoundBox  #parent boundbox
        # fbb = selection[0].SubObjects[0].BoundBox #feature boundbox
        # if fbb.ZMax < bb.ZMax:
        #     zbottom = fbb.ZMax
        # else:
        #     zbottom = bb.ZMin

        zbottom = 0.0
        ztop = 10.0
        
        # if everything is ok, execute and register the transaction in the undo/redo stack
        FreeCAD.ActiveDocument.openTransaction(translate("PathPocket","Create Pocket"))
        FreeCADGui.addModule("PathScripts.PathPocket")
        
        FreeCADGui.doCommand('obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython","Pocket")') 
        FreeCADGui.doCommand('PathScripts.PathPocket.ObjectPocket(obj)')

        FreeCADGui.doCommand('obj.Active = True')

        FreeCADGui.doCommand('PathScripts.PathPocket.ViewProviderPocket(obj.ViewObject)')
        
        FreeCADGui.doCommand('from PathScripts import PathUtils')

        FreeCADGui.doCommand('obj.StepOver = 1.0')
        FreeCADGui.doCommand('obj.ClearanceHeight = 10')# + str(bb.ZMax + 2.0))
        FreeCADGui.doCommand('obj.StepDown = 1.0')
        FreeCADGui.doCommand('obj.StartDepth= ' + str(ztop))
        FreeCADGui.doCommand('obj.FinalDepth=' + str(zbottom))
        FreeCADGui.doCommand('obj.ZigZagAngle=45')
        FreeCADGui.doCommand('obj.UseEntry=True')
        FreeCADGui.doCommand('obj.RampAngle = 3.0')
        FreeCADGui.doCommand('obj.RampSize = 0.75')
        FreeCADGui.doCommand('obj.HelixSize = 0.75')

        FreeCADGui.doCommand('PathScripts.PathUtils.addToProject(obj)')
    
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.doCommand('obj.ViewObject.startEditing()')

class TaskPanel:
    def __init__(self):
        self.form = FreeCADGui.PySideUic.loadUi(FreeCAD.getHomePath() + "Mod/Path/PocketEdit.ui")
        #self.form = FreeCADGui.PySideUic.loadUi(":/PocketEdit.ui")
        self.updating = False


    def accept(self):
        self.getFields()

        FreeCADGui.ActiveDocument.resetEdit()
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.Selection.removeObserver(self.s) 

    def reject(self):
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.Selection.removeObserver(self.s) 

    def getFields(self):    
        if self.obj:
            if hasattr(self.obj,"StartDepth"):
                self.obj.StartDepth = self.form.startDepth.text()
            if hasattr(self.obj,"FinalDepth"):
                self.obj.FinalDepth = self.form.finalDepth.text()
            if hasattr(self.obj,"SafeHeight"):
                self.obj.SafeHeight = self.form.safeHeight.text()
            if hasattr(self.obj,"ClearanceHeight"):
                self.obj.ClearanceHeight = self.form.clearanceHeight.text()
            if hasattr(self.obj,"StepDown"):
                self.obj.StepDown = self.form.stepDown.value()
            if hasattr(self.obj,"MaterialAllowance"):
                self.obj.MaterialAllowance = self.form.extraOffset.value()
            if hasattr(self.obj,"UseStartPoint"):
                self.obj.UseStartPoint = self.form.useStartPoint.isChecked()
            if hasattr(self.obj,"Algorithm"):
                self.obj.Algorithm = str(self.form.algorithmSelect.currentText())
            if hasattr(self.obj,"CutMode"):
                self.obj.CutMode = str(self.form.cutMode.currentText())
        self.obj.Proxy.execute(self.obj)

    def open(self):
        self.s =SelObserver()
        # install the function mode resident
        FreeCADGui.Selection.addObserver(self.s)   

    def addBase(self):
         # check that the selection contains exactly what we want
        selection = FreeCADGui.Selection.getSelectionEx()

        if not len(selection) >= 1:
            FreeCAD.Console.PrintError(translate("PathProject","Please select at least one profileable object\n"))
            return
        for s in selection:
            if s.HasSubObjects:
                for i in s.SubElementNames:
                    self.obj.Proxy.addpocketbase(self.obj, s.Object, i)
            else:      
                self.obj.Proxy.addpocketbase(self.obj, s.Object)

        self.form.baseList.clear()
        for i in self.obj.Base:         
            self.form.baseList.addItem(i[0].Name + "." + i[1])
         
    def deleteBase(self):
        dlist = self.form.baseList.selectedItems()
        newlist = []
        for d in dlist:
            for i in self.obj.Base:
                if i[0].Name != d.text().partition(".")[0] or i[1] != d.text().partition(".")[2] :
                    newlist.append (i)
            self.form.baseList.takeItem(self.form.baseList.row(d))
        self.obj.Base = newlist
        self.obj.Proxy.execute(self.obj)
        FreeCAD.ActiveDocument.recompute()

    def itemActivated(self):
        FreeCADGui.Selection.clearSelection()
        slist = self.form.baseList.selectedItems()
        for i in slist:
            objstring = i.text().partition(".")
            obj = FreeCAD.ActiveDocument.getObject(objstring[0])
          #  sub = o.Shape.getElement(objstring[2])
            if objstring[2] != "": 
                FreeCADGui.Selection.addSelection(obj,objstring[2])
            else:
                FreeCADGui.Selection.addSelection(obj)

        FreeCADGui.updateGui()

    def reorderBase(self):
        newlist = []
        for i in range(self.form.baseList.count()):
            s = self.form.baseList.item(i).text()
            objstring = s.partition(".")

            obj = FreeCAD.ActiveDocument.getObject(objstring[0])
            item = (obj, str(objstring[2]))
            newlist.append(item)
        self.obj.Base=newlist

        self.obj.Proxy.execute(self.obj)
        FreeCAD.ActiveDocument.recompute()

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Ok)


    def edit(self,item,column):
        if not self.updating:
            self.resetObject()

    def setupUi(self):
        self.form.startDepth.setText(str(self.obj.StartDepth.Value))
        self.form.finalDepth.setText(str(self.obj.FinalDepth.Value))
        self.form.safeHeight.setText(str(self.obj.SafeHeight.Value))
        self.form.clearanceHeight.setText(str(self.obj.ClearanceHeight.Value))
        self.form.stepDown.setValue(self.obj.StepDown)
        self.form.extraOffset.setValue(self.obj.MaterialAllowance.Value)
        self.form.useStartPoint.setChecked(self.obj.UseStartPoint)

        index = self.form.algorithmSelect.findText(self.obj.Algorithm, QtCore.Qt.MatchFixedString)
        if index >= 0:
            self.form.algorithmSelect.setCurrentIndex(index)

        for i in self.obj.Base:         
            self.form.baseList.addItem(i[0].Name + "." + i[1])

        #Connect Signals and Slots
        #Base Controls
        self.form.baseList.itemSelectionChanged.connect(self.itemActivated)
        self.form.addBase.clicked.connect(self.addBase)
        self.form.deleteBase.clicked.connect(self.deleteBase)
        self.form.reorderBase.clicked.connect(self.reorderBase)

        #Depths
        self.form.startDepth.editingFinished.connect(self.getFields) 
        self.form.finalDepth.editingFinished.connect(self.getFields)
        self.form.stepDown.editingFinished.connect(self.getFields)
        
        #Heights
        self.form.safeHeight.editingFinished.connect(self.getFields)
        self.form.clearanceHeight.editingFinished.connect(self.getFields)
        
        #operation
        self.form.algorithmSelect.currentIndexChanged.connect(self.getFields)
        self.form.cutMode.currentIndexChanged.connect(self.getFields)
        self.form.useStartPoint.clicked.connect(self.getFields)
        self.form.extraOffset.editingFinished.connect(self.getFields)
        

class SelObserver:
    def __init__(self):
        import PathScripts.PathSelection as PST 
        PST.pocketselect()

    def __del__(self):
        import PathScripts.PathSelection as PST 
        PST.clear()

    def addSelection(self,doc,obj,sub,pnt):               # Selection object
        FreeCADGui.doCommand('Gui.Selection.addSelection(FreeCAD.ActiveDocument.' + obj +')')
        FreeCADGui.updateGui()

if FreeCAD.GuiUp: 
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_Pocket',CommandPathPocket())
    FreeCADGui.addCommand('Set_PocketStartPoint',_CommandSetPocketStartPoint())


FreeCAD.Console.PrintLog("Loading PathPocket... done\n")
