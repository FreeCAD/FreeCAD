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

def frange(start, stop, step, finish):
    x = []
    curdepth = start
    if step == 0:
        return x
    # do the base cuts until finishing round
    while curdepth >= stop + step + finish:
        curdepth = curdepth - step
        if curdepth <= stop + finish:
            curdepth = stop + finish
        x.append(curdepth)

    # we might have to do a last pass or else finish round might be too far away
    if curdepth - stop > finish:
        x.append(stop + finish)

    # do the the finishing round
    if curdepth >= stop:
        curdepth = stop
        x.append(curdepth)

     # Why this?
#    if start >= stop:
#        start = stop
#        x.append (start)

    return x

class ObjectPocket:
    

    def __init__(self,obj):
        obj.addProperty("App::PropertyLinkSub","Base","Path",translate("PathProject","The base geometry of this object"))
        obj.addProperty("App::PropertyIntegerConstraint","ToolNumber","Tool",
                        translate("PathProfile","The tool number in use"))
        obj.ToolNumber = (0, 0, 1000, 0)


        obj.addProperty("App::PropertyFloat", "ClearanceHeight", "Pocket", translate("PathProject","The height needed to clear clamps and obstructions"))
        obj.addProperty("App::PropertyFloatConstraint", "StepDown", "Pocket", translate("PathProject","Incremental Step Down of Tool"))
        obj.StepDown = (0.0, 0.0, 100.0, 1.0)

        obj.addProperty("App::PropertyFloat", "StartDepth", "Pocket", translate("PathProject","Starting Depth of Tool- first cut depth in Z"))
        obj.addProperty("App::PropertyBool","UseStartDepth","Pocket",translate("PathProject","make True, if manually specifying a Start Start Depth"))
        obj.addProperty("App::PropertyFloat", "FinalDepth", "Pocket", translate("PathProject","Final Depth of Tool- lowest value in Z"))
        obj.addProperty("App::PropertyFloat", "RetractHeight", "Pocket", translate("PathProject","The height desired to retract tool when path is finished"))

        obj.addProperty("App::PropertyEnumeration", "CutMode", "Pocket",translate("PathProject", "The direction that the toolpath should go around the part ClockWise CW or CounterClockWise CCW"))
        obj.CutMode = ['Climb','Conventional']
        obj.addProperty("App::PropertyFloat", "MaterialAllowance", "Pocket", translate("PathProject","Amount of material to leave"))
        obj.addProperty("App::PropertyFloat", "FinishDepth", "Pocket", translate("PathProject","Maximum material removed on final pass."))

        obj.addProperty("App::PropertyEnumeration", "StartAt", "Pocket",translate("PathProject", "Start pocketing at center or boundary"))
        obj.StartAt = ['Center', 'Edge']

        obj.addProperty("App::PropertyFloatConstraint", "VertFeed", "Feed",translate("Vert Feed","Feed rate for vertical moves in Z"))
        obj.VertFeed = (0.0, 0.0, 100000.0, 1.0)

        obj.addProperty("App::PropertyFloatConstraint", "HorizFeed", "Feed",translate("Horiz Feed","Feed rate for horizontal moves"))
        obj.HorizFeed = (0.0, 0.0, 100000.0, 1.0)


        obj.addProperty("App::PropertyBool","Active","Path",translate("PathProject","Make False, to prevent operation from generating code"))
        obj.addProperty("App::PropertyString","Comment","Path",translate("PathProject","An optional comment for this profile"))

        obj.Proxy = self

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None
        

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



    #To reload this from FreeCAD, use: import PathScripts.PathPocket; reload(PathScripts.PathPocket)
    def execute(self,obj):
        if obj.Base:
            tool = PathUtils.getLastTool(obj)

            if tool:
                radius = tool.Diameter/2
                if radius < 0:# safe guard
                    radius -= radius
            else:
                # temporary value, to be taken from the properties later on
                radius = 1

            import Part, DraftGeomUtils
            if "Face" in obj.Base[1][0]:
                shape = getattr(obj.Base[0].Shape,obj.Base[1][0])
            else:
                edges = [getattr(obj.Base[0].Shape,sub) for sub in obj.Base[1]]
                shape = Part.Wire(edges)
                print len(edges)

            # absolute coords, millimeters, cancel offsets
            output = "G90\nG21\nG40\n"
            # save tool
            if obj.ToolNumber > 0 and tool.ToolNumber != obj.ToolNumber:
                output += "M06 T" + str(tool.ToolNumber) + "\n"
            
            # build offsets
            offsets = []
            nextradius = radius
            result = DraftGeomUtils.pocket2d(shape,nextradius)
            while result:
                #print "Adding " + str(len(result)) + " wires"
                offsets.extend(result)
                nextradius += radius
                result = DraftGeomUtils.pocket2d(shape,nextradius)
            
            # first move will be rapid, subsequent will be at feed rate
            first = True
            startPoint = None
            fastZPos = max(obj.StartDepth + 2, obj.RetractHeight)
            
            # revert the list so we start with the outer wires
            if obj.StartAt != 'Edge':
                offsets.reverse()

