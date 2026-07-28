# SPDX-License-Identifier: LGPL-2.1-or-later

"""Create a small MbDFEM document for interactive testing."""

import FreeCAD as App
import MbDFEM  # noqa: F401


def create_model():
    """Create and return the example MbDFEM document."""
    document_name = "MbDFEM_Doc"
    if document_name in App.listDocuments():
        App.closeDocument(document_name)

    document = App.newDocument(document_name)

    assembly = document.addObject("MbDFEM::MbDAssembly", "MbDAssembly1")
    assembly.Placement.Base = App.Vector(0, 0, 0)

    subassembly = document.addObject("MbDFEM::MbDAssembly", "MbDAssembly2")
    subassembly.Placement.Base = App.Vector(50, 0, 0)

    assembly_markers = [
        document.addObject("MbDFEM::MbDMarker", "MbDMarker1"),
        document.addObject("MbDFEM::MbDMarker", "MbDMarker2"),
    ]
    assembly_markers[0].Placement.Base = App.Vector(10, 0, 0)
    assembly_markers[1].Placement.Base = App.Vector(20, 0, 0)

    parts = [
        document.addObject("MbDFEM::MbDPart", "MbDPart1"),
        document.addObject("MbDFEM::MbDPart", "MbDPart2"),
    ]
    parts[0].Placement.Base = App.Vector(100, 0, 0)
    parts[1].Placement.Base = App.Vector(200, 0, 0)

    joints = [
        document.addObject("MbDFEM::MbDJoint", "MbDJoint1"),
        document.addObject("MbDFEM::MbDJoint", "MbDJoint2"),
    ]
    motions = [
        document.addObject("MbDFEM::MbDMotion", "MbDMotion1"),
        document.addObject("MbDFEM::MbDMotion", "MbDMotion2"),
    ]
    actions = [
        document.addObject("MbDFEM::MbDAction", "MbDAction1"),
        document.addObject("MbDFEM::MbDAction", "MbDAction2"),
    ]

    part_markers = [
        [
            document.addObject("MbDFEM::MbDMarker", "MbDMarker11"),
            document.addObject("MbDFEM::MbDMarker", "MbDMarker12"),
        ],
        [
            document.addObject("MbDFEM::MbDMarker", "MbDMarker21"),
            document.addObject("MbDFEM::MbDMarker", "MbDMarker22"),
        ],
    ]

    part_markers[0][0].Placement.Base = App.Vector(110, 10, 0)
    part_markers[0][1].Placement.Base = App.Vector(120, 10, 0)
    part_markers[1][0].Placement.Base = App.Vector(210, 10, 0)
    part_markers[1][1].Placement.Base = App.Vector(220, 10, 0)

    for marker in assembly_markers:
        assembly.addMarker(marker)

    assembly.addAssembly(subassembly)

    for part, markers in zip(parts, part_markers):
        assembly.addPart(part)
        for marker in markers:
            part.addMarker(marker)

    for joint in joints:
        assembly.addJoint(joint)

    for motion in motions:
        assembly.addMotion(motion)

    for action in actions:
        assembly.addAction(action)

    joints[0].setMarkers(assembly_markers[0], part_markers[0][0])
    joints[1].setMarkers(part_markers[0][1], part_markers[1][0])
    motions[0].setMarkers(assembly_markers[0], part_markers[0][1])
    motions[1].setMarkers(assembly_markers[1], part_markers[1][1])
    actions[0].setMarkers(assembly_markers[0], part_markers[1][0])
    actions[1].setMarkers(assembly_markers[1], part_markers[1][1])

    document.recompute()
    return document


document = create_model()
App.Console.PrintMessage("Created MbDFEM example document: MbDFEM_Doc\n")
