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
#include "LineGenerator.h"

//getters for parameters used in multiple places.
//ensure this is in sync with preference page user interfaces

using namespace TechDraw;

const double Preferences::DefaultFontSizeInMM = 5.0;
const double Preferences::DefaultArrowSize = 3.5;

//! Returns the TechDraw preference group
Base::Reference<ParameterGrp> Preferences::getPreferenceGroup(const char* Name)
{
    return App::GetApplication().GetUserParameter().GetGroup("BaseApp/Preferences/Mod/TechDraw")->GetGroup(Name);
}

std::string Preferences::labelFont()
{
    return getPreferenceGroup("Labels")->GetASCII("LabelFont", "osifont");
}

QString Preferences::labelFontQString()
{
    std::string fontName = labelFont();
    return QString::fromStdString(fontName);
}

double Preferences::labelFontSizeMM()
{
    return getPreferenceGroup("Labels")->GetFloat("LabelSize", DefaultFontSizeInMM);
}

double Preferences::dimFontSizeMM()
{
    return getPreferenceGroup("Dimensions")->GetFloat("FontSize", DefaultFontSizeInMM);
}

double Preferences::dimArrowSize()
{
    return getPreferenceGroup("Dimensions")->GetFloat("ArrowSize", DefaultArrowSize);
}

App::Color Preferences::normalColor()
{
    App::Color fcColor;
    fcColor.setPackedValue(getPreferenceGroup("Colors")->GetUnsigned("NormalColor", 0x000000FF));//#000000 black
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

    App::Color fcColor;
    fcColor.setPackedValue(getPreferenceGroup("Colors")->GetUnsigned("SelectColor", defColor));
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

    App::Color fcColor;
    fcColor.setPackedValue(getPreferenceGroup("Colors")->GetUnsigned("PreSelectColor", defColor));
    return fcColor;
}

App::Color Preferences::vertexColor()
{
    App::Color fcColor;
    fcColor.setPackedValue(getPreferenceGroup("Decorations")->GetUnsigned("VertexColor", 0x000000FF));//#000000 black
    return fcColor;
}

double Preferences::vertexScale()
{
    return getPreferenceGroup("General")->GetFloat("VertexScale", 3.0);
}

int Preferences::scaleType()
{
    return getPreferenceGroup("General")->GetInt("DefaultScaleType", 0);
}

double Preferences::scale()
{
    int prefScaleType = scaleType();
    if (prefScaleType == 0) {//page scale
        return getPreferenceGroup("General")->GetFloat("DefaultPageScale", 1.0);
    }
    else if (prefScaleType == 1) {//custom scale
        return getPreferenceGroup("General")->GetFloat("DefaultViewScale", 1.0);
    }
    return 1.0;
}

bool Preferences::keepPagesUpToDate()
{
    return getPreferenceGroup("General")->GetBool("KeepPagesUpToDate", true);  // Auto update
}

bool Preferences::useGlobalDecimals()
{
    return getPreferenceGroup("Dimensions")->GetBool("UseGlobalDecimals", true);
}

int Preferences::projectionAngle()
{
    return getPreferenceGroup("General")->GetInt("ProjectionAngle", 0);  //First Angle
}

int Preferences::lineGroup()
{
    return getPreferenceGroup("Decorations")->GetInt("LineGroup", 3);  // FC 0.70mm
}

int Preferences::balloonArrow()
{
    return getPreferenceGroup("Decorations")->GetInt("BalloonArrow", 0);
}

double Preferences::balloonKinkLength()
{
    return getPreferenceGroup("Dimensions")->GetFloat("BalloonKink", 5.0);
}

int Preferences::balloonShape()
{
    return getPreferenceGroup("Decorations")->GetInt("BalloonShape", 0);
}

QString Preferences::defaultTemplate()
{
    std::string defaultDir = App::Application::getResourceDir() + "Mod/TechDraw/Templates/";
    std::string defaultFileName = defaultDir + "A4_LandscapeTD.svg";
    std::string prefFileName = getPreferenceGroup("Files")->GetASCII("TemplateFile", defaultFileName.c_str());
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
    std::string defaultDir = App::Application::getResourceDir() + "Mod/TechDraw/Templates";
    std::string prefTemplateDir = getPreferenceGroup("Files")->GetASCII("TemplateDir", defaultDir.c_str());
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
    std::string defaultDir = App::Application::getResourceDir() + "Mod/TechDraw/LineGroup/";
    std::string defaultFileName = defaultDir + "LineGroup.csv";
    std::string lgFileName = getPreferenceGroup("Files")->GetASCII("LineGroupFile", defaultFileName.c_str());
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
    return getPreferenceGroup("Dimensions")->GetASCII("formatSpec", "%.2w");
}

