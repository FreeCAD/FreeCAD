/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#include <Inventor/nodes/SoBaseColor.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoFaceSet.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoMarkerSet.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoShapeHints.h>
#endif

#include <App/Application.h>
#include <Base/Parameter.h>
#include <Gui/Inventor/MarkerBitmaps.h>
#include <Mod/Mesh/App/MeshFeature.h>
#include <Mod/Mesh/App/Core/Degeneration.h>
#include <Mod/Mesh/App/Core/Iterator.h>

#include "ViewProviderDefects.h"


using namespace Mesh;
using namespace MeshGui;


PROPERTY_SOURCE_ABSTRACT(MeshGui::ViewProviderMeshDefects, Gui::ViewProviderDocumentObject)
PROPERTY_SOURCE(MeshGui::ViewProviderMeshOrientation, MeshGui::ViewProviderMeshDefects)
PROPERTY_SOURCE(MeshGui::ViewProviderMeshNonManifolds, MeshGui::ViewProviderMeshDefects)
PROPERTY_SOURCE(MeshGui::ViewProviderMeshNonManifoldPoints, MeshGui::ViewProviderMeshDefects)
PROPERTY_SOURCE(MeshGui::ViewProviderMeshDuplicatedFaces, MeshGui::ViewProviderMeshDefects)
PROPERTY_SOURCE(MeshGui::ViewProviderMeshDuplicatedPoints, MeshGui::ViewProviderMeshDefects)
PROPERTY_SOURCE(MeshGui::ViewProviderMeshDegenerations, MeshGui::ViewProviderMeshDefects)
PROPERTY_SOURCE(MeshGui::ViewProviderMeshIndices, MeshGui::ViewProviderMeshDefects)
PROPERTY_SOURCE(MeshGui::ViewProviderMeshSelfIntersections, MeshGui::ViewProviderMeshDefects)
PROPERTY_SOURCE(MeshGui::ViewProviderMeshFolds, MeshGui::ViewProviderMeshDefects)

ViewProviderMeshDefects::ViewProviderMeshDefects()
{
    ADD_PROPERTY(LineWidth, (2.0f));

    pcCoords = new SoCoordinate3();
    pcCoords->ref();
    pcDrawStyle = new SoDrawStyle();
    pcDrawStyle->ref();
    pcDrawStyle->style = SoDrawStyle::LINES;
    pcDrawStyle->lineWidth = LineWidth.getValue();
}

ViewProviderMeshDefects::~ViewProviderMeshDefects()
{
    pcCoords->unref();
    pcDrawStyle->unref();
}

void ViewProviderMeshDefects::onChanged(const App::Property* prop)
{
    if (prop == &LineWidth) {
        pcDrawStyle->lineWidth = LineWidth.getValue();
    }
    // Visibility changes must be handled here because in the base class it changes the attribute of
    // the feature and thus affects the visibility of the mesh view provider which is undesired
    // behaviour
    else if (prop == &Visibility) {
        Visibility.getValue() ? show() : hide();
    }
    else {
        ViewProviderDocumentObject::onChanged(prop);
    }
}

// ----------------------------------------------------------------------

ViewProviderMeshOrientation::ViewProviderMeshOrientation()
{
    // NOLINTBEGIN
    pcFaces = new SoFaceSet;
    pcFaces->ref();
    // NOLINTEND
}

ViewProviderMeshOrientation::~ViewProviderMeshOrientation()
{
    pcFaces->unref();
}

void ViewProviderMeshOrientation::attach(App::DocumentObject* pcFeat)
{
    ViewProviderDocumentObject::attach(pcFeat);

    SoGroup* pcFaceRoot = new SoGroup();

    SoDrawStyle* pcFlatStyle = new SoDrawStyle();
    pcFlatStyle->style = SoDrawStyle::FILLED;
    pcFaceRoot->addChild(pcFlatStyle);

    SoShapeHints* flathints = new SoShapeHints;
    flathints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    flathints->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;
    pcFaceRoot->addChild(flathints);

    // Draw faces
    SoSeparator* linesep = new SoSeparator;
    SoBaseColor* basecol = new SoBaseColor;
    basecol->rgb.setValue(1.0f, 0.5f, 0.0f);
    linesep->addChild(basecol);
    linesep->addChild(pcCoords);
    linesep->addChild(pcFaces);

    // Draw markers
    SoBaseColor* markcol = new SoBaseColor;
    markcol->rgb.setValue(1.0f, 1.0f, 0.0f);
    SoMarkerSet* marker = new SoMarkerSet;
    marker->markerIndex = Gui::Inventor::MarkerBitmaps::getMarkerIndex(
        "PLUS",
        App::GetApplication()
            .GetParameterGroupByPath("User parameter:BaseApp/Preferences/View")
            ->GetInt("MarkerSize", 7));
    linesep->addChild(markcol);
    linesep->addChild(marker);

    pcFaceRoot->addChild(linesep);

    addDisplayMaskMode(pcFaceRoot, "Face");
}

