# Copyright (c) 2005 Berthold Grupp
# License: LGPL


"""Python Module for building solid regular geometric objects.

Return value are list of vectors, 3 vectors define a facet.

Sample code for creating a mesh:
	facets = Cube(3.0, 4.0, 5.0)
	m = Mesh.newMesh()
	m.addFacets(facets)
"""


import math


def Sphere(radius, count):
    """Creates a sphere with a given radius.

    bla bla bla

    """
    return Ellipsoid(radius, radius, count)


def Ellipsoid(lenX, lenY, count):
    polyline = []
    step = math.pi / count
    i = 0.0
    while i < math.pi + step / 10.0:
        x = math.cos(i) * lenX
        y = math.sin(i) * lenY
        polyline.append([x, y])
        i = i + step

    return RotationBody(polyline, count)


def Cylinder(radius, len, closed, edgelen, count):
    return Cone(radius, radius, len, closed, edgelen, count)


def Cone(radius1, radius2, len, closed, edgelen, count):
    polyline = []
    if closed:
        try:
            step = radius2 / math.ceil(radius2 / edgelen)
        except ZeroDivisionError:
            pass
        else:
            i = 0.0
            while i < radius2 - step / 2.0:
                polyline.append([len, i])
                i = i + step

    ct = math.ceil(len / edgelen)
    step = len / ct
    rstep = (radius1 - radius2) / ct
    i = len
    r = radius2
    while i > 0.0 + step / 2.0:
        polyline.append([i, r])
        i = i - step
        r = r + rstep
    polyline.append([0.0, radius1])

    if closed:
        try:
            step = radius1 / math.ceil(radius1 / edgelen)
        except ZeroDivisionError:
            pass
        else:
            i = radius1 - step
            while i > 0.0 + step / 2.0:
                polyline.append([0.0, i])
                i = i - step
            polyline.append([0.0, 0.0])

    return RotationBody(polyline, count)


def Toroid(radius1, radius2, count):
    polyline = []

    step = math.pi * 2.0 / count
    i = -math.pi
    while i < math.pi + step / 10.0:
        x = radius1 + math.cos(i) * radius2
        y = radius1 + math.sin(i) * radius2
        polyline.append([x, y])
        i = i + step

    return RotationBody(polyline, count)


def RotationBody(polyline, count):
    """Build a rotation body from a given (closed) polyline, rotation axis is the X-Axis.

    Parameter: polyline: list of tuple of 2 floats (2d vector)

    """
    facets = []

    step = math.pi * 2.0 / count
    i = -math.pi
    while i < math.pi - step / 10.0:
        li = i + step
        for j in range(0, len(polyline) - 1):
            v1 = polyline[j]
            v2 = polyline[j + 1]

            x1 = v1[0]
            y1 = v1[1] * math.cos(i)
            z1 = v1[1] * math.sin(i)
            x2 = v2[0]
            y2 = v2[1] * math.cos(i)
            z2 = v2[1] * math.sin(i)
            x3 = v1[0]
            y3 = v1[1] * math.cos(li)
            z3 = v1[1] * math.sin(li)
            x4 = v2[0]
            y4 = v2[1] * math.cos(li)
            z4 = v2[1] * math.sin(li)

            if v1[1] != 0.0:
                facets.append([x1, y1, z1])
                facets.append([x2, y2, z2])
                facets.append([x3, y3, z3])

            if v2[1] != 0.0:
                facets.append([x2, y2, z2])
                facets.append([x4, y4, z4])
                facets.append([x3, y3, z3])

        i = i + step

    return facets


def Cube(lenX, lenY, lenZ):
    hx = lenX / 2.0
    hy = lenY / 2.0
    hz = lenZ / 2.0

    facets = []

    facets.append([-hx, -hy, -hz])
    facets.append([hx, -hy, -hz])
    facets.append([hx, -hy, hz])

    facets.append([-hx, -hy, -hz])
    facets.append([hx, -hy, hz])
    facets.append([-hx, -hy, hz])

    facets.append([-hx, hy, -hz])
    facets.append([hx, hy, hz])
    facets.append([hx, hy, -hz])

    facets.append([-hx, hy, -hz])
    facets.append([-hx, hy, hz])
    facets.append([hx, hy, hz])

    facets.append([-hx, -hy, -hz])
    facets.append([-hx, hy, hz])
    facets.append([-hx, hy, -hz])

    facets.append([-hx, -hy, -hz])
    facets.append([-hx, -hy, hz])
    facets.append([-hx, hy, hz])

    facets.append([hx, -hy, -hz])
    facets.append([hx, hy, -hz])
    facets.append([hx, hy, hz])

    facets.append([hx, -hy, -hz])
    facets.append([hx, hy, hz])
    facets.append([hx, -hy, hz])

    facets.append([-hx, -hy, -hz])
    facets.append([-hx, hy, -hz])
    facets.append([hx, hy, -hz])

    facets.append([-hx, -hy, -hz])
    facets.append([hx, hy, -hz])
    facets.append([hx, -hy, -hz])

    facets.append([-hx, -hy, hz])
    facets.append([hx, hy, hz])
    facets.append([-hx, hy, hz])

    facets.append([-hx, -hy, hz])
    facets.append([hx, -hy, hz])
    facets.append([hx, hy, hz])

    return facets


