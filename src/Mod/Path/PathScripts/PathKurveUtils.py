# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2015 Dan Falck <ddfalck@gmail.com>                      *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
'''PathKurveUtils - functions needed for using libarea (created by Dan Heeks) for making simple CNC profile paths '''
import FreeCAD
from FreeCAD import Vector
import FreeCADGui as Gui
import Part
import DraftGeomUtils
import DraftVecUtils
from DraftGeomUtils import geomType
import math
import area
import Path
from PathScripts import PathUtils
from nc.nc import *
import PathScripts.nc.iso
from PathScripts.nc.nc import *

def makeAreaVertex(seg):
    if seg.ShapeType == 'Edge':
        if isinstance(seg.Curve, Part.Circle):
            segtype = int(seg.Curve.Axis.z)  # 1=ccw arc,-1=cw arc
            vertex = area.Vertex(segtype, area.Point(seg.valueAt(seg.LastParameter)[0], seg.valueAt(
                seg.LastParameter)[1]), area.Point(seg.Curve.Center.x, seg.Curve.Center.y))
        elif isinstance(seg.Curve, Part.Line):
            point1 = seg.valueAt(seg.FirstParameter)[
                0], seg.valueAt(seg.FirstParameter)[1]
            point2 = seg.valueAt(seg.LastParameter)[
                0], seg.valueAt(seg.LastParameter)[1]
            segtype = 0  # 0=line
            vertex = area.Point(seg.valueAt(seg.LastParameter)[
                                0], seg.valueAt(seg.LastParameter)[1])
        else:
            pass
    # print "returning vertex: area.Point(" +
    # str(seg.valueAt(seg.LastParameter)[0]) +"," +
    # str(seg.valueAt(seg.LastParameter)[1]) +")"
    return vertex


def makeAreaCurve(edges, direction, startpt=None, endpt=None):
    curveobj = area.Curve()

    cleanededges = Part.__sortEdges__(PathUtils.cleanedges(edges, 0.01))

    # for e in cleanededges:
    # print str(e.valueAt(e.FirstParameter)) + "," +
    # str(e.valueAt(e.LastParameter))
    edgelist = []

    if len(cleanededges) == 1:  # user selected a single edge.
        edgelist = cleanededges
    else:
        # edgelist = [] #Multiple edges.  Need to sequence the vetexes.
        # First get the first segment oriented correctly.

        # We first compare the last parameter of the first segment to see if it
        # matches either end of the second segment. If not, it must need
        # flipping.
        if cleanededges[0].valueAt(cleanededges[0].LastParameter) in [cleanededges[1].valueAt(cleanededges[1].FirstParameter), cleanededges[1].valueAt(cleanededges[1].LastParameter)]:
            edge0 = cleanededges[0]
        else:
            edge0 = PathUtils.reverseEdge(cleanededges[0])

        edgelist.append(edge0)

        # Now iterate the rest of the edges matching the last parameter of the
        # previous segment.
        for edge in cleanededges[1:]:

            if edge.valueAt(edge.FirstParameter) == edgelist[-1].valueAt(edgelist[-1].LastParameter):
                nextedge = edge
            else:
                nextedge = PathUtils.reverseEdge(edge)
            edgelist.append(nextedge)
    # print "makeareacurve 87: " + "area.Point(" +
    # str(edgelist[0].Vertexes[0].X) + ", " +
    # str(edgelist[0].Vertexes[0].Y)+")"
    curveobj.append(area.Point(edgelist[0].Vertexes[
                    0].X, edgelist[0].Vertexes[0].Y))
