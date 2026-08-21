// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Yash Suthar <yashsuthar983@gmail.com>              *
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

#include <Gui/BitmapFactory.h>
#include <Mod/Part/App/Geometry.h>
#include <Mod/Sketcher3D/App/Sketch3DObject.h>

#include "TaskSketcher3DMessages.h"
#include "ViewProviderSketch3D.h"


using namespace Sketcher3DGui;

TaskSketcher3DMessages::TaskSketcher3DMessages(ViewProviderSketch3D* view)
    : TaskBox(Gui::BitmapFactory().pixmap("Sketcher_Sketch"), tr("Solver Messages"), true, nullptr)
    , sketchView(view)
{
    auto* body = new QWidget(this);
    auto* root = new QVBoxLayout(body);
    root->setContentsMargins(0, 0, 0, 0);

    statusLabel = new QLabel(body);
    statusLabel->setWordWrap(true);
    root->addWidget(statusLabel);

    addWidget(body, true, false);

    connectionConstraintsChanged = sketchView->signalConstraintsChanged.connect([this]() {
        refresh();
    });
    connectionElementsChanged = sketchView->signalElementsChanged.connect([this]() { refresh(); });
    refresh();
}

TaskSketcher3DMessages::~TaskSketcher3DMessages()
{
    connectionConstraintsChanged.disconnect();
    connectionElementsChanged.disconnect();
}

void TaskSketcher3DMessages::refresh()
{
    auto* sketch = sketchView->getSketch3DObject();

    int pointCount = 0;
    int lineCount = 0;
    int otherCount = 0;
    for (Part::Geometry* geo : sketch->Geometry.getValues()) {
        if (!geo) {
            continue;
        }
        if (geo->is<Part::GeomPoint>()) {
            ++pointCount;
        }
        else if (geo->is<Part::GeomLineSegment>()) {
            ++lineCount;
        }
        else {
            ++otherCount;
        }
    }

    QStringList parts;
    parts << tr("%1 point(s)").arg(pointCount);
    parts << tr("%1 line(s)").arg(lineCount);
    if (otherCount > 0) {
        parts << tr("%1 other").arg(otherCount);
    }

    statusLabel->setText(tr("%1 constraint(s) | %2")
                             .arg(sketch->Constraints.getSize())
                             .arg(parts.join(QStringLiteral(", "))));
}

#include "moc_TaskSketcher3DMessages.cpp"
