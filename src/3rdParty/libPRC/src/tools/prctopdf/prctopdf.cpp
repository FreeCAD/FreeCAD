#include <iostream>
#include <iomanip>
#include <fstream>
#include <cmath>
#include <string>
#include "oPRCFile.h"
#include "pdf3d.h"

bool hasCmd( const char *cmd, int argc, char** argv );
void outCmdErr();
int runBasic( int argc, char** argv );
int runAQuad( int argc, char** argv );
int runQuad( int argc, char** argv );
int runTriStrip( int argc, char** argv );
int runTriStripMulti( int argc, char** argv );
int runTriQuad( int argc, char** argv );

void createPdf( const std::string &pdffile, const std::string &prcfile, const std::string &jsfile );

void createPrcAQuad( const std::string &prcfile );
void createPrcQuad( const std::string &prcfile );
void createPrcTriStrip( const std::string &prcfile );
void createPrcTriStripMulti( const std::string &prcfile );
void createPrcTriQuad( const std::string &prcfile );
void createPrcTest( const std::string &prcfile );


// "D:\d\testdata\teapot.prc" "D:\d\testdata\teapot.pdf" "D:\skewmatrix\projects\libPRC\src\tools\prctopdf\s2plot-prc.js"
// -quad "D:\d\testdata\quad.prc" "D:\d\testdata\quad.pdf" "D:\skewmatrix\projects\libPRC\src\tools\prctopdf\s2plot-prc.js"
// -tristrip "D:\d\testdata\tristrip.prc" "D:\d\testdata\tristrip.pdf" "D:\skewmatrix\projects\libPRC\src\tools\prctopdf\s2plot-prc.js"
// -tristripm "D:\d\testdata\tristripm.prc" "D:\d\testdata\tristripm.pdf" "D:\skewmatrix\projects\libPRC\src\tools\prctopdf\s2plot-prc.js"
// -triquad "D:\d\testdata\triquad.prc" "D:\d\testdata\triquad.pdf" "D:\skewmatrix\projects\libPRC\src\tools\prctopdf\s2plot-prc.js"

int main( int argc, char** argv )
{
    int ret;

    ret = runAQuad( argc, argv );
    if (ret >= 0) return( ret );

    ret = runQuad( argc, argv );
    if (ret >= 0) return( ret );

    ret = runTriStrip( argc, argv );
    if (ret >= 0) return( ret );

    ret = runTriStripMulti( argc, argv );
    if (ret >= 0) return( ret );

    ret = runTriQuad( argc, argv );
    if (ret >= 0) return( ret );

    ret = runBasic( argc, argv );

    if( ret == -1 )
    {
        outCmdErr();
        return 1;
    }

    return ret;
}

bool hasCmd( const char *cmd, int argc, char** argv )
{
    for( int i=0; i<argc; i++ )
    {
        if (!strcmp(argv[i], cmd)) return true;
    }

    return false;
}

void outCmdErr()
{
    std::cerr << "prctopdf <pdfjsfile>" << std::endl;
    std::cerr << "prctopdf <pdfoutfile> <pdfjsfile>" << std::endl;
    std::cerr << "prctopdf <prcinfile> <pdfoutfile> <pdfjsfile>" << std::endl;
}

int runBasic( int argc, char** argv )
{
    if( argc < 2 || argc > 4 )
    {
        return - 1;
    }

    std::string prcfile;
    std::string pdffile;
    std::string jsfile; 

    if( argc == 2 )
    {
        prcfile = "test.prc";
        pdffile ="test.pdf";
        jsfile =argv[ 1 ];   
        createPrcTest( prcfile );
    }
    else if( argc == 3 )
    {
        prcfile = "test.prc";
        pdffile = argv[ 1 ];
        jsfile = argv[ 2 ]; 
        createPrcTest( prcfile );
    }
    else if( argc == 4 )
    {
        prcfile = argv[ 1 ];
        pdffile = argv[ 2 ];
        jsfile = argv[ 3 ]; 
    }

    createPdf( pdffile.c_str(), prcfile.c_str(), jsfile.c_str() );

    return( 0 );
}

int runAQuad( int argc, char** argv )
{
    if (!hasCmd( "-aquad", argc, argv )) return -1;

    if( argc != 5 )
    {
        outCmdErr();
        return 1;
    }

    std::string prcfile( argv[2] );
    std::string pdffile( argv[3] );
    std::string jsfile( argv[4] ); 

    createPrcAQuad( prcfile );
    createPdf( pdffile.c_str(), prcfile.c_str(), jsfile.c_str() );
    return 0;
}

void createPrcAQuad( const std::string &prcfile )
{
    oPRCFile file(prcfile.c_str());
    PRCoptions grpopt;
    grpopt.no_break = true;
    grpopt.do_break = false;
    grpopt.tess = true;
    grpopt.closed = true;

    double t[4][4];

    // create a quad
    const size_t nP = 4;
    double P[nP][3] = {{0,0,0},{1,0,0},{1,1,0},{0,1,0}};
    const size_t nI = 2;
    uint32_t PI[nI][3] = {{0,1,2},{2,3,0}};
    const size_t nM = 2;
    PRCmaterial M[nM];
    M[0] = PRCmaterial(
        RGBAColour(0.0,0.0,0.18),
        RGBAColour(0.0,0.0,0.878431),
        RGBAColour(0.0,0.0,0.32),
        RGBAColour(0.0,0.0,0.072),
        1.0,0.1);
    M[1] = PRCmaterial(
        RGBAColour(0.18,0.0,0.0),
        RGBAColour(0.878431,0.0,0.0),
        RGBAColour(0.32,0.0,0.0),
        RGBAColour(0.072,0.0,0.0),
        0.5,0.1);
    uint32_t MI[nI] = {0,1};
    const size_t nN = 2;
    double N[nN][3] = {{0,0,1},{0,0,-1}};
    uint32_t NI[nI][3] = {{0,0,0},{0,0,0}};

    const uint32_t nC = 3;
    RGBAColour C[nC];
    uint32_t CI[nI][3] = {{0,0,0},{1,1,1}};

    PRCmaterial materialGreen(
        RGBAColour(0.0,0.18,0.0),
        RGBAColour(0.0,0.878431,0.0),
        RGBAColour(0.0,0.32,0.0),
        RGBAColour(0.0,0.072,0.0),
        1.0,0.1);

    t[0][0]=1; t[0][1]=0; t[0][2]=0; t[0][3]=0;
    t[1][0]=0; t[1][1]=1; t[1][2]=0; t[1][3]=0;
    t[2][0]=0; t[2][1]=0; t[2][2]=1; t[2][3]=-1;
    t[3][0]=0; t[3][1]=0; t[3][2]=0; t[3][3]=1;
    file.begingroup("triangles_onecolor_with_normals",&grpopt, (const double *)t);
    file.addTriangles(nP, P, nI, PI, materialGreen, nN, N, NI, 0, NULL, NULL, 0, NULL, NULL, 0, NULL, NULL, 0);
    file.endgroup();

    file.finish();

    /*
    oPRCFile file(prcfile.c_str());
    PRCoptions grpopt;
    grpopt.no_break = true;
    grpopt.do_break = false;
    grpopt.tess = true;
    grpopt.closed = true;

    double t[4][4];

    const size_t nP = 5;
    double P[nP][3] = {{0,0,0},{1,0,0},{1,1,0},{0,1,0},{0,2,0}};
    const size_t nI = 3;
    uint32_t PI[nI][3] = {{0,1,3},{1,2,3},{3,2,4}};
    const size_t nM = 2;
    PRCmaterial M[nM];
    M[0] = PRCmaterial(
        RGBAColour(0.0,0.0,0.18),
        RGBAColour(0.0,0.0,0.878431),
        RGBAColour(0.0,0.0,0.32),
        RGBAColour(0.0,0.0,0.072),
        1.0,0.1);
    M[1] = PRCmaterial(
        RGBAColour(0.18,0.0,0.0),
        RGBAColour(0.878431,0.0,0.0),
        RGBAColour(0.32,0.0,0.0),
        RGBAColour(0.072,0.0,0.0),
        0.5,0.1);
    uint32_t MI[nI] = {0,1,0};
    const size_t nN = 2;
    double N[nN][3] = {{0,0,1},{0,0,-1}};
    uint32_t NI[nI][3] = {{0,0,0},{0,0,0},{1,1,1}};

    const uint32_t nC = 3;
    RGBAColour C[nC];
    uint32_t CI[nI][3] = {{0,0,0},{1,1,1},{1,1,2}};

    PRCmaterial materialGreen(
        RGBAColour(0.0,0.18,0.0),
        RGBAColour(0.0,0.878431,0.0),
        RGBAColour(0.0,0.32,0.0),
        RGBAColour(0.0,0.072,0.0),
        1.0,0.1);

    t[0][0]=1; t[0][1]=0; t[0][2]=0; t[0][3]=0;
    t[1][0]=0; t[1][1]=1; t[1][2]=0; t[1][3]=0;
    t[2][0]=0; t[2][1]=0; t[2][2]=1; t[2][3]=-1;
    t[3][0]=0; t[3][1]=0; t[3][2]=0; t[3][3]=1;
    file.begingroup("triangles_onecolor_with_normals",&grpopt, (const double *)t);
    file.addTriangles(nP, P, nI, PI, materialGreen, nN, N, NI, 0, NULL, NULL, 0, NULL, NULL, 0, NULL, NULL, 0);
    file.endgroup();

    file.finish();
    */
}

int runQuad( int argc, char** argv )
{
    if (!hasCmd( "-quad", argc, argv )) return -1; // not applicable

    if( argc != 5 )
    {
        outCmdErr();
        return 1; // error
    }

    std::string prcfile( argv[2] );
    std::string pdffile( argv[3] );
    std::string jsfile( argv[4] ); 

    createPrcQuad( prcfile );
    createPdf( pdffile.c_str(), prcfile.c_str(), jsfile.c_str() );
    return 0; // success
}

