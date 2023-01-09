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

# include <QString>
#endif

#include <App/Application.h>
#include <App/Material.h>
#include <Base/Console.h>
#include <Base/FileInfo.h>
#include <Base/Parameter.h>

#include "Preferences.h"


//getters for parameters used in multiple places.
//ensure this is in sync with preference page user interfaces

using namespace TechDraw;

const double Preferences::DefaultFontSizeInMM = 5.0;

std::string Preferences::labelFont()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication()
                                             .GetUserParameter()
                                             .GetGroup("BaseApp")
                                             ->GetGroup("Preferences")
                                             ->GetGroup("Mod/TechDraw/Labels");
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
    Base::Reference<ParameterGrp> hGrp = App::GetApplication()
                                             .GetUserParameter()
                                             .GetGroup("BaseApp")
                                             ->GetGroup("Preferences")
                                             ->GetGroup("Mod/TechDraw/Labels");
    return hGrp->GetFloat("LabelSize", DefaultFontSizeInMM);
}

double Preferences::dimFontSizeMM()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication()
                                             .GetUserParameter()
                                             .GetGroup("BaseApp")
                                             ->GetGroup("Preferences")
                                             ->GetGroup("Mod/TechDraw/Dimensions");
    return hGrp->GetFloat("FontSize", DefaultFontSizeInMM);
}

App::Color Preferences::normalColor()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication()
                                             .GetUserParameter()
                                             .GetGroup("BaseApp")
                                             ->GetGroup("Preferences")
                                             ->GetGroup("Mod/TechDraw/Colors");
    App::Color fcColor;
    fcColor.setPackedValue(hGrp->GetUnsigned("NormalColor", 0x000000FF));//#000000 black
    return fcColor;
}

App::Color Preferences::selectColor()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication()
                                             .GetUserParameter()
                                             .GetGroup("BaseApp")
                                             ->GetGroup("Preferences")
                                             ->GetGroup("View");
    unsigned int defColor = hGrp->GetUnsigned("SelectionColor", 0x00FF00FF);//#00FF00 lime

    hGrp = App::GetApplication()
               .GetUserParameter()
               .GetGroup("BaseApp")
               ->GetGroup("Preferences")
               ->GetGroup("Mod/TechDraw/Colors");
    App::Color fcColor;
    fcColor.setPackedValue(hGrp->GetUnsigned("SelectColor", defColor));
    return fcColor;
}

App::Color Preferences::preselectColor()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication()
                                             .GetUserParameter()
                                             .GetGroup("BaseApp")
                                             ->GetGroup("Preferences")
                                             ->GetGroup("View");
    unsigned int defColor = hGrp->GetUnsigned("HighlightColor", 0xFFFF00FF);//#FFFF00 yellow

    hGrp = App::GetApplication()
               .GetUserParameter()
               .GetGroup("BaseApp")
               ->GetGroup("Preferences")
               ->GetGroup("Mod/TechDraw/Colors");
    App::Color fcColor;
    fcColor.setPackedValue(hGrp->GetUnsigned("PreSelectColor", defColor));
    return fcColor;
}

App::Color Preferences::vertexColor()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication()
                                             .GetUserParameter()
                                             .GetGroup("BaseApp")
                                             ->GetGroup("Preferences")
                                             ->GetGroup("Mod/TechDraw/Decorations");
    App::Color fcColor;
    fcColor.setPackedValue(hGrp->GetUnsigned("VertexColor", 0x000000FF));//#000000 black
    return fcColor;
}

double Preferences::vertexScale()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication()
                                             .GetUserParameter()
                                             .GetGroup("BaseApp")
                                             ->GetGroup("Preferences")
                                             ->GetGroup("Mod/TechDraw/General");
    double result = hGrp->GetFloat("VertexScale", 3.0);
    return result;
}

int Preferences::scaleType()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication()
                                             .GetUserParameter()
                                             .GetGroup("BaseApp")
                                             ->GetGroup("Preferences")
                                             ->GetGroup("Mod/TechDraw/General");
    int result = hGrp->GetInt("DefaultScaleType", 0);
    return result;
}

