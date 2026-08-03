// SPDX-License-Identifier: LGPL-2.1-or-later

// Wild Magic Source Code
// David Eberly
// http://www.geometrictools.com
// Copyright (c) 1998-2007
//
// This library is free software; you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation; either version 2.1 of the License, or (at
// your option) any later version.  The license is available for reading at
// either of the locations:
//     http://www.gnu.org/copyleft/lgpl.html
//     http://www.geometrictools.com/License/WildMagicLicense.pdf
// The license applies to versions 0 through 4 of Wild Magic.
//
// Version: 4.0.0 (2006/06/28)

#pragma once

#include "Wm4FoundationLIB.h"
#include "Wm4System.h"

// The class TYPE is either native data or is class data that has the
// following member functions:
//   TYPE::TYPE ()
//   TYPE::TYPE (const TYPE&);
//   TYPE& TYPE::operator= (const TYPE&)

namespace Wm4
{

template <int DIMENSION, class TYPE>
class TTuple
{
public:
    // Construction and destruction.  The default constructor does not
    // initialize the tuple elements for native elements.  The tuple elements
    // are initialized for class data whenever TYPE initializes during its
    // default construction.
    TTuple ();
    TTuple (const TTuple& rkT);
    ~TTuple ();

    // coordinate access
    operator const TYPE* () const;
    operator TYPE* ();
    TYPE operator[] (int i) const;
    TYPE& operator[] (int i);

    // assignment
    TTuple& operator= (const TTuple& rkT);

    // Comparison.  The inequalities make the comparisons using memcmp, thus
    // treating the tuple as an array of unsigned bytes.
    bool operator== (const TTuple& rkT) const;
    bool operator!= (const TTuple& rkT) const;
    bool operator<  (const TTuple& rkT) const;
    bool operator<= (const TTuple& rkT) const;
    bool operator>  (const TTuple& rkT) const;
    bool operator>= (const TTuple& rkT) const;

private:
    TYPE m_atTuple[DIMENSION];
};

}

#include "Wm4TTuple.inl"