from typing import Any, Final

from Base.Metadata import constmethod, export

from App.ComplexGeoData import ComplexGeoData

@export(
    Father="ComplexGeoDataPy",
    Name="FemMeshPy",
    Twin="FemMesh",
    TwinPointer="FemMesh",
    Include="Mod/Fem/App/FemMesh.h",
    Namespace="Fem",
    FatherInclude="App/ComplexGeoDataPy.h",
    FatherNamespace="Data",
)
class FemMeshPy(ComplexGeoData):
    """
    FemMesh class
    """

    @staticmethod
    def PyMake(args: list, kwd: dict) -> Any: ...
    def __init__(self, *args, **kwargs) -> None: ...
    def setShape(self) -> Any:
        """Set the Part shape to mesh"""
        ...

    def compute(self) -> Any:
        """Update the internal mesh structure"""
        ...

    def addHypothesis(self) -> Any:
        """Add hypothesis"""
        ...

    def setStandardHypotheses(self) -> Any:
        """Set some standard hypotheses for the whole shape"""
        ...

    def addNode(self) -> Any:
        """Add a node by setting (x,y,z)."""
        ...

    def addEdge(self) -> Any:
        """Add an edge by setting two node indices."""
        ...

    def addEdgeList(self) -> Any:
        """Add list of edges by list of node indices and list of nodes per edge."""
        ...

    def addFace(self) -> Any:
        """Add a face by setting three node indices."""
        ...

    def addFaceList(self) -> Any:
        """Add list of faces by list of node indices and list of nodes per face."""
        ...

    def addQuad(self) -> Any:
        """Add a quad by setting four node indices."""
        ...

    def addVolume(self) -> Any:
        """Add a volume by setting an arbitrary number of node indices."""
        ...

    def addVolumeList(self) -> Any:
        """Add list of volumes by list of node indices and list of nodes per volume."""
        ...

    def read(self) -> Any:
        """Read in a various FEM mesh file formats.
        read(file.endingToExportTo)
        supported formats: DAT, INP, MED, STL, UNV, VTK, Z88"""
        ...

    @constmethod
    def write(self) -> Any:
        """Write out various FEM mesh file formats.
        write(file.endingToExportTo)
        supported formats: BDF, DAT, INP, MED, STL, UNV, VTK, Z88"""
        ...

    @constmethod
    def writeABAQUS(self) -> Any:
        """Write out as ABAQUS inp
        writeABAQUS(file, int elemParam, bool groupParam, str volVariant, str faceVariant, str edgeVariant)

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

    def setTransform(self) -> Any:
        """Use a Placement object to perform a translation or rotation"""
        ...

    @constmethod
    def copy(self) -> Any:
        """Make a copy of this FEM mesh."""
        ...

    @constmethod
    def getFacesByFace(self) -> Any:
        """Return a list of face IDs which belong to a TopoFace"""
        ...

    @constmethod
    def getEdgesByEdge(self) -> Any:
        """Return a list of edge IDs which belong to a TopoEdge"""
        ...

    @constmethod
    def getVolumesByFace(self) -> Any:
        """Return a dict of volume IDs and face IDs which belong to a TopoFace"""
        ...

    @constmethod
    def getccxVolumesByFace(self) -> Any:
        """Return a dict of volume IDs and ccx face numbers which belong to a TopoFace"""
        ...

    @constmethod
    def getNodeById(self) -> Any:
        """Get the node position vector by a Node-ID"""
        ...

    @constmethod
    def getNodesBySolid(self) -> Any:
        """Return a list of node IDs which belong to a TopoSolid"""
        ...

    @constmethod
    def getNodesByFace(self) -> Any:
        """Return a list of node IDs which belong to a TopoFace"""
        ...

    @constmethod
    def getNodesByEdge(self) -> Any:
        """Return a list of node IDs which belong to a TopoEdge"""
        ...

    @constmethod
    def getNodesByVertex(self) -> Any:
        """Return a list of node IDs which belong to a TopoVertex"""
        ...

    @constmethod
    def getElementNodes(self) -> Any:
        """Return a tuple of node IDs to a given element ID"""
        ...

    @constmethod
    def getNodeElements(self) -> Any:
        """Return a tuple of specific element IDs associated to a given node ID"""
        ...

    @constmethod
    def getGroupName(self) -> Any:
        """Return a string of group name to a given group ID"""
        ...

    @constmethod
    def getGroupElementType(self) -> Any:
        """Return a string of group element type to a given group ID"""
        ...

    @constmethod
    def getGroupElements(self) -> Any:
        """Return a tuple of ElementIDs to a given group ID"""
        ...

    @constmethod
    def addGroup(self) -> Any:
        """Add a group to mesh with specific name and type
        addGroup(name, typestring, [id])
        name: string
        typestring: "All", "Node", "Edge", "Face", "Volume", "0DElement", "Ball"
        id: int
        Optional id is used to force specific id for group, but does
        not work, yet."""
        ...

    @constmethod
    def addGroupElements(self) -> Any:
        """Add a tuple of ElementIDs to a given group ID
        addGroupElements(groupid, list_of_elements)
        groupid: int
        list_of_elements: list of int
        Notice that the elements have to be in the mesh."""
        ...

    @constmethod
    def removeGroup(self) -> Any:
        """Remove a group with a given group ID
        removeGroup(groupid)
        groupid: int
        Returns boolean."""
        ...

    @constmethod
    def getElementType(self) -> Any:
        """Return the element type of a given ID"""
        ...

    @constmethod
    def getIdByElementType(self) -> Any:
        """Return a tuple of IDs to a given element type"""
        ...
    Nodes: Final[Any]
    """Dictionary of Nodes by ID (int ID:Vector())"""

    NodeCount: Final[Any]
    """Number of nodes in the Mesh."""

    Edges: Final[Any]
    """Tuple of edge IDs"""

    EdgesOnly: Final[Any]
    """Tuple of edge IDs which does not belong to any face (and thus not belong to any volume too)"""

    EdgeCount: Final[Any]
    """Number of edges in the Mesh."""

    Faces: Final[Any]
    """Tuple of face IDs"""

    FacesOnly: Final[Any]
    """Tuple of face IDs which does not belong to any volume"""

    FaceCount: Final[Any]
    """Number of Faces in the Mesh."""

    TriangleCount: Final[Any]
    """Number of Triangles in the Mesh."""

    QuadrangleCount: Final[Any]
    """Number of Quadrangles in the Mesh."""

    PolygonCount: Final[Any]
    """Number of Quadrangles in the Mesh."""

    Volumes: Final[Any]
    """Tuple of volume IDs"""

    VolumeCount: Final[Any]
    """Number of Volumes in the Mesh."""

    TetraCount: Final[Any]
    """Number of Tetras in the Mesh."""

    HexaCount: Final[Any]
    """Number of Hexas in the Mesh."""

    PyramidCount: Final[Any]
    """Number of Pyramids in the Mesh."""

    PrismCount: Final[Any]
    """Number of Prisms in the Mesh."""

    PolyhedronCount: Final[Any]
    """Number of Polyhedrons in the Mesh."""

    SubMeshCount: Final[Any]
    """Number of SubMeshs in the Mesh."""

    GroupCount: Final[Any]
    """Number of Groups in the Mesh."""

    Groups: Final[Any]
    """Tuple of Group IDs."""

    Volume: Final[Any]
    """Volume of the mesh."""
