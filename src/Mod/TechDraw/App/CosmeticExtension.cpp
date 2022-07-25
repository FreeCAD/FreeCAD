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

    EXTENSION_ADD_PROPERTY_TYPE(CosmeticVertexes, (nullptr), cgroup, App::Prop_Output, "CosmeticVertex Save/Restore");
    EXTENSION_ADD_PROPERTY_TYPE(CosmeticEdges, (nullptr), cgroup, App::Prop_Output, "CosmeticEdge Save/Restore");
    EXTENSION_ADD_PROPERTY_TYPE(CenterLines ,(nullptr),cgroup,App::Prop_Output,"Geometry format Save/Restore");
    EXTENSION_ADD_PROPERTY_TYPE(GeomFormats ,(nullptr),cgroup,App::Prop_Output,"Geometry format Save/Restore");

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

/** Adds a new CosmeticVertex.
 *
 * @param position will be the position of the new CosmeticVertex.
 * @return the tag of added CosmeticVertex.
 */
std::string CosmeticExtension::addCosmeticVertex(Base::Vector3d position)
{
//    Base::Console().Message("CEx::addCosmeticVertex(%s)\n",
 //                           DrawUtil::formatVector(pos).c_str());
    std::vector<CosmeticVertex*> verts = CosmeticVertexes.getValues();
    Base::Vector3d tempPos = DrawUtil::invertY(position);
    TechDraw::CosmeticVertex* cv = new TechDraw::CosmeticVertex(tempPos);
    verts.push_back(cv);
    CosmeticVertexes.setValues(verts);
    return cv->getTagAsString();
}

/** Returns a CosmeticVertex*.
 *
 * @param tag is the tag of the CosmeticVertex to be returned.
 */
TechDraw::CosmeticVertex* CosmeticExtension::getCosmeticVertex(std::string tag) const
{
//    Base::Console().Message("CEx::getCosmeticVertex(%s)\n", tag.c_str());
    const std::vector<TechDraw::CosmeticVertex*> verts = CosmeticVertexes.getValues();
    for (auto& cv: verts) {
        if (cv->getTagAsString() == tag) {
            return cv;
        }
    }
    return nullptr;
}

/** Returns a CosmeticVertex*.
 *
 * @param name is the selection name for the CosmeticVertex to be returned, eg. "Vertex5".
 * @return nullptr if nothing found.
 */
TechDraw::CosmeticVertex* CosmeticExtension::getCosmeticVertexBySelection(std::string name) const
{
//    Base::Console().Message("CEx::getCVBySelection(%s)\n",name.c_str());
    App::DocumentObject* extObj = const_cast<App::DocumentObject*> (getExtendedObject());
    TechDraw::DrawViewPart* dvp = dynamic_cast<TechDraw::DrawViewPart*>(extObj);
    if (!dvp)
        return nullptr;
    int index = DrawUtil::getIndexFromName(name);
    TechDraw::VertexPtr v = dvp->getProjVertexByIndex(index);
    if (!v || v->cosmeticTag.empty())
        return nullptr;
    return getCosmeticVertex(v->cosmeticTag);
}

/** Returns a CosmeticVertex*.
 *
 * @param index is the index for the CosmeticVertex to be returned, eg. 5.
 * @return nullptr if nothing found.
 */
TechDraw::CosmeticVertex* CosmeticExtension::getCosmeticVertexBySelection(int index) const
{
//    Base::Console().Message("CEx::getCVBySelection(%d)\n", i);
    std::stringstream vertexName;
    vertexName << "Vertex" << index;
    return getCosmeticVertexBySelection(vertexName.str());
}

/** Removes a CosmeticVertex*.
 *
 * @param tag is the tag for the CosmeticVertex to be removed.
 */
void CosmeticExtension::removeCosmeticVertex(std::string tag)
{
//    Base::Console().Message("DVP::removeCV(%s)\n", delTag.c_str());
    std::vector<CosmeticVertex*> cVerts = CosmeticVertexes.getValues();
    std::vector<CosmeticVertex*> newVerts;
    for (auto& cv: cVerts) {
        if (cv->getTagAsString() != tag)  {
            newVerts.push_back(cv);
        }
    }
    CosmeticVertexes.setValues(newVerts);
}

