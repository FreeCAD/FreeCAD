#***************************************************************************
#*   Copyright (c) 2011 Yorik van Havre <yorik@uncreated.net>              *
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

import FreeCAD
import math
import Draft
import ArchCommands
import DraftVecUtils
import ArchComponent
import re
import tempfile
import uuid
import time

from FreeCAD import Vector
if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore, QtGui
    from draftutils.translate import translate
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
#  section planes, to be used in the TechDraw module

ISRENDERING = False # flag to prevent concurrent runs of the coin renderer

def makeSectionPlane(objectslist=None,name=None):

    """makeSectionPlane([objectslist],[name]) : Creates a Section plane objects including the
    given objects. If no object is given, the whole document will be considered."""

    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    obj = FreeCAD.ActiveDocument.addObject("App::FeaturePython","Section")
    obj.Label = name if name else translate("Arch","Section")
    _SectionPlane(obj)
    if FreeCAD.GuiUp:
        _ViewProviderSectionPlane(obj.ViewObject)
    if objectslist:
        obj.Objects = objectslist
        bb = FreeCAD.BoundBox()
        for o in Draft.get_group_contents(objectslist):
            if hasattr(o,"Shape") and hasattr(o.Shape,"BoundBox"):
                bb.add(o.Shape.BoundBox)
        obj.Placement = FreeCAD.DraftWorkingPlane.getPlacement()
        obj.Placement.Base = bb.Center
        if FreeCAD.GuiUp:
            margin = bb.XLength*0.1
            obj.ViewObject.DisplayLength = bb.XLength+margin
            obj.ViewObject.DisplayHeight = bb.YLength+margin
    return obj


def getSectionData(source):

    """Returns some common data from section planes and building parts"""

    if hasattr(source,"Objects"):
        objs = source.Objects
        cutplane = source.Shape
    elif hasattr(source,"Group"):
        import Part
        objs = source.Group
        cutplane = Part.makePlane(1000,1000,FreeCAD.Vector(-500,-500,0))
        m = 1
        if source.ViewObject and hasattr(source.ViewObject,"CutMargin"):
            m = source.ViewObject.CutMargin.Value
        cutplane.translate(FreeCAD.Vector(0,0,m))
        cutplane.Placement = cutplane.Placement.multiply(source.Placement)
    onlySolids = True
    if hasattr(source,"OnlySolids"):
        onlySolids = source.OnlySolids
    clip = False
    if hasattr(source,"Clip"):
        clip = source.Clip
    p = FreeCAD.Placement(source.Placement)
    direction = p.Rotation.multVec(FreeCAD.Vector(0,0,1))
    if objs:
        objs = Draft.get_group_contents(objs, walls=True, addgroups=True)
    return objs,cutplane,onlySolids,clip,direction


def getCutShapes(objs,cutplane,onlySolids,clip,joinArch,showHidden,groupSshapesByObject=False):

    """
    returns a list of shapes (visible, hidden, cut lines...)
    obtained from performing a series of booleans against the given cut plane
    """

    import Part,DraftGeomUtils
    shapes = []
    hshapes = []
    sshapes = []
    objectShapes = []
    objectSshapes = []

    if joinArch:
        shtypes = {}
        for o in objs:
            if Draft.getType(o) in ["Wall","Structure"]:
                if o.Shape.isNull():
                    pass
                elif onlySolids:
                    shtypes.setdefault(o.Material.Name if (hasattr(o,"Material") and o.Material) else "None",[]).extend(o.Shape.Solids)
                else:
                    shtypes.setdefault(o.Material.Name if (hasattr(o,"Material") and o.Material) else "None",[]).append(o.Shape.copy())
            elif hasattr(o,'Shape'):
                if o.Shape.isNull():
                    pass
                elif onlySolids:
                    shapes.extend(o.Shape.Solids)
                    objectShapes.append((o, o.Shape.Solids))
                else:
                    shapes.append(o.Shape.copy())
                    objectShapes.append((o,[o.Shape.copy()]))
        for k,v in shtypes.items():
            v1 = v.pop()
            if v:
                v1 = v1.multiFuse(v)
                v1 = v1.removeSplitter()
            if v1.Solids:
                shapes.extend(v1.Solids)
                objectShapes.append((k,v1.Solids))
            else:
                print("ArchSectionPlane: Fusing Arch objects produced non-solid results")
                shapes.append(v1)
                objectShapes.append((k,[v1]))
    else:
        for o in objs:
            if hasattr(o,'Shape'):
                if o.Shape.isNull():
                    pass
                elif onlySolids:
                    if o.Shape.isValid():
                        shapes.extend(o.Shape.Solids)
                        objectShapes.append((o,o.Shape.Solids))
                else:
                    shapes.append(o.Shape)
                    objectShapes.append((o,[o.Shape]))

    cutface,cutvolume,invcutvolume = ArchCommands.getCutVolume(cutplane,shapes,clip)
    shapes = []
    for o, shapeList in objectShapes:
        tmpSshapes = []
        for sh in shapeList:
            for sub in (sh.SubShapes if sh.ShapeType == "Compound" else [sh]):
                if cutvolume:
                    if sub.Volume < 0:
                        sub = sub.reversed() # Use reversed as sub is immutable.
                    c = sub.cut(cutvolume)
                    s = sub.section(cutface)
                    try:
                        wires = DraftGeomUtils.findWires(s.Edges)
                        for w in wires:
                            f = Part.Face(w)
                            tmpSshapes.append(f)
                    except Part.OCCError:
                        #print "ArchView: unable to get a face"
                        tmpSshapes.append(s)
                    shapes.extend(c.SubShapes if c.ShapeType == "Compound" else [c])
                    if showHidden:
                        c = sub.cut(invcutvolume)
                        hshapes.extend(c.SubShapes if c.ShapeType == "Compound" else [c])
                else:
                    shapes.append(sub)

            if len(tmpSshapes) > 0:
                sshapes.extend(tmpSshapes)

                if groupSshapesByObject:
                    objectSshapes.append((o, tmpSshapes))

    if groupSshapesByObject:
        return shapes,hshapes,sshapes,cutface,cutvolume,invcutvolume,objectSshapes
    else:
        return shapes,hshapes,sshapes,cutface,cutvolume,invcutvolume