#     seglist =[]
#     if direction=='CW':
#         edgelist.reverse()
#         for e in edgelist:
#             seglist.append(PathUtils.reverseEdge(e)) #swap end points on every segment
#     else:
#         for e in edgelist:
#             seglist.append(e)

    for s in edgelist:
        curveobj.append(makeAreaVertex(s))

    if startpt:
        # future nearest point code yet to be worked out -fixme
        #         v1 = Vector(startpt.X,startpt.Y,startpt.Z)
        #         perppoint1 = DraftGeomUtils.findPerpendicular(v1,firstedge)
        #         perppoint1 = DraftGeomUtils.findDistance(v1,firstedge)
        #         if  perppoint1:
        #             curveobj.ChangeStart(area.Point(perppoint1[0].x,perppoint1[0].y))
        #         else:
        #             curveobj.ChangeStart(area.Point(startpt.X,startpt.Y))
        curveobj.ChangeStart(area.Point(startpt.x, startpt.y))
    if endpt:
        # future nearest point code yet to be worked out -fixme
        #         v2 = Vector(endpt.X,endpt.Y,endpt.Z)
        #         perppoint2 = DraftGeomUtils.findPerpendicular(v2,lastedge)
        #         if perppoint2:
        #             curveobj.ChangeEnd(area.Point(perppoint2[0].x,perppoint2[0].y))
        #         else:
        #             curveobj.ChangeEnd(area.Point(endpt.X,endpt.Y))
        curveobj.ChangeEnd(area.Point(endpt.x, endpt.y))

    if curveobj.IsClockwise() and direction == 'CCW':
        curveobj.Reverse()
    elif not curveobj.IsClockwise() and direction == 'CW':
        curveobj.Reverse()
    return curveobj


# profile command,
# side_of_line should be 'Left' or 'Right' or 'On'
def profile(curve, side_of_line, radius=1.0, vertfeed=0.0, horizfeed=0.0, offset_extra=0.0,
            rapid_safety_space=None, clearance=None, start_depth=None, stepdown=None,
            final_depth=None, use_CRC=False,
            roll_on=None, roll_off=None, roll_start=False, roll_end=True, roll_radius=None,
            roll_start_pt=None, roll_end_pt=None):

    output = ""
    output += "G0 Z" + str(clearance) + "\n"
    print "in profile: 151"
    offset_curve = area.Curve(curve)
    if offset_curve.getNumVertices() <= 1:
        raise Exception, "Sketch has no elements!"
    if side_of_line == "On":
        use_CRC = False

    elif (side_of_line == "Left") or (side_of_line == "Right"):
        # get tool radius plus little bit of extra offset, if needed to clean
        # up profile a little more
        offset = radius + offset_extra
        if side_of_line == 'Left':
            offset_curve.Offset(offset)

        else:
            offset_curve.Offset(-offset)

        if offset_curve is False:
            raise Exception, "couldn't offset kurve " + str(offset_curve)
    else:
        raise Exception, "Side must be 'Left','Right', or 'On'"

