# (c) 2014 David Douard <david.douard@gmail.com>
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

import itertools
from involute import CreateExternalGear, CreateInternalGear, rotate

class SVGWireBuilder(object):
    def __init__(self):
        self.theta = 0.0
        self.pos = None
        self.svg = []

    def move(self, p):
        p = rotate(p, self.theta)
        self.svg.append('M %s,%s' % (p[0], p[1]))
        self.pos = p

    def line(self, p):
        p = rotate(p, self.theta)
        self.svg.append('L %s,%s' % (p[0], p[1]))
        self.pos = p

    def arc(self, p, r, sweep):
        p = rotate(p, self.theta)
        self.svg.append('A %s,%s 0,0,%s %s,%s' % (r, r, str(sweep), p[0], p[1]))
        self.pos = p

    def curve(self, *points):
        """Add a Bezier curve from self.pos to points[-1]
        every other points are the control points of the Bezier curve (which
        will thus be of degree len(points) )
        """
        assert len(points) == 3
        points = [rotate(p, self.theta) for p in points]
        self.svg.append('C %s,%s %s,%s %s,%s' % tuple(itertools.chain(*points)))
        self.pos = points[-1]

    def close(self):
        self.svg.append('Z')


if __name__ == '__main__':
    from optparse import OptionParser
    p = OptionParser(
        usage="usage: %prog [options] [[MODULE] NUMER_OF_TEETH]",
        description=("Generates the outline of a metric, involute gear. "
            "Prints out an SVG path. "
            "This is mainly a debugging tool to quickly inspect the gear visually. "
            "For this, online tools like https://yqnn.github.io/svg-path-editor/ are handy. "
            "Most of the time it's enough to just use the first 20 lines or so, e.g.:\n\t"
            "%prog -z50 | head -n20 | pbcopy"))
    p.add_option('-z', '--numer-of-teeth',
        help="The number of teeth for the gear.",
        metavar='NUMER_OF_TEETH', type='int')
    p.add_option('-m', '--module',
        help="The metric module, in svg user unit, i.e. unit-less. [default: %default]",
        metavar='MODULE', type='float', default=1)
    p.add_option('-p', '--pressure-angle',
        help="The pressure angle, in degree. [default: %default]",
        metavar='PRESSURE_ANGLE', type='float', default=20)
    p.add_option('-i', '--internal',
        help=("Generates an internal gear, i.e. the addednum points towards the center "
            "and the root fillet is at the outside. [default: %default]"),
        action='store_true', default=False)
    p.add_option('-a', '--addendum',
        help=("The tooth height above the pitch line, normalized by the MODULE. "
            "[default: %default]"),
        metavar='ADDENDUM_COEFFICIENT', type='float', default=1)
    p.add_option('-d', '--dedendum',
        help=("The tooth height from the root to the pitch line, normalized by the MODULE. "
            "[default: %default]"),
        metavar='DEDENDUM_COEFFICIENT', type='float', default=1.25)
    p.add_option('-f', '--fillet-radius',
        help=("The radius of the root fillet, normalized by the MODULE. "
            "[default: %default]"),
        metavar='FILLET_RADIUS_COEFFICIENT', type='float', default=0.38)

    opts, args = p.parse_args()
    errors = []
    if len(args) == 1:
        opts.numer_of_teeth = int(args[0])
    if len(args) == 2:
        opts.module = float(args[0])
        opts.numer_of_teeth = int(args[1])
    if len(args) > 2:
        errors.append("Too many arguments.")
    if opts.numer_of_teeth is None:
        errors.append("No number of teeth given.")
    if len(errors) > 0:
        errors.append("Try --help for more info.")
        p.error("\n".join(errors))

    w = SVGWireBuilder()
    generator_func = CreateInternalGear if opts.internal else CreateExternalGear
    generator_func(w, opts.module, opts.numer_of_teeth, opts.pressure_angle,
        addCoeff=opts.addendum, dedCoeff=opts.dedendum,
        filletCoeff=opts.fillet_radius)

    for l in w.svg:
        print(l)

