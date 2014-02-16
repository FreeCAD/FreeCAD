# (c) 2014 David Douard <david.douard@gmail.com>
# Based on https://github.com/attoparsec/inkscape-extensions.git
# Based on gearUtils-03.js by Dr A.R.Collins
#          http://www.arc.id.au/gearDrawing.html
#
# Calculation of Bezier coefficients for
# Higuchi et al. approximation to an involute.
# ref: YNU Digital Eng Lab Memorandum 05-1
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

from math import cos, sin, pi, acos, asin, atan, sqrt

def CreateExternalGear(w, m, Z, phi, split=True):
    """
    Create an external gear

    w is wirebuilder object (in which the gear will be constructed)

    if split is True, each profile of a teeth will consist in 2 Bezier
    curves of degree 3, otherwise it will be made of one Bezier curve
    of degree 4
    """
    # ****** external gear specifications
    addendum = m              # distance from pitch circle to tip circle
    dedendum = 1.25 * m         # pitch circle to root, sets clearance
    clearance = dedendum - addendum

    # Calculate radii
    Rpitch = Z * m / 2            # pitch circle radius
    Rb = Rpitch*cos(phi * pi / 180)  # base circle radius
    Ra = Rpitch + addendum    # tip (addendum) circle radius
    Rroot = Rpitch - dedendum # root circle radius
    fRad = 1.5 * clearance # fillet radius, max 1.5*clearance
    Rf = sqrt((Rroot + fRad)**2 - fRad**2) # radius at top of fillet
    if (Rb < Rf):
        Rf = Rroot + clearance

    # ****** calculate angles (all in radians)
    pitchAngle = 2 * pi / Z  # angle subtended by whole tooth (rads)
    baseToPitchAngle = genInvolutePolar(Rb, Rpitch)
    pitchToFilletAngle = baseToPitchAngle  # profile starts at base circle
    if (Rf > Rb):         # start profile at top of fillet (if its greater)
        pitchToFilletAngle -= genInvolutePolar(Rb, Rf)

    filletAngle = atan(fRad / (fRad + Rroot))  # radians

    # ****** generate Higuchi involute approximation
    fe = 1       # fraction of profile length at end of approx
    fs = 0.01    # fraction of length offset from base to avoid singularity
    if (Rf > Rb):
        fs = (Rf**2 - Rb**2) / (Ra**2 - Rb**2)  # offset start to top of fillet

    if split:
        # approximate in 2 sections, split 25% along the involute
        fm = fs + (fe - fs) / 4   # fraction of length at junction (25% along profile)
        dedInv = BezCoeffs(m, Z, phi, 3, fs, fm)
        addInv = BezCoeffs(m, Z, phi, 3, fm, fe)

        # join the 2 sets of coeffs (skip duplicate mid point)
        inv = dedInv + addInv[1:]
    else:
        inv = BezCoeffs(m, Z, phi, 4, fs, fe)

    # create the back profile of tooth (mirror image)
    invR = []
    for i, pt in enumerate(inv):
        # rotate all points to put pitch point at y = 0
        ptx, pty = inv[i] = rotate(pt, -baseToPitchAngle - pitchAngle / 4)
        # generate the back of tooth profile nodes, mirror coords in X axis
        invR.append((ptx, -pty))

    # ****** calculate section junction points R=back of tooth, Next=front of next tooth)
    fillet = toCartesian(Rf, -pitchAngle / 4 - pitchToFilletAngle) # top of fillet
    filletR = [fillet[0], -fillet[1]]   # flip to make same point on back of tooth
    rootR = toCartesian(Rroot, pitchAngle / 4 + pitchToFilletAngle + filletAngle)
    rootNext = toCartesian(Rroot, 3 * pitchAngle / 4 - pitchToFilletAngle - filletAngle)
    filletNext = rotate(fillet, pitchAngle)  # top of fillet, front of next tooth

    # Build the shapes using FreeCAD.Part
    t_inc = 2.0 * pi / float(Z)
    thetas = [(x * t_inc) for x in range(Z)]

    w.move(fillet) # start at top of fillet

    for theta in thetas:
        w.theta = theta
        if (Rf < Rb):
            w.line(inv[0]) # line from fillet up to base circle

        if split:
            w.curve(inv[1], inv[2], inv[3])
            w.curve(inv[4], inv[5], inv[6])
            w.arc(invR[6], Ra, 1) # arc across addendum circle
            w.curve(invR[5], invR[4], invR[3])
            w.curve(invR[2], invR[1], invR[0])
        else:
            w.curve(*inv[1:])
            w.arc(invR[-1], Ra, 1) # arc across addendum circle
            w.curve(*invR[-2::-1])

        if (Rf < Rb):
            w.line(filletR) # line down to topof fillet

        if (rootNext[1] > rootR[1]):    # is there a section of root circle between fillets?
            w.arc(rootR, fRad, 0) # back fillet
            w.arc(rootNext, Rroot, 1) # root circle arc

        w.arc(filletNext, fRad, 0)

    w.close()
    return w



def genInvolutePolar(Rb, R):
    """returns the involute angle as function of radius R.
    Rb = base circle radius
    """
    return (sqrt(R*R - Rb*Rb) / Rb) - acos(Rb / R)


