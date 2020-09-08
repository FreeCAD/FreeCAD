from LibLathe.LLPoint import Point
from LibLathe.LLSegment import Segment
from LibLathe.LLSegmentGroup import SegmentGroup
from LibLathe.LLVector import Vector
from LibLathe.LLCommand import Command


class Intersection:
    def __init__(self, point, segment):
        self.point = point
        self.seg = segment

def sort_intersections_z(intersections):
    ''' sort the a list of intersections by their z position '''
    sortedPoints = sorted(intersections, key=lambda p: p.point.Z, reverse=True)
    return sortedPoints

def remove_the_groove(segmentGroupIn, stock_zmin, tool):

    segments = segmentGroupIn.get_segments()
    segs_out = SegmentGroup()
    index = 0
    while index < len(segments):
        seg = segments[index]
                   
        if seg.bulge != 0:
            if seg.bulge > 0: 
                seg = Segment(seg.start, seg.end)

            segs_out.add_segment(seg)

        if seg.bulge == 0:
            pt1 = seg.start 
            pt2 = seg.end
            #print('seg angle', segments.index(seg), pt1.angle_to(pt2))
            if pt1.angle_to(pt2) > tool.get_tool_cutting_angle():               
                next_index, pt = find_next_good_edge(segments, index, stock_zmin, tool)
                if next_index == False:
                    seg = Segment(pt1, pt)
                    segs_out.add_segment(seg)
                    break
                if next_index != index: 
                    seg = Segment(pt1, pt)
                    segs_out.add_segment(seg)                  
                    next_pt1 = segments[next_index].start
                    next_pt2 = segments[next_index].end 
                if next_pt1 != pt:
                    seg = Segment(pt1, next_pt2)
                    segs_out.add_segment(seg) 
                    next_index +=1
                            
                index = next_index
                continue
            else:
                segs_out.add_segment(seg)
            
        index += 1 
    return segs_out    

def find_next_good_edge(segments, current_index, stock_zmin, tool):
    index = current_index
    pt1 = segments[index].start
    index += 1    
    while index < len(segments):
        pt2 = segments[index].start       
        if pt1.angle_to(pt2) < tool.get_tool_cutting_angle():
            return index, pt2          
        index += 1
    
    stock_pt =  Point(pt1.X, pt1.Y, stock_zmin)
    seg = Segment(pt1, stock_pt)
    index = current_index
    index += 1
    
    while index < len(segments):
        intersect, point = seg.intersect(segments[index])    
        if intersect:
            #print('Utils intersect:', point.X)
            return index, point

        index += 1
    #No solution :(
    #print('find_next_good_edge: FAILED')
    return False, stock_pt    
            
def offsetPath(segGroupIn, step_over):

    #TODO Sort Edges to ensure they're in order.  See: Part.__sortEdges__()
    #nedges = []  
    segs = segGroupIn.get_segments()
    segmentGroup = SegmentGroup()

    for i in range(len(segs)):
        seg = segs[i]
        if seg.bulge != 0:

            if seg.bulge > 0:
                vec = Vector().normalise(seg.start, seg.get_centre_point())
                vec2 = Vector().normalise(seg.end, seg.get_centre_point())
                pt = vec.multiply(step_over)
                pt2 = vec2.multiply(step_over)
                new_start = seg.start.add(pt)
                new_end = seg.end.add(pt2)

                new_start.X = new_start.X - step_over
                new_end.X = new_end.X - step_over
                rad = seg.get_radius() - step_over
                #print('offsetPath arc dims', new_start.X, new_start.Z, new_end.X, new_end.Z)
            else:
                vec = Vector().normalise(seg.get_centre_point(), seg.start)
                vec2 = Vector().normalise(seg.get_centre_point(), seg.end)
                pt = vec.multiply(step_over)
                pt2 = vec2.multiply(step_over)
                new_start = pt.add(seg.start)
                new_end = pt2.add(seg.end)
                rad = seg.get_radius() + step_over #seg.get_centre_point().distance_to(new_start)
           
            segment = Segment(new_start, new_end)

            
            if seg.bulge < 0:
                rad = 0 - rad
            segment.set_bulge_from_radius(rad)

        if seg.bulge == 0:         
            vec = Vector().normalise(seg.start, seg.end)
            vec = vec.rotate_x(-1.570796)
            pt = vec.multiply(step_over)
            segment = Segment(pt.add(seg.start), pt.add(seg.end))
              
        segmentGroup.add_segment(segment)
        
        
    joinedSegmentsGroup = join_edges(segmentGroup) 
        
    return joinedSegmentsGroup
        
