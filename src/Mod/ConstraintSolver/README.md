# ConstraintSolver

## overall concept

The goal of ConstraintSolver is to numerically solve geometric problems, such as sketches and assemblies.

Solver itself is a rather abstract tool, that has no idea what geometry even is. For solver, the problem is defined as:

* an array of real numbers (called "*parameters*")
* a list of constraints ("*solver constraint*")

A solver constraint is just a function of any number of parameters, called "*error function*". 
Solver's job is to make all error functions of constraints to equal zero, by changing parameters.

Most of the code of ConstraintSolver is actually about keeping track of parameters, and defining the error functions, 
by means of creating geometry objects and constraint objects.

Solvers also benefit greatly from the ability to compute partial derivatives of error functions by any parameter. 
This is also one of the big jobs of ConstraintSolver code, and it's done by implementing most computaions using *dual numbers*.
This is the main reason for having to reimplement geometry math, instead of just using existing code from, say, OCCT.

## ParameterStore class

Its job is to contain all parameters that geometries and constraints refer to. It is just a list of real numbers, essentially, 
but it also keeps track of which ones are fixed, and contains scale information, along with some other info.

### Scaling of parameters

Solvers work best when all parameters are of the same order of magnitude. Since parameters can have a range of meanings, 
from coordinates to fractions to angles, the range of values can be enormous. This can lead to solver attempting to wildly 
change angles while being reluctant to changing coordinates, if coordinates are of large values. The outcome is tendency to 
flipping, slow convergence, failing to solve, and poor relative accuracy of solution on some parameters.

To improve the situations, parameters have Scale attribute. The stored value of parameter is of original magnitude. But for the solver, 
the values are scaled by dividing the value by Scale. It's up to the user to provide correct values for Scale for best solver performance.

Same applies to error functions: it's best if solver receives a balanced set of magnitudes. This is assisted by having a 
scaling factor in constraints.

### adding parameters

Parameters can be directly added to the store, and then used to build geometry, like so:

```
import ConstraintSolver
ps = ConstraintSolver.ParameterStore()
px = ps.addOne("x", 0.0)
py = ps.addOne("y", 0.0)
pnt = ConstraintSolver.G2D.ParaPoint(
    x = px,
    y = py
)
```

or they can be created automatically using geometry's `makeParameters` method:

```
import ConstraintSolver
ps = ConstraintSolver.ParameterStore()
pnt = ConstraintSolver.G2D.ParaPoint()
px, py = pnt.makeParameters(ps)
```

The first method is more messy, but it provides the ability to share parameters between geometries. For example, to define a point lying on x=y line, you can:

```
import ConstraintSolver
ps = ConstraintSolver.ParameterStore()
px = ps.addOne("x", 0.0)
pnt = ConstraintSolver.G2D.ParaPoint(
    x = px,
    y = px
)
```

The second method is somewhat less messy, but it may create a lot of unnecessary parameters. 
Both can be combined by explicitly assigning some parameters of the geometry, before using `makeParameters`. 
Then, only the parameters that weren't preassigned will be created automatically.
Also, `makeParameters` automatically gives labels to parameters. Labels are not needed, but 
they can be very useful when debugging.

makeParameters can also be called implicitly, by providing a `store` keyword argument to geometry constructors:

```
import ConstraintSolver
ps = ConstraintSolver.ParameterStore()
line = ConstraintSolver.G2D.ParaLine(
    Label = "my cute little line",
    store = ps
)
print(line.p0.x) # prints <ParameterRef [2] 'my cute little line.p0.x'>
```

### free and fixed parameters

By default, most parameters are created to be free, i.e. to be varied by solver to arrive to a solution. 
They can be marked as fixed by calling `fix` method of ParameterRef.
This mark is there mostly as a convenience to pick the unknowns for solver. 
But be aware, solver itself doesn't care. 
If a fixed parameter is added to a subsystem as an unknown, solver will vary it just like any other parameter. 

Fixedness is also used by `constrainEqual` method of ParameterStore. 
It is also expected to be used by solver front-end, when one is implemented. 

The exact meaning of fixedness is not yet fully decided, time will tell.

### parameter redirection

ParameterStore has constrainEqual method. It makes one of the parameters point to the value of the other, 
effectively making them shared.

\#UNDER CONSTRUCTION

### ParameterRef object

ParameterRef object is the thing used by geometries and constraints to refer to parameters. 
ParameterRef is just a glorified index of the parameter in the store (in addition to the index, 
it knows the store it is referring to, has a bunch of methods to manipulate the referred parameter).

ParameterRefs are often used as an index to read out a value. For example, given a ValueSet object, 
one can retrieve the value of a parameter overridden by the set, with `[]` operator.

## ValueSet object

In order to calculate the errors of constraints at intermediate steps, solver needs to update parameter values somehow. 
Writing directly to ParameterStore is undersirable for two reasons: a) if solver fails hard, it can mess up a good initial state,
b) launching several solvers in separate threads is impossible, as they will fight each other.

ValueSet addresses that problem by holding override values for a subset of parameters. Solver operates on a valueset, 
never touching the saved values of parameters. 

The solution the solver arrives to is thus provided as the modification to ValueSet object you provided when constructing it.
You can write the solution down into the parameters by calling `apply` method of the ValueSet object.

