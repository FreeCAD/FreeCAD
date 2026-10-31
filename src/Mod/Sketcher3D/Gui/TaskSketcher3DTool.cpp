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

#include <QKeyEvent>

#include <Gui/BitmapFactory.h>
#include "TaskSketcher3DTool.h"


using namespace Sketcher3DGui;

TaskSketcher3DTool::TaskSketcher3DTool()
    : TaskBox(Gui::BitmapFactory().pixmap("document-new"), tr("Tool Parameters"), true, nullptr)
{
    hide();
}

TaskSketcher3DTool::~TaskSketcher3DTool() = default;

void TaskSketcher3DTool::setToolWidget(std::unique_ptr<Sketcher3DToolWidget> w)
{
    clearToolWidget();
    widget = std::move(w);
    groupLayout()->addWidget(widget.get());
    widget->installEventFilter(this);
    for (auto* child : widget->findChildren<QWidget*>()) {
        child->installEventFilter(this);
    }
    show();
}

void TaskSketcher3DTool::clearToolWidget()
{
    if (!widget) {
        return;
    }
    widget->removeEventFilter(this);
    for (auto* child : widget->findChildren<QWidget*>()) {
        child->removeEventFilter(this);
    }
    groupLayout()->removeWidget(widget.get());
    widget.reset();
    hide();
}

bool TaskSketcher3DTool::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::KeyPress) {
        auto* key = static_cast<QKeyEvent*>(event);
        if (key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter) {
            widget->accept();
            return true;
        }
    }
    return Gui::TaskView::TaskBox::eventFilter(watched, event);
}

#include "moc_TaskSketcher3DTool.cpp"
