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
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Gui/SelectionFilter.h>
#include <Gui/ViewProvider.h>

#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/UnitsApi.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Mod/Part/App/PartFeatures.h>


using namespace PartGui;

class OffsetWidget::Private
{
public:
    Ui_TaskOffset ui;
    Part::Offset* offset;
    Private()
    {
    }
    ~Private()
    {
    }
};

/* TRANSLATOR PartGui::OffsetWidget */

OffsetWidget::OffsetWidget(Part::Offset* offset, QWidget* parent)
  : d(new Private())
{
    Gui::Application::Instance->runPythonCode("from FreeCAD import Base");
    Gui::Application::Instance->runPythonCode("import Part");

    d->offset = offset;
    d->ui.setupUi(this);
    d->ui.spinOffset->setDecimals(Base::UnitsApi::getDecimals());
    d->ui.spinOffset->setRange(-INT_MAX, INT_MAX);
    d->ui.spinOffset->setSingleStep(0.1);
    d->ui.spinOffset->setValue(d->offset->Value.getValue());
    d->ui.facesButton->hide();
}

OffsetWidget::~OffsetWidget()
{
    delete d;
}

Part::Offset* OffsetWidget::getObject() const
{
    return d->offset;
}

void OffsetWidget::on_spinOffset_valueChanged(double val)
{
    d->offset->Value.setValue(val);
    if (d->ui.updateView->isChecked())
        d->offset->getDocument()->recomputeFeature(d->offset);
}

void OffsetWidget::on_modeType_activated(int val)
{
    d->offset->Mode.setValue(val);
    if (d->ui.updateView->isChecked())
        d->offset->getDocument()->recomputeFeature(d->offset);
}

void OffsetWidget::on_joinType_activated(int val)
{
    d->offset->Join.setValue((long)val);
    if (d->ui.updateView->isChecked())
        d->offset->getDocument()->recomputeFeature(d->offset);
}

void OffsetWidget::on_intersection_toggled(bool on)
{
    d->offset->Intersection.setValue(on);
    if (d->ui.updateView->isChecked())
        d->offset->getDocument()->recomputeFeature(d->offset);
}

void OffsetWidget::on_selfIntersection_toggled(bool on)
{
    d->offset->SelfIntersection.setValue(on);
    if (d->ui.updateView->isChecked())
        d->offset->getDocument()->recomputeFeature(d->offset);
}

void OffsetWidget::on_fillOffset_toggled(bool on)
{
    d->offset->Fill.setValue(on);
    if (d->ui.updateView->isChecked())
        d->offset->getDocument()->recomputeFeature(d->offset);
}

void OffsetWidget::on_updateView_toggled(bool on)
{
    if (on) {
        d->offset->getDocument()->recomputeFeature(d->offset);
    }
}

bool OffsetWidget::accept()
{
    std::string name = d->offset->getNameInDocument();

    try {
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Value = %f",
            name.c_str(),d->ui.spinOffset->value());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Mode = %i",
            name.c_str(),d->ui.modeType->currentIndex());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Join = %i",
            name.c_str(),d->ui.joinType->currentIndex());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Intersection = %s",
            name.c_str(),d->ui.intersection->isChecked() ? "True" : "False");
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.SelfIntersection = %s",
            name.c_str(),d->ui.selfIntersection->isChecked() ? "True" : "False");

        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");
        if (!d->offset->isValid())
            throw Base::Exception(d->offset->getStatusString());
        Gui::Command::doCommand(Gui::Command::Gui,"Gui.ActiveDocument.resetEdit()");
        Gui::Command::commitCommand();
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(this, tr("Input error"), QString::fromLatin1(e.what()));
        return false;
    }

    return true;
}

bool OffsetWidget::reject()
{
    // get the support and Sketch
    App::DocumentObject* source = d->offset->Source.getValue();
    if (source){
        Gui::Application::Instance->getViewProvider(source)->show();
    }

    // roll back the done things
    Gui::Command::abortCommand();
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.ActiveDocument.resetEdit()");
    Gui::Command::updateActive();

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

TaskOffset::TaskOffset(Part::Offset* offset)
{
    widget = new OffsetWidget(offset);
    taskbox = new Gui::TaskView::TaskBox(
        Gui::BitmapFactory().pixmap("Part_Offset"),
        widget->windowTitle(), true, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskOffset::~TaskOffset()
{
}

Part::Offset* TaskOffset::getObject() const
{
    return widget->getObject();
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
