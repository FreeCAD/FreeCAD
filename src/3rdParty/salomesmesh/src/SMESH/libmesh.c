

/*----------------------------------------------------------*/
/*                                                                                                                      */
/*                                              LIBMESH V 5.46                                          */
/*                                                                                                                      */
/*----------------------------------------------------------*/
/*                                                                                                                      */
/*      Description:            handle .meshb file format I/O           */
/*      Author:                         Loic MARECHAL                                           */
/*      Creation date:          feb 16 2007                                                     */
/*      Last modification:      apr 03 2012                                                     */
/*                                                                                                                      */
/*----------------------------------------------------------*/


/*----------------------------------------------------------*/
/* Includes                                                                                                     */
/*----------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <ctype.h>
#include "libmesh5.h"


/*----------------------------------------------------------*/
/* Defines                                                                                                      */
/*----------------------------------------------------------*/

#define Asc 1
#define Bin 2
#define MshFil 4
#define SolFil 8
#define MaxMsh 100
#define InfKwd 1
#define RegKwd 2
#define SolKwd 3
#define WrdSiz 4
#define BufSiz 10000


/*----------------------------------------------------------*/
/* Structures                                                                                           */
/*----------------------------------------------------------*/

typedef struct
{
        int typ, SolSiz, NmbWrd, NmbLin, NmbTyp, TypTab[ GmfMaxTyp ];
        long pos;
        char fmt[ GmfMaxTyp*9 ];
}KwdSct;

typedef struct
{
        int dim, ver, mod, typ, cod, pos;
        long NexKwdPos, siz;
        KwdSct KwdTab[ GmfMaxKwd + 1 ];
        FILE *hdl;
        int *IntBuf;
        float *FltBuf;
        unsigned char *buf;
        char FilNam[ GmfStrSiz ];
        double DblBuf[1000/8];
        unsigned char blk[ BufSiz + 1000 ];
}GmfMshSct;


/*----------------------------------------------------------*/
/* Global variables                                                                                     */
/*----------------------------------------------------------*/

static int GmfIniFlg=0;
static GmfMshSct *GmfMshTab[ MaxMsh + 1 ];
static const char *GmfKwdFmt[ GmfMaxKwd + 1 ][4] = 
{       {"Reserved", "", "", ""},
        {"MeshVersionFormatted", "", "", "i"},
        {"Reserved", "", "", ""},
        {"Dimension", "", "", "i"},
        {"Vertices", "Vertex", "i", "dri"},
        {"Edges", "Edge", "i", "iii"},
        {"Triangles", "Triangle", "i", "iiii"},
        {"Quadrilaterals", "Quadrilateral", "i", "iiiii"},
        {"Tetrahedra", "Tetrahedron", "i", "iiiii"},
        {"Prisms", "Prism", "i", "iiiiiii"},
        {"Hexahedra", "Hexahedron", "i", "iiiiiiiii"},
        {"IterationsAll", "IterationAll","","i"},
        {"TimesAll", "TimeAll","","r"},                                 
        {"Corners", "Corner", "i", "i"},
        {"Ridges", "Ridge", "i", "i"},
        {"RequiredVertices", "RequiredVertex", "i", "i"},
        {"RequiredEdges", "RequiredEdge", "i", "i"},
        {"RequiredTriangles", "RequiredTriangle", "i", "i"},
        {"RequiredQuadrilaterals", "RequiredQuadrilateral", "i", "i"},
        {"TangentAtEdgeVertices", "TangentAtEdgeVertex", "i", "iii"},
        {"NormalAtVertices", "NormalAtVertex", "i", "ii"},
        {"NormalAtTriangleVertices", "NormalAtTriangleVertex", "i", "iii"},
        {"NormalAtQuadrilateralVertices", "NormalAtQuadrilateralVertex", "i", "iiii"},
        {"AngleOfCornerBound", "", "", "r"},
        {"TrianglesP2", "TriangleP2", "i", "iiiiiii"},
        {"EdgesP2", "EdgeP2", "i", "iiii"},
        {"SolAtPyramids", "SolAtPyramid", "i", "sr"},
        {"QuadrilateralsQ2", "QuadrilateralQ2", "i", "iiiiiiiiii"},
        {"ISolAtPyramids", "ISolAtPyramid", "i", "iiiii"},
        {"SubDomainFromGeom", "SubDomainFromGeom", "i", "iiii"},
        {"TetrahedraP2", "TetrahedronP2", "i", "iiiiiiiiiii"},
        {"Fault_NearTri", "Fault_NearTri", "i", "i"},
        {"Fault_Inter", "Fault_Inter", "i", "i"},
        {"HexahedraQ2", "HexahedronQ2", "i", "iiiiiiiiiiiiiiiiiiiiiiiiiiii"},
        {"ExtraVerticesAtEdges", "ExtraVerticesAtEdge", "i", "in"},
        {"ExtraVerticesAtTriangles", "ExtraVerticesAtTriangle", "i", "in"},
        {"ExtraVerticesAtQuadrilaterals", "ExtraVerticesAtQuadrilateral", "i", "in"},
        {"ExtraVerticesAtTetrahedra", "ExtraVerticesAtTetrahedron", "i", "in"},
        {"ExtraVerticesAtPrisms", "ExtraVerticesAtPrism", "i", "in"},
        {"ExtraVerticesAtHexahedra", "ExtraVerticesAtHexahedron", "i", "in"},
        {"VerticesOnGeometricVertices", "VertexOnGeometricVertex", "i", "iir"},
        {"VerticesOnGeometricEdges", "VertexOnGeometricEdge", "i", "iirr"},
        {"VerticesOnGeometricTriangles", "VertexOnGeometricTriangle", "i", "iirrr"},
        {"VerticesOnGeometricQuadrilaterals", "VertexOnGeometricQuadrilateral", "i", "iirrr"},
        {"EdgesOnGeometricEdges", "EdgeOnGeometricEdge", "i", "iir"},
        {"Fault_FreeEdge", "Fault_FreeEdge", "i", "i"},
        {"Polyhedra", "Polyhedron", "i", "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"},
        {"Polygons", "Polygon", "", "iiiiiiiii"},
        {"Fault_Overlap", "Fault_Overlap", "i", "i"},
        {"Pyramids", "Pyramid", "i", "iiiiii"},
        {"BoundingBox", "", "", "drdr"},
        {"Body","i", "drdrdrdr"},
        {"PrivateTable", "PrivateTable", "i", "i"},
        {"Fault_BadShape", "Fault_BadShape", "i", "i"},
        {"End", "", "", ""},
        {"TrianglesOnGeometricTriangles", "TriangleOnGeometricTriangle", "i", "iir"},
        {"TrianglesOnGeometricQuadrilaterals", "TriangleOnGeometricQuadrilateral", "i", "iir"},
        {"QuadrilateralsOnGeometricTriangles", "QuadrilateralOnGeometricTriangle", "i", "iir"},
        {"QuadrilateralsOnGeometricQuadrilaterals", "QuadrilateralOnGeometricQuadrilateral", "i", "iir"},
        {"Tangents", "Tangent", "i", "dr"},
        {"Normals", "Normal", "i", "dr"},
        {"TangentAtVertices", "TangentAtVertex", "i", "ii"},
        {"SolAtVertices", "SolAtVertex", "i", "sr"},
        {"SolAtEdges", "SolAtEdge", "i", "sr"},
        {"SolAtTriangles", "SolAtTriangle", "i", "sr"},
        {"SolAtQuadrilaterals", "SolAtQuadrilateral", "i", "sr"},
        {"SolAtTetrahedra", "SolAtTetrahedron", "i", "sr"},
        {"SolAtPrisms", "SolAtPrism", "i", "sr"},
        {"SolAtHexahedra", "SolAtHexahedron", "i", "sr"},
        {"DSolAtVertices", "DSolAtVertex", "i", "sr"},
        {"ISolAtVertices", "ISolAtVertex", "i", "i"},
        {"ISolAtEdges", "ISolAtEdge", "i", "ii"},
        {"ISolAtTriangles", "ISolAtTriangle", "i", "iii"},
        {"ISolAtQuadrilaterals", "ISolAtQuadrilateral", "i", "iiii"},
        {"ISolAtTetrahedra", "ISolAtTetrahedron", "i", "iiii"},
        {"ISolAtPrisms", "ISolAtPrism", "i", "iiiiii"},
        {"ISolAtHexahedra", "ISolAtHexahedron", "i", "iiiiiiii"},
        {"Iterations", "","","i"},
        {"Time", "","","r"},
        {"Fault_SmallTri", "Fault_SmallTri","i","i"},
        {"CoarseHexahedra", "CoarseHexahedron", "i", "i"}
 };


