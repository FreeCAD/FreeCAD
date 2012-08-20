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
# include <QTextStream>
#endif

#include "Hypothesis.h"
#include "ui_Hypothesis.h"

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Gui/Command.h>

using namespace FemGui;

HypothesisWidget::HypothesisWidget(QWidget* parent)
  : QWidget(parent), ui(new Ui_HypothesisWidget)
{
    ui->setupUi(this);
}

HypothesisWidget::~HypothesisWidget()
{
    delete ui;
}

bool HypothesisWidget::accept()
{
    Base::Type type = Base::Type::fromName("Part::Feature");
    std::vector<App::DocumentObject*> obj = Gui::Selection().getObjectsOfType(type);

    Gui::Command::openCommand("Create FEM");
    Gui::Command::doCommand(Gui::Command::Doc, "import Fem");
    for (std::vector<App::DocumentObject*>::iterator it = obj.begin(); it != obj.end(); ++it) {
        QString cmd;
        QTextStream str(&cmd);
        App::Document* doc = (*it)->getDocument();
        QString doc_name = QString::fromAscii(doc->getName());
        QString obj_name = QString::fromAscii((*it)->getNameInDocument());
        str << QString::fromAscii(
            "__fem__=Fem.FemMesh()\n"
            "__fem__.setShape(FreeCAD.getDocument('%1').%2.Shape)\n")
            .arg(doc_name).arg(obj_name);
        int hyp=0;

        if (ui->maxLength->isChecked()) {
            str << QString::fromAscii(
                "hyp=Fem.StdMeshers_MaxLength(%1,__fem__)\n"
                "hyp.setLength(%2)\n"
                "__fem__.addHypothesis(hyp)\n")
                .arg(hyp++).arg(ui->valMaxLength->value());
        }

        if (ui->localLength->isChecked()) {
            str << QString::fromAscii(
                "hyp=Fem.StdMeshers_LocalLength(%1,__fem__)\n"
                "hyp.setLength(%2)\n"
                "__fem__.addHypothesis(hyp)\n")
                .arg(hyp++).arg(ui->valLocalLength->value());
        }

        if (ui->maxArea->isChecked()) {
            str << QString::fromAscii(
                "hyp=Fem.StdMeshers_MaxElementArea(%1,__fem__)\n"
                "hyp.setMaxArea(%2)\n"
                "__fem__.addHypothesis(hyp)\n").
                arg(hyp++).arg(ui->valMaxArea->value());
        }
#if 0
        str << QString::fromAscii(
            "hyp=Fem.StdMeshers_NumberOfSegments(%1,__fem__)\n"
            "hyp.setNumberOfSegments(1)\n"
            "__fem__.addHypothesis(hyp)\n")
            .arg(hyp++);

        str << QString::fromAscii(
            "hyp=Fem.StdMeshers_Deflection1D(%1,__fem__)\n"
            "hyp.setDeflection(0.02)\n"
            "__fem__.addHypothesis(hyp)\n")
            .arg(hyp++);
#endif
        str << QString::fromAscii(
            "hyp=Fem.StdMeshers_Regular_1D(%1,__fem__)\n"
            "__fem__.addHypothesis(hyp)\n")
            .arg(hyp++);

        if (ui->quadPref->isChecked()) {
            str << QString::fromAscii(
                "hyp=Fem.StdMeshers_QuadranglePreference(%1,__fem__)\n"
                "__fem__.addHypothesis(hyp)\n")
                .arg(hyp++);
        }

        str << QString::fromAscii(
            "hyp=Fem.StdMeshers_Quadrangle_2D(%1,__fem__)\n"
            "__fem__.addHypothesis(hyp)\n")
            .arg(hyp++);

        str << QString::fromAscii(
            "__fem__.compute()\n"
            "FreeCAD.getDocument('%1').addObject"
            "(\"Fem::FemMeshObject\",'%2').FemMesh=__fem__\n"
            "del __fem__, hyp\n")
            .arg(doc_name).arg(obj_name);
        Gui::Command::doCommand(Gui::Command::Doc, "%s", (const char*)cmd.toAscii());
    }
    Gui::Command::commitCommand();
    return true;
}

bool HypothesisWidget::reject()
{
    return true;
}

void HypothesisWidget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}

// -----------------------------------------------

TaskHypothesis::TaskHypothesis()
{
    widget = new HypothesisWidget();
    taskbox = new Gui::TaskView::TaskBox(
        QPixmap(),
        widget->windowTitle(), true, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskHypothesis::~TaskHypothesis()
{
}

void TaskHypothesis::open()
{
}

bool TaskHypothesis::accept()
{
    try {
        return widget->accept();
    }
    catch (const Base::Exception& e) {
        Base::Console().Error("%s\n", e.what());
        return false;
    }
    catch (...) {
        Base::Console().Error("Unknown error\n");
        return false;
    }
}

bool TaskHypothesis::reject()
{
    return widget->reject();
}

#include "moc_Hypothesis.cpp"