void ViewProviderMeshOrientation::showDefects(const std::vector<Mesh::ElementIndex>& inds)
{
    Mesh::Feature* f = static_cast<Mesh::Feature*>(pcObject);
    const MeshCore::MeshKernel& rMesh = f->Mesh.getValue().getKernel();

    pcCoords->point.deleteValues(0);
    pcCoords->point.setNum(3 * inds.size());
    MeshCore::MeshFacetIterator cF(rMesh);
    int i = 0;
    int j = 0;
    for (Mesh::ElementIndex ind : inds) {
        cF.Set(ind);
        for (auto cP : cF->_aclPoints) {
            // move a bit in opposite normal direction to overlay the original faces
            cP -= 0.001f * cF->GetNormal();
            pcCoords->point.set1Value(i++, cP.x, cP.y, cP.z);
        }
        pcFaces->numVertices.set1Value(j++, 3);
    }

    setDisplayMaskMode("Face");
}

// ----------------------------------------------------------------------

ViewProviderMeshNonManifolds::ViewProviderMeshNonManifolds()
{
    // NOLINTBEGIN
    pcLines = new SoLineSet;
    pcLines->ref();
    // NOLINTEND
}

ViewProviderMeshNonManifolds::~ViewProviderMeshNonManifolds()
{
    pcLines->unref();
}

void ViewProviderMeshNonManifolds::attach(App::DocumentObject* pcFeat)
{
    ViewProviderDocumentObject::attach(pcFeat);

    SoGroup* pcLineRoot = new SoGroup();
    pcDrawStyle->lineWidth = 3;
    pcLineRoot->addChild(pcDrawStyle);

    // Draw lines
    SoSeparator* linesep = new SoSeparator;
    SoBaseColor* basecol = new SoBaseColor;
    basecol->rgb.setValue(1.0f, 0.0f, 0.0f);
    linesep->addChild(basecol);
    linesep->addChild(pcCoords);
    linesep->addChild(pcLines);
    pcLineRoot->addChild(linesep);

    // Draw markers
    SoBaseColor* markcol = new SoBaseColor;
    markcol->rgb.setValue(1.0f, 1.0f, 0.0f);
    SoMarkerSet* marker = new SoMarkerSet;
    marker->markerIndex = Gui::Inventor::MarkerBitmaps::getMarkerIndex(
        "PLUS",
        App::GetApplication()
            .GetParameterGroupByPath("User parameter:BaseApp/Preferences/View")
            ->GetInt("MarkerSize", 7));
    linesep->addChild(markcol);
    linesep->addChild(marker);

    addDisplayMaskMode(pcLineRoot, "Line");
}

void ViewProviderMeshNonManifolds::showDefects(const std::vector<Mesh::ElementIndex>& inds)
{
    if ((inds.size() % 2) != 0) {
        return;
    }
    Mesh::Feature* f = static_cast<Mesh::Feature*>(pcObject);
    const MeshCore::MeshKernel& rMesh = f->Mesh.getValue().getKernel();

    pcCoords->point.deleteValues(0);
    pcCoords->point.setNum(inds.size());
    MeshCore::MeshPointIterator cP(rMesh);
    int i = 0;
    int j = 0;
    for (std::vector<Mesh::ElementIndex>::const_iterator it = inds.begin(); it != inds.end();
         ++it) {
        cP.Set(*it);
        pcCoords->point.set1Value(i++, cP->x, cP->y, cP->z);
        ++it;  // go to end point
        cP.Set(*it);
        pcCoords->point.set1Value(i++, cP->x, cP->y, cP->z);
        pcLines->numVertices.set1Value(j++, 2);
    }

    setDisplayMaskMode("Line");
}