/*----------------------------------------------------------*/
/* Prototypes of local procedures                                                       */
/*----------------------------------------------------------*/

static void ScaWrd(GmfMshSct *, unsigned char *);
static void ScaDblWrd(GmfMshSct *, unsigned char *);
static void ScaBlk(GmfMshSct *, unsigned char *, int);
static long GetPos(GmfMshSct *);
static void RecWrd(GmfMshSct *, unsigned char *);
static void RecDblWrd(GmfMshSct *, unsigned char *);
static void RecBlk(GmfMshSct *, unsigned char *, int);
static void SetPos(GmfMshSct *, long);
static int ScaKwdTab(GmfMshSct *);
static void ExpFmt(GmfMshSct *, int);
static void ScaKwdHdr(GmfMshSct *, int);


/*----------------------------------------------------------*/
/* Open a mesh file in read or write mod                                        */
/*----------------------------------------------------------*/

int GmfOpenMesh(const char *FilNam, int mod, ...)
{
        int i, KwdCod, res, *PtrVer, *PtrDim, MshIdx=0;
        char str[ GmfStrSiz ];
        va_list VarArg;
        GmfMshSct *msh;
        char *ptr;
        int k;

        if(!GmfIniFlg)
        {
                for(i=0;i<=MaxMsh;i++)
                        GmfMshTab[i] = NULL;

                GmfIniFlg = 1;
        }

        /*---------------------*/
        /* MESH STRUCTURE INIT */
        /*---------------------*/

        for(i=1;i<=MaxMsh;i++)
                if(!GmfMshTab[i])
                {
                        MshIdx = i;
                        break;
                }

        if( !MshIdx || !(msh = (GmfMshSct *)calloc(1, sizeof(GmfMshSct))) )
                return(0);

        /* Copy the FilNam into the structure */

        if(strlen(FilNam) + 7 >= GmfStrSiz)
        {
                free (msh);
                return(0);
        }

        strcpy(msh->FilNam, FilNam);

        /* Store the opening mod (read or write) and guess the filetype (binary or ascii) depending on the extension */

        msh->mod = mod;
        msh->buf = (unsigned char *)msh->DblBuf;
        msh->FltBuf = (float *)msh->DblBuf;
        msh->IntBuf = (int *)msh->DblBuf;

        k = strlen(msh->FilNam) - 6;
        if(k < 0)
                k = 0;
        ptr = msh->FilNam+k;
        if(strstr(ptr, ".meshb"))
                msh->typ |= (Bin | MshFil);
        else if(strstr(ptr, ".mesh"))
                msh->typ |= (Asc | MshFil);
        else if(strstr(ptr, ".solb"))
                msh->typ |= (Bin | SolFil);
        else if(strstr(ptr, ".sol"))
                msh->typ |= (Asc | SolFil);
        else {
                free (msh);
                return(0);
        }

        /* Open the file in the required mod and initialize the mesh structure */

        if(msh->mod == GmfRead)
        {

                /*-----------------------*/
                /* OPEN FILE FOR READING */
                /*-----------------------*/

                va_start(VarArg, mod);
                PtrVer = va_arg(VarArg, int *);
                PtrDim = va_arg(VarArg, int *);
                va_end(VarArg);

                /* Create the name string and open the file */

                if(!(msh->hdl = fopen(msh->FilNam, "rb")))
                {
                        free (msh);
                        return(0);
                }

                /* Read the endian coding tag, the mesh version and the mesh dimension (mandatory kwd) */

                if(msh->typ & Bin)
                {
                        fread((unsigned char *)&msh->cod, WrdSiz, 1, msh->hdl);

                        if( (msh->cod != 1) && (msh->cod != 16777216) )
                        {
                                free (msh);
                                return(0);
                        }

                        ScaWrd(msh, (unsigned char *)&msh->ver);

                        if( (msh->ver < 1) || (msh->ver > 3) )
                        {
                                free (msh);
                                return(0);
                        }

                        if( (msh->ver == 3) && (sizeof(long) == 4) )
                        {
                                free (msh);
                                return(0);
                        }

                        ScaWrd(msh, (unsigned char *)&KwdCod);

                        if(KwdCod != GmfDimension)
                        {
                                free (msh);
                                return(0);
                        }

                        GetPos(msh);
                        ScaWrd(msh, (unsigned char *)&msh->dim);
                }
                else
                {
                        do
                        {
                                res = fscanf(msh->hdl, "%s", str);
                        }while( (res != EOF) && strcmp(str, "MeshVersionFormatted") );

                        if(res == EOF)
                        {
                                free (msh);
                                return(0);
                        }

                        fscanf(msh->hdl, "%d", &msh->ver);

                        if( (msh->ver < 1) || (msh->ver > 3) )
                        {
                                free (msh);
                                return(0);
                        }

                        do
                        {
                                res = fscanf(msh->hdl, "%s", str);
                        }while( (res != EOF) && strcmp(str, "Dimension") );

                        if(res == EOF)
                        {
                                free (msh);
                                return(0);
                        }

                        fscanf(msh->hdl, "%d", &msh->dim);
                }

                if( (msh->dim != 2) && (msh->dim != 3) )
                {
                        free (msh);
                        return(0);
                }

                (*PtrVer) = msh->ver;
                (*PtrDim) = msh->dim;

                /*------------*/
                /* KW READING */
                /*------------*/

                /* Read the list of kw present in the file */

                if(!ScaKwdTab(msh))
                {
                        free (msh);
                        return(0);
                }

                GmfMshTab[ MshIdx ] = msh;

                return(MshIdx);
        }
        else if(msh->mod == GmfWrite)
        {

                /*-----------------------*/
                /* OPEN FILE FOR WRITING */
                /*-----------------------*/

                msh->cod = 1;

                /* Check if the user provided a valid version number and dimension */

                va_start(VarArg, mod);
                msh->ver = va_arg(VarArg, int);
                msh->dim = va_arg(VarArg, int);
                va_end(VarArg);

                if( (msh->ver < 1) || (msh->ver > 3) )
                {
                        free (msh);
                        return(0);
                }

                if( (msh->ver == 3) && (sizeof(long) == 4) )
                {
                        free (msh);
                        return(0);
                }

                if( (msh->dim != 2) && (msh->dim != 3) )
                {
                        free (msh);
                        return(0);
                }

                /* Create the mesh file */

                if(!(msh->hdl = fopen(msh->FilNam, "wb")))
                {
                        free (msh);
                        return(0);
                }

                GmfMshTab[ MshIdx ] = msh;


                /*------------*/
                /* KW WRITING */
                /*------------*/

                /* Write the mesh version and dimension */

                if(msh->typ & Asc)
                {
                        fprintf(msh->hdl, "%s %d\n\n", GmfKwdFmt[ GmfVersionFormatted ][0], msh->ver);
                        fprintf(msh->hdl, "%s %d\n", GmfKwdFmt[ GmfDimension ][0], msh->dim);
                }
                else
                {
                        RecWrd(msh, (unsigned char *)&msh->cod);
                        RecWrd(msh, (unsigned char *)&msh->ver);
                        GmfSetKwd(MshIdx, GmfDimension, 0);
                        RecWrd(msh, (unsigned char *)&msh->dim);
                }

                return(MshIdx);
        }
        else
        {
                free (msh);
                return(0);
        }
}


