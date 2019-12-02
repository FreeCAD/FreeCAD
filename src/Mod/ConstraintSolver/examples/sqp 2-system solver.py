import ConstraintSolver as CS
ps = CS.ParameterStore()
p1 = CS.G2D.ParaPoint(ps)
p2 = CS.G2D.ParaPoint(ps)
p2.x.Value = 3
p2.y.Value = 4
p3 = CS.G2D.ParaPoint(ps)
p3.x.Value = -1
p3.y.Value = 0
p3.x.fix()
p3.y.fix()

c = CS.G2D.ConstraintDistance(
    p1 = p1,
    p2 = p2,
    store = ps,
    Label = "Constraint1"
)
c.dist.Value = 3
c.dist.fix()

c2 = CS.G2D.ConstraintDistance(
    p1 = p1,
    p2 = p3,
    store = ps,
    Label = "Constraint2"
)
c2.dist.Value = 1
c2.dist.fix()

c.update()
c2.update()

sys = CS.SubSystem()
sys.addUnknown(p1.Parameters)
sys.addUnknown(p2.Parameters)
sys.addConstraint(c)

sysaux = CS.SubSystem()
sysaux.addUnknown(sys.ParameterSet)
sysaux.addConstraint(c2)

vs = CS.ValueSet(CS.ParameterSubset(ps.allFree()))
slv = CS.SketchSolver()
slv.solveSQP(sys, sysaux, vs)

for p in [p1,p2,p3]:
    print(vs[p.x], vs[p.y])
