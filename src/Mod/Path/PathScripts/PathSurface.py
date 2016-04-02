# -*- coding: utf-8 -*-

#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2016 sliptonic  <shopinthewoods@gmail.com>              *
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
from FreeCAD import Vector
from PathScripts import PathUtils,PathSelection,PathProject

if FreeCAD.GuiUp:
    import FreeCADGui, PathGui
    from PySide import QtCore, QtGui
    from DraftTools import translate
    from pivy import coin
else:
    def translate(ctxt,txt):
        return txt

__title__="Path Surface Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"

"""Path surface object and FreeCAD command"""

# Qt tanslation handling
try:
    _encoding = QtGui.QApplication.UnicodeUTF8
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig)


class ObjectSurface:
    

    def __init__(self,obj):
        obj.addProperty("App::PropertyLinkSub","Base","Path",translate("Parent Object(s)","The base geometry of this toolpath"))
        obj.addProperty("App::PropertyBool","Active","Path",translate("Active","Make False, to prevent operation from generating code"))
        obj.addProperty("App::PropertyString","Comment","Path",translate("PathProject","An optional comment for this profile"))
        obj.addProperty("App::PropertyEnumeration", "Algorithm", "Algorithm",translate("PathProject", "The library to use to generate the path"))
        obj.Algorithm = ['OCL Dropcutter', 'OCL Waterline']

        #Tool Properties
        obj.addProperty("App::PropertyIntegerConstraint","ToolNumber","Tool",translate("PathProfile","The tool number in use"))
        obj.ToolNumber = (0, 0, 1000, 0)
        obj.setEditorMode('ToolNumber',1) #make this read only

        #Surface Properties
        obj.addProperty("App::PropertyFloatConstraint", "SampleInterval", "Surface", translate("PathSurface","The Sample Interval.  Small values cause long wait"))
        obj.SampleInterval = (0,0,1,0)
        #Depth Properties
        obj.addProperty("App::PropertyFloat", "ClearanceHeight", "Depth", translate("PathProject","The height needed to clear clamps and obstructions"))
        obj.addProperty("App::PropertyFloat", "SafeHeight", "Depth", translate("PathProject","Rapid Safety Height between locations."))
        obj.addProperty("App::PropertyFloatConstraint", "StepDown", "Depth", translate("PathProject","Incremental Step Down of Tool"))
        obj.StepDown = (0.0, 0.01, 100.0, 0.5)
        obj.addProperty("App::PropertyFloat", "StartDepth", "Depth", translate("PathProject","Starting Depth of Tool- first cut depth in Z"))
        obj.addProperty("App::PropertyFloat", "FinalDepth", "Depth", translate("PathProject","Final Depth of Tool- lowest value in Z"))
        obj.addProperty("App::PropertyFloat", "FinishDepth", "Depth", translate("PathProject","Maximum material removed on final pass."))

        #Feed Properties
        obj.addProperty("App::PropertySpeed", "VertFeed", "Feed",translate("Path","Feed rate for vertical moves in Z"))
        obj.addProperty("App::PropertySpeed", "HorizFeed", "Feed",translate("Path","Feed rate for horizontal moves"))

        
        obj.Proxy = self

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None

    def _waterline(self,obj, s, bb):
        import ocl
