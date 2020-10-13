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

#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Gui/SelectionFilter.h>
#include <Gui/DocumentObserver.h>
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
    PROPERTY_HEADER(SurfaceGui::ViewProviderFilling);
    typedef std::vector<App::PropertyLinkSubList::SubSet> References;

public:
    enum ShapeType {Vertex, Edge, Face};
    virtual void setupContextMenu(QMenu*, QObject*, const char*);
    virtual bool setEdit(int ModNum);
    virtual void unsetEdit(int ModNum);
    QIcon getIcon(void) const;
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

public:
    FillingPanel(ViewProviderFilling* vp, Surface::Filling* obj);
    ~FillingPanel();

    void open();
    void checkOpenCommand();
    bool accept();
    bool reject();
    void setEditedObject(Surface::Filling* obj);

protected:
    void changeEvent(QEvent *e);
    virtual void onSelectionChanged(const Gui::SelectionChanges& msg);
    /** Notifies on undo */
    virtual void slotUndoDocument(const Gui::Document& Doc);
    /** Notifies on redo */
    virtual void slotRedoDocument(const Gui::Document& Doc);
    /** Notifies when the object is about to be removed. */
    virtual void slotDeletedObject(const Gui::ViewProviderDocumentObject& Obj);
    void modifyBoundary(bool);

private Q_SLOTS:
    void on_buttonInitFace_clicked();
    void on_buttonEdgeAdd_clicked();
    void on_buttonEdgeRemove_clicked();
    void on_lineInitFaceName_textChanged(const QString&);
    void on_listBoundary_itemDoubleClicked(QListWidgetItem*);
    void on_buttonAccept_clicked();
    void on_buttonIgnore_clicked();
    void onDeleteEdge(void);
    void onIndexesMoved();
    void clearSelection();
};

class TaskFilling : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskFilling(ViewProviderFilling* vp, Surface::Filling* obj);
    ~TaskFilling();
    void setEditedObject(Surface::Filling* obj);

public:
    void open();
    bool accept();
    bool reject();

    virtual QDialogButtonBox::StandardButtons getStandardButtons() const
    { return QDialogButtonBox::Ok | QDialogButtonBox::Cancel; }

private:
    FillingPanel* widget1;
    FillingEdgePanel* widget2;
    FillingVertexPanel* widget3;
};

} //namespace SurfaceGui

#endif // SURFACEGUI_TASKFILLING_H
