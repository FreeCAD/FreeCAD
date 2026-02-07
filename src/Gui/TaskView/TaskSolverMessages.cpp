// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2011 Jürgen Riegel <juergen.riegel@web.de>              *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include <QAction>
#include <QMenu>

#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/ViewProvider.h>

#include "TaskSolverMessages.h"
#include "ui_TaskSolverMessages.h"


using namespace Gui;
using namespace Gui::TaskView;
namespace sp = std::placeholders;

TaskSolverMessages::TaskSolverMessages(const QPixmap& icon, const QString& title)
    : TaskBox(icon, title, true, nullptr)
    , ui(new Ui_TaskSolverMessages)
{
    // we need a separate container widget to add all controls to
    auto* proxy = new QWidget(this);
    ui->setupUi(proxy);
    setupConnections();

    this->groupLayout()->addWidget(proxy);

    ui->labelStatus->setOpenExternalLinks(false);

    // Set up the possible state values for the status label
    ui->labelStatus->setParameterGroup("User parameter:BaseApp/Preferences/Mod/Sketcher/General");
    ui->labelStatus->registerState(
        QStringLiteral("empty"),
        palette().windowText().color(),
        std::string("EmptySketchMessageColor")
    );
    ui->labelStatus->registerState(
        QStringLiteral("under_constrained"),
        palette().windowText().color(),
        std::string("UnderconstrainedMessageColor")
    );
    ui->labelStatus->registerState(
        QStringLiteral("malformed_constraints"),
        QColor("red"),
        std::string("MalformedConstraintMessageColor")
    );
    ui->labelStatus->registerState(
        QStringLiteral("conflicting_constraints"),
        QColor("orangered"),
        std::string("ConflictingConstraintMessageColor")
    );
    ui->labelStatus->registerState(
        QStringLiteral("redundant_constraints"),
        QColor("red"),
        std::string("RedundantConstraintMessageColor")
    );
    ui->labelStatus->registerState(
        QStringLiteral("partially_redundant_constraints"),
        QColor("royalblue"),
        std::string("PartiallyRedundantConstraintMessageColor")
    );
    ui->labelStatus->registerState(
        QStringLiteral("solver_failed"),
        QColor("red"),
        std::string("SolverFailedMessageColor")
    );
    ui->labelStatus->registerState(
        QStringLiteral("fully_constrained"),
        QColor("green"),
        std::string("FullyConstrainedMessageColor")
    );

    ui->labelStatusLink->setLaunchExternal(false);

    createSettingsButtonActions();
}

TaskSolverMessages::~TaskSolverMessages() = default;

void TaskSolverMessages::setupConnections()
{
    connect(
        ui->labelStatusLink,
        &Gui::UrlLabel::linkClicked,
        this,
        &TaskSolverMessages::onLabelStatusLinkClicked
    );
    connect(ui->manualUpdate, &QToolButton::clicked, this, &TaskSolverMessages::onManualUpdateClicked);
}

void TaskSolverMessages::slotSetUp(
    const QString& state,
    const QString& msg,
    const QString& link,
    const QString& linkText
)
{
    ui->labelStatus->setState(state);
    ui->labelStatus->setText(msg);
    ui->labelStatusLink->setUrl(link);
    ui->labelStatusLink->setText(linkText);
    updateToolTip(link);
}

void TaskSolverMessages::setLinkTooltip(const QString& tooltip)
{
    ui->labelStatusLink->setToolTip(tooltip);
}

QToolButton* TaskSolverMessages::getSettingsButton()
{
    return ui->settingsButton;
}

void TaskSolverMessages::onManualUpdateClicked(bool checked)
{
    Q_UNUSED(checked);
    Gui::Command::updateActive();
}

void TaskSolverMessages::createSettingsButtonActions()
{
    ui->settingsButton->hide();
}

#include "moc_TaskSolverMessages.cpp"
