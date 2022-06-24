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
#include <string>
#include <QString>
#include <QFont>
#include <QColor>
#include <QPen>
#endif

#include <App/Application.h>
#include <App/Material.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Parameter.h>
#include <Base/Vector3D.h>

#include "Rez.h"
#include "PreferencesGui.h"

//getters for parameters used in multiple places.
//ensure this is in sync with preference page user interfaces

using namespace TechDrawGui;
using namespace TechDraw;

QFont PreferencesGui::labelFontQFont()
{
    QString name = Preferences::labelFontQString();
    QFont f(name);
    return f;
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
    App::Color fcColor = Preferences::normalColor();
    return fcColor.asValue<QColor>();
}

QColor PreferencesGui::selectQColor()
{
    App::Color fcColor = Preferences::selectColor();
    return fcColor.asValue<QColor>();
}

QColor PreferencesGui::preselectQColor()
{
    App::Color fcColor = Preferences::preselectColor();
    return fcColor.asValue<QColor>();
}

App::Color PreferencesGui::sectionLineColor()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Decorations");
    App::Color fcColor;
    fcColor.setPackedValue(hGrp->GetUnsigned("SectionColor", 0x000000FF));
    return fcColor;
}

QColor PreferencesGui::sectionLineQColor()
{
    return sectionLineColor().asValue<QColor>();
}

App::Color PreferencesGui::centerColor()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().
                                         GetGroup("BaseApp")->GetGroup("Preferences")->
                                         GetGroup("Mod/TechDraw/Decorations");
    App::Color fcColor = App::Color((uint32_t) hGrp->GetUnsigned("CenterColor", 0x000000FF));
    return fcColor;
}

QColor PreferencesGui::centerQColor()
{
    return centerColor().asValue<QColor>();
}

QColor PreferencesGui::vertexQColor()
{
    return Preferences::vertexColor().asValue<QColor>();
}

App::Color PreferencesGui::dimColor()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().
                                         GetGroup("BaseApp")->GetGroup("Preferences")->
                                         GetGroup("Mod/TechDraw/Dimensions");
    App::Color result;
    result.setPackedValue(hGrp->GetUnsigned("Color", 0x000000FF));  //#000000 black
    return result;
}

QColor PreferencesGui::dimQColor()
{
    return PreferencesGui::dimColor().asValue<QColor>();
}


App::Color PreferencesGui::leaderColor()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().
                                         GetGroup("BaseApp")->GetGroup("Preferences")->
                                         GetGroup("Mod/TechDraw/LeaderLine");
    App::Color result;
    result.setPackedValue(hGrp->GetUnsigned("Color", 0x000000FF));  //#000000 black
    return result;
}

QColor PreferencesGui::leaderQColor()
{
    return PreferencesGui::leaderColor().asValue<QColor>();
}

int PreferencesGui::dimArrowStyle()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().
                                         GetGroup("BaseApp")->GetGroup("Preferences")->
                                         GetGroup("Mod/TechDraw/Dimensions");
    int style = hGrp->GetInt("ArrowStyle", 0);
    return style;
}

double PreferencesGui::dimArrowSize()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().
                                         GetGroup("BaseApp")->GetGroup("Preferences")->
                                         GetGroup("Mod/TechDraw/Dimensions");
    double size = hGrp->GetFloat("ArrowSize", Preferences::dimFontSizeMM());
    return size;
}


double PreferencesGui::edgeFuzz()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().
                                         GetGroup("BaseApp")->GetGroup("Preferences")->
                                         GetGroup("Mod/TechDraw/General");
    double result = hGrp->GetFloat("EdgeFuzz",10.0);
    return result;
}

Qt::PenStyle PreferencesGui::sectionLineStyle()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().
                                         GetGroup("BaseApp")->GetGroup("Preferences")->
                                         GetGroup("Mod/TechDraw/Decorations");
    Qt::PenStyle sectStyle = static_cast<Qt::PenStyle> (hGrp->GetInt("SectionLine", 2));
    return sectStyle;
}


QString PreferencesGui::weldingDirectory()
{
    std::string defaultDir = App::Application::getResourceDir() + "Mod/TechDraw/Symbols/Welding/AWS/";
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->
                                         GetGroup("Preferences")->GetGroup("Mod/TechDraw/Files");
                                    
    std::string symbolDir = hGrp->GetASCII("WeldingDir", defaultDir.c_str());
    if (symbolDir.empty()) {
        symbolDir = defaultDir;
    }
    QString qSymbolDir = QString::fromUtf8(symbolDir.c_str());
    Base::FileInfo fi(symbolDir);
    if (!fi.isReadable()) {
        Base::Console().Warning("Welding Directory: %s is not readable\n", symbolDir.c_str());
        qSymbolDir = QString::fromUtf8(defaultDir.c_str());
    }
    return qSymbolDir;
}


App::Color PreferencesGui::gridColor()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().
                                         GetGroup("BaseApp")->GetGroup("Preferences")->
                                         GetGroup("Mod/TechDraw/Colors");
    App::Color result;
    result.setPackedValue(hGrp->GetUnsigned("gridColor", 0x000000FF));  //#000000 black
    return result;
}

QColor PreferencesGui::gridQColor()
{
    return PreferencesGui::gridColor().asValue<QColor>();
}

double PreferencesGui::gridSpacing()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().
                                         GetGroup("BaseApp")->GetGroup("Preferences")->
                                         GetGroup("Mod/TechDraw/General");
    double spacing = hGrp->GetFloat("gridSpacing", 10.0);
    return spacing;
}

bool PreferencesGui::showGrid()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().
                                         GetGroup("BaseApp")->GetGroup("Preferences")->
                                         GetGroup("Mod/TechDraw/General");
    bool show = hGrp->GetBool("showGrid", false);
    return show;
}
