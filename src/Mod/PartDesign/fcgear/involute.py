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

from math import cos, sin, tan, pi, acos, asin, atan, sqrt, radians
from math import comb as binom


def CreateExternalGear(w, m, Z, phi,
        split=True,
        addCoeff=1.0, dedCoeff=1.25,
        filletCoeff=0.375,
        shiftCoeff=0.0):
    """
    Create an external gear

    w is wire builder object (in which the gear will be constructed)
    m is the gear's module (pitch diameter divided by the number of teeth)
    Z is the number of teeth
    phi is the gear's pressure angle, in degrees
    addCoeff is the addendum coefficient (addendum normalized by module)
    dedCoeff is the dedendum coefficient (dedendum normalized by module)
    filletCoeff is the root fillet radius, normalized by the module.
        The default of 0.375 matches the hard-coded value (1.5 * 0.25) of the implementation
        up to v0.20. The ISO Rack specified 0.38, though.
    shiftCoeff is the profile shift coefficient (profile shift normalized by module)

    if split is True, each profile of a teeth will consist in 2 Bezier
    curves of degree 3, otherwise it will be made of one Bezier curve
    of degree 4
    """
    _create_involute_profile(
        wire_builder=w,
        module=m,
        number_of_teeth=Z,
        pressure_angle=radians(phi),
        split_involute=split,
        outer_height_coefficient=addCoeff,
        inner_height_coefficient=dedCoeff,
        inner_fillet_coefficient=filletCoeff,
        profile_shift_coefficient=shiftCoeff)


def CreateInternalGear(w, m, Z, phi,
        split=True,
        addCoeff=0.6, dedCoeff=1.25,
        filletCoeff=0.375,
        shiftCoeff=0.0):
    """
    Create an internal gear

    w is wire builder object (in which the gear will be constructed)
    m is the gear's module (pitch diameter divided by the number of teeth)
    Z is the number of teeth
    phi is the gear's pressure angle, in degrees
    addCoeff is the addendum coefficient (addendum normalized by module)
        The default of 0.6 comes from the "Handbook of Gear Design" by Gitin M. Maitra,
        with the goal to push the addendum circle beyond the base circle to avoid non-involute
        flanks on the tips.
        It in turn assumes, however, that the mating pinion uses a larger value of 1.25.
        And it's only required for a small number of teeth and/or a relatively large mating gear.
        Anyways, it's kept here as this was the hard-coded value of the implementation up to v0.20.
    dedCoeff is the dedendum coefficient (dedendum normalized by module)
    filletCoeff is the root fillet radius, normalized by the module.
        The default of 0.375 matches the hard-coded value (1.5 * 0.25) of the implementation
        up to v0.20. The ISO Rack specified 0.38, though.
    shiftCoeff is the profile shift coefficient (profile shift normalized by module)

    if split is True, each profile of a teeth will consist in 2 Bezier
    curves of degree 3, otherwise it will be made of one Bezier curve
    of degree 4
    """
    _create_involute_profile(
        wire_builder=w,
        module=m,
        number_of_teeth=Z,
        pressure_angle=radians(phi),
        split_involute=split,
        rotation=pi/Z, # rotate by half a tooth to align the "inner" tooth with the X-axis
        outer_height_coefficient=dedCoeff,
        inner_height_coefficient=addCoeff,
        outer_fillet_coefficient=filletCoeff,
        profile_shift_coefficient=shiftCoeff)


