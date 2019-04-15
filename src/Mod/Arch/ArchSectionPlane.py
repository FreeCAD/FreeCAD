#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2011                                                    *
#*   Yorik van Havre <yorik@uncreated.net>                                 *
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

import FreeCAD,WorkingPlane,math,Draft,ArchCommands,DraftVecUtils,ArchComponent
from FreeCAD import Vector
if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore, QtGui
    from DraftTools import translate
    from pivy import coin
    from PySide.QtCore import QT_TRANSLATE_NOOP
else:
    # \cond
    def translate(ctxt,txt):
        return txt
    def QT_TRANSLATE_NOOP(ctxt,txt):
        return txt
    # \endcond

## @package ArchSectionPlane
#  \ingroup ARCH
#  \brief The Section plane object and tools
#
#  This module provides tools to build Section plane objects.
#  It also contains functionality to produce SVG rendering of
#  section planes, to be used in TechDraw and Drawing modules


def makeSectionPlane(objectslist=None,name="Section"):

    """makeSectionPlane([objectslist]) : Creates a Section plane objects including the
    given objects. If no object is given, the whole document will be considered."""

    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    obj = FreeCAD.ActiveDocument.addObject("App::FeaturePython",name)
    obj.Label = translate("Arch",name)
    _SectionPlane(obj)
    if FreeCAD.GuiUp:
        _ViewProviderSectionPlane(obj.ViewObject)
    if objectslist:
        obj.Objects = objectslist
        bb = FreeCAD.BoundBox()
        for o in Draft.getGroupContents(objectslist):
            if hasattr(o,"Shape") and hasattr(o.Shape,"BoundBox"):
                bb.add(o.Shape.BoundBox)
        obj.Placement = FreeCAD.DraftWorkingPlane.getPlacement()
        obj.Placement.Base = bb.Center
        if FreeCAD.GuiUp:
            margin = bb.XLength*0.1
            obj.ViewObject.DisplayLength = bb.XLength+margin
            obj.ViewObject.DisplayHeight = bb.YLength+margin
    return obj


def makeSectionView(section,name="View"):

    """makeSectionView(section) : Creates a Drawing view of the given Section Plane
    in the active Page object (a new page will be created if none exists"""

    page = None
    for o in FreeCAD.ActiveDocument.Objects:
        if o.isDerivedFrom("Drawing::FeaturePage"):
            page = o
            break
    if not page:
        page = FreeCAD.ActiveDocument.addObject("Drawing::FeaturePage","Page")
        page.Template = Draft.getParam("template",FreeCAD.getResourceDir()+'Mod/Drawing/Templates/A3_Landscape.svg')

    view = FreeCAD.ActiveDocument.addObject("Drawing::FeatureViewPython",name)
    page.addObject(view)
    _ArchDrawingView(view)
    view.Source = section
    view.Label = translate("Arch","View of")+" "+section.Name
    return view

def looksLikeDraft(o):
    # If there is no shape at all ignore it
    if not hasattr(o, 'Shape') or o.Shape.isNull():
        return False
    
    # If there are solids in the object, it will be handled later
    # by getCutShapes
    if len(o.Shape.Solids) > 0:
        return False
    
    # If we have a shape, but no volume, it looks like a flat 2D object
    return o.Shape.Volume == 0

def getCutShapes(objs,section,showHidden,groupSshapesByObject=False):

    import Part,DraftGeomUtils
    shapes = []
    hshapes = []
    sshapes = []
    objectShapes = []
    objectSshapes = []
    for o in objs:
        if o.isDerivedFrom("Part::Feature"):
            if o.Shape.isNull():
                pass
            elif section.OnlySolids:
                if o.Shape.isValid():
                    solids = []
                    solids.extend(o.Shape.Solids)

                    shapes.extend(solids)

                    objectShapes.append((o, solids))
                else:
                    print(section.Label,": Skipping invalid object:",o.Label)
            else:
                shapes.append(o.Shape)
                objectShapes.append((o, [o.Shape]))
    cutface,cutvolume,invcutvolume = ArchCommands.getCutVolume(section.Shape.copy(),shapes)
    if cutvolume:
        for o, shapeList in objectShapes:
            tmpSshapes = []
            for sh in shapeList:
                for sol in sh.Solids:
                    if sol.Volume < 0:
                        sol.reverse()
                    c = sol.cut(cutvolume)
                    s = sol.section(cutface)
                    try:
                        wires = DraftGeomUtils.findWires(s.Edges)
                        for w in wires:
                            f = Part.Face(w)
                            tmpSshapes.append(f)
                        #s = Part.Wire(s.Edges)
                        #s = Part.Face(s)
                    except Part.OCCError:
                        #print "ArchDrawingView: unable to get a face"
                        tmpSshapes.append(s)
                    shapes.extend(c.Solids)
                    #sshapes.append(s)
                    if showHidden:
                        c = sol.cut(invcutvolume)
                        hshapes.append(c)

            if len(tmpSshapes) > 0:
                sshapes.extend(tmpSshapes)
                
                if groupSshapesByObject:
                    objectSshapes.append((o, tmpSshapes))
    
    if groupSshapesByObject:
        return shapes,hshapes,sshapes,cutface,cutvolume,invcutvolume,objectSshapes
    else:
        return shapes,hshapes,sshapes,cutface,cutvolume,invcutvolume

