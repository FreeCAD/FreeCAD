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

#ifndef MESHGUI_VIEWPROVIDER_MESH_DEFECTS_H
#define MESHGUI_VIEWPROVIDER_MESH_DEFECTS_H

#include "ViewProvider.h"

class SoCoordinate3;
class SoPointSet;
class SoLineSet;
class SoFaceSet;

namespace MeshGui
{

/** The ViewProviderMeshDefects class is used to display the most known types of defects of a
 * polymesh. In subclasses defects like non-manifolds, wrong oriented facets, degenerated facets,
 * duplicates, .... are displayed.
 * @author Werner Mayer
 */
class MeshGuiExport ViewProviderMeshDefects: public Gui::ViewProviderDocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(MeshGui::ViewProviderMeshDefects);

public:
    ViewProviderMeshDefects();
    ~ViewProviderMeshDefects() override;

    // NOLINTBEGIN
    // Display properties
    App::PropertyFloat LineWidth;
    // NOLINTEND

    // Build up the initial Inventor node
    void attach(App::DocumentObject* pcFeature) override = 0;
    /// Fill up the Inventor node with data
    virtual void showDefects(const std::vector<Mesh::ElementIndex>&) = 0;

protected:
    /// get called by the container whenever a property has been changed
    void onChanged(const App::Property* prop) override;

protected:
    // NOLINTBEGIN
    SoCoordinate3* pcCoords;
    SoDrawStyle* pcDrawStyle;
    // NOLINTEND
};

/** The ViewProviderMeshOrientation class displays wrong oriented facets (i.e. flipped normals) in
 * orange.
 * @author Werner Mayer
 */
class MeshGuiExport ViewProviderMeshOrientation: public ViewProviderMeshDefects
{
    PROPERTY_HEADER_WITH_OVERRIDE(MeshGui::ViewProviderMeshOrientation);

public:
    ViewProviderMeshOrientation();
    ~ViewProviderMeshOrientation() override;

    void attach(App::DocumentObject* pcFeature) override;
    void showDefects(const std::vector<Mesh::ElementIndex>&) override;

private:
    SoFaceSet* pcFaces;
};

/** The ViewProviderMeshNonManifolds class displays edges with more than two faces attached in red.
 * @author Werner Mayer
 */
class MeshGuiExport ViewProviderMeshNonManifolds: public ViewProviderMeshDefects
{
    PROPERTY_HEADER_WITH_OVERRIDE(MeshGui::ViewProviderMeshNonManifolds);

public:
    ViewProviderMeshNonManifolds();
    ~ViewProviderMeshNonManifolds() override;

    void attach(App::DocumentObject* pcFeature) override;
    void showDefects(const std::vector<Mesh::ElementIndex>&) override;

private:
    SoLineSet* pcLines;
};

/** The ViewProviderMeshNonManifoldPoints class displays non-manifold vertexes in red.
 * @author Werner Mayer
 */
class MeshGuiExport ViewProviderMeshNonManifoldPoints: public ViewProviderMeshDefects
{
    PROPERTY_HEADER_WITH_OVERRIDE(MeshGui::ViewProviderMeshNonManifoldPoints);

public:
    ViewProviderMeshNonManifoldPoints();
    ~ViewProviderMeshNonManifoldPoints() override;

    void attach(App::DocumentObject* pcFeature) override;
    void showDefects(const std::vector<Mesh::ElementIndex>&) override;

private:
    SoPointSet* pcPoints;
};

/** The ViewProviderMeshDuplicatedFaces class displays duplicated faces in red.
 * @author Werner Mayer
 */
class MeshGuiExport ViewProviderMeshDuplicatedFaces: public ViewProviderMeshDefects
{
    PROPERTY_HEADER_WITH_OVERRIDE(MeshGui::ViewProviderMeshDuplicatedFaces);

public:
    ViewProviderMeshDuplicatedFaces();
    ~ViewProviderMeshDuplicatedFaces() override;

    void attach(App::DocumentObject* pcFeature) override;
    void showDefects(const std::vector<Mesh::ElementIndex>&) override;

private:
    SoFaceSet* pcFaces;
};

/** The ViewProviderMeshDegenerations class displays degenerated faces to a line or even a point in
 * orange.
 * @author Werner Mayer
 */
class MeshGuiExport ViewProviderMeshDegenerations: public ViewProviderMeshDefects
{
    PROPERTY_HEADER_WITH_OVERRIDE(MeshGui::ViewProviderMeshDegenerations);

public:
    ViewProviderMeshDegenerations();
    ~ViewProviderMeshDegenerations() override;

    void attach(App::DocumentObject* pcFeature) override;
    void showDefects(const std::vector<Mesh::ElementIndex>&) override;

private:
    SoLineSet* pcLines;
};

class MeshGuiExport ViewProviderMeshDuplicatedPoints: public ViewProviderMeshDefects
{
    PROPERTY_HEADER_WITH_OVERRIDE(MeshGui::ViewProviderMeshDuplicatedPoints);

public:
    ViewProviderMeshDuplicatedPoints();
    ~ViewProviderMeshDuplicatedPoints() override;

    void attach(App::DocumentObject* pcFeature) override;
    void showDefects(const std::vector<Mesh::ElementIndex>&) override;

private:
    SoPointSet* pcPoints;
};

class MeshGuiExport ViewProviderMeshIndices: public ViewProviderMeshDefects
{
    PROPERTY_HEADER_WITH_OVERRIDE(MeshGui::ViewProviderMeshIndices);

public:
    ViewProviderMeshIndices();
    ~ViewProviderMeshIndices() override;

    void attach(App::DocumentObject* pcFeature) override;
    void showDefects(const std::vector<Mesh::ElementIndex>&) override;

private:
    SoFaceSet* pcFaces;
};

/** The ViewProviderMeshSelfIntersections class displays lines of self-intersections.
 * @author Werner Mayer
 */
class MeshGuiExport ViewProviderMeshSelfIntersections: public ViewProviderMeshDefects
{
    PROPERTY_HEADER_WITH_OVERRIDE(MeshGui::ViewProviderMeshSelfIntersections);

public:
    ViewProviderMeshSelfIntersections();
    ~ViewProviderMeshSelfIntersections() override;

    void attach(App::DocumentObject* pcFeature) override;
    void showDefects(const std::vector<Mesh::ElementIndex>&) override;

private:
    SoLineSet* pcLines;
};

class MeshGuiExport ViewProviderMeshFolds: public ViewProviderMeshDefects
{
    PROPERTY_HEADER_WITH_OVERRIDE(MeshGui::ViewProviderMeshFolds);

public:
    ViewProviderMeshFolds();
    ~ViewProviderMeshFolds() override;

    void attach(App::DocumentObject* pcFeature) override;
    void showDefects(const std::vector<Mesh::ElementIndex>&) override;

private:
    SoFaceSet* pcFaces;
};

}  // namespace MeshGui


#endif  // MESHGUI_VIEWPROVIDER_MESH_DEFECTS_H