void createPrcQuad( const std::string &prcfile )
{
    oPRCFile file(prcfile.c_str());
    PRCoptions grpopt;
    grpopt.no_break = true;
    grpopt.do_break = false;
    grpopt.tess = true;
    grpopt.closed = true;

    double t[4][4];

    // create a quad
    PRC3DTess *tess = new PRC3DTess();

    // add verts
    const size_t nP = 4;
    double P[nP][3] = {{0,0,0},{1,0,0},{1,1,0},{0,1,0}};
    for( int v=0; v<nP; v++ )
    {
        for(int i=0; i<3; i++ )
        {
            tess->coordinates.push_back( P[v][i] );
        }
    }

    // add normals
    const size_t nN = 4;
    double N[nN][3] = {{0,0,1},{0,0,1}, {0,0,1}, {0,0,1}};
    for( int v=0; v<nN; v++ )
    {
        for(int i=0; i<3; i++ )
        {
            tess->normal_coordinate.push_back( P[v][i] );
        }
    }

    // add indexes for 2 triangles
    int triCount = 2;
    // add our normal index, then texture, then vert.. no texture coords here though
    tess->triangulated_index.push_back( 3*0 );
    tess->triangulated_index.push_back( 3*0 );
    tess->triangulated_index.push_back( 3*1 );
    tess->triangulated_index.push_back( 3*1 );
    tess->triangulated_index.push_back( 3*2 );
    tess->triangulated_index.push_back( 3*2 );
    tess->triangulated_index.push_back( 3*2 );
    tess->triangulated_index.push_back( 3*2 );
    tess->triangulated_index.push_back( 3*3 );
    tess->triangulated_index.push_back( 3*3 );
    tess->triangulated_index.push_back( 3*0 );
    tess->triangulated_index.push_back( 3*0 );

    bool hasTexCoords = false;
    PRCTessFace *tessFace = new PRCTessFace();
    tessFace->number_of_texture_coordinate_indexes = hasTexCoords ? 1 : 0;
    tessFace->used_entities_flag = hasTexCoords ? PRC_FACETESSDATA_TriangleTextured : PRC_FACETESSDATA_Triangle;

    tessFace->sizes_triangulated.push_back( triCount );
    tessFace->start_triangulated = 0;
    tess->addTessFace( tessFace );

    t[0][0]=1; t[0][1]=0; t[0][2]=0; t[0][3]=0;
    t[1][0]=0; t[1][1]=1; t[1][2]=0; t[1][3]=0;
    t[2][0]=0; t[2][1]=0; t[2][2]=1; t[2][3]=-1;
    t[3][0]=0; t[3][1]=0; t[3][2]=0; t[3][3]=1;

    file.begingroup("2_triangle_quad_with_normals",&grpopt, (const double *)t);

    PRCmaterial materialGreen(
        RGBAColour(0.0,0.18,0.0),
        RGBAColour(0.0,0.878431,0.0),
        RGBAColour(0.0,0.32,0.0),
        RGBAColour(0.0,0.072,0.0),
        1.0,0.1);

    // lets add the material the tess and then use the mesh
    const uint32_t mat_index = file.addMaterial( materialGreen );
    const uint32_t tess_index = file.add3DTess( tess );
    file.useMesh( tess_index, mat_index );

    file.endgroup();

    file.finish();

    /*
    oPRCFile file(prcfile.c_str());
    PRCoptions grpopt;
    grpopt.no_break = true;
    grpopt.do_break = false;
    grpopt.tess = true;
    grpopt.closed = true;

    double t[4][4];

    const size_t nP = 5;
    double P[nP][3] = {{0,0,0},{1,0,0},{1,1,0},{0,1,0},{0,2,0}};
    const size_t nI = 3;
    uint32_t PI[nI][3] = {{0,1,3},{1,2,3},{3,2,4}};
    const size_t nM = 2;
    PRCmaterial M[nM];
    M[0] = PRCmaterial(
        RGBAColour(0.0,0.0,0.18),
        RGBAColour(0.0,0.0,0.878431),
        RGBAColour(0.0,0.0,0.32),
        RGBAColour(0.0,0.0,0.072),
        1.0,0.1);
    M[1] = PRCmaterial(
        RGBAColour(0.18,0.0,0.0),
        RGBAColour(0.878431,0.0,0.0),
        RGBAColour(0.32,0.0,0.0),
        RGBAColour(0.072,0.0,0.0),
        0.5,0.1);
    uint32_t MI[nI] = {0,1,0};
    const size_t nN = 2;
    double N[nN][3] = {{0,0,1},{0,0,-1}};
    uint32_t NI[nI][3] = {{0,0,0},{0,0,0},{1,1,1}};

    const uint32_t nC = 3;
    RGBAColour C[nC];
    uint32_t CI[nI][3] = {{0,0,0},{1,1,1},{1,1,2}};

    PRCmaterial materialGreen(
        RGBAColour(0.0,0.18,0.0),
        RGBAColour(0.0,0.878431,0.0),
        RGBAColour(0.0,0.32,0.0),
        RGBAColour(0.0,0.072,0.0),
        1.0,0.1);

    t[0][0]=1; t[0][1]=0; t[0][2]=0; t[0][3]=0;
    t[1][0]=0; t[1][1]=1; t[1][2]=0; t[1][3]=0;
    t[2][0]=0; t[2][1]=0; t[2][2]=1; t[2][3]=-1;
    t[3][0]=0; t[3][1]=0; t[3][2]=0; t[3][3]=1;
    file.begingroup("triangles_onecolor_with_normals",&grpopt, (const double *)t);
    file.addTriangles(nP, P, nI, PI, materialGreen, nN, N, NI, 0, NULL, NULL, 0, NULL, NULL, 0, NULL, NULL, 0);
    file.endgroup();

    file.finish();
    */
}

int runTriStrip( int argc, char** argv )
{
    if (!hasCmd( "-tristrip", argc, argv )) return -1; // not applicable

    if( argc != 5 )
    {
        outCmdErr();
        return 1; // error
    }

    std::string prcfile( argv[2] );
    std::string pdffile( argv[3] );
    std::string jsfile( argv[4] ); 

    createPrcTriStrip( prcfile );
    createPdf( pdffile.c_str(), prcfile.c_str(), jsfile.c_str() );
    return 0; // success
}

void createPrcTriStrip( const std::string &prcfile )
{
    oPRCFile file(prcfile.c_str());
    PRCoptions grpopt;
    grpopt.no_break = true;
    grpopt.do_break = false;
    grpopt.tess = true;
    grpopt.closed = true;

    double t[4][4];

    // create a tri strip
    PRC3DTess *tess = new PRC3DTess();


    // add verts
    const size_t nP = 10;
    double P[nP][3] = {{0,0,-2},{1,0,-2},{0,0,-1},{1,0,-1},{0,0,0},{1,0,0},{0,0,1},{1,0,1},{0,0,2},{1,0,2}};
    for( int v=0; v<nP; v++ )
    {
        for(int i=0; i<3; i++ )
        {
            tess->coordinates.push_back( P[v][i] );
        }
    }

    // add normals
    const size_t nN = 10;
    double N[nN][3] = {{0,-1,0},{0,-1,0},{0,-1,0},{0,-1,0},{0,-1,0},{0,-1,0},{0,-1,0},{0,-1,0},{0,-1,0},{0,-1,0}};
    for( int v=0; v<nN; v++ )
    {
        for(int i=0; i<3; i++ )
        {
            tess->normal_coordinate.push_back( P[v][i] );
        }
    }

    // add indexes for our tristrip
    int triCount = nP - 2;

    // for strip
    // add our normal index, then texture, then vert.. no texture coords here though
    for( int i=0; i<nP; i++)
    {
        tess->triangulated_index.push_back( 3*i ); // normal index
        tess->triangulated_index.push_back( 3*i ); // vert index
    }

    bool hasTexCoords = false;
    PRCTessFace *tessFace = new PRCTessFace();
    tessFace->number_of_texture_coordinate_indexes = hasTexCoords ? 1 : 0;
    tessFace->used_entities_flag = hasTexCoords ? PRC_FACETESSDATA_TriangleStripeTextured : PRC_FACETESSDATA_TriangleStripe;

    //tessFace->sizes_triangulated.push_back( triCount );
    tessFace->sizes_triangulated.push_back( 1 );
    tessFace->sizes_triangulated.push_back( 10 );
    tessFace->start_triangulated = 0;
    tess->addTessFace( tessFace );

    t[0][0]=1; t[0][1]=0; t[0][2]=0; t[0][3]=0;
    t[1][0]=0; t[1][1]=1; t[1][2]=0; t[1][3]=0;
    t[2][0]=0; t[2][1]=0; t[2][2]=1; t[2][3]=-1;
    t[3][0]=0; t[3][1]=0; t[3][2]=0; t[3][3]=1;

    file.begingroup("triangle_strip",&grpopt, (const double *)t);

    PRCmaterial materialGreen(
        RGBAColour(0.0,0.18,0.0),
        RGBAColour(0.0,0.878431,0.0),
        RGBAColour(0.0,0.32,0.0),
        RGBAColour(0.0,0.072,0.0),
        1.0,0.1);

    // lets add the material the tess and then use the mesh
    const uint32_t mat_index = file.addMaterial( materialGreen );
    const uint32_t tess_index = file.add3DTess( tess );
    file.useMesh( tess_index, mat_index );

    file.endgroup();

    file.finish();
}

int runTriStripMulti( int argc, char** argv )
{
    if (!hasCmd( "-tristripm", argc, argv )) return -1; // not applicable

    if( argc != 5 )
    {
        outCmdErr();
        return 1; // error
    }

    std::string prcfile( argv[2] );
    std::string pdffile( argv[3] );
    std::string jsfile( argv[4] ); 

    createPrcTriStripMulti( prcfile );
    createPdf( pdffile.c_str(), prcfile.c_str(), jsfile.c_str() );
    return 0; // success
}

