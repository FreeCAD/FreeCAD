import re
import math
from FreeCAD import Vector, Matrix
from DraftVecUtils import equals, isNull, angle
from draftutils.utils import svg_precision
from draftutils.messages import _err, _msg, _wrn

from Part import (
    Arc,
    BezierCurve,
    BSplineCurve,
    Ellipse,
    Face,
    LineSegment,
    Shape,
    Edge,
    Wire,
    Compound,
    OCCError
)

def _tolerance(precision):
    return 10**(-precision)

def _arc_end_to_center(lastvec, currentvec, rx, ry,
                      x_rotation=0.0, correction=False):
    '''Calculate the possible centers for an arc in endpoint parameterization.

    Calculate (positive and negative) possible centers for an arc given in
    ``endpoint parametrization``.
    See http://www.w3.org/TR/SVG/implnote.html#ArcImplementationNotes

    the sweepflag is interpreted as: sweepflag <==>  arc is travelled clockwise

    Parameters
    ----------
    lastvec : Base::Vector3
        First point of the arc.
    currentvec : Base::Vector3
        End point (current) of the arc.
    rx : float
        Radius of the ellipse, semi-major axis in the X direction.
    ry : float
        Radius of the ellipse, semi-minor axis in the Y direction.
    x_rotation : float
        Default is 0. Rotation around the Z axis, in radians (CCW).
    correction : bool, optional
        Default is `False`. If it is `True`, the radii will be scaled
        by a factor.

    Returns
    -------
    list, (float, float)
        A tuple that consists of one list, and a tuple of radii.
    [(positive), (negative)], (rx, ry)
        The first element of the list is the positive tuple,
        the second is the negative tuple.
    [(Base::Vector3, float, float),
    (Base::Vector3, float, float)], (float, float)
        Types
    [(vcenter+, angle1+, angledelta+),
    (vcenter-, angle1-, angledelta-)], (rx, ry)
        The first element of the list is the positive tuple,
        consisting of center, angle, and angle increment;
        the second element is the negative tuple.
    '''
    # scalefacsign = 1 if (largeflag != sweepflag) else -1
    rx = float(rx)
    ry = float(ry)
    v0 = lastvec.sub(currentvec)
    v0.multiply(0.5)
    m1 = Matrix()
    m1.rotateZ(-x_rotation)  # eq. 5.1
    v1 = m1.multiply(v0)
    if correction:
        eparam = v1.x**2 / rx**2 + v1.y**2 / ry**2
        if eparam > 1:
            eproot = math.sqrt(eparam)
            rx = eproot * rx
            ry = eproot * ry
    denom = rx**2 * v1.y**2 + ry**2 * v1.x**2
    numer = rx**2 * ry**2 - denom
    results = []

    # If the division is very small, set the scaling factor to zero,
    # otherwise try to calculate it by taking the square root
    if abs(numer/denom) < 1.0e-7:
        scalefacpos = 0
    else:
        try:
            scalefacpos = math.sqrt(numer/denom)
        except ValueError:
            _msg("sqrt({0}/{1})".format(numer, denom))
            scalefacpos = 0

    # Calculate two values because the square root may be positive or negative
    for scalefacsign in (1, -1):
        scalefac = scalefacpos * scalefacsign
        # Step2 eq. 5.2
        vcx1 = Vector(v1.y * rx/ry, -v1.x * ry/rx, 0).multiply(scalefac)
        m2 = Matrix()
        m2.rotateZ(x_rotation)
        centeroff = currentvec.add(lastvec)
        centeroff.multiply(0.5)
        vcenter = m2.multiply(vcx1).add(centeroff)  # Step3 eq. 5.3
        # angle1 = Vector(1, 0, 0).getAngle(Vector((v1.x - vcx1.x)/rx,
        #                                          (v1.y - vcx1.y)/ry,
        #                                          0))  # eq. 5.5
        # angledelta = Vector((v1.x - vcx1.x)/rx,
        #                     (v1.y - vcx1.y)/ry,
        #                     0).getAngle(Vector((-v1.x - vcx1.x)/rx,
        #                                        (-v1.y - vcx1.y)/ry,
        #                                        0))  # eq. 5.6
        # we need the right sign for the angle
        angle1 = angle(Vector(1, 0, 0),
                       Vector((v1.x - vcx1.x)/rx,
                              (v1.y - vcx1.y)/ry,
                              0))  # eq. 5.5
        angledelta = angle(Vector((v1.x - vcx1.x)/rx,
                                  (v1.y - vcx1.y)/ry,
                                  0),
                           Vector((-v1.x - vcx1.x)/rx,
                                  (-v1.y - vcx1.y)/ry,
                                  0))  # eq. 5.6
        results.append((vcenter, angle1, angledelta))

        if rx < 0 or ry < 0:
            _wrn("Warning: 'rx' or 'ry' is negative, check the SVG file")

    return results, (rx, ry)