def getFillForObject(o, defaultFill, source):

    """returns a color tuple from an object's material"""

    if hasattr(source, 'UseMaterialColorForFill') and source.UseMaterialColorForFill:
        material = None
        if hasattr(o, 'Material') and o.Material:
            material = o.Material
        elif isinstance(o,str):
            material = FreeCAD.ActiveDocument.getObject(o)
        if material:
            if hasattr(material, 'SectionColor') and material.SectionColor:
                return material.SectionColor
            elif hasattr(material, 'Color') and material.Color:
                return material.Color
        elif hasattr(o,"ViewObject") and hasattr(o.ViewObject,"ShapeColor"):
            return o.ViewObject.ShapeColor
    return defaultFill


def isOriented(obj,plane):

    """determines if an annotation is facing the cutplane or not"""

    norm1 = plane.normalAt(0,0)
    if hasattr(obj,"Placement"):
        norm2 = obj.Placement.Rotation.multVec(FreeCAD.Vector(0,0,1))
    elif hasattr(obj,"Normal"):
        norm2 = obj.Normal
        if norm2.Length < 0.01:
            return True
    else:
        return True
    a = norm1.getAngle(norm2)
    if (a < 0.01) or (abs(a-math.pi) < 0.01):
        return True
    return False

def update_svg_cache(source, renderMode, showHidden, showFill, fillSpaces, joinArch, allOn, objs):
    """
    Returns None or cached SVG, clears shape cache if required
    """
    svgcache = None
    if hasattr(source,"Proxy"):
        if hasattr(source.Proxy,"svgcache") and source.Proxy.svgcache:
            # TODO check array bounds
            svgcache = source.Proxy.svgcache[0]
            # empty caches if we want to force-recalculate for certain properties
            if (source.Proxy.svgcache[1] != renderMode
              or source.Proxy.svgcache[2] != showHidden
              or source.Proxy.svgcache[3] != showFill
              or source.Proxy.svgcache[4] != fillSpaces
              or source.Proxy.svgcache[5] != joinArch
              or source.Proxy.svgcache[6] != allOn
              or source.Proxy.svgcache[7] != set(objs)):
                svgcache = None
            if (source.Proxy.svgcache[4] != fillSpaces
              or source.Proxy.svgcache[5] != joinArch
              or source.Proxy.svgcache[6] != allOn
              or source.Proxy.svgcache[7] != set(objs)):
                source.Proxy.shapecache = None
    return svgcache