# =========================================================================
#     #roll_on roll_off section
#     roll_on_curve = area.Curve()
#     if offset_curve.getNumVertices() <= 1: return
#     first_span = offset_curve.GetFirstSpan()
#     if roll_on == None:
#         rollstart = first_span.p
#     elif roll_on == 'auto':
#         if roll_radius < 0.0000000001:
#             rollstart = first_span.p
#         v = first_span.GetVector(0.0)
#         if direction == 'right':
#             off_v = area.Point(v.y, -v.x)
#         else:
#             off_v = area.Point(-v.y, v.x)
#         rollstart = first_span.p + off_v * roll_radius
#     else:
#         rollstart = roll_on
#
#     rvertex = area.Vertex(first_span.p)
#
#     if first_span.p == rollstart:
#         rvertex.type = 0
#     else:
#         v = first_span.GetVector(0.0) # get start direction
#         rvertex.c, rvertex.type = area.TangentialArc(first_span.p, rollstart, -v)
#         rvertex.type = -rvertex.type # because TangentialArc was used in reverse
#     # add a start roll on point
#     roll_on_curve.append(rollstart)
#
#     # add the roll on arc
#     roll_on_curve.append(rvertex)
#     #end of roll_on roll_off section
# =========================================================================

    # do multiple depths
    layer_count = int((start_depth - final_depth) / stepdown)
    if layer_count * stepdown + 0.00001 < start_depth - final_depth:
        layer_count += 1
    current_start_depth = start_depth
    prev_depth = start_depth
    for i in range(1, layer_count + 1):
        if i == layer_count:
            depth = final_depth
        else:
            depth = start_depth - i * stepdown
        mat_depth = prev_depth
        start_z = mat_depth
        # first move
        output += "G0 X" + str(PathUtils.fmt(offset_curve.GetFirstSpan().p.x)) +\
            " Y" + str(PathUtils.fmt(offset_curve.GetFirstSpan().p.y)) +\
            " Z" + str(PathUtils.fmt(mat_depth + rapid_safety_space)) + "\n"
        # feed down to depth
        mat_depth = depth
        if start_z > mat_depth:
            mat_depth = start_z
        # feed down in Z
        output += "G1 X" + str(PathUtils.fmt(offset_curve.GetFirstSpan().p.x)) +\
            " Y" + str(PathUtils.fmt(offset_curve.GetFirstSpan().p.y)) + " Z" + str(PathUtils.fmt(depth)) +\
            " F" + str(PathUtils.fmt(vertfeed)) + "\n"
        if use_CRC:
            if side_of_line == 'left':
                output += "G41" + "\n"
            else:
                output += "G42" + "\n"
        # cut the main kurve
        current_perim = 0.0
        lastx = offset_curve.GetFirstSpan().p.x
        lasty = offset_curve.GetFirstSpan().p.y
        for span in offset_curve.GetSpans():
            current_perim += span.Length()
            if span.v.type == 0:  # line
                # feed(span.v.p.x, span.v.p.y, ez)
                output += "G1 X" + str(PathUtils.fmt(span.v.p.x)) + " Y" + str(PathUtils.fmt(span.v.p.y)) +\
                    " Z" + str(PathUtils.fmt(depth)) + " F" + \
                    str(PathUtils.fmt(horizfeed)) + "\n"
                lastx = span.v.p.x
                lasty = span.v.p.y
            elif (span.v.type == 1) or (span.v.type == -1):
                if span.v.type == 1:  # anti-clockwise arc
                    command = 'G3'
                elif span.v.type == -1:  # clockwise arc
                    command = 'G2'
                arc_I = span.v.c.x - lastx
                arc_J = span.v.c.y - lasty
                output += command + "X" + str(PathUtils.fmt(span.v.p.x)) + " Y" + str(
                    PathUtils.fmt(span.v.p.y))  # +" Z"+ str(PathUtils.fmt(depth))
                output += " I" + str(PathUtils.fmt(arc_I)) + " J" + str(PathUtils.fmt(arc_J)) + " F" + str(
                    PathUtils.fmt(horizfeed)) + '\n'  # " K"+str(PathUtils.fmt(depth)) +"\n"
                lastx = span.v.p.x
                lasty = span.v.p.y
            else:
                raise Exception, "valid geometry identifier needed"
        if use_CRC:
            # end_CRC()
            output += "G40" + "\n"
        # rapid up to the clearance height
        output += "G0 Z" + str(PathUtils.fmt(clearance)) + "\n"

    del offset_curve

    return output


def make_smaller(curve, start=None, finish=None, end_beyond=False):
    if start is not None:
        curve.ChangeStart(curve.NearestPoint(start))

    if finish is not None:
        if end_beyond:
            curve2 = area.Curve(curve)
            curve2.ChangeEnd(curve2.NearestPoint(finish))
            first = True
            for vertex in curve2.getVertices():
                if first is False:
                    curve.append(vertex)
                first = False
        else:
            curve.ChangeEnd(curve.NearestPoint(finish))


'''The following procedures are copied almost directly from heekscnc
kurve_funcs.py.  They depend on nc directory existing below PathScripts
and have not been throughly optimized, understood, or tested for FreeCAD.'''