/*----------------------------------------------------------*/
/* Close a meshfile in the right way                                            */
/*----------------------------------------------------------*/

int GmfCloseMesh(int MshIdx)
{
        int res = 1;
        GmfMshSct *msh;

        if( (MshIdx < 1) || (MshIdx > MaxMsh) )
                return(0);

        msh = GmfMshTab[ MshIdx ];
        RecBlk(msh, msh->buf, 0);

        /* In write down the "End" kw in write mode */

        if(msh->mod == GmfWrite){
                if(msh->typ & Asc)
                        fprintf(msh->hdl, "\n%s\n", GmfKwdFmt[ GmfEnd ][0]);
                else
                        GmfSetKwd(MshIdx, GmfEnd, 0);
        }
        /* Close the file and free the mesh structure */

        if(fclose(msh->hdl))
                res = 0;

        free(msh);
        GmfMshTab[ MshIdx ] = NULL;

        return(res);
}


/*----------------------------------------------------------*/
/* Read the number of lines and set the position to this kwd*/
/*----------------------------------------------------------*/

int GmfStatKwd(int MshIdx, int KwdCod, ...)
{
        int i, *PtrNmbTyp, *PtrSolSiz, *TypTab;
        GmfMshSct *msh;
        KwdSct *kwd;
        va_list VarArg;

        if( (MshIdx < 1) || (MshIdx > MaxMsh) )
                return(0);

        msh = GmfMshTab[ MshIdx ];

        if( (KwdCod < 1) || (KwdCod > GmfMaxKwd) )
                return(0);

        kwd = &msh->KwdTab[ KwdCod ];

        if(!kwd->NmbLin)
                return(0);

        /* Read further arguments if this kw is a sol */

        if(kwd->typ == SolKwd)
        {
                va_start(VarArg, KwdCod);

                PtrNmbTyp = va_arg(VarArg, int *);
                *PtrNmbTyp = kwd->NmbTyp;

                PtrSolSiz = va_arg(VarArg, int *);
                *PtrSolSiz = kwd->SolSiz;

                TypTab = va_arg(VarArg, int *);

                for(i=0;i<kwd->NmbTyp;i++)
                        TypTab[i] = kwd->TypTab[i];

                va_end(VarArg);
        }

        return(kwd->NmbLin);
}


