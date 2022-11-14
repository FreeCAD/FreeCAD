# (c) 2020 Adam Spontarelli <adam@vector-space.org>
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU Lesser General Public License (LGPL)
#   as published by the Free Software Foundation; either version 2 of
#   the License, or (at your option) any later version.
#   for detail see the LICENCE text file.
#
#   FCGear is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU Library General Public License for more details.
#
#   You should have received a copy of the GNU Library General Public
#   License along with FCGear; if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307

from math import cos, sin, tan, sqrt, radians, atan, asin, degrees


def CreateSprocket(w, P, N, Dr):
    """
    Create a sprocket

    w is the wirebuilder object (in which the sprocket will be constructed)
    P is the chain pitch
    N is the number of teeth
    Dr is the roller diameter

    Remaining variables can be found in Standard Handbook of Chains
    """
    Ds = 1.005 * Dr + (0.003 * 25.4)
    R = Ds / 2
    M = 0.8 * Dr * cos(radians(35) + radians(60/N))
    T = 0.8 * Dr * sin(radians(35) + radians(60/N))
    E = 1.3025 * Dr + (0.0015 * 25.4)
    W = 1.4 * Dr * cos(radians(180/N))
    V = 1.4 * Dr * sin(radians(180/N))
    F = Dr * (0.8 * cos(radians(18) - radians(56)/N) + 1.4 *
              cos(radians(17) - radians(64) / N) - 1.3025) - (0.0015 * 25.4)
    PD = P / (sin(radians(180)/N))
    # H = sqrt(F**2 - (1.4 * Dr - P/2)**2)
    # OD = P * (0.6 + 1/tan(radians(180/N)))

    # The sprocket tooth gullet consists of four segments
    x0 = 0
    y0 = PD/2 - R

    # ---- Segment 1 -----
    alpha = 35 + 60/N
    x1 = -R * cos(radians(alpha))
    y1 = PD/2 - R * sin(radians(alpha))

    # ---- Segment 2 -----
    alpha = 35 + 60/N
    beta = 18 - 56 / N
    x2 = M - E * cos(radians(alpha-beta))
    y2 = T - E * sin(radians(alpha-beta)) + PD/2

    # # ---- Segment 3 -----
    y2o = y2 - PD/2
    hyp = sqrt((-W-x2)**2 + (-V-y2o)**2)
    AP = sqrt(hyp**2 - F**2)
    gamma = atan((y2o + V)/(x2 + W))
    alpha = asin(AP / hyp)
    beta = 180 - (90 - degrees(alpha)) - (90 - degrees(gamma))
    x3o = AP * sin(radians(beta))
    y3o = AP * cos(radians(beta))
    x3 = x2 - x3o
    y3 = y2 + y3o

    # ---- Segment 4 -----
    alpha = 180/N
    m = -1/tan(radians(alpha))
    yf = PD/2 - V
    A = 1 + m**2
    B = 2*m*yf - 2*W
    C = W**2 + yf**2 - F**2
    # x4a = (-B - sqrt(B**2 - 4 * A * C)) / (2*A)
    x4b = (-B + sqrt(B**2 - 4 * A * C)) / (2*A)
    x4 = -x4b
    y4 = m * x4

    p0 = [x0,y0]
    p1 = [x1,y1]
    p2 = [x2,y2]
    p3 = [x3,y3]
    p4 = [x4,y4]
    p5 = [-x1,y1]
    p6 = [-x2,y2]
    p7 = [-x3,y3]
    p8 = [-x4,y4]

    w.move(p4) # vectors are lists [x,y]
    w.arc(p3, F, 0)
    w.line(p2)
    w.arc(p1, E, 1)
    w.arc(p0, R, 1)

    # ---- Mirror -----
    w.arc(p5, R, 1)
    w.arc(p6, E, 1)
    w.line(p7)
    w.arc(p8, F, 0)

    # ---- Polar Array ----
    alpha = -radians(360/N)
    for n in range(1,N):
        # falling gullet slope
        w.arc(rotate(p3, alpha*n), F, 0)
        w.line(rotate(p2, alpha*n))
        w.arc(rotate(p1, alpha*n), E, 1)
        w.arc(rotate(p0, alpha*n), R, 1)

        # rising gullet slope
        w.arc(rotate(p5, alpha*n), R, 1)
        w.line(rotate(p6, alpha*n))
        w.arc(rotate(p7, alpha*n), E, 0)
        w.arc(rotate(p8, alpha*n), F, 0)

    w.close()

    return w


def rotate(pt, rads):
    """
    rotate pt by rads radians about origin
    """
    sinA = sin(rads)
    cosA = cos(rads)
    return (pt[0] * cosA - pt[1] * sinA,
            pt[0] * sinA + pt[1] * cosA)



