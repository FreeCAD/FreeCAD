/***************************************************************************
 *   Copyright (c) 2013 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef MESHGUI_MESHSELECTION_H
#define MESHGUI_MESHSELECTION_H

#include <vector>
#include <QWidget>
#include <Inventor/nodes/SoEventCallback.h>
#include <Gui/SelectionObject.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>

namespace Gui {
    class View3DInventorViewer;
}

namespace MeshGui {

class ViewProviderMesh;

class MeshGuiExport MeshSelection
{
public:
    MeshSelection();
    ~MeshSelection();

    void startSelection();
    void startDeselection();
    void stopSelection();
    bool deleteSelection();
    void fullSelection();
    void clearSelection();
    void invertSelection();

    void selectComponent(int);
    void deselectComponent(int);
    void selectTriangle();
    void deselectTriangle();

    void setCheckOnlyPointToUserTriangles(bool);
    bool isCheckedOnlyPointToUserTriangles() const;
    void setCheckOnlyVisibleTriangles(bool);
    bool isCheckedOnlyVisibleTriangles() const;
    void setAddComponentOnClick(bool);
    void setRemoveComponentOnClick(bool);
    void setObjects(const std::vector<Gui::SelectionObject>&);
    std::vector<App::DocumentObject*> getObjects() const;

protected:
    std::list<ViewProviderMesh*> getViewProviders() const;
    Gui::View3DInventorViewer* getViewer() const;
    void prepareBrushSelection(bool,SoEventCallbackCB *cb);
    void startInteractiveCallback(Gui::View3DInventorViewer* viewer,SoEventCallbackCB *cb);
    void stopInteractiveCallback(Gui::View3DInventorViewer* viewer);

private:

    static void selectGLCallback(void * ud, SoEventCallback * n);
    static void pickFaceCallback(void * ud, SoEventCallback * n);

private:
    bool onlyPointToUserTriangles, onlyVisibleTriangles;
    bool addToSelection, addComponent, removeComponent;
    SoEventCallbackCB *_activeCB;
    mutable std::vector<Gui::SelectionObject> meshObjects;

    static unsigned char cross_bitmap[];
    static unsigned char cross_mask_bitmap[];
};

}

#endif // MESHGUI_MESHSELECTION_H
