# ***************************************************************************
# *   Copyright (c) 2010 - Juergen Riegel <juergen.riegel@web.de>           *
# *   Copyright (c) 2018 - Bernd Hahnebach <bernd@bimstatik.org>            *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Lesser General Public License for more details.                   *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# *   Juergen Riegel 2002                                                   *
# ***************************************************************************/


# Make mesh of pn junction in TetGen format
import FreeCAD
import FreeCADGui
# import Part
import Mesh
App = FreeCAD  # shortcut
Gui = FreeCADGui  # shortcut

## \addtogroup FEM
#  @{


def exportMeshToTetGenPoly(meshToExport, filePath, beVerbose=1):
    ## Part 1 - write node list to output file
    if beVerbose == 1:
            FreeCAD.Console.PrintMessage("\nExport of mesh to TetGen file ...")
    (allVertices, allFacets) = meshToExport.Topology
    f = open(filePath, 'w')
    f.write("# This file was generated from FreeCAD geometry\n")
    f.write("# Part 1 - node list\n")
    f.write("%(TotalNumOfPoints)i  %(NumOfDimensions)i  %(NumOfProperties)i  %(BoundaryMarkerExists)i\n" % {
        'TotalNumOfPoints': len(allVertices),
        'NumOfDimensions': 3,
        'NumOfProperties': 0,
        'BoundaryMarkerExists': 0
    })
    for PointIndex in range(len(allVertices)):
        f.write("%(PointIndex)5i %(x) e %(y) e %(z) e\n" % {
            'PointIndex': PointIndex,
            'x': allVertices[PointIndex].x,
            'y': allVertices[PointIndex].y,
            'z': allVertices[PointIndex].z
        })

    ## Find out BoundaryMarker for each facet. If edge connects only two facets,
    # then this facets should have the same BoundaryMarker
    BoundaryMarkerExists = 1
    PointList = [allFacets[0][1], allFacets[0][0]]
    PointList.sort()
    EdgeFacets = {(PointList[0], PointList[1]): set([0])}
    Edge = []

    # Finde all facets for each edge
    for FacetIndex in range(len(allFacets)):
        Facet = allFacets[FacetIndex]
        for i in range(0, -len(Facet), -1):
            tmpEdge = [Facet[i], Facet[i + 1]]
            tmpEdge.sort()
            Edge.append(tmpEdge)
        for i in range(len(Edge)):
            EdgeIndex = (Edge[i][0], Edge[i][1])
            if EdgeIndex in EdgeFacets:
                EdgeFacets[EdgeIndex].add(FacetIndex)
            else:
                EdgeFacets[EdgeIndex] = set([FacetIndex])
        Edge = []

    # Find BoundaryMarker for each facet
    BoundaryMarker = []
    for index in range(len(allFacets)):
        BoundaryMarker.append(0)
    MinMarker = -1
    InitialFacet = 0
    BoundaryMarker[InitialFacet] = MinMarker
    EdgeKeys = EdgeFacets.keys()
    # disconnectedEdges = len(EdgeKeys)
    if beVerbose == 1:
        FreeCAD.Console.PrintMessage('\nBoundaryMarker:' + repr(BoundaryMarker) + ' ' + repr(len(EdgeFacets)))
    searchForPair = 1

    # Main loop: first search for all complementary facets, then fill one branch and repeat while edges are available
    while len(EdgeFacets) > 0:
        removeEdge = 0
        for EdgeIndex in EdgeKeys:
            if len(EdgeFacets[EdgeIndex]) == 1:
                removeEdge = 1
                break
            if len(EdgeFacets[EdgeIndex]) == 2:
                FacetPair = []
                for facet in EdgeFacets[EdgeIndex]:
                    FacetPair.append(facet)
                if (BoundaryMarker[FacetPair[0]] == 0) and (BoundaryMarker[FacetPair[1]] == 0):
                    continue
                if (BoundaryMarker[FacetPair[0]] != 0) and (BoundaryMarker[FacetPair[1]] != 0):
                    removeEdge = 1
                    break
                if (BoundaryMarker[FacetPair[0]] != 0):
                    BoundaryMarker[FacetPair[1]] = BoundaryMarker[FacetPair[0]]
                else:
                    BoundaryMarker[FacetPair[0]] = BoundaryMarker[FacetPair[1]]
                removeEdge = 1
                break
            if searchForPair == 1:
                continue
            FacetTree = []
            # AllMarkers = 1
            MarkerSum = 0
            for facet in EdgeFacets[EdgeIndex]:
                FacetTree.append(facet)
                MarkerSum += BoundaryMarker[facet]
            if MarkerSum == 0:
                continue
            for facet in EdgeFacets[EdgeIndex]:
                if BoundaryMarker[facet] == 0:
                    MinMarker -= 1
                    BoundaryMarker[facet] = MinMarker
                    searchForPair = 1
            removeEdge = 1
            break
        if removeEdge == 1:
            del EdgeFacets[EdgeIndex]
            EdgeKeys = EdgeFacets.keys()
            continue
        searchForPair = 0
    # End of main loop
    if beVerbose == 1:
        FreeCAD.Console.PrintMessage('\nNew BoundaryMarker:' + repr(BoundaryMarker) + ' ' + repr(len(EdgeFacets)))

    ## Part 2 - write all facets to *.poly file
    f.write("# Part 2 - facet list\n")
    f.write("%(TotalNumOfFacets)i  %(BoundaryMarkerExists)i\n" % {
        'TotalNumOfFacets': len(allFacets),
        'BoundaryMarkerExists': BoundaryMarkerExists
    })
    for FacetIndex in range(len(allFacets)):
        f.write("# FacetIndex = %(Index)i\n" % {'Index': FacetIndex})
        f.write("%(NumOfPolygons)3i " % {'NumOfPolygons': 1})
        if BoundaryMarkerExists == 1:
            f.write("0 %(BoundaryMarker)i" % {'BoundaryMarker': BoundaryMarker[FacetIndex]})
        f.write("\n%(NumOfConers)3i  " % {'NumOfConers': len(allFacets[FacetIndex])})
        for PointIndex in range(len(allFacets[FacetIndex])):
            #        f.write(repr(allFacets[FacetIndex][PointIndex]))
            f.write("%(PointIndex)i " % {'PointIndex': allFacets[FacetIndex][PointIndex]})
        f.write("\n")
    ## Part 3 and Part 4 are zero
    f.write("# Part 3 - the hole list.\n# There is no hole in bar.\n0\n")
    f.write("# Part 4 - the region list.\n# There is no region defined.\n0\n")
    f.write("# This file was generated from FreeCAD geometry\n")
    f.close()


