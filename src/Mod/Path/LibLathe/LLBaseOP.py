import math 
import LibLathe.LLUtils as utils
from LibLathe.LLPoint import Point
from LibLathe.LLSegment import Segment
from LibLathe.LLSegmentGroup import SegmentGroup
from LibLathe.LLTool import Tool

class BaseOP:
    def __init__(self):

        self.stock = None
        self.part = None
        self.tool = Tool()
        #self.part_edges = None
        self.part_segment_group = SegmentGroup()

        self.offset_edges = []
        self.clearing_paths = []
        
        self.min_dia = 0
        self.extra_dia = 0
        self.start_offset = 0
        self.end_offset = 0
        self.allow_grooving = False
        self.allow_facing = False
        self.step_over = 1.5
        self.finish_passes = 2
        self.hfeed = 100
        self.vfeed = 50

    def set_params(self, params):
        for param in params:
            setattr(self, param, params[param])
            #print(param, params[param])

    def get_params(self):
        pass

    def get_gcode(self):
        '''
        Base function for all turning operations
        '''
        #print('[LLBaseOP] - get_gcode')
        self.remove_the_groove()
        self.offset_part_outline()
        self.generate_path()
        Path = self.generate_gcode()
        #self.clean_up()
        return Path
        
    def remove_the_groove(self):
        '''
        Remove grooves and undercuts from part geometry
        '''
        
        if not self.allow_grooving:
            self.part_segment_group = utils.remove_the_groove(self.part_segment_group, self.stock.ZMin, self.tool)
            self.offset_edges.append(self.part_segment_group)    
        
        #path_profile = Part.makePolygon(profile_points)
        #path_profile = Part.makeCompound(self.part)
        #Part.show(path_profile, 'Final_pass')    
        
 
    def offset_part_outline(self):
        '''
        ####################
        # 5. Offset Part Outline
        ####################
        '''
        f_pass = 1
        while f_pass != self.finish_passes:
            #print('fpass', f_pass, self.finish_passes)
            segmentGroup = utils.offsetPath(self.part_segment_group, self.step_over * f_pass)  
            self.offset_edges.append(segmentGroup)
            f_pass += 1
        
        #if len(self.offset_edges):
        #    self.finish_path = []
        #    for path in self.offset_edges:
        #        for edge in path:
        #            self.finish_path.append(edge)
            #offset_profile = Part.makeCompound(finish_path)
            #Part.show(offset_profile, 'finishing_pass')
    
    def generate_path(self):
        '''
        Main processing function for each op
        '''
        pass
         
    def generate_gcode(self):
        '''
        Generate Gcode for the op segments
        '''
        return ""
           
    def add_part(self, part_bb):
        #print('Add_part', part_edges)
        self.part = part_bb

    def add_part_edges(self, part_edges):
        #print('Add_part', part_edges)
        #self.part_edges = part_edges
        for segment in part_edges:
            self.part_segment_group.add_segment(segment)
        
    def add_stock(self, stock_bb):
        self.stock = stock_bb
        






