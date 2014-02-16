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
from math import cos, sin
from involute import CreateExternalGear, rotate

def makeGear(m, Z, angle):
    w = SVGWireBuilder()
    CreateExternalGear(w, m, Z, angle)
    return '\n'.join(w.svg)

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
    p = OptionParser()
    p.add_option('-a', '--angle', help='pressure angle',
                 dest='angle', default=20)
    opts, args = p.parse_args()
    if len(args) != 2:
        p.error()
    m, Z = [float(v) for v in args]
    print makeGear(m, int(Z), float(opts.angle))

