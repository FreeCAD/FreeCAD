import FreeCAD as App

exports = [ #ConstraintSolver copies methods listed here into itself
'toPartGeom',
'fromPartGeom',
]

def toPartGeom(cs_geom, valueset = None, part_geom = None):
    """toPartGeom(cs_geom, valueset = None, part_geom = None): converts Para-geometry to Part geometry.
    cs_geom: ParaGeometry instance (not ParaShape!)
    valueset: optional, ValueSet to use instead of saved values of parameters
    part_geom: ref to existing Part geometry object, it will be updated with the values from para geometry."""
    
    import ConstraintSolver
    if valueset is None:
        if cs_geom.Touched:
            cs_geom.update()
        valueset = ConstraintSolver.ValueSet(cs_geom.Parameters[0].Host)
    
    method_lookup = {
        ConstraintSolver.G2D.ParaPoint         : toPartGeom_G2D_Point        ,
        ConstraintSolver.G2D.ParaLine          : toPartGeom_G2D_Line         ,
        ConstraintSolver.G2D.ParaCircle        : toPartGeom_G2D_Circle       ,
    }
    
    meth = meth_lookup[type(cs_geom)]
    return meth(cs_geom, valueset, part_geom)
    
def toPartGeom_G2D_Point(cs_shape, valueset, part_geom):
    import Part
    if part_geom is None:
        part_geom = Part.Point()    
    part_geom.X = valueset[cs_shape.x]
    part_geom.Y = valueset[cs_shape.y]
    return part_geom

def toPartGeom_G2D_Line(cs_shape, valueset, part_geom):
    import Part
    if part_geom is None:
        part_geom = Part.LineSegment()    
    part_geom.StartPoint = App.Vector(valueset[cs_shape.p0.x], valueset[cs_shape.p0.y], 0)
    part_geom.EndPoint   = App.Vector(valueset[cs_shape.p1.x], valueset[cs_shape.p1.y], 0)
    return part_geom

def toPartGeom_G2D_Circle(cs_shape, valueset, part_geom):
    import Part
    if part_geom is None:
        if cs_shape.IsFull:
            part_geom = Part.Circle()
        else:
            part_geom = Part.ArcOfCircle(Part.Circle, 0, 1)
    part_geom.Location = App.Vector(valueset[cs_shape.center.x], valueset[cs_shape.center.y], 0)
    part_geom.Axis = App.Vector(0,0,1)
    part_geom.XAxis = App.Vector(1,0,0)
    Part_geom.setParameterRange(valueset[cs_shape.u0], valueset[cs_shape.u1])
    return part_geom

def fromPartGeom(part_geom, cs_shape = None, store = None):
    pass