def profile2(curve, direction="on", radius=1.0, vertfeed=0.0,
             horizfeed=0.0, vertrapid=0.0, horizrapid=0.0, offset_extra=0.0,
             roll_radius=2.0, roll_on=None, roll_off=None, depthparams=None,
             extend_at_start=0.0, extend_at_end=0.0, lead_in_line_len=0.0,
             lead_out_line_len=0.0):

    # print "direction: " + str(direction)
    # print "radius: " + str(radius)
    # print "vertfeed: " + str(vertfeed)
    # print "horizfeed: " + str(horizfeed)
    # print "offset_extra: " + str(offset_extra)
    # print "roll_radius: " + str(roll_radius)
    # print "roll_on: " + str(roll_on)
    # print "roll_off: " + str(roll_off)
    # print "depthparams: " + str(depthparams)
    # print "extend_at_start: " + str(extend_at_start)
    # print "extend_at_end: " + str(extend_at_end)
    # print "lead_in_line_len: " + str(lead_in_line_len)
    # print "lead_out_line_len: " + str(lead_out_line_len)
    # print "in profile2: 318"

    global tags
    direction = direction.lower()
    offset_curve = area.Curve(curve)
    # print "curve: " , str(curve) 
    # print "result curve: ", offset_curve.__dict__

    if direction == "on":
        use_CRC() == False

    if direction != "on":
        if direction != "left" and direction != "right":
            raise "direction must be left or right", direction

        # get tool diameter
        offset = radius + offset_extra
        if use_CRC() is False or (use_CRC() is True and CRC_nominal_path() is True):
            if math.fabs(offset) > 0.00005:
                if direction == "right":
                    offset = -offset
                offset_success = offset_curve.Offset(offset)
                if offset_success is False:
                    global using_area_for_offset
                    if curve.IsClosed() and (using_area_for_offset is False):
                        cw = curve.IsClockwise()
                        using_area_for_offset = True
                        a = area.Area()
                        a.append(curve)
                        print "curve, offset: " , str(curve), str(offset)
                        a.Offset(-offset)
                        for curve in a.getCurves():
                            print "result curve: ", curve
                            curve_cw = curve.IsClockwise()
                            if cw != curve_cw:
                                curve.Reverse()
                            set_good_start_point(curve, False)
                            profile(curve, direction, 0.0, 0.0, roll_radius, roll_on, roll_off, depthparams,
                                    extend_at_start, extend_at_end, lead_in_line_len, lead_out_line_len)
                        using_area_for_offset = False
                        return
                    else:
                        raise Exception, "couldn't offset kurve " + \
                            str(offset_curve)

    # extend curve
    if extend_at_start > 0.0:
        span = offset_curve.GetFirstSpan()
        new_start = span.p + span.GetVector(0.0) * (-extend_at_start)
        new_curve = area.Curve()
        new_curve.append(new_start)
        for vertex in offset_curve.getVertices():
            new_curve.append(vertex)
        offset_curve = new_curve

    if extend_at_end > 0.0:
        span = offset_curve.GetLastSpan()
        new_end = span.v.p + span.GetVector(1.0) * extend_at_end
        offset_curve.append(new_end)

    # remove tags further than radius from the offset kurve
    new_tags = []
    for tag in tags:
        if tag.dist(offset_curve) <= radius + 0.001:
            new_tags.append(tag)
    tags = new_tags

    if offset_curve.getNumVertices() <= 1:
        raise "sketch has no spans!"

    # do multiple depths
    depths = depthparams.get_depths()

    current_start_depth = depthparams.start_depth

    # tags
    if len(tags) > 0:
        # make a copy to restore to after each level
        copy_of_offset_curve = area.Curve(offset_curve)

    prev_depth = depthparams.start_depth

    endpoint = None

    for depth in depths:
        mat_depth = prev_depth

        if len(tags) > 0:
            split_for_tags(
                offset_curve, radius, depthparams.start_depth, depth, depthparams.final_depth)

        # make the roll on and roll off kurves
        roll_on_curve = area.Curve()
        add_roll_on(offset_curve, roll_on_curve, direction,
                    roll_radius, offset_extra, roll_on)
        roll_off_curve = area.Curve()
        add_roll_off(offset_curve, roll_off_curve, direction,
                     roll_radius, offset_extra, roll_off)
        if use_CRC():
            crc_start_point = area.Point()
            add_CRC_start_line(offset_curve, roll_on_curve, roll_off_curve,
                               radius, direction, crc_start_point, lead_in_line_len)

        # get the tag depth at the start
        start_z = get_tag_z_for_span(
            0, offset_curve, radius, depthparams.start_depth, depth, depthparams.final_depth)
        if start_z > mat_depth:
            mat_depth = start_z

        # rapid across to the start
        s = roll_on_curve.FirstVertex().p

        # start point
        if (endpoint is None) or (endpoint != s):
            if use_CRC():
                rapid(crc_start_point.x, crc_start_point.y) + "F " + horizrapid + "\n"
            else:
                rapid(s.x, s.y) #+ "F " + str(horizrapid) + "\n"

            # rapid down to just above the material
            if endpoint is None:
                rapid(z=mat_depth + depthparams.rapid_safety_space) #+ "F " + vertrapid + "\n"

            else:
                rapid(z=mat_depth) #+ "F " + str(vertrapid) + "\n"


        # feed down to depth
        mat_depth = depth
        if start_z > mat_depth:
            mat_depth = start_z
        feed(s.x, s.y, z=mat_depth)

        if use_CRC():
            start_CRC(direction == "left", radius)
            # move to the startpoint
            feed(s.x, s.y)

        # cut the roll on arc
        cut_curve(roll_on_curve)

        # cut the main kurve
        current_perim = 0.0

        for span in offset_curve.GetSpans():
            # height for tags
            current_perim += span.Length()
            ez = get_tag_z_for_span(current_perim, offset_curve, radius,
                                    depthparams.start_depth, depth, depthparams.final_depth)
            if ez is None:
                ez = depth
            if span.v.type == 0:  # line
                feed(span.v.p.x, span.v.p.y, ez)
            else:
                if span.v.type == 1:  # anti-clockwise arc
                    arc_ccw(span.v.p.x, span.v.p.y, ez,
                            i=span.v.c.x, j=span.v.c.y)
                else:
                    arc_cw(span.v.p.x, span.v.p.y, ez,
                           i=span.v.c.x, j=span.v.c.y)

        # cut the roll off arc
        cut_curve(roll_off_curve)

        endpoint = offset_curve.LastVertex().p
        if roll_off_curve.getNumVertices() > 0:
            endpoint = roll_off_curve.LastVertex().p

        # add CRC end_line
        if use_CRC():
            crc_end_point = area.Point()
            add_CRC_end_line(offset_curve, roll_on_curve, roll_off_curve,
                             radius, direction, crc_end_point, lead_out_line_len)
            if direction == "on":
                rapid(z=depthparams.clearance_height) #+ "F " + vertrapid + "\n"
            else:
                feed(crc_end_point.x, crc_end_point.y)

        # restore the unsplit kurve
        if len(tags) > 0:
            offset_curve = area.Curve(copy_of_offset_curve)
        if use_CRC():
            end_CRC()

        if endpoint != s:
            # rapid up to the clearance height
            rapid(z=depthparams.clearance_height)# + "F " + vertrapid + "\n"

        prev_depth = depth

    rapid(z=depthparams.clearance_height)# + "F " + vertrapid + "\n"

    del offset_curve

    if len(tags) > 0:
        del copy_of_offset_curve


