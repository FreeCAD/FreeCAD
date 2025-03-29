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

#include "PreCompiled.h"

#ifndef _PreComp_
# include <string>
# include <QColor>
# include <QFont>
# include <QString>
#endif

#include <App/Application.h>
#include <App/Material.h>
#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Gui/Selection/Selection.h>
#include <Mod/TechDraw/App/ArrowPropEnum.h>
#include <Mod/TechDraw/App/Preferences.h>
#include <Mod/TechDraw/App/LineGenerator.h>


#include "PreferencesGui.h"
#include "Rez.h"


//getters for parameters used in multiple places.
//ensure this is in sync with preference page user interfaces

using namespace TechDrawGui;
using namespace TechDraw;

QFont PreferencesGui::labelFontQFont()
{
    QString name = Preferences::labelFontQString();
    return QFont(name);
}

int PreferencesGui::labelFontSizePX()
{
    return (int) (Rez::guiX(Preferences::labelFontSizeMM()) + 0.5);
}

int PreferencesGui::dimFontSizePX()
{
    return (int) (Rez::guiX(Preferences::dimFontSizeMM()) + 0.5);
}

QColor PreferencesGui::normalQColor()
{
    Base::Color fcColor = Preferences::normalColor();
    return fcColor.asValue<QColor>();
}

QColor PreferencesGui::selectQColor()
{
    Base::Color fcColor = Preferences::selectColor();
    return fcColor.asValue<QColor>();
}

QColor PreferencesGui::preselectQColor()
{
    Base::Color fcColor = Preferences::preselectColor();
    return fcColor.asValue<QColor>();
}

Base::Color PreferencesGui::sectionLineColor()
{
    Base::Color fcColor;
    fcColor.setPackedValue(Preferences::getPreferenceGroup("Decorations")->GetUnsigned("SectionColor", 0x000000FF));
    return fcColor;
}

QColor PreferencesGui::sectionLineQColor()
{
//if the Base::Color version has already lightened the color, we don't want to do it again
    Base::Color fcColor;
    fcColor.setPackedValue(Preferences::getPreferenceGroup("Decorations")->GetUnsigned("SectionColor", 0x000000FF));
    return fcColor.asValue<QColor>();
}

Base::Color PreferencesGui::breaklineColor()
{
    Base::Color fcColor;
    fcColor.setPackedValue(Preferences::getPreferenceGroup("Decorations")->GetUnsigned("BreaklineColor", 0x000000FF));
    return fcColor;
}

QColor PreferencesGui::breaklineQColor()
{
//if the Base::Color version has already lightened the color, we don't want to do it again
    Base::Color fcColor;
    fcColor.setPackedValue(Preferences::getPreferenceGroup("Decorations")->GetUnsigned("BreaklineColor", 0x000000FF));
    return fcColor.asValue<QColor>();
}

Base::Color PreferencesGui::centerColor()
{
    return Base::Color((uint32_t) Preferences::getPreferenceGroup("Decorations")->GetUnsigned("CenterColor", 0x000000FF));
}

QColor PreferencesGui::centerQColor()
{
    Base::Color fcColor = Base::Color((uint32_t) Preferences::getPreferenceGroup("Decorations")->GetUnsigned("CenterColor", 0x000000FF));
    return fcColor.asValue<QColor>();
}

QColor PreferencesGui::vertexQColor()
{
    return Preferences::vertexColor().asValue<QColor>();
}

Base::Color PreferencesGui::dimColor()
{
    Base::Color fcColor;
    fcColor.setPackedValue(Preferences::getPreferenceGroup("Dimensions")->GetUnsigned("Color", 0x000000FF));  //#000000 black
    return fcColor;
}

QColor PreferencesGui::dimQColor()
{
    Base::Color fcColor;
    fcColor.setPackedValue(Preferences::getPreferenceGroup("Dimensions")->GetUnsigned("Color", 0x000000FF));  //#000000 black
    return fcColor.asValue<QColor>();
}

Base::Color PreferencesGui::leaderColor()
{
    Base::Color fcColor;
    fcColor.setPackedValue(Preferences::getPreferenceGroup("LeaderLine")->GetUnsigned("Color", 0x000000FF));  //#000000 black
    return fcColor;
}

QColor PreferencesGui::leaderQColor()
{
    Base::Color fcColor;
    fcColor.setPackedValue(Preferences::getPreferenceGroup("LeaderLine")->GetUnsigned("Color", 0x000000FF));  //#000000 black
    return fcColor.asValue<QColor>();
}

ArrowType PreferencesGui::dimArrowStyle()
{
    int temp = Preferences::getPreferenceGroup("Dimensions")->GetInt("ArrowStyle", 0);
    return static_cast<ArrowType>(temp);
}

