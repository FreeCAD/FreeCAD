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
#include "Wm4TRVector.h"

namespace Wm4
{

template <int ISIZE>
class RVector3 : public TRVector<3,ISIZE>
{
public:
    // construction
    RVector3 ();
    RVector3 (const RVector3& rkV);

#ifdef WM4_USING_VC70
    RVector3 (const TRVector<3,ISIZE>& rkV)
    {
        // The inline body is here because of an apparent MSVC++ .NET 2002
        // compiler bug.  If placed in the *.inl file, the compiler complains:
        //
        //   error C2244: 'Wm4::RVector3<>::__ctor' : unable to match function
        //       definition to an existing declaration
        //   definition
        //       'Wm4::RVector3<>::RVector3(const Wm4::TRVector<3,> &)'
        //   existing declarations
        //       'Wm4::RVector3<>::RVector3(const Wm4::TRational<> &,
        //                                  const Wm4::TRational<> &)'
        //       'Wm4::RVector3<>::RVector3(const Wm4::TRVector<3,> &)'
        //       'Wm4::RVector3<>::RVector3(const Wm4::RVector3<> &)'
        //       'Wm4::RVector3<>::RVector3(void)'
        // The "definition" is in the "existing declarations" list, so I do
        // not know what the compiler is complaining about.

        m_akTuple[0] = rkV[0];
        m_akTuple[1] = rkV[1];
        m_akTuple[2] = rkV[2];
    }
#else
    RVector3 (const TRVector<3,ISIZE>& rkV);
#endif

    RVector3 (const TRational<ISIZE>& rkX, const TRational<ISIZE>& rkY,
        const TRational<ISIZE>& rkZ);

    // member access
    TRational<ISIZE> X () const;
    TRational<ISIZE>& X ();
    TRational<ISIZE> Y () const;
    TRational<ISIZE>& Y ();
    TRational<ISIZE> Z () const;
    TRational<ISIZE>& Z ();

    // assignment
    RVector3& operator= (const RVector3& rkV);

#ifdef WM4_USING_VC70
    RVector3& operator= (const TRVector<3,ISIZE>& rkV)
    {
        // The inline body is here because of an apparent MSVC++ .NET 2002
        // compiler bug.  If placed in the *.inl file, the compiler complains:
        //
        //   error C2244: 'Wm4::RVector3<>::operator`='' : unable to match
        //       function definition to an existing declaration
        //   definition
        //       'Wm4::RVector3<> &Wm4::RVector3<>::operator =(
        //            const Wm4::TRVector<3,> &)'
        //   existing declarations
        //       'Wm4::RVector3<> &Wm4::RVector3<>::operator =(
        //            const Wm4::TRVector<3,> &)'
        //       'Wm4::RVector3<> &Wm4::RVector3<>::operator =(
        //            const Wm4::RVector3<> &)'

        m_akTuple[0] = rkV[0];
        m_akTuple[1] = rkV[1];
        m_akTuple[2] = rkV[2];
        return *this;
    }
#else
    RVector3& operator= (const TRVector<3,ISIZE>& rkV);
#endif

    // returns Dot(this,V)
    TRational<ISIZE> Dot (const RVector3& rkV) const;

    // returns Cross(this,V)
    RVector3 Cross (const RVector3& rkV) const;

    // returns Dot(this,Cross(U,V))
    TRational<ISIZE> TripleScalar (const RVector3& rkU, const RVector3& rkV)
        const;

protected:
    using TRVector<3,ISIZE>::m_akTuple;
};

}

#include "Wm4RVector3.inl"