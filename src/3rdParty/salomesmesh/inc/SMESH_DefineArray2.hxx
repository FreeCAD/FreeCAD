// File:      NCollection_DefineArray2.hxx
// Created:   15.04.02 17:05:16
// Author:    Alexander Kartomin (akm)
//            <a-kartomin@opencascade.com>
//            Automatically created from NCollection_Array2.hxx by GAWK
// Copyright: Open Cascade 2002
//            
// Purpose:   The class Array2 represents bi-dimensional arrays 
//            of fixed size known at run time. 
//            The ranges of indices are user defined.
//            
// Warning:   Programs clients of such class must be independent
//            of the range of the first element. Then, a C++ for
//            loop must be written like this
//            
//            for (i = A.LowerRow(); i <= A.UpperRow(); i++)
//              for (j = A.LowerCol(); j <= A.UpperCol(); j++)
//            

#ifndef SMESH_DefineArray2_HeaderFile
#define SMESH_DefineArray2_HeaderFile

#include <NCollection_DefineBaseCollection.hxx>
#include <SMESH_Array2.hxx>

#ifdef WNT
// Disable the warning "operator new unmatched by delete"
#pragma warning (disable:4291)
#endif

// *********************************************** Template for Array2 class

#define SMESH_DEFINE_ARRAY2(_ClassName_, _BaseCollection_, TheItemType)              \
        typedef SMESH_Array2<TheItemType > _ClassName_;

#endif
