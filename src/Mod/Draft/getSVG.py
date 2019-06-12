import six

import FreeCAD, math, os, DraftVecUtils, WorkingPlane
import Part, DraftGeomUtils
from FreeCAD import Vector
from Draft import getType, getrgb, svgpatterns, gui


def getLineStyle(linestyle, scale):
    "returns a linestyle"
    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
    l = None
    if linestyle == "Dashed":
        l = p.GetString("svgDashedLine","0.09,0.05")
    elif linestyle == "Dashdot":
        l = p.GetString("svgDashdotLine","0.09,0.05,0.02,0.05")
    elif linestyle == "Dotted":
        l = p.GetString("svgDottedLine","0.02,0.02")
    elif linestyle:
        if "," in linestyle:
            l = linestyle
    if l:
        l = l.split(",")
        try:
            # scale dashes
            l = ",".join([str(float(d)/scale) for d in l])
            #print "lstyle ",l
        except:
            return "none"
        else:
            return l
    return "none"


def getProj(vec, plane):
    if not plane: return vec
    nx = DraftVecUtils.project(vec,plane.u)
    lx = nx.Length
    if abs(nx.getAngle(plane.u)) > 0.1: lx = -lx
    ny = DraftVecUtils.project(vec,plane.v)
    ly = ny.Length
    if abs(ny.getAngle(plane.v)) > 0.1: ly = -ly
    #if techdraw: buggy - we now simply do it at the end
    #    ly = -ly
    return Vector(lx,ly,0)


def getDiscretized(edge, plane):
    ml = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft").GetFloat("svgDiscretization",10.0)
    if ml == 0:
        ml = 10
    d = int(edge.Length/ml)
    if d == 0:
        d = 1
    edata = ""
    for i in range(d+1):
        v = getProj(edge.valueAt(edge.FirstParameter+((float(i)/d)*(edge.LastParameter-edge.FirstParameter))), plane)
        if not edata:
            edata += 'M ' + str(v.x) +' '+ str(v.y) + ' '
        else:
            edata += 'L ' + str(v.x) +' '+ str(v.y) + ' '
    return edata


def getPattern(pat):
    if pat in svgpatterns():
        return svgpatterns()[pat][0]
    return ''