def getFillForObject(o, defaultFill, section):
    if hasattr(section, 'UseMaterialColorForFill') and section.UseMaterialColorForFill:
        if hasattr(o, 'Material') and o.Material:
            material = o.Material
            
            if hasattr(material, 'Color') and material.Color:
                return o.Material.Color
    
    return defaultFill

def getSVG(section, renderMode="Wireframe", allOn=False, showHidden=False, scale=1, rotation=0, linewidth=1, lineColor=(0.0,0.0,0.0), fontsize=1, showFill=False, fillColor=(0.8,0.8,0.8), techdraw=False):

    """getSVG(section, [renderMode, allOn, showHidden, scale, rotation,
              linewidth, lineColor, fontsize, showFill, fillColor, techdraw]):

    returns an SVG fragment from an Arch section plane. If
    allOn is True, all cut objects are shown, regardless if they are visible or not.
    renderMode can be Wireframe (default) or Solid to use the Arch solid renderer. If
    showHidden is True, the hidden geometry above the section plane is shown in dashed line.
    If showFill is True, the cut areas get filled with a pattern.
    lineColor -- Color of lines for the renderMode "Wireframe".
    fillColor -- If showFill is True and renderMode is "Wireframe",
                 the cut areas are filled with fillColor.
    """

    if not section.Objects:
        return ""
    import Part,DraftGeomUtils
    p = FreeCAD.Placement(section.Placement)
    direction = p.Rotation.multVec(FreeCAD.Vector(0,0,1))
    objs = Draft.getGroupContents(section.Objects,walls=True,addgroups=True)
    if not allOn:
            objs = Draft.removeHidden(objs)

    # separate spaces and Draft objects
    spaces = []
    nonspaces = []
    drafts = []
    windows = []
    cutface = None
    for o in objs:
        if Draft.getType(o) == "Space":
            spaces.append(o)
        elif Draft.getType(o) in ["Dimension","Annotation","Label"]:
            drafts.append(o)
        elif o.isDerivedFrom("Part::Part2DObject"):
            drafts.append(o)
        elif looksLikeDraft(o):
            drafts.append(o)
        else:
            nonspaces.append(o)
        if Draft.getType(o) == "Window":
            windows.append(o)
    objs = nonspaces

    archUserParameters = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
    scaledLineWidth = linewidth/scale
    svgLineWidth = str(scaledLineWidth) + 'px'
    st = archUserParameters.GetFloat("CutLineThickness",2)
    svgCutLineWidth = str(scaledLineWidth * st) + 'px'
    yt = archUserParameters.GetFloat("SymbolLineThickness",0.6)
    svgSymbolLineWidth = str(linewidth * yt)
    hiddenPattern = archUserParameters.GetString("archHiddenPattern","30,10")
    svgHiddenPattern = hiddenPattern.replace(" ","")
    fillpattern = '<pattern id="sectionfill" patternUnits="userSpaceOnUse" patternTransform="matrix(5,0,0,5,0,0)"'
    fillpattern += ' x="0" y="0" width="10" height="10">'
    fillpattern += '<g>'
    fillpattern += '<rect width="10" height="10" style="stroke:none; fill:#ffffff" /><path style="stroke:#000000; stroke-width:1" d="M0,0 l10,10" /></g></pattern>'
    svgLineColor = Draft.getrgb(lineColor)
    svg = ''

    # reading cached version
    svgcache = None
    if hasattr(section.Proxy,"svgcache") and section.Proxy.svgcache:
        svgcache = section.Proxy.svgcache[0]
        if section.Proxy.svgcache[1] != renderMode:
            svgcache = None
        if section.Proxy.svgcache[2] != showHidden:
            svgcache = None
        if section.Proxy.svgcache[3] != showFill:
            svgcache = None

    # generating SVG
    if renderMode in ["Solid",1]:
        if not svgcache:
            svgcache = ''
            # render using the Arch Vector Renderer
            import ArchVRM, WorkingPlane
            wp = WorkingPlane.plane()
            wp.setFromPlacement(section.Placement)
            #wp.inverse()
            render = ArchVRM.Renderer()
            render.setWorkingPlane(wp)
            render.addObjects(objs)
            if showHidden:
                render.cut(section.Shape,showHidden)
            else:
                render.cut(section.Shape)
            svgcache += '<g transform="scale(1,-1)">\n'
            svgcache += render.getViewSVG(linewidth="SVGLINEWIDTH")
            svgcache += fillpattern
            svgcache += render.getSectionSVG(linewidth="SVGCUTLINEWIDTH",
                                        fillpattern="sectionfill")
            if showHidden:
                svgcache += render.getHiddenSVG(linewidth="SVGLINEWIDTH")
            svgcache += '</g>\n'
            # print(render.info())
            section.Proxy.svgcache = [svgcache,renderMode,showHidden,showFill]
    else:
        if showFill:
            shapes,hshapes,sshapes,cutface,cutvolume,invcutvolume,objectSshapes = getCutShapes(objs,section,showHidden, True)
        else:
            shapes,hshapes,sshapes,cutface,cutvolume,invcutvolume = getCutShapes(objs,section,showHidden)

        if not svgcache:
            svgcache = ""
            # render using the Drawing module
            import Drawing, Part
            if shapes:
                baseshape = Part.makeCompound(shapes)
                style = {'stroke':       "SVGLINECOLOR",
                         'stroke-width': "SVGLINEWIDTH"}
                svgcache += Drawing.projectToSVG(
                    baseshape, direction,
                    hStyle=style, h0Style=style, h1Style=style,
                    vStyle=style, v0Style=style, v1Style=style)
            if hshapes:
                hshapes = Part.makeCompound(hshapes)
                style = {'stroke':           "SVGLINECOLOR",
                         'stroke-width':     "SVGLINEWIDTH",
                         'stroke-dasharray': "SVGHIDDENPATTERN"}
                svgcache += Drawing.projectToSVG(
                    hshapes, direction,
                    hStyle=style, h0Style=style, h1Style=style,
                    vStyle=style, v0Style=style, v1Style=style)
            if sshapes:
                if showFill:
                    #svgcache += fillpattern
                    svgcache += '<g transform="rotate(180)">\n'
                    for o, shapes in objectSshapes:
                        for s in shapes:
                            if s.Edges:
                                objectFill = getFillForObject(o, fillColor, section)
                                #svg += Draft.getSVG(s,direction=direction.negative(),linewidth=0,fillstyle="sectionfill",color=(0,0,0))
                                # temporarily disabling fill patterns
                                svgcache += Draft.getSVG(s, direction=direction.negative(),
                                    linewidth=0,
                                    fillstyle=Draft.getrgb(objectFill),
                                    color=lineColor)
                    svgcache += "</g>\n"
                sshapes = Part.makeCompound(sshapes)
                style = {'stroke':       "SVGLINECOLOR",
                         'stroke-width': "SVGCUTLINEWIDTH"}
                svgcache += Drawing.projectToSVG(
                    sshapes, direction,
                    hStyle=style, h0Style=style, h1Style=style,
                    vStyle=style, v0Style=style, v1Style=style)
            section.Proxy.svgcache = [svgcache,renderMode,showHidden,showFill]
    svgcache = svgcache.replace("SVGLINECOLOR",svgLineColor)
    svgcache = svgcache.replace("SVGLINEWIDTH",svgLineWidth)
    svgcache = svgcache.replace("SVGHIDDENPATTERN",svgHiddenPattern)
    svgcache = svgcache.replace("SVGCUTLINEWIDTH",svgCutLineWidth)
    svg += svgcache

    if drafts:
        if not techdraw:
            svg += '<g transform="scale(1,-1)">'
        for d in drafts:
            svg += Draft.getSVG(d, scale=scale, linewidth=svgSymbolLineWidth,
                                fontsize=fontsize, direction=direction, color=lineColor,
                                techdraw=techdraw, rotation=rotation)
        if not techdraw:
            svg += '</g>'

    # filter out spaces not cut by the section plane
    if cutface and spaces:
        spaces = [s for s in spaces if s.Shape.BoundBox.intersect(cutface.BoundBox)]
    if spaces:
        if not techdraw:
            svg += '<g transform="scale(1,-1)">'
        for s in spaces:
            svg += Draft.getSVG(s, scale=scale, linewidth=svgSymbolLineWidth,
                                fontsize=fontsize, direction=direction, color=lineColor,
                                techdraw=techdraw, rotation=rotation)
        if not techdraw:
            svg += '</g>'

    # add additional edge symbols from windows
    cutwindows = []
    if cutface and windows:
        cutwindows = [w.Name for w in windows if w.Shape.BoundBox.intersect(cutface.BoundBox)]
    if windows:
        sh = []
        for w in windows:
            if not hasattr(w.Proxy,"sshapes"):
                w.Proxy.execute(w)
            if hasattr(w.Proxy,"sshapes"):
                if w.Proxy.sshapes and (w.Name in cutwindows):
                    c = Part.makeCompound(w.Proxy.sshapes)
                    c.Placement = w.Placement
                    sh.append(c)
            # buggy for now...
            #if hasattr(w.Proxy,"vshapes"):
            #    if w.Proxy.vshapes:
            #        c = Part.makeCompound(w.Proxy.vshapes)
            #        c.Placement = w.Placement
            #        sh.append(c)
        if sh:
            if not techdraw:
                svg += '<g transform="scale(1,-1)">'
            for s in sh:
                svg += Draft.getSVG(s, scale=scale,
                                    linewidth=svgSymbolLineWidth,
                                    fontsize=fontsize, fillstyle="none",
                                    direction=direction, color=lineColor,
                                    techdraw=techdraw, rotation=rotation)
            if not techdraw:
                svg += '</g>'

    return svg


