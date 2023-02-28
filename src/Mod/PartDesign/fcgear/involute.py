# (c) 2014 David Douard <david.douard@gmail.com>
# (c) 2023 Jonas Bähr <jonas.baehr@web.de>
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

xrange = range


def CreateExternalGear(w, m, Z, phi,
        split=True,
        addCoeff=1.0, dedCoeff=1.25,
        filletCoeff=0.375):
    """
    Create an external gear

    w is wirebuilder object (in which the gear will be constructed)
    m is the gear's module (pitch diameter divided by the number of teeth)
    Z is the number of teeth
    phi is the gear's pressure angle
    addCoeff is the addendum coefficient (addendum normalized by module)
    dedCoeff is the dedendum coefficient (dedendum normalized by module)
    filletCoeff is the root fillet radius, normalized by the module.
        The default of 0.375 matches the hard-coded value (1.5 * 0.25) of the implementation
        up to v0.20. The ISO Rack specified 0.38, though.

    if split is True, each profile of a teeth will consist in 2 Bezier
    curves of degree 3, otherwise it will be made of one Bezier curve
    of degree 4
    """
    # ****** external gear specifications
    addendum = addCoeff * m         # distance from pitch circle to tip circle
    dedendum = dedCoeff * m         # pitch circle to root, sets clearance

    # Calculate radii
    Rpitch = Z * m / 2            # pitch circle radius
    Rb = Rpitch*cos(phi * pi / 180)  # base circle radius
    Ra = Rpitch + addendum    # tip (addendum) circle radius
    Rroot = Rpitch - dedendum # root circle radius
    fRad = filletCoeff * m  # fillet radius, max 1.5*clearance
    Rc = Rroot + fRad # radius at the center of the fillet circle
    Rf = Rc  # radius at top of fillet, assuming fillet below involute
    filletWithinInvolute = Rf > Rb # above the base circle we have the involute,
                                   # below we have a straight line towards the center.
    if (filletWithinInvolute and fRad > 0):
        # In this case we need tangency of the involute and the fillet circle.
        # It has to be somewhere between max(Rb,Rroot) and Rc.
        # So we need the radius r from the origin depending on the tangent angle, for both:
        # - the involute: ri(t) = Rb * sqrt(1 + t**2), rationale: see below.
        # - the fillet circle: a bit more complex, see below.
        # and then find the r where both tangents are equal.
        # As the tangent angle of the involute equals the parameter t, we can just take the
        # parametric polar representation of the involute as our first equation.
        # For a circle in parameter form, the tangent angle is pi/2 - t (for the first quadrant,
        # i.e. 0 <= t <= pi/2, and more than one quadrant is not of interest for us). Unfortunately,
        # the fillet circle is offset from the origin by (Rc,phi), so things becomes more messy:
        # rc(t) = sqrt((Rc*cos(phi) + fRad*cos(t))**2 + (Rc*sin(phi) + fRad*sin(t))**2)
        # To get rid of the sin(t) part we assume phi "very small", i.e. sin(phi) becomes 0.
        # This is justyfied because Rc is much larger than fRad and the parallax error is
        # neglectable. Next we substitute the parameter t by pi/2-q-pi. For one to account for the
        # tangent angle definitions (see above), and then to turn our cicle by 180° as for the
        # root fillet we need the third quadrant of the circle (NB: we are looking at the upper
        # half of the right most tooth, i.e. the involute grows downwards!). This results in
        # sqrt(2*fRad*Rc*cos(-1/2*pi - q) + fRad**2 + Rc**2) which simplifies to
        # rc(q) = sqrt(-2*fRad*Rc*sin(q) + fRad**2 + Rc**2)
        # which is the polar r for a given tangent angle q (with the simplification of phi being 0)
        # This can now be inverted to give the tangent angle at a given r:
        # qc(r) = asin((-r**2 + fRad**2 + Rc**2)/(2*fRad*Rc))
        # For the involute we have ri(q) = Rb * sqrt(1 + q**2), thus
        # qi(r) = +- sqrt(r**2 - Rb**2)/Rb
        # Now to find the r where the tangents match, our Rf, we set qc=qi and solve for r.
        # This doesn't seem to have an analytical solution, though, so let's apply
        # Newton's Method to find the root of q := qi-qc over Rb...Rc, or for larger number of
        # teeth, Rroot...Rc as in this case the base circle is below the root circle.
        # The graph of q is strictly monotnonous and very steep, no surprises to expect.
        # To simplify the above equations and to find the derivative, SageMath was used.
        q = lambda r: (sqrt(r**2 - Rb**2) / Rb
            - asin((-r**2 + fRad**2 + Rc**2) / (2 * fRad * Rc)))
        q_prime = lambda r: (r / (sqrt(-Rb**2 + r**2) * Rb)
            + r / (fRad * Rc * sqrt(1 - 1/4 * (r**2 - fRad**2 - Rc**2)**2 / (fRad**2 * Rc**2))))

        Rf = findRootNewton(q, q_prime, x_min=max(Rb, Rroot), x_max=Rc)


    # ****** calculate angles (all in radians)
    pitchAngle = 2 * pi / Z  # angle subtended by whole tooth (rads)
    baseToPitchAngle = genInvolutePolar(Rb, Rpitch)
    pitchToFilletAngle = baseToPitchAngle  # profile starts at base circle
    if (filletWithinInvolute): # start profile at top of fillet
        pitchToFilletAngle -= genInvolutePolar(Rb, Rf)

    filletWidth = sqrt(fRad**2 - (Rc - Rf)**2)
    filletAngle = atan(filletWidth / Rf)

    # ****** generate Higuchi involute approximation
    fe = 1       # fraction of profile length at end of approx
    fs = 0.01    # fraction of length offset from base to avoid singularity
    if (filletWithinInvolute):
        fs = (Rf**2 - Rb**2) / (Ra**2 - Rb**2)  # offset start to top of fillet

    if split:
        # approximate in 2 sections, split 25% along the involute
        fm = fs + (fe - fs) / 4   # fraction of length at junction (25% along profile)
        dedInv = BezCoeffs(Rb, Ra, 3, fs, fm)
        addInv = BezCoeffs(Rb, Ra, 3, fm, fe)

        # join the 2 sets of coeffs (skip duplicate mid point)
        inv = dedInv + addInv[1:]
    else:
        inv = BezCoeffs(Rb, Ra, 4, fs, fe)

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

    # Make sure we begin *exactly* where our last curve ends.
    # In theory start == rotate(end, angle_of_last_tooth), but in practice we have limited
    # precision. Especially if we don't have a fillet, we end at rootNext, not filletNext.
    # And even though these two should also be equal, they are calculated differently,
    # which is enough for the resulting wire not being closed any more.
    # So to be on the save side, we begin at rotate(end, angle_of_last_tooth), not start.
    if fRad > 0:
        w.move(rotate(filletNext, thetas[-1])) # start at top of front profile
    else:
        w.move(rotate(rootNext, thetas[-1])) # start at top of front profile

    for theta in thetas:
        w.theta = theta
        if (not filletWithinInvolute):
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

        if (not filletWithinInvolute):
            w.line(filletR) # line down to topof fillet

        if (rootNext[1] > rootR[1]):    # is there a section of root circle between fillets?
            if fRad > 0:
                w.arc(rootR, fRad, 0) # back fillet
            w.arc(rootNext, Rroot, 1) # root circle arc

        if fRad > 0:
            w.arc(filletNext, fRad, 0)

    w.close()
    return w

