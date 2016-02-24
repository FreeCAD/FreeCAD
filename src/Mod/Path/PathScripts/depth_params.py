import math

class depth_params:
    def __init__(self, clearance_height, rapid_safety_space, start_depth, step_down, z_finish_depth, z_thru_depth, final_depth, user_depths):
        self.clearance_height = clearance_height
        self.rapid_safety_space = math.fabs(rapid_safety_space)
        self.start_depth = start_depth
        self.step_down = math.fabs(step_down)
        self.z_finish_depth = math.fabs(z_finish_depth)
        self.z_thru_depth = math.fabs(z_thru_depth)
        self.final_depth = final_depth
        self.user_depths = user_depths
        
    def get_depths(self):
        if self.user_depths != None:
            depths = self.user_depths
        else:
            depth = self.final_depth - self.z_thru_depth
            depths = [depth]
            depth += self.z_finish_depth
            if depth + 0.0000001 < self.start_depth:
                if self.z_finish_depth > 0.0000001: depths.insert(0, depth)
                depth += self.z_thru_depth
                layer_count = int((self.start_depth - depth) / self.step_down - 0.0000001) + 1
                if layer_count > 0:
                    layer_depth = (self.start_depth - depth)/layer_count
                    for i in range(1, layer_count):
                        depth += layer_depth
                        depths.insert(0, depth)
                    
        return depths
