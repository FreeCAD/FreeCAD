"""
CLI: run realtime simulation for a generated model
"""
import sys, os

# Add paths for imports
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", "PathGenerator"))
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", "Kinematics"))
sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))

from PathMultiAxisGenerator4Axis import generate_4axis_helix
from MachineKinematics4Axis import MachineKinematics4Axis
from realtime.RealtimeEngine import animate_model_frames
from models.MachineModelLoader import load_model

if __name__ == "__main__":
    machine_cfg = {"name":"TestMachine","limits":{}}
    model = generate_4axis_helix(machine_cfg, radius=10, turns=1, points=180)
    k = MachineKinematics4Axis(machine_cfg)
    frames = animate_model_frames(model, k, outdir="realtime_frames")
    print(f"Generated {frames} frames in realtime_frames/")