def getDXF(obj):

    "returns a DXF representation from a TechDraw/Drawing view"
    allOn = True
    if hasattr(obj,"AllOn"):
        allOn = obj.AllOn
    elif hasattr(obj,"AlwaysOn"):
        allOn = obj.AlwaysOn
    showHidden = False
    if hasattr(obj,"showCut"):
        showHidden = obj.showCut
    elif hasattr(obj,"showHidden"):
        showHidden = obj.showHidden
    result = []
    import Drawing,Part
    if not obj.Source:
        return result
    section = obj.Source
    if not section.Objects:
        return result
    p = FreeCAD.Placement(section.Placement)
    direction = p.Rotation.multVec(FreeCAD.Vector(0,0,1))
    objs = Draft.getGroupContents(section.Objects,walls=True,addgroups=True)
    if not allOn:
            objs = Draft.removeHidden(objs)
    # separate spaces and Draft objects
    spaces = []
    nonspaces = []
    drafts = []
    objs = [o for o in objs if ((not(Draft.getType(o) in ["Space","Dimension","Annotation"])) and (not (o.isDerivedFrom("Part::Part2DObject"))))]
    shapes,hshapes,sshapes,cutface,cutvolume,invcutvolume = getCutShapes(objs,section,showHidden)
    if shapes:
        result.append(Drawing.projectToDXF(Part.makeCompound(shapes),direction))
    if sshapes:
        result.append(Drawing.projectToDXF(Part.makeCompound(sshapes),direction))
    if hshapes:
        result.append(Drawing.projectToDXF(Part.makeCompound(hshapes),direction))
    return result