def CreateInternalGear(w, m, Z, phi,
        split=True,
        addCoeff=0.6, dedCoeff=1.25,
        filletCoeff=0.375):
    """
    Create an internal gear

    w is wirebuilder object (in which the gear will be constructed)
    m is the gear's module (pitch diameter divided by the number of teeth)
    Z is the number of teeth
    phi is the gear's pressure angle
    addCoeff is the addendum coefficient (addendum normalized by module)
        The default of 0.6 comes from the "Handbook of Gear Design" by Gitin M. Maitra,
        with the goal to push the addendum circle beyond the base circle to avoid non-involute
        flanks on the tips.
        It in turn assumes, however, that the mating pinion usaes a larger value of 1.25.
        And it's only required for a small number of teeth and/or a relatively large mating gear.
        Anyways, it's kept here as this was the hard-coded value of the implementation up to v0.20.
    dedCoeff is the dedendum coefficient (dedendum normalized by module)
    filletCoeff is the root fillet radius, normalized by the module.
        The default of 0.375 matches the hard-coded value (1.5 * 0.25) of the implementation
        up to v0.20. The ISO Rack specified 0.38, though.

    if split is True, each profile of a teeth will consist in 2 Bezier
    curves of degree 3, otherwise it will be made of one Bezier curve
    of degree 4
    """
    # ****** external gear specifications
    addendum = addCoeff * m         # distance from pitch circle to tip circle
    dedendum = dedCoeff * m         # pitch circle to root, sets clearance

    # Calculate radii
    Rpitch = Z * m / 2              # pitch circle radius
    Rb = Rpitch*cos(phi * pi / 180) # base circle radius
    Ra = Rpitch - addendum          # tip (addendum) circle radius
    Rroot = Rpitch + dedendum       # root circle radius
    fRad = filletCoeff * m          # fillet radius, max 1.5*clearance
    Rc = Rroot - fRad      # radius at the center of the fillet circle

    tipWithinInvolute = Ra > Rb     # above the base circle we have the involute,
                                    # below we have a straight line towards the center.

    # Calculate Rf: The radius where the involute ends and the fillet begins.
    # For a detailed explanation see the external gear.
    # Here, however, we substitute t with pi/2-q, i.e. we omit the extra -pi, as we are now
    # interested in the first quadrant: Again, our involute grows downwards but now the fillet
    # circle needs to approach the involute "from inside the tooth" and at the far end of the
    # involute. The fillet is now
    # rc(q) = sqrt(2*fRad*Rc*sin(q) + fRad**2 + Rc**2)
    # which can be inversed to
    # qc(r) = asin((r**2 - fRad**2 - Rc**2)/(2*fRad*Rc))
    # The involute does not change in regard to the external gear.
    # However, the simplification of assuming phi=0 is more questionable here as phi doesn't have
    # an upper bound from fRad any more but gets larger as the involute angle grows. Having a
    # non-zero phi in the original rc(q) makes solving impossible, so lets apply another trick:
    # If we cannot apply a polar angle to the position of the circle, we could do it for the
    # involute. Such a rotation doesn't have any influence on ri, so not on qi, but changes the
    # interpretation of it: The tangent angle of the involute experiences the same rotation as the
    # involute itself. So its is just a simple offset: Our q(r) becomes qi(r) - qc(i) - phi_corr,
    # where phi_corr is the amount we (virtually) turn our involute around the origin.
    if (fRad > 0):
        phi_corr = genInvolutePolar(Rb, Rroot) + atan(fRad / Rroot)
        q = lambda r: (sqrt(r**2 - Rb**2) / Rb
            - asin((r**2 - fRad**2 - Rc**2) / (2 * fRad * Rc))
            - phi_corr)
        q_prime = lambda r: (r / (sqrt(-Rb**2 + r**2) * Rb)
            - r / (fRad * Rc * sqrt(1 - 1/4 * (r**2 - fRad**2 - Rc**2)**2 / (fRad**2 * Rc**2))))
        Rf = findRootNewton(q, q_prime, x_min=max(Rb, Rc), x_max=Rroot)
    else:
        Rf = Rroot # no fillet


    # ****** calculate angles (all in radians)
    pitchAngle = 2 * pi / Z  # angle subtended by whole tooth (rads)
    baseToPitchAngle = genInvolutePolar(Rb, Rpitch)
    tipToPitchAngle = baseToPitchAngle
    if (tipWithinInvolute):  # start profile at tip, not base
        tipToPitchAngle -= genInvolutePolar(Rb, Ra)
    pitchToFilletAngle = genInvolutePolar(Rb, Rf) - baseToPitchAngle;
    filletWidth = sqrt(fRad**2 - (Rf - Rc)**2)
    filletAngle = atan(filletWidth / Rf)

    # ****** generate Higuchi involute approximation
    fe = 1       # fraction of profile length at end of approx
    fs = 0.01    # fraction of length offset from base to avoid singularity
    if (tipWithinInvolute):
        fs = (Ra**2 - Rb**2) / (Rf**2 - Rb**2)  # offset start to tip

    if split:
        # approximate in 2 sections, split 25% along the involute
        fm = fs + (fe - fs) / 4   # fraction of length at junction (25% along profile)
        addInv = BezCoeffs(Rb, Rf, 3, fs, fm)
        dedInv = BezCoeffs(Rb, Rf, 3, fm, fe)

        # join the 2 sets of coeffs (skip duplicate mid point)
        invR = addInv + dedInv[1:]
    else:
        invR = BezCoeffs(Rb, Rf, 4, fs, fe)

    # create the back profile of tooth (mirror image)
    inv = []
    for i, pt in enumerate(invR):
        # rotate involute to put center of tooth at y = 0
        ptx, pty = invR[i] = rotate(pt,  pitchAngle / 4 - baseToPitchAngle)
        # generate the back of tooth profile nodes, flip Y coords
        inv.append((ptx, -pty))

    # ****** calculate section junction points R=back of tooth, Next=front of next tooth)
    #fillet = inv[6] # top of fillet, front of tooth   #toCartesian(Rf, -pitchAngle / 4 - pitchToFilletAngle) # top of fillet
    fillet = [ptx,-pty]
    tip = toCartesian(Ra, -pitchAngle/4+tipToPitchAngle)  # tip, front of tooth
    tipR = [ tip[0], -tip[1] ]
    #filletR = [fillet[0], -fillet[1]]   # flip to make same point on back of tooth
    rootR = toCartesian(Rroot, pitchAngle / 4 + pitchToFilletAngle + filletAngle)
    rootNext = toCartesian(Rroot, 3 * pitchAngle / 4 - pitchToFilletAngle - filletAngle)
    filletNext = rotate(fillet, pitchAngle)  # top of fillet, front of next tooth

    # Build the shapes using FreeCAD.Part
    t_inc = 2.0 * pi / float(Z)
    thetas = [(x * t_inc) for x in range(Z)]

    # Make sure we begin *exactly* where our last curve ends.
    # In theory start == rotate(end, angle_of_last_tooth), but in practice we have limited
    # precision. Especially if we don't have a fillet, we end at rootNext, not filletNext.
    # And even though these two should also be equal, they are calculated differently,
    # which is enough for the resulting wire not being closed any more.
    # So to be on the save side, we begin at rotate(end, angle_of_last_tooth), not start.
    if fRad > 0:
        w.move(rotate(filletNext, thetas[-1])) # start at top of front profile
    else:
        w.move(rotate(rootNext, thetas[-1])) # start at top of front profile

    for theta in thetas:
        w.theta = theta
        if split:
            w.curve(inv[5], inv[4], inv[3])
            w.curve(inv[2], inv[1], inv[0])
        else:
            w.curve(*inv[-2::-1])

        if (not tipWithinInvolute):
            w.line(tip) # line from tip down to base circle

        w.arc(tipR, Ra, 1) # arc across addendum circle

        if (not tipWithinInvolute):
            w.line(invR[0]) # line up to the base circle

        if split:
            w.curve(invR[1], invR[2], invR[3])
            w.curve(invR[4], invR[5], invR[6])
        else:
            w.curve(*invR[1:])

        if (rootNext[1] > rootR[1]):    # is there a section of root circle between fillets?
            if fRad > 0:
                w.arc(rootR, fRad, 1) # back fillet
            w.arc(rootNext, Rroot, 1) # root circle arc

        if fRad > 0:
            w.arc(filletNext, fRad, 1)


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