def getSVG(obj,scale=1,linewidth=0.35,fontsize=12,fillstyle="shape color",direction=None,linestyle=None,color=None,linespacing=None,techdraw=False,rotation=0,fillSpaces=False):
    '''getSVG(object,[scale], [linewidth],[fontsize],[fillstyle],[direction],[linestyle],[color],[linespacing]):
    returns a string containing a SVG representation of the given object,
    with the given linewidth and fontsize (used if the given object contains
    any text). You can also supply an arbitrary projection vector. the
    scale parameter allows to scale linewidths down, so they are resolution-independant.'''

    # if this is a group, gather all the svg views of its children
    if hasattr(obj,"isDerivedFrom"):
        if obj.isDerivedFrom("App::DocumentObjectGroup"):
            svg = ""
            for child in obj.Group:
                svg += getSVG(child,scale,linewidth,fontsize,fillstyle,direction,linestyle,color,linespacing,techdraw)
            return svg

    pathdata = []
    svg = ""
    linewidth = float(linewidth)/scale
    fontsize = (float(fontsize)/scale)/2
    if linespacing:
        linespacing = float(linespacing)/scale
    else:
        linespacing = 0.5
    #print obj.Label," line spacing ",linespacing,"scale ",scale
    pointratio = .75 # the number of times the dots are smaller than the arrow size
    plane = None
    if direction:
        if isinstance(direction,FreeCAD.Vector):
            if direction != Vector(0,0,0):
                plane = WorkingPlane.plane()
                plane.alignToPointAndAxis_SVG(Vector(0,0,0),direction.negative().negative(),0)
        elif isinstance(direction,WorkingPlane.plane):
            plane = direction
    stroke = "#000000"
    if color:
        if "#" in color:
            stroke = color
        else:
            stroke = getrgb(color)
    elif gui:
        if hasattr(obj,"ViewObject"):
            if hasattr(obj.ViewObject,"LineColor"):
                stroke = getrgb(obj.ViewObject.LineColor)


    def getPath(edges=[],wires=[],pathname=None):
        import Part,DraftGeomUtils
        svg = "<path "
        if pathname is None:
            svg += 'id="%s" ' % obj.Name
        elif pathname != "":
            svg += 'id="%s" ' % pathname
        svg += ' d="'
        if not wires:
            egroups = Part.sortEdges(edges)
        else:
            egroups = []
            for w in wires:
                w1=w.copy()
                w1.fixWire()
                egroups.append(Part.__sortEdges__(w1.Edges))
        for egroupindex, edges in enumerate(egroups):
            edata = ""
            vs=() #skipped for the first edge
            for edgeindex,e in enumerate(edges):
                previousvs = vs
                # vertexes of an edge (reversed if needed)
                vs = e.Vertexes
                if previousvs:
                    if (vs[0].Point-previousvs[-1].Point).Length > 1e-6:
                        vs.reverse()
                if edgeindex == 0:
                    v = getProj(vs[0].Point, plane)
                    edata += 'M '+ str(v.x) +' '+ str(v.y) + ' '
                else:
                    if (vs[0].Point-previousvs[-1].Point).Length > 1e-6:
                        raise ValueError('edges not ordered')
                iscircle = DraftGeomUtils.geomType(e) == "Circle"
                isellipse = DraftGeomUtils.geomType(e) == "Ellipse"
                if iscircle or isellipse:
                    import math
                    if hasattr(FreeCAD,"DraftWorkingPlane"):
                        drawing_plane_normal = FreeCAD.DraftWorkingPlane.axis
                    else:
                        drawing_plane_normal = FreeCAD.Vector(0,0,1)
                    if plane: drawing_plane_normal = plane.axis
                    c = e.Curve
                    if round(c.Axis.getAngle(drawing_plane_normal),2) in [0,3.14]:
                        occversion = Part.OCC_VERSION.split(".")
                        done = False
                        if (int(occversion[0]) >= 7) and (int(occversion[1]) >= 1):
                            # if using occ >= 7.1, use HLR algorithm
                            import Drawing
                            snip = Drawing.projectToSVG(e,drawing_plane_normal)
                            if snip:
                                try:
                                    a = "A " + snip.split("path d=\"")[1].split("\"")[0].split("A")[1]
                                except:
                                    pass
                                else:
                                    edata += a
                                    done = True
                        if not done:
                            if len(e.Vertexes) == 1 and iscircle: #complete curve
                                svg = getCircle(e)
                                return svg
                            elif len(e.Vertexes) == 1 and isellipse:
                                #svg = getEllipse(e)
                                #return svg
                                endpoints = [getProj(c.value((c.LastParameter-c.FirstParameter)/2.0), plane),
                                             getProj(vs[-1].Point, plane)]
                            else:
                                endpoints = [getProj(vs[-1].Point, plane)]
                            # arc
                            if iscircle:
                                rx = ry = c.Radius
                                rot = 0
                            else: #ellipse
                                rx = c.MajorRadius
                                ry = c.MinorRadius
                                rot = math.degrees(c.AngleXU * (c.Axis * \
                                    FreeCAD.Vector(0,0,1)))
                                if rot > 90:
                                    rot -=180
                                if rot < -90:
                                    rot += 180
                                #be careful with the sweep flag
                            flag_large_arc = (((e.ParameterRange[1] - \
                                    e.ParameterRange[0]) / math.pi) % 2) > 1
                            #flag_sweep = (c.Axis * drawing_plane_normal >= 0) \
                            #         == (e.LastParameter > e.FirstParameter)
                            #        == (e.Orientation == "Forward")
                            # other method: check the direction of the angle between tangents
                            t1 = e.tangentAt(e.FirstParameter)
                            t2 = e.tangentAt(e.FirstParameter + (e.LastParameter-e.FirstParameter)/10)
                            flag_sweep = (DraftVecUtils.angle(t1,t2,drawing_plane_normal) < 0)
                            for v in endpoints:
                                edata += 'A %s %s %s %s %s %s %s ' % \
                                        (str(rx),str(ry),str(rot),\
                                        str(int(flag_large_arc)),\
                                        str(int(flag_sweep)),str(v.x),str(v.y))
                    else:
                        edata += getDiscretized(e, plane)
                elif DraftGeomUtils.geomType(e) == "Line":
                    v = getProj(vs[-1].Point, plane)
                    edata += 'L '+ str(v.x) +' '+ str(v.y) + ' '
                else:
                    bspline=e.Curve.toBSpline(e.FirstParameter,e.LastParameter)
                    if bspline.Degree > 3 or bspline.isRational():
                        try:
                            bspline=bspline.approximateBSpline(0.05,50, 3,'C0')
                        except RuntimeError:
                            print("Debug: unable to approximate bspline")
                    if bspline.Degree <= 3 and not bspline.isRational():
                        for bezierseg in bspline.toBezier():
                            if bezierseg.Degree>3: #should not happen
                                raise AssertionError
                            elif bezierseg.Degree==1:
                                edata +='L '
                            elif bezierseg.Degree==2:
                                edata +='Q '
                            elif bezierseg.Degree==3:
                                edata +='C '
                            for pole in bezierseg.getPoles()[1:]:
                                v = getProj(pole, plane)
                                edata += str(v.x) +' '+ str(v.y) + ' '
                    else:
                        print("Debug: one edge (hash ",e.hashCode(),\
                                ") has been discretized with parameter 0.1")
                        for linepoint in bspline.discretize(0.1)[1:]:
                            v = getProj(linepoint, plane)
                            edata += 'L '+ str(v.x) +' '+ str(v.y) + ' '
            if fill != 'none':
                edata += 'Z '
            if edata in pathdata:
                # do not draw a path on another identical path
                return ""
            else:
                svg += edata
                pathdata.append(edata)
        svg += '" '
        svg += 'stroke="' + stroke + '" '
        svg += 'stroke-width="' + str(linewidth) + ' px" '
        svg += 'style="stroke-width:'+ str(linewidth)
        svg += ';stroke-miterlimit:4'
        svg += ';stroke-dasharray:' + lstyle
        svg += ';fill:' + fill
        try:
            svg += ';fill-opacity:' + str(fill_opacity)
        except NameError:
            pass
        svg += ';fill-rule: evenodd "'
        svg += '/>\n'
        return svg

    def getCircle(edge):
        cen = getProj(edge.Curve.Center, plane)
        rad = edge.Curve.Radius
        if hasattr(FreeCAD,"DraftWorkingPlane"):
            drawing_plane_normal = FreeCAD.DraftWorkingPlane.axis
        else:
            drawing_plane_normal = FreeCAD.Vector(0,0,1)
        if plane: drawing_plane_normal = plane.axis
        if round(edge.Curve.Axis.getAngle(drawing_plane_normal),2) in [0, 3.14]:
            # perpendicular projection: circle
            svg = '<circle cx="' + str(cen.x)
            svg += '" cy="' + str(cen.y)
            svg += '" r="' + str(rad)+'" '
        else:
            # any other projection: ellipse
            svg = '<path d="'
            svg += getDiscretized(edge, plane)
            svg += '" '
        svg += 'stroke="' + stroke + '" '
        svg += 'stroke-width="' + str(linewidth) + ' px" '
        svg += 'style="stroke-width:'+ str(linewidth)
        svg += ';stroke-miterlimit:4'
        svg += ';stroke-dasharray:' + lstyle
        svg += ';fill:' + fill + '"'
        svg += '/>\n'
        return svg

    def getEllipse(edge):
        cen = getProj(edge.Curve.Center, plane)
        mir = edge.Curve.MinorRadius
        mar = edge.Curve.MajorRadius
        svg = '<ellipse cx="' + str(cen.x)
        svg += '" cy="' + str(cen.y)
        svg += '" rx="' + str(mar)
        svg += '" ry="' + str(mir)+'" '
        svg += 'stroke="' + stroke + '" '
        svg += 'stroke-width="' + str(linewidth) + ' px" '
        svg += 'style="stroke-width:'+ str(linewidth)
        svg += ';stroke-miterlimit:4'
        svg += ';stroke-dasharray:' + lstyle
        svg += ';fill:' + fill + '"'
        svg += '/>\n'
        return svg

    def getArrow(arrowtype,point,arrowsize,color,linewidth,angle=0):
        svg = ""
        if gui:
            if not obj.ViewObject:
                return svg
            if obj.ViewObject.ArrowType == "Circle":
                svg += '<circle cx="'+str(point.x)+'" cy="'+str(point.y)
                svg += '" r="'+str(arrowsize)+'" '
                svg += 'fill="none" stroke="'+ color + '" '
                svg += 'style="stroke-width:'+ str(linewidth) + ';stroke-miterlimit:4;stroke-dasharray:none" '
                svg += 'freecad:skip="1"'
                svg += '/>\n'
            elif obj.ViewObject.ArrowType == "Dot":
                svg += '<circle cx="'+str(point.x)+'" cy="'+str(point.y)
                svg += '" r="'+str(arrowsize)+'" '
                svg += 'fill="'+ color +'" stroke="none" '
                svg += 'style="stroke-miterlimit:4;stroke-dasharray:none" '
                svg += 'freecad:skip="1"'
                svg += '/>\n'
            elif obj.ViewObject.ArrowType == "Arrow":
                svg += '<path transform="rotate('+str(math.degrees(angle))
                svg += ','+ str(point.x) + ',' + str(point.y) + ') '
                svg += 'translate(' + str(point.x) + ',' + str(point.y) + ') '
                svg += 'scale('+str(arrowsize)+','+str(arrowsize)+')" freecad:skip="1" '
                svg += 'fill="'+ color +'" stroke="none" '
                svg += 'style="stroke-miterlimit:4;stroke-dasharray:none" '
                svg += 'd="M 0 0 L 4 1 L 4 -1 Z"/>\n'
            elif obj.ViewObject.ArrowType == "Tick":
                svg += '<path transform="rotate('+str(math.degrees(angle))
                svg += ','+ str(point.x) + ',' + str(point.y) + ') '
                svg += 'translate(' + str(point.x) + ',' + str(point.y) + ') '
                svg += 'scale('+str(arrowsize)+','+str(arrowsize)+')" freecad:skip="1" '
                svg += 'fill="'+ color +'" stroke="none" '
                svg += 'style="stroke-miterlimit:4;stroke-dasharray:none" '
                svg += 'd="M -1 -2 L 0 2 L 1 2 L 0 -2 Z"/>\n'
            elif obj.ViewObject.ArrowType == "Tick-2":
                svg += '<line transform="rotate('+str(math.degrees(angle)+45)
                svg += ','+ str(point.x) + ',' + str(point.y) + ') '
                svg += 'translate(' + str(point.x) + ',' + str(point.y) + ') '
                svg += '" freecad:skip="1" '
                svg += 'fill="none" stroke="'+ color +'" '
                svg += 'style="stroke-dasharray:none;stroke-linecap:square;'
                svg += 'stroke-width:'+ str(linewidth) +'" '
                svg += 'x1="-'+ str(arrowsize*2) +'" y1="0" '
                svg += 'x2="' + str(arrowsize*2) +'" y2="0" />\n'
            else:
                print("getSVG: arrow type not implemented")
        return svg

    def getOvershoot(point,shootsize,color,linewidth,angle=0):
        svg = '<line transform="rotate('+str(math.degrees(angle))
        svg += ','+ str(point.x) + ',' + str(point.y) + ') '
        svg += 'translate(' + str(point.x) + ',' + str(point.y) + ') '
        svg += '" freecad:skip="1" '
        svg += 'fill="none" stroke="'+ color +'" '
        svg += 'style="stroke-dasharray:none;stroke-linecap:square;'
        svg += 'stroke-width:'+ str(linewidth) +'" '
        svg += 'x1="0" y1="0" '
        svg += 'x2="'+ str(shootsize*-1) +'" y2="0" />\n'
        return svg

    def getText(color,fontsize,fontname,angle,base,text,linespacing=0.5,align="center",flip=True):
        if isinstance(angle,FreeCAD.Rotation):
            if not plane:
                angle = angle.Angle
            else:
                if plane.axis.getAngle(angle.Axis) < 0.001:
                    angle = angle.Angle
                elif abs(plane.axis.getAngle(angle.Axis)-math.pi) < 0.001:
                    return "" # text is perpendicular to view, so it shouldn't appear
                else:
                    angle = 0 #TODO maybe there is something better to do here?
        if not isinstance(text,list):
            text = text.split("\n")
        if align.lower() == "center":
            anchor = "middle"
        elif align.lower() == "left":
            anchor = "start"
        else:
            anchor = "end"
        if techdraw:
            svg = ""
            for i in range(len(text)):
                t = text[i]
                if six.PY2 and not isinstance(t, six.text_type):
                    t = t.decode("utf8")
                # possible workaround if UTF8 is unsupported
                #    import unicodedata
                #    t = u"".join([c for c in unicodedata.normalize("NFKD",t) if not unicodedata.combining(c)]).encode("utf8")
                svg += '<text fill="' + color +'" font-size="' + str(fontsize) + '" '
                svg += 'style="text-anchor:'+anchor+';text-align:'+align.lower()+';'
                svg += 'font-family:'+ fontname +'" '
                svg += 'transform="rotate('+str(math.degrees(angle))
                svg += ','+ str(base.x) + ',' + str(base.y-linespacing*i) + ') '
                svg += 'translate(' + str(base.x) + ',' + str(base.y-linespacing*i) + ') '
                svg += 'scale(1,-1)" '
                #svg += '" freecad:skip="1"'
                svg += '>\n' + t + '</text>\n'
        else:
            svg = '<text fill="'
            svg += color +'" font-size="'
            svg += str(fontsize) + '" '
            svg += 'style="text-anchor:'+anchor+';text-align:'+align.lower()+';'
            svg += 'font-family:'+ fontname +'" '
            svg += 'transform="rotate('+str(math.degrees(angle))
            svg += ','+ str(base.x) + ',' + str(base.y) + ') '
            if flip:
                svg += 'translate(' + str(base.x) + ',' + str(base.y) + ')'
            else:
                svg += 'translate(' + str(base.x) + ',' + str(-base.y) + ')'
            #svg += 'scale('+str(tmod/2000)+',-'+str(tmod/2000)+') '
            if flip:
                svg += ' scale(1,-1) '
            else:
                svg += ' scale(1,1) '
            svg += '" freecad:skip="1"'
            svg += '>\n'
            if len(text) == 1:
                try:
                    svg += text[0]
                except:
                    svg += text[0].decode("utf8")
            else:
                for i in range(len(text)):
                    if i == 0:
                        svg += '<tspan>'
                    else:
                        svg += '<tspan x="0" dy="'+str(linespacing)+'">'
                    try:
                        svg += text[i]
                    except:
                        svg += text[i].decode("utf8")
                    svg += '</tspan>\n'
            svg += '</text>\n'
        return svg


    if not obj:
        pass

    elif isinstance(obj,Part.Shape):
        if "#" in fillstyle:
            fill = fillstyle
        elif fillstyle == "shape color":
            fill = "#888888"
        else:
            fill = 'url(#'+fillstyle+')'
        lstyle = getLineStyle(linestyle, scale)
        svg += getPath(obj.Edges,pathname="")


    elif getType(obj) == "Dimension":
        if gui:
            if not obj.ViewObject:
                print ("export of dimensions to SVG is only available in GUI mode")
            elif obj.ViewObject.Proxy:
                if hasattr(obj.ViewObject.Proxy,"p1"):
                    prx = obj.ViewObject.Proxy
                    ts = (len(prx.string)*obj.ViewObject.FontSize.Value)/4.0
                    rm = ((prx.p3.sub(prx.p2)).Length/2.0)-ts
                    p2a = getProj(prx.p2.add(DraftVecUtils.scaleTo(prx.p3.sub(prx.p2),rm)), plane)
                    p2b = getProj(prx.p3.add(DraftVecUtils.scaleTo(prx.p2.sub(prx.p3),rm)), plane)
                    p1 = getProj(prx.p1, plane)
                    p2 = getProj(prx.p2, plane)
                    p3 = getProj(prx.p3, plane)
                    p4 = getProj(prx.p4, plane)
                    tbase = getProj(prx.tbase, plane)
                    r = prx.textpos.rotation.getValue().getValue()
                    rv = FreeCAD.Rotation(r[0],r[1],r[2],r[3]).multVec(FreeCAD.Vector(1,0,0))
                    angle = -DraftVecUtils.angle(getProj(rv, plane))
                    #angle = -DraftVecUtils.angle(p3.sub(p2))

                    # drawing lines
                    svg = '<path '
                    if obj.ViewObject.DisplayMode == "2D":
                        tangle = angle
                        if tangle > math.pi/2:
                            tangle = tangle-math.pi
                        #elif (tangle <= -math.pi/2) or (tangle > math.pi/2):
                        #    tangle = tangle+math.pi
                        #tbase = tbase.add(DraftVecUtils.rotate(Vector(0,2/scale,0),tangle))
                        if rotation != 0:
                            #print "dim: tangle:",tangle," rot: ",rotation," text: ",prx.string
                            if abs(tangle+math.radians(rotation)) < 0.0001:
                                tangle += math.pi
                                tbase = tbase.add(DraftVecUtils.rotate(Vector(0,2/scale,0),tangle))
                        svg += 'd="M '+str(p1.x)+' '+str(p1.y)+' '
                        svg += 'L '+str(p2.x)+' '+str(p2.y)+' '
                        svg += 'L '+str(p3.x)+' '+str(p3.y)+' '
                        svg += 'L '+str(p4.x)+' '+str(p4.y)+'" '
                    else:
                        tangle = 0
                        if rotation != 0:
                            tangle = -math.radians(rotation)
                        tbase = tbase.add(Vector(0,-2.0/scale,0))
                        svg += 'd="M '+str(p1.x)+' '+str(p1.y)+' '
                        svg += 'L '+str(p2.x)+' '+str(p2.y)+' '
                        svg += 'L '+str(p2a.x)+' '+str(p2a.y)+' '
                        svg += 'M '+str(p2b.x)+' '+str(p2b.y)+' '
                        svg += 'L '+str(p3.x)+' '+str(p3.y)+' '
                        svg += 'L '+str(p4.x)+' '+str(p4.y)+'" '

                    svg += 'fill="none" stroke="'
                    svg += stroke + '" '
                    svg += 'stroke-width="' + str(linewidth) + ' px" '
                    svg += 'style="stroke-width:'+ str(linewidth)
                    svg += ';stroke-miterlimit:4;stroke-dasharray:none" '
                    svg += 'freecad:basepoint1="'+str(p1.x)+' '+str(p1.y)+'" '
                    svg += 'freecad:basepoint2="'+str(p4.x)+' '+str(p4.y)+'" '
                    svg += 'freecad:dimpoint="'+str(p2.x)+' '+str(p2.y)+'"'
                    svg += '/>\n'

                    # drawing dimension and extension lines overshoots
                    if hasattr(obj.ViewObject,"DimOvershoot") and obj.ViewObject.DimOvershoot.Value:
                        shootsize = obj.ViewObject.DimOvershoot.Value/pointratio
                        svg += getOvershoot(p2,shootsize,stroke,linewidth,angle)
                        svg += getOvershoot(p3,shootsize,stroke,linewidth,angle+math.pi)
                    if hasattr(obj.ViewObject,"ExtOvershoot") and obj.ViewObject.ExtOvershoot.Value:
                        shootsize = obj.ViewObject.ExtOvershoot.Value/pointratio
                        shootangle = -DraftVecUtils.angle(p1.sub(p2))
                        svg += getOvershoot(p2,shootsize,stroke,linewidth,shootangle)
                        svg += getOvershoot(p3,shootsize,stroke,linewidth,shootangle)

                    # drawing arrows
                    if hasattr(obj.ViewObject,"ArrowType"):
                        arrowsize = obj.ViewObject.ArrowSize.Value/pointratio
                        if hasattr(obj.ViewObject,"FlipArrows"):
                            if obj.ViewObject.FlipArrows:
                                angle = angle+math.pi
                        svg += getArrow(obj.ViewObject.ArrowType,p2,arrowsize,stroke,linewidth,angle)
                        svg += getArrow(obj.ViewObject.ArrowType,p3,arrowsize,stroke,linewidth,angle+math.pi)

                    # drawing text
                    svg += getText(stroke,fontsize,obj.ViewObject.FontName,tangle,tbase,prx.string)

    elif getType(obj) == "AngularDimension":
        if gui:
            if not obj.ViewObject:
                print ("export of dimensions to SVG is only available in GUI mode")
            elif obj.ViewObject.Proxy:
                if hasattr(obj.ViewObject.Proxy,"circle"):
                    prx = obj.ViewObject.Proxy

                    # drawing arc
                    fill= "none"
                    lstyle = getLineStyle(linestyle, scale)
                    if obj.ViewObject.DisplayMode == "2D":
                        svg += getPath([prx.circle])
                    else:
                        if hasattr(prx,"circle1"):
                            svg += getPath([prx.circle1])
                            svg += getPath([prx.circle2])
                        else:
                            svg += getPath([prx.circle])

                    # drawing arrows
                    if hasattr(obj.ViewObject,"ArrowType"):
                        p2 = getProj(prx.p2, plane)
                        p3 = getProj(prx.p3, plane)
                        arrowsize = obj.ViewObject.ArrowSize.Value/pointratio
                        arrowlength = 4*obj.ViewObject.ArrowSize.Value
                        u1 = getProj((prx.circle.valueAt(prx.circle.FirstParameter+arrowlength)).sub(prx.circle.valueAt(prx.circle.FirstParameter)), plane)
                        u2 = getProj((prx.circle.valueAt(prx.circle.LastParameter-arrowlength)).sub(prx.circle.valueAt(prx.circle.LastParameter)), plane)
                        angle1 = -DraftVecUtils.angle(u1)
                        angle2 = -DraftVecUtils.angle(u2)
                        if hasattr(obj.ViewObject,"FlipArrows"):
                            if obj.ViewObject.FlipArrows:
                                angle1 = angle1+math.pi
                                angle2 = angle2+math.pi
                        svg += getArrow(obj.ViewObject.ArrowType,p2,arrowsize,stroke,linewidth,angle1)
                        svg += getArrow(obj.ViewObject.ArrowType,p3,arrowsize,stroke,linewidth,angle2)

                    # drawing text
                    if obj.ViewObject.DisplayMode == "2D":
                        t = prx.circle.tangentAt(prx.circle.FirstParameter+(prx.circle.LastParameter-prx.circle.FirstParameter)/2.0)
                        t = getProj(t, plane)
                        tangle = DraftVecUtils.angle(t)
                        if (tangle <= -math.pi/2) or (tangle > math.pi/2):
                            tangle = tangle + math.pi
                        tbase = getProj(prx.circle.valueAt(prx.circle.FirstParameter+(prx.circle.LastParameter-prx.circle.FirstParameter)/2.0), plane)
                        tbase = tbase.add(DraftVecUtils.rotate(Vector(0,2.0/scale,0),tangle))
                        #print(tbase)
                    else:
                        tangle = 0
                        tbase = getProj(prx.tbase, plane)
                    svg += getText(stroke,fontsize,obj.ViewObject.FontName,tangle,tbase,prx.string)

    elif getType(obj) == "Label":
        if getattr(obj.ViewObject, "Line", True):  # some Labels may have no Line property
            def format_point(coords, action='L'):
                return "{action}{x},{y}".format(
                    x=coords.x, y=coords.y, action=action
                )

            # Draw multisegment line
            proj_points = list(map(lambda x: getProj(x, plane), obj.Points))
            path_dir_list = [format_point(proj_points[0], action='M')]
            path_dir_list += map(format_point, proj_points[1:])
            path_dir_str = " ".join(path_dir_list)
            svg_path = '<path fill="none" stroke="{stroke}" stroke-width="{linewidth}" d="{directions}"/>'.format(
                stroke=stroke,
                linewidth=linewidth,
                directions=path_dir_str
            )
            svg += svg_path

            # Draw arrow.
            # We are different here from 3D view
            # if Line is set to 'off', no arrow is drawn
            if hasattr(obj.ViewObject, "ArrowType") and len(obj.Points) >= 2:
                last_segment = FreeCAD.Vector(obj.Points[-1] - obj.Points[-2])
                angle = -DraftVecUtils.angle(getProj(last_segment, plane)) + math.pi
                svg += getArrow(
                    arrowtype=obj.ViewObject.ArrowType,
                    point=proj_points[-1],
                    arrowsize=obj.ViewObject.ArrowSize.Value/pointratio,
                    color=stroke,
                    linewidth=linewidth,
                    angle=angle
                )

        # print text
        if gui:
            if not obj.ViewObject:
                print("export of texts to SVG is only available in GUI mode")
            else:
                fontname = obj.ViewObject.TextFont
                position = getProj(obj.Placement.Base, plane)
                rotation = obj.Placement.Rotation
                justification = obj.ViewObject.TextAlignment
                text = obj.Text
                svg += getText(stroke, fontsize, fontname, rotation, position,
                               text, linespacing, justification)

    elif getType(obj) in ["Annotation","DraftText"]:
        "returns an svg representation of a document annotation"
        if gui:
            if not obj.ViewObject:
                print ("export of texts to SVG is only available in GUI mode")
            else:
                n = obj.ViewObject.FontName
                if getType(obj) == "Annotation":
                    p = getProj(obj.Position, plane)
                    r = obj.ViewObject.Rotation.getValueAs("rad")
                    t = obj.LabelText
                else: # DraftText
                    p = getProj(obj.Placement.Base, plane)
                    r = obj.Placement.Rotation
                    t = obj.Text
                j = obj.ViewObject.Justification
                svg += getText(stroke,fontsize,n,r,p,t,linespacing,j)

    elif getType(obj) == "Axis":
        "returns the SVG representation of an Arch Axis system"
        if gui:
            if not obj.ViewObject:
                print ("export of axes to SVG is only available in GUI mode")
            else:
                vobj = obj.ViewObject
                lorig = getLineStyle(linestyle, scale)
                fill = 'none'
                rad = vobj.BubbleSize.Value/2
                n = 0
                for e in obj.Shape.Edges:
                    lstyle = lorig
                    svg += getPath([e])
                    lstyle = "none"
                    pos = ["Start"]
                    if hasattr(vobj,"BubblePosition"):
                        if vobj.BubblePosition == "Both":
                            pos = ["Start","End"]
                        else:
                            pos = [vobj.BubblePosition]
                    for p in pos:
                        if p == "Start":
                            p1 = e.Vertexes[0].Point
                            p2 = e.Vertexes[1].Point
                        else:
                            p1 = e.Vertexes[1].Point
                            p2 = e.Vertexes[0].Point
                        dv = p2.sub(p1)
                        dv.normalize()
                        center = p2.add(dv.scale(rad,rad,rad))
                        svg += getCircle(Part.makeCircle(rad,center))
                        if hasattr(vobj.Proxy,"bubbletexts"):
                            if len (vobj.Proxy.bubbletexts) >= n:
                                svg += '<text fill="' + stroke + '" '
                                svg += 'font-size="' + str(rad) + '" '
                                svg += 'style="text-anchor:middle;'
                                svg += 'text-align:center;'
                                svg += 'font-family: sans;" '
                                svg += 'transform="translate(' + str(center.x+rad/4.0) + ',' + str(center.y-rad/3.0) + ') '
                                svg += 'scale(1,-1)"> '
                                svg += '<tspan>' + obj.ViewObject.Proxy.bubbletexts[n].string.getValues()[0] + '</tspan>\n'
                                svg += '</text>\n'
                                n += 1

    elif getType(obj) == "Pipe":
        fill = stroke
        lstyle = getLineStyle(linestyle, scale)
        if obj.Base and obj.Diameter:
            svg += getPath(obj.Base.Shape.Edges)
        for f in obj.Shape.Faces:
            if len(f.Edges) == 1:
                if isinstance(f.Edges[0].Curve,Part.Circle):
                    svg += getCircle(f.Edges[0])

    elif getType(obj) == "Rebar":
        fill = "none"
        lstyle = getLineStyle(linestyle, scale)
        if obj.Proxy:
            if not hasattr(obj.Proxy,"wires"):
                obj.Proxy.execute(obj)
            if hasattr(obj.Proxy,"wires"):
                svg += getPath(wires=obj.Proxy.wires)

    elif getType(obj) == "PipeConnector":
        pass

    elif getType(obj) == "Space":
        "returns an SVG fragment for the text of a space"
        if gui:
            if not obj.ViewObject:
                print ("export of spaces to SVG is only available in GUI mode")
            else:
                if fillSpaces:
                    if hasattr(obj,"Proxy"):
                        if not hasattr(obj.Proxy,"face"):
                            obj.Proxy.getArea(obj,notouch=True)
                        if hasattr(obj.Proxy,"face"):
                            # setting fill
                            if gui:
                                fill = getrgb(obj.ViewObject.ShapeColor,testbw=False)
                                fill_opacity = 1 - (obj.ViewObject.Transparency / 100.0)
                            else:
                                fill = "#888888"
                            lstyle = getLineStyle(linestyle, scale)
                            svg += getPath(wires=[obj.Proxy.face.OuterWire])
                c = getrgb(obj.ViewObject.TextColor)
                n = obj.ViewObject.FontName
                a = 0
                if rotation != 0:
                    a = math.radians(rotation)
                t1 = obj.ViewObject.Proxy.text1.string.getValues()
                t2 = obj.ViewObject.Proxy.text2.string.getValues()
                scale = obj.ViewObject.FirstLine.Value/obj.ViewObject.FontSize.Value
                f1 = fontsize*scale
                p2 = obj.Placement.multVec(FreeCAD.Vector(obj.ViewObject.Proxy.coords.translation.getValue().getValue()))
                lspc = FreeCAD.Vector(obj.ViewObject.Proxy.header.translation.getValue().getValue())
                p1 = p2.add(lspc)
                j = obj.ViewObject.TextAlign
                t3 = getText(c,f1,n,a,getProj(p1, plane),t1,linespacing,j,flip=True)
                svg += t3
                if t2:
                    ofs = FreeCAD.Vector(0,-lspc.Length,0)
                    if a:
                        ofs = FreeCAD.Rotation(FreeCAD.Vector(0,0,1),-rotation).multVec(ofs)
                    t4 = getText(c,fontsize,n,a,getProj(p1, plane).add(ofs),t2,linespacing,j,flip=True)
                    svg += t4

    elif obj.isDerivedFrom('Part::Feature'):
        if obj.Shape.isNull():
            return ''
        # setting fill
        if obj.Shape.Faces:
            if gui:
                try:
                    m = obj.ViewObject.DisplayMode
                except AttributeError:
                    m = None
                if (m != "Wireframe"):
                    if fillstyle == "shape color":
                        fill = getrgb(obj.ViewObject.ShapeColor,testbw=False)
                        fill_opacity = 1 - (obj.ViewObject.Transparency / 100.0)
                    else:
                        fill = 'url(#'+fillstyle+')'
                        svg += getPattern(fillstyle)
                else:
                    fill = "none"
            else:
                fill = "#888888"
        else:
            fill = 'none'
        lstyle = getLineStyle(linestyle, scale)

        if len(obj.Shape.Vertexes) > 1:
            wiredEdges = []
            if obj.Shape.Faces:
                for i,f in enumerate(obj.Shape.Faces):
                    svg += getPath(wires=f.Wires,pathname='%s_f%04d' % \
                            (obj.Name,i))
                    wiredEdges.extend(f.Edges)
            else:
                for i,w in enumerate(obj.Shape.Wires):
                    svg += getPath(w.Edges,pathname='%s_w%04d' % \
                            (obj.Name,i))
                    wiredEdges.extend(w.Edges)
            if len(wiredEdges) != len(obj.Shape.Edges):
                for i,e in enumerate(obj.Shape.Edges):
                    if (DraftGeomUtils.findEdge(e,wiredEdges) == None):
                        svg += getPath([e],pathname='%s_nwe%04d' % \
                                (obj.Name,i))
        else:
            # closed circle or spline
            if obj.Shape.Edges:
                if isinstance(obj.Shape.Edges[0].Curve,Part.Circle):
                    svg = getCircle(obj.Shape.Edges[0])
                else:
                    svg = getPath(obj.Shape.Edges)
        if FreeCAD.GuiUp:
            if hasattr(obj.ViewObject,"EndArrow") and hasattr(obj.ViewObject,"ArrowType") and (len(obj.Shape.Vertexes) > 1):
                if obj.ViewObject.EndArrow:
                    p1 = getProj(obj.Shape.Vertexes[-1].Point, plane)
                    p2 = getProj(obj.Shape.Vertexes[-2].Point, plane)
                    angle = -DraftVecUtils.angle(p2.sub(p1))
                    arrowsize = obj.ViewObject.ArrowSize.Value/pointratio
                    svg += getArrow(obj.ViewObject.ArrowType,p1,arrowsize,stroke,linewidth,angle)

    # techdraw expects bottom-to-top coordinates
    if techdraw:
        svg = '<g transform ="scale(1,-1)">'+svg+'</g>'
    return svg
