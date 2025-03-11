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

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QAction>
#endif

#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include "TaskSketcherGeneral.h"
#include "ViewProviderSketch.h"
#include "ui_TaskSketcherGeneral.h"


// clang-format off
using namespace SketcherGui;
using namespace Gui::TaskView;
namespace sp = std::placeholders;

TaskSketcherGeneral::TaskSketcherGeneral(ViewProviderSketch* sketchView)
    : TaskBox(Gui::BitmapFactory().pixmap("Sketcher_Sketch"), tr("General"), true, nullptr)
    , sketchView(sketchView)
    , ui(new Ui_TaskSketcherGeneral)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui->setupUi(proxy);
    setupConnections();

    this->groupLayout()->addWidget(proxy);

    //NOLINTBEGIN
    connectionSetUp = sketchView->signalSetUp.connect(std::bind(
        &SketcherGui::TaskSketcherGeneral::slotSetUp, this, sp::_1, sp::_2, sp::_3, sp::_4));
    //NOLINTEND

    ui->labelConstrainStatus->setOpenExternalLinks(false);

    // Set up the possible state values for the status label
    ui->labelConstrainStatus->setParameterGroup(
        "User parameter:BaseApp/Preferences/Mod/Sketcher/General");
    ui->labelConstrainStatus->registerState(QStringLiteral("empty_sketch"),
                                            palette().windowText().color(),
                                            std::string("EmptySketchMessageColor"));
    ui->labelConstrainStatus->registerState(QStringLiteral("under_constrained"),
                                            palette().windowText().color(),
                                            std::string("UnderconstrainedMessageColor"));
    ui->labelConstrainStatus->registerState(QStringLiteral("malformed_constraints"),
                                            QColor("red"),
                                            std::string("MalformedConstraintMessageColor"));
    ui->labelConstrainStatus->registerState(QStringLiteral("conflicting_constraints"),
                                            QColor("orangered"),
                                            std::string("ConflictingConstraintMessageColor"));
    ui->labelConstrainStatus->registerState(QStringLiteral("redundant_constraints"),
                                            QColor("red"),
                                            std::string("RedundantConstraintMessageColor"));
    ui->labelConstrainStatus->registerState(
        QStringLiteral("partially_redundant_constraints"),
        QColor("royalblue"),
        std::string("PartiallyRedundantConstraintMessageColor"));
    ui->labelConstrainStatus->registerState(
        QStringLiteral("solver_failed"), QColor("red"), std::string("SolverFailedMessageColor"));
    ui->labelConstrainStatus->registerState(QStringLiteral("fully_constrained"),
                                            QColor("green"),
                                            std::string("FullyConstrainedMessageColor"));

    ui->labelConstrainStatusLink->setLaunchExternal(false);

    // Set up Auto Recompute
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher");
    bool state = hGrp->GetBool("AutoRecompute", false);
    sketchView->getSketchObject()->noRecomputes = !state;

    ui->autoRecompute->setToolTip(tr(
        "If checked, the whole document is recomputed after every sketch action. "
        "Not advisable for complex documents."));
    ui->autoRecompute->setChecked(state);
    connect(ui->autoRecompute, &QCheckBox::stateChanged, this, &TaskSketcherGeneral::onAutoRecomputeStateChanged);

}

TaskSketcherGeneral::~TaskSketcherGeneral()
{
    connectionSetUp.disconnect();
}

void TaskSketcherGeneral::setupConnections()
{
    connect(ui->labelConstrainStatusLink,
            &Gui::UrlLabel::linkClicked,
            this,
            &TaskSketcherGeneral::onLabelConstrainStatusLinkClicked);
    connect(ui->autoRecompute,
            &QCheckBox::stateChanged,
            this,
            &TaskSketcherGeneral::onAutoRecomputeStateChanged);
}

void TaskSketcherGeneral::slotSetUp(const QString& state, const QString& msg, const QString& link,
                                     const QString& linkText)
{
    ui->labelConstrainStatus->setState(state);
    ui->labelConstrainStatus->setText(msg);
    ui->labelConstrainStatusLink->setUrl(link);
    ui->labelConstrainStatusLink->setText(linkText);
    updateToolTip(link);
}

void TaskSketcherGeneral::updateToolTip(const QString& link)
{
    if (link == QStringLiteral("#conflicting"))
        ui->labelConstrainStatusLink->setToolTip(
            tr("Click to select these conflicting constraints."));
    else if (link == QStringLiteral("#redundant"))
        ui->labelConstrainStatusLink->setToolTip(tr("Click to select these redundant constraints."));
    else if (link == QStringLiteral("#dofs"))
        ui->labelConstrainStatusLink->setToolTip(
            tr("The sketch has unconstrained elements giving rise to those Degrees Of Freedom. "
               "Click to select these unconstrained elements."));
    else if (link == QStringLiteral("#malformed"))
        ui->labelConstrainStatusLink->setToolTip(tr("Click to select these malformed constraints."));
    else if (link == QStringLiteral("#partiallyredundant"))
        ui->labelConstrainStatusLink->setToolTip(
            tr("Some constraints in combination are partially redundant. Click to select these "
               "partially redundant constraints."));
}

void TaskSketcherGeneral::onLabelConstrainStatusLinkClicked(const QString& str)
{
    if (str == QStringLiteral("#conflicting"))
        Gui::Application::Instance->commandManager().runCommandByName(
            "Sketcher_SelectConflictingConstraints");
    else if (str == QStringLiteral("#redundant"))
        Gui::Application::Instance->commandManager().runCommandByName(
            "Sketcher_SelectRedundantConstraints");
    else if (str == QStringLiteral("#dofs"))
        Gui::Application::Instance->commandManager().runCommandByName(
            "Sketcher_SelectElementsWithDoFs");
    else if (str == QStringLiteral("#malformed"))
        Gui::Application::Instance->commandManager().runCommandByName(
            "Sketcher_SelectMalformedConstraints");
    else if (str == QStringLiteral("#partiallyredundant"))
        Gui::Application::Instance->commandManager().runCommandByName(
            "Sketcher_SelectPartiallyRedundantConstraints");
}

void TaskSketcherGeneral::onAutoRecomputeStateChanged()
{
    bool state = ui->autoRecompute->isChecked();

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher");
    hGrp->SetBool("AutoRecompute", state);
    sketchView->getSketchObject()->noRecomputes = !state;
}

#include "moc_TaskSketcherGeneral.cpp"
// clang-format on