#        from PathScripts.PathUtils import fmt
        from PathScripts.PathUtils import depth_params, fmt
        import time

        def drawLoops(loops):
            nloop = 0
            waterlinestring = ""
            waterlinestring += "(waterline begin)"
            for loop in loops:
                p = loop[0]
                loopstring = "(loop begin)" +"\n"
                loopstring += "G0 Z" + str(obj.SafeHeight) +"\n"
                loopstring += "G0 X" + str(fmt(p.x)) + " Y" + str(fmt(p.y))  +"\n"
                loopstring += "G1 Z" + str(fmt(p.z)) +"\n"
                for p in loop[1:]:
                    loopstring += "G1 X" + str(fmt(p.x)) + " Y" + str(fmt(p.y)) + " Z" + str(fmt(p.z)) +"\n"
                    zheight = p.z
                p = loop[0]
                loopstring += "G1 X" + str(fmt(p.x)) + " Y" + str(fmt(p.y)) + " Z" + str(fmt(zheight)) +"\n"
                loopstring += "(loop end)" +"\n"
                print "    loop ",nloop, " with ", len(loop), " points"
                nloop = nloop+1
                waterlinestring += loopstring
            waterlinestring += "(waterline end)"  +"\n"
                
            return waterlinestring


            # nloop = 0
            # for lop in loops:
            #     n = 0
            #     N = len(lop)
            #     first_point=ocl.Point(-1,-1,5)
            #     previous=ocl.Point(-1,-1,5)
            #     for p in lop:
            #         if n==0: # don't draw anything on the first iteration
            #             previous=p 
            #             first_point = p
            #         elif n== (N-1): # the last point
            #             # and a line from p to the first point
            #             p1=(p.x,p.y,p.z)
            #             p2=(first_point.x,first_point.y,first_point.z)
            #             output += "G1 X" + str(p2[0]) + " Y" + str (p2[1]) + " Z" +str(p2[2]) +"\n"   
            #         else:
            #             p1=(previous.x,previous.y,previous.z)
            #             p2=(p.x,p.y,p.z)
            #             output += "G1 X" + str(p2[0]) + " Y" + str (p2[1]) + " Z" +str(p2[2]) +"\n"  
            #             #print "line from: " + str (p1) + "To: " + str (p2)   
                        
            #             #print "line X: " + str (p1.x) +" Y: " + str(p1.y) + "Z: " + str(p1.z) + " To X: " + str (p2.x) +" Y: " + str(p2.y) + "Z: " + str(p2.z) 
            #             previous=p
            #         n=n+1
            #     print "    loop ",nloop, " with ", len(lop), " points"
            #     nloop = nloop+1
            # return output            

        depthparams = depth_params (obj.ClearanceHeight, obj.SafeHeight, obj.StartDepth, obj.StepDown, obj.FinishDepth, obj.FinalDepth)
        
        # stlfile = "../../stl/gnu_tux_mod.stl"
        # surface = STLSurfaceSource(stlfile)
        
        surface = s

        t_before = time.time() 
        
        zheights= depthparams.get_depths()
        
        wl = ocl.Waterline()          
        #wl = ocl.AdaptiveWaterline() # this is slower, ca 60 seconds on i7 CPU
        wl.setSTL(surface)
        diam = 0.5
        length= 10 
        cutter = ocl.BallCutter( diam , length ) # any ocl MillingCutter class should work here
        wl.setCutter(cutter)
        wl.setSampling(obj.SampleInterval) # this should be smaller than the smallest details in the STL file
                               # AdaptiveWaterline() also has settings for minimum sampling interval (see c++ code)
        all_loops=[]
        for zh in zheights:
            print "calculating Waterline at z= ", zh
            wl.reset()
            wl.setZ(zh) # height for this waterline
            wl.run()
            all_loops.append( wl.getLoops() )
        t_after = time.time()
        calctime = t_after-t_before
        n=0
        output = ""
        for loops in all_loops: # at each z-height, we may get many loops
            print "  %d/%d:" % (n,len(all_loops))
            output += drawLoops(loops)
            n=n+1
        print "(" + str(calctime) + ")"
        return output

    def _dropcutter(self,obj, s, bb):
        import ocl
        import time

        cutter = ocl.CylCutter(self.radius*2, 5)
        pdc = ocl.PathDropCutter()   # create a pdc
        pdc.setSTL(s)
        pdc.setCutter(cutter) 
        pdc.minimumZ = 0.25                          
        pdc.setSampling(obj.SampleInterval)
         
        # some parameters for this "zigzig" pattern    
        xmin=bb.XMin - cutter.getDiameter()
        xmax=bb.XMax + cutter.getDiameter()
        ymin=bb.YMin - cutter.getDiameter()
        ymax=bb.YMax + cutter.getDiameter()
        zmax=bb.ZMax + cutter.getDiameter()
         
        Ny=int(bb.YLength/cutter.getDiameter())  # number of lines in the y-direction
        dy = float(ymax-ymin)/Ny  # the y step-over
         
        path = ocl.Path()                   # create an empty path object
        
        # add Line objects to the path in this loop
        for n in xrange(0,Ny):
            y = ymin+n*dy
            p1 = ocl.Point(xmin,y,0)   # start-point of line
            p2 = ocl.Point(xmax,y,0)   # end-point of line
            if (n % 2 == 0): #even 
                l = ocl.Line(p1,p2)     # line-object
            else: #odd
                l = ocl.Line(p2,p1)     # line-object

            path.append( l )        # add the line to the path
         
        pdc.setPath( path )
        
        # run drop-cutter on the path
        t_before = time.time()
        pdc.run()
        t_after = time.time()
        print "calculation took ", t_after-t_before," s"

        #retrieve the points
        clp = pdc.getCLPoints()     
        print "points received: " + str(len(clp))

        #generate the path commands
        output = ""
        output += "G0 Z" + str(obj.ClearanceHeight) + "\n"
        output += "G0 X" + str(clp[0].x) +" Y" + str(clp[0].y) + "\n"
        output += "G1 Z" + str(clp[0].z) + " F" + str(obj.VertFeed.Value) + "\n"
       
        for c in clp:
            output += "G1 X" + str(c.x) +" Y" + str(c.y) +" Z" + str(c.z)+ "\n"
        
        return output



    def execute(self,obj):
        import Part
        import Mesh
        import MeshPart
        FreeCAD.Console.PrintWarning(translate("PathSurface","Hold on.  This might take a minute.\n"))
 
        output = ""

        if obj.Algorithm in ['OCL Dropcutter', 'OCL Waterline']:
            try:
                import ocl
            except:
                FreeCAD.Console.PrintError(translate("PathSurface","This operation requires OpenCamLib to be installed.\n"))
                return

        # tie the toolnumber to the PathLoadTool object ToolNumber
        if len(obj.InList)>0: #check to see if obj is in the Project group yet
            project = obj.InList[0]
            tl = int(PathUtils.changeTool(obj,project))
            obj.ToolNumber= tl   
        
        tool = PathUtils.getTool(obj,obj.ToolNumber)
        if tool:
            self.radius = tool.Diameter/2
        else:
            # temporary value,in case we don't have any tools defined already
            self.radius = 0.25


        mesh = obj.Base[0]
        if mesh.TypeId.startswith('Mesh'):
            mesh = mesh.Mesh
            bb = mesh.BoundBox
        else:
            bb = mesh.Shape.BoundBox
            mesh = MeshPart.meshFromShape(mesh.Shape,MaxLength=2)

        s= ocl.STLSurf()
        for f in mesh.Facets:
            p = f.Points[0];q=f.Points[1];r=f.Points[2]
            t= ocl.Triangle(ocl.Point(p[0],p[1],p[2]),ocl.Point(q[0],q[1],q[2]),ocl.Point(r[0],r[1],r[2]))
            s.addTriangle(t)
        
        if obj.Algorithm == 'OCL Dropcutter':
            output = self._dropcutter(obj, s, bb)
        elif obj.Algorithm == 'OCL Waterline':
            output = self._waterline(obj, s, bb)


        path = Path.Path(output)
        obj.Path = path



