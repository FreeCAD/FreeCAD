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
#include "GeometryObject.h"

using namespace TechDraw;
using namespace std;
using DU = DrawUtil;

EXTENSION_PROPERTY_SOURCE(TechDraw::CosmeticExtension, App::DocumentObjectExtension)

CosmeticExtension::CosmeticExtension()
{
    static const char *cgroup = "Cosmetics";

    EXTENSION_ADD_PROPERTY_TYPE(CosmeticVertexes, (nullptr), cgroup, App::Prop_Output, "CosmeticVertex Save/Restore");
    EXTENSION_ADD_PROPERTY_TYPE(CosmeticEdges, (nullptr), cgroup, App::Prop_Output, "CosmeticEdge Save/Restore");
    EXTENSION_ADD_PROPERTY_TYPE(CenterLines ,(nullptr), cgroup, App::Prop_Output, "CenterLine Save/Restore");
    EXTENSION_ADD_PROPERTY_TYPE(GeomFormats ,(nullptr), cgroup, App::Prop_Output, "Geometry format Save/Restore");

    initExtensionType(CosmeticExtension::getExtensionClassTypeId());
}

CosmeticExtension::~CosmeticExtension()
{
    // do not free memory here as the destruction of the properties will
    // delete any entries.
}

/// get a pointer to the parent view
TechDraw::DrawViewPart*  CosmeticExtension::getOwner()
{
    return static_cast<TechDraw::DrawViewPart*>(getExtendedObject());
}

//==============================================================================
//CosmeticVertex x, y are stored as unscaled, but mirrored (inverted Y) values.
//if you are creating a CV based on calculations of scaled geometry, you need to
//unscale x, y before creation.
//if you are creating a CV based on calculations of mirrored geometry, you need to
//mirror again before creation.

// this is never called!
/// remove all the cosmetic vertices in the property list
void CosmeticExtension::clearCosmeticVertexes()
{
    std::vector<TechDraw::CosmeticVertex*> cVerts = CosmeticVertexes.getValues();
    for (auto& vert : cVerts) {
        delete vert;
    }
    std::vector<CosmeticVertex*> noVerts;
    CosmeticVertexes.setValues(noVerts);
}

/// add the cosmetic verts in the property list to view's vertex geometry list
void CosmeticExtension::addCosmeticVertexesToGeom()
{
//    Base::Console().Message("CE::addCosmeticVertexesToGeom()\n");
    const std::vector<TechDraw::CosmeticVertex*> cVerts = CosmeticVertexes.getValues();
    for (auto& cv : cVerts) {
        double scale = getOwner()->getScale();
        double rotDegrees = getOwner()->Rotation.getValue();
        Base::Vector3d cvPosition = cv->rotatedAndScaled(scale, rotDegrees);
        int iGV = getOwner()->getGeometryObject()->addCosmeticVertex(cvPosition, cv->getTagAsString());
        cv->linkGeom = iGV;
    }
}

/// add a single cosmetic vertex in the property list to the view's vertex geometry list
int CosmeticExtension::add1CVToGV(const std::string& tag)
{
//    Base::Console().Message("CE::add1CVToGV(%s)\n", tag.c_str());
    TechDraw::CosmeticVertex* cv = getCosmeticVertex(tag);
    if (!cv) {
        Base::Console().Message("CE::add1CVToGV - cv %s not found\n", tag.c_str());
        return 0;
    }
    double scale = getOwner()->getScale();
    double rotDegrees = getOwner()->Rotation.getValue();
    Base::Vector3d cvPosition = cv->rotatedAndScaled(scale, rotDegrees);
    int iGV = getOwner()->getGeometryObject()->addCosmeticVertex(cvPosition, cv->getTagAsString());
    cv->linkGeom = iGV;
    return iGV;
}

/// update the parent view's vertex geometry with all the cosmetic vertices in the list property
void CosmeticExtension::refreshCVGeoms()
{
    //    Base::Console().Message("CE::refreshCVGeoms()\n");

    std::vector<TechDraw::VertexPtr> gVerts = getOwner()->getVertexGeometry();
    std::vector<TechDraw::VertexPtr> newGVerts;
    for (auto& gv : gVerts) {
        if (gv->getCosmeticTag().empty()) {//keep only non-cv vertices
            newGVerts.push_back(gv);
        }
    }
    getOwner()->getGeometryObject()->setVertexGeometry(newGVerts);
    addCosmeticVertexesToGeom();
}

