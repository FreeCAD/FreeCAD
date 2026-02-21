
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgDB/FileNameUtils>

#include <iostream>

int main( int argc, char** argv )
{
    if( argc != 3 )
    {
        std::cerr << "prcconvert <infile> <outfile>" << std::endl;
        return( 1 );
    }

    
const std::string inFileName( argv[ 1 ] );
    const std::string outFileName( argv[ 2 ] );
    osg::ref_ptr< osg::Node > root( osgDB::readNodeFile( inFileName ) );
    if( root == NULL )
    {
        std::cerr << "Error in reading file " << inFileName << std::endl;
        return( 1 );
    }

    bool writeResult( osgDB::writeNodeFile( *root, outFileName ) );
    if( !writeResult )
    {
        std::cerr << "Error in writing file " << outFileName << std::endl;
        return( 1 );
    }

    std::cout << "Success: " << inFileName << " -> " << outFileName << std::endl;
    return( 0 );
}