/** Removes multiple CosmeticVertexes.
 *
 * @param tag is a vector with the tags of the CosmeticVertexes to be removed.
 */
void CosmeticExtension::removeCosmeticVertex(std::vector<std::string> tag)
{
    for (auto& t: tag) {
        removeCosmeticVertex(t);
    }
}

//********** Cosmetic Edge *****************************************************

//only adds ce to celist property.  does not add to display geometry until dvp executes.
/** Adds a new CosmeticEdge 
 *
 * @param start will be the start position of the new CosmeticEdge.
 * @param end will be the end position of the new CosmeticEdge.
 * @return the tag of added CosmeticEdge.
 */
std::string CosmeticExtension::addCosmeticEdge(Base::Vector3d start,
                                               Base::Vector3d end)
{
//    Base::Console().Message("CEx::addCosmeticEdge(s,e)\n");
    std::vector<CosmeticEdge*> edges = CosmeticEdges.getValues();
    TechDraw::CosmeticEdge* ce = new TechDraw::CosmeticEdge(start, end);
    edges.push_back(ce);
    CosmeticEdges.setValues(edges);
    return ce->getTagAsString();
}

/** Adds a new CosmeticEdge.
 *
 * @param bg will be used to make the new CosmeticEdge to be added
 * @return the tag of added CosmeticEdge.
 */
std::string CosmeticExtension::addCosmeticEdge(TechDraw::BaseGeomPtr bg)
{
//    Base::Console().Message("CEx::addCosmeticEdge(bg: %X)\n", bg);
    std::vector<CosmeticEdge*> edges = CosmeticEdges.getValues();
    TechDraw::CosmeticEdge* ce = new TechDraw::CosmeticEdge(bg);
    edges.push_back(ce);
    CosmeticEdges.setValues(edges);
    return ce->getTagAsString();
}

/** Returns a CosmeticEdge.
 *
 * @param tag is the tag of the CosmeticEdge to be returned.
 * @return nullptr if nothing found.
 */
TechDraw::CosmeticEdge* CosmeticExtension::getCosmeticEdge(std::string tag) const
{
//    Base::Console().Message("CEx::getCosmeticEdge(%s)\n", tag.c_str());
    const std::vector<TechDraw::CosmeticEdge*> edges = CosmeticEdges.getValues();
    for (auto& ce: edges) {
        if (ce->getTagAsString() == tag) {
            return ce;
        }
    }

    // Nothing found?
    Base::Console().Message("CEx::getCosmeticEdge - CE for tag: %s not found.\n", tag.c_str());
    return nullptr;
}

/** Returns a CosmeticEdge.
 *
 * @param name is the selection name of the CosmeticEdge to be returned for, eg. "Edge5".
 */
TechDraw::CosmeticEdge* CosmeticExtension::getCosmeticEdgeBySelection(std::string name) const
{
//    Base::Console().Message("CEx::getCEBySelection(%s)\n",name.c_str());
    App::DocumentObject* extObj = const_cast<App::DocumentObject*> (getExtendedObject());
    TechDraw::DrawViewPart* dvp = dynamic_cast<TechDraw::DrawViewPart*>(extObj);
    if (!dvp)
        return nullptr;
    int index = DrawUtil::getIndexFromName(name);
    TechDraw::BaseGeomPtr base = dvp->getGeomByIndex(index);
    if (!base || base->getCosmeticTag().empty())
        return nullptr;
    return getCosmeticEdge(base->getCosmeticTag());
}

/** Returns a CosmeticEdge.
 *
 * @param index is the index of the CosmeticEdge to be returned for, eg. 5.
 */
TechDraw::CosmeticEdge* CosmeticExtension::getCosmeticEdgeBySelection(int index) const
{
//    Base::Console().Message("CEx::getCEBySelection(%d)\n", i);
    std::stringstream edgeName;
    edgeName << "Edge" << index;
    return getCosmeticEdgeBySelection(edgeName.str());
}

/** Removes a CosmeticEdge.
 *
 * @param tag is the tag of the CosmeticEdge to be removed.
 */
void CosmeticExtension::removeCosmeticEdge(std::string tag)
{
//    Base::Console().Message("DVP::removeCE(%s)\n", delTag.c_str());
    std::vector<CosmeticEdge*> cEdges = CosmeticEdges.getValues();
    std::vector<CosmeticEdge*> newEdges;
    for (auto& ce: cEdges) {
        if (ce->getTagAsString() != tag)  {
            newEdges.push_back(ce);
        }
    }
    CosmeticEdges.setValues(newEdges);
}

