import area
from nc.nc import *
import PathScripts.nc.iso
import math
import PathKurveUtils

# some globals, to save passing variables as parameters too much
area_for_feed_possible = None
tool_radius_for_pocket = None

def cut_curve(curve, need_rapid, p, rapid_safety_space, current_start_depth, final_depth):
    prev_p = p
    first = True
    #comment("cut_curve:14 rss:" + str(rapid_safety_space) + " current start depth :" + str(current_start_depth) + " final depth :" + str(final_depth) + " need rapid: " + str(need_rapid))
    for vertex in curve.getVertices():
        if need_rapid and first:
            # rapid across
            rapid(vertex.p.x, vertex.p.y)
            ##rapid down
            rapid(z = current_start_depth + rapid_safety_space)
            #feed down
            feed(z = final_depth)
            first = False
        else:
            if vertex.type == 1:
                arc_ccw(vertex.p.x, vertex.p.y, i = vertex.c.x, j = vertex.c.y)
            elif vertex.type == -1:
                arc_cw(vertex.p.x, vertex.p.y, i = vertex.c.x, j = vertex.c.y)
            else:
                feed(vertex.p.x, vertex.p.y)
        prev_p = vertex.p
    return prev_p

def area_distance(a, old_area):
    best_dist = None

    for curve in a.getCurves():
        for vertex in curve.getVertices():
            c = old_area.NearestPoint(vertex.p)
            d = c.dist(vertex.p)
            if best_dist == None or d < best_dist:
                best_dist = d

    for curve in old_area.getCurves():
        for vertex in curve.getVertices():
            c = a.NearestPoint(vertex.p)
            d = c.dist(vertex.p)
            if best_dist == None or d < best_dist:
                best_dist = d

    return best_dist

def make_obround(p0, p1, radius):
    dir = p1 - p0
    d = dir.length()
    dir.normalize()
    right = area.Point(dir.y, -dir.x)
    obround = area.Area()
    c = area.Curve()
    vt0 = p0 + right * radius
    vt1 = p1 + right * radius
    vt2 = p1 - right * radius
    vt3 = p0 - right * radius
    c.append(area.Vertex(0, vt0, area.Point(0, 0)))
    c.append(area.Vertex(0, vt1, area.Point(0, 0)))
    c.append(area.Vertex(1, vt2, p1))
    c.append(area.Vertex(0, vt3, area.Point(0, 0)))
    c.append(area.Vertex(1, vt0, p0))
    obround.append(c)
    return obround

def feed_possible(p0, p1):
    if p0 == p1:
        return True
    obround = make_obround(p0, p1, tool_radius_for_pocket)
    a = area.Area(area_for_feed_possible)
    obround.Subtract(a)
    if obround.num_curves() > 0:
        return False
    return True

def cut_curvelist1(curve_list, rapid_safety_space, current_start_depth, depth, clearance_height, keep_tool_down_if_poss):
    p = area.Point(0, 0)
    first = True
    for curve in curve_list:
        need_rapid = True
        if first == False:
            s = curve.FirstVertex().p
            if keep_tool_down_if_poss == True:
                # see if we can feed across
                if feed_possible(p, s):
                    need_rapid = False
            elif s.x == p.x and s.y == p.y:
                need_rapid = False
        if need_rapid:
            rapid(z = clearance_height)
        p = cut_curve(curve, need_rapid, p, rapid_safety_space, current_start_depth, depth)
        first = False

    rapid(z = clearance_height)

def cut_curvelist2(curve_list, rapid_safety_space, current_start_depth, depth, clearance_height, keep_tool_down_if_poss,start_point):
    p = area.Point(0, 0)
    start_x,start_y=start_point
    first = True
    for curve in curve_list:
        need_rapid = True
        if first == True:
            direction = "On";radius = 0.0;offset_extra = 0.0; roll_radius = 0.0;roll_on = 0.0; roll_off = 0.0; rapid_safety_space; step_down = math.fabs(depth);extend_at_start = 0.0;extend_at_end = 0.0
            PathKurveUtils.make_smaller( curve, start = area.Point(start_x,start_y))
            PathKurveUtils.profile(curve, direction, radius , offset_extra, roll_radius, roll_on, roll_off, rapid_safety_space , clearance_height, current_start_depth, step_down , depth, extend_at_start, extend_at_end)
        else:
            s = curve.FirstVertex().p
            if keep_tool_down_if_poss == True:

                # see if we can feed across
                if feed_possible(p, s):
                    need_rapid = False
            elif s.x == p.x and s.y == p.y:
                need_rapid = False

        cut_curve(curve, need_rapid, p, rapid_safety_space, current_start_depth, depth)
        first = False #change to True if you want to rapid back to start side before zigging again with unidirectional set
    rapid(z = clearance_height)

