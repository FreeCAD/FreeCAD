# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2009, 2010                                              *
# *   Yorik van Havre <yorik@uncreated.net>, Ken Cline <cline@frii.com>     *
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

__title__ = "FreeCAD Draft Workbench - Vector library"
__author__ = "Yorik van Havre, Werner Mayer, Martin Burbaum, Ken Cline"
__url__ = ["http://www.freecadweb.org"]

## \defgroup DRAFTVECUTILS DraftVecUtils
#  \ingroup UTILITIES
#  \brief Vector math utilities used in Draft workbench
#
# Vector math utilities used primarily in the Draft workbench
# but which can also be used in other workbenches and macros.

## \addtogroup DRAFTVECUTILS
#  @{

import sys
import math, FreeCAD
from FreeCAD import Vector, Matrix
from FreeCAD import Console as FCC

try:
    long
except NameError:
    long = int

params = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")


def precision():
    return params.GetInt("precision", 6)


def typecheck(args_and_types, name="?"):
    """Check that the argument is an instance of a certain type.

    Parameters
    ----------
    args_and_types : list
        A list of tuples. The first element of the tuple is tested as being
        an instance of the second element.
        ``args_and_types = [(a, Type), (b, Type2), ...]``

        Then
        ``isinstance(a, Type)``,
        ``isinstance(b, Type2)``

        A `Type` can also be a tuple of many types, in which case
        the check is done for any of them.

        ``args_and_types = [(a, (Type3, int, float)), ...]``

        ``isinstance(a, (Type3, int, float))``

    name : str, optional
        Defaults to '?'. The name of the check.

    Raises
    -------
    TypeError
        If the first element in the tuple is not an instance of the second
        element.
    """
    for v, t in args_and_types:
        if not isinstance(v, t):
            _msg = ("typecheck[" + str(name) + "]: "
                    + str(v) + " is not " + str(t) + "\n")
            FCC.PrintWarning(_msg)
            raise TypeError("fcvec." + str(name))


def toString(u):
    """Return a string with the Python command to recreate this Vector.

    Parameters
    ----------
    u : list, or Base::Vector3
        A list of FreeCAD.Vectors, or a single vector.

    Returns
    -------
    str
        The string with the code that can be input in the Python console
        to create the same list of FreeCAD.Vectors, or single vector.
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
        If `True` the output is an array.

    Returns
    -------
    tuple or list
    (x, y, z) or [x, y, z]
        The coordinates of the vector in a tuple or a list, if `array=True`.
    """
    typecheck([(u, Vector)], "tup")
    if array:
        return [u.x, u.y, u.z]
    else:
        return (u.x, u.y, u.z)


def neg(u):
    """Return the negative of a given FreeCAD.Vector.

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
    """Check for equality between two FreeCAD.Vectors.

    Due to rounding errors, two vectors will rarely be 'equal'.
    Therefore, this fucntion checks that the corresponding elements
    of the two vectors differ by less than the precision established
    in the parameter database, accessed through `FreeCAD.ParamGet()`.
    x1 - x2 < precision
    y1 - y2 < precision
    z1 - z2 < precision

    Parameters
    ----------
    u : Base::Vector3
    v : Base::Vector3
        The FreeCAD.Vectors to compare.

    Returns
    ------
    bool
        `True` if the vectors are within the precision, `False` otherwise.
    """
    typecheck([(u, Vector), (v, Vector)], "equals")
    return isNull(u.sub(v))


def scale(u, scalar):
    """Scales (multiplies) a vector by a factor.

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
    # Python 2 has two integer types, int and long.
    # In Python 3 there is no 'long' anymore.
    if sys.version_info.major < 3:
        typecheck([(u, Vector), (scalar, (long, int, float))], "scale")
    else:
        typecheck([(u, Vector), (scalar, (int, int, float))], "scale")
    return Vector(u.x*scalar, u.y*scalar, u.z*scalar)


