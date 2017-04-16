/***************************************************************************
 *   Copyright (c) 2015 Balázs Bámer                                       *
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

#ifndef SURFACE_GUI_SURFACE_H
#define SURFACE_GUI_SURFACE_H

#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Gui/SelectionFilter.h>
#include <Gui/DocumentObserver.h>
#include <Base/BoundBox.h>
#include <Mod/Part/Gui/ViewProviderSpline.h>
#include <Mod/Surface/App/FeatureGeomFillSurface.h>
#include <GeomFill_FillingStyle.hxx>

namespace SurfaceGui
{

class EdgeSelection : public Gui::SelectionFilterGate
{
public:
    EdgeSelection(bool appendEdges, Surface::GeomFillSurface* editedObject)
        : Gui::SelectionFilterGate(static_cast<Gui::SelectionFilter*>(nullptr))
        , appendEdges(appendEdges)
        , editedObject(editedObject)
    {
    }
    /**
      * Allow the user to pick only edges.
      */
    bool allow(App::Document* pDoc, App::DocumentObject* pObj, const char* sSubName);

private:
    bool appendEdges;
    Surface::GeomFillSurface* editedObject;
};

class Ui_SurfaceFilling;

class ViewProviderGeomFillSurface : public PartGui::ViewProviderSpline
{
    PROPERTY_HEADER(SurfaceGui::ViewProviderGeomFillSurface);
public:
    virtual void setupContextMenu(QMenu*, QObject*, const char*);
    virtual bool setEdit(int ModNum);
    virtual void unsetEdit(int ModNum);
    QIcon getIcon(void) const;
};

class SurfaceFilling : public QWidget,
                       public Gui::SelectionObserver,
                       public Gui::DocumentObserver
{
    Q_OBJECT

protected:
    enum SelectionMode { None, Append, Remove };
    SelectionMode selectionMode;
    Surface::GeomFillSurface* editedObject;
    bool checkCommand;

private:
    Ui_SurfaceFilling* ui;
    ViewProviderGeomFillSurface* vp;

public:
    SurfaceFilling(ViewProviderGeomFillSurface* vp, Surface::GeomFillSurface* obj);
    ~SurfaceFilling();

    void open();
    bool accept();
    bool reject();
    void setEditedObject(Surface::GeomFillSurface* obj);

protected:
    void changeEvent(QEvent *e);
    virtual void onSelectionChanged(const Gui::SelectionChanges& msg);
    /** Notifies on undo */
    virtual void slotUndoDocument(const Gui::Document& Doc);
    /** Notifies on redo */
    virtual void slotRedoDocument(const Gui::Document& Doc);
    void changeFillType(GeomFill_FillingStyle);

private Q_SLOTS:
    void on_fillType_stretch_clicked();
    void on_fillType_coons_clicked();
    void on_fillType_curved_clicked();
    void on_buttonEdgeAdd_clicked();
    void on_buttonEdgeRemove_clicked();
    void onDeleteEdge(void);
};

class TaskSurfaceFilling : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskSurfaceFilling(ViewProviderGeomFillSurface* vp, Surface::GeomFillSurface* obj);
    ~TaskSurfaceFilling();
    void setEditedObject(Surface::GeomFillSurface* obj);

public:
    void open();
    bool accept();
    bool reject();

    virtual QDialogButtonBox::StandardButtons getStandardButtons() const
    { return QDialogButtonBox::Ok | QDialogButtonBox::Cancel; }

private:
    SurfaceFilling* widget;
    Gui::TaskView::TaskBox* taskbox;
    ViewProviderGeomFillSurface* view;
};

} //namespace Surface

#endif // SURFACE_GUI_SURFACE_H