class _CommandSectionPlane:

    "the Arch SectionPlane command definition"

    def GetResources(self):

        return {'Pixmap'  : 'Arch_SectionPlane',
                'Accel': "S, E",
                'MenuText': QT_TRANSLATE_NOOP("Arch_SectionPlane","Section Plane"),
                'ToolTip': QT_TRANSLATE_NOOP("Arch_SectionPlane","Creates a section plane object, including the selected objects")}

    def IsActive(self):

        return not FreeCAD.ActiveDocument is None

    def Activated(self):

        sel = FreeCADGui.Selection.getSelection()
        ss = "["
        for o in sel:
            if len(ss) > 1:
                ss += ","
            ss += "FreeCAD.ActiveDocument."+o.Name
        ss += "]"
        FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Section Plane"))
        FreeCADGui.addModule("Arch")
        FreeCADGui.doCommand("section = Arch.makeSectionPlane("+ss+")")
        #FreeCADGui.doCommand("section.Placement = FreeCAD.DraftWorkingPlane.getPlacement()")
        #FreeCADGui.doCommand("Arch.makeSectionView(section)")
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


class _SectionPlane:

    "A section plane object"

    def __init__(self,obj):
        obj.Proxy = self
        self.setProperties(obj)

    def setProperties(self,obj):

        pl = obj.PropertiesList
        if not "Placement" in pl:
            obj.addProperty("App::PropertyPlacement","Placement","SectionPlane",QT_TRANSLATE_NOOP("App::Property","The placement of this object"))
        if not "Shape" in pl:
            obj.addProperty("Part::PropertyPartShape","Shape","SectionPlane",QT_TRANSLATE_NOOP("App::Property","The shape of this object"))
        if not "Objects" in pl:
            obj.addProperty("App::PropertyLinkList","Objects","SectionPlane",QT_TRANSLATE_NOOP("App::Property","The objects that must be considered by this section plane. Empty means the whole document."))
        if not "OnlySolids" in pl:
            obj.addProperty("App::PropertyBool","OnlySolids","SectionPlane",QT_TRANSLATE_NOOP("App::Property","If false, non-solids will be cut too, with possible wrong results."))
            obj.OnlySolids = True
        if not "UseMaterialColorForFill" in pl:
            obj.addProperty("App::PropertyBool","UseMaterialColorForFill","SectionPlane",QT_TRANSLATE_NOOP("App::Property","If true, the color of the objects material will be used to fill cut areas."))
            obj.UseMaterialColorForFill = False
        self.Type = "SectionPlane"

    def onDocumentRestored(self,obj):

        self.setProperties(obj)

    def execute(self,obj):

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
        p = Part.makePlane(l,l,Vector(l/2,-l/2,0),Vector(0,0,-1))
        # make sure the normal direction is pointing outwards, you never know what OCC will decide...
        if p.normalAt(0,0).getAngle(obj.Placement.Rotation.multVec(FreeCAD.Vector(0,0,1))) > 1:
            p.reverse()
        p.Placement = obj.Placement
        obj.Shape = p

    def onChanged(self,obj,prop):

        # clean svg cache if needed
        if prop in ["Placement","Objects","OnlySolids","UseMaterialColorForFill"]:
            self.svgcache = None

    def getNormal(self,obj):

        return obj.Shape.Faces[0].normalAt(0,0)

    def __getstate__(self):

        return None

    def __setstate__(self,state):

        return None


