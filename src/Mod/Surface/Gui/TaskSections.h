/***************************************************************************
 *   Copyright (c) 2020 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef SURFACEGUI_TASKSECTIONS_H
#define SURFACEGUI_TASKSECTIONS_H

#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Gui/SelectionFilter.h>
#include <Gui/DocumentObserver.h>
#include <Base/BoundBox.h>
#include <Mod/Part/Gui/ViewProviderSpline.h>
#include <Mod/Surface/App/FeatureSections.h>
#include <memory>

class QListWidgetItem;

namespace SurfaceGui
{

class Ui_Sections;

class ViewProviderSections : public PartGui::ViewProviderSpline
{
    PROPERTY_HEADER(SurfaceGui::ViewProviderSections);
    typedef std::vector<App::PropertyLinkSubList::SubSet> References;

public:
    enum ShapeType {Vertex, Edge, Face};
    virtual void setupContextMenu(QMenu*, QObject*, const char*);
    virtual bool setEdit(int ModNum);
    virtual void unsetEdit(int ModNum);
    QIcon getIcon(void) const;
    void highlightReferences(ShapeType type, const References& refs, bool on);
};

class SectionsPanel : public QWidget,
                      public Gui::SelectionObserver,
                      public Gui::DocumentObserver
{
    Q_OBJECT

protected:
    class ShapeSelection;
    enum SelectionMode { None, AppendEdge, RemoveEdge };
    SelectionMode selectionMode;
    Surface::Sections* editedObject;
    bool checkCommand;

private:
    std::unique_ptr<Ui_Sections> ui;
    ViewProviderSections* vp;

public:
    SectionsPanel(ViewProviderSections* vp, Surface::Sections* obj);
    ~SectionsPanel();

    void open();
    void checkOpenCommand();
    bool accept();
    bool reject();
    void setEditedObject(Surface::Sections* obj);

protected:
    void changeEvent(QEvent *e);
    virtual void onSelectionChanged(const Gui::SelectionChanges& msg);
    /** Notifies on undo */
    virtual void slotUndoDocument(const Gui::Document& Doc);
    /** Notifies on redo */
    virtual void slotRedoDocument(const Gui::Document& Doc);
    /** Notifies when the object is about to be removed. */
    virtual void slotDeletedObject(const Gui::ViewProviderDocumentObject& Obj);

private Q_SLOTS:
    void on_buttonEdgeAdd_clicked();
    void on_buttonEdgeRemove_clicked();
    void onDeleteEdge(void);
    void clearSelection();
    void onIndexesMoved();

private:
    void appendCurve(App::DocumentObject*, const std::string& subname);
    void removeCurve(App::DocumentObject*, const std::string& subname);
};

class TaskSections : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskSections(ViewProviderSections* vp, Surface::Sections* obj);
    ~TaskSections();
    void setEditedObject(Surface::Sections* obj);

public:
    void open();
    bool accept();
    bool reject();

    virtual QDialogButtonBox::StandardButtons getStandardButtons() const
    { return QDialogButtonBox::Ok | QDialogButtonBox::Cancel; }

private:
    SectionsPanel* widget1;
};

} //namespace SurfaceGui

#endif // SURFACEGUI_TASKSECTIONS_H
