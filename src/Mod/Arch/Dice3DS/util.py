# util.py

"""Utitily function for Dice3DS.

Defines some routines for calculating normals and transforming points.

"""

import numpy
import sys

# xrange is not available in python3
if sys.version_info.major >= 3:
    xrange = range

# Can push numpy.float64 (or even numpy.float80) into this if you
# would like to use higher precision when calculating; results will be
# converted back to numpy.float32
_calc_precision_type = numpy.float32


def translate_points(pointarray,matrix):
    """Translate points in pointarray by the given matrix.

        tpointarray = translate_points(pointarray,matrix)

    Takes array of points and a homogeneous (4D) transformation
    matrix in exactly the same form in which they appear in the
    3DS DOM.

    Returns a pointarray with the points transformed by the matrix.

    """

    n = len(pointarray)
    pt = numpy.ones((n,4),_calc_precision_type)
    pt[:,:3] = pointarray
    tpt = numpy.transpose(numpy.dot(matrix,numpy.transpose(pt)))
    return numpy.asarray(tpt[:,:3]/tpt[:,3:4],numpy.float32)



def calculate_normals_no_smoothing(pointarray,facearray,smarray=None):
    """Calculate normals all perpendicular to the faces.

        points,norms = calculate_normals_no_smoothing(
                pointarray,facearray,smarray=None)

    Takes an array of points and faces in exactly the same form in
    which they appear in the 3DS DOM.  It accepts a smoothing array,
    but ignores it.

    Returns a numpy.array of points, one per row, and a
    numpy.array of the corresponding normals.  The points are
    returned as a list of consecutive triangles; the first three rows
    make up the first triangle, the second three rows make up the
    second triangle, and so on.

    The normal vectors are determined by calculating the normal to
    each face.  There is no smoothing.

    """

    # prepare to calculate normals. define some arrays

    m = len(facearray)
    fnorms = numpy.empty((m*3,3),_calc_precision_type)
    points = pointarray[facearray.ravel()]

    # calculate normals for each face

    A = numpy.asarray(pointarray[facearray[:,0]],_calc_precision_type)
    B = numpy.asarray(pointarray[facearray[:,1]],_calc_precision_type)
    C = numpy.asarray(pointarray[facearray[:,2]],_calc_precision_type)
    b = A - C
    c = B - A
    fnorms[2::3,0] = c[:,2]*b[:,1]-c[:,1]*b[:,2]
    fnorms[2::3,1] = c[:,0]*b[:,2]-c[:,2]*b[:,0]
    fnorms[2::3,2] = c[:,1]*b[:,0]-c[:,0]*b[:,1]
    a = fnorms[2::3]
    q = numpy.maximum(numpy.sqrt(numpy.sum(a*a,axis=1)),1e-10)
    q = q[:,numpy.newaxis]
    a /= q
    fnorms[0::3] = fnorms[1::3] = fnorms[2::3]

    # we're done

    return points, numpy.asarray(fnorms,numpy.float32)



