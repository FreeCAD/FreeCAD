/***************************************************************************
 *   Copyright (c) 2020 WandererFan <wandererfan@gmail.com>                *
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

#include <Qt>
#include <string>

#include <Base/Parameter.h>
#include <Mod/TechDraw/TechDrawGlobal.h>

#include "DrawBrokenView.h"

class QColor;
class QString;

namespace Base
{
class Color;
}

namespace TechDraw
{
enum class ArrowType : int;

//getters for parameters used in multiple places.
class TechDrawExport Preferences
{

public:
    static Base::Reference<ParameterGrp> getPreferenceGroup(const char* Name);

    static std::string labelFont();
    static QString labelFontQString();
    static double labelFontSizeMM();
    static double dimFontSizeMM();
    static double dimArrowSize();

    static Base::Color normalColor();
    static Base::Color selectColor();
    static Base::Color preselectColor();
    static Base::Color vertexColor();
    static double vertexScale();
    static int scaleType();
    static double scale();
    static bool useGlobalDecimals();
    static bool keepPagesUpToDate();

    static int projectionAngle();
    static bool groupAutoDistribute();
    static double groupSpaceX();
    static double groupSpaceY();

    static ArrowType balloonArrow();
    static double balloonKinkLength();
    static int balloonShape();

    static QString defaultTemplate();
    static QString defaultTemplateDir();

    static const double DefaultFontSizeInMM;
    static const double DefaultArrowSize;

    static std::string formatSpec();
    static int altDecimals();

    static int mattingStyle();
    static bool showDetailMatting();
    static bool showDetailHighlight();

    static std::string svgFile();
    static std::string patFile();

    static QString defaultSymbolDir();

    static std::string bitmapFill();

    static double GapISO();
    static double GapASME();

    static bool reportProgress();

    static bool lightOnDark();
    static void lightOnDark(bool state);
    static bool monochrome();
    static void monochrome(bool state);
    static Base::Color lightTextColor();
    static Base::Color lightenColor(Base::Color orig);
    static Base::Color getAccessibleColor(Base::Color orig);

    static bool autoCorrectDimRefs();
    static int scrubCount();

    static double svgHatchFactor();
    static bool SectionUsePreviousCut();

    static int lineStandard();
    static void setLineStandard(int index);
    static std::string lineDefinitionLocation();
    static std::string lineElementsLocation();

    static std::string lineGroupFile();
    static int lineGroup();
    static int SectionLineStyle();
    static int CenterLineStyle();
    static int HighlightLineStyle();
    static int HiddenLineStyle();
    static int BreakLineStyle();
    static int LineCapStyle();
    static int LineCapIndex();

    static float LineSpacingISO();

    static std::string currentLineDefFile();
    static std::string currentElementDefFile();

    static int sectionLineConvention();
    static bool showSectionLine();
    static bool includeCutLine();

    static DrawBrokenView::BreakType BreakType();

    static bool useExactMatchOnDims();

    static bool useCameraDirection();
    static bool alwaysShowLabel();

    static bool SnapViews();
    static double SnapLimitFactor();

    static Qt::KeyboardModifiers multiselectModifiers();

    static Qt::KeyboardModifiers balloonDragModifiers();
    static void setBalloonDragModifiers(Qt::KeyboardModifiers newModifiers);

    static bool enforceISODate();
    static bool switchOnClick();

    static bool checkShapesBeforeUse();
    static bool debugBadShape();

    static bool useLegacySvgScaling();

    static bool showUnits();

    static bool snapDetailHighlights();
    static double detailSnapRadius();

    static bool showCenterMarks();
    static bool printCenterMarks();

    static bool fixColorAlphaOnLoad();

};


}//end namespace TechDraw