# STATUS:
PROPOSED

# Why it is a priority

Simulation is a key part of the CAM workflow.  It gives the user visual confirmation of the correctness of the generated toolpaths.
Simulation becomes more critical as other parts of the CAM stack increase in complexity

- Tools, toolshapes, toolholders all affect collision avoidance
- Multiaxis movement and Machine kinematics should be reflected in the simulation
- RESTful machining and incremental toolpaths are more complicated to visualize

CAM has both a legacy simulator and a new version based on opengl.  The presence of two simulators is confusing and doubles maintenance

-
# Scope

The scope of this focused on building a solid foundation for simulation to continue to improve organically.
It is not attempting to achieve all desired functionality.


| In  | Out |
| --- | --- |
| Remove legacy simulator    |     |
| Add missing functionality to new simulator    |     |
| Improve user expience | Full machine simulation |
| Improve simulation accuracy | Simulating multi-fixture jobs |
| Simulate Rotary moves | Continuous 5 axis simulation |


# Related Epics
-
