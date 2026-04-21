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

#include <QLabel>
#include <QVBoxLayout>

#include <App/DocumentObject.h>
#include <Gui/BitmapFactory.h>

#include "TaskSketcher3DTool.h"
#include "ViewProviderSketch3D.h"


using namespace Sketcher3DGui;


TaskSketcher3DTool::TaskSketcher3DTool(ViewProviderSketch3D* view)
    : TaskBox(
          Gui::BitmapFactory().pixmap("Sketcher_Sketch"),
          view && view->getObject() ? QString::fromUtf8(view->getObject()->Label.getValue())
                                    : tr("Sketch3D Edit"),
          true,
          nullptr
      )
    , sketchView(view)
    , statusLabel(nullptr)
    , hintLabel(nullptr)
{
    auto* body = new QWidget(this);
    auto* root = new QVBoxLayout(body);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(4);

    // Status
    statusLabel = new QLabel();
    setStatus(0, 0);
    root->addWidget(statusLabel);

    // Hint
    hintLabel = new QLabel();
    hintLabel->setWordWrap(true);
    setHint(tr("Pick a tool from the toolbar to start sketching."));
    root->addWidget(hintLabel);

    body->setLayout(root);
    addWidget(body, true, false);
}

TaskSketcher3DTool::~TaskSketcher3DTool() = default;

void TaskSketcher3DTool::setHint(const QString& text)
{
    if (hintLabel) {
        hintLabel->setText(text);
    }
}

void TaskSketcher3DTool::setStatus(int points, int lines)
{
    // add info here about elemets and constraints
}

#include "moc_TaskSketcher3DTool.cpp"