def calculate_normals_by_cross_product(pointarray,facearray,smarray):
    """Calculate normals by smoothing, weighting by cross-product.

        points,norms = calculate_normals_by_cross_product(
                pointarray,facearray,smarray)

    Takes an array of points, faces, and a smoothing group in exactly
    the same form in which they appear in the 3DS DOM.

    Returns a numpy.array of points, one per row, and a numpy.array of
    the corresponding normals.  The points are returned as a list of
    consecutive triangles; the first three rows make up the first
    triangle, the second three rows make up the second triangle, and
    so on.

    To calculate the normal of a given vertex on a given face, this
    function averages the normal vector for all faces which have share
    that vertex and a smoothing group.

    The normals being averaged are weighted by the cross-product used
    to obtain the face's normal, which is proportional to the area of
    the face.

    """

    # prepare to calculate normals. define some arrays

    m = len(facearray)
    rnorms = numpy.zeros((m*3,3),_calc_precision_type)
    fnorms = numpy.zeros((m*3,3),_calc_precision_type)
    points = pointarray[facearray.ravel()]
    exarray = numpy.zeros(3*m,numpy.uint32)
    if smarray is not None:
        exarray[0::3] = exarray[1::3] = exarray[2::3] = smarray

    # calculate scaled normals (according to angle subtended)

    A = numpy.asarray(pointarray[facearray[:,0]],_calc_precision_type)
    B = numpy.asarray(pointarray[facearray[:,1]],_calc_precision_type)
    C = numpy.asarray(pointarray[facearray[:,2]],_calc_precision_type)
    a = C - B
    b = A - C
    c = B - A
    rnorms[0::3,0] = c[:,2]*b[:,1]-c[:,1]*b[:,2]
    rnorms[0::3,1] = c[:,0]*b[:,2]-c[:,2]*b[:,0]
    rnorms[0::3,2] = c[:,1]*b[:,0]-c[:,0]*b[:,1]
    rnorms[1::3,0] = a[:,2]*c[:,1]-a[:,1]*c[:,2]
    rnorms[1::3,1] = a[:,0]*c[:,2]-a[:,2]*c[:,0]
    rnorms[1::3,2] = a[:,1]*c[:,0]-a[:,0]*c[:,1]
    rnorms[2::3,0] = b[:,2]*a[:,1]-b[:,1]*a[:,2]
    rnorms[2::3,1] = b[:,0]*a[:,2]-b[:,2]*a[:,0]
    rnorms[2::3,2] = b[:,1]*a[:,0]-b[:,0]*a[:,1]

    # normalize vectors according to passed in smoothing group

    lex = numpy.lexsort(numpy.transpose(points))
    brs = numpy.nonzero(
        numpy.any(points[lex[1:],:]-points[lex[:-1],:],axis=1))[0]+1
    lslice = numpy.empty((len(brs)+1,),numpy.int)
    lslice[0] = 0
    lslice[1:] = brs
    rslice = numpy.empty((len(brs)+1,),numpy.int)
    rslice[:-1] = brs
    rslice[-1] = 3*m
    for i in xrange(len(brs)+1):
        rgroup = lex[lslice[i]:rslice[i]]
        xgroup = exarray[rgroup]
        normpat = numpy.logical_or(
            numpy.bitwise_and.outer(xgroup,xgroup),
            numpy.eye(len(xgroup)))
        fnorms[rgroup,:] = numpy.dot(normpat,rnorms[rgroup,:])
    q = numpy.sum(fnorms*fnorms,axis=1)
    qnz = numpy.nonzero(q)[0]
    lq = 1.0 / numpy.sqrt(q[qnz])
    fnt = numpy.transpose(fnorms)
    fnt[:,qnz] *= lq

    # we're done

    return points, numpy.asarray(fnorms,numpy.float32)



