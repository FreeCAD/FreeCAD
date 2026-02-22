#!/usr/bin/python3
# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

import os, sys, string
import FreeCAD, FreeCADGui, Robot, RobotGui

x = 1920
y = 1080
Background = "White"

OutDir = "c:/temp/Movie/"

Trajectory = None
Robot = None


def run():
    Tool = Robot.Tool
    Tool = Tool.inverse()
    # duration in seconds time the pictures per second gives the size
    size = int(Trajectory.Duration * 24.0)
    for l in range(size):
        Robot.Tcp = Trajectory.position(l / 24.0).multiply(Tool)
        FreeCADGui.updateGui()
        FreeCADGui.ActiveDocument.ActiveView.saveImage(OutDir + "Rob_" + l + ".jpg", x, y, "White")
