// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#ifndef MESHPARTGUI_PATCHONMESH_H
#define MESHPARTGUI_PATCHONMESH_H

#include <GeomAbs_Shape.hxx>
#include <QObject>
#include <memory>
#include <Inventor/SbVec3f.h>

class SoPickedPoint;

namespace Gui
{
class View3DInventor;
class View3DInventorViewer;
}  // namespace Gui

namespace MeshPartGui
{

class PatchOnMeshHandler: public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(PatchOnMeshHandler)

public:
    explicit PatchOnMeshHandler(QObject* parent = nullptr);
    ~PatchOnMeshHandler() override;
    void setParameters(int maxDegree, GeomAbs_Shape cont, double tol3d);
    void enableCallback(Gui::View3DInventor* viewer);
    void disableCallback();
    void recomputeDocument();

private Q_SLOTS:
    void onContextMenu();
    void onCreate();
    void onClear();
    void onCancel();

private:
    void handlePickedPoint(Gui::View3DInventorViewer* view, const SoPickedPoint* pp);
    void reset();
    void performAction();
    std::vector<SbVec3f> getPoints() const;

private:
    class Private;
    std::unique_ptr<Private> d_ptr;
};
}  // namespace MeshPartGui

#endif  // MESHPARTGUI_PATCHONMESH_H
