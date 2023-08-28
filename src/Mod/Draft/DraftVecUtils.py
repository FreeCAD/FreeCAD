# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
"""Provide vector math utilities used in the Draft workbench.

Vector math utilities used primarily in the Draft workbench
but which can also be used in other workbenches and in macros.
"""
## \defgroup DRAFTVECUTILS DraftVecUtils
#  \ingroup UTILITIES
#  \brief Vector math utilities used in Draft workbench
#
# Vector math utilities used primarily in the Draft workbench
# but which can also be used in other workbenches and in macros.

# Check code with
# flake8 --ignore=E226,E266,E401,W503

import math

import FreeCAD
from FreeCAD import Vector
import draftutils.messages as messages

__title__ = "FreeCAD Draft Workbench - Vector library"
__author__ = "Yorik van Havre, Werner Mayer, Martin Burbaum, Ken Cline"
__url__ = "https://www.freecad.org"

params = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")

## \addtogroup DRAFTVECUTILS
#  @{


def precision():
    """Get the number of decimal numbers used for precision.

    Returns
    -------
    int
        Return the number of decimal places set up in the preferences,
        or a standard value (6), if the parameter is missing.
    """
    return params.GetInt("precision", 6)


def typecheck(args_and_types, name="?"):
    """Check that the arguments are instances of certain types.

    Parameters
    ----------
    args_and_types : list
        A list of tuples. The first element of a tuple is tested as being
        an instance of the second element.
        ::
            args_and_types = [(a, Type), (b, Type2), ...]

        Then
        ::
            isinstance(a, Type)
            isinstance(b, Type2)

        A `Type` can also be a tuple of many types, in which case
        the check is done for any of them.
        ::
            args_and_types = [(a, (Type3, int, float)), ...]

            isinstance(a, (Type3, int, float))

    name : str, optional
        Defaults to `'?'`. The name of the check.

    Raises
    ------
    TypeError
        If the first element in the tuple is not an instance of the second
        element.
    """
    for v, t in args_and_types:
        if not isinstance(v, t):
            _msg = "typecheck[{0}]: {1} is not {2}".format(name, v, t)
            messages._wrn(_msg)
            raise TypeError("fcvec." + str(name))


def toString(u):
    """Return a string with the Python command to recreate this vector.

    Parameters
    ----------
    u : list, or Base::Vector3
        A list of FreeCAD.Vectors, or a single vector.

    Returns
    -------
    str
        The string with the code that can be used in the Python console
        to create the same list of vectors, or single vector.
    """
    if isinstance(u, list):
        s = "["
        first = True
        for v in u:
            s += "FreeCAD.Vector("
            s += str(v.x) + ", " + str(v.y) + ", " + str(v.z)
            s += ")"
            # This test isn't needed, because `first` never changes value?
            if first:
                s += ", "
                first = True
        # Remove the last comma
        s = s.rstrip(", ")
        s += "]"
        return s
    else:
        s = "FreeCAD.Vector("
        s += str(u.x) + ", " + str(u.y) + ", " + str(u.z)
        s += ")"
        return s


def tup(u, array=False):
    """Return a tuple or a list with the coordinates of a vector.

    Parameters
    ----------
    u : Base::Vector3
        A FreeCAD.Vector.
    array : bool, optional
        Defaults to `False`, and the output is a tuple.
        If `True` the output is a list.

    Returns
    -------
    tuple or list
        The coordinates of the vector in a tuple `(x, y, z)`
        or in a list `[x, y, z]`, if `array=True`.
    """
    typecheck([(u, Vector)], "tup")
    if array:
        return [u.x, u.y, u.z]
    else:
        return (u.x, u.y, u.z)


def neg(u):
    """Return the negative of a given vector.

    Parameters
    ----------
    u : Base::Vector3
        A FreeCAD.Vector.

    Returns
    -------
    Base::Vector3
        A vector in which each element has the opposite sign of
        the original element.
    """
    typecheck([(u, Vector)], "neg")
    return Vector(-u.x, -u.y, -u.z)


def equals(u, v):
    """Check for equality between two vectors.

    Due to rounding errors, two vectors will rarely be `equal`.
    Therefore, this function checks that the corresponding elements
    of the two vectors differ by less than the decimal `precision` established
    in the parameter database, accessed through `FreeCAD.ParamGet()`.
    ::
        x1 - x2 < precision
        y1 - y2 < precision
        z1 - z2 < precision

    Parameters
    ----------
    u : Base::Vector3
        The first vector.
    v : Base::Vector3
        The second vector.

    Returns
    -------
    bool
        `True` if the vectors are within the precision, `False` otherwise.
    """
    typecheck([(u, Vector), (v, Vector)], "equals")
    return isNull(u.sub(v))


