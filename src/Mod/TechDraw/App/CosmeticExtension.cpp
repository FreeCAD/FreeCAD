/***************************************************************************
 *   Copyright (c) 2019 WandererFan <wandererfan@gmail.com>                *
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
#endif  // #ifndef _PreComp_

#include "CosmeticExtension.h"

#include <Base/Console.h>

#include <App/Application.h>
#include <App/FeaturePythonPyImp.h>

#include "CosmeticExtensionPy.h"

#include "Cosmetic.h"
#include "DrawUtil.h"
#include "DrawViewPart.h"

using namespace TechDraw;
using namespace std;


EXTENSION_PROPERTY_SOURCE(TechDraw::CosmeticExtension, App::DocumentObjectExtension)

CosmeticExtension::CosmeticExtension() 
{
    static const char *cgroup = "Cosmetics";

    EXTENSION_ADD_PROPERTY_TYPE(CosmeticVertexes ,(0),cgroup,App::Prop_Output,"CosmeticVertex Save/Restore");

    initExtensionType(CosmeticExtension::getExtensionClassTypeId());
}

CosmeticExtension::~CosmeticExtension() 
{
}

//void CosmeticExtension::extHandleChangedPropertyName(Base::XMLReader &reader, 
//                                                     const char* TypeName, 
//                                                     const char* PropName)
//{
//}

//==============================================================================
//CosmeticVertex x,y are stored as unscaled, but mirrored (inverted Y) values.
//if you are creating a CV based on calculations of scaled geometry, you need to 
//unscale x,y before creation.
//if you are creating a CV based on calculations of mirrored geometry, you need to
//mirror again before creation.

//returns unique CV id
//only adds cv to cvlist property.  does not add to display geometry until dvp executes.
std::string CosmeticExtension::addCosmeticVertex(Base::Vector3d pos)
{
//    Base::Console().Message("CEx::addCosmeticVertex(%s)\n",
 //                           DrawUtil::formatVector(pos).c_str());
    std::vector<CosmeticVertex*> verts = CosmeticVertexes.getValues();
    Base::Vector3d tempPos = DrawUtil::invertY(pos);
    TechDraw::CosmeticVertex* cv = new TechDraw::CosmeticVertex(tempPos);
    verts.push_back(cv);
    CosmeticVertexes.setValues(verts);
    std::string result = cv->getTagAsString();
    return result;
}

//get CV by unique id
TechDraw::CosmeticVertex* CosmeticExtension::getCosmeticVertex(std::string tagString) const
{
//    Base::Console().Message("CEx::getCosmeticVertex(%s)\n", tagString.c_str());
    CosmeticVertex* result = nullptr;
    const std::vector<TechDraw::CosmeticVertex*> verts = CosmeticVertexes.getValues();
    for (auto& cv: verts) {
        std::string cvTag = cv->getTagAsString();
        if (cvTag == tagString) {
            result = cv;
            break;
        }
    }
    return result;
}

// find the cosmetic vertex corresponding to selection name (Vertex5)
// used when selecting
TechDraw::CosmeticVertex* CosmeticExtension::getCosmeticVertexBySelection(std::string name) const
{
//    Base::Console().Message("CEx::getCVBySelection(%s)\n",name.c_str());
    CosmeticVertex* result = nullptr;
    App::DocumentObject* extObj = const_cast<App::DocumentObject*> (getExtendedObject());
    TechDraw::DrawViewPart* dvp = dynamic_cast<TechDraw::DrawViewPart*>(extObj);
    if (dvp == nullptr) {
        return result;
    }
    int idx = DrawUtil::getIndexFromName(name);
    TechDraw::Vertex* v = dvp->getProjVertexByIndex(idx);
    if (v == nullptr) {
        return result;
    }
    if (!v->cosmeticTag.empty()) {
        result = getCosmeticVertex(v->cosmeticTag);
    }
    return result;
}

//overload for index only 
TechDraw::CosmeticVertex* CosmeticExtension::getCosmeticVertexBySelection(int i) const
{
//    Base::Console().Message("CEx::getCVBySelection(%d)\n", i);
    std::stringstream ss;
    ss << "Vertex" << i;
    std::string vName = ss.str();
    return getCosmeticVertexBySelection(vName);
}

void CosmeticExtension::removeCosmeticVertex(std::string delTag)
{
//    Base::Console().Message("DVP::removeCV(%s)\n", delTag.c_str());
    std::vector<CosmeticVertex*> cVerts = CosmeticVertexes.getValues();
    std::vector<CosmeticVertex*> newVerts;
    for (auto& cv: cVerts) {
        if (cv->getTagAsString() != delTag)  {
            newVerts.push_back(cv);
        }
    }
    CosmeticVertexes.setValues(newVerts);
}

void CosmeticExtension::removeCosmeticVertex(std::vector<std::string> delTags)
{
    for (auto& t: delTags) {
        removeCosmeticVertex(t);
    }
}

bool CosmeticExtension::replaceCosmeticVertex(CosmeticVertex* newCV)
{
//    Base::Console().Message("DVP::replaceCV(%s)\n", newCV->getTagAsString().c_str());
    bool result = false;
    std::vector<CosmeticVertex*> cVerts = CosmeticVertexes.getValues();
    std::vector<CosmeticVertex*> newVerts;
    std::string tag = newCV->getTagAsString();
    for (auto& cv: cVerts) {
        if (cv->getTagAsString() == tag)  {
            newVerts.push_back(newCV);
            result = true;
        } else { 
            newVerts.push_back(cv);
        }
    }
    CosmeticVertexes.setValues(newVerts);
    return result;
}

//================================================================================
PyObject* CosmeticExtension::getExtensionPyObject(void) {
    if (ExtensionPythonObject.is(Py::_None())){
        // ref counter is set to 1
        ExtensionPythonObject = Py::Object(new CosmeticExtensionPy(this),true);
    }
    return Py::new_reference_to(ExtensionPythonObject);
}

namespace App {
/// @cond DOXERR
  EXTENSION_PROPERTY_SOURCE_TEMPLATE(TechDraw::CosmeticExtensionPython, TechDraw::CosmeticExtension)
/// @endcond

// explicit template instantiation
  template class TechDrawExport ExtensionPythonT<TechDraw::CosmeticExtension>;
}