def recur(arealist, a1, stepover, from_center):
    # this makes arealist by recursively offsetting a1 inwards

    if a1.num_curves() == 0:
        return

    if from_center:
        arealist.insert(0, a1)
    else:
        arealist.append(a1)

    a_offset = area.Area(a1)
    a_offset.Offset(stepover)

    # split curves into new areas
    if area.holes_linked():
        for curve in a_offset.getCurves():
            a2 = area.Area()
            a2.append(curve)
            recur(arealist, a2, stepover, from_center)

    else:
        # split curves into new areas
        a_offset.Reorder()
        a2 = None

        for curve in a_offset.getCurves():
            if curve.IsClockwise():
                if a2 != None:
                    a2.append(curve)
            else:
                if a2 != None:
                    recur(arealist, a2, stepover, from_center)
                a2 = area.Area()
                a2.append(curve)

        if a2 != None:
            recur(arealist, a2, stepover, from_center)

def get_curve_list(arealist, reverse_curves = False):
    curve_list = list()
    for a in arealist:
        for curve in a.getCurves():
            if reverse_curves == True:
                curve.Reverse()
            curve_list.append(curve)
    return curve_list

curve_list_for_zigs = []
rightward_for_zigs = True
sin_angle_for_zigs = 0.0
cos_angle_for_zigs = 1.0
sin_minus_angle_for_zigs = 0.0
cos_minus_angle_for_zigs = 1.0
one_over_units = 1.0

def make_zig_curve(curve, y0, y, zig_unidirectional):
    if rightward_for_zigs:
        curve.Reverse()

    # find a high point to start looking from
    high_point = None
    for vertex in curve.getVertices():
        if high_point == None:
            high_point = vertex.p
        elif vertex.p.y > high_point.y:
            # use this as the new high point
            high_point = vertex.p
        elif math.fabs(vertex.p.y - high_point.y) < 0.002 * one_over_units:
            # equal high point
            if rightward_for_zigs:
                # use the furthest left point
                if vertex.p.x < high_point.x:
                    high_point = vertex.p
            else:
                 # use the furthest right point
                if vertex.p.x > high_point.x:
                    high_point = vertex.p

    zig = area.Curve()

    high_point_found = False
    zig_started = False
    zag_found = False

    for i in range(0, 2): # process the curve twice because we don't know where it will start
        prev_p = None
        for vertex in curve.getVertices():
            if zag_found: break
            if prev_p != None:
                if zig_started:
                    zig.append(unrotated_vertex(vertex))
                    if math.fabs(vertex.p.y - y) < 0.002 * one_over_units:
                        zag_found = True
                        break
                elif high_point_found:
                    if math.fabs(vertex.p.y - y0) < 0.002 * one_over_units:
                        if zig_started:
                            zig.append(unrotated_vertex(vertex))
                        elif math.fabs(prev_p.y - y0) < 0.002 * one_over_units and vertex.type == 0:
                            zig.append(area.Vertex(0, unrotated_point(prev_p), area.Point(0, 0)))
                            zig.append(unrotated_vertex(vertex))
                            zig_started = True
                elif vertex.p.x == high_point.x and vertex.p.y == high_point.y:
                    high_point_found = True
            prev_p = vertex.p

    if zig_started:

        if zig_unidirectional == True:
            # remove the last bit of zig
            if math.fabs(zig.LastVertex().p.y - y) < 0.002 * one_over_units:
                vertices = zig.getVertices()
                while len(vertices) > 0:
                    v = vertices[len(vertices)-1]
                    if math.fabs(v.p.y - y0) < 0.002 * one_over_units:
                        break
                    else:
                        vertices.pop()
                zig = area.Curve()
                for v in vertices:
                    zig.append(v)

        curve_list_for_zigs.append(zig)

def make_zig(a, y0, y, zig_unidirectional):
    for curve in a.getCurves():
        make_zig_curve(curve, y0, y, zig_unidirectional)

reorder_zig_list_list = []

def add_reorder_zig(curve):
    global reorder_zig_list_list

    # look in existing lists
    s = curve.FirstVertex().p
    for curve_list in reorder_zig_list_list:
        last_curve = curve_list[len(curve_list) - 1]
        e = last_curve.LastVertex().p
        if math.fabs(s.x - e.x) < 0.002 * one_over_units and math.fabs(s.y - e.y) < 0.002 * one_over_units:
            curve_list.append(curve)
            return

    # else add a new list
    curve_list = []
    curve_list.append(curve)
    reorder_zig_list_list.append(curve_list)

def reorder_zigs():
    global curve_list_for_zigs
    global reorder_zig_list_list
    reorder_zig_list_list = []
    for curve in curve_list_for_zigs:
        add_reorder_zig(curve)

    curve_list_for_zigs = []
    for curve_list in reorder_zig_list_list:
        for curve in curve_list:
            curve_list_for_zigs.append(curve)

def rotated_point(p):
    return area.Point(p.x * cos_angle_for_zigs - p.y * sin_angle_for_zigs, p.x * sin_angle_for_zigs + p.y * cos_angle_for_zigs)

def unrotated_point(p):
    return area.Point(p.x * cos_minus_angle_for_zigs - p.y * sin_minus_angle_for_zigs, p.x * sin_minus_angle_for_zigs + p.y * cos_minus_angle_for_zigs)

def rotated_vertex(v):
    if v.type:
        return area.Vertex(v.type, rotated_point(v.p), rotated_point(v.c))
    return area.Vertex(v.type, rotated_point(v.p), area.Point(0, 0))