def calculate_normals_by_angle_subtended(pointarray,facearray,smarray):
    """Calculate normals by smoothing, weighting by angle subtended.

        points,norms = calculate_normals_by_angle_subtended(
                pointarray,facearray,smarray)

    Takes an array of points, faces, and a smoothing group in exactly
    the same form in which they appear in the 3DS DOM.

    Returns a numpy.array of points, one per row, and a numpy.array of
    the corresponding normals.  The points are returned as a list of
    consecutive triangles; the first three rows make up the first
    triangle, the second three rows make up the second triangle, and
    so on.

    To calculate the normal of a given vertex on a given face, this
    function averages the normal vector for all faces which have share
    that vertex, and a smoothing group.

    The normals being averaged are weighted by the angle subtended.

    """

    # prepare to calculate normals. define some arrays

    m = len(facearray)
    rnorms = numpy.zeros((m*3,3),_calc_precision_type)
    fnorms = numpy.zeros((m*3,3),_calc_precision_type)
    points = pointarray[facearray.ravel()]
    exarray = numpy.zeros(3*m,numpy.uint32)
    if smarray is not None:
        exarray[0::3] = exarray[1::3] = exarray[2::3] = smarray

    # weed out degenerate triangles first
    # unlike cross-product, angle subtended blows up on degeneracy

    A = numpy.asarray(pointarray[facearray[:,0]],_calc_precision_type)
    B = numpy.asarray(pointarray[facearray[:,1]],_calc_precision_type)
    C = numpy.asarray(pointarray[facearray[:,2]],_calc_precision_type)
    a = C - B
    b = A - C
    c = B - A
    p = numpy.zeros((len(facearray),3),_calc_precision_type)
    p[:,0] = c[:,2]*b[:,1]-c[:,1]*b[:,2]
    p[:,1] = c[:,0]*b[:,2]-c[:,2]*b[:,0]
    p[:,2] = c[:,1]*b[:,0]-c[:,0]*b[:,1]
    aa = numpy.sum(a*a,axis=1)
    bb = numpy.sum(b*b,axis=1)
    cc = numpy.sum(c*c,axis=1)
    pp = numpy.sum(p*p,axis=1)
    ndg = numpy.nonzero(numpy.logical_and.reduce((aa,bb,cc,pp)))[0]

    # calculate scaled normals (according to angle subtended)

    p = p[ndg]
    la = numpy.sqrt(aa[ndg])
    lb = numpy.sqrt(bb[ndg])
    lc = numpy.sqrt(cc[ndg])
    lp = numpy.sqrt(pp[ndg])
    sinA = numpy.clip(lp/lb/lc,-1.0,1.0)
    sinB = numpy.clip(lp/la/lc,-1.0,1.0)
    sinC = numpy.clip(lp/la/lb,-1.0,1.0)
    sinA2 = sinA*sinA
    sinB2 = sinB*sinB
    sinC2 = sinC*sinC
    angA = numpy.arcsin(sinA)
    angB = numpy.arcsin(sinB)
    angC = numpy.arcsin(sinC)
    angA = numpy.where(sinA2 > sinB2 + sinC2, numpy.pi - angA, angA)
    angB = numpy.where(sinB2 > sinA2 + sinC2, numpy.pi - angB, angB)
    angC = numpy.where(sinC2 > sinA2 + sinB2, numpy.pi - angC, angC)
    rnorms[0::3][ndg] = p*(angA/lp)[:,numpy.newaxis]
    rnorms[1::3][ndg] = p*(angB/lp)[:,numpy.newaxis]
    rnorms[2::3][ndg] = p*(angC/lp)[:,numpy.newaxis]

    # normalize vectors according to passed in smoothing group

    lex = numpy.lexsort(numpy.transpose(points))
    brs = numpy.nonzero(
        numpy.any(points[lex[1:],:]-points[lex[:-1],:],axis=1))[0]+1
    lslice = numpy.empty((len(brs)+1,),numpy.int)
    lslice[0] = 0
    lslice[1:] = brs
    rslice = numpy.empty((len(brs)+1,),numpy.int)
    rslice[:-1] = brs
    rslice[-1] = 3*m
    for i in xrange(len(brs)+1):
        rgroup = lex[lslice[i]:rslice[i]]
        xgroup = exarray[rgroup]
        normpat = numpy.logical_or(
            numpy.bitwise_and.outer(xgroup,xgroup),
            numpy.eye(len(xgroup)))
        fnorms[rgroup,:] = numpy.dot(normpat,rnorms[rgroup,:])
    q = numpy.sum(fnorms*fnorms,axis=1)
    qnz = numpy.nonzero(q)[0]
    lq = 1.0 / numpy.sqrt(q[qnz])
    fnt = numpy.transpose(fnorms)
    fnt[:,qnz] *= lq

    # we're done

    return points, numpy.asarray(fnorms,numpy.float32)