double PreferencesGui::dimArrowSize()
{
    return Preferences::getPreferenceGroup("Dimensions")->GetFloat("ArrowSize", Preferences::dimArrowSize());
}


double PreferencesGui::edgeFuzz()
{
    return Preferences::getPreferenceGroup("General")->GetFloat("EdgeFuzz", 10.0);
}

QString PreferencesGui::weldingDirectory()
{
    std::string defaultDir = App::Application::getResourceDir() + "Mod/TechDraw/Symbols/Welding/AWS/";

    std::string symbolDir = Preferences::getPreferenceGroup("Files")->GetASCII("WeldingDir", defaultDir.c_str());
    if (symbolDir.empty()) {
        symbolDir = defaultDir;
    }
    QString qSymbolDir = QString::fromUtf8(symbolDir.c_str());
    Base::FileInfo fi(symbolDir);
    if (!fi.isReadable()) {
        Base::Console().warning("Welding Directory: %s is not readable\n", symbolDir.c_str());
        qSymbolDir = QString::fromUtf8(defaultDir.c_str());
    }
    return qSymbolDir;
}

Base::Color PreferencesGui::gridColor()
{
    Base::Color fcColor;
    fcColor.setPackedValue(Preferences::getPreferenceGroup("Colors")->GetUnsigned("gridColor", 0x000000FF));  //#000000 black
    return fcColor;
}

QColor PreferencesGui::gridQColor()
{
    Base::Color fcColor;
    fcColor.setPackedValue(Preferences::getPreferenceGroup("Colors")->GetUnsigned("gridColor", 0x000000FF));  //#000000 black
    return fcColor.asValue<QColor>();
}

double PreferencesGui::gridSpacing()
{
    return Preferences::getPreferenceGroup("General")->GetFloat("gridSpacing", 10.0);
}

bool PreferencesGui::showGrid()
{
    return Preferences::getPreferenceGroup("General")->GetBool("showGrid", false);
}

bool PreferencesGui::multiSelection()
{
    bool greedy = Gui::Selection().getSelectionStyle() == Gui::SelectionSingleton::SelectionStyle::GreedySelection;
    return greedy || Preferences::getPreferenceGroup("General")->GetBool("multiSelection", false);
}

Base::Color PreferencesGui::pageColor()
{
    Base::Color result;
    result.setPackedValue(Preferences::getPreferenceGroup("Colors")->GetUnsigned("PageColor", 0xFFFFFFFF));  //#FFFFFFFF white
    return result;
}

QColor PreferencesGui::pageQColor()
{
    return PreferencesGui::pageColor().asValue<QColor>();
}

QColor PreferencesGui::getAccessibleQColor(QColor orig)
{
    if (Preferences::lightOnDark() && Preferences::monochrome()) {
        return lightTextQColor();
    }
    if (Preferences::lightOnDark()) {
        return lightenColor(orig);
    }
    return orig;
}

QColor PreferencesGui::lightTextQColor()
{
    return Preferences::lightTextColor().asValue<QColor>();
}

QColor PreferencesGui::reverseColor(QColor orig)
{
    int revRed = 255 - orig.red();
    int revBlue = 255 - orig.blue();
    int revGreen = 255 - orig.green();
    return QColor(revRed, revGreen, revBlue);
}

// largely based on code from https://invent.kde.org/graphics/okular and
// https://en.wikipedia.org/wiki/HSL_and_HSV#HSL_to_RGB
QColor PreferencesGui::lightenColor(QColor orig)
{
    // get component colours on [0, 255]
    uchar red = orig.red();
    uchar blue = orig.blue();
    uchar green = orig.green();
    uchar alpha = orig.alpha();

    // shift color values
    uchar m = std::min( {red, blue, green} );
    red -= m;
    blue -= m;
    green -= m;

    // calculate chroma (colour range)
    uchar chroma = std::max( {red, blue, green} );

    // calculate lightened colour value
    uchar newm = 255 - chroma - m;
    red += newm;
    green += newm;
    blue += newm;

    return QColor(red, green, blue, alpha);
}


double PreferencesGui::templateClickBoxSize()
{
    return Preferences::getPreferenceGroup("General")->GetFloat("TemplateDotSize", 5.0);
}


QColor PreferencesGui::templateClickBoxColor()
{
    Base::Color fcColor;
    fcColor.setPackedValue(Preferences::getPreferenceGroup("Colors")->GetUnsigned("TemplateUnderlineColor", 0x0000FFFF));  //#0000FF blue
    return fcColor.asValue<QColor>();
}

int PreferencesGui::get3dMarkerSize()
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath
                                ("User parameter:BaseApp/Preferences/View");
    return hGrp->GetInt("MarkerSize", 9L);
}

