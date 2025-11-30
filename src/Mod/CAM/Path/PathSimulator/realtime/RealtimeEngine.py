"""
RealtimeEngine - prototype for animating kinematic states and exporting per-frame OBJ snapshots.
"""

import os, math, sys

# Add parent directory to path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))

from visualizer import write_path_obj


def animate_model_frames(model, kinematic_model, outdir="realtime_frames"):
    os.makedirs(outdir, exist_ok=True)
    sampled = model.interpolate()
    frames = 0
    for i, s in enumerate(sampled):
        # For this prototype we export per-sample OBJ of tool position only
        fname = os.path.join(outdir, f"frame_{i:04d}.obj")
        write_path_obj(model, out=fname)  # reuse simple visualizer for quick snapshot
        frames += 1
        if frames > 50:
            break
    return frames
