// SPDX-License-Identifier: LGPL-2.0-or-later

/***************************************************************************
 *   Copyright (c) 2024 wandererfan <wandererfan at gmail dot com>         *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

//! MeasureInfo.h
//! ancestor class for the various flavours of MeasureXXXXXInfo.

#ifndef MEASURE_HANDLERMANAGER_H
#define MEASURE_HANDLERMANAGER_H

#include <Mod/Measure/MeasureGlobal.h>

namespace Measure {

class MeasureExport MeasureInfo {
public:
    MeasureInfo() = default;
    MeasureInfo(bool val) { valid = val; };
    virtual ~MeasureInfo() = default;           // virtual to make class polymorphic

    bool valid{false};
};

}  //end namespace Measure

#endif