void createPrcTriStripMulti( const std::string &prcfile )
{
    oPRCFile file(prcfile.c_str());
    PRCoptions grpopt;
    grpopt.no_break = true;
    grpopt.do_break = false;
    grpopt.tess = true;
    grpopt.closed = true;

    double t[4][4];

    
    t[0][0]=1; t[0][1]=0; t[0][2]=0; t[0][3]=0;
    t[1][0]=0; t[1][1]=1; t[1][2]=0; t[1][3]=0;
    t[2][0]=0; t[2][1]=0; t[2][2]=1; t[2][3]=-1;
    t[3][0]=0; t[3][1]=0; t[3][2]=0; t[3][3]=1;

    file.begingroup("2_triangle_strip",&grpopt, (const double *)t);

    PRCmaterial materialWhite(
        RGBAColour(0.0,0.18,0.0),
        RGBAColour(1.0,1.0,1.0),
        RGBAColour(0.0,0.32,0.0),
        RGBAColour(0.0,0.072,0.0),
        1.0,0.1);
    
    PRCmaterial materialGreen(
        RGBAColour(0.0,0.18,0.0),
        RGBAColour(0.0,0.878431,0.0),
        RGBAColour(0.0,0.32,0.0),
        RGBAColour(0.0,0.072,0.0),
        1.0,0.1);

    PRCmaterial materialRed(
        RGBAColour(0.18, 0.,0.0),
        RGBAColour(1.0,0.0,0.0),
        RGBAColour(0.32,0.,0.0),
        RGBAColour(0.072,0.0,0.0),
        1.0,0.1);

    // lets add the material the tess and then use the mesh
    const uint32_t mat_index_white = file.addMaterial( materialWhite );
    const uint32_t mat_index_green = file.addMaterial( materialGreen );
    const uint32_t mat_index_red = file.addMaterial( materialRed );



    // create a tri strip
    PRC3DTess *tess = new PRC3DTess();


    // add verts
    const size_t nP = 10;
    double P[nP][3] = {{0,0,-2},{1,0,-2},{0,0,-1},{1,0,-1},{0,0,0},{1,0,0},{0,0,1},{1,0,1},{0,0,2},{1,0,2}};
    for( int v=0; v<nP; v++ )
    {
        for(int i=0; i<3; i++ )
        {
            tess->coordinates.push_back( P[v][i] );
        }
    }

    // add normals
    const size_t nN = 10;
    double N[nN][3] = {{0,-1,0},{0,-1,0},{0,-1,0},{0,-1,0},{0,-1,0},{0,-1,0},{0,-1,0},{0,-1,0},{0,-1,0},{0,-1,0}};
    for( int v=0; v<nN; v++ )
    {
        for(int i=0; i<3; i++ )
        {
            tess->normal_coordinate.push_back( P[v][i] );
        }
    }

    // add indexes for our tristrip
    int triCount = nP - 2;

    // for strip
    // add our normal index, then texture, then vert.. no texture coords here though
    for( int i=0; i<nP; i++)
    {
        tess->triangulated_index.push_back( 3*i ); // normal index
        tess->triangulated_index.push_back( 3*i ); // vert index
    }

    bool hasTexCoords = false;
    PRCTessFace *tessFace = new PRCTessFace();
    tessFace->number_of_texture_coordinate_indexes = hasTexCoords ? 1 : 0;
    tessFace->used_entities_flag = hasTexCoords ? PRC_FACETESSDATA_TriangleStripeTextured : PRC_FACETESSDATA_TriangleStripe;

    //tessFace->sizes_triangulated.push_back( triCount );
    tessFace->sizes_triangulated.push_back( 1 );
    tessFace->sizes_triangulated.push_back( 4 );
    tessFace->start_triangulated = 0;
    //tessFace->line_attributes.push_back(mat_index_green);
    tess->addTessFace( tessFace );

    tessFace = new PRCTessFace();
    tessFace->number_of_texture_coordinate_indexes = hasTexCoords ? 1 : 0;
    tessFace->used_entities_flag = hasTexCoords ? PRC_FACETESSDATA_TriangleStripeTextured : PRC_FACETESSDATA_TriangleStripe;

    //tessFace->sizes_triangulated.push_back( triCount );
    tessFace->sizes_triangulated.push_back( 1 );
    tessFace->sizes_triangulated.push_back( 6 );
    tessFace->start_triangulated = 4;
    tessFace->line_attributes.push_back(mat_index_red);
    tess->addTessFace( tessFace );

    const uint32_t tess_index = file.add3DTess( tess );
    file.useMesh( tess_index, mat_index_green );

    file.endgroup();

    file.finish();
}


int runTriQuad( int argc, char** argv )
{
    if (!hasCmd( "-triquad", argc, argv )) return -1; // not applicable

    if( argc != 5 )
    {
        outCmdErr();
        return 1; // error
    }

    std::string prcfile( argv[2] );
    std::string pdffile( argv[3] );
    std::string jsfile( argv[4] ); 

    createPrcTriQuad( prcfile );
    createPdf( pdffile.c_str(), prcfile.c_str(), jsfile.c_str() );
    return 0; // success
}

void createPrcTriQuad( const std::string &prcfile )
{
    oPRCFile file(prcfile.c_str());
    PRCoptions grpopt;
    grpopt.no_break = true;
    grpopt.do_break = false;
    grpopt.tess = true;
    grpopt.closed = true;

    double t[4][4];

    
    t[0][0]=1; t[0][1]=0; t[0][2]=0; t[0][3]=0;
    t[1][0]=0; t[1][1]=1; t[1][2]=0; t[1][3]=0;
    t[2][0]=0; t[2][1]=0; t[2][2]=1; t[2][3]=-1;
    t[3][0]=0; t[3][1]=0; t[3][2]=0; t[3][3]=1;

    file.begingroup("tri_2quad",&grpopt, (const double *)t);

    PRCmaterial materialWhite(
        RGBAColour(0.0,0.18,0.0),
        RGBAColour(1.0,1.0,1.0),
        RGBAColour(0.0,0.32,0.0),
        RGBAColour(0.0,0.072,0.0),
        1.0,0.1);
    
    PRCmaterial materialGreen(
        RGBAColour(0.0,0.18,0.0),
        RGBAColour(0.0,0.878431,0.0),
        RGBAColour(0.0,0.32,0.0),
        RGBAColour(0.0,0.072,0.0),
        1.0,0.1);

    PRCmaterial materialRed(
        RGBAColour(0.18, 0.,0.0),
        RGBAColour(1.0,0.0,0.0),
        RGBAColour(0.32,0.,0.0),
        RGBAColour(0.072,0.0,0.0),
        1.0,0.1);

    // lets add the material the tess and then use the mesh
    const uint32_t mat_index_white = file.addMaterial( materialWhite );
    const uint32_t mat_index_green = file.addMaterial( materialGreen );
    const uint32_t mat_index_red = file.addMaterial( materialRed );



    // create a tri strip
    PRC3DTess *tess = new PRC3DTess();


    // add verts
    const size_t nP = 11;
    double P[nP][3] = {{0,0,-2},{1,0,-2},{1,0,-1},{0,0,-1},{0,0,0},{1,0,0},{1,0,1},{0,0,1},{-1,0,-1},{-.5,0,-1},{-.5,0,0}};
    for( int v=0; v<nP; v++ )
    {
        for(int i=0; i<3; i++ )
        {
            tess->coordinates.push_back( P[v][i] );
        }
    }

    // add normals
    const size_t nN = 11;
    double N[nN][3] = {{0,-1,0},{0,-1,0},{0,-1,0},{0,-1,0},{0,-1,0},{0,-1,0},{0,-1,0},{0,-1,0},{0,-1,0},{0,-1,0},{0,-1,0}};
    for( int v=0; v<nN; v++ )
    {
        for(int i=0; i<3; i++ )
        {
            tess->normal_coordinate.push_back( P[v][i] );
        }
    }

    int triCount = 1;

    // for strip
    // add our normal index, then texture, then vert.. no texture coords here though
    for( int i=8; i<11; i++)
    {
        tess->triangulated_index.push_back( 3*i ); // normal index
        tess->triangulated_index.push_back( 3*i ); // vert index
    }

    bool hasTexCoords = false;
    PRCTessFace *tessFace = new PRCTessFace();
    tessFace->number_of_texture_coordinate_indexes = hasTexCoords ? 1 : 0;
    tessFace->used_entities_flag = hasTexCoords ? PRC_FACETESSDATA_TriangleTextured : PRC_FACETESSDATA_Triangle;

    //tessFace->sizes_triangulated.push_back( triCount );
    tessFace->sizes_triangulated.push_back( 1 );
    tessFace->start_triangulated = 0;
    //tessFace->line_attributes.push_back(mat_index_green);
    tess->addTessFace( tessFace );

    // create a quads
    
    int i = 0;
    tess->triangulated_index.push_back( 3*i ); // normal index
    tess->triangulated_index.push_back( 3*i ); // vert index

    i=1;
    tess->triangulated_index.push_back( 3*i ); // normal index
    tess->triangulated_index.push_back( 3*i ); // vert index

    i=2;
    tess->triangulated_index.push_back( 3*i ); // normal index
    tess->triangulated_index.push_back( 3*i ); // vert index

    i = 0;
    tess->triangulated_index.push_back( 3*i ); // normal index
    tess->triangulated_index.push_back( 3*i ); // vert index

    i = 2;
    tess->triangulated_index.push_back( 3*i ); // normal index
    tess->triangulated_index.push_back( 3*i ); // vert index

    i = 3;
    tess->triangulated_index.push_back( 3*i ); // normal index
    tess->triangulated_index.push_back( 3*i ); // vert index
    

    tessFace = new PRCTessFace();
    tessFace->number_of_texture_coordinate_indexes = hasTexCoords ? 1 : 0;
    tessFace->used_entities_flag = hasTexCoords ? PRC_FACETESSDATA_TriangleTextured : PRC_FACETESSDATA_Triangle;
    tessFace->sizes_triangulated.push_back( 2 );
    tessFace->start_triangulated = 6;
    tessFace->line_attributes.push_back(mat_index_red);
    tess->addTessFace( tessFace );

    const uint32_t tess_index = file.add3DTess( tess );
    file.useMesh( tess_index, mat_index_green );

    file.endgroup();

    file.finish();
}