class _ViewProviderSectionPlane:

    "A View Provider for Section Planes"

    def __init__(self,vobj):

        vobj.Proxy = self
        self.setProperties(vobj)

    def setProperties(self,vobj):

        pl = vobj.PropertiesList
        d = 0
        if "DisplaySize" in pl:
            d = vobj.DisplaySize.Value
            vobj.removeProperty("DisplaySize")
        if not "DisplayLength" in pl:
            vobj.addProperty("App::PropertyLength","DisplayLength","SectionPlane",QT_TRANSLATE_NOOP("App::Property","The display length of this section plane"))
            if d:
                vobj.DisplayLength = d
            else:
                vobj.DisplayLength = 1000
        if not "DisplayHeight" in pl:
            vobj.addProperty("App::PropertyLength","DisplayHeight","SectionPlane",QT_TRANSLATE_NOOP("App::Property","The display height of this section plane"))
            if d:
                vobj.DisplayHeight = d
            else:
                vobj.DisplayHeight = 1000
        if not "ArrowSize" in pl:
            vobj.addProperty("App::PropertyLength","ArrowSize","SectionPlane",QT_TRANSLATE_NOOP("App::Property","The size of the arrows of this section plane"))
            vobj.ArrowSize = 50
        if not "Transparency" in pl:
            vobj.addProperty("App::PropertyPercent","Transparency","SectionPlane",QT_TRANSLATE_NOOP("App::Property","The transparency of this object"))
            vobj.Transparency = 85
        if not "LineWidth" in pl:
            vobj.addProperty("App::PropertyFloat","LineWidth","SectionPlane",QT_TRANSLATE_NOOP("App::Property","The line width of this object"))
            vobj.LineWidth = 1
        if not "CutDistance" in pl:
            vobj.addProperty("App::PropertyLength","CutDistance","SectionPlane",QT_TRANSLATE_NOOP("App::Property","Show the cut in the 3D view"))
        if not "LineColor" in pl:
            vobj.addProperty("App::PropertyColor","LineColor","SectionPlane",QT_TRANSLATE_NOOP("App::Property","The color of this object"))
            vobj.LineColor = ArchCommands.getDefaultColor("Helpers")
        if not "CutView" in pl:
            vobj.addProperty("App::PropertyBool","CutView","SectionPlane",QT_TRANSLATE_NOOP("App::Property","Show the cut in the 3D view"))
        if not "CutMargin" in pl:
            vobj.addProperty("App::PropertyLength","CutMargin","SectionPlane",QT_TRANSLATE_NOOP("App::Property","The distance between the cut plane and the actual view cut (keep this a very small value but not zero)"))
            vobj.CutMargin = 1

    def onDocumentRestored(self,vobj):

        self.setProperties(vobj)

    def getIcon(self):

        import Arch_rc
        return ":/icons/Arch_SectionPlane_Tree.svg"

    def claimChildren(self):
        # buggy at the moment so it's disabled - it will for ex. swallow a building object directly at the root of the document...
        #if hasattr(self,"Object") and hasattr(self.Object,"Objects"):
        #    return self.Object.Objects
        return []

    def attach(self,vobj):

        self.Object = vobj.Object
        self.clip = None
        self.mat1 = coin.SoMaterial()
        self.mat2 = coin.SoMaterial()
        self.fcoords = coin.SoCoordinate3()
        #fs = coin.SoType.fromName("SoBrepFaceSet").createInstance() # this causes a FreeCAD freeze for me
        fs = coin.SoIndexedFaceSet()
        fs.coordIndex.setValues(0,7,[0,1,2,-1,0,2,3])
        self.drawstyle = coin.SoDrawStyle()
        self.drawstyle.style = coin.SoDrawStyle.LINES
        self.lcoords = coin.SoCoordinate3()
        ls = coin.SoType.fromName("SoBrepEdgeSet").createInstance()
        ls.coordIndex.setValues(0,57,[0,1,-1,2,3,4,5,-1,6,7,8,9,-1,10,11,-1,12,13,14,15,-1,16,17,18,19,-1,20,21,-1,22,23,24,25,-1,26,27,28,29,-1,30,31,-1,32,33,34,35,-1,36,37,38,39,-1,40,41,42,43,44])
        sep = coin.SoSeparator()
        psep = coin.SoSeparator()
        fsep = coin.SoSeparator()
        fsep.addChild(self.mat2)
        fsep.addChild(self.fcoords)
        fsep.addChild(fs)
        psep.addChild(self.mat1)
        psep.addChild(self.drawstyle)
        psep.addChild(self.lcoords)
        psep.addChild(ls)
        sep.addChild(fsep)
        sep.addChild(psep)
        vobj.addDisplayMode(sep,"Default")
        self.onChanged(vobj,"DisplayLength")
        self.onChanged(vobj,"LineColor")
        self.onChanged(vobj,"Transparency")
        self.onChanged(vobj,"CutView")

    def getDisplayModes(self,vobj):

        return ["Default"]

    def getDefaultDisplayMode(self):

        return "Default"

    def setDisplayMode(self,mode):

        return mode

    def updateData(self,obj,prop):

        if prop in ["Placement"]:
            self.onChanged(obj.ViewObject,"DisplayLength")
            self.onChanged(obj.ViewObject,"CutView")
        return

    def onChanged(self,vobj,prop):

        if prop == "LineColor":
            if hasattr(vobj,"LineColor"):
                l = vobj.LineColor
                self.mat1.diffuseColor.setValue([l[0],l[1],l[2]])
                self.mat2.diffuseColor.setValue([l[0],l[1],l[2]])
        elif prop == "Transparency":
            if hasattr(vobj,"Transparency"):
                self.mat2.transparency.setValue(vobj.Transparency/100.0)
        elif prop in ["DisplayLength","DisplayHeight","ArrowSize"]:
            if hasattr(vobj,"DisplayLength") and hasattr(vobj,"DisplayHeight"):
                ld = vobj.DisplayLength.Value/2
                hd = vobj.DisplayHeight.Value/2
            elif hasattr(vobj,"DisplaySize"):
                # old objects
                ld = vobj.DisplaySize.Value/2
                hd = vobj.DisplaySize.Value/2
            else:
                ld = 1
                hd = 1
            verts = []
            fverts = []
            for v in [[-ld,-hd],[ld,-hd],[ld,hd],[-ld,hd]]:
                if hasattr(vobj,"ArrowSize"):
                    l1 = vobj.ArrowSize.Value if vobj.ArrowSize.Value > 0 else 0.1
                else:
                    l1 = 0.1
                l2 = l1/3
                pl = FreeCAD.Placement(vobj.Object.Placement)
                p1 = pl.multVec(Vector(v[0],v[1],0))
                p2 = pl.multVec(Vector(v[0],v[1],-l1))
                p3 = pl.multVec(Vector(v[0]-l2,v[1],-l1+l2))
                p4 = pl.multVec(Vector(v[0]+l2,v[1],-l1+l2))
                p5 = pl.multVec(Vector(v[0],v[1]-l2,-l1+l2))
                p6 = pl.multVec(Vector(v[0],v[1]+l2,-l1+l2))
                verts.extend([[p1.x,p1.y,p1.z],[p2.x,p2.y,p2.z]])
                fverts.append([p1.x,p1.y,p1.z])
                verts.extend([[p2.x,p2.y,p2.z],[p3.x,p3.y,p3.z],[p4.x,p4.y,p4.z],[p2.x,p2.y,p2.z]])
                verts.extend([[p2.x,p2.y,p2.z],[p5.x,p5.y,p5.z],[p6.x,p6.y,p6.z],[p2.x,p2.y,p2.z]])
            verts.extend(fverts+[fverts[0]])
            self.lcoords.point.setValues(verts)
            self.fcoords.point.setValues(fverts)
        elif prop == "LineWidth":
            self.drawstyle.lineWidth = vobj.LineWidth
        elif prop in ["CutView","CutMargin"]:
            if hasattr(vobj,"CutView") and FreeCADGui.ActiveDocument.ActiveView:
                sg = FreeCADGui.ActiveDocument.ActiveView.getSceneGraph()
                if vobj.CutView:
                    if self.clip:
                        sg.removeChild(self.clip)
                        self.clip = None
                    for o in Draft.getGroupContents(vobj.Object.Objects,walls=True):
                        if hasattr(o.ViewObject,"Lighting"):
                            o.ViewObject.Lighting = "One side"
                    self.clip = coin.SoClipPlane()
                    self.clip.on.setValue(True)
                    norm = vobj.Object.Proxy.getNormal(vobj.Object)
                    mp = vobj.Object.Shape.CenterOfMass
                    mp = DraftVecUtils.project(mp,norm)
                    dist = mp.Length #- 0.1 # to not clip exactly on the section object
                    norm = norm.negative()
                    marg = 1
                    if hasattr(vobj,"CutMargin"):
                        marg = vobj.CutMargin.Value
                    if mp.getAngle(norm) > 1:
                        dist += marg
                        dist = -dist
                    else:
                        dist -= marg
                    plane = coin.SbPlane(coin.SbVec3f(norm.x,norm.y,norm.z),dist)
                    self.clip.plane.setValue(plane)
                    sg.insertChild(self.clip,0)
                else:
                    if self.clip:
                        sg.removeChild(self.clip)
                        self.clip = None
        return

    def __getstate__(self):

        return None

    def __setstate__(self,state):

        return None

    def setEdit(self,vobj,mode):

        taskd = SectionPlaneTaskPanel()
        taskd.obj = vobj.Object
        taskd.update()
        FreeCADGui.Control.showDialog(taskd)
        return True

    def unsetEdit(self,vobj,mode):

        FreeCADGui.Control.closeDialog()
        return False

    def doubleClicked(self,vobj):

        self.setEdit(vobj,None)