def scaleTo(u, l):
    """Scale a vector so that its magnitude is equal to a given length.

    The magnitude of a vector is
    ``L = sqrt(x**2 + y**2 + z**2)``

    This function multiplies each coordinate, x, y, z,
    by a factor to produce the desired magnitude.
    This factor is the ratio of the new magnitude to the old magnitude,
    ``x_scaled = x * (new/old)``

    Parameters
    ----------
    u : Base::Vector3
        The FreeCAD.Vector to scale.
    l : int or float
        The new magnitude of the vector in standard units (mm).

    Returns
    -------
    Base::Vector3
        The new vector with each of its elements scaled by a factor.
        Or the same vector `u` if the vector is (0, 0, 0).
    """
    # Python 2 has two integer types, int and long.
    # In Python 3 there is no 'long' anymore.
    if sys.version_info.major < 3:
        typecheck([(u, Vector), (l, (long, int, float))], "scaleTo")
    else:
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
        First point, defined by a FreeCAD.Vector.
    v : Base::Vector3
        Second point, defined by a FreeCAD.Vector.

    Returns
    -------
    float
        The scalar distance from one point to the other.
    """
    typecheck([(u, Vector), (v, Vector)], "dist")
    return u.sub(v).Length


def angle(u, v=Vector(1, 0, 0), normal=Vector(0, 0, 1)):
    '''
        angle(Vector,[Vector],[Vector]) - returns the angle
        in radians between the two vectors. If only one is given,
        angle is between the vector and the horizontal East direction.
    If a third vector is given, it is the normal used to determine
        the sign of the angle.
    '''
    typecheck([(u, Vector), (v, Vector)], "angle")
    ll = u.Length*v.Length
    if ll == 0:
        return 0
    dp = u.dot(v)/ll
    if (dp < -1):
        dp = -1  # roundoff errors can push dp out of the ...
    elif (dp > 1):
        dp = 1  # ...geometrically meaningful interval [-1,1]
    ang = math.acos(dp)
    normal1 = u.cross(v)
    coeff = normal.dot(normal1)
    if coeff >= 0:
        return ang
    else:
        return -ang


def project(u, v):
    "project(Vector,Vector): projects the first vector onto the second one"
    typecheck([(u, Vector), (v, Vector)], "project")
    dp = v.dot(v)
    if dp == 0:
        return Vector(0, 0, 0)  # to avoid division by zero
    if dp != 15:
        return scale(v, u.dot(v)/dp)
    return Vector(0, 0, 0)


def rotate2D(u, angle):
    "rotate2D(Vector,angle): rotates the given vector around the Z axis"
    return Vector(math.cos(-angle)*u.x-math.sin(-angle)*u.y,
                  math.sin(-angle)*u.x+math.cos(-angle)*u.y,
                  u.z)


def rotate(u, angle, axis=Vector(0, 0, 1)):
    '''rotate(Vector,Float,axis=Vector): rotates the first Vector
    around the given axis, at the given angle.
    If axis is omitted, the rotation is made on the xy plane.'''
    typecheck([(u, Vector), (angle, (int, float)), (axis, Vector)], "rotate")

    if angle == 0:
        return u

    L = axis.Length
    x = axis.x/L
    y = axis.y/L
    z = axis.z/L
    c = math.cos(angle)
    s = math.sin(angle)
    t = 1 - c

    xyt = x*y*t
    xzt = x*z*t
    yzt = y*z*t
    xs = x*s
    ys = y*s
    zs = z*s

    m = Matrix(c + x*x*t,   xyt - zs,   xzt + ys,   0,
               xyt + zs,    c + y*y*t,  yzt - xs,   0,
               xzt - ys,    yzt + xs,   c + z*z*t,  0)

    return m.multiply(u)


def getRotation(vector, reference=Vector(1, 0, 0)):
    '''getRotation(vector,[reference]): returns a tuple
    representing a quaternion rotation between the reference
    (or X axis if omitted) and the vector'''
    c = vector.cross(reference)
    if isNull(c):
        return (0, 0, 0, 1.0)
    c.normalize()
    return (c.x, c.y, c.z, math.sqrt((vector.Length ** 2) * (reference.Length ** 2)) + vector.dot(reference))


def isNull(vector):
    '''isNull(vector): Tests if a vector is nul vector'''
    p = precision()
    return (round(vector.x, p) == 0 and round(vector.y, p) == 0 and round(vector.z, p) == 0)


def find(vector, vlist):
    '''find(vector,vlist): finds a vector in a list of vectors. returns
    the index of the matching vector, or None if none is found.
    '''
    typecheck([(vector, Vector), (vlist, list)], "find")
    for i, v in enumerate(vlist):
        if equals(vector, v):
            return i
    return None


def closest(vector, vlist):
    '''closest(vector,vlist): finds the closest vector to the given vector
    in a list of vectors'''
    typecheck([(vector, Vector), (vlist, list)], "closest")
    dist = 9999999999999999
    index = None
    for i, v in enumerate(vlist):
        d = vector.sub(v).Length
        if d < dist:
            dist = d
            index = i
    return index


def isColinear(vlist):
    '''isColinear(list_of_vectors): checks if vectors in given list are colinear'''
    typecheck([(vlist, list)], "isColinear")
    if len(vlist) < 3:
        return True
    p = precision()
    first = vlist[1].sub(vlist[0])
    for i in range(2, len(vlist)):
        if round(angle(vlist[i].sub(vlist[0]), first), p) != 0:
            return False
    return True


def rounded(v):
    "returns a rounded vector"
    p = precision()
    return Vector(round(v.x, p), round(v.y, p), round(v.z, p))


def getPlaneRotation(u, v, w=None):
    "returns a rotation matrix defining the (u,v,w) coordinates system"
    if (not u) or (not v):
        return None
    if not w:
        w = u.cross(v)
    typecheck([(u, Vector), (v, Vector), (w, Vector)], "getPlaneRotation")
    m = FreeCAD.Matrix(u.x, v.x, w.x, 0,
                       u.y, v.y, w.y, 0,
                       u.z, v.z, w.z, 0,
                       0.0, 0.0, 0.0, 1.0)
    return m


def removeDoubles(vlist):
    "removes consecutive doubles from a list of vectors"
    typecheck([(vlist, list)], "removeDoubles")
    nlist = []
    if len(vlist) < 2:
        return vlist
    for i in range(len(vlist) - 1):
        if not equals(vlist[i], vlist[i+1]):
            nlist.append(vlist[i])
    nlist.append(vlist[-1])
    return nlist

##  @}