class Tag:

    def __init__(self, p, width, angle, height):
        self.p = p
        self.width = width  # measured at the top of the tag. In the toolpath, the tag width will be this with plus the tool diameter, so that the finished tag has this "width" at it's smallest
        # the angle of the ramp in radians. Between 0 and Pi/2; 0 is
        # horizontal, Pi/2 is vertical
        self.angle = angle
        self.height = height  # the height of the tag, always measured above "final_depth"
        self.ramp_width = self.height / math.tan(self.angle)

    def split_curve(self, curve, radius, start_depth, depth, final_depth):
        tag_top_depth = final_depth + self.height

        if depth > tag_top_depth - 0.0000001:
            return  # kurve is above this tag, so doesn't need splitting

        height_above_depth = tag_top_depth - depth
        ramp_width_at_depth = height_above_depth / math.tan(self.angle)
        cut_depth = start_depth - depth
        half_flat_top = radius + self.width / 2

        d = curve.PointToPerim(self.p)
        d0 = d - half_flat_top
        perim = curve.Perim()
        if curve.IsClosed():
            while d0 < 0:
                d0 += perim
            while d0 > perim:
                d0 -= perim
        p = curve.PerimToPoint(d0)
        curve.Break(p)
        d1 = d + half_flat_top
        if curve.IsClosed():
            while d1 < 0:
                d1 += perim
            while d1 > perim:
                d1 -= perim
        p = curve.PerimToPoint(d1)
        curve.Break(p)

        d0 = d - half_flat_top - ramp_width_at_depth
        if curve.IsClosed():
            while d0 < 0:
                d0 += perim
            while d0 > perim:
                d0 -= perim
        p = curve.PerimToPoint(d0)
        curve.Break(p)
        d1 = d + half_flat_top + ramp_width_at_depth
        if curve.IsClosed():
            while d1 < 0:
                d1 += perim
            while d1 > perim:
                d1 -= perim
        p = curve.PerimToPoint(d1)
        curve.Break(p)

    def get_z_at_perim(self, current_perim, curve, radius, start_depth, depth, final_depth):
        # return the z for this position on the kurve ( specified by current_perim ), for this tag
        # if the position is not within the tag, then depth is returned
        cut_depth = start_depth - depth
        half_flat_top = radius + self.width / 2

        z = depth
        d = curve.PointToPerim(self.p)
        dist_from_d = math.fabs(current_perim - d)
        if dist_from_d < half_flat_top:
            # on flat top of tag
            z = final_depth + self.height
        elif dist_from_d < half_flat_top + self.ramp_width:
            # on ramp
            dist_up_ramp = (half_flat_top + self.ramp_width) - dist_from_d
            z = final_depth + dist_up_ramp * math.tan(self.angle)
        if z < depth:
            z = depth
        return z

    def dist(self, curve):
        # return the distance from the tag point to the given kurve
        d = curve.PointToPerim(self.p)
        p = curve.PerimToPoint(d)
        v = self.p - p
        return v.length()