def scale(u, scalar):
    """Scales (multiplies) a vector by a scalar factor.

    Parameters
    ----------
    u : Base::Vector3
        The FreeCAD.Vector to scale.
    scalar : float
        The scaling factor.

    Returns
    -------
    Base::Vector3
        The new vector with each of its elements multiplied by `scalar`.
    """
    typecheck([(u, Vector), (scalar, (int, int, float))], "scale")
    return Vector(u.x*scalar, u.y*scalar, u.z*scalar)


def scaleTo(u, l):
    """Scale a vector so that its magnitude is equal to a given length.

    The magnitude of a vector is
    ::
        L = sqrt(x**2 + y**2 + z**2)

    This function multiplies each coordinate, `x`, `y`, `z`,
    by a factor to produce the desired magnitude `L`.
    This factor is the ratio of the new magnitude to the old magnitude,
    ::
        x_scaled = x * (L_new/L_old)

    Parameters
    ----------
    u : Base::Vector3
        The vector to scale.
    l : int or float
        The new magnitude of the vector in standard units (mm).

    Returns
    -------
    Base::Vector3
        The new vector with each of its elements scaled by a factor.
        Or the same input vector `u`, if it is `(0, 0, 0)`.
    """
    typecheck([(u, Vector), (l, (int, int, float))], "scaleTo")
    if u.Length == 0:
        return Vector(u)
    else:
        a = l/u.Length
        return Vector(u.x*a, u.y*a, u.z*a)


def dist(u, v):
    """Return the distance between two points (or vectors).

    Parameters
    ----------
    u : Base::Vector3
        First point, defined by a vector.
    v : Base::Vector3
        Second point, defined by a vector.

    Returns
    -------
    float
        The scalar distance from one point to the other.
    """
    typecheck([(u, Vector), (v, Vector)], "dist")
    return u.sub(v).Length


def angle(u, v=Vector(1, 0, 0), normal=Vector(0, 0, 1)):
    """Return the angle in radians between the two vectors.

    It uses the definition of the dot product
    ::
        A * B = |A||B| cos(angle)

    If only one vector is given, the angle is between that one and the
    horizontal (+X).

    If a third vector is given, it is the normal used to determine
    the sign of the angle.
    This normal is used to calculate a `factor` as the dot product
    with the cross product of the first two vectors.
    ::
        C = A x B
        factor = normal * C

    If the `factor` is positive the angle is positive, otherwise
    it is the opposite sign.

    Parameters
    ----------
    u : Base::Vector3
        The first vector.
    v : Base::Vector3, optional
        The second vector to test against the first one.
        It defaults to `(1, 0, 0)`, or +X.
    normal : Base::Vector3, optional
        The vector indicating the normal.
        It defaults to `(0, 0, 1)`, or +Z.

    Returns
    -------
    float
        The angle in radians between the vectors.
        It is zero if the magnitude of one of the vectors is zero,
        or if they are colinear.
    """
    typecheck([(u, Vector), (v, Vector)], "angle")
    ll = u.Length * v.Length
    if ll == 0:
        return 0

    # The dot product indicates the projection of one vector over the other
    dp = u.dot(v)/ll

    # Due to rounding errors, the dot product could be outside
    # the range [-1, 1], so let's force it to be within this range.
    if dp < -1:
        dp = -1
    elif dp > 1:
        dp = 1

    ang = math.acos(dp)

    # The cross product compared with the provided normal
    normal1 = u.cross(v)
    coeff = normal.dot(normal1)
    if coeff >= 0:
        return ang
    else:
        return -ang


def project(u, v):
    """Project the first vector onto the second one.

    The projection is just the second vector scaled by a factor.
    This factor is the dot product divided by the square
    of the second vector's magnitude.
    ::
        f = A * B / |B|**2 = |A||B| cos(angle) / |B|**2
        f = |A| cos(angle)/|B|

    Parameters
    ----------
    u : Base::Vector3
        The first vector.
    v : Base::Vector3
        The second vector.

    Returns
    -------
    Base::Vector3
        The new vector, which is the same vector `v` scaled by a factor.
        Return `Vector(0, 0, 0)`, if the magnitude of the second vector
        is zero.
    """
    typecheck([(u, Vector), (v, Vector)], "project")

    # Dot product with itself equals the magnitude squared.
    dp = v.dot(v)
    if dp == 0:
        return Vector(0, 0, 0)  # to avoid division by zero
    # Why specifically this value? This should be an else?
    if dp != 15:
        return scale(v, u.dot(v)/dp)

    # Return a null vector if the magnitude squared is 15, why?
    return Vector(0, 0, 0)