/*----------------------------------------------------------*/
/* Set the current file position to a given kwd                         */
/*----------------------------------------------------------*/

int GmfGotoKwd(int MshIdx, int KwdCod)
{
        GmfMshSct *msh;
        KwdSct *kwd;

        if( (MshIdx < 1) || (MshIdx > MaxMsh) )
                return(0);

        msh = GmfMshTab[ MshIdx ];

        if( (KwdCod < 1) || (KwdCod > GmfMaxKwd) )
                return(0);

        kwd = &msh->KwdTab[ KwdCod ];

        if(!kwd->NmbLin)
                return(0);

        return(fseek(msh->hdl, kwd->pos, SEEK_SET));
}


/*----------------------------------------------------------*/
/* Write the kwd and set the number of lines                            */
/*----------------------------------------------------------*/

int GmfSetKwd(int MshIdx, int KwdCod, ...)
{
        int i, NmbLin=0, *TypTab;
        long CurPos;
        va_list VarArg;
        GmfMshSct *msh;
        KwdSct *kwd;

        if( (MshIdx < 1) || (MshIdx > MaxMsh) )
                return(0);

        msh = GmfMshTab[ MshIdx ];
        RecBlk(msh, msh->buf, 0);

        if( (KwdCod < 1) || (KwdCod > GmfMaxKwd) )
                return(0);

        kwd = &msh->KwdTab[ KwdCod ];

        /* Read further arguments if this kw has a header */

        if(strlen(GmfKwdFmt[ KwdCod ][2]))
        {
                va_start(VarArg, KwdCod);
                NmbLin = va_arg(VarArg, int);

                if(!strcmp(GmfKwdFmt[ KwdCod ][3], "sr"))
                {
                        kwd->NmbTyp = va_arg(VarArg, int);
                        TypTab = va_arg(VarArg, int *);

                        for(i=0;i<kwd->NmbTyp;i++)
                                kwd->TypTab[i] = TypTab[i];
                }

                va_end(VarArg);
        }

        /* Setup the kwd info */

        ExpFmt(msh, KwdCod);

        if(!kwd->typ)
                return(0);
        else if(kwd->typ == InfKwd)
                kwd->NmbLin = 1;
        else
                kwd->NmbLin = NmbLin;

        /* Store the next kwd position in binary file */

        if( (msh->typ & Bin) && msh->NexKwdPos )
        {
                CurPos = ftell(msh->hdl);
                fseek(msh->hdl, msh->NexKwdPos, SEEK_SET);
                SetPos(msh, CurPos);
                fseek(msh->hdl, CurPos, SEEK_SET);
        }

        /* Write the header */

        if(msh->typ & Asc)
        {
                fprintf(msh->hdl, "\n%s\n", GmfKwdFmt[ KwdCod ][0]);

                if(kwd->typ != InfKwd)
                        fprintf(msh->hdl, "%d\n", kwd->NmbLin);

                /* In case of solution field, write the extended header */

                if(kwd->typ == SolKwd)
                {
                        fprintf(msh->hdl, "%d ", kwd->NmbTyp);

                        for(i=0;i<kwd->NmbTyp;i++)
                                fprintf(msh->hdl, "%d ", kwd->TypTab[i]);

                        fprintf(msh->hdl, "\n\n");
                }
        }
        else
        {
                RecWrd(msh, (unsigned char *)&KwdCod);
                msh->NexKwdPos = ftell(msh->hdl);
                SetPos(msh, 0);

                if(kwd->typ != InfKwd)
                        RecWrd(msh, (unsigned char *)&kwd->NmbLin);

                /* In case of solution field, write the extended header at once */

                if(kwd->typ == SolKwd)
                {
                        RecWrd(msh, (unsigned char *)&kwd->NmbTyp);

                        for(i=0;i<kwd->NmbTyp;i++)
                                RecWrd(msh, (unsigned char *)&kwd->TypTab[i]);
                }
        }

        /* Reset write buffer position */
        msh->pos = 0;

        /* Estimate the total file size and check whether it crosses the 2GB threshold */

        msh->siz += kwd->NmbLin * kwd->NmbWrd * WrdSiz;

        if(msh->siz > 2E9)
                return(0);
        else
                return(kwd->NmbLin);
}


