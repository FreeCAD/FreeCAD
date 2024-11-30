# Example how to use the basic robot class Robot6Axis which represent a 6-Axis
# industrial robot. The Robot Module is dependent on Part but not on other Modules.
# It works mostly with the basic types Placement, Vector and Matrix. So we need
# only:
from Robot import *
from Part import *
from FreeCAD import *
import FreeCAD as App
import tempfile

# === Basic robot stuff ===
# create the robot. If you not specify a other kinematic it becomes a Puma 560
rob = Robot6Axis()
print(rob)

# accessing the axis and the Tcp. Axis go from 1-6 and are in degrees:
Start = rob.Tcp
print(Start)
print(rob.Axis1)

# move the first Axis of the robot:
rob.Axis1 = 5.0
# the Tcp has changed (forward kinematic)
print(rob.Tcp)

# move the robot back to start position (reverse kinematic):
rob.Tcp = Start
print(rob.Axis1)

# the same with axis 2:
rob.Axis2 = 5.0
print(rob.Tcp)
rob.Tcp = Start
print(rob.Axis2)

# Waypoints:
w = Waypoint(Placement(), name="Pt", type="LIN")
print(w.Name, w.Type, w.Pos, w.Cont, w.Velocity, w.Base, w.Tool)

# generate more. The Trajectory find always outomatically a unique name for the waypoints
l = [w]
for i in range(5):
    l.append(Waypoint(Placement(Vector(0, 0, i * 100), Vector(1, 0, 0), 0), "LIN", "Pt"))

# create a trajectory
t = Trajectory(l)
print(t)
for i in range(7):
    t.insertWaypoints(
        Waypoint(Placement(Vector(0, 0, i * 100 + 500), Vector(1, 0, 0), 0), "LIN", "Pt")
    )

# see a list of all waypoints:
print(t.Waypoints)

del rob, Start, t, l, w

# === working with the document ===
#
# Working with the robot document objects:
# first create a robot in the active document
if App.activeDocument() is None:
    App.newDocument()

App.activeDocument().addObject("Robot::RobotObject", "Robot")
# Define the visual representation and the kinematic definition (see [[6-Axis Robot]] for details about that)
App.activeDocument().Robot.RobotVrmlFile = App.getResourceDir() + "Mod/Robot/Lib/Kuka/kr500_1.wrl"
App.activeDocument().Robot.RobotKinematicFile = (
    App.getResourceDir() + "Mod/Robot/Lib/Kuka/kr500_1.csv"
)
# start position of the Axis (only that which differ from 0)
App.activeDocument().Robot.Axis2 = -90
App.activeDocument().Robot.Axis3 = 90

# retrieve the Tcp position
pos = App.getDocument("Unnamed").getObject("Robot").Tcp
# move the robot
pos.move(App.Vector(-10, 0, 0))
App.getDocument("Unnamed").getObject("Robot").Tcp = pos

# create an empty Trajectory object in the active document
App.activeDocument().addObject("Robot::TrajectoryObject", "Trajectory")
# get the Trajectory
t = App.activeDocument().Trajectory.Trajectory
# add the actual TCP position of the robot to the trajectory
StartTcp = App.activeDocument().Robot.Tcp
t.insertWaypoints(StartTcp)
App.activeDocument().Trajectory.Trajectory = t
print(App.activeDocument().Trajectory.Trajectory)

# insert some more Waypoints and the start point at the end again:
for i in range(7):
    t.insertWaypoints(
        Waypoint(Placement(Vector(0, 1000, i * 100 + 500), Vector(1, 0, 0), i), "LIN", "Pt")
    )

t.insertWaypoints(StartTcp)  # end point of the trajectory
App.activeDocument().Trajectory.Trajectory = t
print(App.activeDocument().Trajectory.Trajectory)

# === Simulation ===
# To be done..... ;-)

# === Exporting the trajectory ===
# the Trajectory is exported by python. That means for every Control Cabinet type is a Post processor
# python module. Here is in detail the Kuka Postprocessor described
from KukaExporter import ExportCompactSub

ExportCompactSub(
    App.activeDocument().Robot,
    App.activeDocument().Trajectory,
    tempfile.gettempdir() + "/TestOut.src",
)

# and that's kind of how its done:
for w in App.activeDocument().Trajectory.Trajectory.Waypoints:
    (A, B, C) = w.Pos.Rotation.toEuler()
    print(
        "LIN {X %.3f,Y %.3f,Z %.3f,A %.3f,B %.3f,C %.3f} ; %s"
        % (w.Pos.Base.x, w.Pos.Base.y, w.Pos.Base.z, A, B, C, w.Name)
    )