def FineCube(lenX, lenY, lenZ, edgelen):
    hx = lenX / 2.0
    hy = lenY / 2.0
    hz = lenZ / 2.0
    cx = int(max(lenX / edgelen, 1))
    dx = lenX / cx
    cy = int(max(lenY / edgelen, 1))
    dy = lenY / cy
    cz = int(max(lenZ / edgelen, 1))
    dz = lenZ / cz

    facets = []

    # z
    for i in range(0, cx):
        for j in range(0, cy):
            facets.append([-hx + (i + 0) * dx, -hy + (j + 0) * dy, -hz])
            facets.append([-hx + (i + 0) * dx, -hy + (j + 1) * dy, -hz])
            facets.append([-hx + (i + 1) * dx, -hy + (j + 1) * dy, -hz])

            facets.append([-hx + (i + 0) * dx, -hy + (j + 0) * dy, -hz])
            facets.append([-hx + (i + 1) * dx, -hy + (j + 1) * dy, -hz])
            facets.append([-hx + (i + 1) * dx, -hy + (j + 0) * dy, -hz])

            facets.append([-hx + (i + 0) * dx, -hy + (j + 0) * dy, hz])
            facets.append([-hx + (i + 1) * dx, -hy + (j + 1) * dy, hz])
            facets.append([-hx + (i + 0) * dx, -hy + (j + 1) * dy, hz])

            facets.append([-hx + (i + 0) * dx, -hy + (j + 0) * dy, hz])
            facets.append([-hx + (i + 1) * dx, -hy + (j + 0) * dy, hz])
            facets.append([-hx + (i + 1) * dx, -hy + (j + 1) * dy, hz])

    # y
    for i in range(0, cx):
        for j in range(0, cz):
            facets.append([-hx + (i + 0) * dx, -hy, -hz + (j + 0) * dz])
            facets.append([-hx + (i + 1) * dx, -hy, -hz + (j + 1) * dz])
            facets.append([-hx + (i + 0) * dx, -hy, -hz + (j + 1) * dz])

            facets.append([-hx + (i + 0) * dx, -hy, -hz + (j + 0) * dz])
            facets.append([-hx + (i + 1) * dx, -hy, -hz + (j + 0) * dz])
            facets.append([-hx + (i + 1) * dx, -hy, -hz + (j + 1) * dz])

            facets.append([-hx + (i + 0) * dx, hy, -hz + (j + 0) * dz])
            facets.append([-hx + (i + 0) * dx, hy, -hz + (j + 1) * dz])
            facets.append([-hx + (i + 1) * dx, hy, -hz + (j + 1) * dz])

            facets.append([-hx + (i + 0) * dx, hy, -hz + (j + 0) * dz])
            facets.append([-hx + (i + 1) * dx, hy, -hz + (j + 1) * dz])
            facets.append([-hx + (i + 1) * dx, hy, -hz + (j + 0) * dz])

    # x
    for i in range(0, cy):
        for j in range(0, cz):
            facets.append([-hx, -hy + (i + 0) * dy, -hz + (j + 0) * dz])
            facets.append([-hx, -hy + (i + 0) * dy, -hz + (j + 1) * dz])
            facets.append([-hx, -hy + (i + 1) * dy, -hz + (j + 1) * dz])

            facets.append([-hx, -hy + (i + 0) * dy, -hz + (j + 0) * dz])
            facets.append([-hx, -hy + (i + 1) * dy, -hz + (j + 1) * dz])
            facets.append([-hx, -hy + (i + 1) * dy, -hz + (j + 0) * dz])

            facets.append([hx, -hy + (i + 0) * dy, -hz + (j + 0) * dz])
            facets.append([hx, -hy + (i + 1) * dy, -hz + (j + 1) * dz])
            facets.append([hx, -hy + (i + 0) * dy, -hz + (j + 1) * dz])

            facets.append([hx, -hy + (i + 0) * dy, -hz + (j + 0) * dz])
            facets.append([hx, -hy + (i + 1) * dy, -hz + (j + 0) * dz])
            facets.append([hx, -hy + (i + 1) * dy, -hz + (j + 1) * dz])

    return facets


def main():
    Cylinder(10.0, 20.0, 1, 10, 10)


if __name__ == "__main__":
    main()
