
#include "PRCFile.h"
#include "PRCNode.h"

#include <cstddef>


namespace prc {


File::File()
    : _root( NULL )
{
}
File::~File()
{
    if( _root != NULL )
        recurseDelete( _root );
}

Node* File::getRoot()
{
    return( _root );
}
void File::setRoot( Node* root )
{
    if( _root != NULL )
        recurseDelete( _root );
    _root = root;
}

void File::recurseDelete( Node* node )
{
}



// namespace prc
}
