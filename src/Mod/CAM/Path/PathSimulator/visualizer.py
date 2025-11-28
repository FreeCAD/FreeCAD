"""
Visualizer: output simple OBJ polyline + optional collision markers
"""
def write_path_obj(model, out="preview_path.obj"):
    pts = model.interpolate()
    with open(out,"w") as f:
        f.write("o path\n")
        for p in pts:
            x,y,z = p.pos
            f.write("v %.6f %.6f %.6f\n" % (x,y,z))
        f.write("l")
        for i in range(1,len(pts)+1):
            f.write(" %d" % i)
        f.write("\n")
