from KinematicModelBase import KinematicModelBase
import math


class MachineKinematics4Axis(KinematicModelBase):
    def inverse(self, tool_pos, tool_orient):
        # naive: compute rotation A about Z to align X axis with tool_orient XY projection
        dx, dy, dz = tool_orient
        A = math.degrees(math.atan2(dy, dx))
        return {"X": tool_pos[0], "Y": tool_pos[1], "Z": tool_pos[2], "A": A}