def rotate2D(u, angle):
    """Rotate the given vector around the Z axis by the specified angle.

    The rotation occurs in two dimensions only by means of
    a rotation matrix.
    ::
         u_rot                R                 u
        (x_rot) = (cos(-angle) -sin(-angle)) * (x)
        (y_rot)   (sin(-angle)  cos(-angle))   (y)

    Normally the angle is positive, but in this case it is negative.

    `"Such non-standard orientations are rarely used in mathematics
    but are common in 2D computer graphics, which often have the origin
    in the top left corner and the y-axis pointing down."`
    W3C Recommendations (2003), Scalable Vector Graphics: the initial
    coordinate system.

    Parameters
    ----------
    u : Base::Vector3
        The vector.
    angle : float
        The angle of rotation given in radians.

    Returns
    -------
    Base::Vector3
        The new rotated vector.
    """
    x_rot = math.cos(-angle) * u.x - math.sin(-angle) * u.y
    y_rot = math.sin(-angle) * u.x + math.cos(-angle) * u.y

    return Vector(x_rot, y_rot, u.z)


def rotate(u, angle, axis=Vector(0, 0, 1)):
    """Rotate the vector by the specified angle, around the given axis.

    If the axis is omitted, the rotation is made around the Z axis
    (on the XY plane).

    It uses a 3x3 rotation matrix.
    ::
        u_rot = R u

                (c + x*x*t    xyt - zs     xzt + ys )
        u_rot = (xyt + zs     c + y*y*t    yzt - xs ) * u
                (xzt - ys     yzt + xs     c + z*z*t)

    Where `x`, `y`, `z` indicate unit components of the axis;
    `c` denotes a cosine of the angle; `t` indicates a complement
    of that cosine; `xs`, `ys`, `zs` indicate products of the unit
    components and the sine of the angle; and `xyt`, `xzt`, `yzt`
    indicate products of two unit components and the complement
    of the cosine.

    Parameters
    ----------
    u : Base::Vector3
        The vector.
    angle : float
        The angle of rotation given in radians.
    axis : Base::Vector3, optional
        The vector specifying the axis of rotation.
        It defaults to `(0, 0, 1)`, the +Z axis.

    Returns
    -------
    Base::Vector3
        The new rotated vector.
        If the `angle` is zero, return the original vector `u`.
    """
    typecheck([(u, Vector), (angle, (int, float)), (axis, Vector)], "rotate")

    if angle == 0:
        return u

    # Unit components, so that x**2 + y**2 + z**2 = 1
    L = axis.Length
    x = axis.x/L
    y = axis.y/L
    z = axis.z/L

    c = math.cos(angle)
    s = math.sin(angle)
    t = 1 - c

    # Various products
    xyt = x * y * t
    xzt = x * z * t
    yzt = y * z * t
    xs = x * s
    ys = y * s
    zs = z * s

    m = FreeCAD.Matrix(c + x*x*t,   xyt - zs,   xzt + ys,   0,
                       xyt + zs,    c + y*y*t,  yzt - xs,   0,
                       xzt - ys,    yzt + xs,   c + z*z*t,  0)

    return m.multiply(u)


def getRotation(vector, reference=Vector(1, 0, 0)):
    """Return a quaternion rotation between a vector and a reference.

    If the reference is omitted, the +X axis is used.

    Parameters
    ----------
    vector : Base::Vector3
        The original vector.
    reference : Base::Vector3, optional
        The reference vector. It defaults to `(1, 0, 0)`, the +X axis.

    Returns
    -------
    (x, y, z, Q)
        A tuple with the unit elements (normalized) of the cross product
        between the `vector` and the `reference`, and a `Q` value,
        which is the sum of the products of the magnitudes,
        and of the dot product of those vectors.
        ::
            Q = |A||B| + |A||B| cos(angle)

        It returns `(0, 0, 0, 1.0)`
        if the cross product between the `vector` and the `reference`
        is null.

    See Also
    --------
    rotate2D, rotate
    """
    c = vector.cross(reference)
    if isNull(c):
        return (0, 0, 0, 1.0)
    c.normalize()

    q1 = math.sqrt((vector.Length**2) * (reference.Length**2))
    q2 = vector.dot(reference)
    Q = q1 + q2

    return (c.x, c.y, c.z, Q)