double Preferences::scale()
{
    int prefScaleType = scaleType();
    if (prefScaleType == 0) {//page scale
        Base::Reference<ParameterGrp> hGrp = App::GetApplication()
                                                 .GetUserParameter()
                                                 .GetGroup("BaseApp")
                                                 ->GetGroup("Preferences")
                                                 ->GetGroup("Mod/TechDraw/General");
        return hGrp->GetFloat("DefaultPageScale", 1.0);
    }
    else if (prefScaleType == 1) {//custom scale
        Base::Reference<ParameterGrp> hGrp = App::GetApplication()
                                                 .GetUserParameter()
                                                 .GetGroup("BaseApp")
                                                 ->GetGroup("Preferences")
                                                 ->GetGroup("Mod/TechDraw/General");
        return hGrp->GetFloat("DefaultViewScale", 1.0);
    }
    return 1.0;
}

bool Preferences::keepPagesUpToDate()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication()
                                             .GetUserParameter()
                                             .GetGroup("BaseApp")
                                             ->GetGroup("Preferences")
                                             ->GetGroup("Mod/TechDraw/General");
    bool autoUpdate = hGrp->GetBool("KeepPagesUpToDate", true);
    return autoUpdate;
}

bool Preferences::useGlobalDecimals()
{
    bool result = false;
    Base::Reference<ParameterGrp> hGrp = App::GetApplication()
                                             .GetUserParameter()
                                             .GetGroup("BaseApp")
                                             ->GetGroup("Preferences")
                                             ->GetGroup("Mod/TechDraw/Dimensions");
    result = hGrp->GetBool("UseGlobalDecimals", true);
    return result;
}

int Preferences::projectionAngle()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication()
                                             .GetUserParameter()
                                             .GetGroup("BaseApp")
                                             ->GetGroup("Preferences")
                                             ->GetGroup("Mod/TechDraw/General");
    int projType = hGrp->GetInt("ProjectionAngle", 0);//First Angle
    return projType;
}

int Preferences::lineGroup()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication()
                                             .GetUserParameter()
                                             .GetGroup("BaseApp")
                                             ->GetGroup("Preferences")
                                             ->GetGroup("Mod/TechDraw/Decorations");
    int lgInt = hGrp->GetInt("LineGroup", 3);// FC 0.70mm
    return lgInt;
}

int Preferences::balloonArrow()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication()
                                             .GetUserParameter()
                                             .GetGroup("BaseApp")
                                             ->GetGroup("Preferences")
                                             ->GetGroup("Mod/TechDraw/Decorations");
    int end = hGrp->GetInt("BalloonArrow", 0);
    return end;
}

QString Preferences::defaultTemplate()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication()
                                             .GetUserParameter()
                                             .GetGroup("BaseApp")
                                             ->GetGroup("Preferences")
                                             ->GetGroup("Mod/TechDraw/Files");
    std::string defaultDir = App::Application::getResourceDir() + "Mod/TechDraw/Templates/";
    std::string defaultFileName = defaultDir + "A4_LandscapeTD.svg";
    std::string prefFileName = hGrp->GetASCII("TemplateFile", defaultFileName.c_str());
    if (prefFileName.empty()) {
        prefFileName = defaultFileName;
    }
    QString templateFileName = QString::fromStdString(prefFileName);
    Base::FileInfo fi(prefFileName);
    if (!fi.isReadable()) {
        Base::Console().Warning("Template File: %s is not readable\n", prefFileName.c_str());
        templateFileName = QString::fromStdString(defaultFileName);
    }
    return templateFileName;
}

QString Preferences::defaultTemplateDir()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication()
                                             .GetUserParameter()
                                             .GetGroup("BaseApp")
                                             ->GetGroup("Preferences")
                                             ->GetGroup("Mod/TechDraw/Files");

    std::string defaultDir = App::Application::getResourceDir() + "Mod/TechDraw/Templates";
    std::string prefTemplateDir = hGrp->GetASCII("TemplateDir", defaultDir.c_str());
    if (prefTemplateDir.empty()) {
        prefTemplateDir = defaultDir;
    }
    QString templateDir = QString::fromStdString(prefTemplateDir);
    Base::FileInfo fi(prefTemplateDir);
    if (!fi.isReadable()) {
        Base::Console().Warning("Template Directory: %s is not readable\n",
                                prefTemplateDir.c_str());
        templateDir = QString::fromStdString(defaultDir);
    }
    return templateDir;
}