// ----------------------------------------------------------------------

ViewProviderMeshNonManifoldPoints::ViewProviderMeshNonManifoldPoints()
{
    // NOLINTBEGIN
    pcPoints = new SoPointSet;
    pcPoints->ref();
    // NOLINTEND
}

ViewProviderMeshNonManifoldPoints::~ViewProviderMeshNonManifoldPoints()
{
    pcPoints->unref();
}

void ViewProviderMeshNonManifoldPoints::attach(App::DocumentObject* pcFeat)
{
    ViewProviderDocumentObject::attach(pcFeat);

    SoGroup* pcPointRoot = new SoGroup();
    pcDrawStyle->pointSize = 3;
    pcPointRoot->addChild(pcDrawStyle);

    // Draw points
    SoSeparator* pointsep = new SoSeparator;
    SoBaseColor* basecol = new SoBaseColor;
    basecol->rgb.setValue(1.0f, 0.5f, 0.0f);
    pointsep->addChild(basecol);
    pointsep->addChild(pcCoords);
    pointsep->addChild(pcPoints);
    pcPointRoot->addChild(pointsep);

    // Draw markers
    SoBaseColor* markcol = new SoBaseColor;
    markcol->rgb.setValue(1.0f, 1.0f, 0.0f);
    SoMarkerSet* marker = new SoMarkerSet;
    marker->markerIndex = Gui::Inventor::MarkerBitmaps::getMarkerIndex(
        "PLUS",
        App::GetApplication()
            .GetParameterGroupByPath("User parameter:BaseApp/Preferences/View")
            ->GetInt("MarkerSize", 7));
    pointsep->addChild(markcol);
    pointsep->addChild(marker);

    addDisplayMaskMode(pcPointRoot, "Point");
}

void ViewProviderMeshNonManifoldPoints::showDefects(const std::vector<Mesh::ElementIndex>& inds)
{
    Mesh::Feature* f = static_cast<Mesh::Feature*>(pcObject);
    const MeshCore::MeshKernel& rMesh = f->Mesh.getValue().getKernel();
    pcCoords->point.deleteValues(0);
    pcCoords->point.setNum(inds.size());
    MeshCore::MeshPointIterator cP(rMesh);
    int i = 0;
    for (Mesh::ElementIndex ind : inds) {
        cP.Set(ind);
        pcCoords->point.set1Value(i++, cP->x, cP->y, cP->z);
    }

    setDisplayMaskMode("Point");
}

// ----------------------------------------------------------------------

ViewProviderMeshDuplicatedFaces::ViewProviderMeshDuplicatedFaces()
{
    // NOLINTBEGIN
    pcFaces = new SoFaceSet;
    pcFaces->ref();
    // NOLINTEND
}

ViewProviderMeshDuplicatedFaces::~ViewProviderMeshDuplicatedFaces()
{
    pcFaces->unref();
}

void ViewProviderMeshDuplicatedFaces::attach(App::DocumentObject* pcFeat)
{
    ViewProviderDocumentObject::attach(pcFeat);

    SoGroup* pcFaceRoot = new SoGroup();

    SoDrawStyle* pcFlatStyle = new SoDrawStyle();
    pcFlatStyle->style = SoDrawStyle::FILLED;
    pcFaceRoot->addChild(pcFlatStyle);

    SoShapeHints* flathints = new SoShapeHints;
    flathints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    flathints->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;
    pcFaceRoot->addChild(flathints);

    // Draw lines
    SoSeparator* linesep = new SoSeparator;
    SoBaseColor* basecol = new SoBaseColor;
    basecol->rgb.setValue(1.0f, 0.0f, 0.0f);
    linesep->addChild(basecol);
    linesep->addChild(pcCoords);
    linesep->addChild(pcFaces);
    pcFaceRoot->addChild(linesep);

    // Draw markers
    SoBaseColor* markcol = new SoBaseColor;
    markcol->rgb.setValue(1.0f, 1.0f, 0.0f);
    SoMarkerSet* marker = new SoMarkerSet;
    marker->markerIndex = Gui::Inventor::MarkerBitmaps::getMarkerIndex(
        "PLUS",
        App::GetApplication()
            .GetParameterGroupByPath("User parameter:BaseApp/Preferences/View")
            ->GetInt("MarkerSize", 7));
    linesep->addChild(markcol);
    linesep->addChild(marker);

    addDisplayMaskMode(pcFaceRoot, "Face");
}

