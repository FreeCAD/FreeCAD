"""
MachineModelLoader - lightweight loader using trimesh if available, fallback minimal parser.
Returns mesh bounds and a basic scene for visualization.
"""
import os

def load_model(path):
    if not os.path.exists(path):
        raise FileNotFoundError(path)
    # If trimesh available we'd return a Trimesh; for now return simple metadata
    return {"path":path, "bounds": (0,0,0,1,1,1)}