std::string Preferences::lineGroupFile()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication()
                                             .GetUserParameter()
                                             .GetGroup("BaseApp")
                                             ->GetGroup("Preferences")
                                             ->GetGroup("Mod/TechDraw/Files");
    std::string defaultDir = App::Application::getResourceDir() + "Mod/TechDraw/LineGroup/";
    std::string defaultFileName = defaultDir + "LineGroup.csv";
    std::string lgFileName = hGrp->GetASCII("LineGroupFile", defaultFileName.c_str());
    if (lgFileName.empty()) {
        lgFileName = defaultFileName;
    }
    Base::FileInfo fi(lgFileName);
    if (!fi.isReadable()) {
        Base::Console().Warning("Line Group File: %s is not readable\n", lgFileName.c_str());
        lgFileName = defaultFileName;
    }
    return lgFileName;
}

std::string Preferences::formatSpec()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication()
                                             .GetUserParameter()
                                             .GetGroup("BaseApp")
                                             ->GetGroup("Preferences")
                                             ->GetGroup("Mod/TechDraw/Dimensions");
    return hGrp->GetASCII("formatSpec", "%.2w");
}

int Preferences::altDecimals()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication()
                                             .GetUserParameter()
                                             .GetGroup("BaseApp")
                                             ->GetGroup("Preferences")
                                             ->GetGroup("Mod/TechDraw/Dimensions");
    return hGrp->GetInt("AltDecimals", 2);
}

int Preferences::mattingStyle()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication()
                                             .GetUserParameter()
                                             .GetGroup("BaseApp")
                                             ->GetGroup("Preferences")
                                             ->GetGroup("Mod/TechDraw/Decorations");
    int style = hGrp->GetInt("MattingStyle", 0);
    return style;
}

std::string Preferences::svgFile()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication()
                                             .GetUserParameter()
                                             .GetGroup("BaseApp")
                                             ->GetGroup("Preferences")
                                             ->GetGroup("Mod/TechDraw/Files");

    std::string defaultDir = App::Application::getResourceDir() + "Mod/TechDraw/Patterns/";
    std::string defaultFileName = defaultDir + "simple.svg";
    std::string prefHatchFile = hGrp->GetASCII("FileHatch", defaultFileName.c_str());
    if (prefHatchFile.empty()) {
        prefHatchFile = defaultFileName;
    }
    Base::FileInfo fi(prefHatchFile);
    if (!fi.isReadable()) {
        Base::Console().Warning("Svg Hatch File: %s is not readable\n", prefHatchFile.c_str());
        prefHatchFile = defaultFileName;
    }
    return prefHatchFile;
}

std::string Preferences::patFile()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication()
                                             .GetUserParameter()
                                             .GetGroup("BaseApp")
                                             ->GetGroup("Preferences")
                                             ->GetGroup("Mod/TechDraw/PAT");

    std::string defaultDir = App::Application::getResourceDir() + "Mod/TechDraw/PAT/";
    std::string defaultFileName = defaultDir + "FCPAT.pat";
    std::string prefHatchFile = hGrp->GetASCII("FilePattern", defaultFileName.c_str());
    if (prefHatchFile.empty()) {
        prefHatchFile = defaultFileName;
    }
    Base::FileInfo fi(prefHatchFile);
    if (!fi.isReadable()) {
        Base::Console().Warning("Pat Hatch File: %s is not readable\n", prefHatchFile.c_str());
        prefHatchFile = defaultFileName;
    }

    return prefHatchFile;
}

std::string Preferences::bitmapFill()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication()
                                             .GetUserParameter()
                                             .GetGroup("BaseApp")
                                             ->GetGroup("Preferences")
                                             ->GetGroup("Mod/TechDraw/Files");

    std::string defaultDir = App::Application::getResourceDir() + "Mod/TechDraw/Patterns/";
    std::string defaultFileName = defaultDir + "default.png";
    std::string prefBitmapFile = hGrp->GetASCII("BitmapFill", defaultFileName.c_str());
    if (prefBitmapFile.empty()) {
        prefBitmapFile = defaultFileName;
    }
    Base::FileInfo fi(prefBitmapFile);
    if (!fi.isReadable()) {
        Base::Console().Warning("Bitmap Fill File: %s is not readable\n", prefBitmapFile.c_str());
        prefBitmapFile = defaultFileName;
    }
    return prefBitmapFile;
}

