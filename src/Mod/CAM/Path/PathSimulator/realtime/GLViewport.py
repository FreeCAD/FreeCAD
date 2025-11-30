"""
GLViewport stub - writes a small JSON referencing frame OBJ files and axis states.
A real OpenGL/Qt viewer would consume this.
"""

import json, os


def create_view_manifest(frames_dir="realtime_frames", out="view_manifest.json"):
    objs = sorted([f for f in os.listdir(frames_dir) if f.endswith(".obj")])
    data = {"frames": objs, "frames_dir": frames_dir}
    with open(out, "w") as f:
        json.dump(data, f, indent=2)
    return out
