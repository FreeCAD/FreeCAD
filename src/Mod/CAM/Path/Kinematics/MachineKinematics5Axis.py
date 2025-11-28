from KinematicModelBase import KinematicModelBase
import math

class MachineKinematics5Axis(KinematicModelBase):
    def inverse(self, tool_pos, tool_orient):
        # placeholder: map normal to two angles A and C (very naive)
        nx, ny, nz = tool_orient
        A = math.degrees(math.atan2(ny, nx))
        C = math.degrees(math.atan2(nz, math.sqrt(nx*nx + ny*ny)))
        return {"X": tool_pos[0], "Y": tool_pos[1], "Z": tool_pos[2], "A": A, "C": C}
