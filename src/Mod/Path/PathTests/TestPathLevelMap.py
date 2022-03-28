# This is a standalone test, use it from command line.
# This test will be rewritten as an automatic one and will be moved into the appropriate directory.
# For visualization of test results the appropriate scilab scripts should be used.
import PathLevelMap
import math
import numpy
import random
import time

class TestPathLevelMap():
    def sphere(self, x0, y0, z0, r, n ):
        # r - radius
        # returns list of facets, n rows, 2 * n facets each
        # as a list of (pa, pb, pc) where pa, pb, pc are (x, y, z)
        answer = []
        prev = [(0, 0, -r)] * n
        for par in range(1, n + 1):
            lat = math.pi * (par / n - 0.5)
            z = r * math.sin( lat )
            rp = r * math.cos( lat )
            
            pp = []
            for mer in range(0, n):
                lon = 2 * math.pi * (mer + par * 0.5) / n
                pp.append(( rp * math.cos(lon), rp * math.sin(lon), z))
            
            for i in range(0, n):
                if par != n:
                    answer.append((pp[i-1], pp[i], prev[i]))
                if par != 1:
                    answer.append((prev[i-1], pp[i-1], prev[i]))
                
            prev = pp
        return answer

    def add_sphere(self, radius, base_level, sample_interval, granularity):
        # Returns (level_map, facets, dt, up_err, down_err) where
        #   level_map -- a LevelMap with a sphere placed at 0,0,0.
        #   facets    -- list of (pa, pb, pc) where pa, pb, pc are (x, y, z)
        #   dt        -- time of execution in seconds
        #   up_err,
        #   down_err  -- distance between the level map and a true sphere in
        #                a positive and negative directions
        # The spere is tessellated to 2*granularity**2 facets.
        side = max(10, 1.5 * radius)
        lm = PathLevelMap.LevelMap( -side, side, -side, side, 
                                    base_level, sample_interval, 10 )
        facets = self.sphere( 0, 0, 0, radius, granularity )
        t0 = time.time()
        for f in facets:
            lm.add_facet( f[0], f[1], f[2] )
        dt = time.time() - t0

        # Verification
        x = lm.xmin + numpy.arange( 0, lm.level().shape[1] ) * lm.sampleInterval
        y = lm.ymin + numpy.arange( 0, lm.level().shape[0] ) * lm.sampleInterval
        r = numpy.sqrt(x[None,:] * x[None,:] + y[:,None] * y[:,None])
        z = numpy.sqrt(numpy.abs(radius ** 2 - r * r)) 
        r_in  = radius * ( 0.9999 - math.cos(math.pi / granularity))
        r_out = radius + 1.5 * sample_interval
        delta = (numpy.where( r < r_in, lm.level() - z, 0 ) +
                 numpy.where( r > r_out, lm.level() - base_level, 0 ))
        down_err = max(0, - numpy.min(delta))
        up_err = max(0, numpy.max(delta))
                
        return (lm, facets, dt, up_err, down_err)        

    def level_map_to_file(self, radius, base_level, sample_interval, granularity,
                                file_name_for_facets, file_name_for_map ):
        lm, facets, dt, up_err, down_err = self.add_sphere(
                             radius, base_level, sample_interval, granularity)
        fp = open(file_name_for_facets, 'w')
        for f in facets:
            fp.write( ("%.2f, " * 8 + "%.2f\n") % (f[0] + f[1] + f[2]))
        fp.close

        fp = open(file_name_for_map, 'w')
        for i in range(0, lm.z.shape[0]):
            fp.write( ', '.join(['%.3f' % z for z in lm.z[:,i]]) + "\n")
        fp.close

    def test_level_map(self, radiuses, n_random, granularities, verbose=False ):
        # make n_random probes near each radius in radiuses
        # return (err, r, t) where
        #    err -- list of radiuses where algorithm failed, should be []
        #    r   -- list of tested radiuses
        #    t   -- list of execution time for each radius
        err = []
        r   = []
        t   = []
        if verbose:
            print("radius gran  time  up_err  down_err")
        for radius in radiuses:
            for gran in granularities:
                ri = radius
                for n in range(0, n_random): 
                    level_map, facets, ti, up_err, down_err = self.add_sphere(
                        ri, -9.0, 0.1, gran)
                    r.append( ri ) 
                    t.append( ti )
                    if up_err > 0.01 or down_err > ri * ( 1.001 - math.cos(math.pi / gran)):
                        err.append(ri)
                        if verbose:
                            print( "---------" )
                    if verbose:
                        print("%6.2f %i %7.3f %.3f  %.3f" % (ri, gran, ti, up_err, down_err))
                    ri = radius + random.random() * radius * 0.1
        return (err, r, t)

    def coverage_to_file(self, border, rt, file_name, errors=[] ):
        # errors is the list of [j, i, delta]
        lm = PathLevelMap.LevelMap( -15, 15, -15, 15, -9, 0.1, border )
        border = lm.border
        job = []
        maxcol = max(border, (2000 - border) // 16 * 16)  # to fit in L1 cache
        R, C = lm.z.shape
        partial = [numpy.empty((R, min(maxcol + 2 * border, C)))]
        lm._create_coverage(job, partial, rt)
        fp = open(file_name, 'w')
        fp.write( "0, -1, %i, %.3f\n" % (border, rt))
        for ji in job:
            fp.write( "%i, %i, %i, %.3f\n" % ji )
        for err in errors:
            fp.write( "%i, -2, %i, %.3f\n" % tuple(err))
        fp.close
        
    def test_coverage(self, rt ):
        # Create a single notch and build the surface for endmill with radius rt.
        # Check if there are holes or peaks inside and outside the circle with radius rt.
        # Return True if there are no defects.
        side = max(10, 1.5 * rt)
        border = int(rt / 0.1) + 2
        lm = PathLevelMap.LevelMap( -side, side, -side, side, -10, 0.1, border )
        ic = lm.level().shape[0] // 2
        lm.level()[ic,ic] = 5
        lm.applyTool(rt, None)
        
        # Verification
        x = (numpy.arange( 0, lm.level().shape[1] ) - ic)
        xa = numpy.maximum(numpy.abs(x) - 1, 0) * lm.sampleInterval
        r = numpy.sqrt(xa[None,:] * xa[None,:] + xa[:,None] * xa[:,None])
        r_in  = rt - 0.01
        r_out = rt + 0.105
        delta = (numpy.where( r < r_in, lm.level() - 5, 0 ) +
                 numpy.where( r > r_out, lm.level() - -10, 0 ))
        err = numpy.max(numpy.abs(delta))
        if err > 0:
            j,i=numpy.where(numpy.abs(delta) > 0)
            self.coverage_to_file(border, rt * 10.0, "coverage_errors.csv",
                                  numpy.hstack((j[:,None]-ic,i[:,None]-ic,delta[j,i][:,None])))
        return err == 0

    def test_coverages(self, radiuses, n_random):
        OK = True
        for radius in radiuses:
            ri = radius
            for n in range(0, n_random):
                if not self.test_coverage( ri ):
                    print( "Error" )
                    OK = False
                    break
                ri = radius + random.random() * 0.1
            if not OK:
                break
        return OK

if __name__ == "__main__":
    #TestPathLevelMap().test_level_map( [2, 3, 10, 30, 77, 150, 670], 10, [10, 30, 100], True )
    print( TestPathLevelMap().test_coverages( [1.0043], 500 ))    
    print( TestPathLevelMap().test_coverages( [n * 0.1 for n in range(1,100)], 50 ))
