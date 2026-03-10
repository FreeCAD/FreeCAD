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

#pragma once

#include <App/DocumentObjectExtension.h>
#include <App/ExtensionPython.h>
#include <Base/Vector3D.h>
#include <Mod/TechDraw/TechDrawGlobal.h>

#include "Geometry.h"
#include "PropertyCenterLineList.h"
#include "PropertyCosmeticEdgeList.h"
#include "PropertyCosmeticVertexList.h"
#include "PropertyGeomFormatList.h"


namespace TechDraw {
class DrawViewPart;
class GeometryObject;

class TechDrawExport CosmeticExtension : public App::DocumentObjectExtension {
    EXTENSION_PROPERTY_HEADER_WITH_OVERRIDE(TechDraw::CosmeticObject);

public:
    CosmeticExtension();
    ~CosmeticExtension() override;

    TechDraw::PropertyCosmeticVertexList CosmeticVertexes;
    TechDraw::PropertyCosmeticEdgeList   CosmeticEdges;
    TechDraw::PropertyCenterLineList     CenterLines;
    TechDraw::PropertyGeomFormatList     GeomFormats;          //formats for geometric edges

    virtual CosmeticVertex* getCosmeticVertexBySelection(const int i) const;
    virtual CosmeticVertex* getCosmeticVertexBySelection(const std::string& name) const;
    virtual CosmeticVertex* getCosmeticVertex(const std::string& tag) const;
    virtual int             add1CVToGV(const std::string& tag);
    virtual int             getCVIndex(const std::string& tag);
    virtual std::string     addCosmeticVertex(const Base::Vector3d& pos, bool invert = true);
    virtual void            addCosmeticVertexesToGeom();
    virtual void            clearCosmeticVertexes();
    virtual void            refreshCVGeoms();
    virtual void            removeCosmeticVertex(const std::string& tag);
    virtual void            removeCosmeticVertex(const std::vector<std::string>& tags);

    virtual std::string     addCosmeticEdge(Base::Vector3d start, Base::Vector3d end);
    virtual std::string     addCosmeticEdge(TechDraw::BaseGeomPtr bg);
    virtual CosmeticEdge*   getCosmeticEdgeBySelection(const std::string& name) const;
    virtual CosmeticEdge*   getCosmeticEdgeBySelection(int i) const;
    virtual CosmeticEdge*   getCosmeticEdge(const std::string& id) const;
    virtual void            removeCosmeticEdge(const std::string& tag);
    virtual void            removeCosmeticEdge(const std::vector<std::string>& delTags);
    virtual void            clearCosmeticEdges();
    virtual int             add1CEToGE(const std::string& tag);
    virtual void            addCosmeticEdgesToGeom();
    virtual void            refreshCEGeoms();

    virtual void refreshCLGeoms();
    virtual void addCenterLinesToGeom();
    virtual int add1CLToGE(const std::string &tag);
    virtual std::string     addCenterLine(Base::Vector3d start, Base::Vector3d end);
    virtual std::string     addCenterLine(TechDraw::CenterLine* cl);
    virtual std::string     addCenterLine(TechDraw::BaseGeomPtr bg);
    virtual CenterLine*     getCenterLineBySelection(const std::string& name) const;
    virtual CenterLine*     getCenterLineBySelection(int i) const;
    virtual CenterLine*     getCenterLine(const std::string& tag) const;
    virtual void            removeCenterLine(const std::string& tag);
    virtual void            removeCenterLine(const std::vector<std::string>& delTags);
    virtual void            clearCenterLines();

    virtual std::string     addGeomFormat(TechDraw::GeomFormat* gf);
    virtual GeomFormat*     getGeomFormatBySelection(const std::string& name) const;
    virtual GeomFormat*     getGeomFormatBySelection(int i) const;
    virtual GeomFormat*     getGeomFormat(const std::string& id) const;
    virtual void            removeGeomFormat(const std::string& tag);
    virtual void            clearGeomFormats();

    void deleteCosmeticElements(std::vector<std::string> removables);

    TechDraw::DrawViewPart* getOwner();

    PyObject* getExtensionPyObject() override;

private:

};

using CosmeticExtensionPython = App::ExtensionPythonT<CosmeticExtension>;

} //end namespace TechDraw