def _create_involute_profile(
        wire_builder,
        module,
        number_of_teeth,
        pressure_angle=radians(20.0),
        split_involute=True,
        rotation=radians(0),
        outer_height_coefficient=1.0,
        inner_height_coefficient=1.0,
        outer_fillet_coefficient=0.0,
        inner_fillet_coefficient=0.0,
        profile_shift_coefficient=0.0):
    """
    Create an involute gear profile in the given wire builder

    This method can be used to create external as well as internal gear and spline profiles.
    Thus this method does not use the terms "addednum" and "dedendum" or "tip" and "root",
    but refers to the elements below the reference circle (i.e. towards the center) as "inner",
    and those above the reference circle (i.e. away from the center) as "outer".

    For an external gear, outer_height is the addendum, inner_height is the dedendum,
    and inner_fillet is the root fillet.
    For an internal gear, inner_height is the addendum, outer_height is the dedendum,
    and outer_fillet is the root fillet.

    The "_coefficient" suffix denotes values normalized by the module.
    """

    profile_shift = profile_shift_coefficient * module
    outer_height = outer_height_coefficient * module + profile_shift # ext: addendum, int: dedednum
    inner_height = inner_height_coefficient * module - profile_shift # ext: dedendum, int: addednum

    # ****** calculate radii
    # All distances from the center of the profile start with "R".
    # As those are mostly used in mathematical formulae, we use "symbols" rather than "speaking
    # names" to keep them short. Otherwise the math becomes unreadable.
    Rref = number_of_teeth * module / 2 # reference circle radius
    Rb = Rref * cos(pressure_angle) # base circle radius
    Ro = Rref + outer_height # radius of outer circle (tip for external, root for internal gears)
    Ri = Rref - inner_height # radius of inner circle (root for external, tip for internal gears)

    fi = inner_fillet_coefficient * module # fillet radius at inner circle (ext: root fillet)
    Rci = Ri + fi # radius at which the center of the inner fillet circle sits
    Rfi = Rci # radius at which the inner fillet ends (for now assuming some non-involute flank)

    fo = outer_fillet_coefficient * module # fillet radius at outer circle (int: root fillet)
    Rco = Ro - fo # radius at which the center of the outer fillet circle sits
    Rfo = Ro # radius at which the outer fillet ends (for now assuming no outer fillet)

    has_non_involute_flank = Rfi < Rb # above the base circle we have the involute,
                                      # below we have a straight line towards the center.
    has_inner_fillet = fi > 0
    has_outer_fillet = fo > 0

    if has_inner_fillet and not has_non_involute_flank:
        # In this case we need tangency of the involute's start and the inner fillet circle.
        # It has to be somewhere between max(Rb,Ri) and Rci.
        # So we need the radius R from the origin depending on the tangent angle, for both:
        # - the involute: Rinv(t) = Rb * sqrt(1 + t**2), rationale: see below.
        # - the fillet circle: Rcirc(t) = ... a bit more complex, see below.
        # and then find the R where both tangents are equal.
        # As the tangent angle of the involute equals the parameter t, we can just take the
        # parametric polar representation of the involute as our first equation.
        # For a circle in parameter form, the tangent angle is pi/2 - t (for the first quadrant,
        # i.e. 0 <= t <= pi/2, and more than one quadrant is not of interest for us). Unfortunately,
        # the fillet circle is offset from the origin by (Rci,phi), so things becomes more messy:
        # Rcirc(t) = sqrt((Rci*cos(phi) + fi*cos(t))**2 + (Rci*sin(phi) + fi*sin(t))**2)
        # To get rid of the sin(t) part we assume phi "very small", i.e. sin(phi) becomes 0.
        # This is justyfied because Rci is much larger than fi and the parallax error is
        # neglectable. Next we substitute the parameter t by pi/2-q-pi. For one to account for the
        # tangent angle definitions (see above), and then to turn our circle by 180° as for the
        # inner fillet we need the third quadrant of the circle (NB: we are looking at the upper
        # half of the right most tooth, i.e. the involute grows downwards!). This results in
        # sqrt(2*fi*Rci*cos(-1/2*pi - q) + fi**2 + Rci**2) which simplifies to
        # Rcirc(q) = sqrt(-2*fi*Rci*sin(q) + fi**2 + Rci**2)
        # which is the polar r for a given tangent angle q (with the simplification of phi being 0)
        # This can now be inverted to give the tangent angle at a given r:
        # Qcirc(r) = asin((-r**2 + fi**2 + Rci**2)/(2*fi*Rci))
        # For the involute we have Rinv(q) = Rb * sqrt(1 + q**2), with q=t as the circle is already
        # adapted, thus: Qinv(r) = +- sqrt(r**2 - Rb**2)/Rb
        # Now to find the r where the tangents match, our Rfi, we set Qcirc=Qinv and solve for r.
        # This doesn't seem to have an analytical solution, though, so let's apply Newton's
        # Method to find the root of q := Qinv - Qcirc over Rb...Rci, or for larger number
        # of teeth, Ri...Rci as in this case the base circle is below the inner circle.
        # The graph of q is strictly monotnonous and very steep, no surprises to expect.
        # To simplify the above equations and to find the derivative, SageMath was used.
        q = lambda r: (sqrt(r**2 - Rb**2) / Rb
            - asin((-r**2 + fi**2 + Rci**2) / (2 * fi * Rci)))
        q_prime = lambda r: (r / (sqrt(-Rb**2 + r**2) * Rb)
            + r / (fi * Rci * sqrt(1 - 1/4 * (r**2 - fi**2 - Rci**2)**2 / (fi**2 * Rci**2))))
        Rfi = findRootNewton(q, q_prime, x_min=max(Rb, Ri), x_max=Rci)

    if has_outer_fillet:
        # In this case we need tangency of the involute's end and the outer fillet circle.
        # For a detailed explanation refer to the inner fillet above.
        # Here, however, we substitute t with pi/2-q, i.e. we omit the extra -pi, as we are now
        # interested in the first quadrant: Again, our involute grows downwards but now the fillet
        # circle needs to approach the involute "from inside the tooth" and at the far end of the
        # involute. The fillet is now
        # Rcirc(q) = sqrt(2*fo*Rco*sin(q) + fo**2 + Rco**2)
        # which can be inversed to
        # Qcirc(r) = asin((r**2 - fo**2 - Rco**2)/(2*fo*Rco))
        # The involute is the very same as for the inner fillet.
        # However, the simplification of assuming phi=0 is more questionable here as phi doesn't
        # have an upper bound from the fillet's radius any more but gets larger as the involute
        # angle grows. Having a non-zero phi in the original Rcirc(q) makes solving impossible,
        # so lets apply another trick:
        # If we cannot apply a polar angle to the position of the circle, we could do it for the
        # involute. Such a rotation doesn't have any influence on Rinv, so not on Qinv, but changes
        # the interpretation of it: The tangent angle of the involute experiences the same rotation
        # as the involute itself. So it is just a simple offset:
        # Our q(r) becomes Qinv(r) - Qcirc(r) - phi_corr, where phi_corr is the amount we
        # (virtually) rotate our involute around the origin. To bring the fillet circle back on
        # X-axis, we assume the (real) polar angle of its center position being fo below the
        # involute's end at Ro.
        phi_corr = genInvolutePolar(Rb, Ro) + atan(fo / Ro)
        q = lambda r: (sqrt(r**2 - Rb**2) / Rb
            - asin((r**2 - fo**2 - Rco**2) / (2 * fo * Rco))
            - phi_corr)
        q_prime = lambda r: (r / (sqrt(-Rb**2 + r**2) * Rb)
            - r / (fo * Rco * sqrt(1 - 1/4 * (r**2 - fo**2 - Rco**2)**2 / (fo**2 * Rco**2))))
        Rfo = findRootNewton(q, q_prime, x_min=max(Rb, Rco), x_max=Ro)

    # ****** calculate angles (all in radians)
    angular_pitch = 2 * pi / number_of_teeth # angle subtended by complete tooth/space pair
    base_to_ref = genInvolutePolar(Rb, Rref) # angle between base and reference circle
    ref_to_stop = genInvolutePolar(Rb, Rfo) - base_to_ref # angle between ref and involute end
    if has_non_involute_flank: # involute starts at base circle
        start_to_ref = base_to_ref
    else: # involute starts at top of inner fillet, i.e. somewhat above the base circle
        start_to_ref = base_to_ref - genInvolutePolar(Rb, Rfi)

    inner_fillet_width = sqrt(fi**2 - (Rci - Rfi)**2)
    inner_fillet_angle = atan(inner_fillet_width / Rfi)
    outer_fillet_width = sqrt(fo**2 - (Rfo - Rco)**2)
    outer_fillet_angle = atan(outer_fillet_width / Rfo)

    # ****** generate Higuchi involute approximation
    fe = 1       # fraction of involute length at end of approx
    fs = 0.01    # fraction of length offset from base to avoid singularity
    if (not has_non_involute_flank):
        fs = (Rfi**2 - Rb**2) / (Rfo**2 - Rb**2)  # offset start to top of fillet

    if split_involute:
        # approximate in 2 sections, split 25% along the involute
        fm = fs + (fe - fs) / 4   # fraction of length at junction (25% along involute)
        part1 = BezCoeffs(Rb, Rfo, 3, fs, fm)
        part2 = BezCoeffs(Rb, Rfo, 3, fm, fe)
        inv = part1 + part2[1:] # join the 2 sets of coeffs (skip duplicate mid point)
    else:
        inv = BezCoeffs(Rb, Rfo, 4, fs, fe)

    # ****** calculate angular tooth thickness at reference circle
    enlargement_by_shift = profile_shift * tan(pressure_angle) / Rref
    tooth_thickness_half_angle = angular_pitch / 4 + enlargement_by_shift
    psi = tooth_thickness_half_angle # for the formulae below, a symbol is more readable

    # rotate all points to make the tooth symmetric to the X axis
    inv = [rotate(pt, -base_to_ref - psi) for pt in inv]

    # create the back profile of tooth (mirror image on X axis)
    invR = [mirror(pt) for pt in inv]

    # ****** calculate section junction points.
    # Those are the points where the named element ends (start is the end of the previous element).
    # Suffix _back is back of this tooth, suffix _next is front of next tooth.
    inner_fillet = toCartesian(Rfi, -psi - start_to_ref) # top of fillet
    inner_fillet_back = mirror(inner_fillet) # flip to make same point on back of tooth
    inner_circle_back = toCartesian(Ri, psi + start_to_ref + inner_fillet_angle)
    inner_circle_next = toCartesian(Ri, angular_pitch - psi - start_to_ref - inner_fillet_angle)
    inner_fillet_next = rotate(inner_fillet, angular_pitch)  # top of fillet, front of next tooth
    outer_fillet = toCartesian(Ro, -psi + ref_to_stop + outer_fillet_angle)
    outer_circle = mirror(outer_fillet)

    # ****** build the gear profile using the provided wire builder
    thetas = [x * angular_pitch + rotation for x in range(number_of_teeth)]

    # Make sure we begin *exactly* where our last curve ends.
    # In theory start == rotate(end, angle_of_last_tooth), but in practice we have limited
    # precision. Especially if we don't have an inner fillet, we end at inner_circle_next,
    # not inner_fillet_next.
    # And even though these two should also be equal (if no inner fillet), they are calculated
    # differently, which is enough for the resulting wire not being closed any more.
    # So to be on the save side, we begin at rotate(end, angle_of_last_tooth), not start.
    if has_inner_fillet:
        wire_builder.move(rotate(inner_fillet_next, thetas[-1]))
    else:
        wire_builder.move(rotate(inner_circle_next, thetas[-1]))

    for theta in thetas:
        wire_builder.theta = theta

        if (has_non_involute_flank):
            wire_builder.line(inv[0]) # line from inner fillet up to base circle

        # front involute
        if split_involute:
            wire_builder.curve(inv[1], inv[2], inv[3])
            wire_builder.curve(inv[4], inv[5], inv[6])
        else:
            wire_builder.curve(*inv[1:])

        # is there a section of outer circle between outer fillets?
        if (outer_circle[1] > outer_fillet[1]):
            if has_outer_fillet:
                wire_builder.arc(outer_fillet, fo, 1) # outer fillet
            wire_builder.arc(outer_circle, Ro, 1) # arc across the outer circle

        if has_outer_fillet:
            wire_builder.arc(invR[-1], fo, 1) # outer fillet on back side

        # back involute
        if split_involute:
            wire_builder.curve(invR[5], invR[4], invR[3])
            wire_builder.curve(invR[2], invR[1], invR[0])
        else:
            wire_builder.curve(*invR[-2::-1])

        if (has_non_involute_flank):
            wire_builder.line(inner_fillet_back) # line down to top of inner fillet

        # is there a section of inner circle between inner fillets?
        if (inner_circle_next[1] > inner_circle_back[1]):
            if has_inner_fillet:
                wire_builder.arc(inner_circle_back, fi, 0) # inner fillet on back side
            wire_builder.arc(inner_circle_next, Ri, 1) # arc across the inner circle

        if has_inner_fillet:
            wire_builder.arc(inner_fillet_next, fi, 0)

    wire_builder.close()


