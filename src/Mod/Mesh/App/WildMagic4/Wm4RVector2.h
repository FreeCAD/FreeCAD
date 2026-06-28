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
class RVector2 : public TRVector<2,ISIZE>
{
public:
    // construction
    RVector2 ();
    RVector2 (const RVector2& rkV);

#ifdef WM4_USING_VC70
    RVector2 (const TRVector<2,ISIZE>& rkV)
    {
        // The inline body is here because of an apparent MSVC++ .NET 2002
        // compiler bug.  If placed in the *.inl file, the compiler complains:
        //
        //   error C2244: 'Wm4::RVector2<>::__ctor' : unable to match function
        //       definition to an existing declaration
        //   definition
        //       'Wm4::RVector2<>::RVector2(const Wm4::TRVector<2,> &)'
        //   existing declarations
        //       'Wm4::RVector2<>::RVector2(const Wm4::TRational<> &,
        //                                  const Wm4::TRational<> &)'
        //       'Wm4::RVector2<>::RVector2(const Wm4::TRVector<2,> &)'
        //       'Wm4::RVector2<>::RVector2(const Wm4::RVector2<> &)'
        //       'Wm4::RVector2<>::RVector2(void)'
        // The "definition" is in the "existing declarations" list, so I do
        // not know what the compiler is complaining about.

        m_akTuple[0] = rkV[0];
        m_akTuple[1] = rkV[1];
    }
#else
    RVector2 (const TRVector<2,ISIZE>& rkV);
#endif

    RVector2 (const TRational<ISIZE>& rkX, const TRational<ISIZE>& rkY);

    // member access
    TRational<ISIZE> X () const;
    TRational<ISIZE>& X ();
    TRational<ISIZE> Y () const;
    TRational<ISIZE>& Y ();

    // assignment
    RVector2& operator= (const RVector2& rkV);

#ifdef WM4_USING_VC70
    RVector2& operator= (const TRVector<2,ISIZE>& rkV)
    {
        // The inline body is here because of an apparent MSVC++ .NET 2002
        // compiler bug.  If placed in the *.inl file, the compiler complains:
        //
        //   error C2244: 'Wm4::RVector2<>::operator`='' : unable to match
        //       function definition to an existing declaration
        //   definition
        //       'Wm4::RVector2<> &Wm4::RVector2<>::operator =(
        //            const Wm4::TRVector<2,> &)'
        //   existing declarations
        //       'Wm4::RVector2<> &Wm4::RVector2<>::operator =(
        //            const Wm4::TRVector<2,> &)'
        //       'Wm4::RVector2<> &Wm4::RVector2<>::operator =(
        //            const Wm4::RVector2<> &)'

        m_akTuple[0] = rkV[0];
        m_akTuple[1] = rkV[1];
        return *this;
    }
#else
    RVector2& operator= (const TRVector<2,ISIZE>& rkV);
#endif

    // returns Dot(this,V)
    TRational<ISIZE> Dot (const RVector2& rkV) const;

    // returns (y,-x)
    RVector2 Perp () const;

    // returns Cross((x,y,0),(V.x,V.y,0)) = x*V.y - y*V.x
    TRational<ISIZE> DotPerp (const RVector2& rkV) const;

protected:
    using TRVector<2,ISIZE>::m_akTuple;
};

}

#include "Wm4RVector2.inl"