/** Removes multiple CosmeticEdge.
 *
 * @param tags is a vector with the tags of the CosmeticEdges to be removed.
 */
void CosmeticExtension::removeCosmeticEdge(std::vector<std::string> tags)
{
    for (auto& t: tags) {
        removeCosmeticEdge(t);
    }
}

//********** Center Line *******************************************************

//only adds cl to cllist property.  does not add to display geometry until dvp executes.
/** Adds a new CenterLine.
 *
 * @param start will be the start position of the new CenterLine.
 * @param end will be the end position of the new CenterLine.
 */
std::string CosmeticExtension::addCenterLine(Base::Vector3d start,
                                               Base::Vector3d end)
{
//    Base::Console().Message("CEx::addCenterLine(%s)\n",
//                            DrawUtil::formatVector(start).c_str(),
//                            DrawUtil::formatVector(end).c_str());
    std::vector<CenterLine*> cLines = CenterLines.getValues();
    TechDraw::CenterLine* cl = new TechDraw::CenterLine(start, end);
    cLines.push_back(cl);
    CenterLines.setValues(cLines);
    return cl->getTagAsString();
}

/** Adds a new CenterLine.
 *
 * @param cl will be the CenterLine to be added.
 * @return the tag of added CenterLine.
 */
std::string CosmeticExtension::addCenterLine(TechDraw::CenterLine* cl)
{
//    Base::Console().Message("CEx::addCenterLine(cl: %X)\n", cl);
    std::vector<CenterLine*> cLines = CenterLines.getValues();
    cLines.push_back(cl);
    CenterLines.setValues(cLines);
    return cl->getTagAsString();
}

/** Adds a new CenterLine.
 *
 * @param bg will be used to make the new CenterLine to be added.
 * @return the tag of added CenterLine.
 */
std::string CosmeticExtension::addCenterLine(TechDraw::BaseGeomPtr bg)
{
//    Base::Console().Message("CEx::addCenterLine(bg: %X)\n", bg);
    std::vector<CenterLine*> cLines = CenterLines.getValues();
    TechDraw::CenterLine* cl = new TechDraw::CenterLine(bg);
    cLines.push_back(cl);
    CenterLines.setValues(cLines);
    return cl->getTagAsString();
}


/** Returns a CenterLine.
 *
 * @param tag is the tag of the CenterLine to be returned.
 * @return nullptr if nothing found.
 */
TechDraw::CenterLine* CosmeticExtension::getCenterLine(std::string tag) const
{
//    Base::Console().Message("CEx::getCenterLine(%s)\n", tag.c_str());
    const std::vector<TechDraw::CenterLine*> cLines = CenterLines.getValues();
    for (auto& cl: cLines) {
        if (cl->getTagAsString() == tag) {
            return cl;
        }
    }

    // Nothing found?
    return nullptr;
}

/** Returns a CosmeticVertex.
 *
 * @param name is the selection name for the CenterLine to be returned, eg. "Edge5".
 * @return nullptr if nothing found.
 */
TechDraw::CenterLine* CosmeticExtension::getCenterLineBySelection(std::string name) const
{
//    Base::Console().Message("CEx::getCLBySelection(%s)\n",name.c_str());
    App::DocumentObject* extObj = const_cast<App::DocumentObject*> (getExtendedObject());
    TechDraw::DrawViewPart* dvp = dynamic_cast<TechDraw::DrawViewPart*>(extObj);
    if (!dvp)
        return nullptr;
    int index = DrawUtil::getIndexFromName(name);
    TechDraw::BaseGeomPtr base = dvp->getGeomByIndex(index);
    if (!base || base->getCosmeticTag().empty())
        return nullptr;
    return getCenterLine(base->getCosmeticTag());
}

/** Returns a CenterLine.
 *
 * @param index is the index for the CenterLine to be returned, eg. 5.
 * @return nullptr if nothing found.
 */
TechDraw::CenterLine* CosmeticExtension::getCenterLineBySelection(int index) const
{
//    Base::Console().Message("CEx::getCLBySelection(%d)\n", i);
    std::stringstream edgeName;
    edgeName << "Edge" << index;
    return getCenterLineBySelection(edgeName.str());
}

