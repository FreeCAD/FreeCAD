import FreeCAD
def shapedict(shapelst):
    return dict([(shape.hashCode(),shape) for shape in shapelst])

def shapeset(shapelst):
    return set([shape.hashCode() for shape in shapelst])

def mostbasiccompound(comp):
    '''searches fo the most basic shape in a Compound'''
    solids=shapeset(comp.Solids)
    shells=shapeset(comp.Shells)
    faces=shapeset(comp.Faces)
    wires=shapeset(comp.Wires)
    edges=shapeset(comp.Edges)
    vertexes=shapeset(comp.Vertexes)
    #FreeCAD.Console.PrintMessage('%s\n' % (str((len(solids),len(shells),len(faces),len(wires),len(edges),len(vertexes)))))
    for shape in comp.Solids:
        shells -= shapeset(shape.Shells)
        faces -= shapeset(shape.Faces)
        wires -= shapeset(shape.Wires)
        edges -= shapeset(shape.Edges)
        vertexes -= shapeset(shape.Vertexes)
    for shape in comp.Shells:
        faces -= shapeset(shape.Faces)
        wires -= shapeset(shape.Wires)
        edges -= shapeset(shape.Edges)
        vertexes -= shapeset(shape.Vertexes)
    for shape in comp.Faces:
        wires -= shapeset(shape.Wires)
        edges -= shapeset(shape.Edges)
        vertexes -= shapeset(shape.Vertexes)
    for shape in comp.Wires:
        edges -= shapeset(shape.Edges)
        vertexes -= shapeset(shape.Vertexes)
    for shape in comp.Edges:
        vertexes -= shapeset(shape.Vertexes)
    #FreeCAD.Console.PrintMessage('%s\n' % (str((len(solids),len(shells),len(faces),len(wires),len(edges),len(vertexes)))))
    #return len(solids),len(shells),len(faces),len(wires),len(edges),len(vertexes)
    if vertexes:
        return "Vertex"
    elif edges:
        return "Edge"
    elif wires:
        return "Wire"
    elif faces:
        return "Face"
    elif shells:
        return "Shell"
    elif solids:
        return "Solid"

def colorcodeshapes(objs):
    shapecolors={
        "Compound":(0.3,0.3,0.4),
        "CompSolid":(0.1,0.5,0.0),
        "Solid":(0.0,0.8,0.0),
        "Shell":(0.8,0.0,0.0),
        "Face":(0.6,0.6,0.0),
        "Wire":(0.1,0.1,0.1),
        "Edge":(1.0,1.0,1.0),
        "Vertex":(8.0,8.0,8.0),
        "Shape":(0.0,0.0,1.0),
        None:(0.0,0.0,0.0)}

    for obj in objs:
        if hasattr(obj,'Shape'):
            try:
                if obj.Shape.isNull():
                    continue
                if not obj.Shape.isValid():
                        color=(1.0,0.4,0.4)
                else:
                    st=obj.Shape.ShapeType
                    if st in ["Compound","CompSolid"]:
                        st = mostbasiccompound(obj.Shape)
                    color=shapecolors[st]
                obj.ViewObject.ShapeColor = color
            except:
                raise

#colorcodeshapes(App.ActiveDocument.Objects)
