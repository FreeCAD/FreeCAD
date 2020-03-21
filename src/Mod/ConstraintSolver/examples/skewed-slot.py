# load skewed-slot-preset.FCStd, then paste this to console

import ConstraintSolver as FCS
ps = FCS.ParameterStore()

part_geoms = App.ActiveDocument.Sketch.Geometry

#convert Part geometry to FCS geometry
solver_geoms = [FCS.fromPartGeom_G2D(part_geom, store= ps) for part_geom in part_geoms]

arc1, line1, line2, arc2 = solver_geoms

# make constraints
constraints = []

for it in solver_geoms:
    constraints.extend(it.makeRuleConstraints())

# fixed zero angle parameter for making tangent constraints
zero = ps.addOne("zeroangle", fixed= True)

def connectTangent(edge1, edge2, label):
    return [
        FCS.G2D.ConstraintPointCoincident(
            p1= edge1.p1, p2= edge2.p0,
            Label= "Coincident_" + label
        ), 
        FCS.G2D.ConstraintAngleAtXY(
            p= edge1.p1,
            crv1= edge1, crv2= edge2,
            angle= zero,
            Label= "Tangent_" + label
        ),             
    ]

constraints.extend(
    connectTangent(
        arc1, line1,
        label= "1"
    )
)
constraints.extend(
    connectTangent(
        line1, arc2,
        label= "2"
    )
)
constraints.extend(
    connectTangent(
        arc2, line2,
        label= "3"
    )
)
constraints.extend(
    connectTangent(
        line2, arc1,
        label= "4"
    )
)

#print out constraint errors
for c in constraints:
    c.update()
    print(c.NetError)

# build system to solve
sys = FCS.SubSystem()
sys.addUnknown(ps.allFree())
sys.addConstraint(constraints)

# solve
vs = FCS.ValueSet(FCS.ParameterSubset(ps.allFree()))
slv = FCS.SolverBackend("FCS::LM")
slv.solve(sys, vs)

# show the result
for g in solver_geoms:
    FCS.show(g, vs)