void ViewProviderMeshDuplicatedFaces::showDefects(const std::vector<Mesh::ElementIndex>& inds)
{
    Mesh::Feature* f = static_cast<Mesh::Feature*>(pcObject);
    const MeshCore::MeshKernel& rMesh = f->Mesh.getValue().getKernel();

    pcCoords->point.deleteValues(0);
    pcCoords->point.setNum(3 * inds.size());
    MeshCore::MeshFacetIterator cF(rMesh);
    int i = 0;
    int j = 0;
    for (Mesh::ElementIndex ind : inds) {
        cF.Set(ind);
        for (auto cP : cF->_aclPoints) {
            // move a bit in normal direction to overlay the original faces
            cP += 0.001f * cF->GetNormal();
            pcCoords->point.set1Value(i++, cP.x, cP.y, cP.z);
        }
        pcFaces->numVertices.set1Value(j++, 3);
    }

    setDisplayMaskMode("Face");
}

// ----------------------------------------------------------------------

ViewProviderMeshDuplicatedPoints::ViewProviderMeshDuplicatedPoints()
{
    // NOLINTBEGIN
    pcPoints = new SoPointSet;
    pcPoints->ref();
    // NOLINTEND
}

ViewProviderMeshDuplicatedPoints::~ViewProviderMeshDuplicatedPoints()
{
    pcPoints->unref();
}

void ViewProviderMeshDuplicatedPoints::attach(App::DocumentObject* pcFeat)
{
    ViewProviderDocumentObject::attach(pcFeat);

    SoGroup* pcPointRoot = new SoGroup();
    pcDrawStyle->pointSize = 3;
    pcPointRoot->addChild(pcDrawStyle);

    // Draw points
    SoSeparator* pointsep = new SoSeparator;
    SoBaseColor* basecol = new SoBaseColor;
    basecol->rgb.setValue(1.0f, 0.5f, 0.0f);
    pointsep->addChild(basecol);
    pointsep->addChild(pcCoords);
    pointsep->addChild(pcPoints);
    pcPointRoot->addChild(pointsep);

    // Draw markers
    SoBaseColor* markcol = new SoBaseColor;
    markcol->rgb.setValue(1.0f, 1.0f, 0.0f);
    SoMarkerSet* marker = new SoMarkerSet;
    marker->markerIndex = Gui::Inventor::MarkerBitmaps::getMarkerIndex(
        "PLUS",
        App::GetApplication()
            .GetParameterGroupByPath("User parameter:BaseApp/Preferences/View")
            ->GetInt("MarkerSize", 7));
    pointsep->addChild(markcol);
    pointsep->addChild(marker);

    addDisplayMaskMode(pcPointRoot, "Point");
}

void ViewProviderMeshDuplicatedPoints::showDefects(const std::vector<Mesh::ElementIndex>& inds)
{
    Mesh::Feature* f = static_cast<Mesh::Feature*>(pcObject);
    const MeshCore::MeshKernel& rMesh = f->Mesh.getValue().getKernel();
    pcCoords->point.deleteValues(0);
    pcCoords->point.setNum(inds.size());
    MeshCore::MeshPointIterator cP(rMesh);
    int i = 0;
    for (Mesh::ElementIndex ind : inds) {
        cP.Set(ind);
        pcCoords->point.set1Value(i++, cP->x, cP->y, cP->z);
    }

    setDisplayMaskMode("Point");
}

// ----------------------------------------------------------------------

ViewProviderMeshDegenerations::ViewProviderMeshDegenerations()
{
    // NOLINTBEGIN
    pcLines = new SoLineSet;
    pcLines->ref();
    // NOLINTEND
}

ViewProviderMeshDegenerations::~ViewProviderMeshDegenerations()
{
    pcLines->unref();
}

