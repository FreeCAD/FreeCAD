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

namespace Wm4
{

class Query
{
public:
    // abstract base class
    virtual ~Query ();

    // run-time type information
    enum Type
    {
        QT_INT64,
        QT_INTEGER,
        QT_RATIONAL,
        QT_REAL,
        QT_FILTERED
    };

    virtual Type GetType () const = 0;

protected:
    Query ();
};

}

#include "Wm4Query.inl"