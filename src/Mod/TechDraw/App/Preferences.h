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

#ifndef _Preferences_h_
#define _Preferences_h_

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <string>

class QString;

namespace App
{
class Color;
}

namespace TechDraw
{

//getters for parameters used in multiple places.
class TechDrawExport Preferences {

public:
static std::string labelFont();
static QString     labelFontQString();
static double      labelFontSizeMM();
static double      dimFontSizeMM();

static App::Color  normalColor();
static App::Color  selectColor();
static App::Color  preselectColor();
static App::Color  vertexColor();
static double      vertexScale();

static bool        useGlobalDecimals();
static bool        keepPagesUpToDate();

static int         projectionAngle();
static int         lineGroup();

static int         balloonArrow();

static QString     defaultTemplate();
static QString     defaultTemplateDir();
static std::string lineGroupFile();

static const double DefaultFontSizeInMM;

static std::string  formatSpec();
static int          altDecimals();

static int         mattingStyle();

static std::string svgFile();
static std::string patFile();

static std::string bitmapFill(void);

static double      ISOGap();
static double      ASMEGap();

};

} //end namespace TechDraw
#endif