def genInvolutePolar(Rb, R):
    """return the involute angle as function of radius R.
    Rb = base circle radius
    """
    return (sqrt(R*R - Rb*Rb) / Rb) - acos(Rb / R)


def rotate(pt, rads):
    """rotate pt by rads radians about origin"""
    sinA = sin(rads)
    cosA = cos(rads)
    return (pt[0] * cosA - pt[1] * sinA,
            pt[0] * sinA + pt[1] * cosA)


def mirror(pt):
    """mirror pt on the X axis, i.e. flip its Y"""
    return (pt[0], -pt[1])


def toCartesian(radius, angle):
    """convert polar coords to cartesian"""
    return (radius * cos(angle), radius * sin(angle))


def findRootNewton(f, f_prime, x_min, x_max):
    """Apply Newton's Method to find the root of f within x_min and x_max
    We assume that there is a root in that range and that f is strictly monotonic,
    i.e. we don't take precautions for overshooting beyond the input range.
    """
    # As initial guess let's take the middle of our input range
    x = (x_min + x_max) / 2

    # FreeCAD.Base.Precision.intersection() is 1e-9, but this file doesn't depend on FreeCAD,
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
    for k in range(1, N + 1):
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

    for k in range(1, p):
        for j in range(len(T[k]) - 1):
            T[k + 1][j + 1] = 2 * T[k][j]
        for j in range(len(T[k - 1])):
            T[k + 1][j] -= T[k - 1][j]

    # convert the chebyshev function series into a simple polynomial
    # and collect like terms, out T polynomial coefficients
    for k in range(p + 1):
        fnCoeff.append(chebyExpnCoeffs(k, func))

    for k in range(p + 1):
        for pwr in range(p + 1):
            coeffs[pwr] += fnCoeff[k] * T[k][pwr]

    coeffs[0] -= fnCoeff[0] / 2  # fix the 0th coeff
    return coeffs


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
    for i in range(p + 1):
        bx = bezCoeff(i, p, polyCoeffsX)
        by = bezCoeff(i, p, polyCoeffsY)
        bzCoeffs.append((bx, by))
    return bzCoeffs

