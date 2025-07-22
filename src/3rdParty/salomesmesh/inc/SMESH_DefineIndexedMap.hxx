// File:        NCollection_DefineIndexedMap.hxx
// Created:     Thu Apr 24 15:02:53 2002
// Author:      Alexander KARTOMIN (akm)
//              <akm@opencascade.com>
//
// Purpose:     An indexed map is used to  store  keys and to bind
//              an index to them.  Each new key stored in  the map
//              gets an index.  Index are incremented  as keys are
//              stored in the map. A key can be found by the index
//              and an index by the  key. No key  but the last can
//              be removed so the indices are in the range 1..Extent.
//              See  the  class   Map   from NCollection   for   a
//              discussion about the number of buckets.
//              

#ifndef SMESH_DefineIndexedMap_HeaderFile
#define SMESH_DefineIndexedMap_HeaderFile

#include <NCollection_DefineBaseCollection.hxx>
#include <SMESH_IndexedMap.hxx>

#ifdef WNT
// Disable the warning "operator new unmatched by delete"
#pragma warning (disable:4291)
#endif

// *********************************************** Class IndexedMap ***********

#define SMESH_DEFINE_INDEXEDMAP(_ClassName_, _BaseCollection_, TheKeyType)           \
        typedef SMESH_IndexedMap <TheKeyType > _ClassName_;

#endif
