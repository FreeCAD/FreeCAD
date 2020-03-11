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
# include <sstream>
#include <Precision.hxx>
#include <cmath>

#endif

#include <iomanip>

# include <QFile>
# include <QFileInfo>

#include <App/Application.h>
#include <App/Document.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Parameter.h>
#include <Base/UnitsApi.h>

#include "DrawViewPart.h"
#include "DrawUtil.h"
#include "DrawHatch.h"

#include <Mod/TechDraw/App/DrawHatchPy.h>  // generated from DrawHatchPy.xml

using namespace TechDraw;
using namespace std;

PROPERTY_SOURCE(TechDraw::DrawHatch, App::DocumentObject)


DrawHatch::DrawHatch(void)
{
    static const char *vgroup = "Hatch";

    ADD_PROPERTY_TYPE(DirProjection ,(0,0,1.0)    ,vgroup,App::Prop_None,"Projection direction when Hatch was defined");     //sb RO?
    ADD_PROPERTY_TYPE(Source,(0),vgroup,(App::PropertyType)(App::Prop_None),"The View + Face to be hatched");
    Source.setScope(App::LinkScope::Global);
    ADD_PROPERTY_TYPE(HatchPattern ,(""),vgroup,App::Prop_None,"The hatch pattern file for this area");
    ADD_PROPERTY_TYPE(SvgIncluded, (""), vgroup,App::Prop_None,
                                            "Embedded Svg hatch file. System use only.");   // n/a to end users

    DirProjection.setStatus(App::Property::ReadOnly,true);

    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Files");

    std::string defaultDir = App::Application::getResourceDir() + "Mod/TechDraw/Patterns/";
    std::string defaultFileName = defaultDir + "simple.svg";
    QString patternFileName = QString::fromStdString(hGrp->GetASCII("FileHatch",defaultFileName.c_str()));
    if (patternFileName.isEmpty()) {
        patternFileName = QString::fromStdString(defaultFileName);
    }
    QFileInfo tfi(patternFileName);
    if (tfi.isReadable()) {
        HatchPattern.setValue(patternFileName.toUtf8().constData());
    }
    
    std::string svgFilter("Svg files (*.svg *.SVG);;All files (*)");
    HatchPattern.setFilter(svgFilter);

//    SvgIncluded.setStatus(App::Property::ReadOnly,true);

}

DrawHatch::~DrawHatch()
{
}

void DrawHatch::onChanged(const App::Property* prop)
{
    if (!isRestoring()) {
        if (prop == &Source) {
            DrawHatch::execute();
        }
        App::Document* doc = getDocument();
        if ((prop == &HatchPattern) &&
            (doc != nullptr) ) {
            if (!HatchPattern.isEmpty()) {
                replaceSvgIncluded(HatchPattern.getValue());
            }
        }
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
        PythonObject = Py::Object(new DrawHatchPy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}

bool DrawHatch::faceIsHatched(int i,std::vector<TechDraw::DrawHatch*> hatchObjs)
{
    bool result = false;
    bool found = false;
    for (auto& h:hatchObjs) {
        const std::vector<std::string> &sourceNames = h->Source.getSubValues();
        for (auto& s : sourceNames) {
            int fdx = TechDraw::DrawUtil::getIndexFromName(s);
            if (fdx == i) {
                result = true;
                found = true;
                break;
            }
        }
        if (found) {
            break;
        }
    }
    return result;
}

//does this hatch affect face i
bool DrawHatch::affectsFace(int i)
{
    bool result = false;
    const std::vector<std::string> &sourceNames = Source.getSubValues();
    for (auto& s : sourceNames) {
        int fdx = TechDraw::DrawUtil::getIndexFromName(s);
            if (fdx == i) {
                result = true;
                break;
            }
    }
    return result;
}

//remove a subElement(Face) from Source PropertyLinkSub
bool DrawHatch::removeSub(std::string toRemove)
{
//    Base::Console().Message("DH::removeSub(%s)\n",toRemove.c_str());
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
//    Base::Console().Message("DH::removeSub(%d)\n",i);
    std::stringstream ss;
    ss << "Face" << i;
    return removeSub(ss.str());
}

bool DrawHatch::empty(void)
{
    const std::vector<std::string> &sourceNames = Source.getSubValues();
    return sourceNames.empty();
}

void DrawHatch::replaceSvgIncluded(std::string newSvgFile)
{
//    Base::Console().Message("DH::replaceSvgHatch(%s)\n", newSvgFile.c_str());
    if (SvgIncluded.isEmpty()) {
        setupSvgIncluded();
    } else {
        std::string tempName = SvgIncluded.getExchangeTempFile();
        copyFile(newSvgFile, tempName);
        SvgIncluded.setValue(tempName.c_str());
    }
}

void DrawHatch::onDocumentRestored() 
{
//if this is a restore, we should be checking for SvgIncluded empty,
// if it is, set it up from hatchPattern,
// else, don't do anything
//    Base::Console().Message("DH::onDocumentRestored()\n");
    if (SvgIncluded.isEmpty()) {
        if (!HatchPattern.isEmpty()) {
            std::string svgFileName = HatchPattern.getValue();
            Base::FileInfo tfi(svgFileName);
            if (tfi.isReadable()) {
                if (SvgIncluded.isEmpty()) {
                    setupSvgIncluded();
                }
            }
        }
    }
    App::DocumentObject::onDocumentRestored();
}

void DrawHatch::setupObject()
{
    //by this point DH should have a name and belong to a document
    setupSvgIncluded();

    App::DocumentObject::setupObject();
}

void DrawHatch::setupSvgIncluded(void)
{
//    Base::Console().Message("DH::setupSvgIncluded()\n");
    App::Document* doc = getDocument();
    std::string special = getNameInDocument();
    special += "SvgHatch.svg";
    std::string dir = doc->TransientDir.getValue();
    std::string svgName = dir + special;

    if (SvgIncluded.isEmpty()) {
        copyFile(std::string(), svgName);
        SvgIncluded.setValue(svgName.c_str());
    }

    if (!HatchPattern.isEmpty()) {
        std::string exchName = SvgIncluded.getExchangeTempFile();
        copyFile(HatchPattern.getValue(), exchName);
        SvgIncluded.setValue(exchName.c_str(), special.c_str());
    }
}

//copy whole text file from inSpec to outSpec
void DrawHatch::copyFile(std::string inSpec, std::string outSpec)
{
//    Base::Console().Message("DH::copyFile(%s, %s)\n", inSpec.c_str(), outSpec.c_str());
    if (inSpec.empty()) {
        std::ofstream  dst(outSpec);   //make an empty file
    } else {
        std::ifstream  src(inSpec);
        std::ofstream  dst(outSpec);
        dst << src.rdbuf();
    }
}

void DrawHatch::unsetupObject(void)
{
//    Base::Console().Message("DH::unsetupObject() - status: %lu  removing: %d \n", getStatus(), isRemoving());
    App::DocumentObject* source = Source.getValue();
    DrawView* dv = dynamic_cast<DrawView*>(source);
    if (dv != nullptr) {
        dv->requestPaint();
    }
    App::DocumentObject::unsetupObject();
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
