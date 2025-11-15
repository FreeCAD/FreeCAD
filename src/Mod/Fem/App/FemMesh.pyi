# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from typing import Any, Final, overload

from Base.Metadata import constmethod, export
from Base.Vector import Vector
from Base.Placement import Placement
from App.ComplexGeoData import ComplexGeoData
from Part.App.TopoShape import TopoShape
from Part.App.TopoShapeFace import TopoShapeFace
from Part.App.TopoShapeEdge import TopoShapeEdge
from Part.App.TopoShapeSolid import TopoShapeSolid
from Part.App.TopoShapeVertex import TopoShapeVertex

@export(
    Include="Mod/Fem/App/FemMesh.h",
    Namespace="Fem",
    FatherInclude="App/ComplexGeoDataPy.h",
    FatherNamespace="Data",
    Constructor=True,
)
class FemMesh(ComplexGeoData):
    """
    FemMesh class

    Author: Juergen Riegel (Juergen.Riegel@web.de)
    License: LGPL-2.1-or-later
    """

    def setShape(self, shape: TopoShape, /) -> None:
        """Set the Part shape to mesh"""
        ...

    def compute(self) -> None:
        """Update the internal mesh structure"""
        ...

    def addHypothesis(self, hypothesis: object, shape: TopoShape, /) -> None:
        """Add hypothesis"""
        ...

    def setStandardHypotheses(self) -> None:
        """Set some standard hypotheses for the whole shape"""
        ...

    def addNode(self, x: float, y: float, z: float, elem_id: int | None = None, /) -> int:
        """Add a node by setting (x,y,z)."""
        ...

    @overload
    def addEdge(self, n1: int, n2: int, /) -> int: ...
    @overload
    def addEdge(self, nodes: list[int], elem_id: int | None = None, /) -> int: ...
    def addEdge(self, *args) -> int:
        """Add an edge by setting two node indices."""
        ...

    def addEdgeList(self, nodes: list[int], np: list[int], /) -> list[int]:
        """Add list of edges by list of node indices and list of nodes per edge."""
        ...

    @overload
    def addFace(self, n1: int, n2: int, n3: int, /) -> int: ...
    @overload
    def addFace(self, nodes: list[int], elem_id: int | None = None, /) -> int: ...
    def addFace(self) -> Any:
        """Add a face by setting three node indices."""
        ...

    def addFaceList(self, nodes: list[int], np: list[int], /) -> list[int]:
        """Add list of faces by list of node indices and list of nodes per face."""
        ...

    def addQuad(self, n1: int, n2: int, n3: int, n4: int, /) -> int:
        """Add a quad by setting four node indices."""
        ...

    @overload
    def addVolume(self, n1: int, n2: int, n3: int, n4: int, /) -> int: ...
    @overload
    def addVolume(self, nodes: list[int], elem_id: int | None = None, /) -> int: ...
    def addVolume(self, *args) -> int:
        """Add a volume by setting an arbitrary number of node indices."""
        ...

    def addVolumeList(self, nodes: list[int], np: list[int], /) -> list[int]:
        """Add list of volumes by list of node indices and list of nodes per volume."""
        ...

    def read(self, file_name: str, /) -> None:
        """
        Read in a various FEM mesh file formats.


        Supported formats: DAT, INP, MED, STL, UNV, VTK, Z88
        """
        ...

    @constmethod
    def write(self, file_name: str, /) -> None:
        """
        Write out various FEM mesh file formats.

        Supported formats: BDF, DAT, INP, MED, STL, UNV, VTK, Z88
        """
        ...

    @constmethod
    def writeABAQUS(
        self,
        fileName: str,
        elemParam: int,
        groupParam: bool,
        volVariant: str = "standard",
        faceVariant: str = "shell",
        edgeVariant: str = "beam",
    ) -> None:
        """
        Write out as ABAQUS inp.

        elemParam:
            0: All elements
            1: Highest elements only
            2: FEM elements only (only edges not belonging to faces and faces not belonging to volumes)

        groupParam:
            True: Write group data
            False: Do not write group data

        volVariant: Volume elements
            "standard": Tetra4 -> C3D4, Penta6 -> C3D6, Hexa8 -> C3D8, Tetra10 -> C3D10, Penta15 -> C3D15, Hexa20 -> C3D20
            "reduced": Hexa8 -> C3D8R, Hexa20 -> C3D20R
            "incompatible": Hexa8 -> C3D8I
            "modified": Tetra10 -> C3D10T
            "fluid": Tetra4 -> F3D4, Penta6 -> F3D6, Hexa8  -> F3D8

        faceVariant: Face elements
            "shell": Tria3 -> S3, Quad4 -> S4, Tria6 -> S6, Quad8 -> S8
            "shell reduced": Tria3 -> S3, Quad4 -> S4R, Tria6 -> S6, Quad8 -> S8R
            "membrane": Tria3 -> M3D3, Quad4 -> M3D4, Tria6 -> M3D6, Quad8 -> M3D8
            "membrane reduced": Tria3 -> M3D3, Quad4 -> M3D4R, Tria6 -> M3D6, Quad8 -> M3D8R
            "stress": Tria3 -> CPS3, Quad4 -> CPS4, Tria6 -> CPS6, Quad8 -> CPS8
            "stress reduced": Tria3 -> CPS3, Quad4 -> CPS4R, Tria6 -> CPS6, Quad8 -> CPS8R
            "strain": Tria3 -> CPE3, Quad4 -> CPE4, Tria6 -> CPE6, Quad8 -> CPE8
            "strain reduced": Tria3 -> CPE3, Quad4 -> CPE4R, Tria6 -> CPE6, Quad8 -> CPE8R
            "axisymmetric": Tria3 -> CAX3, Quad4 -> CAX4, Tria6 -> CAX6, Quad8 -> CAX8
            "axisymmetric reduced": Tria3 -> CAX3, Quad4 -> CAX4R, Tria6 -> CAX6, Quad8 -> CAX8R

        edgeVariant: Edge elements
            "beam": Seg2 -> B31, Seg3 -> B32
            "beam reduced": Seg2 -> B31R, Seg3 -> B32R
            "truss": Seg2 -> T3D2, eg3 -> T3D3
            "network": Seg3 -> D

        Elements are selected according to CalculiX availability.
        For example if volume variant "modified" is selected, Tetra10 mesh
        elements are assigned to C3D10T and remain elements uses "standard".
        Axisymmetric, plane strain and plane stress elements expect nodes in the plane z=0.
        """
        ...

    def setTransform(self, placement: Placement, /) -> None:
        """Use a Placement object to perform a translation or rotation"""
        ...

    @constmethod
    def copy(self) -> FemMesh:
        """Make a copy of this FEM mesh."""
        ...

    @constmethod
    def getFacesByFace(self, face: TopoShapeFace, /) -> list[int]:
        """Return a list of face IDs which belong to a TopoFace"""
        ...

    @constmethod
    def getEdgesByEdge(self, edge: TopoShapeEdge, /) -> list[int]:
        """Return a list of edge IDs which belong to a TopoEdge"""
        ...

    @constmethod
    def getVolumesByFace(self, face: TopoShapeFace, /) -> list[tuple[int, int]]:
        """Return a list of tuples of volume IDs and face IDs which belong to a TopoFace"""
        ...

    @constmethod
    def getccxVolumesByFace(self, face: TopoShapeFace, /) -> list[tuple[int, int]]:
        """Return a list of tuples of volume IDs and ccx face numbers which belong to a TopoFace"""
        ...

    @constmethod
    def getNodeById(self, node_id: int, /) -> Vector:
        """Get the node position vector by a Node-ID"""
        ...

    @constmethod
    def getNodesBySolid(self, shape: TopoShapeSolid, /) -> list[int]:
        """Return a list of node IDs which belong to a TopoSolid"""
        ...

    @constmethod
    def getNodesByFace(self, face: TopoShapeFace, /) -> list[int]:
        """Return a list of node IDs which belong to a TopoFace"""
        ...

    @constmethod
    def getNodesByEdge(self, edge: TopoShapeEdge, /) -> list[int]:
        """Return a list of node IDs which belong to a TopoEdge"""
        ...

    @constmethod
    def getNodesByVertex(self, vertex: TopoShapeVertex, /) -> list[int]:
        """Return a list of node IDs which belong to a TopoVertex"""
        ...

    @constmethod
    def getElementNodes(self, elem_id: int, /) -> tuple[int, ...]:
        """Return a tuple of node IDs to a given element ID"""
        ...

    @constmethod
    def getNodeElements(self, elem_id: int, elem_type: str = "All", /) -> tuple[int, ...]:
        """Return a tuple of specific element IDs associated to a given node ID"""
        ...

    @constmethod
    def getGroupName(self, elem_id: int, /) -> str:
        """Return a string of group name to a given group ID"""
        ...

    @constmethod
    def getGroupElementType(self, elem_id: int, /) -> str:
        """Return a string of group element type to a given group ID"""
        ...

    @constmethod
    def getGroupElements(self, elem_id: int, /) -> tuple[int, ...]:
        """Return a tuple of ElementIDs to a given group ID"""
        ...

    @constmethod
    def addGroup(self, name: str, group_type: str, group_id: int = -1, /) -> None:
        """
        Add a group to mesh with specific name and type

        name: string
        group_type: "All", "Node", "Edge", "Face", "Volume", "0DElement", "Ball"
        group_id: int
            Optional group_id is used to force specific id for group, but does
            not work, yet.
        """
        ...

    @constmethod
    def addGroupElements(self, group_id: int, elements: list[int], /) -> None:
        """
        Add a tuple of ElementIDs to a given group ID

        group_id: int
        elements: list of int
            Notice that the elements have to be in the mesh.
        """
        ...

    @constmethod
    def removeGroup(self, group_id: int, /) -> bool:
        """
        Remove a group with a given group ID
                            removeGroup(groupid)
                            groupid: int
                            Returns boolean."""
        ...

    @constmethod
    def renameGroup(self) -> Any:
        """Rename a group with a given group ID
        renameGroup(id, name)
        groupid: int
        name: string"""
        ...

    @constmethod
    def getElementType(self, elem_id: int, /) -> str:
        """Return the element type of a given ID"""
        ...

    @constmethod
    def getIdByElementType(self, elem_type: str, /) -> tuple[int, ...]:
        """Return a tuple of IDs to a given element type"""
        ...
    Nodes: Final[dict]
    """Dictionary of Nodes by ID (int ID:Vector())"""

    NodeCount: Final[int]
    """Number of nodes in the Mesh."""

    Edges: Final[tuple]
    """Tuple of edge IDs"""

    EdgesOnly: Final[tuple]
    """Tuple of edge IDs which does not belong to any face (and thus not belong to any volume too)"""

    EdgeCount: Final[int]
    """Number of edges in the Mesh."""

    Faces: Final[tuple]
    """Tuple of face IDs"""

    FacesOnly: Final[tuple]
    """Tuple of face IDs which does not belong to any volume"""

    FaceCount: Final[int]
    """Number of Faces in the Mesh."""

    TriangleCount: Final[int]
    """Number of Triangles in the Mesh."""

    QuadrangleCount: Final[int]
    """Number of Quadrangles in the Mesh."""

    PolygonCount: Final[int]
    """Number of Quadrangles in the Mesh."""

    Volumes: Final[tuple]
    """Tuple of volume IDs"""

    VolumeCount: Final[int]
    """Number of Volumes in the Mesh."""

    TetraCount: Final[int]
    """Number of Tetras in the Mesh."""

    HexaCount: Final[int]
    """Number of Hexas in the Mesh."""

    PyramidCount: Final[int]
    """Number of Pyramids in the Mesh."""

    PrismCount: Final[int]
    """Number of Prisms in the Mesh."""

    PolyhedronCount: Final[int]
    """Number of Polyhedrons in the Mesh."""

    SubMeshCount: Final[int]
    """Number of SubMeshs in the Mesh."""

    GroupCount: Final[int]
    """Number of Groups in the Mesh."""

    Groups: Final[tuple]
    """Tuple of Group IDs."""

    Volume: Final[Any]
    """Volume of the mesh."""