def _arc_center_to_end(center, rx, ry, angle1, angledelta, xrotation=0.0):
    '''Calculate start and end points, and flags of an arc.

    Calculate start and end points, and flags of an arc given in
    ``center parametrization``.
    See http://www.w3.org/TR/SVG/implnote.html#ArcImplementationNotes

    Parameters
    ----------
    center : Base::Vector3
        Coordinates of the center of the ellipse.
    rx : float
        Radius of the ellipse, semi-major axis in the X direction
    ry : float
        Radius of the ellipse, semi-minor axis in the Y direction
    angle1 : float
        Initial angle in radians
    angledelta : float
        Additional angle in radians
    xrotation : float, optional
        Default 0. Rotation around the Z axis

    Returns
    -------
    v1, v2, largerc, sweep
        Tuple indicating the end points of the arc, and two boolean values
        indicating whether the arc is less than 180 degrees or not,
        and whether the angledelta is negative.
    '''
    vr1 = Vector(rx * math.cos(angle1), ry * math.sin(angle1), 0)
    vr2 = Vector(rx * math.cos(angle1 + angledelta),
                 ry * math.sin(angle1 + angledelta),
                 0)
    mxrot = Matrix()
    mxrot.rotateZ(xrotation)
    v1 = mxrot.multiply(vr1).add(center)
    v2 = mxrot.multiply(vr2).add(center)
    fa = ((abs(angledelta) / math.pi) % 2) > 1  # < 180 deg
    fs = angledelta < 0
    return v1, v2, fa, fs


def _approx_bspline(
    curve: BezierCurve,
    num: int = 10,
    tol: float = 1e-7,
) -> BSplineCurve | BezierCurve:
    _p0, d0 = curve.getD1(curve.FirstParameter)
    _p1, d1 = curve.getD1(curve.LastParameter)
    if (d0.Length < tol) or (d1.Length < tol):
        tan1 = curve.tangent(curve.FirstParameter)[0]
        tan2 = curve.tangent(curve.LastParameter)[0]
        pts = curve.discretize(num)
        bs = BSplineCurve()
        try:
            bs.interpolate(Points=pts, InitialTangent=tan1, FinalTangent=tan2)
            return bs
        except OCCError:
            pass
    return curve


def _make_wire(path : list[Edge], precision : int, checkclosed : bool=False, donttry : bool=False):
    '''Try to make a wire out of the list of edges.

    If the wire functions fail or the wire is not closed,
    if required the TopoShapeCompoundPy::connectEdgesToWires()
    function is used.

    Parameters
    ----------
    path : list[Edge]
        A collection of edges
    checkclosed : bool, optional
        Default is `False`.
    donttry : bool, optional
        Default is `False`. If it's `True` it won't try to check
        for a closed path.

    Returns
    -------
    Part::Wire
        A wire created from the ordered edges.
    Part::Compound
        A compound made of the edges, but unable to form a wire.
    '''
    if not donttry:
        try:
            sh = Wire(path)
            # sh = Wire(path)
            isok = (not checkclosed) or sh.isClosed()
            if len(sh.Edges) != len(path):
                isok = False
        # BRep_API: command not done
        except OCCError:
            isok = False
    if donttry or not isok:
        # Code from wmayer forum p15549 to fix the tolerance problem
        # original tolerance = 0.00001
        comp = Compound(path)
        _sh = comp.connectEdgesToWires(False, _tolerance(precision))
        sh = _sh.Wires[0]
        if len(sh.Edges) != len(path):
            _wrn("Unable to form a wire. Resort to a Compound of Edges.")
            sh = comp
    return sh


