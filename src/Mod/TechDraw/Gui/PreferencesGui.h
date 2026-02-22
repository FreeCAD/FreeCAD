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

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <QColor>

#include "QGIView.h"

class QColor;
class QString;

namespace Base
{
class Color;
}

class QFont;
class QString;

#include <Mod/TechDraw/App/Preferences.h>

namespace TechDraw{
enum class ArrowType : int;
}

namespace TechDrawGui
{

//getters for parameters used in multiple places.
class TechDrawGuiExport PreferencesGui {

public:
static QFont       labelFontQFont();
static int         labelFontSizePX();
static int         dimFontSizePX();

static QColor      normalQColor();
static QColor      selectQColor();
static QColor      preselectQColor();
static Base::Color sectionLineColor();
static QColor      sectionLineQColor();
static Base::Color centerColor();
static QColor      centerQColor();
static QColor      vertexQColor();
static Base::Color leaderColor();
static QColor      leaderQColor();
static Base::Color dimColor();
static QColor      dimQColor();
static Base::Color pageColor();
static QColor      pageQColor();
static Base::Color breaklineColor();
static QColor      breaklineQColor();

static TechDraw::ArrowType dimArrowStyle();
static double      dimArrowSize();

static double      edgeFuzz();

static QString     weldingDirectory();

static bool showGrid();
static Base::Color gridColor();
static QColor gridQColor();
static double gridSpacing();
static bool multiSelection();

static QColor       getAccessibleQColor(QColor orig);
static QColor       lightTextQColor();
static QColor       reverseColor(QColor orig);
static QColor       lightenColor(QColor orig);

static double       templateClickBoxSize();
static QColor       templateClickBoxColor();

static int          get3dMarkerSize();

static ViewFrameMode getViewFrameMode();
static void setViewFrameMode(ViewFrameMode newMode);


};

} //end namespace TechDrawGui