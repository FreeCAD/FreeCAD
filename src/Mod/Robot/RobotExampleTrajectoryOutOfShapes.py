# Examples to generate trajectories out of shapes
import FreeCADGui as Gui

# getting selected edges from the selection and sort them
count = 0
FirstPos1 = None
FirstPos2 = None
LastPos2 = None
SortedEdgeList = []
for so in Gui.Selection.getSelectionEx():
    for edge in obj.SubObjects:
        if edge.Type != "Part::TopoShape":
            continue
        pos1 = edge.valueAt(0)
        pos2 = edge.valueAt(edge.Length)
        print(pos1, pos2)
        if count == 0:  # first edge
            FirstPos1 = pos1
            FirstPos2 = pos2
        elif count == 1:  # second edge
            continue
        else:  # the rest
            SortedEdgeList.append((pos1, pos2, edge))