int Preferences::altDecimals()
{
    return getPreferenceGroup("Dimensions")->GetInt("AltDecimals", 2);
}

int Preferences::mattingStyle()
{
    return getPreferenceGroup("Decorations")->GetInt("MattingStyle", 0);
}

bool Preferences::showDetailMatting()
{
    return getPreferenceGroup("General")->GetBool("ShowDetailMatting", true);
}

bool Preferences::showDetailHighlight()
{
    return getPreferenceGroup("General")->GetBool("ShowDetailHighlight", true);
}

std::string Preferences::svgFile()
{
    std::string defaultDir = App::Application::getResourceDir() + "Mod/TechDraw/Patterns/";
    std::string defaultFileName = defaultDir + "simple.svg";
    std::string prefHatchFile = getPreferenceGroup("Files")->GetASCII("FileHatch", defaultFileName.c_str());
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
    std::string defaultDir = App::Application::getResourceDir() + "Mod/TechDraw/PAT/";
    std::string defaultFileName = defaultDir + "FCPAT.pat";
    std::string prefHatchFile = getPreferenceGroup("PAT")->GetASCII("FilePattern", defaultFileName.c_str());
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
    std::string defaultDir = App::Application::getResourceDir() + "Mod/TechDraw/Patterns/";
    std::string defaultFileName = defaultDir + "default.png";
    std::string prefBitmapFile = getPreferenceGroup("Files")->GetASCII("BitmapFill", defaultFileName.c_str());
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

//! Returns the factor for calculating the ISO extension line gap, not the actual distance.
double Preferences::GapISO()
{
    double factor = getPreferenceGroup("Dimensions")->GetFloat("GapISO", 0.0);
    return factor;
}

//! Returns the factor for calculating the ASME extension line gap, not the actual distance.
double Preferences::GapASME()
{
    double factor = getPreferenceGroup("Dimensions")->GetFloat("GapASME", 0.0);
    return factor;
}

//! current setting for reporting progress of HLR/face finding
bool Preferences::reportProgress()
{
    return getPreferenceGroup("General")->GetBool("ReportProgress", false);
}

bool Preferences::lightOnDark()
{
    return getPreferenceGroup("Colors")->GetBool("LightOnDark", false);
}

void Preferences::lightOnDark(bool state)
{
    getPreferenceGroup("Colors")->SetBool("LightOnDark", state);
}

//! current setting (on/off) for monochrome display
bool Preferences::monochrome()
{
    return getPreferenceGroup("Colors")->GetBool("Monochrome", false);
}

//! set monochrome display on/off
void Preferences::monochrome(bool state)
{
    Base::Console().Message("Pref::useLightText - set to %d\n", state);
    getPreferenceGroup("Colors")->SetBool("Monochrome", state);
}

App::Color Preferences::lightTextColor()
{
    App::Color result;
    result.setPackedValue(getPreferenceGroup("Colors")->GetUnsigned("LightTextColor", 0xFFFFFFFF));//#FFFFFFFF white
    return result;
}

//! attempt to lighten the give color
// not currently used
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

    double redF = (double)red / 255.0;
    double greenF = (double)green / 255.0;
    double blueF = (double)blue / 255.0;

    return App::Color(redF, greenF, blueF, orig.a);
}

//! color to use for monochrome display
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

//! automatic correction of dimension references on/off
bool Preferences::autoCorrectDimRefs()
{
    return getPreferenceGroup("Dimensions")->GetBool("AutoCorrectRefs", true);
}

//! number of times to clean the output edges from HLR
int Preferences::scrubCount()
{
    return getPreferenceGroup("General")->GetInt("ScrubCount", 0);
}

//! Returns the factor for the overlap of svg tiles when hatching faces
double Preferences::svgHatchFactor()
{
    double factor = getPreferenceGroup("Decorations")->GetFloat("SvgOverlapFactor", 1.25);
    return factor;
}

