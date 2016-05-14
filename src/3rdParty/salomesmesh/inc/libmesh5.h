

/*----------------------------------------------------------*/
/*                                                                                                                      */
/*                                              LIBMESH V 5.46                                          */
/*                                                                                                                      */
/*----------------------------------------------------------*/
/*                                                                                                                      */
/*      Description:            handle .meshb file format I/O           */
/*      Author:                         Loic MARECHAL                                           */
/*      Creation date:          feb 16 2007                                                     */
/*      Last modification:      dec 09 2011                                                     */
/*                                                                                                                      */
/*----------------------------------------------------------*/


/*----------------------------------------------------------*/
/* Defines                                                                                                      */
/*----------------------------------------------------------*/

#include "SMESH_DriverGMF.hxx"

#define GmfStrSiz 1024
#define GmfMaxTyp 1000
#define GmfMaxKwd 80
#define GmfMshVer 1
#define GmfRead 1
#define GmfWrite 2
#define GmfSca 1
#define GmfVec 2
#define GmfSymMat 3
#define GmfMat 4
#define GmfFloat 1
#define GmfDouble 2

enum GmfKwdCod
{
        GmfReserved1, \
        GmfVersionFormatted, \
        GmfReserved2, \
        GmfDimension, \
        GmfVertices, \
        GmfEdges, \
        GmfTriangles, \
        GmfQuadrilaterals, \
        GmfTetrahedra, \
        GmfPrisms, \
        GmfHexahedra, \
        GmfIterationsAll, \
        GmfTimesAll, \
        GmfCorners, \
        GmfRidges, \
        GmfRequiredVertices, \
        GmfRequiredEdges, \
        GmfRequiredTriangles, \
        GmfRequiredQuadrilaterals, \
        GmfTangentAtEdgeVertices, \
        GmfNormalAtVertices, \
        GmfNormalAtTriangleVertices, \
        GmfNormalAtQuadrilateralVertices, \
        GmfAngleOfCornerBound, \
        GmfTrianglesP2, \
        GmfEdgesP2, \
        GmfSolAtPyramids, \
        GmfQuadrilateralsQ2, \
        GmfISolAtPyramids, \
        GmfSubDomainFromGeom, \
        GmfTetrahedraP2, \
        GmfFault_NearTri, \
        GmfFault_Inter, \
        GmfHexahedraQ2, \
        GmfExtraVerticesAtEdges, \
        GmfExtraVerticesAtTriangles, \
        GmfExtraVerticesAtQuadrilaterals, \
        GmfExtraVerticesAtTetrahedra, \
        GmfExtraVerticesAtPrisms, \
        GmfExtraVerticesAtHexahedra, \
        GmfVerticesOnGeometricVertices, \
        GmfVerticesOnGeometricEdges, \
        GmfVerticesOnGeometricTriangles, \
        GmfVerticesOnGeometricQuadrilaterals, \
        GmfEdgesOnGeometricEdges, \
        GmfFault_FreeEdge, \
        GmfPolyhedra, \
        GmfPolygons, \
        GmfFault_Overlap, \
        GmfPyramids, \
        GmfBoundingBox, \
        GmfBody, \
        GmfPrivateTable, \
        GmfFault_BadShape, \
        GmfEnd, \
        GmfTrianglesOnGeometricTriangles, \
        GmfTrianglesOnGeometricQuadrilaterals, \
        GmfQuadrilateralsOnGeometricTriangles, \
        GmfQuadrilateralsOnGeometricQuadrilaterals, \
        GmfTangents, \
        GmfNormals, \
        GmfTangentAtVertices, \
        GmfSolAtVertices, \
        GmfSolAtEdges, \
        GmfSolAtTriangles, \
        GmfSolAtQuadrilaterals, \
        GmfSolAtTetrahedra, \
        GmfSolAtPrisms, \
        GmfSolAtHexahedra, \
        GmfDSolAtVertices, \
        GmfISolAtVertices, \
        GmfISolAtEdges, \
        GmfISolAtTriangles, \
        GmfISolAtQuadrilaterals, \
        GmfISolAtTetrahedra, \
        GmfISolAtPrisms, \
        GmfISolAtHexahedra, \
        GmfIterations, \
        GmfTime, \
        GmfFault_SmallTri, \
        GmfCoarseHexahedra
};


/*----------------------------------------------------------*/
/* External procedures                                                                          */
/*----------------------------------------------------------*/

MESHDriverGMF_EXPORT extern int GmfOpenMesh(const char *, int, ...);
MESHDriverGMF_EXPORT extern int GmfCloseMesh(int);
MESHDriverGMF_EXPORT extern int GmfStatKwd(int, int, ...);
MESHDriverGMF_EXPORT extern int GmfGotoKwd(int, int);
MESHDriverGMF_EXPORT extern int GmfSetKwd(int, int, ...);
MESHDriverGMF_EXPORT extern void GmfGetLin(int, int, ...);
MESHDriverGMF_EXPORT extern void GmfSetLin(int, int, ...);


/*----------------------------------------------------------*/
/* Fortran 77 API                                                                                       */
/*----------------------------------------------------------*/

#if defined(F77_NO_UNDER_SCORE)
#define call(x) x
#else
#define call(x) x ## _
#endif


/*----------------------------------------------------------*/
/* Transmesh private API                                                                        */
/*----------------------------------------------------------*/

#ifdef TRANSMESH

MESHDriverGMF_EXPORT extern char *GmfKwdFmt[ GmfMaxKwd + 1 ][4];
MESHDriverGMF_EXPORT extern int GmfCpyLin(int, int, int);

#endif