def findRootNewton(f, f_prime, x_min, x_max):
    """Appy Newton's Method to find the root of f within x_min and x_max
    We assume that there is a root in that range and that f is strictly monotonic,
    i.e. we don't take precautions for overshooting beyond the input range.
    """
    # As initial guess let's take the middle of our input range
    x = (x_min + x_max) / 2

    # FreeCAD.Base.Precision.intersection() is 1e-9, but file doesn't depend on FreeCAD,
    # a property very handy when testing in isolation, so let's keep it.
    PRECISION_INTERSECTION = 1e-9

    # Experience has shown that usually 2-3 iterations are enough, but better save than sorry
    for i in range(6):
        f_x = f(x)
        if abs(f_x) < PRECISION_INTERSECTION:
            return x
        x = x - f_x/f_prime(x)

    raise RuntimeError(f"No convergence after {i+1} iterations.")

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


def BezCoeffs(baseRadius, limitRadius, order, fstart, fstop):
    """Approximates an involute using a Bezier-curve

    Parameters:
    baseRadius - the radius of base circle of the involute.
        This is where the involute starts, too.
    limitRadius - the radius of an outer circle, where the involute ends.
    order - the order of the Bezier curve to be fitted e.g. 3, 4, 5, ...
    fstart - fraction of distance along the involute to start the approximation.
    fstop - fraction of distance along the involute to stop the approximation.
    """
    Rb = baseRadius
    Ra = limitRadius
    ta = sqrt(Ra * Ra - Rb * Rb) / Rb   # involute angle at the limit radius
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

