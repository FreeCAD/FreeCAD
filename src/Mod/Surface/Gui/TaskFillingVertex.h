/***************************************************************************
 *   Copyright (c) 2017 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef SURFACEGUI_TASKFILLINGVERTEX_H
#define SURFACEGUI_TASKFILLINGVERTEX_H

#include <Gui/DocumentObserver.h>
#include <Gui/SelectionFilter.h>
#include <Mod/Surface/App/FeatureFilling.h>
#include <Mod/Surface/Gui/SelectionMode.h>
#include <QWidget>


class QListWidgetItem;

namespace Gui
{
class ButtonGroup;
}

namespace SurfaceGui
{

class ViewProviderFilling;
class Ui_TaskFillingVertex;

class FillingVertexPanel: public QWidget,
                          public Gui::SelectionObserver,
                          public Gui::DocumentObserver
{
    Q_OBJECT

protected:
    class VertexSelection;
    enum SelectionMode
    {
        None = SurfaceGui::SelectionMode::None,
        AppendVertex = SurfaceGui::SelectionMode::AppendVertexConstraint,
        RemoveVertex = SurfaceGui::SelectionMode::RemoveVertexConstraint
    };
    SelectionMode selectionMode;
    Surface::Filling* editedObject;
    bool checkCommand;

private:
    Ui_TaskFillingVertex* ui;
    ViewProviderFilling* vp;

public:
    FillingVertexPanel(ViewProviderFilling* vp, Surface::Filling* obj);
    ~FillingVertexPanel() override;

    void open();
    void reject();
    void checkOpenCommand();
    void setEditedObject(Surface::Filling* obj);
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
    void onButtonVertexAddToggled(bool checked);
    void onButtonVertexRemoveToggled(bool checked);
    void onDeleteVertex();
    void clearSelection();

private:
    void exitSelectionMode();
};

}  // namespace SurfaceGui

#endif  // SURFACEGUI_TASKFILLINGVERTEX_H
