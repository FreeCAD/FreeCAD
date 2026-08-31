// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2026 Andrew Shkolik <shkolik@gmail.com>                  *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#ifndef SURFACEGUI_TASKFILLING_H
#define SURFACEGUI_TASKFILLING_H

#include <App/DocumentObserver.h>
#include <App/PropertyLinks.h>
#include <Gui/DocumentObserver.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Mod/Surface/App/Gordon/FeatureGordonSurface.h>
#include <Mod/Surface/Gui/SelectionMode.h>
#include <Gui/Widgets.h>

#include "ViewProviderGordonSurface.h"


class QListWidgetItem;

namespace Gui
{
class ButtonGroup;
}

namespace SurfaceGui
{

class GordonSurfacePanel;
class Ui_TaskGordonSurface;

class GordonSurfacePanel: public QWidget, public Gui::SelectionObserver, public Gui::DocumentObserver
{
    Q_OBJECT

protected:
    class ShapeSelection;
    enum SelectionMode
    {
        None = SurfaceGui::SelectionMode::None,
        AppendEdge = SurfaceGui::SelectionMode::AppendEdge,
        RemoveEdge = SurfaceGui::SelectionMode::RemoveEdge
    };
    enum SelectionType
    {
        Profile,
        Guide
    };

    SelectionMode selectionMode;
    SelectionType selectionType;
    App::WeakPtrT<Surface::GordonSurface> editedObject;
    bool checkCommand;

private:
    Ui_TaskGordonSurface* ui;
    ViewProviderGordonSurface* vp;

public:
    GordonSurfacePanel(ViewProviderGordonSurface* vp, Surface::GordonSurface* obj);
    ~GordonSurfacePanel() override;

    void open();
    void checkOpenCommand();
    bool accept();
    bool reject();
    void setEditedObject(Surface::GordonSurface* obj);
    void appendButtons(Gui::ButtonGroup*);

protected:
    void changeEvent(QEvent* e) override;
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;
    /** Notifies on undo */
    void slotUndoDocument(const Gui::Document& Doc) override;
    /** Notifies on redo */
    void slotRedoDocument(const Gui::Document& Doc) override;
    /** Notifies when the object is about to be removed. */
    void slotDeletedObject(const Gui::ViewProviderDocumentObject& Obj) override;

private:
    void setupConnections();
    void onButtonProfileAddToggled(bool checked);
    void onButtonProfileRemoveToggled(bool checked);
    void onButtonGuideAddToggled(bool checked);
    void onButtonGuideRemoveToggled(bool checked);
    void onDeleteProfile();
    void onDeleteGuide();
    void onReverseProfile();
    void onReverseGuide();
    void onToleranceChanged(double value);
    void onUseNativeToggled(bool checked);
    void onParallelModeToggled(bool checked);
    void onApproxModeChanged(int index);
    void clearSelection();
    void appendEdges(
        const QList<QVariant> data,
        QListWidget* list,
        App::PropertyLinkSubList& edges,
        App::PropertyBoolList& directions
    );
    void removeEdge(
        const QList<QVariant> data,
        QListWidget* list,
        App::PropertyLinkSubList& edges,
        App::PropertyBoolList& directions
    );

private:
    void exitSelectionMode();
};

class TaskGordonSurface: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskGordonSurface(ViewProviderGordonSurface* vp, Surface::GordonSurface* obj);
    void setEditedObject(Surface::GordonSurface* obj);

public:
    void open() override;
    bool accept() override;
    bool reject() override;

    QDialogButtonBox::StandardButtons getStandardButtons() const override
    {
        return QDialogButtonBox::Ok | QDialogButtonBox::Cancel;
    }

private:
    Gui::ButtonGroup* buttonGroup;
    GordonSurfacePanel* widget;
};

}  // namespace SurfaceGui

#endif  // SURFACEGUI_TASKFILLING_H