def isNull(vector):
    """Return False if each of the components of the vector is zero.

    Due to rounding errors, an element is probably never going to be
    exactly zero. Therefore, it rounds the element by the number
    of decimals specified in the `precision` parameter
    in the parameter database, accessed through `FreeCAD.ParamGet()`.
    It then compares the rounded numbers against zero.

    Parameters
    ----------
    vector : Base::Vector3
        The tested vector.

    Returns
    -------
    bool
        `True` if each of the elements is zero within the precision.
        `False` otherwise.
    """
    p = precision()
    x = round(vector.x, p)
    y = round(vector.y, p)
    z = round(vector.z, p)
    return (x == 0 and y == 0 and z == 0)


def find(vector, vlist):
    """Find a vector in a list of vectors, and return the index.

    Finding a vector tests for `equality` which depends on the `precision`
    parameter in the parameter database.

    Parameters
    ----------
    vector : Base::Vector3
        The tested vector.
    vlist : list
        A list of Base::Vector3 vectors.

    Returns
    -------
    int
        The index of the list where the vector is found,
        or `None` if the vector is not found.

    See Also
    --------
    equals : test for equality between two vectors
    """
    typecheck([(vector, Vector), (vlist, list)], "find")
    for i, v in enumerate(vlist):
        if equals(vector, v):
            return i
    return None


def closest(vector, vlist, return_length=False):
    """Find the closest point to one point in a list of points (vectors).

    The scalar distance between the original point and one point in the list
    is calculated. If the distance is smaller than a previously calculated
    value, its index is saved, otherwise the next point in the list is tested.

    Parameters
    ----------
    vector: Base::Vector3
        The tested point or vector.

    vlist: list
        A list of points or vectors.

    return_length: bool, optional
        It defaults to `False`.
        If it is `True`, the value of the smallest distance will be returned.

    Returns
    -------
    int
        The index of the list where the closest point is found.

    int, float
        If `return_length` is `True`, it returns both the index
        and the length to the closest point.
    """
    typecheck([(vector, Vector), (vlist, list)], "closest")

    # Initially test against a very large distance, then test the next point
    # in the list which will probably be much smaller.
    dist = 9999999999999999
    index = None
    for i, v in enumerate(vlist):
        d = (vector - v).Length
        if d < dist:
            dist = d
            index = i

    if return_length:
        return index, dist
    else:
        return index


def isColinear(vlist):
    """Check if the vectors in the list are colinear.

    Colinear vectors are those whose angle between them is zero.

    This function tests for colinearity between the difference
    of the first two vectors, and the difference of the nth vector with
    the first vector.
    ::
        vlist = [a, b, c, d, ..., n]

        k = b - a
        k2 = c - a
        k3 = d - a
        kn = n - a

    Then test
    ::
        angle(k2, k) == 0
        angle(k3, k) == 0
        angle(kn, k) == 0

    Parameters
    ----------
    vlist : list
        List of Base::Vector3 vectors.
        At least three elements must be present.

    Returns
    -------
    bool
        `True` if the vector differences are colinear,
        or if the list only has two vectors.
        `False` otherwise.

    Notes
    -----
    Due to rounding errors, the angle may not be exactly zero;
    therefore, it rounds the angle by the number
    of decimals specified in the `precision` parameter
    in the parameter database, and then compares the value to zero.
    """
    typecheck([(vlist, list)], "isColinear")

    # Return True if the list only has two vectors, why?
    # This doesn't test for colinearity between the first two vectors.
    if len(vlist) < 3:
        return True

    p = precision()

    # Difference between the second vector and the first one
    first = vlist[1].sub(vlist[0])

    # Start testing from the third vector and onward
    for i in range(2, len(vlist)):

        # Difference between the 3rd vector and onward, and the first one.
        diff = vlist[i].sub(vlist[0])

        # The angle between the difference and the first difference.
        _angle = angle(diff, first)

        if round(_angle, p) != 0:
            return False
    return True


def rounded(v,d=None):
    """Return a vector rounded to the `precision` in the parameter database
    or to the given decimals value

    Each of the components of the vector is rounded to the decimal
    precision set in the parameter database.

    Parameters
    ----------
    v : Base::Vector3
        The input vector.
    d : (Optional) the number of decimals to round to

    Returns
    -------
    Base::Vector3
        The new vector where each element `x`, `y`, `z` has been rounded
        to the number of decimals specified in the `precision` parameter
        in the parameter database.
    """
    p = precision()
    if d:
        p = d
    return Vector(round(v.x, p), round(v.y, p), round(v.z, p))