//what is the CV's position in the big geometry q
/// find the position of a cosmetic vertex with the given tag in the parent view's geometry list
int CosmeticExtension::getCVIndex(const std::string& tag)
{
    //    Base::Console().Message("CE::getCVIndex(%s)\n", tag.c_str());
    std::vector<TechDraw::VertexPtr> gVerts = getOwner()->getVertexGeometry();
    std::vector<TechDraw::CosmeticVertex*> cVerts = CosmeticVertexes.getValues();

    int i = 0;
    for (auto& gv : gVerts) {
        if (gv->getCosmeticTag() == tag) {
            return i;
        }
        i++;
    }

    // Nothing found
    int base = gVerts.size();
    i = 0;
    for (auto& cv : cVerts) {
        //        Base::Console().Message("CE::getCVIndex - cv tag: %s\n",
        //                                cv->getTagAsString().c_str());
        if (cv->getTagAsString() == tag) {
            return base + i;
        }
        i++;
    }

    //    Base::Console().Message("CE::getCVIndex - returns: %d\n", result);
    return -1;
}

/// adds a cosmetic vertex to the property list.  does not add to display geometry until dvp executes.
/// returns unique CV id
std::string CosmeticExtension::addCosmeticVertex(const Base::Vector3d& pos)
{
//    Base::Console().Message("CEx::addCosmeticVertex(%s)\n",
//                             DrawUtil::formatVector(pos).c_str());
    std::vector<CosmeticVertex*> verts = CosmeticVertexes.getValues();
    Base::Vector3d tempPos = DrawUtil::invertY(pos);
    TechDraw::CosmeticVertex* cv = new TechDraw::CosmeticVertex(tempPos);
    verts.push_back(cv);
    CosmeticVertexes.setValues(verts);
    std::string result = cv->getTagAsString();
    return result;
}

/// retrieve a cosmetic vertex by unique id
TechDraw::CosmeticVertex* CosmeticExtension::getCosmeticVertex(const std::string& tagString) const
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

/// retrieve a cosmetic vertex by selection name (Vertex5)
/// used when selecting
TechDraw::CosmeticVertex* CosmeticExtension::getCosmeticVertexBySelection(const std::string& name) const
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

/// retrieve a cosmetic vertex by index (the 5 in Vertex5)
TechDraw::CosmeticVertex* CosmeticExtension::getCosmeticVertexBySelection(const int i) const
{
//    Base::Console().Message("CEx::getCVBySelection(%d)\n", i);
    std::stringstream ss;
    ss << "Vertex" << i;
    std::string vName = ss.str();
    return getCosmeticVertexBySelection(vName);
}

/// remove the cosmetic vertex with the given tag from the list property
void CosmeticExtension::removeCosmeticVertex(const std::string& delTag)
{
//    Base::Console().Message("DVP::removeCV(%s)\n", delTag.c_str());
    std::vector<CosmeticVertex*> cVerts = CosmeticVertexes.getValues();
    std::vector<CosmeticVertex*> newVerts;
    for (auto& cv: cVerts) {
        if (cv->getTagAsString() == delTag)  {
            delete cv;
        } else {
            newVerts.push_back(cv);
        }
    }
    CosmeticVertexes.setValues(newVerts);
}

/// remove the cosmetic vertices with the given tags from the list property
void CosmeticExtension::removeCosmeticVertex(const std::vector<std::string>& delTags)
{
    for (auto& t: delTags) {
        removeCosmeticVertex(t);
    }
}

//********** Cosmetic Edge *****************************************************

// this is never called!
/// remove all the cosmetic edges for this view
void CosmeticExtension::clearCosmeticEdges()
{
    std::vector<TechDraw::CosmeticEdge*> cEdges = CosmeticEdges.getValues();
    for (auto& edge : cEdges) {
        delete edge;
    }
    std::vector<CosmeticEdge*> noEdges;
    CosmeticEdges.setValues(noEdges);
}

/// add the cosmetic edges to geometry edge list
void CosmeticExtension::addCosmeticEdgesToGeom()
{
//    Base::Console().Message("CEx::addCosmeticEdgesToGeom()\n");
    const std::vector<TechDraw::CosmeticEdge*> cEdges = CosmeticEdges.getValues();
    for (auto& ce : cEdges) {
        double scale = getOwner()->getScale();
        double rotDegrees = getOwner()->Rotation.getValue();
        TechDraw::BaseGeomPtr scaledGeom = ce->scaledAndRotatedGeometry(scale, rotDegrees);
        if (!scaledGeom)
            continue;
        //        int iGE =
        getOwner()->getGeometryObject()->addCosmeticEdge(scaledGeom, ce->getTagAsString());
    }
}

