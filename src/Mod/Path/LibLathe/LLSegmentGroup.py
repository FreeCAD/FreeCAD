class SegmentGroup:
    '''
    Container Group for segments
    '''
    def __init__(self):
        self.segments = []

    def add_segment(self, segment):
        '''
        Add segment to group 
        '''
        self.segments.append(segment)
    
    def get_segments(self):
        '''
        Return segments of group as a list
        '''
        return self.segments
    
    def extend(self, segmentGroup):
        '''
        Add segment group to this segmentgroup
        '''
        self.segments.extend(segmentGroup.get_segments())

    def count(self):
        '''
        Return the number of segments in the segmentgroup
        '''
        return len(self.segments)