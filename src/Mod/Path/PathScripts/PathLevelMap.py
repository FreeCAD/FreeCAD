# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2022 Oleg Belov <obelov@audiology.ru>                   *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published yb the Free Software Foundation; either version 2 of     *
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

import math
import numpy
#try:
from PathScripts.PathContourMap import ContourMap
#except:
#    pass  # In case of stanalone test

import platform
import re

if platform.system() == "Windows":
    try:
        import subprocess
    except:
        pass

# This is a square grid of elevations in given direction.
# Each cell [i,j] holds a maximum value of the model elevation in a square region
# with coordinates
#   (xmin + (i - border) * sample_interval .. xmin + (i - border + 1) * sample_interval,
#    ymin + (j - border) * sample_interval .. ymin + (j - border + 1) * sample_interval)

class LevelMap():
    L2size = None
  
    def __init__( self, xmin, xmax, ymin, ymax, zmin, sample_interval, border,
                  cols = None, rows = None):
        self.sampleInterval = sample_interval
        if xmax < xmin:
          xmax, xmin = xmin, xmax
        if ymax < ymin:
          ymax, ymin = ymin, ymax
        self.xmin = xmin
        if cols is None:
            cols = int(math.ceil((xmax - xmin) / sample_interval) + 1)
        self.xmax = self.xmin + cols * sample_interval
        self.ymin = ymin
        if rows is None:
            rows = int(math.ceil((ymax - ymin) / sample_interval) + 1)
        self.ymax = self.ymin + rows * sample_interval
        self.zmin = zmin
        self.matrix = None
        self.border = border
        self.z = numpy.full((rows + 2 * border, cols + 2 * border),  # elevation data
                             zmin,
                             dtype = numpy.single
                            )
        self.kk = numpy.zeros(max(rows, cols) + 2 * border, dtype=int)
        self.tool_radius = 5
        self.tool_profile = None   # sorted list of (radius, elevation)

        if LevelMap.L2size is None:
            try:
                if platform.system() == "Windows":
                    for line in subprocess.Popen('wmic cpu get L2CacheSize').split("\n"):
                        if str.isdigit(line):
                            LevelMap.L2size = max(256, int(line)) * 1024
                            break
                else: #  platform.system() == "Linux":
                    fp = open("/proc/cpuinfo", "r")
                    info = fp.read().split("\n")
                    fp.close()
                    r = re.compile("cache size\s*:\s*(\d+)\s*(\w)")
                    for line in info:
                        rm = r.match(line)
                        if not rm is None:
                            LevelMap.L2size = int(rm.group(1)) * (
                                1024 * 1024 if rm.group(2) == "M" else 1024)
                            break
            except:
                pass
        if LevelMap.L2size is None:
            LevelMap.L2size = 256 * 1024
        
    def empty_copy(self):
        R, C = self.z.shape
        answer = LevelMap(self.xmin, self.xmax, self.ymin, self.ymax, 
                          self.zmin, self.sampleInterval, self.border,
                          cols = C - 2 * self.border,
                          rows = R - 2 * self.border)
        answer.matrix = self.matrix
        return answer
        
    def set_rotation(self):
        pass
        #TODO set matrix in accordance with sample_interval and given rotation.
        
    def reset(self, z = None):
        if not z is None:
            self.zmin = z
        self.z[:] = self.zmin
      
    def rows(self):
        R, C = self.z.shape
        return R - 2 * self.border
      
    def columns(self):
        R, C = self.z.shape
        return C - 2 * self.border
      
    def level(self):
        return self.z[self.border:-self.border, self.border:-self.border]
      
    def includes(self, obj):   # True if this object is inside the map XY area
        if hasattr(obj, "BoundBox"):
            obj = obj.BoundBox
        if not hasattr(obj, "XMin"):
            return True
        
        brd = self.border * self.sampleInterval
        if self.matrix is None:
            if obj.XMax < self.xmin - brd or obj.XMin >= self.xmax + brd:
                return False
            if obj.YMax < self.ymin - brd or obj.YMin >= self.ymax + brd:
                return False
            return True
        else:
            return True    #TODO Make exact decision
          
    def activeRadius(self, z ):
        if not self.tool_profile is None:
            for rad, elev in self.tool_profile[::-1]:
                if elev < z:
                    return rad
        return self.tool_radius
      
    def add_facet( self, va, vb, vc ):
        if not self.matrix is None:
            mr = self.matrix[0]
            xva = va[0] * mr[0] + va[1] * mr[1] + va[2] * mr[2] + self.border
            xvb = vb[0] * mr[0] + vb[1] * mr[1] + vb[2] * mr[2] + self.border
            xvc = vc[0] * mr[0] + vc[1] * mr[1] + vc[2] * mr[2] + self.border
            mr = self.matrix[1]
            yva = va[0] * mr[0] + va[1] * mr[1] + va[2] * mr[2] + self.border
            yvb = vb[0] * mr[0] + vb[1] * mr[1] + vb[2] * mr[2] + self.border
            yvc = vc[0] * mr[0] + vc[1] * mr[1] + vc[2] * mr[2] + self.border
            mr = self.matrix[2]
            zva = va[0] * mr[0] + va[1] * mr[1] + va[2] * mr[2]
            zvb = vb[0] * mr[0] + vb[1] * mr[1] + vb[2] * mr[2]
            zvc = vc[0] * mr[0] + vc[1] * mr[1] + vc[2] * mr[2]
            
        else:
            xva = (va[0] - self.xmin) / self.sampleInterval + self.border
            xvb = (vb[0] - self.xmin) / self.sampleInterval + self.border
            xvc = (vc[0] - self.xmin) / self.sampleInterval + self.border
            yva = (va[1] - self.ymin) / self.sampleInterval + self.border
            yvb = (vb[1] - self.ymin) / self.sampleInterval + self.border
            yvc = (vc[1] - self.ymin) / self.sampleInterval + self.border
            zva = va[2]
            zvb = vb[2]
            zvc = vc[2]
        
        b1 = self._add_edge( xva, yva, zva, xvb, yvb, zvb )
        b2 = self._add_edge( xvb, yvb, zvb, xvc, yvc, zvc )
        b3 = self._add_edge( xvc, yvc, zvc, xva, yva, zva )
        if b1 or b2 or b3:
            self._add_triangle( xva, yva, zva, xvb, yvb, zvb, xvc, yvc, zvc ) 
            
    def getContourMap( self, z, out = None, air = 0 ):
        if not out is None:
            out.setContourMap(self.xmin, self.ymin, z, self.sampleInterval,
                       self.z[self.border:-self.border, self.border:-self.border], 
                       air = air)
            return out
        return ContourMap(self.xmin, self.ymin, z, self.sampleInterval,
                          self.z[self.border:-self.border, self.border:-self.border], 
                          air = air)
      
    def _add_edge( self, xa, ya, za, xb, yb, zb ):
        # This algorithm should be coded in C
        R, C = self.z.shape
        if ((xa < 0 and xb < 0) or (xa >= C and xb >= C) or 
            (ya < 0 and yb < 0) or (ya >= R and yb >= R)):
            return False
        
        if zb < za:
            xa, xb = xb, xa
            ya, yb = yb, ya
            za, zb = zb, za
        
        ia = int(math.floor( xa ))
        ja = int(math.floor( ya ))
        ib = int(math.floor( xb ))
        jb = int(math.floor( yb ))

        _x = xa - ia
        _y = ya - ja
        nx = abs(ia - ib)
        ny = abs(ja - jb)

        if nx > 0:
            dy_adx = (yb - ya) / abs(xb - xa)  # dy / abs(dx)
            dz_adx = (zb - za) / abs(xb - xa)
            if xb > xa:
                dx = 1
                y0 = ya + (1 - _x) * dy_adx  # y and z of the first crossection with 
                z0 = za + (1 - _x) * dz_adx  # a horizontal line of the grid
                if ia < 0:
                    y0 -= ia * dy_adx   # skip lines
                    z0 -= ia * dz_adx
                    nx += ia
                    ia = 0
                if ib >= C:
                    nx -= (ib - C + 1)
            else:
                dx = -1
                y0 = ya + _x * dy_adx  # y and z of the first crossection with 
                z0 = za + _x * dz_adx  # a horizontal line of the grid
                if ia >= C:
                    y0 += (ia - C + 1) * dy_adx   # skip lines
                    z0 += (ia - C + 1) * dz_adx
                    nx -= (ia - C + 1)
                    ia = C - 1
                if ib < 0:
                    nx += ib
              
            if y0 < 0 or ya < 0 or yb < 0 or y0 >= R or ya >= R or yb >= R: 
                # loop with range check
                for i in range(ia, ia + nx * dx, dx):
                    j = int(math.floor(y0))
                    if j >= 0 and j < R:
                        if self.z[j, i] < z0:
                            self.z[j, i] = z0
                    y0 += dy_adx
                    z0 += dz_adx
                    
            elif nx < 10:             # simple loop
                for i in range(ia, ia + nx * dx, dx):
                    j = int(math.floor(y0))
                    if self.z[j, i] < z0:
                        self.z[j, i] = z0
                    y0 += dy_adx
                    z0 += dz_adx
                    
            else:                     # vectorized, not necessary in C implementation
                if abs(dy_adx) > 0.0001:
                    self.kk[0:nx] = numpy.arange(y0, (y0 + dy_adx * (nx - 0.5)), 
                                                  dy_adx)
                    self.kk[0:nx] *= C
                else:
                    self.kk[0:nx] = int(y0) * C
                self.kk[0:nx] += numpy.arange(ia, ia + dx * nx, dx)
                if dz_adx < 0.001 / nx:
                    self.z.flat[self.kk[0:nx]] = numpy.maximum(
                                       self.z.flat[self.kk[0:nx]], z0)
                else:
                    self.z.flat[self.kk[0:nx]] = numpy.maximum(
                                  self.z.flat[self.kk[0:nx]], 
                                  numpy.arange(z0, z0 + dz_adx * (nx - 0.5), dz_adx))
                    
        if ny > 0:
            dx_ady = (xb - xa) / abs(yb - ya)  # dx / abs(dy)
            dz_ady = (zb - za) / abs(yb - ya)
            if yb > ya:
                dy = 1
                x0 = xa + (1 - _y) * dx_ady  # x and z of the first crossection with 
                z0 = za + (1 - _y) * dz_ady  # a horizontal line of the grid
                if ja < 0:
                    x0 -= ja * dx_ady   # skip lines
                    z0 -= ja * dz_ady
                    ny += ja
                    ja = 0
                if jb >= R:
                    ny -= (jb - R + 1)
            else:
                dy = -1
                x0 = xa + _y * dx_ady  # x and z of the first crossection with 
                z0 = za + _y * dz_ady  # a horizontal line of the grid
                if ja >= R:
                    x0 += (ja - R + 1) * dx_ady   # skip lines
                    z0 += (ja - R + 1) * dz_ady
                    ny -= (ja - R + 1)
                    ja = R - 1
                if jb < 0:
                    ny += jb
              
            if x0 < 0 or xa < 0 or xb < 0 or x0 >= C or xa >= C or xb >= C: 
                # loop with range check
                for j in range(ja, ja + ny * dy, dy):
                    i = int(math.floor(x0))
                    if i >= 0 and i < C:
                        if self.z[j, i] < z0:
                            self.z[j, i] = z0
                    x0 += dx_ady
                    z0 += dz_ady
                    
            elif ny < 10:             # simple loop        
                for j in range(ja, ja + ny * dy, dy):
                    i = int(math.floor(x0))
                    if self.z[j, i] < z0:
                        self.z[j, i] = z0
                    x0 += dx_ady
                    z0 += dz_ady
                    
            else:                     # vectorized, not necessary in C implementation
                base = ja * C + x0
                step = C * dy + dx_ady
                self.kk[0:ny] = numpy.arange(base, 
                                             base + step * (ny - 0.5), 
                                             step)
                if dz_ady < 0.001 / ny:
                    self.z.flat[self.kk[0:ny]] = numpy.maximum(
                                               self.z.flat[self.kk[0:ny]], z0)
                else:
                    self.z.flat[self.kk[0:ny]] = numpy.maximum(
                                  self.z.flat[self.kk[0:ny]], 
                                  numpy.arange(z0, z0 + dz_ady * (ny - 0.5), dz_ady))
                    

        if ib >=0 and jb >=0 and ib < C and jb < R and self.z[jb, ib] < zb:
            self.z[jb, ib] = zb
            
        return True    

    def _add_triangle( self, xa, ya, za, xb, yb, zb, xc, yc, zc ):
        # This algorithm should be coded in C
        # Only cells located inside the given triangle and not crossed with it's 
        # edges are marked.
        R, C = self.z.shape
        i0 = max(0, int(numpy.floor(min( xa, xb, xc ))) + 1)
        i1 = min(C - 1, int(numpy.floor(max( xa, xb, xc ))))
        j0 = max(0, int(numpy.floor(min( ya, yb, yc ))) + 1)
        j1 = min(R - 1, int(numpy.floor(max( ya, yb, yc ))))
        ni = i1 - i0
        nj = j1 - j0
        if ni <= 0 or nj <= 0:
            return
        
        if ni > nj:
            # sort ya <= yc <= yb
            if yb < ya:
                xb, xa = xa, xb
                yb, ya = ya, yb
                zb, za = za, zb
            if yc < ya:
                xc, xa = xa, xc
                yc, ya = ya, yc
                zc, za = za, zc
            if yc > yb:
                xc, xb = xb, xc
                yc, yb = yb, yc
                zc, zb = zb, zc
                
            jc = min(R - 1, int(numpy.floor(yc)))

        else:
            # sort xa <= xc <= xb
            if xb < xa:
                xb, xa = xa, xb
                yb, ya = ya, yb
                zb, za = za, zb
            if xc < xa:
                xc, xa = xa, xc
                yc, ya = ya, yc
                zc, za = za, zc
            if xc > xb:
                xc, xb = xb, xc
                yc, yb = yb, yc
                zc, zb = zb, zc
              
            ic = min(C - 1, int(numpy.floor(xc)))

        yab = ya - yb
        ybc = yb - yc
        yca = yc - ya
        xab = xa - xb
        xbc = xb - xc
        xca = xc - xa
        zac = za - zc
        zbc = zb - zc
            
        den =  xbc * yca - ybc * xca
        if abs(den) < 1.0:
            return

        dzdx = (zac * ybc + zbc * yca) / den
        dzdy = -(zac * xbc + zbc * xca) / den
        d    = 0

        if ni > nj:

            zfirst = za + (j0 - ya + int(dzdy > 0)) * dzdy
            
            a0 = a2 = xab / yab
            if j0 <= jc:
                a1 = xca / yca  # yca is not zero because j0-1 < jc
                if a0 > a1:
                    a0, a1 = a1, a0
            if jc < j1:
                a3 = xbc / ybc
                if a2 < a3:
                    a2, a3 = a3, a2

            for j in range(j0, j1):
                if j <= jc:
                    i0 = max(0, int(numpy.ceil(xa + (j - ya + int(a0 > 0)) * a0)))
                    i1 = min(C, int(numpy.floor(xa + (j - ya + int(a1 < 0)) * a1)))
                else:
                    i0 = 0
                    i1 = C
                
                if j >= jc:
                    i0 = max(i0, int(numpy.ceil(xb + (j - yb + int(a2 > 0)) * a2)))
                    i1 = min(i1, int(numpy.floor(xb + (j - yb + int(a3 < 0)) * a3)))

                z0 = zfirst + (i0 - xa + int(dzdx > 0)) * dzdx
                zfirst += dzdy
                
                if i1 - i0 < 10:
                    for i in range(i0, i1):
                        if self.z[j, i] < z0:
                            self.z[j, i] = z0
                            z0 += dzdx
                elif abs(dzdx) < 0.001 / (i1 - i0):
                    numpy.maximum(self.z[j, i0:i1], z0,
                                  out=self.z[j, i0:i1]
                                  )
                else:
                    numpy.maximum(self.z[j, i0:i1],
                                  numpy.arange(z0, z0 + dzdx * (i1 - i0 - 0.5), dzdx),
                                  out=self.z[j, i0:i1]
                                  )
          
        else:
              
            zfirst = za + (i0 - xa + int(dzdx > 0)) * dzdx
            
            a0 = a2 = yab / xab
            if i0 <= ic:
                a1 = yca / xca  # xca is not zero because i0-1 < ic
                if a0 > a1:
                    a0, a1 = a1, a0
            if ic < i1:
                a3 = ybc / xbc
                if a2 < a3:
                    a2, a3 = a3, a2

            for i in range(i0, i1):
                if i <= ic:
                    j0 = max(0, int(numpy.ceil(ya + (i - xa + int(a0 > 0)) * a0)))
                    j1 = min(R, int(numpy.floor(ya + (i - xa + int(a1 < 0)) * a1)))
                else:
                    j0 = 0
                    j1 = R
                
                if i >= ic:
                    j0 = max(j0, int(numpy.ceil(yb + (i - xb + int(a2 > 0)) * a2)))
                    j1 = min(j1, int(numpy.floor(yb + (i - xb + int(a3 < 0)) * a3)))

                z0 = zfirst + (j0 - ya + int(dzdy > 0)) * dzdy
                zfirst += dzdx

                if j1 - j0 < 10:
                    for j in range(j0, j1):
                        if self.z[j, i] < z0:
                            self.z[j, i] = z0
                            z0 += dzdy
                elif abs(dzdy) < 0.001 / (j1 - j0):
                    numpy.maximum(self.z[j0:j1, i], z0,
                                  out=self.z[j0:j1, i]
                                  )
                else:
                    numpy.maximum(self.z[j0:j1, i],
                                  numpy.arange(z0, z0 + dzdy * (j1 - j0 - 0.5), dzdy),
                                  out=self.z[j0:j1, i]
                                  )

    # Transformation of the level map to the lowest surface where the given
    # toolbit can travel in any direction without cutting something from
    # the original profile,
    
    # For rectangular mill an optimized algorithm is used.
    # Several partial maximums ara calculated and stored in arrays organized in
    # the "partial" list.
    # partial[0] contains the original level map, while
    # partial[1], partial[3] .. etc. contain square blocks and
    # partial[2], partial[4] .. etc. contain diagonal blocks.
    # Each cell marked here as + contains a maximum of cells from the original
    # array marked as point.
    #
    #  [0]   [1]   [2]      [3]     [4]  .      ...
    #                .      ....        ...
    #         ..    ...     ....       ..... 
    #   +     +.   +....    ....      ....... 
    #               ...     +...     .........
    #                .              +..........
    #                                .........
    #                                 .......
    #                                  .....
    #                                   ...
    #                                    .

    # BLOCK_SIZE = (1, 2, 3,  4, 5,  8, 9,  16, 17,  32, 33, 64)

    # Job is a sorted list of (y, source_index, x, elevation, dzdxmin, dzdxmax, dzdymin, dzdymax)
 

    def _symmetric_append(self, job, i, j, index, d0, d1, d2, d3):
        bs = self.bss[index]
        job.append(( j, index, i, 0, d0, d1, d2, d3 ))
        if index % 2 == 1:
            job.append(( -j - bs + 1, index, i, 0, d0, d1, -d3, -d2 ))
            job.append(( i, index, j, 0, d2, d3, d0, d1 ))
            job.append(( i, index, -j - bs + 1, 0, -d3, -d2, d0, d1 ))
        else:
            job.append(( -j, index, i, 0, d0, d1, -d3, -d2 ))
            job.append(( j, index, -i - (bs - 1) * 2, 0, -d1, -d0, d2, d3))
            job.append(( -j, index, -i - (bs - 1) * 2, 0, -d1, -d0, -d3, -d2))

    def _paint_job(self, job, irt, filled = None):
        # return numpy boolean array (irt*2+1, irt*2+1)
        # Used both in algorithm and for test
        if filled is None:
            answer = numpy.zeros((irt*2+1, irt*2+1), dtype=bool)
        else:
            answer = filled

        for ji in job:
            j, k, i = ji[0:3]
            if k == 0:
                answer[irt+j, irt+i] = True
            elif k % 2 == 1:
                bs = 2 ** ((k + 1) // 2)
                answer[irt+j:irt+j+bs, irt+i:irt+i+bs] = True
            else:
                bs = 2 ** (k // 2) + 1
                for m in range(0, bs):
                    answer[irt+j+m, irt+i+m:irt+i+bs*2-m-1] = True
                    answer[irt+j-m, irt+i+m:irt+i+bs*2-m-1] = True
        return answer


    def _create_coverage(self, job, partial, rt):  
        irt = min(self.border, int(numpy.ceil(rt)))
        # calculate row half width:
        # Each row contains odd number of cells, and there are irt * 2 + 1 rows.
        hw = []
        for i in range(0, irt):
            hw.append(min(self.border, 
                          int(numpy.ceil(math.sqrt(rt ** 2 - (i) ** 2)))),
                     )

        # Create blocks, maximum block size is rt * 2/3
        self.bss = [1,2,3]
        while True:
            bs = self.bss[-2] * 2
            if bs > irt * 2 // 3 or bs > self.border:
                break
            self.bss.append( bs )
            self.bss.append( bs + 1 )
        self.bss = self.bss[:-1]
        
        # Cover top irt * (1-cos(22.5)) rows by maximum available blocks
        MAX = 1e37
        top = int(math.floor(irt * 0.9))
        min_full_covered_row = irt
        for r in range(irt - 1, top - 1, -1):
            width = hw[r] * 2 + 1
            ind = sum(t <= width for t in self.bss) - 1 # get the appropriate block size
            if ind > 0 and (ind % 2 == 0):
                ind -= 1
            bs = self.bss[ind]
            i = -hw[r]
            if (r == irt - 1 or r < min_full_covered_row or 
                r == top and min_full_covered_row > hw[min_full_covered_row]):
                while width > 0:
                    dzdxmin = 0 if i > 0 else -MAX
                    dzdxmax = 0 if i + bs - 1 < 0 else MAX
                    self._symmetric_append( job, i, r - bs + 2, ind,
                                            dzdxmin, dzdxmax, 0, MAX )
                    width -= bs
                    i += bs
                    if width > 0 and width < bs:
                        dzdxmin = 0 if i + width - bs > 0 else -MAX
                        self._symmetric_append( job, i + width - bs, 
                                                r - bs + 2, ind,
                                                dzdxmin, MAX, 0, MAX)
                        break
                min_full_covered_row = r - bs + 1
            else:
                dzdxmax = 0 if width > bs else MAX
                self._symmetric_append( job, i, r - bs + 2, ind, -MAX, dzdxmax, 0, MAX )
                if width > bs:
                    self._symmetric_append( job, hw[r] - bs + 1, 
                                            r - bs + 2, ind, 0, MAX, 0, MAX )

        # locate diagonal cell and cover diagonal rows
        j = 0
        while j < len(hw) and hw[j] - 1 > j:  #index of the last cell
            j += 1
        i = hw[j] - 1
        r = 0
        while j < top and i > 0:
            # trace diagonal elements
            while hw[j + 1] == i and j < top - 1:
                j += 1
                i -= 1
            width = j - i + 1
            if width == 2:
                width = 3
            ind = sum(t <= width for t in self.bss) - 1 # get the appropriate block size
            if ind % 2 == 1:
                ind -= 1
            bs = self.bss[ind]
            if r == 0:
                d = 0
                while d < width:
                    self._symmetric_append(job, 
                                           i + d - bs + 2, 
                                           j - d - bs + 2, 
                                           ind, 
                                           0, MAX, 0, MAX )
                    d += bs
                    if d < width and width - d < bs:
                        self._symmetric_append(job, 
                                               i - 2 * bs + width + 2, 
                                               j - width + 2, 
                                               ind, 
                                               0, MAX, 0, MAX )
                        break
            else:
                self._symmetric_append(job, 
                                       i - bs + 2, 
                                       j - bs + 2, 
                                       ind, 
                                       0, MAX, 0, MAX )
                if width > bs:
                    self._symmetric_append(job, 
                                           i - 2 * bs + width + 2, 
                                           j - width + 2,
                                           ind, 
                                           0, MAX, 0, MAX )
            r += 1
            i -= 1
            if j == top - 1 and hw[top] > i:
                break
        
        #  Cover the center
        #    Get cells to fill
        filled = self._paint_job(job, irt)
        #paint area outside the mill
        for i in range(0, irt):
            filled[irt + i + 1, :irt - hw[i]] = True
            filled[irt + i + 1, irt + 1 + hw[i]:] = True
            filled[irt - i - 1, :irt - hw[i]] = True
            filled[irt - i - 1, irt + 1 + hw[i]:] = True

        indj, indi = numpy.where(~filled)
        if len(indj) > 0:
            j0 = indj.min() -irt
            j1 = indj.max() -irt + 1
            i0 = indi.min() -irt
            i1 = indi.max() -irt + 1
#        if min_full_covered_row > 0:
#            j0 = -min_full_covered_row
#            j1 = min_full_covered_row + 1
#            i0 = j0
#            i1 = j1
            # get the appropriate block size
            width = min(j1-j0, i1-i0)
            ind = sum(t <= width for t in self.bss) - 1
            if ind > 0 and (ind % 2 == 0):
                ind -= 1
            bs = self.bss[ind]
            # if this block size was not used take the previous one
            if sum([jj[1] == ind for jj in job]) == 0 and bs > 2:
                ind = ind - 2
                bs = bs // 2
            for j in range(j0, j1, bs):
                for i in range(i0, i1, bs):
                    job.append((min(j, j1-bs), ind, min(i, i1-bs), 0, 0, 0, 0, 0))

        # remove unused block sizes
        for i in range(len(self.bss)-1, 0, -1):
            if sum([jj[1] == i for jj in job]) == 0:
                self.bss = self.bss[0:-1]
            else:
                break
              
        # allocate memory
        for i in range(1, len(self.bss)):
            partial.append(numpy.zeros(partial[-1].shape))
                         
    def applyTool( self, radius, profile ):
        # profile is None for square end mill or sorted list of (radius, elevation)
        self.tool_radius = radius
        self.tool_profile = profile
        rt = radius / self.sampleInterval
        border = self.border
        job = []
        maxcol = max(border, (2000 - border) // 16 * 16)  # to fit in L1 cache
        R, C = self.z.shape
        
        # For optimization find the first and the last significant rows and columns
        R0 = R
        R1 = 0
        C0 = C
        C1 = 0
        for j in range(0, R):
            ind = numpy.where(self.z[j,:] > self.zmin)[0]
            if len(ind) > 0:
                R0 = min(R0, j)
                R1 = j+1
                C0 = min(C0, ind.min())
                C1 = max(C1, ind.max()+1)
        if R0 > R1:
            return  # nothing to do
        # Only part R0-border..R1+border, C0-border..C1+border should taken into
        # account in calculation of the minimal mill elevation.
        R0 = max(0, R0 - 2 * border)
        R1 = min(R, R1 + 2 * border)
        C0 = max(0, C0 - 2 * border)
        C1 = min(C, C1 + 2 * border)
        RA = R1 - R0
        CA = C1 - C0
        
        partial = [numpy.zeros((RA, min(maxcol + 2 * border, CA)))]
        
        if CA > maxcol + 2 * border:
            buff = numpy.empty((RA, border))
          
        # minimum and maximum of derivatives by rows:
        #   -min(dzdx) max(dzdx) -min(dzdy) max(dzdy)
        dd = numpy.empty((RA, 4))
        dz = numpy.empty((RA, min(maxcol + 2 * border, CA)))
          
        if profile is None:
            self._create_coverage(job, partial, rt)
                
        else:
            pr = [p[0] for p in profile]
            pz = [p[1] for p in profile]
            # if the tool has flat surface around the center create
            # coverage for it.
            irc = 0
            while irc < len(pz) - 1 and pz[irc+1] == pz[0]:
                irc += 1
            if irc > 4:
                self._create_coverage(job, partial, irc)
            else:
                irc = 0
            irt = int(numpy.ceil(rt))
            zz =  numpy.interp(numpy.arange(0, irt+2) * self.sampleInterval, pr, pz)
            dzz = numpy.diff(zz)
            dzz[-2:-1] = 1e37
            for i in range(irc+1, min(border, irt + 1)):
                z = - zz[i-1]
                dzm = dzz[i-1]
                dzp = dzz[i]
                job.append(( i, 0,  0, z, 0, 0, dzm, dzp)) 
                job.append((-i, 0,  0, z, 0, 0, -dzp, -dzm)) 
                job.append(( 0, 0,  i, z, dzm, dzp, 0, 0)) 
                job.append(( 0, 0, -i, z, -dzp, -dzm, 0, 0)) 
                for j in range(1, i + 1):
                    r = math.sqrt((i - 1)**2 + (j - 1)**2)
                    if r < rt and r > irc:
                        z = - numpy.interp(r * self.sampleInterval, pr, pz)
                        dzxm = i / r * dzz[int(r)]
                        dzxp = i / r * dzz[int(r)+1]
                        dzym = j / r * dzz[int(r)]
                        dzyp = j / r * dzz[int(r)+1]
                        job.append(( j, 0,  i, z, dzxm, dzxp, dzym, dzyp)) 
                        job.append(( j, 0, -i, z, -dzxp, -dzxm, dzym, dzyp)) 
                        job.append((-j, 0,  i, z, dzxm, dzxp, -dzyp, -dzym)) 
                        job.append((-j, 0, -i, z, -dzxp, -dzxm, -dzyp, -dzym)) 
                        if j < i:
                            job.append(( i, 0,  j, z, dzym, dzyp, dzxm, dzxp)) 
                            job.append(( i, 0, -j, z, -dzyp, -dzym, dzxm, dzxp)) 
                            job.append((-i, 0,  j, z, dzym, dzyp, -dzxp, -dzxm)) 
                            job.append((-i, 0, -j, z, -dzyp, -dzym, -dzxp, -dzxm)) 
        
        job.sort()
        
        offs = C0 + border
        
        if len(partial) > 1:
            TR = self.bss[-1]
            temp = numpy.zeros((TR, partial[0].shape[1]))                         

        while offs < C1 - border:
            cols = min( maxcol, C1 - border - offs )
            if offs > C0 + border:
                partial[0][:,:cols+2*border] = numpy.column_stack(
                                       (buff, self.z[R0:R1, offs:offs+cols+border])
                                     )
            else:
                partial[0][:,:cols+2*border] = self.z[R0:R1, 
                                                      offs-border:offs+cols+border]
            
            PC = cols+2*border
            for k in range(1, len(partial)):
                if k == 2:
                    this = partial[2]
                    base = partial[1]
                    orig = partial[0]
                    for j in range(2, RA-2):
                        partial[2][j,:PC-2] = numpy.maximum(      
                                                base[j-1,1:PC-1],     
                                                base[j-1,2:PC])     
                    for j in range(2, RA-2):
                        numpy.maximum(this[j,:],     this[j+1,:],    out=this[j,:])   
                        numpy.maximum(this[j,:PC],   orig[j,:PC],    out=this[j,:PC])
                        numpy.maximum(this[j,:PC-4], orig[j,4:PC],   out=this[j,:PC-4])
                        numpy.maximum(this[j,:PC-2], orig[j-2,2:PC], out=this[j,:PC-2])
                        numpy.maximum(this[j,:PC-2], orig[j+2,2:PC], out=this[j,:PC-2])

                else:
                    if k == 1:
                        bs = 1
                        base = partial[0]
                    else:
                        bs = self.bss[k-2]
                        base = partial[k-2]

                    if bs % 2 == 1 and bs != 1:   # diagonal elements
                        sh = bs - 1
                        for j in range(0, RA-sh ):
                            temp[j % TR, :PC-sh] = numpy.maximum(
                                                    base[j, :PC-sh],
                                                    base[j+sh, sh:PC])
                            partial[k][j, :PC-sh] = numpy.maximum(
                                                    temp[j % TR, :PC-sh],
                                                    temp[(j-sh) % TR, sh:PC])
                    else:
                        for j in range(RA-1, -1, -1):
                            temp[j % TR, :PC-bs] = numpy.maximum(
                                                    base[j, :PC-bs],
                                                    base[j, bs:PC])
                            partial[k][j, :PC] = numpy.maximum(
                                                 temp[j % TR, :PC],
                                                 temp[(j+bs) % TR,:PC]
                                               )

            if offs + cols < C1 - border:
                buff[:,:] = self.z[R0:R1, offs+cols-border:offs+cols]
            
            # Calculate the range of slopes in x and y directions
            dz[:,:cols+2*border-1] = numpy.diff(
                            self.z[R0:R1, offs-border:offs+cols+border], 1, 1)
            dd[:,0] = -numpy.min(dz[:,:cols+2*border-1], 1)
            dd[:,1] = numpy.max(dz[:,:cols+2*border-1], 1)
            dz[:-1,:cols+2*border] = numpy.diff(
                            self.z[R0:R1, offs-border:offs+cols+border], 1, 0)
            dd[:,2] = -numpy.min(dz[:,:cols+2*border], 1)
            dd[:,3] = numpy.max(dz[:,:cols+2*border], 1)
            dd[-1,2:3] = dd[-2,2:3]
            
            # Expand this range along the maximum block size
            n = 1
            for i in range(0, ((len(partial) - 1) // 2)):
                dd[0:-n, :] = numpy.maximum(dd[0:-n, :], dd[n:, :])
                n += n
            if n > 0:
                dd[n:, :] = numpy.maximum(dd[n:, :], dd[0:-n, :])
            
       #     fd = file.open("minmax", "w")
       #     for ddi in dd:
       #         fd.write( "%.3f, %.3f, %.3f, %.3f" % list(ddi))
       #     fd.close
            jb = 0
            l2_items = LevelMap.L2size // (cols + 2 * border) - 1
            while jb < len(job):
                je = jb
                counter = set()
                while je < len(job):
                    counter.add(job[je][0:2])
                    if len(counter) > l2_items:
                        break
                    je += 1
                for m in range(R0+border, R1-border):
                    for j, k, i, z, dzdxmin, dzdxmax, dzdymin, dzdymax in job[jb:je]:
                        ddi = dd[m-R0+j, :]
                        if (dzdxmin > ddi[1] or dzdxmax < -ddi[0] or 
                            dzdymin > ddi[3] or dzdymax < -ddi[2]):
                            continue
                        numpy.maximum(
                            self.z[m, offs:offs+cols],
                            partial[k][m-R0+j, border+i:cols+border+i] + z,
                            out = self.z[m, offs:offs+cols]
                            )
                jb = je
            offs += cols
            
    def profileAlongPath(self, nodes, tolerance, shift_z, min_z):
        # return profile along the given path. The existed nodes are included with
        # a changed z coordinate (node[2]). New nodes can be inserted. 
        # The elevation of the created path above the map is in range 0..tolerance,
        # but points (crosses of the path and the grid) which are below min_z level 
        # are shifted up to this level.
        answer = []
        border = self.border
        ln = len(nodes)
        cycled = ln > 3 and nodes[0] == nodes[-1]
        zs = min_z   # z value of the current point shifted down by shift_z
        for i in range(0, ln):
            c, r = nodes[i][0:2]
            if len(nodes[i]) > 2:
                zs = nodes[i][2] - shift_z  # original z shifted down
            # pick z of endpoint from the map
            if c != int(c) or r != int(r):
                z = self.z[int(r) + border, int(c) + border]
            elif i == ln - 1 and cycled:
                z = answer[0][2]
            else:
                zp = self.z[int(r) + border - 1: int(r) + border + 1, 
                            int(c) + border - 1: int(c) + border + 1]
                if len(nodes[i]) > 2:
                    z = max(list(zp[zp <= nodes[i][2]]) + [min_z])
                else:
                    z = numpy.max(zp)
            if i == 0:
                answer.append(nodes[i][0:2] + (max(z, zs, min_z),) + nodes[i][3:])
            else:
                # create all intermediate crosses with the grid and optimize them
                c0, r0 = answer[-1][0:2]
                #if abs(r0 - 262) + abs(c0 - 380) < 5:
                #    print(c0, r0)
                #    print(self.z[int(r0) - 5 + border:int(r0) + 6 + border,
                #                 int(c0) - 5 + border:int(c0) + 6 + border])
                part = [answer[-1]]
                if c0 == c and r0 == r:
                    continue            # duplicates are not inserted
                dc = c - c0
                dr = r - r0
                dz = zs - zprev
                zz = nodes[i-1][2:3] + nodes[i][2:3]
                if abs(dc) > abs(dr):
                    if c > c0:
                        rc = range(int(numpy.floor(c0)) + 1, int(numpy.ceil(c)))
                    else:
                        rc = range(int(numpy.ceil(c0) - 1), int(numpy.floor(c)), -1)
                    for ci in rc:
                        k   = (ci - c0) / dc
                        ri  = r0 + k * dr
                        zis = zprev + k * dz
                        zp = self.z[int(numpy.ceil(ri)) - 1 + border : 
                                                int(ri) + 1 + border, 
                                    ci + border - 1 : ci + border + 1]
                        if len(zz) > 0:
                            z = max(list(zp[zp <= max(zz)]) + [min_z])
                        else:
                            z = numpy.max(zp)
                        part.append((ci, ri, max(z, zis, min_z)))
                else:
                    if r > r0:
                        rr = range(int(numpy.floor(r0)) + 1, int(numpy.ceil(r)))
                    else:
                        rr = range(int(numpy.ceil(r0) - 1), int(numpy.floor(r)), -1)
                    for ri in rr:
                        k   =  (ri - r0) / dr
                        ci  = c0 + k * dc
                        zis = zprev + k * dz
                        zp = self.z[ri + border - 1 : ri + border + 1, 
                                    int(numpy.ceil(ci)) - 1 + border : 
                                               int(ci) + 1 + border]
                        if len(zz) > 0:
                            z = max(list(zp[zp <= max(zz)]) + [min_z])
                        else:
                            z = numpy.max(zp)
                        part.append((ci, ri, max(z, zis, min_z)))
                part.append(nodes[i][0:2] + (max(z, zs, min_z),) + nodes[i][3:])
                # Optimization
                answer.extend(self.optimize(part, tolerance))
            zprev = zs        
        return answer
      
    def optimize(self, part, tolerance):
        # recursive procedure. Returns optimized path without the first node.
        x0, y0, z0 = part[0][0:3]
        al = abs(x0 - part[-1][0]) + abs(y0 - part[-1][1])
        if al == 0:
            return []
        dzdal = (part[-1][2] - z0) / al
        max_dev = 0
        peak_ind = None
        for i in range(1, len(part)-1):
            x, y, z = part[i][0:3]
            ax = abs(x0 - x) + abs(y0 - y)
            ze = z0 + dzdal * ax
            if ze > z + tolerance + max_dev:
                max_dev = ze - z - tolerance
                peak_ind = i
            elif ze < z - max_dev:
                max_dev = z - ze
                peak_ind = i
        if peak_ind is None:
            return [part[-1]]
        if len(part) == 3:
            return part[1:]
        if peak_ind == 1:
            return ([part[peak_ind]] + 
                    self.optimize(part[peak_ind:-1], tolerance))
        if peak_ind == len(part)-2:
            return (self.optimize(part[0:peak_ind+1], tolerance) + 
                    part[peak_ind:-1])
        return (self.optimize(part[0:peak_ind+1], tolerance) + 
                self.optimize(part[peak_ind:-1], tolerance))
                
