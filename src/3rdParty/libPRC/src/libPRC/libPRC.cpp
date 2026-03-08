#include "libPRC.h"
#include <PRCFile.h>

#include <cstddef>
#include <cstring>


namespace prc {


File* open( const char* fileName, const char* opt )
{
    if( ( opt == NULL ) ||
        ( strlen( opt ) != 1 ) )
        return( NULL );

    if( strcmp( opt, "r" ) == 0 )
        // Read-only not yet implemented.
        return( NULL );

    if( strcmp( opt, "w" ) != 0 )
        // Invalid option. Must be either "r" or "w".
        return( NULL );

    // file fptr = fopen ...
    // File* prcFile( new File( fptr )
    // return( prcFile );
    File* prcFile( new File() );

    return( prcFile );
}

bool close( File* prcFile )
{
    if( prcFile != NULL )
    {
        delete( prcFile );
        return( true );
    }

    // If prcFile was opened for writing
    //   Write data to file
    // close the fptr.
    // return( true ) on success.

    return( false );
}


// namespace prc
}
