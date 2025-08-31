/***************************************************************************
 *   Copyright (c) 2011 Jürgen Riegel <juergen.riegel@web.de>              *
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

#ifndef GUI_TASKVIEW_TaskSketcherMessages_H
#define GUI_TASKVIEW_TaskSketcherMessages_H

#include <Gui/TaskView/TaskSolverMessages.h>

namespace SketcherGui
{

class ViewProviderSketch;

class TaskSketcherMessages: public Gui::TaskSolverMessages
{
    Q_OBJECT

public:
    explicit TaskSketcherMessages(ViewProviderSketch* sketchView);
    ~TaskSketcherMessages() override;

private:
    void createSettingsButtonActions() override;
    void onLabelStatusLinkClicked(const QString&) override;

    void updateToolTip(const QString& link) override;

protected:
    ViewProviderSketch* sketchView;
};

}  // namespace SketcherGui

#endif  // GUI_TASKVIEW_TaskSketcherMessages_H