double Preferences::GapISO()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication()
                                             .GetUserParameter()
                                             .GetGroup("BaseApp")
                                             ->GetGroup("Preferences")
                                             ->GetGroup("Mod/TechDraw/Dimensions");
    double factor = hGrp->GetFloat("GapISO", 8.0);
    return factor;
}

double Preferences::GapASME()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication()
                                             .GetUserParameter()
                                             .GetGroup("BaseApp")
                                             ->GetGroup("Preferences")
                                             ->GetGroup("Mod/TechDraw/Dimensions");
    double factor = hGrp->GetFloat("GapASME", 6.0);
    return factor;
}

bool Preferences::reportProgress()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication()
                                             .GetUserParameter()
                                             .GetGroup("BaseApp")
                                             ->GetGroup("Preferences")
                                             ->GetGroup("Mod/TechDraw/General");
    bool report = hGrp->GetBool("ReportProgress", false);
    return report;
}

bool Preferences::lightOnDark()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication()
                                             .GetUserParameter()
                                             .GetGroup("BaseApp")
                                             ->GetGroup("Preferences")
                                             ->GetGroup("Mod/TechDraw/Colors");
    bool light = hGrp->GetBool("LightOnDark", false);
    return light;
}

void Preferences::lightOnDark(bool state)
{
    Base::Console().Message("Pref::useLightText - set to %d\n", state);
    Base::Reference<ParameterGrp> hGrp = App::GetApplication()
                                             .GetUserParameter()
                                             .GetGroup("BaseApp")
                                             ->GetGroup("Preferences")
                                             ->GetGroup("Mod/TechDraw/Colors");
    hGrp->SetBool("LightOnDark", state);
}

bool Preferences::monochrome()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication()
                                             .GetUserParameter()
                                             .GetGroup("BaseApp")
                                             ->GetGroup("Preferences")
                                             ->GetGroup("Mod/TechDraw/Colors");
    bool mono = hGrp->GetBool("Monochrome", false);
    return mono;
}

void Preferences::monochrome(bool state)
{
    Base::Console().Message("Pref::useLightText - set to %d\n", state);
    Base::Reference<ParameterGrp> hGrp = App::GetApplication()
                                             .GetUserParameter()
                                             .GetGroup("BaseApp")
                                             ->GetGroup("Preferences")
                                             ->GetGroup("Mod/TechDraw/Colors");
    hGrp->SetBool("Monochrome", state);
}

App::Color Preferences::lightTextColor()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication()
                                             .GetUserParameter()
                                             .GetGroup("BaseApp")
                                             ->GetGroup("Preferences")
                                             ->GetGroup("Mod/TechDraw/Colors");
    App::Color result;
    result.setPackedValue(hGrp->GetUnsigned("LightTextColor", 0xFFFFFFFF));//#FFFFFFFF white
    return result;
}

App::Color Preferences::lightenColor(App::Color orig)
{
    // get component colours on [0, 255]
    uchar red = orig.r * 255;
    uchar blue = orig.b * 255;
    uchar green = orig.g * 255;
    //    uchar alpha = orig.a * 255;

    // shift color values
    uchar m = std::min({red, blue, green});
    red -= m;
    blue -= m;
    green -= m;

    // calculate chroma (colour range)
    uchar chroma = std::max({red, blue, green});

    // calculate lightened colour value
    uchar newm = 255 - chroma - m;
    red += newm;
    green += newm;
    blue += newm;

    double redF = (float)red / 255.0;
    double greenF = (float)green / 255.0;
    double blueF = (float)blue / 255.0;

    return App::Color(redF, greenF, blueF, orig.a);
}

App::Color Preferences::getAccessibleColor(App::Color orig)
{
    if (Preferences::lightOnDark() && Preferences::monochrome()) {
        return lightTextColor();
    }
    if (Preferences::lightOnDark()) {
        return lightenColor(orig);
    }
    return orig;
}