/// add a single cosmetic edge to the geometry edge list
int CosmeticExtension::add1CEToGE(const std::string& tag)
{
    //    Base::Console().Message("CEx::add1CEToGE(%s) 2\n", tag.c_str());
    TechDraw::CosmeticEdge* ce = getCosmeticEdge(tag);
    if (!ce) {
        Base::Console().Message("CEx::add1CEToGE 2 - ce %s not found\n", tag.c_str());
        return -1;
    }
    double scale = getOwner()->getScale();
    double rotDegrees = getOwner()->Rotation.getValue();
    TechDraw::BaseGeomPtr scaledGeom = ce->scaledAndRotatedGeometry(scale, rotDegrees);
    int iGE = getOwner()->getGeometryObject()->addCosmeticEdge(scaledGeom, tag);

    return iGE;
}

/// update Edge geometry with current CE's
void CosmeticExtension::refreshCEGeoms()
{
    //    Base::Console().Message("CEx::refreshCEGeoms()\n");
    std::vector<TechDraw::BaseGeomPtr> gEdges = getOwner()->getEdgeGeometry();
    std::vector<TechDraw::BaseGeomPtr> oldGEdges;
    for (auto& ge : gEdges) {
        if (ge->source() != SourceType::COSEDGE) {
            oldGEdges.push_back(ge);
        }
    }
    getOwner()->getGeometryObject()->setEdgeGeometry(oldGEdges);
    addCosmeticEdgesToGeom();
}

/// adds a new cosmetic edge to the list property.  does not add to display geometry until dvp executes.
/// returns unique CE id
std::string CosmeticExtension::addCosmeticEdge(Base::Vector3d start,
                                               Base::Vector3d end)
{
//    Base::Console().Message("CEx::addCosmeticEdge(s, e)\n");
    std::vector<CosmeticEdge*> edges = CosmeticEdges.getValues();
    TechDraw::CosmeticEdge* ce = new TechDraw::CosmeticEdge(start, end);
    edges.push_back(ce);
    CosmeticEdges.setValues(edges);
    return ce->getTagAsString();
}

/// adds a new cosmetic edge to the list property.  does not add to display geometry until dvp executes.
/// returns unique CE id
std::string CosmeticExtension::addCosmeticEdge(TechDraw::BaseGeomPtr bg)
{
//    Base::Console().Message("CEx::addCosmeticEdge(bg: %X)\n", bg);
    std::vector<CosmeticEdge*> edges = CosmeticEdges.getValues();
    TechDraw::CosmeticEdge* ce = new TechDraw::CosmeticEdge(bg);
    edges.push_back(ce);
    CosmeticEdges.setValues(edges);
    return ce->getTagAsString();
}

/// retrieve a CE by unique id
TechDraw::CosmeticEdge* CosmeticExtension::getCosmeticEdge(const std::string& tagString) const
{
//    Base::Console().Message("CEx::getCosmeticEdge(%s)\n", tagString.c_str());
    const std::vector<TechDraw::CosmeticEdge*> edges = CosmeticEdges.getValues();
    for (auto& ce: edges) {
        std::string ceTag = ce->getTagAsString();
        if (ceTag == tagString) {
            return ce;
        }
    }

    // None found
//    Base::Console().Message("CEx::getCosmeticEdge - CE for tag: %s not found.\n", tagString.c_str());
    return nullptr;
}

/// find the cosmetic edge corresponding to selection name (Edge5)
/// used when selecting
TechDraw::CosmeticEdge* CosmeticExtension::getCosmeticEdgeBySelection(const std::string& name) const
{
//    Base::Console().Message("CEx::getCEBySelection(%s)\n", name.c_str());
    App::DocumentObject* extObj = const_cast<App::DocumentObject*> (getExtendedObject());
    TechDraw::DrawViewPart* dvp = dynamic_cast<TechDraw::DrawViewPart*>(extObj);
    if (!dvp) {
        return nullptr;
    }
    int idx = DrawUtil::getIndexFromName(name);
    TechDraw::BaseGeomPtr base = dvp->getGeomByIndex(idx);
    if (!base || base->getCosmeticTag().empty()) {
        return nullptr;
    }

    return getCosmeticEdge(base->getCosmeticTag());
}

/// find the cosmetic edge corresponding to the input parameter (the 5 in Edge5)
TechDraw::CosmeticEdge* CosmeticExtension::getCosmeticEdgeBySelection(int i) const
{
//    Base::Console().Message("CEx::getCEBySelection(%d)\n", i);
    std::stringstream edgeName;
    edgeName << "Edge" << i;
    return getCosmeticEdgeBySelection(edgeName.str());
}

