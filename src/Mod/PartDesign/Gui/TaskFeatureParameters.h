/***************************************************************************
 *   Copyright (C) 2015 Alexander Golubev (Fat-Zer) <fatzer2@gmail.com>    *
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

#ifndef TASKFEATUREPARAMETERS_H_NAHKE2YZ
#define TASKFEATUREPARAMETERS_H_NAHKE2YZ


#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Gui/DocumentObserver.h>

#include "ViewProvider.h"

namespace PartDesignGui {

/// Convenience class to collect common methods for all SketchBased features
class TaskFeatureParameters : public Gui::TaskView::TaskBox,
                              public Gui::DocumentObserver
{
    Q_OBJECT

public:
    TaskFeatureParameters(PartDesignGui::ViewProvider* vp, QWidget *parent,
                              const std::string& pixmapname, const QString& parname);
    ~TaskFeatureParameters() override = default;

    /// save field history
    virtual void saveHistory() {}
    /// apply changes made in the parameters input to the model via commands
    virtual void apply() {}

    void recomputeFeature();

protected Q_SLOTS:
    // TODO Add update view to all dialogs (2015-12-05, Fat-Zer)
    void onUpdateView(bool on);

private:
    /** Notifies when the object is about to be removed. */
    void slotDeletedObject(const Gui::ViewProviderDocumentObject& Obj) override;

protected:
    PartDesignGui::ViewProvider *vp;
    /// Lock updateUI(), applying changes to the underlying feature and calling recomputeFeature()
    bool blockUpdate;
};

/// A common base for sketch based, dressup and other solid parameters dialogs
class TaskDlgFeatureParameters : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    explicit TaskDlgFeatureParameters(PartDesignGui::ViewProvider *vp);
    ~TaskDlgFeatureParameters() override;

public:
    /// is called by the framework if the dialog is accepted (Ok)
    bool accept() override;
    /// is called by the framework if the dialog is rejected (Cancel)
    bool reject() override;

    /// Returns the view provider dialog is runed for
     PartDesignGui::ViewProvider *viewProvider() const { return vp; }

protected:
    PartDesignGui::ViewProvider *vp;
};

} //namespace PartDesignGui


#endif /* end of include guard: TASKFEATUREPARAMETERS_H_NAHKE2YZ */
