// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2024 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#include <algorithm>
#include <QMessageBox>
#endif

#include <Gui/CommandT.h>
#include <Gui/WaitCursor.h>

#include "FitBSplineCurve.h"
#include "ui_FitBSplineCurve.h"


using namespace ReenGui;

class FitBSplineCurveWidget::Private
{
public:
    Ui_FitBSplineCurve ui {};
    App::DocumentObjectT obj {};
};

/* TRANSLATOR ReenGui::FitBSplineCurveWidget */

FitBSplineCurveWidget::FitBSplineCurveWidget(const App::DocumentObjectT& obj, QWidget* parent)
    : d(new Private())
{
    Q_UNUSED(parent);
    d->ui.setupUi(this);
    d->obj = obj;

    // clang-format off
    connect(d->ui.checkBox, &QCheckBox::toggled,
            this, &FitBSplineCurveWidget::toggleParametrizationType);
    connect(d->ui.groupBoxSmooth, &QGroupBox::toggled,
            this, &FitBSplineCurveWidget::toggleSmoothing);
    // clang-format on
}

FitBSplineCurveWidget::~FitBSplineCurveWidget()
{
    delete d;
}

void FitBSplineCurveWidget::toggleParametrizationType(bool on)
{
    d->ui.paramType->setEnabled(on);
    if (on) {
        d->ui.groupBoxSmooth->setChecked(false);
    }
}

void FitBSplineCurveWidget::toggleSmoothing(bool on)
{
    if (on) {
        d->ui.checkBox->setChecked(false);
        d->ui.paramType->setEnabled(false);
    }
}

bool FitBSplineCurveWidget::accept()
{
    try {
        tryAccept();
    }
    catch (const Base::Exception& e) {
        Gui::Command::abortCommand();
        QMessageBox::warning(this, tr("Input error"), QString::fromLatin1(e.what()));
        return false;
    }

    return true;
}

void FitBSplineCurveWidget::tryAccept()
{
    QString document = QString::fromStdString(d->obj.getDocumentPython());
    QString object = QString::fromStdString(d->obj.getObjectPython());

    QStringList arguments;
    arguments.append(
        QStringLiteral("Points=getattr(%1, %1.getPropertyNameOfGeometry())").arg(object));
    if (!d->ui.groupBoxSmooth->isChecked()) {
        arguments.append(QStringLiteral("MinDegree = %1").arg(d->ui.degreeMin->value()));
    }
    arguments.append(QStringLiteral("MaxDegree = %1").arg(d->ui.degreeMax->value()));
    arguments.append(QStringLiteral("Continuity = %1").arg(d->ui.continuity->currentIndex()));
    if (d->ui.checkBoxClosed->isChecked()) {
        arguments.append(QStringLiteral("Closed = True"));
    }
    else {
        arguments.append(QStringLiteral("Closed = False"));
    }
    if (d->ui.checkBox->isChecked()) {
        int index = d->ui.paramType->currentIndex();
        arguments.append(QStringLiteral("ParametrizationType = %1").arg(index));
    }
    if (d->ui.groupBoxSmooth->isChecked()) {
        arguments.append(QStringLiteral("Weight1 = %1").arg(d->ui.curveLength->value()));
        arguments.append(QStringLiteral("Weight2 = %1").arg(d->ui.curvature->value()));
        arguments.append(QStringLiteral("Weight3 = %1").arg(d->ui.torsion->value()));
    }

    QString argument = arguments.join(QLatin1String(", "));
    QString command = QStringLiteral("%1.addObject(\"Part::Spline\", \"Spline\").Shape = "
                                     "ReverseEngineering.approxCurve(%2).toShape()")
                          .arg(document, argument);

    tryCommand(command);
}

void FitBSplineCurveWidget::exeCommand(const QString& cmd)
{
    Gui::WaitCursor wc;
    Gui::Command::addModule(Gui::Command::App, "ReverseEngineering");
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Fit B-spline"));
    Gui::Command::runCommand(Gui::Command::Doc, cmd.toLatin1());
    Gui::Command::commitCommand();
    Gui::Command::updateActive();
}

void FitBSplineCurveWidget::tryCommand(const QString& cmd)
{
    try {
        exeCommand(cmd);
    }
    catch (const Base::Exception& e) {
        Gui::Command::abortCommand();
        e.reportException();
    }
}


void FitBSplineCurveWidget::changeEvent(QEvent* e)
{
    QWidget::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        d->ui.retranslateUi(this);
    }
}


/* TRANSLATOR ReenGui::TaskFitBSplineCurve */

TaskFitBSplineCurve::TaskFitBSplineCurve(const App::DocumentObjectT& obj)
    : widget {new FitBSplineCurveWidget(obj)}
{
    addTaskBox(widget);
}

void TaskFitBSplineCurve::open()
{}

bool TaskFitBSplineCurve::accept()
{
    return widget->accept();
}

#include "moc_FitBSplineCurve.cpp"