void ViewProviderMeshDegenerations::attach(App::DocumentObject* pcFeat)
{
    ViewProviderDocumentObject::attach(pcFeat);

    SoGroup* pcLineRoot = new SoGroup();
    pcDrawStyle->lineWidth = 3;
    pcLineRoot->addChild(pcDrawStyle);

    // Draw lines
    SoSeparator* linesep = new SoSeparator;
    SoBaseColor* basecol = new SoBaseColor;
    basecol->rgb.setValue(1.0f, 0.5f, 0.0f);
    linesep->addChild(basecol);
    linesep->addChild(pcCoords);
    linesep->addChild(pcLines);
    pcLineRoot->addChild(linesep);

    // Draw markers
    SoBaseColor* markcol = new SoBaseColor;
    markcol->rgb.setValue(1.0f, 1.0f, 0.0f);
    SoMarkerSet* marker = new SoMarkerSet;
    marker->markerIndex = Gui::Inventor::MarkerBitmaps::getMarkerIndex(
        "PLUS",
        App::GetApplication()
            .GetParameterGroupByPath("User parameter:BaseApp/Preferences/View")
            ->GetInt("MarkerSize", 7));
    linesep->addChild(markcol);
    linesep->addChild(marker);

    addDisplayMaskMode(pcLineRoot, "Line");
}

void ViewProviderMeshDegenerations::showDefects(const std::vector<Mesh::ElementIndex>& inds)
{
    Mesh::Feature* f = static_cast<Mesh::Feature*>(pcObject);
    const MeshCore::MeshKernel& rMesh = f->Mesh.getValue().getKernel();

    pcCoords->point.deleteValues(0);
    pcCoords->point.setNum(2 * inds.size());
    MeshCore::MeshFacetIterator cF(rMesh);
    int i = 0;
    int j = 0;
    for (Mesh::ElementIndex ind : inds) {
        cF.Set(ind);
        const MeshCore::MeshPoint& rE0 = cF->_aclPoints[0];
        const MeshCore::MeshPoint& rE1 = cF->_aclPoints[1];
        const MeshCore::MeshPoint& rE2 = cF->_aclPoints[2];

        // check if the points are coincident
        if (rE0 == rE1 && rE0 == rE2) {
            // set a small tolerance to get a non-degenerated line
            float eps = 0.005f;
            Base::Vector3f cP1, cP2;
            cP1.Set(rE1.x + eps, rE1.y + eps, rE1.z + eps);
            cP2.Set(rE2.x - eps, rE2.y - eps, rE2.z - eps);
            pcCoords->point.set1Value(i++, cP1.x, cP1.y, cP1.z);
            pcCoords->point.set1Value(i++, cP2.x, cP2.y, cP2.z);
        }
        else if (rE0 == rE1) {
            pcCoords->point.set1Value(i++, rE1.x, rE1.y, rE1.z);
            pcCoords->point.set1Value(i++, rE2.x, rE2.y, rE2.z);
        }
        else if (rE1 == rE2) {
            pcCoords->point.set1Value(i++, rE2.x, rE2.y, rE2.z);
            pcCoords->point.set1Value(i++, rE0.x, rE0.y, rE0.z);
        }
        else if (rE2 == rE0) {
            pcCoords->point.set1Value(i++, rE0.x, rE0.y, rE0.z);
            pcCoords->point.set1Value(i++, rE1.x, rE1.y, rE1.z);
        }
        else {
            for (int j = 0; j < 3; j++) {
                Base::Vector3f cVec1 = cF->_aclPoints[(j + 1) % 3] - cF->_aclPoints[j];
                Base::Vector3f cVec2 = cF->_aclPoints[(j + 2) % 3] - cF->_aclPoints[j];

                // adjust the neighbourhoods and point indices
                if (cVec1 * cVec2 < 0.0f) {
                    pcCoords->point.set1Value(i++,
                                              cF->_aclPoints[(j + 1) % 3].x,
                                              cF->_aclPoints[(j + 1) % 3].y,
                                              cF->_aclPoints[(j + 1) % 3].z);
                    pcCoords->point.set1Value(i++,
                                              cF->_aclPoints[(j + 2) % 3].x,
                                              cF->_aclPoints[(j + 2) % 3].y,
                                              cF->_aclPoints[(j + 2) % 3].z);
                    break;
                }
            }
        }

        pcLines->numVertices.set1Value(j++, 2);
    }

    setDisplayMaskMode("Line");
}

// ----------------------------------------------------------------------

ViewProviderMeshIndices::ViewProviderMeshIndices()
{
    // NOLINTBEGIN
    pcFaces = new SoFaceSet;
    pcFaces->ref();
    // NOLINTEND
}

