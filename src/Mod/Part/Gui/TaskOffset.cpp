/***************************************************************************
 *   Copyright (c) 2012 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QMessageBox>
# include <QTextStream>
#endif

#include "ui_TaskOffset.h"
#include "TaskOffset.h"

#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Gui/SelectionFilter.h>
#include <Gui/ViewProvider.h>

#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Mod/Part/App/PartFeature.h>


using namespace PartGui;

class OffsetWidget::Private
{
public:
    Ui_TaskOffset ui;
    std::string document;
    Private()
    {
    }
    ~Private()
    {
    }
};

/* TRANSLATOR PartGui::OffsetWidget */

OffsetWidget::OffsetWidget(QWidget* parent)
  : d(new Private())
{
    Gui::Application::Instance->runPythonCode("from FreeCAD import Base");
    Gui::Application::Instance->runPythonCode("import Part");

    d->ui.setupUi(this);
}

OffsetWidget::~OffsetWidget()
{
    delete d;
}

bool OffsetWidget::accept()
{
    return true;
}

bool OffsetWidget::reject()
{
    return true;
}

void OffsetWidget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        d->ui.retranslateUi(this);
    }
}


/* TRANSLATOR PartGui::TaskOffset */

TaskOffset::TaskOffset()
{
    widget = new OffsetWidget();
    taskbox = new Gui::TaskView::TaskBox(
        Gui::BitmapFactory().pixmap("Part_Offset"),
        widget->windowTitle(), true, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskOffset::~TaskOffset()
{
}

void TaskOffset::open()
{
}

void TaskOffset::clicked(int)
{
}

bool TaskOffset::accept()
{
    return widget->accept();
}

bool TaskOffset::reject()
{
    return widget->reject();
}

#include "moc_TaskOffset.cpp"