/*----------------------------------------------------------*/
/* Read a full line from the current kwd                                        */
/*----------------------------------------------------------*/

void GmfGetLin(int MshIdx, int KwdCod, ...)
{
        int i, j;
        float *FltSolTab;
        double *DblSolTab;
        va_list VarArg;
        GmfMshSct *msh = GmfMshTab[ MshIdx ];
        KwdSct *kwd = &msh->KwdTab[ KwdCod ];

        /* Start decoding the arguments */

        va_start(VarArg, KwdCod);

        if(kwd->typ != SolKwd)
        {
                int k, nb_repeat = 0;

                if(msh->ver == 1)
                {
                        if(msh->typ & Asc)
                        {
                                for(i=0;i<kwd->SolSiz;i++)
                                        if(kwd->fmt[i] == 'r')
                                                fscanf(msh->hdl, "%f", va_arg(VarArg, float *));
                                        else if(kwd->fmt[i] == 'n') {
                                                fscanf(msh->hdl, "%d", &nb_repeat);
                                                *(va_arg(VarArg,  int *)) = nb_repeat;
                                                for(k=0;k<nb_repeat;k++)
                                                        fscanf(msh->hdl, "%d", va_arg(VarArg, int *));
                                        }
                                        else
                                                fscanf(msh->hdl, "%d", va_arg(VarArg, int *));
                        }
                        else
                        {
                                for(i=0;i<kwd->SolSiz;i++)
                                        if(kwd->fmt[i] == 'r')
                                                ScaWrd(msh, (unsigned char *)va_arg(VarArg, float *));
                                        else if(kwd->fmt[i] == 'n') {
                                                ScaWrd(msh, (unsigned char *)&nb_repeat);
                                                *(va_arg(VarArg,  int *)) = nb_repeat;
                                                for(k=0;k<nb_repeat;k++)
                                                        ScaWrd(msh, (unsigned char *)va_arg(VarArg, int *));
                                        }
                                        else
                                                ScaWrd(msh, (unsigned char *)va_arg(VarArg, int *));
                        }
                }
                else
                {
                        if(msh->typ & Asc)
                        {
                                for(i=0;i<kwd->SolSiz;i++)
                                        if(kwd->fmt[i] == 'r')
                                                fscanf(msh->hdl, "%lf", va_arg(VarArg, double *));
                                        else if(kwd->fmt[i] == 'n') {
                                                fscanf(msh->hdl, "%d", &nb_repeat);
                                                *(va_arg(VarArg,  int *)) = nb_repeat;
                                                for(k=0;k<nb_repeat;k++)
                                                        fscanf(msh->hdl, "%d", va_arg(VarArg, int *));
                                        }
                                        else
                                                fscanf(msh->hdl, "%d", va_arg(VarArg, int *));
                        }
                        else
                                for(i=0;i<kwd->SolSiz;i++)
                                        if(kwd->fmt[i] == 'r')
                                                ScaDblWrd(msh, (unsigned char *)va_arg(VarArg, double *));
                                        else if(kwd->fmt[i] == 'n') {
                                                ScaWrd(msh, (unsigned char *)&nb_repeat);
                                                *(va_arg(VarArg,  int *)) = nb_repeat;
                                                for(k=0;k<nb_repeat;k++)
                                                        ScaWrd(msh, (unsigned char *)va_arg(VarArg, int *));
                                        }
                                        else
                                                ScaWrd(msh, (unsigned char *)va_arg(VarArg, int *));
                }
        }
        else
        {
                if(msh->ver == 1)
                {
                        FltSolTab = va_arg(VarArg, float *);

                        if(msh->typ & Asc)
                                for(j=0;j<kwd->SolSiz;j++)
                                        fscanf(msh->hdl, "%f", &FltSolTab[j]);
                        else
                                ScaBlk(msh, (unsigned char *)FltSolTab, kwd->NmbWrd);
                }
                else
                {
                        DblSolTab = va_arg(VarArg, double *);

                        if(msh->typ & Asc)
                                for(j=0;j<kwd->SolSiz;j++)
                                        fscanf(msh->hdl, "%lf", &DblSolTab[j]);
                        else
                                for(j=0;j<kwd->SolSiz;j++)
                                        ScaDblWrd(msh, (unsigned char *)&DblSolTab[j]);
                }
        }

        va_end(VarArg);
}


/*----------------------------------------------------------*/
/* Write a full line from the current kwd                                       */
/*----------------------------------------------------------*/