#            print "startDepth: " + str(obj.StartDepth)
#            print "finalDepth: " + str(obj.FinalDepth)
#            print "stepDown: " + str(obj.StepDown)
#            print "finishDepth" + str(obj.FinishDepth)
#            print "offsets:", len(offsets)

            def prnt(vlu): return str("%.4f" % round(vlu, 4))
            
            #Fraction of tool radius our plunge helix is to be
            #FIXME: This should be configurable
            plungeR = 0.75
            
            #(minimum) Fraction of tool DIAMETER to go back and forth while ramp-plunging
            #FIXME: This should be configurable
            #FIXME: The ramp plunging should maybe even be limited to this distance; I don't know what's best
            rampD = 0.75
            
            
            #Total offset from the desired pocket edge is tool radius plus the plunge helix radius
            #Any point on these curves could be the center of a plunge
            helixBounds = DraftGeomUtils.pocket2d(shape, tool.Diameter / 2. * (1 + plungeR))
            
            
            #Try to find a location to nicely plunge, starting with a helix, then ramp
            #Can't do it without knowledge of a tool
            plungePos = None
            rampEdge = None
            if not tool:
                    raise Error("Ramp plunge location-finding requires a tool")
                    return
            else:
                #Since we're going to start machining either the inner-most
                #edge or the outer (depending on StartAt setting), try to
                #plunge near that location
                
                if helixBounds:
                    #Edge is easy- pick a point on helixBounds and go with it
                    if obj.StartAt == 'Edge':
                        plungePos = helixBounds[0].Edges[0].Vertexes[0].Point
                    #Center is harder- use a point from the first offset, check if it works
                    else:
                        plungePos = offsets[0].Edges[0].Vertexes[0].Point
                        
                        #If it turns out this is invalid for some reason, nuke plungePos
                        [perp,idx] = DraftGeomUtils.findPerpendicular(plungePos, shape.Edges)
                        if not perp or perp.Length < tool.Diameter / 2. * (1 + plungeR):
                            plungePos = None
                        #FIXME: Really need to do a point-in-polygon operation to make sure this is within helixBounds
                        #Or some math to prove that it has to be (doubt that's true)
                        #Maybe reverse helixBounds and pick off that?
                
                
                #If we didn't find a place to helix, how about a ramp?
                if not plungePos:
                    #Check first edge of our offsets
                    if (offsets[0].Edges[0].Length >= tool.Diameter * rampD) and not (isinstance(offsets[0].Edges[0].Curve, Part.Circle)):
                        rampEdge = offsets[0].Edges[0]
                    #The last edge also connects with the starting location- try that
                    elif (offsets[0].Edges[-1].Length >= tool.Diameter * rampD) and not (isinstance(offsets[0].Edges[-1].Curve, Part.Circle)):
                        rampEdge = offsets[0].Edges[-1]
                    else:
                        print "Neither edge works: " + str(offsets[0].Edges[0]) + ", " + str(offsets[0].Edges[-1])
                    #FIXME: There's got to be a smarter way to find a place to ramp
                
            
            #Returns gcode to perform a rapid move
            def rapid(x=None, y=None, z=None):
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
                
            #Returns gcode to perform a linear feed
            def feed(x=None, y=None, z=None):
                global feedxy
                retstr = "G01 F"
                if(x == None) and (y == None):
                    retstr += str("%.4f" % obj.HorizFeed)
                else:
                    retstr += str("%.4f" % obj.VertFeed)
                
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
            
            #Returns gcode to perform an arc
            #Assumes XY plane or helix around Z
            #Don't worry about starting Z- assume that's dealt with elsewhere
            def arc(cx, cy, sx, sy, ex, ey, ez=None, ccw=False):
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
                retstr += str(obj.HorizFeed)
                
                #End location
                retstr += " X" + str("%.4f" % ex) + " Y" + str("%.4f" % ey)
                
                #Helix if requested
                if ez != None:
                    retstr += " Z" + str("%.4f" % ez)
                
                #Append center offsets
                retstr += " I" + str("%.4f" % (cx - sx)) + " J" + str("%.4f" % (cy - sy))
                
                return retstr + "\n"

            #Returns gcode to helically plunge
            #destZ is the milling level
            #startZ is the height we can safely feed down to before helix-ing
            def helicalPlunge(plungePos, rampangle, destZ, startZ):
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
                
                
            #Returns commands to linearly ramp into a cut
            #FIXME: This ramps along the first edge, assuming it's long
            #enough, NOT just wiggling back and forth by ~0.75 * toolD.
            #Not sure if that's any worse, but it's simpler
            #FIXME: This code is untested
            def rampPlunge(edge, rampangle, destZ, startZ):
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
                    
            
            
            
            #For helix-ing/ramping, know where we were last time
            #FIXME: Can probably get this from the "machine"?
            lastZ = fastZPos

            for vpos in frange(obj.StartDepth, obj.FinalDepth, obj.StepDown, obj.FinishDepth):