def rotate(pt, rads):
    "rotate pt by rads radians about origin"
    sinA = sin(rads)
    cosA = cos(rads)
    return (pt[0] * cosA - pt[1] * sinA,
            pt[0] * sinA + pt[1] * cosA)



def toCartesian(radius, angle):
    "convert polar coords to cartesian"
    return [radius * cos(angle), radius * sin(angle)]


def chebyExpnCoeffs(j, func):
    N = 50      # a suitably large number  N>>p
    c = 0
    for k in xrange(1, N + 1):
        c += func(cos(pi * (k - 0.5) / N)) * cos(pi * j * (k - 0.5) / N)
    return 2 *c / N


def chebyPolyCoeffs(p, func):
    coeffs = [0]*(p+1)
    fnCoeff = []
    T = [coeffs[:] for i in range(p+1)]
    T[0][0] = 1
    T[1][1] = 1
    # now generate the Chebyshev polynomial coefficient using
    # formula T(k+1) = 2xT(k) - T(k-1) which yields
    # T = [ [ 1,  0,  0,  0,  0,  0],    # T0(x) =  +1
    #       [ 0,  1,  0,  0,  0,  0],    # T1(x) =   0  +x
    #       [-1,  0,  2,  0,  0,  0],    # T2(x) =  -1  0  +2xx
    #       [ 0, -3,  0,  4,  0,  0],    # T3(x) =   0 -3x    0   +4xxx
    #       [ 1,  0, -8,  0,  8,  0],    # T4(x) =  +1  0  -8xx       0  +8xxxx
    #       [ 0,  5,  0,-20,  0, 16],    # T5(x) =   0  5x    0  -20xxx       0  +16xxxxx
    #     ...                     ]

    for k in xrange(1, p):
        for j in xrange(len(T[k]) - 1):
            T[k + 1][j + 1] = 2 * T[k][j]
        for j in xrange(len(T[k - 1])):
            T[k + 1][j] -= T[k - 1][j]

    # convert the chebyshev function series into a simple polynomial
    # and collect like terms, out T polynomial coefficients
    for k in xrange(p + 1):
        fnCoeff.append(chebyExpnCoeffs(k, func))

    for k in xrange(p + 1):
        for pwr in xrange(p + 1):
            coeffs[pwr] += fnCoeff[k] * T[k][pwr]

    coeffs[0] -= fnCoeff[0] / 2  # fix the 0th coeff
    return coeffs


def binom(n, k):
    coeff = 1
    for i in xrange(n - k + 1, n + 1):
        coeff *= i

    for i in xrange(1, k + 1):
        coeff /= i

    return coeff


def bezCoeff(i, p, polyCoeffs):
    '''generate the polynomial coeffs in one go'''
    return sum(binom(i, j) * polyCoeffs[j] / binom(p, j) for j in range(i+1))


    # Parameters:
    # module - sets the size of teeth (see gear design texts)
    # numTeeth - number of teeth on the gear
    # pressure angle - angle in degrees, usually 14.5 or 20
    # order - the order of the Bezier curve to be fitted [3, 4, 5, ..]
    # fstart - fraction of distance along tooth profile to start
    # fstop - fraction of distance along profile to stop
def BezCoeffs(module, numTeeth, pressureAngle, order, fstart, fstop):
    Rpitch = module * numTeeth / 2       # pitch circle radius
    phi = pressureAngle        # pressure angle
    Rb = Rpitch * cos(phi * pi / 180) # base circle radius
    Ra = Rpitch + module               # addendum radius (outer radius)
    ta = sqrt(Ra * Ra - Rb * Rb) / Rb   # involute angle at addendum
    te = sqrt(fstop) * ta          # involute angle, theta, at end of approx
    ts = sqrt(fstart) * ta         # involute angle, theta, at start of approx
    p = order                     # order of Bezier approximation

    def involuteXbez(t):
        "Equation of involute using the Bezier parameter t as variable"
        # map t (0 <= t <= 1) onto x (where -1 <= x <= 1)
        x = t * 2 - 1
        # map theta (where ts <= theta <= te) from x (-1 <=x <= 1)
        theta = x * (te - ts) / 2 + (ts + te) / 2
        return Rb * (cos(theta) + theta * sin(theta))

    def involuteYbez(t):
        "Equation of involute using the Bezier parameter t as variable"
        # map t (0 <= t <= 1) onto x (where -1 <= x <= 1)
        x = t * 2 - 1
        # map theta (where ts <= theta <= te) from x (-1 <=x <= 1)
        theta = x * (te - ts) / 2 + (ts + te) / 2
        return Rb * (sin(theta) - theta * cos(theta))

    # calc Bezier coeffs
    bzCoeffs = []
    polyCoeffsX = chebyPolyCoeffs(p, involuteXbez)
    polyCoeffsY = chebyPolyCoeffs(p, involuteYbez)
    for i in xrange(p + 1):
        bx = bezCoeff(i, p, polyCoeffsX)
        by = bezCoeff(i, p, polyCoeffsY)
        bzCoeffs.append((bx, by))
    return bzCoeffs