ViewProviderMeshIndices::~ViewProviderMeshIndices()
{
    pcFaces->unref();
}

void ViewProviderMeshIndices::attach(App::DocumentObject* pcFeat)
{
    ViewProviderDocumentObject::attach(pcFeat);

    SoGroup* pcFaceRoot = new SoGroup();

    SoDrawStyle* pcFlatStyle = new SoDrawStyle();
    pcFlatStyle->style = SoDrawStyle::FILLED;
    pcFaceRoot->addChild(pcFlatStyle);

    SoShapeHints* flathints = new SoShapeHints;
    flathints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    flathints->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;
    pcFaceRoot->addChild(flathints);

    // Draw lines
    SoSeparator* linesep = new SoSeparator;
    SoBaseColor* basecol = new SoBaseColor;
    basecol->rgb.setValue(1.0f, 0.5f, 0.0f);
    linesep->addChild(basecol);
    linesep->addChild(pcCoords);
    linesep->addChild(pcFaces);
    pcFaceRoot->addChild(linesep);

    // Draw markers
    SoBaseColor* markcol = new SoBaseColor;
    markcol->rgb.setValue(1.0f, 1.0f, 0.0f);
    SoMarkerSet* marker = new SoMarkerSet;
    marker->markerIndex = Gui::Inventor::MarkerBitmaps::getMarkerIndex(
        "PLUS",
        App::GetApplication()
            .GetParameterGroupByPath("User parameter:BaseApp/Preferences/View")
            ->GetInt("MarkerSize", 7));
    linesep->addChild(markcol);
    linesep->addChild(marker);

    addDisplayMaskMode(pcFaceRoot, "Face");
}

void ViewProviderMeshIndices::showDefects(const std::vector<Mesh::ElementIndex>& inds)
{
    Mesh::Feature* f = static_cast<Mesh::Feature*>(pcObject);
    const MeshCore::MeshKernel& rMesh = f->Mesh.getValue().getKernel();

    if (!inds.empty()) {
        pcCoords->point.deleteValues(0);
        pcCoords->point.setNum(3 * inds.size());
        MeshCore::MeshFacetIterator cF(rMesh);
        int i = 0;
        int j = 0;
        for (Mesh::ElementIndex ind : inds) {
            cF.Set(ind);
            for (auto cP : cF->_aclPoints) {
                // move a bit in opposite normal direction to overlay the original faces
                cP -= 0.001f * cF->GetNormal();
                pcCoords->point.set1Value(i++, cP.x, cP.y, cP.z);
            }
            pcFaces->numVertices.set1Value(j++, 3);
        }

        setDisplayMaskMode("Face");
    }
}

// ----------------------------------------------------------------------

ViewProviderMeshSelfIntersections::ViewProviderMeshSelfIntersections()
{
    // NOLINTBEGIN
    pcLines = new SoLineSet;
    pcLines->ref();
    // NOLINTEND
}

ViewProviderMeshSelfIntersections::~ViewProviderMeshSelfIntersections()
{
    pcLines->unref();
}

void ViewProviderMeshSelfIntersections::attach(App::DocumentObject* pcFeat)
{
    ViewProviderDocumentObject::attach(pcFeat);

    SoGroup* pcLineRoot = new SoGroup();
    pcDrawStyle->lineWidth = 3;
    pcLineRoot->addChild(pcDrawStyle);

    // Draw lines
    SoSeparator* linesep = new SoSeparator;
    SoBaseColor* basecol = new SoBaseColor;
    basecol->rgb.setValue(1.0f, 0.5f, 0.0f);
    linesep->addChild(basecol);
    linesep->addChild(pcCoords);
    linesep->addChild(pcLines);
    pcLineRoot->addChild(linesep);

    // Draw markers
    SoBaseColor* markcol = new SoBaseColor;
    markcol->rgb.setValue(1.0f, 1.0f, 0.0f);
    SoMarkerSet* marker = new SoMarkerSet;
    marker->markerIndex = Gui::Inventor::MarkerBitmaps::getMarkerIndex(
        "PLUS",
        App::GetApplication()
            .GetParameterGroupByPath("User parameter:BaseApp/Preferences/View")
            ->GetInt("MarkerSize", 7));
    linesep->addChild(markcol);
    linesep->addChild(marker);

    addDisplayMaskMode(pcLineRoot, "Line");
}