#                print "vpos: " + str(vpos)
                #Every for every depth we should helix down
                first = True
                # loop over successive wires
                for currentWire in offsets:
#                    print "new line (offset)"
                    last = None
                    for edge in currentWire.Edges:
#                        print "new edge"
                        if not last:
                            # we set the base GO to our fast move to our starting pos
                            if first:
                                #If we can helix, do so
                                if plungePos:
                                    output += helicalPlunge(plungePos, 3, vpos, lastZ)
                                    #print output
                                    lastZ = vpos
                                #Otherwise, see if we can ramp
                                #FIXME: This could be a LOT smarter (eg, searching for a longer leg of the edge to ramp along)
                                elif rampEdge:
                                    output += rampPlunge(rampEdge, 3, vpos, lastZ)
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
#                                print "flipped"
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
            output += "G0 Z" + prnt(fastZPos) + "\n"
#            print output
#            path = Path.Path(output)
#            obj.Path = path
            if obj.Active:
                path = Path.Path(output)
                obj.Path = path
                obj.ViewObject.Visibility = True
            else:
                path = Path.Path("(inactive operation)")
                obj.Path = path
                obj.ViewObject.Visibility = False


class ViewProviderPocket:

    def __init__(self,vobj):
        vobj.Proxy = self

    def attach(self,vobj):
        self.Object = vobj.Object
        return

    def getIcon(self):
        return ":/icons/Path-Pocket.svg"

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None


class CommandPathPocket:


    def GetResources(self):
        return {'Pixmap'  : 'Path-Pocket',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_Pocket","Pocket"),
                'Accel': "P, O",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_Pocket","Creates a Path Pocket object from a loop of edges or a face")}

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None
        
    def Activated(self):
        
        # check that the selection contains exactly what we want
        selection = FreeCADGui.Selection.getSelectionEx()
        if len(selection) != 1:
            FreeCAD.Console.PrintError(translate("Path_Pocket","Please select an edges loop from one object, or a single face\n"))
            return
        if len(selection[0].SubObjects) == 0:
            FreeCAD.Console.PrintError(translate("Path_Pocket","Please select an edges loop from one object, or a single face\n"))
            return
        for s in selection[0].SubObjects:
            if s.ShapeType != "Edge":
                if (s.ShapeType != "Face") or (len(selection[0].SubObjects) != 1):
                    FreeCAD.Console.PrintError(translate("Path_Pocket","Please select only edges or a single face\n"))
                    return
        if selection[0].SubObjects[0].ShapeType == "Edge":
            try:
                import Part
                w = Part.Wire(selection[0].SubObjects)
            except:
                FreeCAD.Console.PrintError(translate("Path_Pocket","The selected edges don't form a loop\n"))
                return
        
        # if everything is ok, execute and register the transaction in the undo/redo stack
        FreeCAD.ActiveDocument.openTransaction(translate("Path_Pocket","Create Pocket"))
        FreeCADGui.addModule("PathScripts.PathPocket")
        FreeCADGui.doCommand('prjexists = False')
        FreeCADGui.doCommand('obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython","Pocket")')
        FreeCADGui.doCommand('PathScripts.PathPocket.ObjectPocket(obj)')
        FreeCADGui.doCommand('PathScripts.PathPocket.ViewProviderPocket(obj.ViewObject)')
        subs = "["
        for s in selection[0].SubElementNames:
            subs += '"' + s + '",'
        subs += "]"
        FreeCADGui.doCommand('obj.Base = (FreeCAD.ActiveDocument.' + selection[0].ObjectName + ',' + subs + ')')
        FreeCADGui.doCommand('obj.Active = True')
        snippet = '''
from PathScripts import PathUtils
PathUtils.addToProject(obj)

ZMax = obj.Base[0].Shape.BoundBox.ZMax
ZMin = obj.Base[0].Shape.BoundBox.ZMin
obj.StepDown = 1.0
obj.StartDepth = ZMax
obj.FinalDepth = ZMin
obj.ClearanceHeight =  ZMax + 5.0
    
'''
        FreeCADGui.doCommand(snippet)
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp: 
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_Pocket',CommandPathPocket())

FreeCAD.Console.PrintLog("Loading PathPocket... done\n")