//! For Sections with a Section as a base view, use the cut shape from the base
//! view instead of the original shape
bool Preferences::SectionUsePreviousCut()
{
    return getPreferenceGroup("General")->GetBool("SectionUsePreviousCut", false);
}

//! an index into the list of available line standards/version found in LineGroupDirectory
int Preferences::lineStandard()
{
    return getPreferenceGroup("Standards")->GetInt("LineStandard", 1);
}

//! update the line standard preference.  used in the preferences dialog.
void Preferences::setLineStandard(int index)
{
    getPreferenceGroup("Standards")->SetInt("LineStandard", index);
}

std::string Preferences::lineDefinitionLocation()
{
    std::string defaultDir = App::Application::getResourceDir() + "Mod/TechDraw/LineGroup/";
    std::string prefDir = getPreferenceGroup("Files")->GetASCII("LineDefLocation", defaultDir.c_str());
    return prefDir;
}

std::string Preferences::lineElementsLocation()
{
    std::string defaultDir = App::Application::getResourceDir() + "Mod/TechDraw/LineGroup/";
    std::string prefDir = getPreferenceGroup("Files")->GetASCII("LineElementLocation", defaultDir.c_str());
    return prefDir;
}

// Note: line numbering starts at 1, but the saved parameter is the position of the
// line style in the list, starting at 0.  We add 1 to the stored value to get the
// correct line number.
int Preferences::SectionLineStyle()
{
    // default is line #4 long dash dotted, which is index 3
    return getPreferenceGroup("Decorations")->GetInt("LineStyleSection", 3) + 1;
}

int Preferences::CenterLineStyle()
{
    // default is line #5 long dash double dotted, which is index 4
    return getPreferenceGroup("Decorations")->GetInt("LineStyleCenter", 4) + 1;
}

int Preferences::HighlightLineStyle()
{
    // default is line #2 dashed, which is index 1
    return getPreferenceGroup("Decorations")->GetInt("LineStyleHighLight", 1) + 1;
}

int Preferences::HiddenLineStyle()
{
    // default is line #2 dashed, which is index 1
    return getPreferenceGroup("Decorations")->GetInt("LineStyleHidden", 1) + 1;
}

int Preferences::LineSpacingISO()
{
    return getPreferenceGroup("Dimensions")->GetInt("LineSpacingFactorISO", 2);
}

std::string Preferences::currentLineDefFile()
{
    std::string lineDefDir = Preferences::lineDefinitionLocation();
    std::vector<std::string> choices = LineGenerator::getAvailableLineStandards();
    std::string fileName = choices.at(Preferences::lineStandard()) + ".LineDef.csv";
    return lineDefDir + fileName;
}

std::string Preferences::currentElementDefFile()
{
    std::string lineDefDir = Preferences::lineElementsLocation();
    std::vector<std::string> choices = LineGenerator::getAvailableLineStandards();
    std::string fileName = choices.at(Preferences::lineStandard()) + ".ElementDef.csv";
    return lineDefDir + fileName;
}

//! returns a Qt::PenCapStyle based on the index of the preference comboBox.
//! the comboBox choices are 0-Round, 1-Square, 2-Flat.  The Qt::PenCapStyles are
//! 0x00-Flat, 0x10-Square, 0x20-Round
int Preferences::LineCapStyle()
{
    int currentIndex = LineCapIndex();
    int result{0x20};
        switch (currentIndex) {
        case 0:
            result = static_cast<Qt::PenCapStyle>(0x20);   //round;
            break;
        case 1:
            result = static_cast<Qt::PenCapStyle>(0x10);   //square;
            break;
        case 2:
            result = static_cast<Qt::PenCapStyle>(0x00);   //flat
            break;
        default:
            result = static_cast<Qt::PenCapStyle>(0x20);
    }
    return result;
}

//! returns the line cap index without conversion to a Qt::PenCapStyle
int Preferences::LineCapIndex()
{
    return getPreferenceGroup("General")->GetInt("EdgeCapStyle", 0x20);
}

//! returns 0 (use ANSI style section cut line) or 1 (use ISO style section cut line)
int Preferences::sectionLineConvention()
{
    return getPreferenceGroup("Standards")->GetInt("SectionLineStandard", 1);
}


//! true if the GeometryMatcher should be used in correcting Dimension references
bool Preferences::useExactMatchOnDims()
{
    return getPreferenceGroup("Dimensions")->GetBool("UseMatcher", true);
}