For that reason, all value-querying methods of geometries take ValueSet object as their first argument. 
The argument os never optional, to ensure one doesn't forget to put it in.

A value of a specific parameter can be read from a valueset using subscript operator: `x_of_my_point = a_valueset_object[pnt.x]`

ValueSet also is the source of dual parts of dual numbers all the calculations are done with. Setting these dual parts allows 
to calculate either a derivative by one parameter, or along a vector in parameter space. 

ValueSet caches parameter scale factors. So if you change scale factors of your parameters, ValueSet objects must be reconstructed to attain these new scales.

## Dual numbers

For an introduction on how calculating everything in dual numbers can provide automatic derivatives, see, for example:
https://mitmath.github.io/18337/lecture9/autodiff_dimensions
(thanks @sasobadovinac for this link)

\#UNDER CONSTRUCTION

## Geometries

There are two kinds of classes regarding geometry: value versions, and `Para`-versions. For example, there is `Vector`, and there is `ParaVector`.

`Vector` has just dual numbers as its coordinates. It is there to store as an intermediate result of a calculation. 
`ParaVector` has ParameterRef objects as its coordinates. Its job is to keep track of solver parameters, and fetch them to a `Vector` object.

Not all geometries have two versions, only those that need to be (temporarily) created for intermediate computations of constraint math honour the existence of non-para version.

Geometries and constraints are divided into two groups: 2D geometry (on plane), found in `G2D` submodule, and 3d geometry (none implemented yet), to be found in `G3D` submodule.

\#UNDER CONSTRUCTION

## Placements and Shapes

\#UNDER CONSTRUCTION

## Constraints

The main job of a constraint is to provide an error function. 

There are two base types of constraint objects:

* SimpleConstraint
* Constraint

The only difference between them is in how many degrees of freedom they take out. 
SimpleConstraint provides just one error function (returns a DualNumber), while Constraint can provide many.

The number of DOFs removed, which is also the number of error functions the constraint provides, is called "rank" (well, DeepSOIC called it that way, as he failed to find/invent a better name). 
That is, SimpleConstraint has a rank of 1, and Constraint can have arbitrary rank.

Rank of a constraint must never be changed in the process of solving. But the changing rank can potentially be emulated by returning plan zeros in some of the error functions; this is unexplored territory.


\#UNDER CONSTRUCTION

## Solvers

### SubSystem

SubSystem is a subset of constraints, isolated from others (usually because they can be solved independently).

### Solver back-ends

They are the actual solver algorithms that zero out the error function. 
A solver back-end takes SubSystem and ValueSet as inputs, and modifies the provided ValueSet in the process of solving, to ultimately contain the solution.
If the solver fails, the ValueSet will contain the values it arrived to in the process of failing.

Back-ends can be used without setting up a front-end. 

The general sequence of using a back-end to solve a geometric problem is like this:

1. set up the problem.
```
import ConstraintSolver as CS

# set up the problem.
ps = CS.ParameterStore()
p1 = CS.G2D.ParaPoint(ps)
p2 = CS.G2D.ParaPoint(ps)
p2.x.Value = 3
p2.y.Value = 4

c = CS.G2D.ConstraintDistance(
    p1 = p1,
    p2 = p2,
    store = ps,
    Label = "Constraint1"
)
c.dist.Value = 3
```

2. update the geometry and constraints. This is needed for constraints to prepare lists of parameters they depend on; these lists are used to optimize gradient and jacobi matrix computations, and thus are cached, not generated on demand.
```
c.update()
```

3. set up a SubSystem
```
sys = CS.SubSystem()
sys.addUnknown(p1.Parameters)
sys.addUnknown(p2.Parameters)
sys.addConstraint(c)
```

4. use the back-end
```
vs = CS.ValueSet(CS.ParameterSubset(ps.allFree()))
slv = CS.SolverBackend("FCS::LM")
slv.solve(sys, vs) #returns "Success", "Minimized", or throws an error if fails.
```

5. (optional) apply the solution, and use it
```
vs.apply()
# PROFIT!
```

### solver front-end

\#UNDER CONSTRUCTION

Intended jobs of solver front-end are:
* split a problem it into subsystems, 
* solve the subsystems using appropriate back-ends, and provide the complete solution
* identify degrees of freedom, redundant and conflicting constraints.


## Object lifetime

### Python

All ConstraintSolver object's lifetime is controlled by python's reference counting mechanism. 
It is not governed by python's cycle detection, so linking cycles can lead to memory leaks.

It might be tempting, for example, to make a geometric object know its rule constraints. 
One can add that by using `UserData` field (that most ConstraintSolver objects have), however that will make the pair immortal. 
Because the constraint references the geometry, and making geometry reference the constraint will keep their refcounts positive.
So if you do that, you must break these links manually after you're done.
Weakrefs to ConstraintSolver objects are not yet supported, and it's somewhat unlikely they will be in the future.

### C++

In C++, object lifetime is also managed by python reference counting. 
It is made convenient by means of a PyHandle object, which holds a python reference to the object, and acts as a pointer to C++ object.
Thanks to this, exposing the objects to python is extremely straightforward.

See documentation of Base::PyHandle on how to use it (in src/Base/PyHandle.h).