/// remove the cosmetic edge with the given tag from the list property
void CosmeticExtension::removeCosmeticEdge(const std::string& delTag)
{
//    Base::Console().Message("DVP::removeCE(%s)\n", delTag.c_str());
    std::vector<CosmeticEdge*> cEdges = CosmeticEdges.getValues();
    std::vector<CosmeticEdge*> newEdges;
    for (auto& ce: cEdges) {
        if (ce->getTagAsString() == delTag)  {
            delete ce;
        } else {
            newEdges.push_back(ce);
        }
    }
    CosmeticEdges.setValues(newEdges);
}


/// remove the cosmetic edges with the given tags from the list property
void CosmeticExtension::removeCosmeticEdge(const std::vector<std::string>& delTags)
{
    for (auto& t: delTags) {
        removeCosmeticEdge(t);
    }
}

//********** Center Line *******************************************************

// this is never called!
void CosmeticExtension::clearCenterLines()
{
    std::vector<TechDraw::CenterLine*> cLines = CenterLines.getValues();
    for (auto& line : cLines) {
        delete line;
    }
    std::vector<CenterLine*> noLines;
    CenterLines.setValues(noLines);
}

int CosmeticExtension::add1CLToGE(const std::string& tag)
{
    //    Base::Console().Message("CEx::add1CLToGE(%s) 2\n", tag.c_str());
    TechDraw::CenterLine* cl = getCenterLine(tag);
    if (!cl) {
//        Base::Console().Message("CEx::add1CLToGE 2 - cl %s not found\n", tag.c_str());
        return -1;
    }
    TechDraw::BaseGeomPtr scaledGeom = cl->scaledAndRotatedGeometry(getOwner());
//    TechDraw::BaseGeomPtr scaledGeom = cl->scaledGeometry(getOwner());
    int iGE = getOwner()->getGeometryObject()->addCenterLine(scaledGeom, tag);

    return iGE;
}

//update Edge geometry with current CL's
void CosmeticExtension::refreshCLGeoms()
{
    //    Base::Console().Message("CE::refreshCLGeoms()\n");
    std::vector<TechDraw::BaseGeomPtr> gEdges = getOwner()->getEdgeGeometry();
    std::vector<TechDraw::BaseGeomPtr> newGEdges;
    for (auto& ge : gEdges) {
        if (ge->source() != SourceType::CENTERLINE) {
            newGEdges.push_back(ge);
        }
    }
    getOwner()->getGeometryObject()->setEdgeGeometry(newGEdges);
    addCenterLinesToGeom();
}

//add the center lines to geometry Edges list
void CosmeticExtension::addCenterLinesToGeom()
{
    //   Base::Console().Message("CE::addCenterLinesToGeom()\n");
    const std::vector<TechDraw::CenterLine*> lines = CenterLines.getValues();
    for (auto& cl : lines) {
//        TechDraw::BaseGeomPtr scaledGeom = cl->scaledGeometry(getOwner());
        TechDraw::BaseGeomPtr scaledGeom = cl->scaledAndRotatedGeometry(getOwner());
        if (!scaledGeom) {
            Base::Console().Error("CE::addCenterLinesToGeom - scaledGeometry is null\n");
            continue;
        }
        //        int idx =
        getOwner()->getGeometryObject()->addCenterLine(scaledGeom, cl->getTagAsString());
    }
}

//returns unique CL id
//only adds cl to cllist property.  does not add to display geometry until dvp executes.
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

std::string CosmeticExtension::addCenterLine(TechDraw::CenterLine* cl)
{
//    Base::Console().Message("CEx::addCenterLine(cl: %X)\n", cl);
    std::vector<CenterLine*> cLines = CenterLines.getValues();
    cLines.push_back(cl);
    CenterLines.setValues(cLines);
    return cl->getTagAsString();
}


std::string CosmeticExtension::addCenterLine(TechDraw::BaseGeomPtr bg)
{
//    Base::Console().Message("CEx::addCenterLine(bg: %X)\n", bg);
    std::vector<CenterLine*> cLines = CenterLines.getValues();
    TechDraw::CenterLine* cl = new TechDraw::CenterLine(bg);
    cLines.push_back(cl);
    CenterLines.setValues(cLines);
    return cl->getTagAsString();
}

//get CL by unique id
TechDraw::CenterLine* CosmeticExtension::getCenterLine(const std::string& tagString) const
{
//    Base::Console().Message("CEx::getCenterLine(%s)\n", tagString.c_str());
    const std::vector<TechDraw::CenterLine*> cLines = CenterLines.getValues();
    for (auto& cl: cLines) {
        std::string clTag = cl->getTagAsString();
        if (clTag == tagString) {
            return cl;
        }
    }
    return nullptr;
}