/** Removes a CenterLine.
 *
 * @param tag is the tag for the CenterLine to be removed.
 */
void CosmeticExtension::removeCenterLine(std::string tag)
{
//    Base::Console().Message("DVP::removeCL(%s)\n", delTag.c_str());
    std::vector<CenterLine*> cLines = CenterLines.getValues();
    std::vector<CenterLine*> newLines;
    for (auto& cl: cLines) {
        if (cl->getTagAsString() != tag)  {
            newLines.push_back(cl);
        }
    }
    CenterLines.setValues(newLines);
}

/** Removes multiple CenterLines.
 *
 * @param tags is a vector with the tags of the CosmeticVertexes to be removed.
 */
void CosmeticExtension::removeCenterLine(std::vector<std::string> tags)
{
    for (auto& t: tags) {
        removeCenterLine(t);
    }
}


//********** Geometry Formats **************************************************
//only adds gf to gflist property.  does not add to display geometry until dvp repaints.
/** Adds a new GeomFormat.
 *
 * @param gf will be the GeomFormat of the new GeomFormat.
 * @return the tag of added GeomFormat.
 */
std::string CosmeticExtension::addGeomFormat(TechDraw::GeomFormat* gf)
{
//    Base::Console().Message("CEx::addGeomFormat(gf: %X)\n", gf);
    std::vector<GeomFormat*> formats = GeomFormats.getValues();
    TechDraw::GeomFormat* newGF = new TechDraw::GeomFormat(gf);
    formats.push_back(newGF);
    GeomFormats.setValues(formats);
    return newGF->getTagAsString();
}


/** Returns a GeomFormat.
 *
 * @param tag is the tag of the GeomFormat to be returned.
 * @return nullptr if nothing found.
 */
TechDraw::GeomFormat* CosmeticExtension::getGeomFormat(std::string tag) const
{
//    Base::Console().Message("CEx::getGeomFormat(%s)\n", tag.c_str());
    const std::vector<TechDraw::GeomFormat*> formats = GeomFormats.getValues();
    for (auto& gf: formats) {
        if (gf->getTagAsString() == tag) {
            return gf;
        }
    }
    return nullptr;
}

/** Returns a GeomFormat.
 *
 * @param name is the selection name for the GeomFormat to be returned, eg. "Edge5".
 * @return nullptr if nothing found.
 */
TechDraw::GeomFormat* CosmeticExtension::getGeomFormatBySelection(std::string name) const
{
//    Base::Console().Message("CEx::getCEBySelection(%s)\n",name.c_str());
    App::DocumentObject* extObj = const_cast<App::DocumentObject*> (getExtendedObject());
    TechDraw::DrawViewPart* dvp = dynamic_cast<TechDraw::DrawViewPart*>(extObj);
    if (!dvp)
        return nullptr;
    int index = DrawUtil::getIndexFromName(name);
    const std::vector<TechDraw::GeomFormat*> formats = GeomFormats.getValues();
    for (auto& gf: formats) {
        if (gf->m_geomIndex == index) {
            return gf;
        }
    }
    return nullptr;
}

/** Returns a GeomFormat.
 *
 * @param index is the index for the GeomFormat to be returned, eg. 5.
 * @return nullptr if nothing found.
 */
TechDraw::GeomFormat* CosmeticExtension::getGeomFormatBySelection(int index) const
{
//    Base::Console().Message("CEx::getCEBySelection(%d)\n", i);
    std::stringstream edgeName;
    edgeName << "Edge" << index;
    return getGeomFormatBySelection(edgeName.str());
}

/** Removes a GeomFormat.
 *
 * @param tag is the tag for the GeomFormat to be removed.
 */
void CosmeticExtension::removeGeomFormat(std::string tag)
{
//    Base::Console().Message("DVP::removeCE(%s)\n", delTag.c_str());
    std::vector<GeomFormat*> cFormats = GeomFormats.getValues();
    std::vector<GeomFormat*> newFormats;
    for (auto& gf: cFormats) {
        if (gf->getTagAsString() != tag)  {
            newFormats.push_back(gf);
        }
    }
    GeomFormats.setValues(newFormats);
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


