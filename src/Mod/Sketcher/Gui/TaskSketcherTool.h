// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2022 Pierre-Louis Boyer <pierrelouis.boyer@gmail.com>   *
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

#include <Gui/TaskView/TaskView.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/Selection/Selection.h>
#include <fastsignals/signal.h>

namespace App
{
class Property;
}

namespace Gui
{
class ViewProvider;
}

namespace SketcherGui
{

class ViewProviderSketch;


class TaskSketcherTool: public Gui::TaskView::TaskBox
{
    Q_OBJECT

public:
    TaskSketcherTool(ViewProviderSketch* sketchView);
    ~TaskSketcherTool();

    QWidget* getWidget()
    {
        return widget.get();
    }

    void toolChanged(const std::string& toolname);

    template<typename F>
    fastsignals::connection registerToolWidgetChanged(F&& f)
    {
        return signalToolWidgetChanged.connect(std::forward<F>(f));
    }

private:
    ViewProviderSketch* sketchView;
    std::unique_ptr<QWidget> widget;
    fastsignals::scoped_connection changedSketchView;

    fastsignals::signal<void(QWidget* newwidget)> signalToolWidgetChanged;
};

}  // namespace SketcherGui
