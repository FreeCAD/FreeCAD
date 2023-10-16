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


#include "PreCompiled.h"

#ifndef _PreComp_
#endif

#include "TaskSketcherTool.h"
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Base/Tools.h>
#include <Base/UnitsApi.h>

#include <QEvent>

#include "ViewProviderSketch.h"

#include "SketcherToolDefaultWidget.h"

using namespace SketcherGui;
using namespace Gui::TaskView;

TaskSketcherTool::TaskSketcherTool(ViewProviderSketch* sketchView)
    : TaskBox(Gui::BitmapFactory().pixmap("document-new"), tr("Tool parameters"), true, nullptr)
    , sketchView(sketchView)
{}

TaskSketcherTool::~TaskSketcherTool()
{}

void TaskSketcherTool::toolChanged(const std::string& toolname)
{
    Q_UNUSED(toolname)

    widget = sketchView->toolManager.createToolWidget();

    if (widget) {
        this->groupLayout()->addWidget(widget.get());

        setHeaderText(sketchView->toolManager.getToolWidgetText());
        setHeaderIcon(sketchView->toolManager.getToolIcon());

        signalToolWidgetChanged(this->widget.get());
    }
    else {
        signalToolWidgetChanged(nullptr);
    }
}

#include "moc_TaskSketcherTool.cpp"