def unrotated_vertex(v):
    if v.type:
        return area.Vertex(v.type, unrotated_point(v.p), unrotated_point(v.c))
    return area.Vertex(v.type, unrotated_point(v.p), area.Point(0, 0))

def rotated_area(a):
    an = area.Area()
    for curve in a.getCurves():
        curve_new = area.Curve()
        for v in curve.getVertices():
            curve_new.append(rotated_vertex(v))
        an.append(curve_new)
    return an

def zigzag(a, stepover, zig_unidirectional):
    if a.num_curves() == 0:
        return

    global rightward_for_zigs
    global curve_list_for_zigs
    global sin_angle_for_zigs
    global cos_angle_for_zigs
    global sin_minus_angle_for_zigs
    global cos_minus_angle_for_zigs
    global one_over_units

    one_over_units = 1 / area.get_units()

    a = rotated_area(a)

    b = area.Box()
    a.GetBox(b)

    x0 = b.MinX() - 1.0
    x1 = b.MaxX() + 1.0

    height = b.MaxY() - b.MinY()
    num_steps = int(height / stepover + 1)
    y = b.MinY() + 0.1 * one_over_units
    null_point = area.Point(0, 0)
    rightward_for_zigs = True
    curve_list_for_zigs = []

    for i in range(0, num_steps):
        y0 = y
        y = y + stepover
        p0 = area.Point(x0, y0)
        p1 = area.Point(x0, y)
        p2 = area.Point(x1, y)
        p3 = area.Point(x1, y0)
        c = area.Curve()
        c.append(area.Vertex(0, p0, null_point, 0))
        c.append(area.Vertex(0, p1, null_point, 0))
        c.append(area.Vertex(0, p2, null_point, 1))
        c.append(area.Vertex(0, p3, null_point, 0))
        c.append(area.Vertex(0, p0, null_point, 1))
        a2 = area.Area()
        a2.append(c)
        a2.Intersect(a)
        make_zig(a2, y0, y, zig_unidirectional)
        if zig_unidirectional == False:
            rightward_for_zigs = (rightward_for_zigs == False)

    reorder_zigs()

def pocket(a,tool_radius, extra_offset, stepover, depthparams, from_center, keep_tool_down_if_poss, use_zig_zag, zig_angle, zig_unidirectional = False,start_point=None, cut_mode = 'conventional'):
    global tool_radius_for_pocket
    global area_for_feed_possible
    #if len(a.getCurves()) > 1:
    #    for crv in a.getCurves():
    #        ar = area.Area()
    #        ar.append(crv)
    #        pocket(ar, tool_radius, extra_offset, rapid_safety_space, start_depth, final_depth, stepover, stepdown, clearance_height, from_center, keep_tool_down_if_poss, use_zig_zag, zig_angle, zig_unidirectional)
    #    return

    tool_radius_for_pocket = tool_radius

    if keep_tool_down_if_poss:
        area_for_feed_possible = area.Area(a)
        area_for_feed_possible.Offset(extra_offset - 0.01)

    use_internal_function = False #(area.holes_linked() == False) # use internal function, if area module is the Clipper library

    if use_internal_function:
        curve_list = a.MakePocketToolpath(tool_radius, extra_offset, stepover, from_center, use_zig_zag, zig_angle)

    else:
        global sin_angle_for_zigs
        global cos_angle_for_zigs
        global sin_minus_angle_for_zigs
        global cos_minus_angle_for_zigs
        radians_angle = zig_angle * math.pi / 180
        sin_angle_for_zigs = math.sin(-radians_angle)
        cos_angle_for_zigs = math.cos(-radians_angle)
        sin_minus_angle_for_zigs = math.sin(radians_angle)
        cos_minus_angle_for_zigs = math.cos(radians_angle)

        arealist = list()

        a_offset = area.Area(a)
        current_offset = tool_radius + extra_offset
        a_offset.Offset(current_offset)

        do_recursive = True

        if use_zig_zag:
            zigzag(a_offset, stepover, zig_unidirectional)
            curve_list = curve_list_for_zigs
        else:
            if do_recursive:
                recur(arealist, a_offset, stepover, from_center)
            else:
                while(a_offset.num_curves() > 0):
                    if from_center:
                        arealist.insert(0, a_offset)
                    else:
                        arealist.append(a_offset)
                    current_offset = current_offset + stepover
                    a_offset = area.Area(a)
                    a_offset.Offset(current_offset)
            curve_list = get_curve_list(arealist, cut_mode == 'climb')
            
    depths = depthparams.get_depths()
    current_start_depth = depthparams.start_depth
    if start_point==None:
        for depth in depths:
            cut_curvelist1(curve_list, depthparams.rapid_safety_space, current_start_depth, depth, depthparams.clearance_height, keep_tool_down_if_poss)
            current_start_depth = depth

    else:
        for depth in depths:
            cut_curvelist2(curve_list, depthparams.rapid_safety_space, current_start_depth, depth, depthparams.clearance_height, keep_tool_down_if_poss, start_point)
            current_start_depth = depth


