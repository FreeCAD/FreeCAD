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
//#include <QFont>
//#include <QColor>
#endif

#include <App/Application.h>
#include <App/Material.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Parameter.h>
#include <Base/Vector3D.h>

#include "Preferences.h"

//getters for parameters used in multiple places.
//ensure this is in sync with preference page uis

using namespace TechDraw;

const double Preferences::DefaultFontSizeInMM = 5.0;

std::string Preferences::labelFont()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().
                                         GetGroup("BaseApp")->GetGroup("Preferences")->
                                         GetGroup("Mod/TechDraw/Labels");
    std::string fontName = hGrp->GetASCII("LabelFont", "osifont");
    return fontName;
}

QString Preferences::labelFontQString()
{
    std::string fontName = labelFont();
    return QString::fromStdString(fontName);
}

double Preferences::labelFontSizeMM()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().
                                         GetGroup("BaseApp")->GetGroup("Preferences")->
                                         GetGroup("Mod/TechDraw/Labels");
    return hGrp->GetFloat("LabelSize", DefaultFontSizeInMM);
}

double Preferences::dimFontSizeMM()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().
                                         GetGroup("BaseApp")->GetGroup("Preferences")->
                                         GetGroup("Mod/TechDraw/Dimensions");
    return hGrp->GetFloat("FontSize", DefaultFontSizeInMM);
}

App::Color Preferences::normalColor()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().
           GetGroup("BaseApp")->GetGroup("Preferences")->
           GetGroup("Mod/TechDraw/Colors");
    App::Color fcColor;
    fcColor.setPackedValue(hGrp->GetUnsigned("NormalColor", 0x000000FF));     //#000000 black
    return fcColor;
}

App::Color Preferences::selectColor()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().
                                         GetGroup("BaseApp")->GetGroup("Preferences")->
                                         GetGroup("View");
    unsigned int defColor = hGrp->GetUnsigned("SelectionColor", 0x00FF00FF);  //#00FF00 lime

    hGrp = App::GetApplication().GetUserParameter().
           GetGroup("BaseApp")->GetGroup("Preferences")->
           GetGroup("Mod/TechDraw/Colors");
    App::Color fcColor;
    fcColor.setPackedValue(hGrp->GetUnsigned("SelectColor", defColor));
    return fcColor;
}

App::Color Preferences::preselectColor()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().
                                         GetGroup("BaseApp")->GetGroup("Preferences")->
                                         GetGroup("View");
    unsigned int defColor = hGrp->GetUnsigned("HighlightColor", 0xFFFF00FF);  //#FFFF00 yellow

    hGrp = App::GetApplication().GetUserParameter().
           GetGroup("BaseApp")->GetGroup("Preferences")->
           GetGroup("Mod/TechDraw/Colors");
    App::Color fcColor;
    fcColor.setPackedValue(hGrp->GetUnsigned("PreSelectColor", defColor));
    return fcColor;
}

App::Color Preferences::vertexColor()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().
           GetGroup("BaseApp")->GetGroup("Preferences")->
           GetGroup("Mod/TechDraw/Decorations");
    App::Color fcColor;
    fcColor.setPackedValue(hGrp->GetUnsigned("VertexColor", 0x000000FF));     //#000000 black
    return fcColor;
}

double Preferences::vertexScale()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->
                                         GetGroup("Preferences")->GetGroup("Mod/TechDraw/General");
    double result = hGrp->GetFloat("VertexScale", 3.0);
    return result;
}



//lightgray #D3D3D3 

bool Preferences::keepPagesUpToDate()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().
                                         GetGroup("BaseApp")->GetGroup("Preferences")->
                                         GetGroup("Mod/TechDraw/General");
    bool autoUpdate = hGrp->GetBool("KeepPagesUpToDate", true);
    return autoUpdate;
}

bool Preferences::useGlobalDecimals()
{
    bool result = false;
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().
                                         GetUserParameter().GetGroup("BaseApp")->
                                         GetGroup("Preferences")->GetGroup("Mod/TechDraw/Dimensions");
    result = hGrp->GetBool("UseGlobalDecimals", true);
    return result;
}

int Preferences::projectionAngle()
{
    Base::Reference<ParameterGrp>  hGrp = App::GetApplication().GetUserParameter().
                                          GetGroup("BaseApp")->GetGroup("Preferences")->
                                          GetGroup("Mod/TechDraw/General");
    int projType = hGrp->GetInt("ProjectionAngle", 0);    //First Angle
    return projType;
}

int Preferences::lineGroup()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().
                                         GetGroup("BaseApp")->GetGroup("Preferences")->
                                         GetGroup("Mod/TechDraw/Decorations");
    int lgInt = hGrp->GetInt("LineGroup", 3); // FC 0.70mm
    return lgInt;
}

int Preferences::balloonArrow()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().
                                         GetGroup("BaseApp")->GetGroup("Preferences")->
                                         GetGroup("Mod/TechDraw/Decorations");
    int end = hGrp->GetInt("BalloonArrow", 0);
    return end;
}

QString Preferences::defaultTemplate()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().
                                         GetGroup("BaseApp")->GetGroup("Preferences")->
                                         GetGroup("Mod/TechDraw/Files");
    std::string defaultDir = App::Application::getResourceDir() + "Mod/TechDraw/Templates/";
    std::string defaultFileName = defaultDir + "A4_LandscapeTD.svg";
    std::string prefFileName = hGrp->GetASCII("TemplateFile",defaultFileName.c_str());
    QString templateFileName = QString::fromStdString(prefFileName);
    Base::FileInfo fi(prefFileName);
    if (!fi.isReadable()) {
        templateFileName = QString::fromStdString(defaultFileName);
        Base::Console().Warning("Template File: %s is not readable\n", prefFileName.c_str());
    }
    return templateFileName;
}

QString Preferences::defaultTemplateDir()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Files");

    std::string defaultDir = App::Application::getResourceDir() + "Mod/TechDraw/Templates";
    std::string prefTemplateDir = hGrp->GetASCII("TemplateDir", defaultDir.c_str());
    QString templateDir = QString::fromStdString(prefTemplateDir);
    Base::FileInfo fi(prefTemplateDir);
    if (!fi.isReadable()) {
        templateDir = QString::fromStdString(defaultDir);
        Base::Console().Warning("Template Directory: %s is not readable\n", prefTemplateDir.c_str());
   }
    return templateDir;
}

std::string Preferences::lineGroupFile()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().
                                         GetUserParameter().GetGroup("BaseApp")->
                                         GetGroup("Preferences")->GetGroup("Mod/TechDraw/Files");
    std::string defaultDir = App::Application::getResourceDir() + "Mod/TechDraw/LineGroup/";
    std::string defaultFileName = defaultDir + "LineGroup.csv";
    std::string lgFileName = hGrp->GetASCII("LineGroupFile",defaultFileName.c_str());
    Base::FileInfo fi(lgFileName);
    if (!fi.isReadable()) {
        lgFileName = defaultFileName;
        Base::Console().Warning("Line Group File: %s is not readable\n", lgFileName.c_str());
    }
    return lgFileName;
}