// find the center line corresponding to selection name (Edge5)
// used when selecting
TechDraw::CenterLine* CosmeticExtension::getCenterLineBySelection(const std::string& name) const
{
//    Base::Console().Message("CEx::getCLBySelection(%s)\n", name.c_str());
    App::DocumentObject* extObj = const_cast<App::DocumentObject*> (getExtendedObject());
    TechDraw::DrawViewPart* dvp = dynamic_cast<TechDraw::DrawViewPart*>(extObj);
    if (!dvp) {
        return nullptr;
    }
    int idx = DrawUtil::getIndexFromName(name);
    TechDraw::BaseGeomPtr base = dvp->getGeomByIndex(idx);
    if (!base || base->getCosmeticTag().empty()) {
        return nullptr;
    }
    return getCenterLine(base->getCosmeticTag());
}

//overload for index only
TechDraw::CenterLine* CosmeticExtension::getCenterLineBySelection(int i) const
{
//    Base::Console().Message("CEx::getCLBySelection(%d)\n", i);
    std::stringstream edgeName;
    edgeName << "Edge" << i;
    return getCenterLineBySelection(edgeName.str());
}

void CosmeticExtension::removeCenterLine(const std::string& delTag)
{
//    Base::Console().Message("DVP::removeCL(%s)\n", delTag.c_str());
    std::vector<CenterLine*> cLines = CenterLines.getValues();
    std::vector<CenterLine*> newLines;
    for (auto& cl: cLines) {
        if (cl->getTagAsString() == delTag)  {
            delete cl;
        } else {
            newLines.push_back(cl);
        }
    }
    CenterLines.setValues(newLines);
}

void CosmeticExtension::removeCenterLine(const std::vector<std::string>& delTags)
{
    for (auto& t: delTags) {
        removeCenterLine(t);
    }
}


//********** Geometry Formats **************************************************

void CosmeticExtension::clearGeomFormats()
{
    std::vector<GeomFormat*> noFormats;
    std::vector<GeomFormat*> fmts = GeomFormats.getValues();
    GeomFormats.setValues(noFormats);
    for (auto& f : fmts) {
        delete f;
    }
}

//returns unique GF id
//only adds gf to gflist property.  does not add to display geometry until dvp repaints.
std::string CosmeticExtension::addGeomFormat(TechDraw::GeomFormat* gf)
{
//    Base::Console().Message("CEx::addGeomFormat(gf: %X)\n", gf);
    std::vector<GeomFormat*> formats = GeomFormats.getValues();
    TechDraw::GeomFormat* newGF = new TechDraw::GeomFormat(gf);
    formats.push_back(newGF);
    GeomFormats.setValues(formats);
    return newGF->getTagAsString();
}


//get GF by unique id
TechDraw::GeomFormat* CosmeticExtension::getGeomFormat(const std::string& tagString) const
{
//    Base::Console().Message("CEx::getGeomFormat(%s)\n", tagString.c_str());
    const std::vector<TechDraw::GeomFormat*> formats = GeomFormats.getValues();
    for (auto& gf: formats) {
        std::string gfTag = gf->getTagAsString();
        if (gfTag == tagString) {
            return gf;
        }
    }

    // Nothing found
    return nullptr;
}

// find the cosmetic edge corresponding to selection name (Edge5)
// used when selecting
TechDraw::GeomFormat* CosmeticExtension::getGeomFormatBySelection(const std::string& name) const
{
//    Base::Console().Message("CEx::getCEBySelection(%s)\n", name.c_str());
    App::DocumentObject* extObj = const_cast<App::DocumentObject*> (getExtendedObject());
    TechDraw::DrawViewPart* dvp = dynamic_cast<TechDraw::DrawViewPart*>(extObj);
    if (!dvp) {
        return nullptr;
    }
    int idx = DrawUtil::getIndexFromName(name);
    const std::vector<TechDraw::GeomFormat*> formats = GeomFormats.getValues();
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

void CosmeticExtension::removeGeomFormat(const std::string& delTag)
{
//    Base::Console().Message("DVP::removeCE(%s)\n", delTag.c_str());
    std::vector<GeomFormat*> cFormats = GeomFormats.getValues();
    std::vector<GeomFormat*> newFormats;
    for (auto& gf: cFormats) {
        if (gf->getTagAsString() != delTag)  {
            newFormats.push_back(gf);
        }
    }
    GeomFormats.setValues(newFormats);
}

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


