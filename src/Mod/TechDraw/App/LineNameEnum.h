/***************************************************************************
 *   Copyright (c) 2023 WandererFan <wandererfan@gmail.com>                *
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

#pragma once

#include <string>
#include <vector>
#include <QCoreApplication>

#include <Mod/TechDraw/TechDrawGlobal.h>


namespace TechDraw
{



//! common definitions for line numbers, names and lineicon names

class TechDrawExport LineName {
public:
    static std::string translationContext(size_t iStandard);
    static std::string currentTranslationContext();

    static std::vector<std::string> ContextStrings;
};

class TechDrawExport ISOLineName {
    Q_DECLARE_TR_FUNCTIONS(TechDraw::ISOLineName)

public:
    enum class ISOLine {
        NOLINE = 0,
        Continuous,
        Dashed,
        DashedSpaced,
        LongDashedDotted,
        LongDashedDoubleDotted,
        LongDashedTripleDotted,
        Dotted,
        LongDashShortDash,
        LongDashDoubleShortDash,
        DashedDotted,
        DoubleDashedDotted,
        DashedDoubleDotted,
        DoubleDashedDoubleDotted,
        DashedTripleDotted,
        DoubleDashedTripleDotted
    };

    static const char* ISOLineNameEnums[];
    static const int   ISOLineNameCount;

private:

};

class TechDrawExport ANSILineName {
    Q_DECLARE_TR_FUNCTIONS(TechDraw::ANSILineName)

public:
    enum class ANSILineType {
        NOLINE = 0,
        Continuous,
        Dashed,
        LongDashedDashed,
        LongDashedDoubleDashed
    };

    static const char* ANSILineNameEnums[];
    static const int   ANSILineNameCount;

private:

};

class TechDrawExport ASMELineName {
    Q_DECLARE_TR_FUNCTIONS(TechDraw::ASMELineName)

public:
    enum class ASMELineType {
        NOLINE = 0,
        Visible,
        Hidden,
        Section,
        Center,
        Symmetry,
        Dimension,
        Extension,
        Leader,
        CuttingPlane,
        ViewingPlane,
        OtherPlane,
        Break1,
        Break2,
        Phantom,
        Stitch1,
        Stitch2,
        Chain
    };

    static const char* ASMELineNameEnums[];
    static const int   ASMELineNameCount;

private:

};
} //end namespace TechDraw