void GmfSetLin(int MshIdx, int KwdCod, ...)
{
        int i, j, pos, *IntBuf;
        float *FltSolTab;
        double *DblSolTab, *DblBuf;
        va_list VarArg;
        GmfMshSct *msh = GmfMshTab[ MshIdx ];
        KwdSct *kwd = &msh->KwdTab[ KwdCod ];

        /* Start decoding the arguments */

        va_start(VarArg, KwdCod);

        if(kwd->typ != SolKwd)
        {
                int k, nb_repeat = 0;

                if(msh->ver == 1)
                {
                        if(msh->typ & Asc)
                        {
                                for(i=0;i<kwd->SolSiz;i++)
                                        if(kwd->fmt[i] == 'r')
                                                fprintf(msh->hdl, "%g ", (float)va_arg(VarArg, double));
                                        else if(kwd->fmt[i] == 'n') {
                                                nb_repeat = va_arg(VarArg, int);
                                                fprintf(msh->hdl, "%d ", nb_repeat);
                                                for(k=0;k<nb_repeat;k++)
                                                        fprintf(msh->hdl, "%d ", va_arg(VarArg, int));
                                        }
                                        else
                                                fprintf(msh->hdl, "%d ", va_arg(VarArg, int));
                        }
                        else
                        {
                                int size_of_block = kwd->SolSiz;
                                for(i=0;i<kwd->SolSiz;i++)
                                        if(kwd->fmt[i] == 'r')
                                                msh->FltBuf[i] = va_arg(VarArg, double);
                                        else if(kwd->fmt[i] == 'n') {
                                                nb_repeat = va_arg(VarArg, int);
                                                msh->FltBuf[i] = nb_repeat;
                                                for(k=0;k<nb_repeat;k++) {
                                                        msh->IntBuf[i+1+k] = va_arg(VarArg, int);
                                                        size_of_block ++;
                                                }
                                        }
                                        else
                                                msh->IntBuf[i] = va_arg(VarArg, int);

                                RecBlk(msh, msh->buf, size_of_block);
                        }
                }
                else
                {
                        if(msh->typ & Asc)
                        {
                                for(i=0;i<kwd->SolSiz;i++)
                                        if(kwd->fmt[i] == 'r')
                                                fprintf(msh->hdl, "%.15lg ", va_arg(VarArg, double));
                                        else if(kwd->fmt[i] == 'n') {
                                                nb_repeat = va_arg(VarArg, int);
                                                fprintf(msh->hdl, "%d ", nb_repeat);
                                                for(k=0;k<nb_repeat;k++)
                                                        fprintf(msh->hdl, "%d ", va_arg(VarArg, int));
                                        }
                                        else
                                                fprintf(msh->hdl, "%d ", va_arg(VarArg, int));
                        }
                        else
                        {
                                pos = 0;

                                for(i=0;i<kwd->SolSiz;i++)
                                        if(kwd->fmt[i] == 'r')
                                        {
                                                DblBuf = (double *)&msh->buf[ pos ];
                                                *DblBuf = va_arg(VarArg, double);
                                                pos += 8;
                                        }
                                        else if(kwd->fmt[i] == 'n')
                                        {
                                                IntBuf = (int *)&msh->buf[ pos ];
                                                nb_repeat = va_arg(VarArg, int);
                                                *IntBuf = nb_repeat;
                                                pos += 4;
                                                for(k=0;k<nb_repeat;k++) {
                                                        IntBuf = (int *)&msh->buf[ pos ];
                                                        *IntBuf = va_arg(VarArg, int);
                                                        pos += 4;
                                                }
                                        }
                                        else
                                        {
                                                IntBuf = (int *)&msh->buf[ pos ];
                                                *IntBuf = va_arg(VarArg, int);
                                                pos += 4;
                                        }
                                RecBlk(msh, msh->buf, pos/4);
                        }
                }
        }
        else
        {
                if(msh->ver == 1)
                {
                        FltSolTab = va_arg(VarArg, float *);

                        if(msh->typ & Asc)
                                for(j=0;j<kwd->SolSiz;j++)
                                        fprintf(msh->hdl, "%g ", FltSolTab[j]);
                        else
                                RecBlk(msh, (unsigned char *)FltSolTab, kwd->NmbWrd);
                }
                else
                {
                        DblSolTab = va_arg(VarArg, double *);

                        if(msh->typ & Asc)
                                for(j=0;j<kwd->SolSiz;j++)
                                        fprintf(msh->hdl, "%.15lg ", DblSolTab[j]);
                        else
                                RecBlk(msh, (unsigned char *)DblSolTab, kwd->NmbWrd);
                }
        }

        va_end(VarArg);

        if(msh->typ & Asc)
                fprintf(msh->hdl, "\n");
}


/*----------------------------------------------------------*/
/* Private procedure for transmesh : copy a whole line          */
/*----------------------------------------------------------*/