class FaceTreeNode:
    '''
    Building Block of a tree structure holding one-closed-wire faces 
    sorted after their enclosure of each other.
    This class only works with faces that have exactly one closed wire
    '''
    face     : Face
    children : list
    name     : str

    
    def __init__(self, face=None, name="root"):
        super().__init__()
        self.face = face
        self.name = name
        self.children = [] 

      
    def insert (self, face, name):
        ''' 
        takes a single-wire named face, and inserts it into the tree 
        depending on its enclosure in/of already added faces.

        Parameters
        ----------
        face : Face
               single closed wire face to be added to the tree
        name : str
               face identifier       
        ''' 
        for node in self.children:
            if  node.face.Area > face.Area:
                # new face could be encompassed
                if (face.distToShape(node.face)[0] == 0.0 and 
                    face.Wires[0].distToShape(node.face.Wires[0])[0] != 0.0):
                    # it is encompassed - enter next tree layer
                    node.insert(face, name)
                    return
            else:
                # new face could encompass
                if (node.face.distToShape(face)[0] == 0.0 and
                    node.face.Wires[0].distToShape(face.Wires[0])[0] != 0.0):
                    # it does encompass the current child nodes face
                    # create new node from face
                    new = FaceTreeNode(face, name)
                    # swap the new one with the child node 
                    self.children.remove(node)
                    self.children.append(new)
                    # add former child node as child to the new node
                    new.children.append(node)
                    return
        # the face is not encompassing and is not encompassed (from) any
        # other face, we add it as new child 
        new = FaceTreeNode(face, name)
        self.children.append(new)

     
    def makeCuts(self):
        ''' 
        recursively traverse the tree and cuts all faces in even 
        numbered tree levels with their direct childrens faces. 
        Additionally the tree is shrunk by removing the odd numbered 
        tree levels.                 
        '''
        result = self.face
        if not result:
            for node in self.children:
                node.makeCuts()
        else:
            new_children = []
            for node in self.children:
                result = result.cut(node.face)
                for subnode in node.children:
                    subnode.makeCuts()
                    new_children.append(subnode)
            self.children = new_children
            self.face = result

           
    def flatten(self):
        ''' creates a flattened list of face-name tuples from the facetree
            content
        '''
        result = []
        result.append((self.name, self.face))
        for node in self.children:
            result.extend(node.flatten())
        return result  
        
  
  
