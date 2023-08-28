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

#ifndef PreferencesGui_h_
#define PreferencesGui_h_

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <QColor>

class QFont;
class QString;

#include <Mod/TechDraw/App/Preferences.h>

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
static App::Color  sectionLineColor();
static QColor      sectionLineQColor();
static App::Color  centerColor();
static QColor      centerQColor();
static QColor      vertexQColor();
static App::Color  leaderColor();
static QColor      leaderQColor();
static App::Color  dimColor();
static QColor      dimQColor();
static App::Color  pageColor();
static QColor      pageQColor();

static int         dimArrowStyle();
static double      dimArrowSize();

static double      edgeFuzz();

static Qt::PenStyle  sectionLineStyle();
static bool          sectionLineMarks();

static QString     weldingDirectory();

static bool showGrid();
static App::Color gridColor();
static QColor gridQColor();
static double gridSpacing();

static QColor       getAccessibleQColor(QColor orig);
static QColor       lightTextQColor();
static QColor       reverseColor(QColor orig);
static QColor       lightenColor(QColor orig);
};

} //end namespace TechDrawGui
#endif
