/***************************************************************************
 *   Copyright (c) 2015 WandererFan <wandererfan@gmail.com>                *
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
# include <iomanip>
# include <sstream>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <Base/Console.h>
#include <Base/FileInfo.h>
#include <Base/Parameter.h>

#include "DrawHatch.h"
#include "DrawHatchPy.h"  // generated from DrawHatchPy.xml
#include "DrawUtil.h"
#include "DrawViewPart.h"
#include "Preferences.h"


using namespace TechDraw;
using DU = DrawUtil;

PROPERTY_SOURCE(TechDraw::DrawHatch, App::DocumentObject)

DrawHatch::DrawHatch(void)
{
    static const char *vgroup = "Hatch";

    ADD_PROPERTY_TYPE(Source, (nullptr), vgroup, App::PropertyType::Prop_None, "The View + Face to be hatched");
    Source.setScope(App::LinkScope::Global);
    ADD_PROPERTY_TYPE(HatchPattern, (prefSvgHatch()), vgroup, App::Prop_None, "The hatch pattern file for this area");
    ADD_PROPERTY_TYPE(SvgIncluded, (""), vgroup, App::Prop_None,
                                            "Embedded SVG hatch file. System use only.");   // n/a to end users
    std::string svgFilter("SVG files (*.svg *.SVG);;Bitmap files(*.jpg *.jpeg *.png *.bmp);;All files (*)");
    HatchPattern.setFilter(svgFilter);
}

void DrawHatch::onChanged(const App::Property* prop)
{
    if (isRestoring()) {
        App::DocumentObject::onChanged(prop);
        return;
    }

    if (prop == &HatchPattern) {
        replaceFileIncluded(HatchPattern.getValue());
    }
    App::DocumentObject::onChanged(prop);
}

App::DocumentObjectExecReturn *DrawHatch::execute(void)
{
    DrawViewPart* parent = getSourceView();
    if (parent) {
        parent->requestPaint();
    }
    return App::DocumentObject::StdReturn;
}

DrawViewPart* DrawHatch::getSourceView(void) const
{
    App::DocumentObject* obj = Source.getValue();
    DrawViewPart* result = dynamic_cast<DrawViewPart*>(obj);
    return result;
}

PyObject *DrawHatch::getPyObject(void)
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DrawHatchPy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}

bool DrawHatch::faceIsHatched(int i, std::vector<TechDraw::DrawHatch*> hatchObjs)
{
    for (auto& h:hatchObjs) {
        const std::vector<std::string> &sourceNames = h->Source.getSubValues();
        for (auto& s : sourceNames) {
            int fdx = TechDraw::DrawUtil::getIndexFromName(s);
            if (fdx == i) {
                return true;  // Found something
            }
        }
    }
    return false;  // Found nothing
}

//does this hatch affect face i
bool DrawHatch::affectsFace(int i)
{
    const std::vector<std::string> &sourceNames = Source.getSubValues();
    for (auto& s : sourceNames) {
        int fdx = TechDraw::DrawUtil::getIndexFromName(s);
        if (fdx == i) {
            return true;  // Found something
        }
    }
    return false;  // Found nothing
}

//remove a subElement(Face) from Source PropertyLinkSub
bool DrawHatch::removeSub(std::string toRemove)
{
//    Base::Console().Message("DH::removeSub(%s)\n", toRemove.c_str());
    bool removed = false;
    const std::vector<std::string> &sourceNames = Source.getSubValues();
    std::vector<std::string> newList;
    App::DocumentObject* sourceFeat = Source.getValue();
    for (auto& s: sourceNames) {
        if (s == toRemove) {
            removed = true;
        } else {
            newList.push_back(s);
        }
    }
    if (removed) {
        Source.setValue(sourceFeat, newList);
    }
    return removed;
}

bool DrawHatch::removeSub(int i)
{
//    Base::Console().Message("DH::removeSub(%d)\n", i);
    std::stringstream ss;
    ss << "Face" << i;
    return removeSub(ss.str());
}

bool DrawHatch::empty(void)
{
    const std::vector<std::string> &sourceNames = Source.getSubValues();
    return sourceNames.empty();
}

void DrawHatch::replaceFileIncluded(std::string newHatchFileName)
{
//    Base::Console().Message("DH::replaceFileIncluded(%s)\n", newHatchFileName.c_str());
    if (newHatchFileName.empty()) {
        return;
    }

    Base::FileInfo tfi(newHatchFileName);
    if (tfi.isReadable()) {
        SvgIncluded.setValue(newHatchFileName.c_str());
    } else {
        throw Base::RuntimeError("Could not read the new svg file");
    }
}

void DrawHatch::setupObject()
{
//    Base::Console().Message("DH::setupObject()\n");
    replaceFileIncluded(HatchPattern.getValue());
}

void DrawHatch::unsetupObject(void)
{
//    Base::Console().Message("DH::unsetupObject() - status: %lu  removing: %d \n", getStatus(), isRemoving());
    App::DocumentObject* source = Source.getValue();
    DrawView* dv = dynamic_cast<DrawView*>(source);
    if (dv) {
        dv->requestPaint();
    }
    App::DocumentObject::unsetupObject();
}

bool DrawHatch::isSvgHatch(void) const
{
    Base::FileInfo fi(HatchPattern.getValue());
    return fi.hasExtension("svg");
}

bool DrawHatch::isBitmapHatch(void) const
{
    Base::FileInfo fi(HatchPattern.getValue());
    return fi.hasExtension({"bmp", "png", "jpg", "jpeg"});
}

//! get a translated label string from the context (ex TaskActiveView), the base name (ex ActiveView) and
//! the unique name within the document (ex ActiveView001), and use it to update the Label property.
void DrawHatch::translateLabel(std::string context, std::string baseName, std::string uniqueName)
{
    Label.setValue(DU::translateArbitrary(context, baseName, uniqueName));
}

//standard preference getters
std::string DrawHatch::prefSvgHatch(void)
{
    return Preferences::svgFile();
}

App::Color DrawHatch::prefSvgHatchColor(void)
{
    App::Color fcColor;
    fcColor.setPackedValue(Preferences::getPreferenceGroup("Colors")->GetUnsigned("Hatch", 0x00FF0000));
    return fcColor;
}

// Python Drawing feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TechDraw::DrawHatchPython, TechDraw::DrawHatch)
template<> const char* TechDraw::DrawHatchPython::getViewProviderName(void) const {
    return "TechDrawGui::ViewProviderHatch";
}
/// @endcond

// explicit template instantiation
template class TechDrawExport FeaturePythonT<TechDraw::DrawHatch>;
}
