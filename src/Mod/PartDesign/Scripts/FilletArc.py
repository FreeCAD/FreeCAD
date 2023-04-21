#! python
# -*- coding: utf-8 -*-
# (c) 2010 Werner Mayer LGPL

__author__ = "Werner Mayer <wmayer[at]users.sourceforge.net>"

# Formulas:
# M2 = P + b*r2 + t*u
# S1 = (r2*M1 + r1*M2)/(r1+r2)
# S2 = M2-b*r2

import math

# 3d vector class
class Vector:
    def __init__(self, x, y, z):
        self.x = x
        self.y = y
        self.z = z

    def add(self, vec):
        return Vector(self.x+vec.x, self.y+vec.y, self.z+vec.z)

    def sub(self, vec):
        return Vector(self.x-vec.x, self.y-vec.y, self.z-vec.z)

    def dot(self, vec):
        return self.x*vec.x+self.y*vec.y+self.z*vec.z

    def mult(self, s):
        return Vector(self.x*s, self.y*s, self.z*s)

    def cross(self,vec):
        return Vector(
            self.y * vec.z - self.z * vec.y,
            self.z * vec.x - self.x * vec.z,
            self.x * vec.y - self.y * vec.x)

    def length(self):
        return math.sqrt(self.x*self.x+self.y*self.y+self.z*self.z)

    def norm(self):
        l = self.length()
        if l > 0:
            self.x /= l
            self.y /= l
            self.z /= l

    def __repr__(self):
        return "(%f,%f,%f)" % (self.x, self.y, self.z)


# A signum function
def sgn(val):
    if val > 0:
        return 1
    elif val < 0:
        return -1
    else:
        return 0


# M1  ... is the center of the arc
# P   ... is the end point of the arc and start point of the line
# Q   ..  is a second point on the line
# N   ... is the normal of the plane where the arc and the line lie on, usually N=(0,0,1)
# r2  ... the fillet radius
# ccw ... counter-clockwise means which part of the arc is given. ccw must be either True or False


def makeFilletArc(M1,P,Q,N,r2,ccw):
    u = Q.sub(P)
    v = P.sub(M1)
    if ccw:
        b = u.cross(N)
    else:
        b = N.cross(u)
    b.norm()

    uu = u.dot(u)
    uv = u.dot(v)
    r1 = v.length()

    # distinguish between internal and external fillets
    r2 *= sgn(uv)

    cc = 2.0 * r2 * (b.dot(v)-r1)
    dd = uv * uv - uu * cc
    if dd < 0:
        raise RuntimeError("Unable to calculate intersection points")
    t1 = (-uv + math.sqrt(dd)) / uu
    t2 = (-uv - math.sqrt(dd)) / uu

    if (abs(t1) < abs(t2)):
        t = t1
    else:
        t = t2

    br2 = b.mult(r2)
    print(br2)
    ut = u.mult(t)
    print(ut)
    M2 = P.add(ut).add(br2)
    S1 = M1.mult(r2/(r1+r2)).add(M2.mult(r1/(r1+r2)))
    S2 = M2.sub(br2)

    return (S1, S2, M2)



def test():
    from FreeCAD import Base
    import Part

    P1 = Base.Vector(1, -5, 0)
    P2 = Base.Vector(-5, 2, 0)
    P3 = Base.Vector(1, 5, 0)
    # Q = Base.Vector(5, 10, 0)
    # Q = Base.Vector(5, 11, 0)
    Q = Base.Vector(5, 0, 0)
    r2 = 3.0
    axis = Base.Vector(0, 0, 1)
    ccw = False

    arc = Part.ArcOfCircle(P1, P2, P3)
    C = arc.Center
    Part.show(Part.makeLine(P3, Q))
    Part.show(arc.toShape())

    (S1, S2, M2) = makeArc(Vector(C.x,C.y,C.z), Vector(P3.x,P3.y,P3.z), Vector(Q.x, Q.y, Q.z), Vector(axis.x, axis.y, axis.z), r2, ccw)
    circle = Part.Circle(Base.Vector(M2.x, M2.y, M2.z), Base.Vector(0, 0, 1), math.fabs(r2))
    Part.show(circle.toShape())