def getSVG(source,
           renderMode="Wireframe",
           allOn=False,
           showHidden=False,
           scale=1,
           rotation=0,
           linewidth=1,
           lineColor=(0.0, 0.0, 0.0),
           fontsize=1,
           linespacing=None,
           showFill=False,
           fillColor=(1.0, 1.0, 1.0),
           techdraw=False,
           fillSpaces=False,
           cutlinewidth=0,
           joinArch=False):
    """
    Return an SVG fragment from an Arch SectionPlane or BuildingPart.

    allOn
        If it is `True`, all cut objects are shown, regardless of if they are
        visible or not.

    renderMode
        Can be `'Wireframe'` (default) or `'Solid'` to use the Arch solid
        renderer.

    showHidden
        If it is `True`, the hidden geometry above the section plane
        is shown in dashed line.

    showFill
        If it is `True`, the cut areas get filled with a pattern.

    lineColor
        Color of lines for the `renderMode` is `'Wireframe'`.

    fillColor
        If `showFill` is `True` and `renderMode` is `'Wireframe'`,
        the cut areas are filled with `fillColor`.

    fillSpaces
        If `True`, shows space objects as filled surfaces.
    """
    import Part

    objs, cutplane, onlySolids, clip, direction = getSectionData(source)
    if not objs:
        return ""
    if not allOn:
        objs = Draft.removeHidden(objs)

    # separate spaces and Draft objects
    spaces = []
    nonspaces = []
    drafts = [] # Only used for annotations.
    windows = []
    cutface = None
    for o in objs:
        if Draft.getType(o) == "Space":
            spaces.append(o)
        elif Draft.getType(o) in ["Dimension","AngularDimension","LinearDimension","Annotation","Label","Text","DraftText"]:
            if isOriented(o,cutplane):
                drafts.append(o)
        elif o.isDerivedFrom("App::DocumentObjectGroup"):
            # These will have been expanded by getSectionData already
            pass
        else:
            nonspaces.append(o)
        if Draft.getType(o.getLinkedObject()) == "Window":  # To support Link of Windows(Doors)
            windows.append(o)
    objs = nonspaces

    archUserParameters = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
    scaledLineWidth = linewidth/scale
    if renderMode in ["Coin",2,"Coin mono",3]:
        # don't scale linewidths in coin mode
        svgLineWidth = str(linewidth) + 'px'
    else:
        svgLineWidth = str(scaledLineWidth) + 'px'
    if cutlinewidth:
        scaledCutLineWidth = cutlinewidth/scale
        svgCutLineWidth = str(scaledCutLineWidth) + 'px'
    else:
        st = archUserParameters.GetFloat("CutLineThickness",2)
        svgCutLineWidth = str(scaledLineWidth * st) + 'px'
    yt = archUserParameters.GetFloat("SymbolLineThickness",0.6)
    svgSymbolLineWidth = str(linewidth * yt)
    hiddenPattern = archUserParameters.GetString("archHiddenPattern","30,10")
    svgHiddenPattern = hiddenPattern.replace(" ","")
    #fillpattern = '<pattern id="sectionfill" patternUnits="userSpaceOnUse" patternTransform="matrix(5,0,0,5,0,0)"'
    #fillpattern += ' x="0" y="0" width="10" height="10">'
    #fillpattern += '<g>'
    #fillpattern += '<rect width="10" height="10" style="stroke:none; fill:#ffffff" /><path style="stroke:#000000; stroke-width:1" d="M0,0 l10,10" /></g></pattern>'
    svgLineColor = Draft.getrgb(lineColor)
    svg = ''
    # reading cached version
    svgcache = update_svg_cache(source, renderMode, showHidden, showFill, fillSpaces, joinArch, allOn, objs)
    should_update_svg_cache = False
    if showFill or not svgcache:
        should_update_svg_cache = True

    # generating SVG
    if renderMode in ["Coin",2,"Coin mono",3]:
        # render using a coin viewer
        if hasattr(source.ViewObject,"ViewData") and source.ViewObject.ViewData:
            cameradata = None#getCameraData(source.ViewObject.ViewData)
        else:
            cameradata = None
        if should_update_svg_cache:
            if renderMode in ["Coin mono",3]:
                svgcache = getCoinSVG(cutplane,objs,cameradata,linewidth="SVGLINEWIDTH",facecolor="#ffffff")
            else:
                svgcache = getCoinSVG(cutplane,objs,cameradata,linewidth="SVGLINEWIDTH")
    elif renderMode in ["Solid",1]:
        if should_update_svg_cache:
            svgcache = ''
            # render using the Arch Vector Renderer
            import ArchVRM, WorkingPlane
            wp = WorkingPlane.plane()
            pl = FreeCAD.Placement(source.Placement)
            if source.ViewObject and hasattr(source.ViewObject,"CutMargin"):
                mv = pl.multVec(FreeCAD.Vector(0,0,1))
                mv.multiply(source.ViewObject.CutMargin)
                pl.move(mv)
            wp.setFromPlacement(pl)
            #wp.inverse()
            render = ArchVRM.Renderer()
            render.setWorkingPlane(wp)
            render.addObjects(objs)
            if showHidden:
                render.cut(cutplane,showHidden)
            else:
                render.cut(cutplane)
            g = '<g transform="scale(1,-1)">\n'
            if hasattr(source.ViewObject,"RotateSolidRender"):
                if (source.ViewObject.RotateSolidRender.Value != 0):
                    g = '<g transform="scale(1,-1) rotate('
                    g += str(source.ViewObject.RotateSolidRender.Value)
                    g += ')">\n'
            svgcache += g
            svgcache += render.getViewSVG(linewidth="SVGLINEWIDTH")
            #svgcache += fillpattern
            svgcache += render.getSectionSVG(linewidth="SVGCUTLINEWIDTH",fillpattern="#ffffff")
            if showHidden:
                svgcache += render.getHiddenSVG(linewidth="SVGLINEWIDTH")
            svgcache += '</g>\n'
            # print(render.info())
    else:
        # Wireframe (0) mode

        if hasattr(source,"Proxy") and hasattr(source.Proxy,"shapecache") and source.Proxy.shapecache:
            vshapes = source.Proxy.shapecache[0]
            hshapes = source.Proxy.shapecache[1]
            sshapes = source.Proxy.shapecache[2]
            cutface = source.Proxy.shapecache[3]
            # cutvolume = source.Proxy.shapecache[4] # Unused
            # invcutvolume = source.Proxy.shapecache[5] # Unused
            objectSshapes = source.Proxy.shapecache[6]
        else:
            if showFill:
                vshapes,hshapes,sshapes,cutface,cutvolume,invcutvolume,objectSshapes = getCutShapes(objs,cutplane,onlySolids,clip,joinArch,showHidden,True)
            else:
                vshapes,hshapes,sshapes,cutface,cutvolume,invcutvolume = getCutShapes(objs,cutplane,onlySolids,clip,joinArch,showHidden)
                objectSshapes = []
            source.Proxy.shapecache = [vshapes,hshapes,sshapes,cutface,cutvolume,invcutvolume,objectSshapes]

        if should_update_svg_cache:
            svgcache = ""
            # render using the TechDraw module
            import TechDraw, Part
            if vshapes:
                baseshape = Part.makeCompound(vshapes)
                style = {'stroke':       "SVGLINECOLOR",
                         'stroke-width': "SVGLINEWIDTH"}
                svgcache += TechDraw.projectToSVG(
                    baseshape, direction,
                    hStyle=style, h0Style=style, h1Style=style,
                    vStyle=style, v0Style=style, v1Style=style)
            if hshapes:
                hshapes = Part.makeCompound(hshapes)
                style = {'stroke':           "SVGLINECOLOR",
                         'stroke-width':     "SVGLINEWIDTH",
                         'stroke-dasharray': "SVGHIDDENPATTERN"}
                svgcache += TechDraw.projectToSVG(
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
                                objectFill = getFillForObject(o, fillColor, source)
                                # svg += Draft.get_svg(s,
                                #                      direction=direction.negative(),
                                #                      linewidth=0,
                                #                      fillstyle="sectionfill",
                                #                      color=(0,0,0))
                                # temporarily disabling fill patterns
                                svgcache += Draft.get_svg(s,
                                                          linewidth=0,
                                                          fillstyle=Draft.getrgb(objectFill,testbw=False),
                                                          direction=direction.negative(),
                                                          color=lineColor)
                    svgcache += "</g>\n"
                sshapes = Part.makeCompound(sshapes)
                style = {'stroke':       "SVGLINECOLOR",
                         'stroke-width': "SVGCUTLINEWIDTH"}
                svgcache += TechDraw.projectToSVG(
                    sshapes, direction,
                    hStyle=style, h0Style=style, h1Style=style,
                    vStyle=style, v0Style=style, v1Style=style)
    if should_update_svg_cache:
        if hasattr(source,"Proxy"):
            source.Proxy.svgcache = [svgcache,renderMode,showHidden,showFill,fillSpaces,joinArch,allOn,set(objs)]

    svgcache = svgcache.replace("SVGLINECOLOR",svgLineColor)
    svgcache = svgcache.replace("SVGLINEWIDTH",svgLineWidth)
    svgcache = svgcache.replace("SVGHIDDENPATTERN",svgHiddenPattern)
    svgcache = svgcache.replace("SVGCUTLINEWIDTH",svgCutLineWidth)
    svg += svgcache

    if drafts:
        if not techdraw:
            svg += '<g transform="scale(1,-1)">'
        for d in drafts:
            svg += Draft.get_svg(d,
                                 scale=scale,
                                 linewidth=svgSymbolLineWidth,
                                 fontsize=fontsize,
                                 linespacing=linespacing,
                                 direction=direction,
                                 color=lineColor,
                                 techdraw=techdraw,
                                 rotation=rotation)
        if not techdraw:
            svg += '</g>'

    if not cutface:
        # if we didn't calculate anything better, use the cutplane...
        cutface = cutplane

    # filter out spaces not cut by the source plane
    if cutface and spaces:
        spaces = [s for s in spaces if s.Shape.BoundBox.intersect(cutface.BoundBox)]
    if spaces:
        if not techdraw:
            svg += '<g transform="scale(1,-1)">'
        for s in spaces:
            svg += Draft.get_svg(s,
                                 scale=scale,
                                 linewidth=svgSymbolLineWidth,
                                 fontsize=fontsize,
                                 linespacing=linespacing,
                                 direction=direction,
                                 color=lineColor,
                                 techdraw=techdraw,
                                 rotation=rotation,
                                 fillspaces=fillSpaces)
        if not techdraw:
            svg += '</g>'

    # add additional edge symbols from windows
    cutwindows = []
    if cutface and windows:
        cutwindows = [w.Name for w in windows if w.Shape.BoundBox.intersect(cutface.BoundBox)]
    if windows:
        sh = []
        for w in windows:
            if w.Name in cutwindows:
                wlo = w.getLinkedObject()  # To support Link of Windows(Doors)
                if hasattr(wlo, "SymbolPlan") and wlo.SymbolPlan:
                    if not hasattr(wlo.Proxy, "sshapes"):
                        wlo.Proxy.execute(wlo)
                    if hasattr(wlo.Proxy, "sshapes") and wlo.Proxy.sshapes:
                        c = Part.makeCompound(wlo.Proxy.sshapes)
                        c.Placement = w.Placement
                        sh.append(c)
        if sh:
            if not techdraw:
                svg += '<g transform="scale(1,-1)">'
            for s in sh:
                svg += Draft.get_svg(s,
                                     scale=scale,
                                     linewidth=svgSymbolLineWidth,
                                     fontsize=fontsize,
                                     linespacing=linespacing,
                                     fillstyle="none",
                                     direction=direction,
                                     color=lineColor,
                                     techdraw=techdraw,
                                     rotation=rotation)
            if not techdraw:
                svg += '</g>'

    return svg


def getDXF(obj):
    """Return a DXF representation from a TechDraw view."""
    allOn = getattr(obj, "AllOn", True)
    showHidden = getattr(obj, "ShowHidden", False)
    result = []
    import TechDraw, Part
    if not obj.Source:
        return result
    source = obj.Source
    objs,cutplane,onlySolids,clip,direction = getSectionData(source)
    if not objs:
        return result
    if not allOn:
            objs = Draft.removeHidden(objs)
    objs = [o for o in objs if ((not(Draft.getType(o) in ["Space","Dimension","Annotation"])) and (not (o.isDerivedFrom("Part::Part2DObject"))))]
    vshapes,hshapes,sshapes,cutface,cutvolume,invcutvolume = getCutShapes(objs,cutplane,onlySolids,clip,False,showHidden)
    if vshapes:
        result.append(TechDraw.projectToDXF(Part.makeCompound(vshapes),direction))
    if sshapes:
        result.append(TechDraw.projectToDXF(Part.makeCompound(sshapes),direction))
    if hshapes:
        result.append(TechDraw.projectToDXF(Part.makeCompound(hshapes),direction))
    return result


def getCameraData(floatlist):

    """reconstructs a valid camera data string from stored values"""

    c = ""
    if len(floatlist) >= 12:
        d = floatlist
        camtype = "orthographic"
        if len(floatlist) == 13:
            if d[12] == 1:
                camtype = "perspective"
        if camtype == "orthographic":
            c = "#Inventor V2.1 ascii\n\n\nOrthographicCamera {\n  viewportMapping ADJUST_CAMERA\n  "
        else:
            c = "#Inventor V2.1 ascii\n\n\nPerspectiveCamera {\n  viewportMapping ADJUST_CAMERA\n  "
        c += "position " + str(d[0]) + " " + str(d[1]) + " " + str(d[2]) + "\n  "
        c += "orientation " + str(d[3]) + " " + str(d[4]) + " " + str(d[5]) + "  " + str(d[6]) + "\n  "
        c += "aspectRatio " + str(d[9]) + "\n  "
        c += "focalDistance " + str(d[10]) + "\n  "
        if camtype == "orthographic":
            c += "height " + str(d[11]) + "\n\n}\n"
        else:
            c += "heightAngle " + str(d[11]) + "\n\n}\n"
    return c


def getCoinSVG(cutplane,objs,cameradata=None,linewidth=0.2,singleface=False,facecolor=None):

    """Returns an SVG fragment generated from a coin view"""

    if not FreeCAD.GuiUp:
        return ""

    # do not allow concurrent runs
    # wait until the other rendering has finished
    global ISRENDERING
    while ISRENDERING:
        time.sleep(0.1)

    ISRENDERING = True

    # a name to save a temp file
    svgfile = tempfile.mkstemp(suffix=".svg")[1]

    # set object lighting to single face to get black fills
    # but this creates artifacts in svg output, triangulation gets visible...
    ldict = {}
    if singleface:
        for obj in objs:
            if hasattr(obj,"ViewObject") and hasattr(obj.ViewObject,"Lighting"):
                ldict[obj.Name] = obj.ViewObject.Lighting
                obj.ViewObject.Lighting = "One side"

    # get nodes to render
    root_node = coin.SoSeparator()
    boundbox = FreeCAD.BoundBox()
    for obj in objs:
        if hasattr(obj.ViewObject,"RootNode") and obj.ViewObject.RootNode:
            old_visibility = obj.ViewObject.isVisible()
            # ignore visibility as only visible objects are passed here
            obj.ViewObject.show()
            node_copy = obj.ViewObject.RootNode.copy()
            root_node.addChild(node_copy)
            if(old_visibility):
              obj.ViewObject.show()
            else:
                obj.ViewObject.hide()

        if hasattr(obj,"Shape") and hasattr(obj.Shape,"BoundBox"):
            boundbox.add(obj.Shape.BoundBox)

    # reset lighting of objects
    if ldict:
        for obj in objs:
            if obj.Name in ldict:
                obj.ViewObject.Lighting = ldict[obj.Name]

    # create viewer
    view_window = FreeCADGui.createViewer()
    view_window_name = "Temp" + str(uuid.uuid4().hex[:8])
    view_window.setName(view_window_name)
    inventor_view = view_window.getViewer()

    inventor_view.setBackgroundColor(1,1,1)
    view_window.redraw()

    # set clip plane
    clip = coin.SoClipPlane()
    norm = cutplane.normalAt(0,0).negative()
    proj = DraftVecUtils.project(cutplane.CenterOfMass,norm)
    dist = proj.Length
    if proj.getAngle(norm) > 1:
        dist = -dist
    clip.on = True
    plane = coin.SbPlane(coin.SbVec3f(norm.x,norm.y,norm.z),dist) #dir, position on dir
    clip.plane.setValue(plane)
    root_node.insertChild(clip,0)

    # add white marker at scene bound box corner
    markervec = FreeCAD.Vector(10,10,10)
    a = cutplane.normalAt(0,0).getAngle(markervec)
    if (a < 0.01) or (abs(a-math.pi) < 0.01):
        markervec = FreeCAD.Vector(10,-10,10)
    boundbox.enlarge(10) # so the marker don't overlap the objects
    sep = coin.SoSeparator()
    mat = coin.SoMaterial()
    mat.diffuseColor.setValue([1,1,1])
    sep.addChild(mat)
    coords = coin.SoCoordinate3()
    coords.point.setValues([[boundbox.XMin,boundbox.YMin,boundbox.ZMin],
                            [boundbox.XMin+markervec.x,boundbox.YMin+markervec.y,boundbox.ZMin+markervec.z]])
    sep.addChild(coords)
    lset = coin.SoIndexedLineSet()
    lset.coordIndex.setValues(0,2,[0,1])
    sep.addChild(lset)
    root_node.insertChild(sep,0)

    # set scenegraph
    inventor_view.setSceneGraph(root_node)

    # set camera
    if cameradata:
        view_window.setCamera(cameradata)
    else:
        view_window.setCameraType("Orthographic")
        #rot = FreeCAD.Rotation(FreeCAD.Vector(0,0,1),cutplane.normalAt(0,0))
        vx = cutplane.Placement.Rotation.multVec(FreeCAD.Vector(1,0,0))
        vy = cutplane.Placement.Rotation.multVec(FreeCAD.Vector(0,1,0))
        vz = cutplane.Placement.Rotation.multVec(FreeCAD.Vector(0,0,1))
        rot = FreeCAD.Rotation(vx,vy,vz,"ZXY")
        view_window.setCameraOrientation(rot.Q)
    # this is needed to set correct focal depth, otherwise saving doesn't work properly
    view_window.fitAll()

    # save view
    #print("saving to",svgfile)
    view_window.saveVectorGraphic(svgfile,1) # number is pixel size

    # set linewidth placeholder
    f = open(svgfile,"r")
    svg = f.read()
    f.close()
    svg = svg.replace("stroke-width:1.0;","stroke-width:"+str(linewidth)+";")
    svg = svg.replace("stroke-width=\"1px","stroke-width=\""+str(linewidth))

    # find marker and calculate scale factor and translation
    # <line x1="284.986" y1="356.166" x2="285.038" y2="356.166" stroke="#ffffff" stroke-width="1px" />
    factor = None
    trans = None
    import WorkingPlane
    wp = WorkingPlane.plane()
    wp.alignToPointAndAxis_SVG(Vector(0,0,0),cutplane.normalAt(0,0),0)
    p = wp.getLocalCoords(markervec)
    orlength = FreeCAD.Vector(p.x,p.y,0).Length
    marker = re.findall("<line x1=.*?stroke=\"\#ffffff\".*?\/>",svg)
    if marker:
        marker = marker[0].split("\"")
        x1 = float(marker[1])
        y1 = float(marker[3])
        x2 = float(marker[5])
        y2 = float(marker[7])
        p1 = FreeCAD.Vector(x1,y1,0)
        p2 = FreeCAD.Vector(x2,y2,0)
        factor = orlength/p2.sub(p1).Length
        if factor:
            orig = wp.getLocalCoords(FreeCAD.Vector(boundbox.XMin,boundbox.YMin,boundbox.ZMin))
            orig = FreeCAD.Vector(orig.x,-orig.y,0)
            scaledp1 = FreeCAD.Vector(p1.x*factor,p1.y*factor,0)
            trans = orig.sub(scaledp1)
        # remove marker
        svg = re.sub("<line x1=.*?stroke=\"\#ffffff\".*?\/>","",svg,count=1)

    # remove background rectangle
    svg = re.sub("<path.*?>","",svg,count=1,flags=re.MULTILINE|re.DOTALL)

    # set face color to white
    if facecolor:
        res = re.findall("fill:(.*?); stroke:(.*?);",svg)
        pairs = []
        for pair in res:
            if (pair not in pairs) and (pair[0] == pair[1]) and(pair[0] not in ["#0a0a0a"]):
                # coin seems to be rendering a lot of lines as thin triangles with the #0a0a0a color...
                pairs.append(pair)
        for pair in pairs:
            svg = re.sub("fill:"+pair[0]+"; stroke:"+pair[1]+";","fill:"+facecolor+"; stroke:"+facecolor+";",svg)

    # embed everything in a scale group and scale the viewport
    if factor:
        if trans:
            svg = svg.replace("<g>","<g transform=\"translate("+str(trans.x)+" "+str(trans.y)+") scale("+str(factor)+","+str(factor)+")\">\n<g>",1)
        else:
            svg = svg.replace("<g>","<g transform=\"scale("+str(factor)+","+str(factor)+")\">\n<g>",1)
        svg = svg.replace("</svg>","</g>\n</svg>")

    # trigger viewer close
    QtCore.QTimer.singleShot(1,lambda: closeViewer(view_window_name))

    # strip svg tags (needed for TD Arch view)
    svg = re.sub("<\?xml.*?>","",svg,flags=re.MULTILINE|re.DOTALL)
    svg = re.sub("<svg.*?>","",svg,flags=re.MULTILINE|re.DOTALL)
    svg = re.sub("<\/svg>","",svg,flags=re.MULTILINE|re.DOTALL)

    ISRENDERING = False

    return svg


def closeViewer(name):

    """Close temporary viewers"""

    mw = FreeCADGui.getMainWindow()
    for sw in mw.findChildren(QtGui.QMdiSubWindow):
        if sw.windowTitle() == name:
            sw.close()



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
        if not "Clip" in pl:
            obj.addProperty("App::PropertyBool","Clip","SectionPlane",QT_TRANSLATE_NOOP("App::Property","If True, resulting views will be clipped to the section plane area."))
        if not "UseMaterialColorForFill" in pl:
            obj.addProperty("App::PropertyBool","UseMaterialColorForFill","SectionPlane",QT_TRANSLATE_NOOP("App::Property","If true, the color of the objects material will be used to fill cut areas."))
            obj.UseMaterialColorForFill = False
        if not "Depth" in pl:
            obj.addProperty("App::PropertyLength","Depth","SectionPlane",QT_TRANSLATE_NOOP("App::Property","Geometry further than this value will be cut off. Keep zero for unlimited."))
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
        p = Part.makePlane(l,h,Vector(l/2,-h/2,0),Vector(0,0,-1))
        # make sure the normal direction is pointing outwards, you never know what OCC will decide...
        if p.normalAt(0,0).getAngle(obj.Placement.Rotation.multVec(FreeCAD.Vector(0,0,1))) > 1:
            p.reverse()
        p.Placement = obj.Placement
        obj.Shape = p
        self.svgcache = None
        self.shapecache = None

    def getNormal(self,obj):

        return obj.Shape.Faces[0].normalAt(0,0)

    def dumps(self):

        return None

    def loads(self,state):

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
        if not "ShowLabel" in pl:
            vobj.addProperty("App::PropertyBool","ShowLabel","SectionPlane",QT_TRANSLATE_NOOP("App::Property","Show the label in the 3D view"))
        if not "FontName" in pl:
            vobj.addProperty("App::PropertyFont","FontName", "SectionPlane",QT_TRANSLATE_NOOP("App::Property","The name of the font"))
            vobj.FontName = Draft.getParam("textfont","")
        if not "FontSize" in pl:
            vobj.addProperty("App::PropertyLength","FontSize","SectionPlane",QT_TRANSLATE_NOOP("App::Property","The size of the text font"))
            vobj.FontSize = Draft.getParam("textheight",10)


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
        import PartGui # Required for "SoBrepEdgeSet" (because a SectionPlane is not a Part::FeaturePython object).
        ls = coin.SoType.fromName("SoBrepEdgeSet").createInstance()
        ls.coordIndex.setValues(0,57,[0,1,-1,2,3,4,5,-1,6,7,8,9,-1,10,11,-1,12,13,14,15,-1,16,17,18,19,-1,20,21,-1,22,23,24,25,-1,26,27,28,29,-1,30,31,-1,32,33,34,35,-1,36,37,38,39,-1,40,41,42,43,44])
        self.txtcoords = coin.SoTransform()
        self.txtfont = coin.SoFont()
        self.txtfont.name = ""
        self.txt = coin.SoAsciiText()
        self.txt.justification = coin.SoText2.LEFT
        self.txt.string.setValue(" ")
        sep = coin.SoSeparator()
        psep = coin.SoSeparator()
        fsep = coin.SoSeparator()
        tsep = coin.SoSeparator()
        fsep.addChild(self.mat2)
        fsep.addChild(self.fcoords)
        fsep.addChild(fs)
        psep.addChild(self.mat1)
        psep.addChild(self.drawstyle)
        psep.addChild(self.lcoords)
        psep.addChild(ls)
        tsep.addChild(self.mat1)
        tsep.addChild(self.txtcoords)
        tsep.addChild(self.txtfont)
        tsep.addChild(self.txt)
        sep.addChild(fsep)
        sep.addChild(psep)
        sep.addChild(tsep)
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
            # for some reason the text doesn't rotate with the host placement??
            self.txtcoords.rotation.setValue(obj.Placement.Rotation.Q)
            self.onChanged(obj.ViewObject,"DisplayLength")
            self.onChanged(obj.ViewObject,"CutView")
        elif prop == "Label":
            if hasattr(obj.ViewObject,"ShowLabel") and obj.ViewObject.ShowLabel:
                self.txt.string = obj.Label
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
            pl = FreeCAD.Placement(vobj.Object.Placement)
            if hasattr(vobj,"ArrowSize"):
                l1 = vobj.ArrowSize.Value if vobj.ArrowSize.Value > 0 else 0.1
            else:
                l1 = 0.1
            l2 = l1/3
            for v in [[-ld,-hd],[ld,-hd],[ld,hd],[-ld,hd]]:
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
            p7 = pl.multVec(Vector(-ld+l2,-hd+l2,0)) # text pos
            verts.extend(fverts+[fverts[0]])
            self.lcoords.point.setValues(verts)
            self.fcoords.point.setValues(fverts)
            self.txtcoords.translation.setValue([p7.x,p7.y,p7.z])
            #self.txtfont.size = l1
        elif prop == "LineWidth":
            self.drawstyle.lineWidth = vobj.LineWidth
        elif prop in ["CutView","CutMargin"]:
            if hasattr(vobj, "CutView") \
                    and FreeCADGui.ActiveDocument.ActiveView \
                    and hasattr(FreeCADGui.ActiveDocument.ActiveView, "getSceneGraph"):
                sg = FreeCADGui.ActiveDocument.ActiveView.getSceneGraph()
                if vobj.CutView:
                    if self.clip:
                        sg.removeChild(self.clip)
                        self.clip = None
                    for o in Draft.get_group_contents(vobj.Object.Objects,
                                                      walls=True):
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
        elif prop == "ShowLabel":
            if vobj.ShowLabel:
                self.txt.string = vobj.Object.Label or " "
            else:
                self.txt.string = " "
        elif prop == "FontName":
            if hasattr(self,"txtfont") and hasattr(vobj,"FontName"):
                if vobj.FontName:
                    self.txtfont.name = vobj.FontName
                else:
                    self.txtfont.name = ""
        elif prop == "FontSize":
            if hasattr(self,"txtfont") and hasattr(vobj,"FontSize"):
                self.txtfont.size = vobj.FontSize.Value
        return

    def dumps(self):

        return None

    def loads(self,state):

        return None

    def setEdit(self, vobj, mode):
        if mode != 0:
            return None

        taskd = SectionPlaneTaskPanel()
        taskd.obj = vobj.Object
        taskd.update()
        FreeCADGui.Control.showDialog(taskd)
        return True

    def unsetEdit(self, vobj, mode):
        if mode != 0:
            return None

        FreeCADGui.Control.closeDialog()
        return True

    def doubleClicked(self, vobj):
        self.edit()

    def setupContextMenu(self, vobj, menu):
        actionEdit = QtGui.QAction(translate("Arch", "Edit"),
                                   menu)
        QtCore.QObject.connect(actionEdit,
                               QtCore.SIGNAL("triggered()"),
                               self.edit)
        menu.addAction(actionEdit)

        actionToggleCutview = QtGui.QAction(QtGui.QIcon(":/icons/Draft_Edit.svg"),
                                            translate("Arch", "Toggle Cutview"),
                                            menu)
        actionToggleCutview.triggered.connect(lambda f=self.toggleCutview, arg=vobj: f(arg))
        menu.addAction(actionToggleCutview)

    def edit(self):
        FreeCADGui.ActiveDocument.setEdit(self.Object, 0)

    def toggleCutview(self, vobj):
        vobj.CutView = not vobj.CutView


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
        self.delButton.setEnabled(False)

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
        QtCore.QObject.connect(self.tree, QtCore.SIGNAL("itemSelectionChanged()"), self.onTreeClick)
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
        elif hasattr(obj.ViewObject, "Icon"):
            return QtGui.QIcon(obj.ViewObject.Icon)
        return QtGui.QIcon(":/icons/Part_3D_object.svg")

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
            added = False
            for o in FreeCADGui.Selection.getSelection():
                if o != self.obj:
                    ArchComponent.addToComponent(self.obj,o,"Objects")
                    added = True
            if added:
                self.update()
            else:
                FreeCAD.Console.PrintWarning("Please select objects in the 3D view or in the model tree before pressing the button\n")

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
            for o in Draft.get_group_contents(self.obj.Objects):
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

    def onTreeClick(self):
        if self.tree.selectedItems():
            self.delButton.setEnabled(True)
        else:
            self.delButton.setEnabled(False)

    def accept(self):
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.ActiveDocument.resetEdit()
        return True

    def retranslateUi(self, TaskPanel):
        TaskPanel.setWindowTitle(QtGui.QApplication.translate("Arch", "Section plane settings", None))
        self.delButton.setText(QtGui.QApplication.translate("Arch", "Remove", None))
        self.delButton.setToolTip(QtGui.QApplication.translate("Arch", "Remove highlighted objects from the list above", None))
        self.addButton.setText(QtGui.QApplication.translate("Arch", "Add selected", None))
        self.addButton.setToolTip(QtGui.QApplication.translate("Arch", "Add selected object(s) to the scope of this section plane", None))
        self.title.setText(QtGui.QApplication.translate("Arch", "Objects seen by this section plane:", None))
        self.rlabel.setText(QtGui.QApplication.translate("Arch", "Section plane placement:", None))
        self.rotateXButton.setText(QtGui.QApplication.translate("Arch", "Rotate X", None))
        self.rotateXButton.setToolTip(QtGui.QApplication.translate("Arch", "Rotates the plane along the X axis", None))
        self.rotateYButton.setText(QtGui.QApplication.translate("Arch", "Rotate Y", None))
        self.rotateYButton.setToolTip(QtGui.QApplication.translate("Arch", "Rotates the plane along the Y axis", None))
        self.rotateZButton.setText(QtGui.QApplication.translate("Arch", "Rotate Z", None))
        self.rotateZButton.setToolTip(QtGui.QApplication.translate("Arch", "Rotates the plane along the Z axis", None))
        self.resizeButton.setText(QtGui.QApplication.translate("Arch", "Resize", None))
        self.resizeButton.setToolTip(QtGui.QApplication.translate("Arch", "Resizes the plane to fit the objects in the list above", None))
        self.recenterButton.setText(QtGui.QApplication.translate("Arch", "Center", None))
        self.recenterButton.setToolTip(QtGui.QApplication.translate("Arch", "Centers the plane on the objects in the list above", None))

if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Arch_SectionPlane',_CommandSectionPlane())