class SvgPathElement:

    path : list[dict]

    def __init__(self, precision : int, interpol_pts : int, origin : Vector = Vector(0, 0, 0)):
        self.precision = precision
        self.interpol_pts = interpol_pts
        self.path = [{"type": "start", "last_v": origin }]
        
    def add_move(self, x : float, y : float, relative : bool) -> None:
        if relative:
            last_v = self.path[-1]["last_v"].add(Vector(x, -y, 0))
        else:
            last_v = Vector(x, -y, 0)
        # if we're at the beginning of a wire we overwrite the start vector
        if self.path[-1]["type"] == "start":
            self.path[-1]["last_v"] = last_v
        else:
            self.path.append({"type": "start", "last_v": last_v})

    def add_lines(self, coords: list[float], relative: bool) -> None:
        last_v = self.path[-1]["last_v"]
        for x, y in zip(coords[0::2], coords[1::2]):
            if relative:
                last_v = last_v.add(Vector(x, -y, 0))
            else:
                last_v = Vector(x, -y, 0)
            self.path.append({"type": "line", "last_v": last_v})

    def add_horizontals(self, x_coords: list[float], relative: bool) -> None:
        last_v = self.path[-1]["last_v"]
        for x in x_coords:
            if relative:
                last_v = Vector(x + last_v.x, last_v.y, 0)
            else:
                last_v = Vector(x, last_v.y, 0)
            self.path.append({"type": "line", "last_v": last_v})

    def add_verticals(self, y_coords: list[float], relative: bool) -> None:
        last_v = self.path[-1]["last_v"]
        if relative:
            for y in y_coords:
                last_v = Vector(last_v.x, last_v.y - y, 0)
                self.path.append({"type": "line", "last_v": last_v})
        else:
            for y in y_coords:
                last_v = Vector(last_v.x, -y, 0)
                self.path.append({"type": "line", "last_v": last_v})

    def add_arcs(self, args: list[float], relative: bool) -> None:
        p_iter = zip(
            args[0::7], args[1::7], args[2::7], args[3::7],
            args[4::7], args[5::7], args[6::7], strict=False,
        )
        for rx, ry, x_rotation, large_flag, sweep_flag, x, y in p_iter:
            # support for large-arc and x-rotation is missing
            if relative:
                last_v = self.path[-1]["last_v"].add(Vector(x, -y, 0))
            else:
                last_v = Vector(x, -y, 0)
            self.path.append({
                "type": "arc",
                "rx": rx,
                "ry": ry,
                "x_rotation": x_rotation,
                "large_flag": large_flag != 0,
                "sweep_flag": sweep_flag != 0,
                "last_v": last_v
            })

    def add_cubic_beziers(self, args: list[float], relative: bool, smooth: bool) -> None:
        last_v = self.path[-1]["last_v"]
        if smooth:
            p_iter = list(
                zip(
                    args[2::4], args[3::4],
                    args[0::4], args[1::4],
                    args[2::4], args[3::4], strict=False )
            )
        else:
            p_iter = list(
                zip(
                    args[0::6], args[1::6],
                    args[2::6], args[3::6],
                    args[4::6], args[5::6], strict=False )
            )
        for p1x, p1y, p2x, p2y, x, y in p_iter:
            if smooth:
                if self.path[-1]["type"] == "cbezier":
                    pole1 = last_v.sub(self.path[-1]["pole2"]).add(last_v)
                else:
                    pole1 = last_v
            else:
                if relative:
                    pole1 = last_v.add(Vector(p1x, -p1y, 0))
                else:
                    pole1 = Vector(p1x, -p1y, 0)
            if relative:
                pole2 = last_v.add(Vector(p2x, -p2y, 0))
                last_v = last_v.add(Vector(x, -y, 0))
            else:
                pole2 = Vector(p2x, -p2y, 0)
                last_v = Vector(x, -y, 0)

            self.path.append({
                "type": "cbezier",
                "pole1": pole1,
                "pole2": pole2,
                "last_v": last_v
            })

    def add_quadratic_beziers(self, args: list[float], relative: bool, smooth: bool):
        last_v = self.path[-1]["last_v"]
        if smooth:
            p_iter = list( zip( args[1::2], args[1::2],
                                args[0::2], args[1::2], strict=False ) )
        else:
            p_iter = list( zip( args[0::4], args[1::4],
                                args[2::4], args[3::4], strict=False ) )
        for px, py, x, y in p_iter:
            if smooth:
                if self.path[-1]["type"] == "qbezier":
                    pole = last_v.sub(self.path[-1]["pole"]).add(last_v)
                else:
                    pole = last_v
            else:
                if relative:
                    pole = last_v.add(Vector(px, -py, 0))
                else:
                    pole = Vector(px, -py, 0)
            if relative:
                last_v = last_v.add(Vector(x, -y, 0))
            else:
                last_v = Vector(x, -y, 0)

            self.path.append({
                "type": "qbezier",
                "pole": pole,
                "last_v": last_v 
            })

    def add_close(self):
        last_v  = self.path[-1]["last_v"]
        first_v = self.__get_last_start()
        if not equals(last_v, first_v, self.precision):
            self.path.append({"type": "line", "last_v": first_v})
        # assume that a close command finalizes a subpath
        self.path.append({"type": "start", "last_v": first_v})

    def __get_last_start(self) -> Vector:
        """
        Return the startpoint of the last SubPath.
        """
        for pds in reversed(self.path):
            if pds["type"] == "start":
                return pds["last_v"]
        return Vector(0, 0, 0)

    def __correct_last_v(self, pds: dict, last_v: Vector) -> None:
        """
        Correct the endpoint of the given path dataset to the 
        given vector and move possibly associated members accordingly.
        """
        delta = last_v.sub(pds["last_v"])
        # we won't move last_v if it's already correct or if the delta 
        # is substantially greater than what rounding errors could accumulate,
        # so we assume the path is intended to be open. 
        if (delta.x == 0 and delta.y == 0 and delta.z == 0 or
            not isNull(delta, self.precision)):
            return
        
        # for cbeziers we also relocate the second pole
        if pds["type"] == "cbezier":
            pds["pole2"] = pds["pole2"].add(delta)
        # for qbeziers we also relocate the pole by half of the delta
        elif pds["type"] == "qbezier":
            pds["pole"] = pds["pole"].add(delta.scale(0.5, 0.5, 0))
        # all data types have last_v
        pds["last_v"] = last_v


    def correct_endpoints(self):
        """ 
        Correct the endpoints of all subpaths and move possibly 
        associated members accordingly.
        """
        start = None
        last = None
        for pds in self.path:
            if pds["type"] == "start":
                if start:
                    # there is already a start
                    if last:
                        # and there are edges behind us. 
                        # we correct the last to the start vector
                        self.__correct_last_v(last, start["last_v"])
                        last = None
                start = pds
                continue
            last = pds
        if start and last and start != last:
            self.__correct_last_v(last, start["last_v"])


    def create_edges(self) -> list[list[Edge]]:
        """
        Creates shapes from prepared path datasets and returns them in an 
        ordered list of lists of edges, where each 1st order list entry
        represents a single continuous (and probably closed) sub-path.
        """
        result = []
        edges = None
        last_v = Vector(0, 0, 0)
        for pds in self.path:
            next_v = pds["last_v"]
            match pds["type"]:
                case "start":
                    if edges and len(edges) > 0 :
                        result.append(edges)
                    edges = []
                case "line":
                    if equals(last_v, next_v, self.precision):
                        # line segment too short, skip it
                        next_v = last_v
                    else:
                        edges.append(LineSegment(last_v, next_v).toShape())
                case "arc":
                    rx = pds["rx"]
                    ry = pds["ry"]
                    x_rotation = pds["x_rotation"]
                    large_flag = pds["large_flag"]
                    sweep_flag = pds["sweep_flag"]
                    # Calculate the possible centers for an arc
                    # in 'endpoint parameterization'.
                    _x_rot = math.radians(-x_rotation)
                    (solution, (rx, ry)) = _arc_end_to_center(
                        last_v, next_v,
                        rx, ry,
                        _x_rot,
                        correction=True
                    )
                    # Choose one of the two solutions
                    neg_sol = large_flag != sweep_flag
                    v_center, angle1, angle_delta = solution[neg_sol]
                    if ry > rx:
                        rx, ry = ry, rx
                        swap_axis = True
                    else:
                        swap_axis = False
                    e1 = Ellipse(v_center, rx, ry)
                    if sweep_flag:
                        angle1 = angle1 + angle_delta
                        angle_delta = -angle_delta

                    d90 = math.radians(90)
                    e1a = Arc(e1, angle1 - swap_axis * d90, angle1 + angle_delta - swap_axis * d90)
                    seg = e1a.toShape()
                    if swap_axis:
                        seg.rotate(v_center, Vector(0, 0, 1), 90)
                    _tol = _tolerance(self.precision)
                    if abs(x_rotation) > _tol:
                        seg.rotate(v_center, Vector(0, 0, 1), -x_rotation)
                    if sweep_flag:
                        seg.reverse()
                    edges.append(seg)

                case "cbezier":
                    pole1 = pds["pole1"]
                    pole2 = pds["pole2"]
                    _tol = _tolerance(self.precision + 2)
                    _d1 = pole1.distanceToLine(last_v, next_v)
                    _d2 = pole2.distanceToLine(last_v, next_v)
                    if _d1 < _tol and _d2 < _tol:
                        # poles and endpoints are all on a line
                        if equals(last_v, next_v, self.precision):
                            # in this case we don't accept (nearly) zero
                            # distance between start and end (skip it).
                            next_v = last_v
                        else:
                            seg = LineSegment(last_v, next_v).toShape()
                            edges.append(seg)
                    else:
                        b = BezierCurve()
                        b.setPoles([last_v, pole1, pole2, next_v])
                        seg = _approx_bspline(b, self.interpol_pts).toShape()
                        edges.append(seg)
                case "qbezier":
                    if equals(last_v, next_v, self.precision):
                        # segment too small - skipping.
                        next_v = last_v
                    else:
                        pole = pds["pole"]
                        _tol = _tolerance(self.precision + 2)
                        _distance = pole.distanceToLine(last_v, next_v)
                        if _distance < _tol:
                            # pole is on the line
                            _seg = LineSegment(last_v, next_v)
                            seg = _seg.toShape()
                        else:
                            b = BezierCurve()
                            b.setPoles([last_v, pole, next_v])
                            seg = _approx_bspline(b, self.interpol_pts).toShape()
                        edges.append(seg)
                case _:
                    _msg("Illegal path_data type. {}".format(pds['type']))
                    return []
            last_v = next_v
        if not edges is None and len(edges) > 0 :
            result.append(edges)
        return result

        