void GmfCpyLin(int InpIdx, int OutIdx, int KwdCod)
{
        double d;
        float f;
        int i, a;
        GmfMshSct *InpMsh = GmfMshTab[ InpIdx ], *OutMsh = GmfMshTab[ OutIdx ];
        KwdSct *kwd = &InpMsh->KwdTab[ KwdCod ];

        for(i=0;i<kwd->SolSiz;i++)
        {
                if(kwd->fmt[i] == 'r')
                {
                        if(InpMsh->ver == 1)
                        {
                                if(InpMsh->typ & Asc)
                                        fscanf(InpMsh->hdl, "%f", &f);
                                else
                                        ScaWrd(InpMsh, (unsigned char *)&f);

                                d = f;
                        }
                        else
                        {
                                if(InpMsh->typ & Asc)
                                        fscanf(InpMsh->hdl, "%lf", &d);
                                else
                                        ScaDblWrd(InpMsh, (unsigned char *)&d);

                                f = (float)d;
                        }

                        if(OutMsh->ver == 1)
                                if(OutMsh->typ & Asc)
                                        fprintf(OutMsh->hdl, "%g ", f);
                                else
                                        RecWrd(OutMsh, (unsigned char *)&f);
                        else
                                if(OutMsh->typ & Asc)
                                        fprintf(OutMsh->hdl, "%.15g ", d);
                                else
                                        RecDblWrd(OutMsh, (unsigned char *)&d);
                }
                else if(kwd->fmt[i] == 'n')
                {
                        int k, nb_repeat = 0;

                        if(InpMsh->typ & Asc)
                                fscanf(InpMsh->hdl, "%d", &a);
                        else
                                ScaWrd(InpMsh, (unsigned char *)&a);

                        nb_repeat = a;

                        if(OutMsh->typ & Asc)
                                fprintf(OutMsh->hdl, "%d ", a);
                        else
                                RecWrd(OutMsh, (unsigned char *)&a);

                        for(k=0;k<nb_repeat;k++) {
                                if(InpMsh->typ & Asc)
                                        fscanf(InpMsh->hdl, "%d", &a);
                                else
                                        ScaWrd(InpMsh, (unsigned char *)&a);

                                if(OutMsh->typ & Asc)
                                        fprintf(OutMsh->hdl, "%d ", a);
                                else
                                        RecWrd(OutMsh, (unsigned char *)&a);
                        }
                }
                else
                {
                        if(InpMsh->typ & Asc)
                                fscanf(InpMsh->hdl, "%d", &a);
                        else
                                ScaWrd(InpMsh, (unsigned char *)&a);

                        if(OutMsh->typ & Asc)
                                fprintf(OutMsh->hdl, "%d ", a);
                        else
                                RecWrd(OutMsh, (unsigned char *)&a);
                }
        }

        if(OutMsh->typ & Asc)
                fprintf(OutMsh->hdl, "\n");
}


/*----------------------------------------------------------*/
/* Find every kw present in a meshfile                                          */
/*----------------------------------------------------------*/

static int ScaKwdTab(GmfMshSct *msh)
{
        int KwdCod;
        long  NexPos, CurPos, EndPos;
        char str[ GmfStrSiz ];

        if(msh->typ & Asc)
        {
                /* Scan each string in the file until the end */

                while(fscanf(msh->hdl, "%s", str) != EOF)
                {
                        /* Fast test in order to reject quickly the numeric values */

                        if(isalpha(str[0]))
                        {
                                /* Search which kwd code this string is associated with, 
                                        then get its header and save the current position in file (just before the data) */

                                for(KwdCod=1; KwdCod<= GmfMaxKwd; KwdCod++)
                                        if(!strcmp(str, GmfKwdFmt[ KwdCod ][0]))
                                        {
                                                ScaKwdHdr(msh, KwdCod);
                                                break;
                                        }
                        }
                        else if(str[0] == '#')
                                while(fgetc(msh->hdl) != '\n');
                }
        }
        else
        {
                /* Get file size */

                CurPos = ftell(msh->hdl);
                fseek(msh->hdl, 0, SEEK_END);
                EndPos = ftell(msh->hdl);
                fseek(msh->hdl, CurPos, SEEK_SET);

                /* Jump through kwd positions in the file */

                do
                {
                        /* Get the kwd code and the next kwd position */

                        ScaWrd(msh, (unsigned char *)&KwdCod);
                        NexPos = GetPos(msh);

                        if(NexPos > EndPos)
                                return(0);

                        /* Check if this kwd belongs to this mesh version */

                        if( (KwdCod >= 1) && (KwdCod <= GmfMaxKwd) )
                                ScaKwdHdr(msh, KwdCod);

                        /* Go to the next kwd */

                        if(NexPos)
                                fseek(msh->hdl, NexPos, SEEK_SET);
                }while(NexPos && (KwdCod != GmfEnd));
        }

        return(1);
}


/*----------------------------------------------------------*/
/* Read and setup the keyword's header                                          */
/*----------------------------------------------------------*/

static void ScaKwdHdr(GmfMshSct *msh, int KwdCod)
{
        int i;
        KwdSct *kwd = &msh->KwdTab[ KwdCod ];

        if(!strcmp("i", GmfKwdFmt[ KwdCod ][2]))
        {
                if(msh->typ & Asc)
                        fscanf(msh->hdl, "%d", &kwd->NmbLin);
                else
                        ScaWrd(msh, (unsigned char *)&kwd->NmbLin);
        }
        else
                kwd->NmbLin = 1;

        if(!strcmp("sr", GmfKwdFmt[ KwdCod ][3]))
        {
                if(msh->typ & Asc)
                {
                        fscanf(msh->hdl, "%d", &kwd->NmbTyp);

                        for(i=0;i<kwd->NmbTyp;i++)
                                fscanf(msh->hdl, "%d", &kwd->TypTab[i]);
                }
                else
                {
                        ScaWrd(msh, (unsigned char *)&kwd->NmbTyp);

                        for(i=0;i<kwd->NmbTyp;i++)
                                ScaWrd(msh, (unsigned char *)&kwd->TypTab[i]);
                }
        }

        ExpFmt(msh, KwdCod);
        kwd->pos = ftell(msh->hdl);
}


/*----------------------------------------------------------*/
/* Expand the compacted format and compute the line size        */
/*----------------------------------------------------------*/

