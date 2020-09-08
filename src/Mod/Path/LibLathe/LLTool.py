class Tool:
    def __init__(self, tool_string = None):
        self.shape = None
        self.clearance_angle = None
        self.tolerance = None
        self.type = None
        self.length = None
        self.thickness = None
        self.nose_radius = None
        self.direction = None #LH / RH

    def set_tool(self, tool_string):
        pass

    def get_tool_cutting_angle(self):
        return 275