class _ArchDrawingView:

    def __init__(self, obj):

        obj.Proxy = self
        self.setProperties(obj)

    def setProperties(self,obj):

        pl = obj.PropertiesList
        if not "Source" in pl:
            obj.addProperty("App::PropertyLink", "Source", "Base", QT_TRANSLATE_NOOP("App::Property","The linked object"))
        if not "RenderingMode" in pl:
            obj.addProperty("App::PropertyEnumeration", "RenderingMode", "Drawing view", QT_TRANSLATE_NOOP("App::Property","The rendering mode to use"))
            obj.RenderingMode = ["Solid","Wireframe"]
            obj.RenderingMode = "Wireframe"
        if not "ShowCut" in pl:
            obj.addProperty("App::PropertyBool", "ShowCut", "Drawing view", QT_TRANSLATE_NOOP("App::Property","If cut geometry is shown or not"))
        if not "ShowFill" in pl:
            obj.addProperty("App::PropertyBool", "ShowFill", "Drawing view", QT_TRANSLATE_NOOP("App::Property","If cut geometry is filled or not"))
        if not "LineWidth" in pl:
            obj.addProperty("App::PropertyFloat", "LineWidth", "Drawing view", QT_TRANSLATE_NOOP("App::Property","The line width of the rendered objects"))
            obj.LineWidth = 0.35
        if not "FontSize" in pl:
            obj.addProperty("App::PropertyLength", "FontSize", "Drawing view", QT_TRANSLATE_NOOP("App::Property","The size of the texts inside this object"))
            obj.FontSize = 12
        if not "AlwaysOn" in pl:
            obj.addProperty("App::PropertyBool", "AlwaysOn", "Drawing view", QT_TRANSLATE_NOOP("App::Property","If checked, source objects are displayed regardless of being visible in the 3D model"))
        if not "LineColor" in pl:
            obj.addProperty("App::PropertyColor", "LineColor", "Drawing view",QT_TRANSLATE_NOOP("App::Property","The line color of the projected objects"))
        if not "FillColor" in pl:
            obj.addProperty("App::PropertyColor", "FillColor", "Drawing view",QT_TRANSLATE_NOOP("App::Property","The color of the cut faces (if turned on)"))
            obj.FillColor = (0.8, 0.8, 0.8)
        self.Type = "ArchSectionView"

    def onDocumentRestored(self, obj):

        self.setProperties(obj)

    def execute(self, obj):

        if hasattr(obj,"Source"):
            if obj.Source:
                svgbody = getSVG(section=obj.Source,
                                 renderMode=obj.RenderingMode,
                                 allOn=getattr(obj, 'AlwaysOn', False),
                                 showHidden=obj.ShowCut,
                                 scale=obj.Scale,
                                 linewidth=obj.LineWidth,
                                 lineColor=obj.LineColor,
                                 fontsize=obj.FontSize,
                                 showFill=obj.ShowFill,
                                 fillColor=obj.FillColor)
                if svgbody:
                    result = '<g id="' + obj.Name + '"'
                    result += ' transform="'
                    result += 'rotate('+str(obj.Rotation)+','+str(obj.X)+','+str(obj.Y)+') '
                    result += 'translate('+str(obj.X)+','+str(obj.Y)+') '
                    result += 'scale('+str(obj.Scale)+','+str(obj.Scale)+')'
                    result += '">\n'
                    result += svgbody
                    result += '</g>\n'
                    obj.ViewResult = result

    def __getstate__(self):

        return self.Type

    def __setstate__(self,state):

        if state:
            self.Type = state

    def getDisplayModes(self,vobj):

        modes=["Default"]
        return modes

    def setDisplayMode(self,mode):

        return mode

    def getDXF(self,obj):

        "returns a DXF representation of the view"
        if obj.RenderingMode == "Solid":
            print("Unable to get DXF from Solid mode: ",obj.Label)
            return ""
        result = []
        import Drawing
        if not hasattr(self,"baseshape"):
            self.onChanged(obj,"Source")
        if hasattr(self,"baseshape"):
            if self.baseshape:
                result.append(Drawing.projectToDXF(self.baseshape,self.direction))
        if hasattr(self,"sectionshape"):
            if self.sectionshape:
                result.append(Drawing.projectToDXF(self.sectionshape,self.direction))
        if hasattr(self,"hiddenshape"):
            if self.hiddenshape:
                result.append(Drawing.projectToDXF(self.hiddenshape,self.direction))
        return result