static void ExpFmt(GmfMshSct *msh, int KwdCod)
{
        int i, j, TmpSiz=0;
        char chr;
        const char *InpFmt = GmfKwdFmt[ KwdCod ][3];
        KwdSct *kwd = &msh->KwdTab[ KwdCod ];

        /* Set the kwd's type */

        if(!strlen(GmfKwdFmt[ KwdCod ][2]))
                kwd->typ = InfKwd;
        else if(!strcmp(InpFmt, "sr"))
                kwd->typ = SolKwd;
        else
                kwd->typ = RegKwd;

        /* Get the solution-field's size */

        if(kwd->typ == SolKwd)
                for(i=0;i<kwd->NmbTyp;i++)
                        switch(kwd->TypTab[i])
                        {
                                case GmfSca    : TmpSiz += 1; break;
                                case GmfVec    : TmpSiz += msh->dim; break;
                                case GmfSymMat : TmpSiz += (msh->dim * (msh->dim+1)) / 2; break;
                                case GmfMat    : TmpSiz += msh->dim * msh->dim; break;
                        }

        /* Scan each character from the format string */

        i = kwd->SolSiz = kwd->NmbWrd = 0;

        while(i < (int)strlen(InpFmt))
        {
                chr = InpFmt[ i++ ];

                if(chr == 'd')
                {
                        chr = InpFmt[i++];

                        for(j=0;j<msh->dim;j++)
                                kwd->fmt[ kwd->SolSiz++ ] = chr;
                }
                else if(chr == 's')
                {
                        chr = InpFmt[i++];

                        for(j=0;j<TmpSiz;j++)
                                kwd->fmt[ kwd->SolSiz++ ] = chr;
                }
                else
                        kwd->fmt[ kwd->SolSiz++ ] = chr;
        }

        for(i=0;i<kwd->SolSiz;i++)
                if(kwd->fmt[i] == 'i')
                        kwd->NmbWrd++;
                else if(msh->ver >= 2)
                        kwd->NmbWrd += 2;
                else
                        kwd->NmbWrd++;
}


/*----------------------------------------------------------*/
/* Read a four bytes word from a mesh file                                      */
/*----------------------------------------------------------*/

static void ScaWrd(GmfMshSct *msh, unsigned char *wrd)
{
        unsigned char swp;

        fread(wrd, WrdSiz, 1, msh->hdl);

        if(msh->cod == 1)
                return;

        swp = wrd[3];
        wrd[3] = wrd[0];
        wrd[0] = swp;

        swp = wrd[2];
        wrd[2] = wrd[1];
        wrd[1] = swp;
}


/*----------------------------------------------------------*/
/* Read an eight bytes word from a mesh file                            */
/*----------------------------------------------------------*/

static void ScaDblWrd(GmfMshSct *msh, unsigned char *wrd)
{
        int i;
        unsigned char swp;

        fread(wrd, WrdSiz, 2, msh->hdl);

        if(msh->cod == 1)
                return;

        for(i=0;i<4;i++)
        {
                swp = wrd[7-i];
                wrd[7-i] = wrd[i];
                wrd[i] = swp;
        }
}


/*----------------------------------------------------------*/
/* Read ablock of four bytes word from a mesh file                      */
/*----------------------------------------------------------*/

static void ScaBlk(GmfMshSct *msh, unsigned char *blk, int siz)
{
        int i, j;
        unsigned char swp, *wrd;

        fread(blk, WrdSiz, siz, msh->hdl);

        if(msh->cod == 1)
                return;

        for(i=0;i<siz;i++)
        {
                wrd = &blk[ i * 4 ];

                for(j=0;j<2;j++)
                {
                        swp = wrd[ 3-j ];
                        wrd[ 3-j ] = wrd[j];
                        wrd[j] = swp;
                }
        }
}


/*----------------------------------------------------------*/
/* Read a 4 or 8 bytes position in mesh file                            */
/*----------------------------------------------------------*/

static long GetPos(GmfMshSct *msh)
{
        int IntVal;
        long pos;

        if(msh->ver >= 3)
                ScaDblWrd(msh, (unsigned char*)&pos);
        else
        {
                ScaWrd(msh, (unsigned char*)&IntVal);
                pos = IntVal;
        }

        return(pos);
}


/*----------------------------------------------------------*/
/* Write a four bytes word to a mesh file                                       */
/*----------------------------------------------------------*/

static void RecWrd(GmfMshSct *msh, unsigned char *wrd)
{
        fwrite(wrd, WrdSiz, 1, msh->hdl);
}


/*----------------------------------------------------------*/
/* Write an eight bytes word to a mesh file                                     */
/*----------------------------------------------------------*/

static void RecDblWrd(GmfMshSct *msh, unsigned char *wrd)
{
        fwrite(wrd, WrdSiz, 2, msh->hdl);
}


/*----------------------------------------------------------*/
/* Write a block of four bytes word to a mesh file                      */
/*----------------------------------------------------------*/

static void RecBlk(GmfMshSct *msh, unsigned char *blk, int siz)
{
        /* Copy this line-block into the main mesh buffer */

        if(siz)
        {
                memcpy(&msh->blk[ msh->pos ], blk, siz * WrdSiz);
                msh->pos += siz * WrdSiz;
        }

        /* When the buffer is full or this procedure is called with a 0 size, flush the cache on disk */

        if( (msh->pos > BufSiz) || (!siz && msh->pos) )
        {
                fwrite(msh->blk, 1, msh->pos, msh->hdl);
                msh->pos = 0;
        }
}


/*----------------------------------------------------------*/
/* Write a 4 or 8 bytes position in a mesh file                         */
/*----------------------------------------------------------*/

static void SetPos(GmfMshSct *msh, long pos)
{
        int IntVal;

        if(msh->ver >= 3)
                RecDblWrd(msh, (unsigned char*)&pos);
        else
        {
                IntVal = pos;
                RecWrd(msh, (unsigned char*)&IntVal);
        }
}
