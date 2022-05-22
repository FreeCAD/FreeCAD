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

#ifndef SURFACEGUI_TASKFILLING_H
#define SURFACEGUI_TASKFILLING_H

#include <Gui/DocumentObserver.h>
#include <Gui/SelectionFilter.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Gui/Widgets.h>
#include <Base/BoundBox.h>
#include <Mod/Part/Gui/ViewProviderSpline.h>
#include <Mod/Surface/App/FeatureFilling.h>


class QListWidgetItem;

namespace SurfaceGui
{

class FillingVertexPanel;
class FillingEdgePanel;
class Ui_TaskFilling;

class ViewProviderFilling : public PartGui::ViewProviderSpline
{
    PROPERTY_HEADER_WITH_OVERRIDE(SurfaceGui::ViewProviderFilling);
    using References = std::vector<App::PropertyLinkSubList::SubSet>;

public:
    enum ShapeType {Vertex, Edge, Face};
    void setupContextMenu(QMenu*, QObject*, const char*) override;
    bool setEdit(int ModNum) override;
    void unsetEdit(int ModNum) override;
    QIcon getIcon() const override;
    void highlightReferences(ShapeType type, const References& refs, bool on);
};

class FillingPanel : public QWidget,
                     public Gui::SelectionObserver,
                     public Gui::DocumentObserver
{
    Q_OBJECT

protected:
    class ShapeSelection;
    enum SelectionMode { None, InitFace, AppendEdge, RemoveEdge };
    SelectionMode selectionMode;
    Surface::Filling* editedObject;
    bool checkCommand;

private:
    Ui_TaskFilling* ui;
    ViewProviderFilling* vp;
    Gui::ButtonGroup *buttonGroup;

public:
    FillingPanel(ViewProviderFilling* vp, Surface::Filling* obj);
    ~FillingPanel() override;

    void open();
    void checkOpenCommand();
    bool accept();
    bool reject();
    void setEditedObject(Surface::Filling* obj);

protected:
    void changeEvent(QEvent *e) override;
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;
    /** Notifies on undo */
    void slotUndoDocument(const Gui::Document& Doc) override;
    /** Notifies on redo */
    void slotRedoDocument(const Gui::Document& Doc) override;
    /** Notifies when the object is about to be removed. */
    void slotDeletedObject(const Gui::ViewProviderDocumentObject& Obj) override;
    void modifyBoundary(bool);

private Q_SLOTS:
    void on_buttonInitFace_clicked();
    void on_buttonEdgeAdd_toggled(bool checked);
    void on_buttonEdgeRemove_toggled(bool checked);
    void on_lineInitFaceName_textChanged(const QString&);
    void on_listBoundary_itemDoubleClicked(QListWidgetItem*);
    void on_buttonAccept_clicked();
    void on_buttonIgnore_clicked();
    void onDeleteEdge();
    void onIndexesMoved();
    void clearSelection();

private:
    void exitSelectionMode();
};

class TaskFilling : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskFilling(ViewProviderFilling* vp, Surface::Filling* obj);
    ~TaskFilling() override;
    void setEditedObject(Surface::Filling* obj);

public:
    void open() override;
    void closed() override;
    bool accept() override;
    bool reject() override;

    QDialogButtonBox::StandardButtons getStandardButtons() const override
    { return QDialogButtonBox::Ok | QDialogButtonBox::Cancel; }

private:
    FillingPanel* widget1;
    FillingEdgePanel* widget2;
    FillingVertexPanel* widget3;
};

} //namespace SurfaceGui

#endif // SURFACEGUI_TASKFILLING_H