void ViewProviderMeshSelfIntersections::showDefects(const std::vector<Mesh::ElementIndex>& indices)
{
    if (indices.size() % 2 != 0) {
        return;
    }
    Mesh::Feature* f = static_cast<Mesh::Feature*>(pcObject);
    const MeshCore::MeshKernel& rMesh = f->Mesh.getValue().getKernel();
    MeshCore::MeshEvalSelfIntersection eval(rMesh);

    std::vector<std::pair<Mesh::ElementIndex, Mesh::ElementIndex>> intersection;
    std::vector<Mesh::ElementIndex>::const_iterator it;
    for (it = indices.begin(); it != indices.end();) {
        Mesh::ElementIndex id1 = *it;
        ++it;
        Mesh::ElementIndex id2 = *it;
        ++it;
        intersection.emplace_back(id1, id2);
    }

    std::vector<std::pair<Base::Vector3f, Base::Vector3f>> lines;
    eval.GetIntersections(intersection, lines);

    pcCoords->point.deleteValues(0);
    pcCoords->point.setNum(2 * lines.size());
    int i = 0;
    int j = 0;
    for (const auto& line : lines) {
        pcCoords->point.set1Value(i++, line.first.x, line.first.y, line.first.z);
        pcCoords->point.set1Value(i++, line.second.x, line.second.y, line.second.z);
        pcLines->numVertices.set1Value(j++, 2);
    }

    setDisplayMaskMode("Line");
}

// ----------------------------------------------------------------------

ViewProviderMeshFolds::ViewProviderMeshFolds()
{
    // NOLINTBEGIN
    pcFaces = new SoFaceSet;
    pcFaces->ref();
    // NOLINTEND
}

ViewProviderMeshFolds::~ViewProviderMeshFolds()
{
    pcFaces->unref();
}

void ViewProviderMeshFolds::attach(App::DocumentObject* pcFeat)
{
    ViewProviderDocumentObject::attach(pcFeat);

    SoGroup* pcFaceRoot = new SoGroup();

    SoDrawStyle* pcFlatStyle = new SoDrawStyle();
    pcFlatStyle->style = SoDrawStyle::FILLED;
    pcFaceRoot->addChild(pcFlatStyle);

    SoShapeHints* flathints = new SoShapeHints;
    flathints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    flathints->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;
    pcFaceRoot->addChild(flathints);

    // Draw lines
    SoSeparator* linesep = new SoSeparator;
    SoBaseColor* basecol = new SoBaseColor;
    basecol->rgb.setValue(1.0f, 0.0f, 0.0f);
    linesep->addChild(basecol);
    linesep->addChild(pcCoords);
    linesep->addChild(pcFaces);
    pcFaceRoot->addChild(linesep);

    // Draw markers
    SoBaseColor* markcol = new SoBaseColor;
    markcol->rgb.setValue(1.0f, 1.0f, 0.0f);
    SoMarkerSet* marker = new SoMarkerSet;
    marker->markerIndex = Gui::Inventor::MarkerBitmaps::getMarkerIndex(
        "PLUS",
        App::GetApplication()
            .GetParameterGroupByPath("User parameter:BaseApp/Preferences/View")
            ->GetInt("MarkerSize", 7));
    linesep->addChild(markcol);
    linesep->addChild(marker);

    addDisplayMaskMode(pcFaceRoot, "Face");
}

void ViewProviderMeshFolds::showDefects(const std::vector<Mesh::ElementIndex>& inds)
{
    Mesh::Feature* f = static_cast<Mesh::Feature*>(pcObject);
    const MeshCore::MeshKernel& rMesh = f->Mesh.getValue().getKernel();

    pcCoords->point.deleteValues(0);
    pcCoords->point.setNum(3 * inds.size());
    MeshCore::MeshFacetIterator cF(rMesh);
    int i = 0;
    int j = 0;
    for (Mesh::ElementIndex ind : inds) {
        cF.Set(ind);
        for (auto cP : cF->_aclPoints) {
            // move a bit in normal direction to overlay the original faces
            cP += 0.001f * cF->GetNormal();
            pcCoords->point.set1Value(i++, cP.x, cP.y, cP.z);
        }
        pcFaces->numVertices.set1Value(j++, 3);
    }

    setDisplayMaskMode("Face");
}