class SvgPathParser:
    """Parse SVG path data and create FreeCAD Shapes."""

    commands : list[tuple]
    pointsre : re.Pattern
    data     : dict
    shapes   : list[list[Shape]]
    faces    : FaceTreeNode  
    name     : str

    def __init__(self, data, name):
        super().__init__()
        """Evaluate path data and initialize."""
        _op = '([mMlLhHvVaAcCqQsStTzZ])'
        _op2 = '([^mMlLhHvVaAcCqQsStTzZ]*)'
        _command = '\\s*?' + _op + '\\s*?' + _op2 + '\\s*?'
        pathcommandsre = re.compile(_command, re.DOTALL)
    
        _num = '[-+]?[0-9]*\\.?[0-9]+'
        _exp = '([eE][-+]?[0-9]+)?'
        _arg = '(' + _num + _exp + ')'
        self.commands = pathcommandsre.findall(' '.join(data['d']))
        self.argsre = re.compile(_arg, re.DOTALL)
        self.data = data
        self.paths = []
        self.shapes = []
        self.faces = None
        self.name = name
     
        
    def parse(self):
        ''' 
        Creates lists of SvgPathElements from raw svg path 
        data. It's supposed to be called direct after SvgPath Object
        creation.
        '''
        path = SvgPathElement(svg_precision(), 10)
        self.paths = []
        for d, argsstr in self.commands:
            relative = d.islower()
            
            _args = self.argsre.findall(argsstr.replace(',', ' '))
            args = [float(number) for number, exponent in _args]

            if d in "Mm":
                path.add_move(args.pop(0), args.pop(0), relative)
            if d in "LlMm":
                path.add_lines(args, relative)
            elif d in "Hh":
                path.add_horizontals(args, relative)
            elif d in "Vv":
                path.add_verticals(args, relative)
            elif d in "Aa":
                path.add_arcs(args, relative)
            elif d in "Cc":
                path.add_cubic_beziers(args, relative, False)
            elif d in "Ss":
                path.add_cubic_beziers(args, relative, True)
            elif d in "Qq":
                path.add_quadratic_beziers(args, relative, False)
            elif d in "Tt":
                path.add_quadratic_beziers(args, relative, True)
            elif d in "Zz":
                path.add_close()
                
        path.correct_endpoints();
        self.shapes = path.create_edges()
        
        
    def create_faces(self, fill=True, add_wire_for_invalid_face=False):
        ''' 
        Generate Faces from lists of Shapes.
        If shapes form a closed wire and the fill Attribute is set, we 
        generate a closed Face. Otherwise we treat the shape as pure wire.
        
        Parameters
        ----------
        fill : Object/bool
               if True or not None Faces are generated from closed shapes.
        '''
        precision = svg_precision()
        cnt = -1;
        openShapes = []
        self.faces = FaceTreeNode()
        for sh in self.shapes:
            cnt += 1
            add_wire = True
            wr = _make_wire(sh, precision, checkclosed=True)
            wrcpy = wr.copy();
            wire_reason = ""
            if cnt > 0:
                face_name = self.name + "_" + str(cnt)
            else:
                face_name = self.name 

                
            if not fill:
                wire_reason = " no-fill"
            if not wr.Wires[0].isClosed():
                wire_reason += " open Wire"
            if fill and wr.Wires[0].isClosed():
                try:
                    face = Face(wr)
                    if not face.isValid():
                        add_wire = add_wire_for_invalid_face
                        wire_reason = " invalid Face"
                        if face.fix(1e-6, 0, 1):
                            res = "succeed"
                        else:
                            res = "fail"
                        _wrn("Invalid Face '{}' created. Attempt to fix - {}ed."
                              .format(face_name, res))
                    else:
                        add_wire = False
                    if not (face.Area < 10 * (_tolerance(precision) ** 2)):
                        self.faces.insert(face, face_name)
                except:
                    _wrn("Failed to make a shape from '{}'. ".format(face_name) 
                          + "This Path will be discarded.")
            if add_wire:
                if wrcpy.Length > _tolerance(precision):
                    _msg("Adding wire for '{}' - reason: {}."
                          .format(face_name, wire_reason))
                    openShapes.append((face_name + "_w", wrcpy))

        self.shapes = openShapes


    def doCuts(self):
        ''' Exposes the FaceTreeNode.makeCuts function of the tree containing 
            closed wire faces.
            This function is called after creating closed Faces with
            'createFaces' in order to hollow faces encompassing others.
        '''      
        self.faces.makeCuts()


    def getShapeList(self):
        ''' Returns the resulting list of tuples containing name and face of 
            each created element.
        ''' 
        result = self.faces.flatten()
        result.extend(self.shapes)             
        return result
