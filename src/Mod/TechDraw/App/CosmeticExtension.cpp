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

#include <Base/Console.h>

#include "CenterLine.h"
#include "CosmeticExtension.h"
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

    EXTENSION_ADD_PROPERTY_TYPE(CosmeticVertexes, (nullptr), cgroup, App::Prop_Output, "CosmeticVertex Save/Restore");
    EXTENSION_ADD_PROPERTY_TYPE(Cosmetics, (nullptr), cgroup, App::Prop_Output, "Cosmetics Save/Restore");

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
//CosmeticVertex x, y are stored as unscaled, but mirrored (inverted Y) values.
//if you are creating a CV based on calculations of scaled geometry, you need to
//unscale x, y before creation.
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
    const std::vector<TechDraw::CosmeticVertex*> verts = CosmeticVertexes.getValues();
    for (auto& cv: verts) {
        std::string cvTag = cv->getTagAsString();
        if (cvTag == tagString) {
            return cv;
        }
    }
    return nullptr;
}

// find the cosmetic vertex corresponding to selection name (Vertex5)
// used when selecting
TechDraw::CosmeticVertex* CosmeticExtension::getCosmeticVertexBySelection(std::string name) const
{
//    Base::Console().Message("CEx::getCVBySelection(%s)\n", name.c_str());
    App::DocumentObject* extObj = const_cast<App::DocumentObject*> (getExtendedObject());
    TechDraw::DrawViewPart* dvp = dynamic_cast<TechDraw::DrawViewPart*>(extObj);
    if (!dvp) {
        return nullptr;
    }
    int idx = DrawUtil::getIndexFromName(name);
    TechDraw::VertexPtr v = dvp->getProjVertexByIndex(idx);
    if (!v || v->getCosmeticTag().empty()) {
        return nullptr;
    }
    return getCosmeticVertex(v->getCosmeticTag());
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

//********** Geometry Formats **************************************************
// find the cosmetic edge corresponding to selection name (Edge5)
// used when selecting
TechDraw::GeomFormat* CosmeticExtension::getGeomFormatBySelection(std::string name) const
{
//    Base::Console().Message("CEx::getCEBySelection(%s)\n", name.c_str());
    App::DocumentObject* extObj = const_cast<App::DocumentObject*> (getExtendedObject());
    TechDraw::DrawViewPart* dvp = dynamic_cast<TechDraw::DrawViewPart*>(extObj);
    if (!dvp) {
        return nullptr;
    }
    int idx = DrawUtil::getIndexFromName(name);
    const std::vector<TechDraw::GeomFormat*> formats = Cosmetics.getValues<GeomFormat*>();
    for (auto& gf: formats) {
        if (gf->m_geomIndex == idx) {
            return gf;
        }
    }

    // Nothing found
    return nullptr;
}

//overload for index only
TechDraw::GeomFormat* CosmeticExtension::getGeomFormatBySelection(int i) const
{
//    Base::Console().Message("CEx::getCEBySelection(%d)\n", i);
    std::stringstream edgeName;
    edgeName << "Edge" << i;
    return getGeomFormatBySelection(edgeName.str());
}

template<typename T>
std::string CosmeticExtension::addCosmetic(BaseGeomPtr bg) {
    static_assert(!std::is_pointer<T>::value, "Template argument must not be pointer!!!");

    T* cosmetic = new T(bg);
    Cosmetics.addValue(cosmetic);
    return cosmetic->getTagAsString();
}
template std::string CosmeticExtension::addCosmetic<CosmeticEdge>(BaseGeomPtr bg);

//only adds gf to gflist property.  does not add to display geometry until dvp repaints.
template<typename T>
std::string CosmeticExtension::addCosmetic(T* cosmetic) {
    static_assert(!std::is_pointer<T>::value, "Template argument must not be pointer!!!");

    T* newCosmetic = new T(cosmetic);
    Cosmetics.addValue(newCosmetic);
    return newCosmetic->getTagAsString();
}
template std::string CosmeticExtension::addCosmetic<GeomFormat>(GeomFormat* cosmetic);
template std::string CosmeticExtension::addCosmetic<CenterLine>(CenterLine* cosmetic);

void CosmeticExtension::removeCosmetic(std::string tag) {
    Cosmetics.removeValue(tag);
}

void CosmeticExtension::removeCosmetic(std::vector<std::string> tags) {
    for(auto& tag: tags) {
        Cosmetics.removeValue(tag);
    }
}

// find the center line corresponding to selection name (Edge5)
//overload for index only
template<typename T>
T CosmeticExtension::getCosmeticByName(int index) const
{
    static_assert(std::is_pointer<T>::value, "Template argument must be pointer!!!");
    // Base::Console().Message("CEx::getCLBySelection(%d)\n", i);
    App::DocumentObject* extObj = const_cast<App::DocumentObject*> (getExtendedObject());
    TechDraw::DrawViewPart* dvp = dynamic_cast<TechDraw::DrawViewPart*>(extObj);
    if (!dvp) {
        return nullptr;
    }
    TechDraw::BaseGeomPtr base = dvp->getGeomByIndex(index);
    if (!base || base->getCosmeticTag().empty()) {
        return nullptr;
    }
    return Cosmetics.getValue<T>(base->getCosmeticTag());
}
template CenterLine* CosmeticExtension::getCosmeticByName(int i) const;
template CosmeticEdge* CosmeticExtension::getCosmeticByName(int i) const;


// used when selecting
template<typename T>
T CosmeticExtension::getCosmeticByName(std::string name) const
{
    static_assert(std::is_pointer<T>::value, "Template argument must be pointer!!!");
    // Base::Console().Message("CEx::getCLBySelection(%s)\n", name.c_str());
    int index = DrawUtil::getIndexFromName(name);
    return getCosmeticByName<T>(index);
}
template CenterLine* CosmeticExtension::getCosmeticByName(std::string name) const;
template CosmeticEdge* CosmeticExtension::getCosmeticByName(std::string name) const;

//returns unique CE id
//only adds ce to celist property.  does not add to display geometry until dvp executes.
template<typename T>
std::string CosmeticExtension::addCosmetic(const Base::Vector3d& start, const Base::Vector3d& end)
{
    static_assert(!std::is_pointer<T>::value, "Template argument must not be pointer!!!");
    //    Base::Console().Message("CEx::addCosmeticEdge(s, e)\n");
    T* newCosmetic = new T(start, end);
    Cosmetics.addValue(newCosmetic);
    return newCosmetic->getTagAsString();  // What happens to newCosmetic??? Memory-leak???
}
template std::string CosmeticExtension::addCosmetic<CenterLine>(const Base::Vector3d& start, const Base::Vector3d& end);
template std::string CosmeticExtension::addCosmetic<CosmeticEdge>(const Base::Vector3d& start, const Base::Vector3d& end);


//================================================================================
PyObject* CosmeticExtension::getExtensionPyObject(void) {
    if (ExtensionPythonObject.is(Py::_None())){
        // ref counter is set to 1
        ExtensionPythonObject = Py::Object(new CosmeticExtensionPy(this), true);
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