def getPlaneRotation(u, v, _ = None):
    """Return a rotation matrix defining the (u,v,w) coordinate system.

    The rotation matrix uses the elements from each vector.
    `v` is adjusted to be perpendicular to `u`
    ::
            (u.x  v.x  w.x  0  )
        R = (u.y  v.y  w.y  0  )
            (u.z  v.z  w.z  0  )
            (0    0    0    1.0)

    Parameters
    ----------
    u : Base::Vector3
        The first vector.
    v : Base::Vector3
        Hint for the second vector.
    _ : Ignored. For backwards compatibility

    Returns
    -------
    Base::Matrix4D
        The new rotation matrix defining a new coordinate system,
        or `None` if `u` or `v` is `None` or
        if `u` and `v` are parallel.
    """
    if (not u) or (not v):
        return None
    typecheck([(u, Vector), (v, Vector)], "getPlaneRotation")
    u = Vector(u)
    u.normalize()
    w = u.cross(v)
    if not w.Length:
        return None
    w.normalize()
    v = w.cross(u)

    m = FreeCAD.Matrix(u.x, v.x, w.x, 0,
                       u.y, v.y, w.y, 0,
                       u.z, v.z, w.z, 0,
                       0.0, 0.0, 0.0, 1.0)
    return m


def removeDoubles(vlist):
    """Remove duplicated vectors from a list of vectors.

    It removes only the duplicates that are next to each other in the list.

    It tests the `i` element, and compares it to the `i+1` element.
    If the former one is different from the latter,
    the former is added to the new list, otherwise it is skipped.
    The last element is always included.
    ::
        [a, b, b, c, c] -> [a, b, c]
        [a, a, b, a, a, b] -> [a, b, a, b]

    Finding duplicated vectors tests for `equality` which depends
    on the `precision` parameter in the parameter database.

    Parameters
    ----------
    vlist : list of Base::Vector3
        List with vectors.

    Returns
    -------
    list of Base::Vector3
        New list with sequential duplicates removed,
        or the original `vlist` if there is only one element in the list.

    See Also
    --------
    equals : test for equality between two vectors
    """
    typecheck([(vlist, list)], "removeDoubles")
    nlist = []
    if len(vlist) < 2:
        return vlist

    # Iterate until the penultimate element, and test for equality
    # with the element in front
    for i in range(len(vlist) - 1):
        if not equals(vlist[i], vlist[i+1]):
            nlist.append(vlist[i])
    # Add the last element
    nlist.append(vlist[-1])
    return nlist

def get_spherical_coords(x, y, z):
    """Get the Spherical coordinates of the vector represented
    by Cartesian coordinates (x, y, z).

    Parameters
    ----------
    vector : Base::Vector3
        The input vector.

    Returns
    -------
    tuple of float
        Tuple (radius, theta, phi) with the Spherical coordinates.
        Radius is the radial coordinate, theta the polar angle and
        phi the azimuthal angle in radians.

    Notes
    -----
    The vector (0, 0, 0) has undefined values for theta and phi, while
    points on the z axis has undefined value for phi. The following
    conventions are used (useful in DraftToolBar methods):
    (0, 0, 0) -> (0, pi/2, 0)
    (0, 0, z) -> (radius, theta, 0)
    """

    v = Vector(x,y,z)
    x_axis = Vector(1,0,0)
    z_axis = Vector(0,0,1)
    y_axis = Vector(0,1,0)
    rad = v.Length

    if not bool(round(rad, precision())):
        return (0, math.pi/2, 0)

    theta = v.getAngle(z_axis)
    v.projectToPlane(Vector(0,0,0), z_axis)
    phi = v.getAngle(x_axis)
    if math.isnan(phi):
        return (rad, theta, 0)
    # projected vector is on 3rd or 4th quadrant
    if v.dot(Vector(y_axis)) < 0:
        phi = -1*phi

    return (rad, theta, phi)


def get_cartesian_coords(radius, theta, phi):
    """Get the three-dimensional Cartesian coordinates of the vector
    represented by Spherical coordinates (radius, theta, phi).

    Parameters
    ----------
    radius : float, int
        Radial coordinate of the vector.
    theta : float, int
        Polar coordinate of the vector in radians.
    phi : float, int
        Azimuthal coordinate of the vector in radians.

    Returns
    -------
    tuple of float :
        Tuple (x, y, z) with the Cartesian coordinates.
    """

    x = radius*math.sin(theta)*math.cos(phi)
    y = radius*math.sin(theta)*math.sin(phi)
    z = radius*math.cos(theta)

    return (x, y, z)


##  @}