tags = []


def add_roll_on(curve, roll_on_curve, direction, roll_radius, offset_extra, roll_on):
    if direction == "on":
        roll_on = None
    if curve.getNumVertices() <= 1:
        return
    first_span = curve.GetFirstSpan()

    if roll_on is None:
        rollstart = first_span.p
    elif roll_on == 'auto':
        if roll_radius < 0.0000000001:
            rollstart = first_span.p
        v = first_span.GetVector(0.0)
        if direction == 'right':
            off_v = area.Point(v.y, -v.x)
        else:
            off_v = area.Point(-v.y, v.x)
        rollstart = first_span.p + off_v * roll_radius
    else:
        rollstart = roll_on

    rvertex = area.Vertex(first_span.p)

    if first_span.p == rollstart:
        rvertex.type = 0
    else:
        v = first_span.GetVector(0.0)  # get start direction
        rvertex.c, rvertex.type = area.TangentialArc(
            first_span.p, rollstart, -v)
        rvertex.type = -rvertex.type  # because TangentialArc was used in reverse
    # add a start roll on point
    roll_on_curve.append(rollstart)

    # add the roll on arc
    roll_on_curve.append(rvertex)


def add_roll_off(curve, roll_off_curve, direction, roll_radius, offset_extra, roll_off):
    if direction == "on":
        return
    if roll_off is None:
        return
    if curve.getNumVertices() <= 1:
        return

    last_span = curve.GetLastSpan()

    if roll_off == 'auto':
        if roll_radius < 0.0000000001:
            return
        v = last_span.GetVector(1.0)  # get end direction
        if direction == 'right':
            off_v = area.Point(v.y, -v.x)
        else:
            off_v = area.Point(-v.y, v.x)

        rollend = last_span.v.p + off_v * roll_radius
    else:
        rollend = roll_off

    # add the end of the original kurve
    roll_off_curve.append(last_span.v.p)
    if rollend == last_span.v.p:
        return
    rvertex = area.Vertex(rollend)
    v = last_span.GetVector(1.0)  # get end direction
    rvertex.c, rvertex.type = area.TangentialArc(last_span.v.p, rollend, v)

    # add the roll off arc
    roll_off_curve.append(rvertex)