def export(objectslist, filename):
    for obj in objectslist:
        if isinstance(obj, Mesh.Feature):
            exportMeshToTetGenPoly(obj.Mesh, filename, False)
            break


def createMesh():
    ## ========================  Script beginning...  ========================
    beVerbose = 1
    if beVerbose == 1:
        FreeCAD.Console.PrintMessage("\n\n\n\n\n\n\n\nScript starts...")
    ## Geometry definition
    # Define objects names
    PyDocumentName = "pnJunction"
    PSideBoxName = "PSide"
    NSideBoxName = "NSide"
    DepletionBoxName = "Depletion"
    SurfDepletionBoxName = "SurfDepletion"
    OxideBoxName = "Oxide"
    AdsorbtionBoxName = "Adsorbtion"
    pnMeshName = "pnMesh"

    # Init objects
    if beVerbose == 1:
        FreeCAD.Console.PrintMessage("\nInit Objects...")
    # App.closeDocument(App.ActiveDocument.Label) #closeDocument after restart of macro. Needs any ActiveDocument.
    AppPyDoc = App.newDocument(PyDocumentName)
    NSideBox = AppPyDoc.addObject("Part::Box", NSideBoxName)
    PSideBox = AppPyDoc.addObject("Part::Box", PSideBoxName)
    DepletionBox = AppPyDoc.addObject("Part::Box", DepletionBoxName)
    SurfDepletionBox = AppPyDoc.addObject("Part::Box", SurfDepletionBoxName)
    OxideBox = AppPyDoc.addObject("Part::Box", OxideBoxName)
    AdsorbtionBox = AppPyDoc.addObject("Part::Box", AdsorbtionBoxName)
    pnMesh = AppPyDoc.addObject("Mesh::Feature", pnMeshName)

    BoxList = [NSideBox, DepletionBox, PSideBox, OxideBox, AdsorbtionBox, SurfDepletionBox]
    NSideBoxMesh = Mesh.Mesh()
    PSideBoxMesh = Mesh.Mesh()
    DepletionBoxMesh = Mesh.Mesh()
    SurfDepletionBoxMesh = Mesh.Mesh()
    OxideBoxMesh = Mesh.Mesh()
    AdsorbtionBoxMesh = Mesh.Mesh()
    BoxMeshList = [NSideBoxMesh, DepletionBoxMesh, PSideBoxMesh, OxideBoxMesh, AdsorbtionBoxMesh, SurfDepletionBoxMesh]
    if beVerbose == 1:
        if len(BoxList) != len(BoxMeshList):
            FreeCAD.Console.PrintMessage("\n ERROR! Input len() of BoxList and BoxMeshList is not the same! ")

    ## Set sizes in nanometers
    if beVerbose == 1:
        FreeCAD.Console.PrintMessage("\nSet sizes...")
    tessellationTollerance = 0.05
    ModelWidth = 300
    BulkHeight = 300
    BulkLength = 300
    DepletionSize = 50
    OxideThickness = 5
    AdsorbtionThickness = 10

    # Big volumes of n and p material
    NSideBox.Height = BulkHeight  # Z-direction
    NSideBox.Width = ModelWidth  # Y-direction = const
    NSideBox.Length = BulkLength  # X-direction
    PSideBox.Height = BulkHeight
    PSideBox.Width = ModelWidth
    PSideBox.Length = BulkLength
    # Thin depletion layer between
    DepletionBox.Height = BulkHeight
    DepletionBox.Width = ModelWidth
    DepletionBox.Length = DepletionSize * 2
    # Surface deplation layer
    SurfDepletionBox.Height = DepletionSize
    SurfDepletionBox.Width = ModelWidth
    SurfDepletionBox.Length = BulkLength * 2 + DepletionSize * 2
    # Oxide on the top
    OxideBox.Height = OxideThickness
    OxideBox.Width = ModelWidth
    OxideBox.Length = BulkLength * 2 + DepletionSize * 2
    # Adsorbtion layer
    AdsorbtionBox.Height = AdsorbtionThickness
    AdsorbtionBox.Width = ModelWidth
    AdsorbtionBox.Length = BulkLength * 2 + DepletionSize * 2

    # Object placement
    Rot = App.Rotation(0, 0, 0, 1)
    NSideBox.Placement = App.Placement(App.Vector(0, 0, -BulkHeight), Rot)
    PSideBox.Placement = App.Placement(App.Vector(DepletionSize * 2 + BulkLength, 0, -BulkHeight), Rot)
    DepletionBox.Placement = App.Placement(App.Vector(BulkLength, 0, -BulkHeight), Rot)
    SurfDepletionBox.Placement = App.Placement(App.Vector(0, 0, 0), Rot)
    OxideBox.Placement = App.Placement(App.Vector(0, 0, DepletionSize), Rot)
    AdsorbtionBox.Placement = App.Placement(App.Vector(0, 0, DepletionSize + OxideThickness), Rot)

    ## Unite
    if beVerbose == 1:
        FreeCAD.Console.PrintMessage("\nFuse objects...")
    fuseShape = BoxList[0].Shape
    for index in range(1, len(BoxList), 1):
        fuseShape = fuseShape.fuse(BoxList[index].Shape)
    nmesh = Mesh.Mesh()
    nmesh.addFacets(fuseShape.tessellate(tessellationTollerance))

    # for index in range(len(BoxList)):
    for index in range(len(BoxList) - 1):  # Manual hack
        BoxMeshList[index].addFacets(BoxList[index].Shape.tessellate(tessellationTollerance))
        nmesh.addMesh(BoxMeshList[index])

    nmesh.removeDuplicatedPoints()
    nmesh.removeDuplicatedFacets()
    pnMesh.Mesh = nmesh

    # Hide all boxes
    for box in BoxList:
        Gui.hideObject(box)
    # # Remove all boxes
    # for box in BoxList:
    #     App.ActiveDocument.removeObject(box.Name)

    # Update document
    AppPyDoc.recompute()

    ## export to TenGen *.poly (use File|Export instead)
    # filePath = "/home/tig/tmp/tetgen/pnJunction.poly"
    # exportMeshToTetGenPoly(pnMesh.Mesh,filePath,beVerbose)

    Gui.activeDocument().activeView().viewAxometric()
    Gui.SendMsgToActiveView("ViewFit")
    if beVerbose == 1:
        FreeCAD.Console.PrintMessage("\nScript finished without errors.")

##  @}