class SectionPlaneTaskPanel:

    '''A TaskPanel for all the section plane object'''

    def __init__(self):

        # the panel has a tree widget that contains categories
        # for the subcomponents, such as additions, subtractions.
        # the categories are shown only if they are not empty.

        self.obj = None
        self.form = QtGui.QWidget()
        self.form.setObjectName("TaskPanel")
        self.grid = QtGui.QGridLayout(self.form)
        self.title = QtGui.QLabel(self.form)
        self.grid.addWidget(self.title, 0, 0, 1, 2)

        # tree
        self.tree = QtGui.QTreeWidget(self.form)
        self.grid.addWidget(self.tree, 1, 0, 1, 2)
        self.tree.setColumnCount(1)
        self.tree.header().hide()

        # add / remove buttons
        self.addButton = QtGui.QPushButton(self.form)
        self.addButton.setIcon(QtGui.QIcon(":/icons/Arch_Add.svg"))
        self.grid.addWidget(self.addButton, 3, 0, 1, 1)

        self.delButton = QtGui.QPushButton(self.form)
        self.delButton.setIcon(QtGui.QIcon(":/icons/Arch_Remove.svg"))
        self.grid.addWidget(self.delButton, 3, 1, 1, 1)
        
        # rotate / resize buttons
        self.rlabel = QtGui.QLabel(self.form)
        self.grid.addWidget(self.rlabel, 4, 0, 1, 2)
        self.rotateXButton = QtGui.QPushButton(self.form)
        self.grid.addWidget(self.rotateXButton, 5, 0, 1, 1)
        self.rotateYButton = QtGui.QPushButton(self.form)
        self.grid.addWidget(self.rotateYButton, 5, 1, 1, 1)
        self.rotateZButton = QtGui.QPushButton(self.form)
        self.grid.addWidget(self.rotateZButton, 6, 0, 1, 1)
        self.resizeButton = QtGui.QPushButton(self.form)
        self.grid.addWidget(self.resizeButton, 7, 0, 1, 1)
        self.recenterButton = QtGui.QPushButton(self.form)
        self.grid.addWidget(self.recenterButton, 7, 1, 1, 1)

        QtCore.QObject.connect(self.addButton, QtCore.SIGNAL("clicked()"), self.addElement)
        QtCore.QObject.connect(self.delButton, QtCore.SIGNAL("clicked()"), self.removeElement)
        QtCore.QObject.connect(self.rotateXButton, QtCore.SIGNAL("clicked()"), self.rotateX)
        QtCore.QObject.connect(self.rotateYButton, QtCore.SIGNAL("clicked()"), self.rotateY)
        QtCore.QObject.connect(self.rotateZButton, QtCore.SIGNAL("clicked()"), self.rotateZ)
        QtCore.QObject.connect(self.resizeButton, QtCore.SIGNAL("clicked()"), self.resize)
        QtCore.QObject.connect(self.recenterButton, QtCore.SIGNAL("clicked()"), self.recenter)
        self.update()

    def isAllowedAlterSelection(self):
        return True

    def isAllowedAlterView(self):
        return True

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Ok)

    def getIcon(self,obj):
        if hasattr(obj.ViewObject,"Proxy"):
            return QtGui.QIcon(obj.ViewObject.Proxy.getIcon())
        elif obj.isDerivedFrom("Sketcher::SketchObject"):
            return QtGui.QIcon(":/icons/Sketcher_Sketch.svg")
        elif obj.isDerivedFrom("App::DocumentObjectGroup"):
            return QtGui.QApplication.style().standardIcon(QtGui.QStyle.SP_DirIcon)
        else:
            return QtGui.QIcon(":/icons/Tree_Part.svg")

    def update(self):
        'fills the treewidget'
        self.tree.clear()
        if self.obj:
            for o in self.obj.Objects:
                item = QtGui.QTreeWidgetItem(self.tree)
                item.setText(0,o.Label)
                item.setToolTip(0,o.Name)
                item.setIcon(0,self.getIcon(o))
        self.retranslateUi(self.form)

    def addElement(self):
        if self.obj:
            for o in FreeCADGui.Selection.getSelection():
                ArchComponent.addToComponent(self.obj,o,"Objects")
            self.update()

    def removeElement(self):
        if self.obj:
            it = self.tree.currentItem()
            if it:
                comp = FreeCAD.ActiveDocument.getObject(str(it.toolTip(0)))
                ArchComponent.removeFromComponent(self.obj,comp)
            self.update()
    
    def rotate(self,axis):
        if self.obj and self.obj.Shape and self.obj.Shape.Faces:
            face = self.obj.Shape.copy()
            import Part
            face.rotate(self.obj.Placement.Base, axis, 90)
            self.obj.Placement = face.Placement
            self.obj.Proxy.execute(self.obj)
    
    def rotateX(self):
        self.rotate(FreeCAD.Vector(1,0,0))

    def rotateY(self):
        self.rotate(FreeCAD.Vector(0,1,0))

    def rotateZ(self):
        self.rotate(FreeCAD.Vector(0,0,1))
    
    def getBB(self):
        bb = FreeCAD.BoundBox()
        if self.obj:
            for o in Draft.getGroupContents(self.obj.Objects):
                if hasattr(o,"Shape") and hasattr(o.Shape,"BoundBox"):
                    bb.add(o.Shape.BoundBox)
        return bb

    def resize(self):
        if self.obj and self.obj.ViewObject:
            bb = self.getBB()
            n = self.obj.Proxy.getNormal(self.obj)
            margin = bb.XLength*0.1
            if (n.getAngle(FreeCAD.Vector(1,0,0)) < 0.1) or (n.getAngle(FreeCAD.Vector(-1,0,0)) < 0.1):
                self.obj.ViewObject.DisplayLength = bb.YLength+margin
                self.obj.ViewObject.DisplayHeight = bb.ZLength+margin
            elif (n.getAngle(FreeCAD.Vector(0,1,0)) < 0.1) or (n.getAngle(FreeCAD.Vector(0,-1,0)) < 0.1):
                self.obj.ViewObject.DisplayLength = bb.XLength+margin
                self.obj.ViewObject.DisplayHeight = bb.ZLength+margin
            elif (n.getAngle(FreeCAD.Vector(0,0,1)) < 0.1) or (n.getAngle(FreeCAD.Vector(0,0,-1)) < 0.1):
                self.obj.ViewObject.DisplayLength = bb.XLength+margin
                self.obj.ViewObject.DisplayHeight = bb.YLength+margin
            self.obj.Proxy.execute(self.obj)

    def recenter(self):
        if self.obj:
            self.obj.Placement.Base = self.getBB().Center

    def accept(self):
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.ActiveDocument.resetEdit()
        return True

    def retranslateUi(self, TaskPanel):
        TaskPanel.setWindowTitle(QtGui.QApplication.translate("Arch", "Section plane settings", None))
        self.delButton.setText(QtGui.QApplication.translate("Arch", "Remove", None))
        self.addButton.setText(QtGui.QApplication.translate("Arch", "Add", None))
        self.title.setText(QtGui.QApplication.translate("Arch", "Objects seen by this section plane:", None))
        self.rlabel.setText(QtGui.QApplication.translate("Arch", "Section plane placement:", None))
        self.rotateXButton.setText(QtGui.QApplication.translate("Arch", "Rotate X", None))
        self.rotateYButton.setText(QtGui.QApplication.translate("Arch", "Rotate Y", None))
        self.rotateZButton.setText(QtGui.QApplication.translate("Arch", "Rotate Z", None))
        self.resizeButton.setText(QtGui.QApplication.translate("Arch", "Resize", None))
        self.recenterButton.setText(QtGui.QApplication.translate("Arch", "Center", None))

if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Arch_SectionPlane',_CommandSectionPlane())
