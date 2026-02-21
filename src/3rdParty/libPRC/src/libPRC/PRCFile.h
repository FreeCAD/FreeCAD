
#ifndef __prc_file_h__
#define __prc_file_h__ 1


#include <Export.h>


namespace prc {


class Node;


/** \class File PRCFile.h <libPRC\PRCFile.h>
\brief TBD
\details TBD
*/
class PRC_EXPORT File
{
public:
    File();
    virtual ~File();

    Node* getRoot();
    void setRoot( Node* root );

protected:
    void recurseDelete( Node* node );

    Node* _root;
};


// namespace prc
}


// __prc_file_h__
#endif
