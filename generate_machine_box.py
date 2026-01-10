# -*- coding: utf-8 -*-
"""
FreeCAD Macro: Generate Machine Boundary Box

This macro creates a wireframe box representing the working envelope
of a CNC machine based on its configuration.

COORDINATE SYSTEM:
- Uses MACHINE coordinates (absolute travel limits of the machine)
- Not work coordinates (relative to workpiece)
- Shows the full extent the machine can move in X, Y, Z directions

Author: Generated for FreeCAD CAM
"""

import FreeCAD
import Part
import Path
from Path.Machine.models.machine import MachineFactory
import os

def get_machine_file():
    """Prompt user to select a machine configuration file."""
    # Get available machine files
    machines = MachineFactory.list_configuration_files()
    machine_names = [name for name, path in machines if path is not None]

    if not machine_names:
        FreeCAD.Console.PrintError("No machine configuration files found.\n")
        return None

    # For now, use the first machine. In a real macro, you'd use a dialog
    # to let the user choose
    selected_name = machine_names[0]  # Default to first
    selected_path = None
    for name, path in machines:
        if name == selected_name and path:
            selected_path = path
            break

    if not selected_path:
        FreeCAD.Console.PrintError("Could not find selected machine file.\n")
        return None

    return selected_path

def create_machine_boundary_box(machine_file, color=(1.0, 0.0, 0.0), line_width=2.0, draw_style="Dashed"):
    """Create a wireframe box showing machine boundaries.

    Args:
        machine_file: Path to the machine configuration file
        color: RGB tuple for wire color (default: red)
        line_width: Width of the wires (default: 2.0)
        draw_style: "Solid", "Dashed", or "Dotted" (default: "Dashed")
    """

    try:
        # Load the machine configuration
        machine = MachineFactory.load_configuration(machine_file)
        FreeCAD.Console.PrintMessage(f"Loaded machine: {machine.name}\n")

        # Get axis limits
        x_min = y_min = z_min = float('inf')
        x_max = y_max = z_max = float('-inf')

        # Find min/max for linear axes
        for axis_name, axis_obj in machine.linear_axes.items():
            if axis_name.upper() == 'X':
                x_min = min(x_min, axis_obj.min_limit)
                x_max = max(x_max, axis_obj.max_limit)
            elif axis_name.upper() == 'Y':
                y_min = min(y_min, axis_obj.min_limit)
                y_max = max(y_max, axis_obj.max_limit)
            elif axis_name.upper() == 'Z':
                z_min = min(z_min, axis_obj.min_limit)
                z_max = max(z_max, axis_obj.max_limit)

        # Check if we have valid limits
        if x_min == float('inf') or y_min == float('inf') or z_min == float('inf'):
            FreeCAD.Console.PrintError("Machine does not have X, Y, Z linear axes defined.\n")
            return None

        FreeCAD.Console.PrintMessage(f"Machine boundaries: X({x_min:.3f}, {x_max:.3f}), Y({y_min:.3f}, {y_max:.3f}), Z({z_min:.3f}, {z_max:.3f})\n")
        FreeCAD.Console.PrintMessage("Note: These are MACHINE coordinates showing the absolute travel limits.\n")
        FreeCAD.Console.PrintMessage("Work coordinates would be relative to the workpiece origin.\n")

        # Create the 8 corner points of the box
        p1 = FreeCAD.Vector(x_min, y_min, z_min)
        p2 = FreeCAD.Vector(x_max, y_min, z_min)
        p3 = FreeCAD.Vector(x_max, y_max, z_min)
        p4 = FreeCAD.Vector(x_min, y_max, z_min)
        p5 = FreeCAD.Vector(x_min, y_min, z_max)
        p6 = FreeCAD.Vector(x_max, y_min, z_max)
        p7 = FreeCAD.Vector(x_max, y_max, z_max)
        p8 = FreeCAD.Vector(x_min, y_max, z_max)

        # Create edges (12 edges for wireframe box)
        edges = [
            Part.makeLine(p1, p2),  # bottom face
            Part.makeLine(p2, p3),
            Part.makeLine(p3, p4),
            Part.makeLine(p4, p1),
            Part.makeLine(p5, p6),  # top face
            Part.makeLine(p6, p7),
            Part.makeLine(p7, p8),
            Part.makeLine(p8, p5),
            Part.makeLine(p1, p5),  # vertical edges
            Part.makeLine(p2, p6),
            Part.makeLine(p3, p7),
            Part.makeLine(p4, p8),
        ]

        # Create a compound of all edges (wireframe)
        compound = Part.makeCompound(edges)

        # Create a new document if none exists
        if not FreeCAD.ActiveDocument:
            FreeCAD.newDocument("MachineBoundary")

        # Create the shape in the document
        obj = FreeCAD.ActiveDocument.addObject("Part::Feature", f"MachineBoundary_{machine.name.replace(' ', '_')}")
        obj.Shape = compound
        obj.Label = f"Machine Boundary: {machine.name}"

        # Set visual properties
        obj.ViewObject.ShapeColor = color
        obj.ViewObject.LineWidth = line_width
        obj.ViewObject.DrawStyle = draw_style

        FreeCAD.ActiveDocument.recompute()

        FreeCAD.Console.PrintMessage(f"Created machine boundary box for {machine.name}\n")
        return obj

    except Exception as e:
        FreeCAD.Console.PrintError(f"Error creating machine boundary box: {str(e)}\n")
        return None

def main():
    """Main macro function."""
    FreeCAD.Console.PrintMessage("FreeCAD Macro: Generate Machine Boundary Box\n")

    # Get machine file
    machine_file = get_machine_file()
    if not machine_file:
        return

    # Create the boundary box with customizable appearance
    # You can change these parameters:
    # color: (R, G, B) tuple, e.g., (1.0, 0.0, 0.0) for red, (0.0, 1.0, 0.0) for green
    # line_width: thickness of the wires
    # draw_style: "Solid", "Dashed", or "Dotted"
    obj = create_machine_boundary_box(machine_file,
                                    color=(1.0, 0.0, 0.0),  # Red
                                    line_width=2.0,
                                    draw_style="Dashed")  # Broken/dashed lines
    if obj:
        FreeCAD.Console.PrintMessage("Macro completed successfully.\n")
    else:
        FreeCAD.Console.PrintError("Macro failed.\n")

# Run the macro
if __name__ == "__main__":
    main()