def join_edges(segmentGroupIn):

    segments = segmentGroupIn.get_segments()

    segmentGroupOut = SegmentGroup()
    
    for i in range(len(segments)):

        pt1 = segments[i].start
        pt2 = segments[i].end 

        seg1 = segments[i]    
        if i !=0:
            seg1 = segments[i-1]
            intersect, pt = seg1.intersect(segments[i], extend=True)
            if intersect:
                if type(pt) is list:
                    pt = pt1.nearest(pt)
                pt1 = pt         
        
        if i != len(segments)-1:      
            seg2 = segments[i+1]
            intersect2, pt = seg2.intersect(segments[i], extend=True) 
            if intersect2:
               # print('intersect2')
                if type(pt) is list:
                    #print('join_edges type of', type(pt))
                    pt = pt2.nearest(pt)
                pt2 = pt 

            #print('join_edges', i, pt1, pt2, pt2.X, pt2.Z) 
                         
        if pt1 and pt2:
            if segments[i].bulge != 0:               
                nseg = Segment(pt1, pt2)
                rad = segments[i].get_centre_point().distance_to(pt1)
                if segments[i].bulge < 0:
                    rad = 0 - rad
                nseg.set_bulge_from_radius(rad)
                segmentGroupOut.add_segment(nseg) 
            else:
                segmentGroupOut.add_segment(Segment(pt1, pt2))
        else:
            #No Intersections found. Return the segment in its current state
            #print('join_edges - No Intersection found for index:', i)
            segmentGroupOut.add_segment(segments[i])

    return segmentGroupOut
    
def toPathCommand(part_segment_group, segmentGroup, stock, step_over, hSpeed, vSpeed):
    ''' generates gcode for the geometry within a segment group '''

    def previousSegmentConnected(seg, segments):
        ''' returns true if seg is connect to the previous seg '''

        currentIdx = segments.index(seg)
        previousIdx = currentIdx - 1

        if not currentIdx == 0:
            currentStartPt = seg.start
            previousEndPt = segments[previousIdx].end

            if currentStartPt.is_same(previousEndPt):
                #print('segs are connected')
                return True

        return False 

    def get_min_retract_x(seg, segments, part_segment_group):
        ''' returns the minimum x retract based on the current segments and the part_segments '''
        part_segments = part_segment_group.get_segments()
        currentIdx = segments.index(seg)
        x_values = []

        ## get the xmax from the current pass segments
        for idx, segment in enumerate(segments):
            x_values.append(segment.get_x_max())
            if idx == currentIdx:
                break

        ## get the xmax from the part segments up to the z position of the current segment
        seg_z_max = seg.get_z_max()
        for part_seg in part_segments:

            part_seg_z_max = part_seg.get_z_max()
            x_values.append(part_seg.get_x_max())

            if part_seg_z_max < seg_z_max:
                break

        min_retract_x = max(x_values, key=abs)
        return min_retract_x
            


    segments = segmentGroup.get_segments()

    cmds = []
    #cmd = Path.Command('G17')  #xy plane
    #cmd = Command('(start of section)')
    cmd = Command('G18')   #xz plane
    #cmd = Command('G19')  #yz plane
    cmds.append(cmd)


    for seg in segments:  

        min_x_retract = get_min_retract_x(seg, segments, part_segment_group)
        x_retract = min_x_retract - step_over
        min_z_retract = stock.ZMax
        z_retract = min_z_retract + step_over

        print('min_x_retract:', min_x_retract)
             
        if segments.index(seg) == 0:
            #params = {'X': seg.start.X, 'Y': 0, 'Z': seg.start.Z + step_over, 'F': hSpeed}
            params = {'X': seg.start.X, 'Y': 0, 'Z': z_retract, 'F': hSpeed}
            rapid =  Command('G0', params)
            cmds.append(rapid)    

            params = {'X': seg.start.X, 'Y': 0, 'Z': seg.start.Z, 'F': hSpeed}
            rapid =  Command('G0', params)
            cmds.append(rapid)  
        
        if seg.bulge == 0:
            if not previousSegmentConnected(seg, segments):
                #if edges.index(edge) == 1:
                pt = seg.start #edge.valueAt(edge.FirstParameter) 
                params = {'X': pt.X, 'Y': pt.Y, 'Z': pt.Z, 'F': hSpeed}
                cmd =  Command('G0', params)
                cmds.append(cmd)
            
            pt = seg.end #edge.valueAt(edge.LastParameter)
            params = {'X': pt.X, 'Y': pt.Y, 'Z': pt.Z, 'F': hSpeed}
            cmd =  Command('G1', params)

        if seg.bulge != 0:
            #TODO: define arctype from bulge sign +/-

            pt1 = seg.start
            pt2 = seg.end 
            #print('toPathCommand - bulge', seg.bulge )
            if seg.bulge < 0:
                arcType = 'G2' 
            else:
                arcType = 'G3'
                
            cen = seg.get_centre_point().sub(pt1) 
            #print('toPathCommand arc cen', seg.get_centre_point().X, seg.get_centre_point().Z)          
            params = {'X': pt2.X, 'Z': pt2.Z, 'I': cen.X, 'K': cen.Z, 'F': hSpeed}
            #print('toPathCommand', params)
            cmd =  Command(arcType, params)

        cmds.append(cmd)

        if segments.index(seg) == len(segments)-1:
            params = {'X': x_retract, 'Y': 0, 'Z': seg.end.Z, 'F': hSpeed}
            rapid =  Command('G0', params)
            cmds.append(rapid)

            #params = {'X': x_retract, 'Y': 0, 'Z': segments[0].start.Z + step_over, 'F': hSpeed}
            params = {'X': x_retract, 'Y': 0, 'Z': z_retract, 'F': hSpeed}
            
            rapid =  Command('G0', params)
            cmds.append(rapid)
           
    return cmds
