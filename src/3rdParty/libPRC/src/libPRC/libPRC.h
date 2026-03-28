
#ifndef __lib_prc_h__
#define __lib_prc_h__ 1


#include <Export.h>
#include <PRCFile.h>
#include <PRCNode.h>


namespace prc {


/** \brief Open a PRC file.
\details If \c opt is "w", create the file if it doesn't
already exist. The PRC file will be written during the
subsequent close() call.

If \c opt is "r", the existin g PRC file is opened for
reading. Note that reading is currently not implemented,
so NULL will be returned for now.

Returns NULL if the file can not be opened. */
PRC_EXPORT File* open( const char* fileName, const char* opt );

/** \brief Close a PRC file.
\detauls Returns true on success. Returns false if the file
can not be closed, or if \c prcFile is not a pointer to a
valid prc::File object. */
PRC_EXPORT bool close( File* prcFile );


// namespace prc
}


// __lib_prc_h__
#endif