class ViewProviderSurface:
    def __init__(self,obj): #mandatory
#        obj.addProperty("App::PropertyFloat","SomePropertyName","PropertyGroup","Description of this property")
        obj.Proxy = self

    def __getstate__(self): #mandatory
        return None

    def __setstate__(self,state): #mandatory
        return None

    def getIcon(self): #optional
        return ":/icons/Path-Surfacing.svg"

#    def attach(self): #optional
#        # this is executed on object creation and object load from file
#        pass

    def onChanged(self,obj,prop): #optional
        # this is executed when a property of the VIEW PROVIDER changes
        pass

    def updateData(self,obj,prop): #optional
        # this is executed when a property of the APP OBJECT changes
        pass

    def setEdit(self,vobj,mode): #optional
        # this is executed when the object is double-clicked in the tree
        pass

    def unsetEdit(self,vobj,mode): #optional
        # this is executed when the user cancels or terminates edit mode
        pass



class CommandPathSurfacing:


    def GetResources(self):
        return {'Pixmap'  : 'Path-Surfacing',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_Surface","Surfacing"),
                'Accel': "P, D",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_Surface","Creates a Path Surfacing object")}

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None

    def Activated(self):
        # check that the selection contains exactly what we want
        selection = FreeCADGui.Selection.getSelectionEx()
        if len(selection) != 1:
            FreeCAD.Console.PrintError(translate("PathSurface","Please select a single solid object from the project tree\n"))
            return
        if not len(selection[0].SubObjects) == 0:
            FreeCAD.Console.PrintError(translate("PathSurface","Please select a single solid object from the project tree\n"))
            return
        for s in selection[0].SubObjects:
            if s.ShapeType != "Edge":
                if (s.ShapeType != "Face") or (len(selection[0].SubObjects) != 1):
                    FreeCAD.Console.PrintError(translate("PathSurface","Please select only edges or a single face\n"))
                    return

        sel = selection[0].Object
         #get type of object
        if sel.TypeId.startswith('Mesh'):
            #it is a mesh already
            print 'was already mesh'
            ztop = sel.Mesh.BoundBox.ZMax
            zbottom = sel.Mesh.BoundBox.ZMin

            #mesh = sel
        elif sel.TypeId.startswith('Part') and \
            (sel.Shape.BoundBox.XLength > 0) and \
            (sel.Shape.BoundBox.YLength > 0) and \
            (sel.Shape.BoundBox.ZLength > 0):
            ztop = sel.Shape.BoundBox.ZMax 
            zbottom = sel.Shape.BoundBox.ZMin
            print 'this is a solid Part object'
 
        else:
            FreeCAD.Console.PrintError(translate("PathSurface","Cannot work with this object\n"))
            return

        # if everything is ok, execute and register the transaction in the undo/redo stack
        FreeCAD.ActiveDocument.openTransaction(translate("Path_Surfacing","Create Surface"))
        FreeCADGui.addModule("PathScripts.PathSurface")
        FreeCADGui.doCommand('obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython","Surface")')
        FreeCADGui.doCommand('PathScripts.PathSurface.ObjectSurface(obj)')
        FreeCADGui.doCommand('obj.Active = True')
        FreeCADGui.doCommand('PathScripts.PathSurface.ViewProviderSurface(obj.ViewObject)')
        FreeCADGui.doCommand('obj.Base = (FreeCAD.ActiveDocument.'+selection[0].ObjectName+',[])')
        FreeCADGui.doCommand('from PathScripts import PathUtils')
        FreeCADGui.doCommand('obj.ClearanceHeight = ' + str(ztop + 2))
        FreeCADGui.doCommand('obj.StartDepth = ' + str(ztop))
        FreeCADGui.doCommand('obj.SafeHeight = ' + str(ztop + 2))
        FreeCADGui.doCommand('obj.StepDown = ' + str((ztop-zbottom)/8))
        FreeCADGui.doCommand('obj.SampleInterval = 0.4')

        FreeCADGui.doCommand('obj.FinalDepth=' + str(zbottom))
        FreeCADGui.doCommand('PathScripts.PathUtils.addToProject(obj)')

        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp: 
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_Surfacing',CommandPathSurfacing())

FreeCAD.Console.PrintLog("Loading PathSurfacing... done\n")
