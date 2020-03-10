import ConstraintSolver as CS
ps = CS.ParameterStore()
p1 = CS.G2D.ParaPoint(store= ps)
p2 = CS.G2D.ParaPoint(store= ps)
p2.x.Value = 3
p2.y.Value = 4

c = CS.G2D.ConstraintDistance(
    p1 = p1,
    p2 = p2,
    store = ps,
    Label = "Constraint1"
)
c.dist.Value = 3

c.update()
c.NetError

sys = CS.SubSystem()
sys.addUnknown(p1.Parameters)
sys.addUnknown(p2.Parameters)
sys.addConstraint(c)

vs = CS.ValueSet(CS.ParameterSubset(ps.allFree()))
slv = CS.SolverBackend("FCS::LM")
slv.solve(sys, vs)
