import math 

import LibLathe.LLBaseOP
import LibLathe.LLUtils as utils
from LibLathe.LLPoint import Point
from LibLathe.LLSegment import Segment
from LibLathe.LLSegmentGroup import SegmentGroup

class ProfileOP(LibLathe.LLBaseOP.BaseOP):
    
    def generate_path(self):
        '''
        Generate the path for the profile operation
        '''
        #xmin = self.stock.XMin - self.extra_dia
        zmax = self.stock.ZMax + self.start_offset            
        
        self.clearing_paths = []
        length = self.stock.ZLength + self.end_offset + self.start_offset 
        width = self.stock.XLength/2 - self.min_dia + self.extra_dia 
        step_over = self.step_over
        line_count = math.ceil(width / step_over)

        xstart = 0 - (step_over * line_count + self.min_dia)

        #roughing_boundary = self.offset_edges[-1]
        roughing_boundary = utils.offsetPath(self.part_segment_group, self.step_over * self.finish_passes)
        self.offset_edges.append(roughing_boundary)
           
        #counter = 0
        #while counter < line_count + 1:
        for roughing_pass in range(line_count + 1):
            xpt = xstart + roughing_pass * self.step_over
            pt1 = Point(xpt, 0 , zmax)
            pt2 = Point(xpt , 0 , zmax-length)
            path_line = Segment(pt1, pt2)
            intersections = []
            for seg in roughing_boundary.get_segments():
                #if roughing_boundary.index(seg) == 0:
                #print(roughing_boundary.index(seg), counter)
                intersect, point = seg.intersect(path_line) 
                if intersect:
                    if type(point) is list:
                        for p in point:
                            intersection = utils.Intersection(p, seg)
                            intersections.append(intersection)
                    else: 
                        #intersections.append(point)
                        intersection = utils.Intersection(point, seg)
                        intersections.append(intersection)

            ## build list of segments
            segmentGroup = SegmentGroup()

            if not intersections:
                pass
                seg = path_line
                segmentGroup.add_segment(seg)

            if len(intersections) == 1:
                ## Only one intersection, trim line to intersection. 
                seg = Segment(pt1, intersections[0].point)
                segmentGroup.add_segment(seg)
            
            if len(intersections) > 1:
                ## more than one intersection
                intersection = utils.Intersection(pt1, None)
                intersections.insert(0, intersection)

                intersection2 = utils.Intersection(pt2, None)
                intersections.append(intersection2)

                intersections = utils.sort_intersections_z(intersections)

                for i in range(len(intersections)):
                    if i + 1 < len(intersections):
                        if intersections[i].seg:
                            if intersections[i].seg.is_same(intersections[i+1].seg):
                                #print('segments Match')
                                seg = intersections[i].seg
                                rad = seg.get_radius()

                                if seg.bulge < 0:
                                    rad = 0 - rad

                                path_line = Segment(intersections[i].point, intersections[i+1].point)
                                path_line.set_bulge_from_radius(rad)

                                segmentGroup.add_segment(path_line)

                        if i % 2 == 0:
                            #print('intersection:', i, 'of', len(intersections), i % 2)
                            path_line = Segment(intersections[i].point, intersections[i+1].point)
                            segmentGroup.add_segment(path_line)

            if segmentGroup.count():
                self.clearing_paths.append(segmentGroup)
                    
        #clearing_lines = Part.makeCompound(self.clearing_paths)
        #Part.show(clearing_lines, 'clearing_path')
        
    def generate_gcode(self):
        '''
        Generate Gcode for the op segments
        '''
        Path = []

        for segmentGroup in self.clearing_paths: 
            rough = utils.toPathCommand(self.part_segment_group, segmentGroup, self.stock, self.step_over, self.hfeed,  self.vfeed)
            Path.append(rough)
        for segmentGroup in self.offset_edges:   
            finish = utils.toPathCommand(self.part_segment_group, segmentGroup, self.stock,  self.step_over, self.hfeed,  self.vfeed)
            Path.append(finish)

        return Path