void createPrcTest(const std::string &prcfile)
{
    // List of pictures used; keep track of memory allocated to free it in the end
    // shared pointers or garbage collector may be an alternative
    uint8_t *picture1 = NULL;
    uint8_t *picture2 = NULL;
    uint8_t *picture3 = NULL;
    uint8_t *picture4 = NULL;

    oPRCFile file(prcfile.c_str());

    const size_t N_COLOURS = 32;
    RGBAColour colours[N_COLOURS];
    for(size_t i = 0; i < N_COLOURS; ++i)
    {
        colours[i%N_COLOURS].R = 0.0;
        colours[i%N_COLOURS].G = (i%N_COLOURS)/static_cast<double>(N_COLOURS);
        colours[i%N_COLOURS].B = 0.95;
        colours[i%N_COLOURS].A = 0.75;
    }

    PRCmaterial materials[N_COLOURS];
    for(size_t i = 0; i < N_COLOURS; ++i)
    {
        materials[i%N_COLOURS].diffuse.R = 0.0;
        materials[i%N_COLOURS].diffuse.G = (i%N_COLOURS)/static_cast<double>(N_COLOURS);
        materials[i%N_COLOURS].diffuse.B = 0.95;
        materials[i%N_COLOURS].diffuse.A = 0.75;
        materials[i%N_COLOURS].specular.R = 0.01*0.0;
        materials[i%N_COLOURS].specular.G = 0.01*(i%N_COLOURS)/static_cast<double>(N_COLOURS);
        materials[i%N_COLOURS].specular.B = 0.01*0.95;
        materials[i%N_COLOURS].specular.A = 0.01*0.75;
        materials[i%N_COLOURS].emissive.R = 0.20*0.0;
        materials[i%N_COLOURS].emissive.G = 0.20*(i%N_COLOURS)/static_cast<double>(N_COLOURS);
        materials[i%N_COLOURS].emissive.B = 0.20*0.95;
        materials[i%N_COLOURS].emissive.A = 0.20*0.75;
        materials[i%N_COLOURS].ambient.R  = 0.05*0.0;
        materials[i%N_COLOURS].ambient.G  = 0.05*(i%N_COLOURS)/static_cast<double>(N_COLOURS);
        materials[i%N_COLOURS].ambient.B  = 0.05*0.95;
        materials[i%N_COLOURS].ambient.A  = 0.05*0.75;
        materials[i%N_COLOURS].alpha      = 0.75;
        materials[i%N_COLOURS].shininess  = 0.1;
    }

    if(1) {
        double knotsU[] = {1,1,1,1,2,2,2,2};
        double knotsV[] = {1,1,1,1,2,2,2,2};
        const size_t NUMBER_OF_PATCHES = 32;
        double controlPoints[NUMBER_OF_PATCHES][16][3] = 
        {
            { // Patch 0
                {1.4,0,2.4},{1.4,-0.784,2.4},{0.784,-1.4,2.4},{0,-1.4,2.4},
                {1.3375,0,2.53125},{1.3375,-0.749,2.53125},{0.749,-1.3375,2.53125},{0,-1.3375,2.53125},
                {1.4375,0,2.53125},{1.4375,-0.805,2.53125},{0.805,-1.4375,2.53125},{0,-1.4375,2.53125},
                {1.5,0,2.4},{1.5,-0.84,2.4},{0.84,-1.5,2.4},{0,-1.5,2.4},
            },
            { // Patch 1
                {0,-1.4,2.4},{-0.784,-1.4,2.4},{-1.4,-0.784,2.4},{-1.4,0,2.4},
                {0,-1.3375,2.53125},{-0.749,-1.3375,2.53125},{-1.3375,-0.749,2.53125},{-1.3375,0,2.53125},
                {0,-1.4375,2.53125},{-0.805,-1.4375,2.53125},{-1.4375,-0.805,2.53125},{-1.4375,0,2.53125},
                {0,-1.5,2.4},{-0.84,-1.5,2.4},{-1.5,-0.84,2.4},{-1.5,0,2.4},
                },
                { // Patch 2
                    {-1.4,0,2.4},{-1.4,0.784,2.4},{-0.784,1.4,2.4},{0,1.4,2.4},
                    {-1.3375,0,2.53125},{-1.3375,0.749,2.53125},{-0.749,1.3375,2.53125},{0,1.3375,2.53125},
                    {-1.4375,0,2.53125},{-1.4375,0.805,2.53125},{-0.805,1.4375,2.53125},{0,1.4375,2.53125},
                    {-1.5,0,2.4},{-1.5,0.84,2.4},{-0.84,1.5,2.4},{0,1.5,2.4},
                },
                { // Patch 3
                    {0,1.4,2.4},{0.784,1.4,2.4},{1.4,0.784,2.4},{1.4,0,2.4},
                    {0,1.3375,2.53125},{0.749,1.3375,2.53125},{1.3375,0.749,2.53125},{1.3375,0,2.53125},
                    {0,1.4375,2.53125},{0.805,1.4375,2.53125},{1.4375,0.805,2.53125},{1.4375,0,2.53125},
                    {0,1.5,2.4},{0.84,1.5,2.4},{1.5,0.84,2.4},{1.5,0,2.4},
                    },
                    { // Patch 4
                        {1.5,0,2.4},{1.5,-0.84,2.4},{0.84,-1.5,2.4},{0,-1.5,2.4},
                        {1.75,0,1.875},{1.75,-0.98,1.875},{0.98,-1.75,1.875},{0,-1.75,1.875},
                        {2,0,1.35},{2,-1.12,1.35},{1.12,-2,1.35},{0,-2,1.35},
                        {2,0,0.9},{2,-1.12,0.9},{1.12,-2,0.9},{0,-2,0.9},
                    },
                    { // Patch 5
                        {0,-1.5,2.4},{-0.84,-1.5,2.4},{-1.5,-0.84,2.4},{-1.5,0,2.4},
                        {0,-1.75,1.875},{-0.98,-1.75,1.875},{-1.75,-0.98,1.875},{-1.75,0,1.875},
                        {0,-2,1.35},{-1.12,-2,1.35},{-2,-1.12,1.35},{-2,0,1.35},
                        {0,-2,0.9},{-1.12,-2,0.9},{-2,-1.12,0.9},{-2,0,0.9},
                        },
                        { // Patch 6
                            {-1.5,0,2.4},{-1.5,0.84,2.4},{-0.84,1.5,2.4},{0,1.5,2.4},
                            {-1.75,0,1.875},{-1.75,0.98,1.875},{-0.98,1.75,1.875},{0,1.75,1.875},
                            {-2,0,1.35},{-2,1.12,1.35},{-1.12,2,1.35},{0,2,1.35},
                            {-2,0,0.9},{-2,1.12,0.9},{-1.12,2,0.9},{0,2,0.9},
                        },
                        { // Patch 7
                            {0,1.5,2.4},{0.84,1.5,2.4},{1.5,0.84,2.4},{1.5,0,2.4},
                            {0,1.75,1.875},{0.98,1.75,1.875},{1.75,0.98,1.875},{1.75,0,1.875},
                            {0,2,1.35},{1.12,2,1.35},{2,1.12,1.35},{2,0,1.35},
                            {0,2,0.9},{1.12,2,0.9},{2,1.12,0.9},{2,0,0.9},
                            },
                            { // Patch 8
                                {2,0,0.9},{2,-1.12,0.9},{1.12,-2,0.9},{0,-2,0.9},
                                {2,0,0.45},{2,-1.12,0.45},{1.12,-2,0.45},{0,-2,0.45},
                                {1.5,0,0.225},{1.5,-0.84,0.225},{0.84,-1.5,0.225},{0,-1.5,0.225},
                                {1.5,0,0.15},{1.5,-0.84,0.15},{0.84,-1.5,0.15},{0,-1.5,0.15},
                            },
                            { // Patch 9
                                {0,-2,0.9},{-1.12,-2,0.9},{-2,-1.12,0.9},{-2,0,0.9},
                                {0,-2,0.45},{-1.12,-2,0.45},{-2,-1.12,0.45},{-2,0,0.45},
                                {0,-1.5,0.225},{-0.84,-1.5,0.225},{-1.5,-0.84,0.225},{-1.5,0,0.225},
                                {0,-1.5,0.15},{-0.84,-1.5,0.15},{-1.5,-0.84,0.15},{-1.5,0,0.15},
                                },
                                { // Patch 10
                                    {-2,0,0.9},{-2,1.12,0.9},{-1.12,2,0.9},{0,2,0.9},
                                    {-2,0,0.45},{-2,1.12,0.45},{-1.12,2,0.45},{0,2,0.45},
                                    {-1.5,0,0.225},{-1.5,0.84,0.225},{-0.84,1.5,0.225},{0,1.5,0.225},
                                    {-1.5,0,0.15},{-1.5,0.84,0.15},{-0.84,1.5,0.15},{0,1.5,0.15},
                                },
                                { // Patch 11
                                    {0,2,0.9},{1.12,2,0.9},{2,1.12,0.9},{2,0,0.9},
                                    {0,2,0.45},{1.12,2,0.45},{2,1.12,0.45},{2,0,0.45},
                                    {0,1.5,0.225},{0.84,1.5,0.225},{1.5,0.84,0.225},{1.5,0,0.225},
                                    {0,1.5,0.15},{0.84,1.5,0.15},{1.5,0.84,0.15},{1.5,0,0.15},
                                    },
                                    { // Patch 12
                                        {-1.6,0,2.025},{-1.6,-0.3,2.025},{-1.5,-0.3,2.25},{-1.5,0,2.25},
                                        {-2.3,0,2.025},{-2.3,-0.3,2.025},{-2.5,-0.3,2.25},{-2.5,0,2.25},
                                        {-2.7,0,2.025},{-2.7,-0.3,2.025},{-3,-0.3,2.25},{-3,0,2.25},
                                        {-2.7,0,1.8},{-2.7,-0.3,1.8},{-3,-0.3,1.8},{-3,0,1.8},
                                    },
                                    { // Patch 13
                                        {-1.5,0,2.25},{-1.5,0.3,2.25},{-1.6,0.3,2.025},{-1.6,0,2.025},
                                        {-2.5,0,2.25},{-2.5,0.3,2.25},{-2.3,0.3,2.025},{-2.3,0,2.025},
                                        {-3,0,2.25},{-3,0.3,2.25},{-2.7,0.3,2.025},{-2.7,0,2.025},
                                        {-3,0,1.8},{-3,0.3,1.8},{-2.7,0.3,1.8},{-2.7,0,1.8},
                                        },
                                        { // Patch 14
                                            {-2.7,0,1.8},{-2.7,-0.3,1.8},{-3,-0.3,1.8},{-3,0,1.8},
                                            {-2.7,0,1.575},{-2.7,-0.3,1.575},{-3,-0.3,1.35},{-3,0,1.35},
                                            {-2.5,0,1.125},{-2.5,-0.3,1.125},{-2.65,-0.3,0.9375},{-2.65,0,0.9375},
                                            {-2,0,0.9},{-2,-0.3,0.9},{-1.9,-0.3,0.6},{-1.9,0,0.6},
                                        },
                                        { // Patch 15
                                            {-3,0,1.8},{-3,0.3,1.8},{-2.7,0.3,1.8},{-2.7,0,1.8},
                                            {-3,0,1.35},{-3,0.3,1.35},{-2.7,0.3,1.575},{-2.7,0,1.575},
                                            {-2.65,0,0.9375},{-2.65,0.3,0.9375},{-2.5,0.3,1.125},{-2.5,0,1.125},
                                            {-1.9,0,0.6},{-1.9,0.3,0.6},{-2,0.3,0.9},{-2,0,0.9},
                                            },
                                            { // Patch 16
                                                {1.7,0,1.425},{1.7,-0.66,1.425},{1.7,-0.66,0.6},{1.7,0,0.6},
                                                {2.6,0,1.425},{2.6,-0.66,1.425},{3.1,-0.66,0.825},{3.1,0,0.825},
                                                {2.3,0,2.1},{2.3,-0.25,2.1},{2.4,-0.25,2.025},{2.4,0,2.025},
                                                {2.7,0,2.4},{2.7,-0.25,2.4},{3.3,-0.25,2.4},{3.3,0,2.4},
                                            },
                                            { // Patch 17
                                                {1.7,0,0.6},{1.7,0.66,0.6},{1.7,0.66,1.425},{1.7,0,1.425},
                                                {3.1,0,0.825},{3.1,0.66,0.825},{2.6,0.66,1.425},{2.6,0,1.425},
                                                {2.4,0,2.025},{2.4,0.25,2.025},{2.3,0.25,2.1},{2.3,0,2.1},
                                                {3.3,0,2.4},{3.3,0.25,2.4},{2.7,0.25,2.4},{2.7,0,2.4},
                                                },
                                                { // Patch 18
                                                    {2.7,0,2.4},{2.7,-0.25,2.4},{3.3,-0.25,2.4},{3.3,0,2.4},
                                                    {2.8,0,2.475},{2.8,-0.25,2.475},{3.525,-0.25,2.49375},{3.525,0,2.49375},
                                                    {2.9,0,2.475},{2.9,-0.15,2.475},{3.45,-0.15,2.5125},{3.45,0,2.5125},
                                                    {2.8,0,2.4},{2.8,-0.15,2.4},{3.2,-0.15,2.4},{3.2,0,2.4},
                                                },
                                                { // Patch 19
                                                    {3.3,0,2.4},{3.3,0.25,2.4},{2.7,0.25,2.4},{2.7,0,2.4},
                                                    {3.525,0,2.49375},{3.525,0.25,2.49375},{2.8,0.25,2.475},{2.8,0,2.475},
                                                    {3.45,0,2.5125},{3.45,0.15,2.5125},{2.9,0.15,2.475},{2.9,0,2.475},
                                                    {3.2,0,2.4},{3.2,0.15,2.4},{2.8,0.15,2.4},{2.8,0,2.4},
                                                    },
                                                    { // Patch 20
                                                        {0,0,3.15},{0,0,3.15},{0,0,3.15},{0,0,3.15},
                                                        {0.8,0,3.15},{0.8,-0.45,3.15},{0.45,-0.8,3.15},{0,-0.8,3.15},
                                                        {0,0,2.85},{0,0,2.85},{0,0,2.85},{0,0,2.85},
                                                        {0.2,0,2.7},{0.2,-0.112,2.7},{0.112,-0.2,2.7},{0,-0.2,2.7},
                                                    },
                                                    { // Patch 21
                                                        {0,0,3.15},{0,0,3.15},{0,0,3.15},{0,0,3.15},
                                                        {0,-0.8,3.15},{-0.45,-0.8,3.15},{-0.8,-0.45,3.15},{-0.8,0,3.15},
                                                        {0,0,2.85},{0,0,2.85},{0,0,2.85},{0,0,2.85},
                                                        {0,-0.2,2.7},{-0.112,-0.2,2.7},{-0.2,-0.112,2.7},{-0.2,0,2.7},
                                                        },
                                                        { // Patch 22
                                                            {0,0,3.15},{0,0,3.15},{0,0,3.15},{0,0,3.15},
                                                            {-0.8,0,3.15},{-0.8,0.45,3.15},{-0.45,0.8,3.15},{0,0.8,3.15},
                                                            {0,0,2.85},{0,0,2.85},{0,0,2.85},{0,0,2.85},
                                                            {-0.2,0,2.7},{-0.2,0.112,2.7},{-0.112,0.2,2.7},{0,0.2,2.7},
                                                        },
                                                        { // Patch 23
                                                            {0,0,3.15},{0,0,3.15},{0,0,3.15},{0,0,3.15},
                                                            {0,0.8,3.15},{0.45,0.8,3.15},{0.8,0.45,3.15},{0.8,0,3.15},
                                                            {0,0,2.85},{0,0,2.85},{0,0,2.85},{0,0,2.85},
                                                            {0,0.2,2.7},{0.112,0.2,2.7},{0.2,0.112,2.7},{0.2,0,2.7},
                                                            },
                                                            { // Patch 24
                                                                {0.2,0,2.7},{0.2,-0.112,2.7},{0.112,-0.2,2.7},{0,-0.2,2.7},
                                                                {0.4,0,2.55},{0.4,-0.224,2.55},{0.224,-0.4,2.55},{0,-0.4,2.55},
                                                                {1.3,0,2.55},{1.3,-0.728,2.55},{0.728,-1.3,2.55},{0,-1.3,2.55},
                                                                {1.3,0,2.4},{1.3,-0.728,2.4},{0.728,-1.3,2.4},{0,-1.3,2.4},
                                                            },
                                                            { // Patch 25
                                                                {0,-0.2,2.7},{-0.112,-0.2,2.7},{-0.2,-0.112,2.7},{-0.2,0,2.7},
                                                                {0,-0.4,2.55},{-0.224,-0.4,2.55},{-0.4,-0.224,2.55},{-0.4,0,2.55},
                                                                {0,-1.3,2.55},{-0.728,-1.3,2.55},{-1.3,-0.728,2.55},{-1.3,0,2.55},
                                                                {0,-1.3,2.4},{-0.728,-1.3,2.4},{-1.3,-0.728,2.4},{-1.3,0,2.4},
                                                                },
                                                                { // Patch 26
                                                                    {-0.2,0,2.7},{-0.2,0.112,2.7},{-0.112,0.2,2.7},{0,0.2,2.7},
                                                                    {-0.4,0,2.55},{-0.4,0.224,2.55},{-0.224,0.4,2.55},{0,0.4,2.55},
                                                                    {-1.3,0,2.55},{-1.3,0.728,2.55},{-0.728,1.3,2.55},{0,1.3,2.55},
                                                                    {-1.3,0,2.4},{-1.3,0.728,2.4},{-0.728,1.3,2.4},{0,1.3,2.4},
                                                                },
                                                                { // Patch 27
                                                                    {0,0.2,2.7},{0.112,0.2,2.7},{0.2,0.112,2.7},{0.2,0,2.7},
                                                                    {0,0.4,2.55},{0.224,0.4,2.55},{0.4,0.224,2.55},{0.4,0,2.55},
                                                                    {0,1.3,2.55},{0.728,1.3,2.55},{1.3,0.728,2.55},{1.3,0,2.55},
                                                                    {0,1.3,2.4},{0.728,1.3,2.4},{1.3,0.728,2.4},{1.3,0,2.4},
                                                                    },
                                                                    { // Patch 28
                                                                        {0,0,0},{0,0,0},{0,0,0},{0,0,0},
                                                                        {1.425,0,0},{1.425,0.798,0},{0.798,1.425,0},{0,1.425,0},
                                                                        {1.5,0,0.075},{1.5,0.84,0.075},{0.84,1.5,0.075},{0,1.5,0.075},
                                                                        {1.5,0,0.15},{1.5,0.84,0.15},{0.84,1.5,0.15},{0,1.5,0.15},
                                                                    },
                                                                    { // Patch 29
                                                                        {0,0,0},{0,0,0},{0,0,0},{0,0,0},
                                                                        {0,1.425,0},{-0.798,1.425,0},{-1.425,0.798,0},{-1.425,0,0},
                                                                        {0,1.5,0.075},{-0.84,1.5,0.075},{-1.5,0.84,0.075},{-1.5,0,0.075},
                                                                        {0,1.5,0.15},{-0.84,1.5,0.15},{-1.5,0.84,0.15},{-1.5,0,0.15},
                                                                        },
                                                                        { // Patch 30
                                                                            {0,0,0},{0,0,0},{0,0,0},{0,0,0},
                                                                            {-1.425,0,0},{-1.425,-0.798,0},{-0.798,-1.425,0},{0,-1.425,0},
                                                                            {-1.5,0,0.075},{-1.5,-0.84,0.075},{-0.84,-1.5,0.075},{0,-1.5,0.075},
                                                                            {-1.5,0,0.15},{-1.5,-0.84,0.15},{-0.84,-1.5,0.15},{0,-1.5,0.15},
                                                                        },
                                                                        { // Patch 31
                                                                            {0,0,0},{0,0,0},{0,0,0},{0,0,0},
                                                                            {0,-1.425,0},{0.798,-1.425,0},{1.425,-0.798,0},{1.425,0,0},
                                                                            {0,-1.5,0.075},{0.84,-1.5,0.075},{1.5,-0.84,0.075},{1.5,0,0.075},
                                                                            {0,-1.5,0.15},{0.84,-1.5,0.15},{1.5,-0.84,0.15},{1.5,0,0.15},
                                                                            },
        };
        file.begingroup("Teapot");
        for(size_t i = 0; i < NUMBER_OF_PATCHES; ++i)
        {
            if(1) file.addPatch(controlPoints[i],materials[i%N_COLOURS]);
            if(0) file.addSurface(3,3,4,4,controlPoints[i],knotsU,knotsV,materials[i%N_COLOURS],NULL); // use (too) general API for the same result as above
        }
        file.endgroup();

        double t[4][4];
        t[0][0]=1; t[0][1]=0; t[0][2]=0; t[0][3]=6;
        t[1][0]=0; t[1][1]=1; t[1][2]=0; t[1][3]=0;
        t[2][0]=0; t[2][1]=0; t[2][2]=1; t[2][3]=0;
        t[3][0]=0; t[3][1]=0; t[3][2]=0; t[3][3]=1;

        PRCoptions teapotopt;
        teapotopt.no_break = true;
        teapotopt.do_break = false;
        teapotopt.compression = 0.0001;
        // force joining together of patches, damaging transparency
        file.begingroup("Teapot rendered in the way of opaque surfaces and transferred",&teapotopt, (const double *)t);
        for(size_t i = 0; i < NUMBER_OF_PATCHES; ++i)
        {
            file.addPatch(controlPoints[i],materials[i%N_COLOURS]);
        }
        file.endgroup();
    }

    const size_t NUMBER_OF_POINTS = 31;
    double points[NUMBER_OF_POINTS][3];
    for(size_t i = 0; i < NUMBER_OF_POINTS; ++i)
    {
        points[i][0] = 3.5*cos(3.0*i/NUMBER_OF_POINTS*2.0*pi);
        points[i][1] = 3.5*sin(3.0*i/NUMBER_OF_POINTS*2.0*pi);
        points[i][2] = 5.0*i/NUMBER_OF_POINTS-1.0;
    }
    const size_t NUMBER_OF_WIRES = 2;
    double shifted_points[NUMBER_OF_WIRES][NUMBER_OF_POINTS][3];
    for(size_t wire = 0; wire < NUMBER_OF_WIRES; ++wire)
        for(size_t point = 0; point < NUMBER_OF_POINTS; ++point)
        {
            shifted_points[wire][point][0] = points[point][0];
            shifted_points[wire][point][1] = points[point][1];
            shifted_points[wire][point][2] = points[point][2]+0.1*wire+0.1;
        }
        double knots[3+NUMBER_OF_POINTS+1];
        knots[0] = 1;
        for(size_t i = 1; i < 3+NUMBER_OF_POINTS; ++i)
        {
            knots[i] = (i+2)/3; // integer division is intentional
        }
        knots[3+NUMBER_OF_POINTS] = (3+NUMBER_OF_POINTS+1)/3;

        PRCoptions grpopt;
        grpopt.no_break = true;
        grpopt.do_break = false;
        grpopt.tess = true;
        if(1){
            double point1[3] = {11,0,0};
            double point2[3] = {12,0,0};
            double points[2][3] = {{9,0,0},{10,0,0}};
            file.begingroup("points",&grpopt);
            file.addPoint(point1, RGBAColour(1.0,0.0,0.0));
            file.addPoint(point2, RGBAColour(1.0,0.0,0.0));
            file.addPoints(2, points, RGBAColour(1.0,0.0,0.0,0.5),10);
            file.endgroup();
        }

        if(1){
            PRCoptions grpopt;
            grpopt.no_break = true;
            grpopt.do_break = false;
            grpopt.tess = true;
            grpopt.closed = true;

            double t[4][4];

            const size_t nP = 5;
            double P[nP][3] = {{0,0,0},{1,0,0},{1,1,0},{0,1,0},{0,2,0}};
            const size_t nI = 3;
            uint32_t PI[nI][3] = {{0,1,3},{1,2,3},{3,2,4}};
            const size_t nM = 2;
            PRCmaterial M[nM];
            M[0] = PRCmaterial(
                RGBAColour(0.0,0.0,0.18),
                RGBAColour(0.0,0.0,0.878431),
                RGBAColour(0.0,0.0,0.32),
                RGBAColour(0.0,0.0,0.072),
                1.0,0.1);
            M[1] = PRCmaterial(
                RGBAColour(0.18,0.0,0.0),
                RGBAColour(0.878431,0.0,0.0),
                RGBAColour(0.32,0.0,0.0),
                RGBAColour(0.072,0.0,0.0),
                0.5,0.1);
            uint32_t MI[nI] = {0,1,0};
            const size_t nN = 2;
            double N[nN][3] = {{0,0,1},{0,0,-1}};
            uint32_t NI[nI][3] = {{0,0,0},{0,0,0},{1,1,1}};

            const uint32_t nC = 3;
            RGBAColour C[nC];
            uint32_t CI[nI][3] = {{0,0,0},{1,1,1},{1,1,2}};

            PRCmaterial materialGreen(
                RGBAColour(0.0,0.18,0.0),
                RGBAColour(0.0,0.878431,0.0),
                RGBAColour(0.0,0.32,0.0),
                RGBAColour(0.0,0.072,0.0),
                1.0,0.1);

            if(1){
                t[0][0]=1; t[0][1]=0; t[0][2]=0; t[0][3]=0;
                t[1][0]=0; t[1][1]=1; t[1][2]=0; t[1][3]=0;
                t[2][0]=0; t[2][1]=0; t[2][2]=1; t[2][3]=-1;
                t[3][0]=0; t[3][1]=0; t[3][2]=0; t[3][3]=1;
                file.begingroup("triangles_onecolor_with_normals",&grpopt, (const double *)t);
                file.addTriangles(nP, P, nI, PI, materialGreen, nN, N, NI, 0, NULL, NULL, 0, NULL, NULL, 0, NULL, NULL, 0);
                file.endgroup();
            }

            if(1){
                file.begingroup("triangles_onecolor",&grpopt);
                file.addTriangles(nP, P, nI, PI, materialGreen, 0, NULL, NULL, 0, NULL, NULL, 0, NULL, NULL, 0, NULL, NULL, 0);
                file.endgroup();
            }

            if(1){
                t[0][0]=1; t[0][1]=0; t[0][2]=0; t[0][3]=0;
                t[1][0]=0; t[1][1]=1; t[1][2]=0; t[1][3]=0;
                t[2][0]=0; t[2][1]=0; t[2][2]=1; t[2][3]=1;
                t[3][0]=0; t[3][1]=0; t[3][2]=0; t[3][3]=1;
                file.begingroup("triangles_manymaterials",&grpopt,(const double *)t);
                file.addTriangles(nP, P, nI, PI, materialGreen, 0, NULL, NULL, 0, NULL, NULL, 0, NULL, NULL, nM, M, MI, 0);
                file.endgroup();
            }

            if(1){
                t[0][0]=1; t[0][1]=0; t[0][2]=0; t[0][3]=0;
                t[1][0]=0; t[1][1]=1; t[1][2]=0; t[1][3]=0;
                t[2][0]=0; t[2][1]=0; t[2][2]=1; t[2][3]=2;
                t[3][0]=0; t[3][1]=0; t[3][2]=0; t[3][3]=1;
                PRCmaterial materialBase(
                    RGBAColour(0.1,0.1,0.1,1),
                    RGBAColour(1,1,1,1),
                    RGBAColour(0.1,0.1,0.1,1),
                    RGBAColour(0.1,0.1,0.1,1),
                    1.0,0.1);
                C[0] = RGBAColour(1,0,0,0.1);
                C[1] = RGBAColour(0,1,0,0.5);
                C[2] = RGBAColour(0,0,1,0.9);
                file.begingroup("triangles_rgba_vertexcolors_on_opaque_a_component_of_vertexcolor_ignored",&grpopt,(const double *)t);
                file.addTriangles(nP, P, nI, PI, materialBase, 0, NULL, NULL, 0, NULL, NULL, nC, C, CI, 0, NULL, NULL, 0);
                file.endgroup();

                t[0][0]=1; t[0][1]=0; t[0][2]=0; t[0][3]=0;
                t[1][0]=0; t[1][1]=1; t[1][2]=0; t[1][3]=0;
                t[2][0]=0; t[2][1]=0; t[2][2]=1; t[2][3]=3;
                t[3][0]=0; t[3][1]=0; t[3][2]=0; t[3][3]=1;
                PRCmaterial materialTransparent(
                    RGBAColour(0.1,0.1,0.1,0.3),
                    RGBAColour(1,1,1,0.3),
                    RGBAColour(0.1,0.1,0.1,0.3),
                    RGBAColour(0.1,0.1,0.1,0.3),
                    0.3,0.1);
                C[0] = RGBAColour(1,0,0,0.1);
                C[1] = RGBAColour(0,1,0,0.5);
                C[2] = RGBAColour(0,0,1,0.9);
                file.begingroup("triangles_rgba_vertexcolors_on_transparent_a_component_of_vertexcolor_ignored",&grpopt,(const double *)t);
                file.addTriangles(nP, P, nI, PI, materialTransparent, 0, NULL, NULL, 0, NULL, NULL, nC, C, CI, 0, NULL, NULL, 0);
                file.endgroup();

                t[0][0]=1; t[0][1]=0; t[0][2]=0; t[0][3]=0;
                t[1][0]=0; t[1][1]=1; t[1][2]=0; t[1][3]=0;
                t[2][0]=0; t[2][1]=0; t[2][2]=1; t[2][3]=4;
                t[3][0]=0; t[3][1]=0; t[3][2]=0; t[3][3]=1;
                C[0] = RGBAColour(1,0,0,0.5);
                C[1] = RGBAColour(0,1,0,0.5);
                C[2] = RGBAColour(0,0,1,0.5);
                file.begingroup("triangles_rgb_vertexcolors_on_transparent_may_not_work_in_OpenGL",&grpopt,(const double *)t);
                file.addTriangles(nP, P, nI, PI, materialTransparent, 0, NULL, NULL, 0, NULL, NULL, nC, C, CI, 0, NULL, NULL, 0);
                file.endgroup();
            }

            if(1){
                t[0][0]=1; t[0][1]=0; t[0][2]=0; t[0][3]=0;
                t[1][0]=0; t[1][1]=1; t[1][2]=0; t[1][3]=0;
                t[2][0]=0; t[2][1]=0; t[2][2]=1; t[2][3]=5;
                t[3][0]=0; t[3][1]=0; t[3][2]=0; t[3][3]=1;
                const uint32_t picture_width=2;
                const uint32_t picture_height=2;
                const uint8_t picRGB[picture_width*picture_height*3] =
                {255,0,0, 0,255,0, 0,0,255, 0,0,0 };
                //      {255,255,0, 255,0,0, 255,0,0, 255,0,0 };
                //     { 255,0,0, 255,255,0, 255,255,0, 255,255,255 };
                //      {255,0,0, 0,255,0, 0,0,255, 0,0,0 };

                const uint8_t picRGBA[picture_width*picture_height*4] =
                {255,0,0,255, 0,255,0,150, 0,0,255,150, 0,0,0,100 };
                // (1,0) 2 3 (1,1)
                // (0,0) 0 1 (1,0)
                uint8_t *pictureRGB = new uint8_t[picture_width*picture_height*3];
                for(size_t i=0; i<picture_width*picture_height*3; i++) pictureRGB[i]=picRGB[i];
                picture1 = pictureRGB;
                uint8_t *pictureRGBA = new uint8_t[picture_width*picture_height*4];
                for(size_t i=0; i<picture_width*picture_height*4; i++) pictureRGBA[i]=picRGBA[i];
                picture2 = pictureRGBA;
                const uint32_t nT = 4;
                const double T[nT][2] = { {0.1,0.1}, {0.9,0.1}, {0.9,0.9}, {0.9,0.1} };
                uint32_t TI[nI][3] = {{0,1,3},{1,2,3},{3,2,3}};

                PRCmaterial materialBase(
                    RGBAColour(0.1,0.1,0.1,1.0),
                    RGBAColour(1,1,1,1),
                    RGBAColour(0.1,0.1,0.1,1),
                    RGBAColour(0.1,0.1,0.1,1),
                    1.0,0.1,
                    pictureRGB, KEPRCPicture_BITMAP_RGB_BYTE,
                    picture_width, picture_height, 0, false, false);
                file.begingroup("triangles_rgb_texture",&grpopt,(const double *)t);
                file.addTriangles(nP, P, nI, PI, materialBase, 0, NULL, NULL, nT, T, TI, 0, NULL, NULL, 0, NULL, NULL, 0);
                file.endgroup();

                t[0][0]=1; t[0][1]=0; t[0][2]=0; t[0][3]=0;
                t[1][0]=0; t[1][1]=1; t[1][2]=0; t[1][3]=0;
                t[2][0]=0; t[2][1]=0; t[2][2]=1; t[2][3]=6;
                t[3][0]=0; t[3][1]=0; t[3][2]=0; t[3][3]=1;
                PRCmaterial materialTransparent(
                    RGBAColour(0.1,0.1,0.1,0.3),
                    RGBAColour(1,1,1,0.3),
                    RGBAColour(0.1,0.1,0.1,0.3),
                    RGBAColour(0.1,0.1,0.1,0.3),
                    0.3,0.1,
                    pictureRGBA, KEPRCPicture_BITMAP_RGB_BYTE,
                    picture_width, picture_height, 0, true, true);
                file.begingroup("triangles_rgba_texture_replacing",&grpopt,(const double *)t);
                file.addTriangles(nP, P, nI, PI, materialTransparent, 0, NULL, NULL, nT, T, TI, 0, NULL, NULL, 0, NULL, NULL, 0);
                file.endgroup();
            }

        }

        if(1){
            PRCoptions grpopt;
            grpopt.no_break = true;
            grpopt.do_break = false;
            grpopt.tess = true;
            grpopt.closed = true;

            double t[4][4];

            const size_t nP = 6;
            double P[nP][3] = {{3+0,0,0},{3+1,0,0},{3+0,1,0},{3+1,1,0},{3+0,2,0},{3+1,2,0}};
            const size_t nI = 2;
            uint32_t PI[nI][4] = {{0,1,3,2},{2,3,5,4}};
            const size_t nM = 2;
            PRCmaterial M[nM];
            M[0] = PRCmaterial(
                RGBAColour(0.0,0.0,0.18),
                RGBAColour(0.0,0.0,0.878431),
                RGBAColour(0.0,0.0,0.32),
                RGBAColour(0.0,0.0,0.072),
                1.0,0.1);
            M[1] = PRCmaterial(
                RGBAColour(0.18,0.0,0.0),
                RGBAColour(0.878431,0.0,0.0),
                RGBAColour(0.32,0.0,0.0),
                RGBAColour(0.072,0.0,0.0),
                0.5,0.1);
            uint32_t MI[nI] = {0,1};
            const size_t nN = 2;
            double N[nN][3] = {{0,0,1},{0,0,-1}};
            uint32_t NI[nI][4] = {{0,0,0,0},{1,1,1,1}};

            const uint32_t nC = 3;
            RGBAColour C[nC];
            uint32_t CI[nI][4] = {{0,0,1,1},{1,1,2,2}};
            C[0] = RGBAColour(1,0,0,0.5);
            C[1] = RGBAColour(0,1,0,0.5);
            C[2] = RGBAColour(0,0,1,0.5);

            PRCmaterial materialGreen(
                RGBAColour(0.0,0.18,0.0),
                RGBAColour(0.0,0.878431,0.0),
                RGBAColour(0.0,0.32,0.0),
                RGBAColour(0.0,0.072,0.0),
                1.0,0.1);

            PRCmaterial materialBase(
                RGBAColour(0.1,0.1,0.1,1),
                RGBAColour(1,1,1,1),
                RGBAColour(0.1,0.1,0.1,1),
                RGBAColour(0.1,0.1,0.1,1),
                1.0,0.1);

            PRCmaterial materialTransparent(
                RGBAColour(0.1,0.1,0.1,0.3),
                RGBAColour(1,1,1,0.3),
                RGBAColour(0.1,0.1,0.1,0.3),
                RGBAColour(0.1,0.1,0.1,0.3),
                0.3,0.1);

            if(1){
                t[0][0]=1; t[0][1]=0; t[0][2]=0; t[0][3]=0;
                t[1][0]=0; t[1][1]=1; t[1][2]=0; t[1][3]=0;
                t[2][0]=0; t[2][1]=0; t[2][2]=1; t[2][3]=-1;
                t[3][0]=0; t[3][1]=0; t[3][2]=0; t[3][3]=1;
                file.begingroup("quads_onecolor_with_normals",&grpopt,(const double *)t);
                file.addQuads(nP, P, nI, PI, materialGreen, nN, N, NI, 0, NULL, NULL, 0, NULL, NULL, 0, NULL, NULL, 0);
                file.endgroup();
            }

            if(1){
                file.begingroup("quads_onecolor",&grpopt);
                file.addQuads(nP, P, nI, PI, materialGreen, 0, NULL, NULL, 0, NULL, NULL, 0, NULL, NULL, 0, NULL, NULL, 0);
                file.endgroup();
            }

            if(1){
                t[0][0]=1; t[0][1]=0; t[0][2]=0; t[0][3]=0;
                t[1][0]=0; t[1][1]=1; t[1][2]=0; t[1][3]=0;
                t[2][0]=0; t[2][1]=0; t[2][2]=1; t[2][3]=1;
                t[3][0]=0; t[3][1]=0; t[3][2]=0; t[3][3]=1;
                file.begingroup("quads_manymaterials",&grpopt,(const double *)t);
                file.addQuads(nP, P, nI, PI, materialGreen, 0, NULL, NULL, 0, NULL, NULL, 0, NULL, NULL, nM, M, MI, 0);
                file.endgroup();
            }

            if(1){
                t[0][0]=1; t[0][1]=0; t[0][2]=0; t[0][3]=0;
                t[1][0]=0; t[1][1]=1; t[1][2]=0; t[1][3]=0;
                t[2][0]=0; t[2][1]=0; t[2][2]=1; t[2][3]=2;
                t[3][0]=0; t[3][1]=0; t[3][2]=0; t[3][3]=1;
                file.begingroup("quads_rgb_vertexcolors_on_transparent_may_not_work_in_OpenGL",&grpopt,(const double *)t);
                file.addQuads(nP, P, nI, PI, materialTransparent, 0, NULL, NULL, 0, NULL, NULL, nC, C, CI, 0, NULL, NULL, 0);
                file.endgroup();
            }

            if(1){
                t[0][0]=1; t[0][1]=0; t[0][2]=0; t[0][3]=0;
                t[1][0]=0; t[1][1]=1; t[1][2]=0; t[1][3]=0;
                t[2][0]=0; t[2][1]=0; t[2][2]=1; t[2][3]=5;
                t[3][0]=0; t[3][1]=0; t[3][2]=0; t[3][3]=1;
                const uint32_t picture_width=2;
                const uint32_t picture_height=2;
                const uint8_t picRGB[picture_width*picture_height*3] =
                {255,0,0, 0,255,0, 0,0,255, 0,0,0 };
                //      {255,255,0, 255,0,0, 255,0,0, 255,0,0 };
                //     { 255,0,0, 255,255,0, 255,255,0, 255,255,255 };
                //      {255,0,0, 0,255,0, 0,0,255, 0,0,0 };

                const uint8_t picRGBA[picture_width*picture_height*4] =
                {255,0,0,255, 0,255,0,150, 0,0,255,150, 0,0,0,100 };
                // (1,0) 2 3 (1,1)
                // (0,0) 0 1 (1,0)
                uint8_t *pictureRGB = new uint8_t[picture_width*picture_height*3];
                for(size_t i=0; i<picture_width*picture_height*3; i++) pictureRGB[i]=picRGB[i];
                picture3 = pictureRGB;
                uint8_t *pictureRGBA = new uint8_t[picture_width*picture_height*4];
                for(size_t i=0; i<picture_width*picture_height*4; i++) pictureRGBA[i]=picRGBA[i];
                picture4 = pictureRGBA;
                const uint32_t nT = 3;
                const double T[nT][2] = { {0.1,0.1}, {0.5,0.5}, {0.9,0.9} };
                uint32_t TI[nI][4] = {{0,0,1,1},{1,1,2,2}};

                PRCmaterial materialBase(
                    RGBAColour(0.1,0.1,0.1,1),
                    RGBAColour(1,1,1,1),
                    RGBAColour(0.1,0.1,0.1,1),
                    RGBAColour(0.1,0.1,0.1,1),
                    1.0,0.1,
                    pictureRGB, KEPRCPicture_BITMAP_RGB_BYTE,
                    picture_width, picture_height, 0, false, false);
                file.begingroup("quads_rgb_texture",&grpopt,(const double *)t);
                file.addQuads(nP, P, nI, PI, materialBase, 0, NULL, NULL, nT, T, TI, 0, NULL, NULL, 0, NULL, NULL, 0);
                file.endgroup();

                t[0][0]=1; t[0][1]=0; t[0][2]=0; t[0][3]=0;
                t[1][0]=0; t[1][1]=1; t[1][2]=0; t[1][3]=0;
                t[2][0]=0; t[2][1]=0; t[2][2]=1; t[2][3]=6;
                t[3][0]=0; t[3][1]=0; t[3][2]=0; t[3][3]=1;
                PRCmaterial materialTransparent(
                    RGBAColour(0.1,0.1,0.1,0.3),
                    RGBAColour(1,1,1,0.3),
                    RGBAColour(0.1,0.1,0.1,0.3),
                    RGBAColour(0.1,0.1,0.1,0.3),
                    0.6,0.1,
                    pictureRGBA, KEPRCPicture_BITMAP_RGB_BYTE,
                    picture_width, picture_height, 0, true, true);
                file.begingroup("quads_rgba_texture_replacing",&grpopt,(const double *)t);
                file.addQuads(nP, P, nI, PI, materialTransparent, 0, NULL, NULL, nT, T, TI, 0, NULL, NULL, 0, NULL, NULL, 0);
                file.endgroup();
            }

        }

        if(1){
            file.begingroup("polyline",&grpopt);
            file.addLine(NUMBER_OF_POINTS, points, RGBAColour(1.0,0.0,0.0));
            file.endgroup();

            file.begingroup("polylines",&grpopt);
            file.addLine(NUMBER_OF_POINTS, shifted_points[0], RGBAColour(0.0,1.0,0.0),2.0);
            file.addLine(NUMBER_OF_POINTS, shifted_points[1], RGBAColour(1.0,1.0,0.0),2.0);
            file.endgroup();
        }

        if(1){
            PRCoptions grpopt;
            grpopt.no_break = true;
            grpopt.do_break = false;
            grpopt.tess = true;
            grpopt.closed = true;

            double t[4][4];

            const size_t nP = 5;
            double P[nP][3] = {{0,0,0},{1,0,0},{1,1,0},{0,1,0},{0,2,0}};
            const size_t nI = 1+5 + 1+3;
            uint32_t PI[nI] = {5,0,1,2,3,0, 3,2,4,3 }; // square of 5 points, wire of 2 points
            const uint32_t nC = 5;  // number of colors
            RGBAColour C[nC]; // color for segments
            C[0] = RGBAColour(1,0,0);
            C[1] = RGBAColour(0,1,0);
            C[2] = RGBAColour(0,0,1);
            C[3] = RGBAColour(0,0,0);
            C[4] = RGBAColour(1,1,0);
            const uint32_t nCI = 5-1 + 3-1;  // number of segments
            uint32_t CI[nCI] = { 0,1,2,3, 4,4 };

            t[0][0]=1; t[0][1]=0; t[0][2]=0; t[0][3]=0;
            t[1][0]=0; t[1][1]=1; t[1][2]=0; t[1][3]=0;
            t[2][0]=0; t[2][1]=0; t[2][2]=1; t[2][3]=5;
            t[3][0]=0; t[3][1]=0; t[3][2]=0; t[3][3]=1;
            file.begingroup("polyline_indexed_one_color",&grpopt,(const double *)t);
            file.addLines(nP, P, nI, PI, RGBAColour(1,0,0), 1, false, 0, NULL, 0, NULL);
            file.endgroup();

            t[0][0]=1; t[0][1]=0; t[0][2]=0; t[0][3]=0;
            t[1][0]=0; t[1][1]=1; t[1][2]=0; t[1][3]=0;
            t[2][0]=0; t[2][1]=0; t[2][2]=1; t[2][3]=6;
            t[3][0]=0; t[3][1]=0; t[3][2]=0; t[3][3]=1;
            file.begingroup("polyline_indexed_color_per_segment",&grpopt,(const double *)t);
            file.addLines(nP, P, nI, PI, RGBAColour(1,0,0), 2, true, nC, C, nCI, CI);
            file.endgroup();

            t[0][0]=1; t[0][1]=0; t[0][2]=0; t[0][3]=0;
            t[1][0]=0; t[1][1]=1; t[1][2]=0; t[1][3]=0;
            t[2][0]=0; t[2][1]=0; t[2][2]=1; t[2][3]=7;
            t[3][0]=0; t[3][1]=0; t[3][2]=0; t[3][3]=1;
            const uint32_t nCI2 = 5 + 3;  // number of vertices
            uint32_t CI2[nCI2] = { 0,1,2,3,0, 4,4,4 };
            file.begingroup("polyline_indexed_color_per_vertex_is_broken",&grpopt,(const double *)t);
            file.addLines(nP, P, nI, PI, RGBAColour(1,0,0), 2, false, nC, C, nCI2, CI2);
            file.endgroup();
        }

        if(1)
        {
            file.begingroup("bezier_wire");
            file.addBezierCurve(NUMBER_OF_POINTS,points,RGBAColour(1.0,1.0,1.0));
            file.endgroup();
        }
        if(1)
        {
            file.begingroup("NURBS_wire");
            file.addCurve(3, NUMBER_OF_POINTS, points, knots, RGBAColour(1.0,1.0,1.0), NULL); // genarl API for the above
            file.endgroup();
        }

        // following box examples show a) different ways to represent a surface consisting of flat rectangles
        // b) that the only way to have almost working transparency is a set of NURBS bodies.
        // (Or may be other topology types like plane also work
        // demonstration how non-transparent materials work the same for all kinds of objects  

        if(1) { // demonstration how non-transparent materials work the same for all kinds of objects  
            const size_t NUMBER_OF_PATCHES = 6;
            double vertices[NUMBER_OF_PATCHES][4][3] = 
            {
                { // Patch 0
                    {-1,-1,-1},
                    { 1,-1,-1},
                    {-1, 1,-1},
                    { 1, 1,-1}
                },
                { // Patch 1
                    {-1,-1, 1},
                    { 1,-1, 1},
                    {-1, 1, 1},
                    { 1, 1, 1}
                },
                { // Patch 2
                    {-1,-1,-1},
                    { 1,-1,-1},
                    {-1,-1, 1},
                    { 1,-1, 1}
                },
                { // Patch 3
                    {-1, 1,-1},
                    { 1, 1,-1},
                    {-1, 1, 1},
                    { 1, 1, 1}
                },
                { // Patch 4
                    {-1,-1,-1},
                    {-1, 1,-1},
                    {-1,-1, 1},
                    {-1, 1, 1}
                },
                { // Patch 5
                    { 1,-1,-1},
                    { 1, 1,-1},
                    { 1,-1, 1},
                    { 1, 1, 1}
                }
            };
            const size_t NUMBER_OF_BOXES = 6;
            double shifted_vertices[NUMBER_OF_BOXES][NUMBER_OF_PATCHES][4][3];
            for(size_t box = 0; box < NUMBER_OF_BOXES; ++box)
                for(size_t patch = 0; patch < NUMBER_OF_PATCHES; ++patch)
                    for(size_t i = 0; i < 4; ++i)
                    {
                        shifted_vertices[box][patch][i][0] = vertices[patch][i][0]+3*box;
                        shifted_vertices[box][patch][i][1] = vertices[patch][i][1];
                        shifted_vertices[box][patch][i][2] = vertices[patch][i][2]-2;
                    }
                    PRCmaterial materialGreen(
                        RGBAColour(0.0,0.18,0.0),
                        RGBAColour(0.0,0.878431,0.0),
                        RGBAColour(0.0,0.32,0.0),
                        RGBAColour(0.0,0.072,0.0),
                        1.0,0.1);

                    PRCoptions grpopt;
                    file.begingroup("TransparentBox");
                    grpopt.no_break = false;
                    grpopt.do_break = true;
                    grpopt.tess = false;
                    file.begingroup("TransparentBoxSetOfNURBSBodies",&grpopt);
                    for(size_t patch = 0; patch < NUMBER_OF_PATCHES; ++patch)
                    {
                        file.addRectangle(shifted_vertices[0][patch], materials[(patch*5)%N_COLOURS]);
                    }
                    file.endgroup();
                    grpopt.no_break = true;
                    grpopt.do_break = false;
                    grpopt.tess = false;
                    file.begingroup("TransparentBoxNURBSFaces",&grpopt);
                    for(size_t patch = 0; patch < NUMBER_OF_PATCHES; ++patch)
                    {
                        file.addRectangle(shifted_vertices[1][patch], materials[(patch*5)%N_COLOURS]);
                    }
                    file.endgroup();
                    grpopt.no_break = true;
                    grpopt.do_break = false;
                    grpopt.tess = true;
                    file.begingroup("TransparentBoxTessellated",&grpopt);
                    for(size_t patch = 0; patch < NUMBER_OF_PATCHES; ++patch)
                    {
                        file.addRectangle(shifted_vertices[2][patch], materials[(patch*5)%N_COLOURS]);
                    }
                    file.endgroup();
                    file.endgroup();

                    file.begingroup("Box");
                    grpopt.no_break = false;
                    grpopt.do_break = false;
                    grpopt.tess = true;
                    file.begingroup("BoxTessellated",&grpopt);
                    for(size_t patch = 0; patch < NUMBER_OF_PATCHES; ++patch)
                    {
                        file.addRectangle(shifted_vertices[3][patch], materialGreen);
                    }
                    file.endgroup();
                    grpopt.no_break = true;
                    grpopt.do_break = false;
                    grpopt.tess = false;
                    file.begingroup("BoxNURBSFaces",&grpopt);
                    for(size_t patch = 0; patch < NUMBER_OF_PATCHES; ++patch)
                    {
                        file.addRectangle(shifted_vertices[4][patch], materialGreen);
                    }
                    file.endgroup();
                    grpopt.no_break = false;
                    grpopt.do_break = true;
                    grpopt.tess = false;
                    file.begingroup("BoxSetOfNURBSBodies",&grpopt);
                    for(size_t patch = 0; patch < NUMBER_OF_PATCHES; ++patch)
                    {
                        file.addRectangle(shifted_vertices[5][patch], materialGreen);
                    }
                    file.endgroup();
                    file.endgroup();
        }

        if(1) {
            PRCmaterial materialGreen(
                RGBAColour(0.0,0.18,0.0),
                RGBAColour(0.0,0.878431,0.0),
                RGBAColour(0.0,0.32,0.0),
                RGBAColour(0.0,0.072,0.0),
                1.0,0.1);
            PRCoptions grpopt;
            grpopt.closed = true;

            file.begingroup("Complex elements, one-sided");

            const double disk_origin[3] = {11,0,2};
            const double disk_x_axis[3] = {1,0,0};
            const double disk_y_axis[3] = {0,-1,0};
            const double disk_scale = 2;
            file.begingroup("disk",&grpopt);
            file.addDisk(1,materialGreen,disk_origin,disk_x_axis,disk_y_axis,disk_scale);
            file.endgroup();
            const double hs_origin[3] = {11,0,2};
            const double hs_x_axis[3] = {1,0,0};
            const double hs_y_axis[3] = {0,1,0};
            const double hs_scale = 2;
            file.begingroup("hemisphere",&grpopt);
            file.addHemisphere(1,materialGreen,hs_origin,hs_x_axis,hs_y_axis,hs_scale);
            file.endgroup();
            const double cyl_origin[3] = {11,0,1};
            const double cyl_x_axis[3] = {1,0,0};
            const double cyl_y_axis[3] = {0,1,0};
            const double cyl_scale = 1;
            file.begingroup("cylinder",&grpopt);
            file.addCylinder(1,1,materialGreen,cyl_origin,cyl_x_axis,cyl_y_axis,cyl_scale);
            file.endgroup();
            const double cone_origin[3] = {11,0,1};
            const double cone_x_axis[3] = {1,0,0};
            const double cone_y_axis[3] = {0,-1,0};
            const double cone_scale = 1;
            file.begingroup("cone",&grpopt);
            file.addCone(1,1,materialGreen,cone_origin,cone_x_axis,cone_y_axis,cone_scale);
            file.endgroup();
            const double sp_origin[3] = {11,0,-1};
            const double sp_x_axis[3] = {1,0,0};
            const double sp_y_axis[3] = {0,1,0};
            const double sp_scale = 1;
            file.begingroup("sphere",&grpopt);
            file.addSphere(0.5,materialGreen,sp_origin,sp_x_axis,sp_y_axis,sp_scale);
            file.endgroup();
            const double tor_origin[3] = {11,0,0};
            const double tor_x_axis[3] = {1,0,0};
            const double tor_y_axis[3] = {0,1,0};
            const double tor_scale = 1;
            file.begingroup("torus",&grpopt);
            file.addTorus(0.5,0.1,0,360,materialGreen,tor_origin,tor_x_axis,tor_y_axis,tor_scale);
            file.endgroup();

            const size_t NUMBER_OF_POINTS = 4;
            double cpoints[NUMBER_OF_POINTS][3];
            cpoints[0][0] = 1; cpoints[0][1] = 0;                 cpoints[0][2] = 0;
            cpoints[1][0] = 1; cpoints[1][1] = 0.552284749830793; cpoints[1][2] = 0;
            cpoints[2][0] = 0.552284749830793; cpoints[2][1] = 1; cpoints[2][2] = 0;
            cpoints[3][0] = 0;                 cpoints[3][1] = 1; cpoints[3][2] = 0;
            double opoints[NUMBER_OF_POINTS][3];
            opoints[0][0] = 1.1*1; opoints[0][1] = 1.1*0;                 opoints[0][2] = 0;
            opoints[1][0] = 1.1*1; opoints[1][1] = 1.1*0.552284749830793; opoints[1][2] = 0;
            opoints[2][0] = 1.1*0.552284749830793; opoints[2][1] = 1.1*1; opoints[2][2] = 0;
            opoints[3][0] = 1.1*0;                 opoints[3][1] = 1.1*1; opoints[3][2] = 0;
            const double tube_origin[3] = {11,0,0};
            const double tube_x_axis[3] = {1,0,0};
            const double tube_y_axis[3] = {0,1,0};
            const double tube_scale = 1;
            file.begingroup("tube",&grpopt);
            file.addTube(NUMBER_OF_POINTS,cpoints,opoints,false,materialGreen,tube_origin,tube_x_axis,tube_y_axis,tube_scale);
            file.endgroup();

            file.endgroup();
        }

    file.finish();

    delete[] picture1;
    delete[] picture2;
    delete[] picture3;
    delete[] picture4;
}

void createPdf( const std::string &pdffile, const std::string &prcfile, const std::string &jsfile )
{
    Pdf3d pdf;
    pdf.createAdvancedPdf( pdffile.c_str(), prcfile.c_str(), jsfile.c_str() );
}
