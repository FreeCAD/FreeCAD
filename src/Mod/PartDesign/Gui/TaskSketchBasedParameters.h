/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinländer <jrheinlaender@users.sourceforge.net>*
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


#ifndef GUI_TASKVIEW_TaskSketchBasedParameters_H
#define GUI_TASKVIEW_TaskSketchBasedParameters_H

#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection.h>
#include <Gui/TaskView/TaskDialog.h>
#include "ViewProvider.h"

namespace App {
class Property;
}

namespace PartDesignGui { 


/// Convenience class to collect common methods for all SketchBased features
class TaskSketchBasedParameters : public Gui::TaskView::TaskBox, public Gui::SelectionObserver
{
    Q_OBJECT

public:
    TaskSketchBasedParameters(PartDesignGui::ViewProvider* vp, QWidget *parent,
                              const std::string& pixmapname, const QString& parname);
    ~TaskSketchBasedParameters();

protected:
    void onSelectionChanged(const Gui::SelectionChanges& msg)=0;
    const QString onAddSelection(const Gui::SelectionChanges& msg);
    void onSelectReference(const bool pressed, const bool edge, const bool face, const bool planar);
    void exitSelectionMode();
    const QByteArray onFaceName(const QString& text);
    QString getFaceReference(const QString& obj, const QString& sub) const;
    void recomputeFeature();
    
    App::DocumentObject* getPartPlanes(const char* str) const;
    App::DocumentObject* getPartLines(const char* str) const;

protected Q_SLOTS:
    void onUpdateView(bool on);

protected:
    PartDesignGui::ViewProvider *vp;
    /// Lock updateUI(), applying changes to the underlying feature and calling recomputeFeature()
    bool blockUpdate;
};

class TaskDlgSketchBasedParameters : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDlgSketchBasedParameters(PartDesignGui::ViewProvider *vp);
    ~TaskDlgSketchBasedParameters();

public:
    /// is called the TaskView when the dialog is opened
    virtual void open();
    /// is called by the framework if an button is clicked which has no accept or reject role
    virtual void clicked(int);
    /// is called by the framework if the dialog is accepted (Ok)
    virtual bool accept()=0;
    /// is called by the framework if the dialog is rejected (Cancel)
    virtual bool reject();
    /// is called by the framework if the user presses the help button
    virtual bool isAllowedAlterDocument(void) const
    { return false; }

    /// returns for Close and Help button
    virtual QDialogButtonBox::StandardButtons getStandardButtons(void) const
    { return QDialogButtonBox::Ok|QDialogButtonBox::Cancel; }

protected:
    PartDesignGui::ViewProvider   *vp;
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TaskSketchBasedParameters_H