def clear_tags():
    global tags
    tags = []


def add_tag(p, width, angle, height):
    global tags
    tag = Tag(p, width, angle, height)
    tags.append(tag)


def split_for_tags(curve, radius, start_depth, depth, final_depth):
    global tags
    for tag in tags:
        tag.split_curve(curve, radius, start_depth, depth, final_depth)


def get_tag_z_for_span(current_perim, curve, radius, start_depth, depth, final_depth):
    global tags
    max_z = None
    perim = curve.Perim()
    for tag in tags:
        z = tag.get_z_at_perim(current_perim, curve,
                               radius, start_depth, depth, final_depth)
        if max_z is None or z > max_z:
            max_z = z
        if curve.IsClosed():
            # do the same test, wrapped around the closed kurve
            z = tag.get_z_at_perim(
                current_perim - perim, curve, radius, start_depth, depth, final_depth)
            if max_z is None or z > max_z:
                max_z = z
            z = tag.get_z_at_perim(
                current_perim + perim, curve, radius, start_depth, depth, final_depth)
            if max_z is None or z > max_z:
                max_z = z

    return max_z


def cut_curve(curve):
    for span in curve.GetSpans():
        if span.v.type == 0:  # line
            feed(span.v.p.x, span.v.p.y)
        else:
            if span.v.type == 1:  # anti-clockwise arc
                arc_ccw(span.v.p.x, span.v.p.y, i=span.v.c.x, j=span.v.c.y)
            else:
                arc_cw(span.v.p.x, span.v.p.y, i=span.v.c.x, j=span.v.c.y)


def add_CRC_start_line(curve, roll_on_curve, roll_off_curve, radius, direction, crc_start_point, lead_in_line_len):
    first_span = curve.GetFirstSpan()
    v = first_span.GetVector(0.0)
    if direction == 'right':
        off_v = area.Point(v.y, -v.x)
    else:
        off_v = area.Point(-v.y, v.x)
    startpoint_roll_on = roll_on_curve.FirstVertex().p
    crc_start = startpoint_roll_on + off_v * lead_in_line_len
    crc_start_point.x = crc_start.x
    crc_start_point.y = crc_start.y


def add_CRC_end_line(curve, roll_on_curve, roll_off_curve, radius, direction, crc_end_point, lead_out_line_len):
    last_span = curve.GetLastSpan()
    v = last_span.GetVector(1.0)
    if direction == 'right':
        off_v = area.Point(v.y, -v.x)
    else:
        off_v = area.Point(-v.y, v.x)
    endpoint_roll_off = roll_off_curve.LastVertex().p
    crc_end = endpoint_roll_off + off_v * lead_out_line_len
    crc_end_point.x = crc_end.x
    crc_end_point.y = crc_end.y

using_area_for_offset = False
