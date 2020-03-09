import FreeCAD as App

exports = [ #ConstraintSolver copies methods listed here into itself
'show',
'toPartGeom',
'fromPartGeom_G2D',
]


def show(obj, valueset = None):
    """show(obj, valueset = None): displays a constraintsolver object in 3d view"""
    import ConstraintSolver as FCS
    import Part
    if obj.isDerivedFrom('FCS::G2D::ParaGeometry2D'):
        geom = toPartGeom(obj, valueset)
        Part.show(geom.toShape())
    else:
        raise TypeError('type not supported: ' + str(type(obj)))

def toPartGeom(cs_geom, valueset = None, part_geom = None):
    """toPartGeom(cs_geom, valueset = None, part_geom = None): converts Para-geometry to Part geometry.
    cs_geom: ParaGeometry instance (not ParaShape!)
    valueset: optional, ValueSet to use instead of saved values of parameters
    part_geom: ref to existing Part geometry object, it will be updated with the values from para geometry."""
    
    import ConstraintSolver
    if valueset is None:
        if cs_geom.Touched:
            cs_geom.update()
        valueset = cs_geom.Parameters[0].Host.asValueSet()
    
    method_lookup = {
        ConstraintSolver.G2D.ParaPoint         : toPartGeom_G2D_Point        ,
        ConstraintSolver.G2D.ParaLine          : toPartGeom_G2D_Line         ,
        ConstraintSolver.G2D.ParaCircle        : toPartGeom_G2D_Circle       ,
    }
    
    meth = method_lookup[type(cs_geom)]
    return meth(cs_geom, valueset, part_geom)
    
def fromPartGeom_G2D(part_geom, cs_geom = None, valueset = None, store = None):
    import Part
    import ConstraintSolver as FCS

    # ensure store is assigned
    if cs_geom is None and store is None:
        raise ValueError("fromPartGeom: neither a shape object nor parameterstore object have been given. Need at least one.")
    if store is None:
        cs_geom.update()
        store = cs_geom.parameters[0].Host

    # ensure valueset is assigned
    if valueset is not None and cs_geom is None:
        raise ValueError("fromPartGeom: if valueset is provided, cs_geom should not be None, but it is")
    if valueset is None:
        valueset = store.asValueSet()
    
    method_lookup = {
        App.Vector                  : fromPartGeom_G2D_Point        ,
        Part.Point                  : fromPartGeom_G2D_Point        ,
        Part.LineSegment            : fromPartGeom_G2D_Line         ,
        Part.Circle                 : fromPartGeom_G2D_Circle       ,
        Part.ArcOfCircle            : fromPartGeom_G2D_Circle       ,
    }
    
    meth = method_lookup[type(part_geom)]
    return meth(part_geom, cs_geom, valueset, store)

    
def toPartGeom_G2D_Point(cs_geom, valueset, part_geom):
    import Part
    if part_geom is None:
        part_geom = Part.Point()    
    part_geom.X = valueset[cs_geom.x].re
    part_geom.Y = valueset[cs_geom.y].re
    return part_geom

def fromPartGeom_G2D_Point (part_geom, cs_geom, valueset, store):
    import FreeCAD as App
    import ConstraintSolver as FCS
    if cs_geom is None:
        cs_geom = FCS.G2D.ParaPoint(store= store)
    if type(part_geom) is App.Vector:
        cs_geom.setValue(valueset, part_geom)
    else:
        cs_geom.setValue(valueset, FCS.G2D.Vector(part_geom.X, part_geom.Y))
    return cs_geom


def toPartGeom_G2D_Line(cs_geom, valueset, part_geom):
    import Part
    if part_geom is None:
        part_geom = Part.LineSegment()    
    part_geom.StartPoint = cs_geom.p0.value(valueset).re
    part_geom.EndPoint   = cs_geom.p1.value(valueset).re
    return part_geom

def fromPartGeom_G2D_Line (part_geom, cs_geom, valueset, store):
    import ConstraintSolver as FCS
    if cs_geom is None:
        cs_geom = FCS.G2D.ParaLine(store= store)
    cs_geom.p0.setValue(valueset, part_geom.StartPoint)
    cs_geom.p1.setValue(valueset, part_geom.EndPoint)
    return cs_geom


def toPartGeom_G2D_Circle(cs_geom, valueset, part_geom):
    import Part
    if part_geom is None:
        if cs_geom.IsFull:
            part_geom = Part.Circle()
        else:
            part_geom = Part.ArcOfCircle(Part.Circle(), 0, 1)
    part_geom.Location = cs_geom.center.value(valueset).re
    part_geom.Axis = App.Vector(0,0,1)
    part_geom.XAxis = App.Vector(1,0,0)
    if not cs_geom.IsFull:
        part_geom.setParameterRange(valueset[cs_geom.u0].re, valueset[cs_geom.u1].re)
    part_geom.Radius = valueset[cs_geom.radius].re
    return part_geom

def fromPartGeom_G2D_Circle (part_geom, cs_geom, valueset, store):
    import ConstraintSolver as FCS
    import Part
    from math import atan2, floor, tau as turn
    
    is_full = type(part_geom) is Part.Circle #i.e. is not Part.ArcOfCircle
    
    if cs_geom is None:
        cs_geom = FCS.G2D.ParaCircle(is_full, store= store)
    cs_geom.center.setValue(valueset, part_geom.Location)
    valueset[cs_geom.radius] = part_geom.Radius
    
    if not is_full:
        p0 = part_geom.StartPoint
        p1 = part_geom.EndPoint
        
        #compute u0,u1, taking rotation and possible reversal of arc's LCS
        zax = part_geom.Axis
        xax = part_geom.XAxis
        if abs(zax.x) + abs(zax.y) > 2e-12:
            raise ValueError("fromPartGeom_G2D_Circle: circle/arc is not on XY plane")
        orimult = 1.0 if zax.z > 0 else -1.0 #is this arc reversed or not?
        rot = atan2(xax.y, xax.x)
        u0 = part_geom.FirstParameter * orimult
        u1 = part_geom.LastParameter * orimult
        if orimult < 0: # reversed, -> swap endpoints
            u0,u1 = u1,u0
            p0,p1 = p1,p0
        u0 = u0 + rot
        u1 = u1 + rot
        
        #constrain angles to [0..turn) range. 1e-9 reverse-engineered by testing 
        # this is to match behavior of Part.ArcOfCircle, which auto constrains the
        # parameter range to this range.
        unrot = floor((u0 + 1e-9) / turn) * turn 
        valueset[cs_geom.u0] = u0 - unrot
        valueset[cs_geom.u1] = u1 - unrot
        
        cs_geom.p0.setValue(valueset, p0)
        cs_geom.p1.setValue(valueset, p1)
        
    return cs_geom
