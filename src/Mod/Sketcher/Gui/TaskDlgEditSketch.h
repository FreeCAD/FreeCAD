// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2009 Jürgen Riegel <juergen.riegel@web.de>              *
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

#pragma once

#include <fastsignals/signal.h>

#include <Gui/TaskView/TaskDialog.h>

#include "TaskSketcherConstraints.h"
#include "TaskSketcherElements.h"
#include "TaskSketcherMessages.h"
#include "TaskSketcherSolverAdvanced.h"
#include "TaskSketcherTool.h"
#include "ViewProviderSketch.h"


using Connection = fastsignals::connection;

namespace SketcherGui
{


/// simulation dialog for the TaskView
class SketcherGuiExport TaskDlgEditSketch: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    explicit TaskDlgEditSketch(ViewProviderSketch* sketchView);
    ~TaskDlgEditSketch() override;
    ViewProviderSketch* getSketchView() const
    {
        return sketchView;
    }

public:
    /// is called the TaskView when the dialog is opened
    void open() override;
    /// is called by the framework if an button is clicked which has no accept or reject role
    void clicked(int) override;
    /// is called by the framework if the dialog is accepted (Ok)
    bool accept() override;
    /// is called by the framework if the dialog is rejected (Cancel)
    bool reject() override;
    bool isAllowedAlterDocument() const override
    {
        return false;
    }
    void autoClosedOnClosedView() override;

    QDialogButtonBox::StandardButtons getStandardButtons() const override;

    /** @brief Function used to register a slot to be triggered when the tool widget is changed. */
    template<typename F>
    fastsignals::connection registerToolWidgetChanged(F&& f)
    {
        return ToolSettings->registerToolWidgetChanged(std::forward<F>(f));
    }

protected:
    void slotUndoDocument(const App::Document&);
    void slotRedoDocument(const App::Document&);

private:
    void slotToolChanged(const std::string& toolname);

protected:
    ViewProviderSketch* sketchView;
    TaskSketcherConstraints* Constraints;
    TaskSketcherElements* Elements;
    TaskSketcherMessages* Messages;
    TaskSketcherSolverAdvanced* SolverAdvanced;
    TaskSketcherTool* ToolSettings;

private:
    Connection connectionToolSettings;
};


}  // namespace SketcherGui
