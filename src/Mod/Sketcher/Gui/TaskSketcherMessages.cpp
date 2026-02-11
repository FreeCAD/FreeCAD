// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/***************************************************************************
 *   Copyright (c) 2011 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include <QAction>
#include <QMenu>
#include <QCheckBox>

#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include "Command.h"
#include "TaskSketcherMessages.h"
#include "ViewProviderSketch.h"


// clang-format off
using namespace SketcherGui;
using namespace Gui::TaskView;
namespace sp = std::placeholders;

TaskSketcherMessages::TaskSketcherMessages(ViewProviderSketch* sketchView)
    : TaskSolverMessages(Gui::BitmapFactory().pixmap("Sketcher_Sketch"), tr("Sketch Edit"))
    , sketchView(sketchView)
{
    createSettingsButtonActions();

    //NOLINTBEGIN
    connectionSetUp = sketchView->signalSetUp.connect(std::bind(
        &SketcherGui::TaskSketcherMessages::slotSetUp, this, sp::_1, sp::_2, sp::_3, sp::_4));
    //NOLINTEND
}

TaskSketcherMessages::~TaskSketcherMessages()
{
    connectionSetUp.disconnect();
}

void TaskSketcherMessages::updateToolTip(const QString& link)
{
    if (link == QStringLiteral("#conflicting")) {
        setLinkTooltip(tr("Click to select these conflicting constraints."));
    }
    else if (link == QStringLiteral("#redundant")) {
        setLinkTooltip(tr("Click to select these redundant constraints."));
    }
    else if (link == QStringLiteral("#dofs")) {
        setLinkTooltip(tr("The sketch has unconstrained elements giving rise to those "
            "Degrees Of Freedom. Click to select these unconstrained elements."));
    }
    else if (link == QStringLiteral("#malformed")) {
        setLinkTooltip(tr("Click to select these malformed constraints."));
    }
    else if (link == QStringLiteral("#partiallyredundant")) {
        setLinkTooltip(
            tr("Some constraints in combination are partially redundant. Click to select these "
               "partially redundant constraints."));
    }
}

void TaskSketcherMessages::onLabelStatusLinkClicked(const QString& str)
{
    if (str == QStringLiteral("#conflicting")) {
        Gui::Application::Instance->commandManager().runCommandByName("Sketcher_SelectConflictingConstraints");
    }
    else if (str == QStringLiteral("#redundant")) {
        Gui::Application::Instance->commandManager().runCommandByName("Sketcher_SelectRedundantConstraints");
    }
    else if (str == QStringLiteral("#dofs")) {
        Gui::Application::Instance->commandManager().runCommandByName("Sketcher_SelectElementsWithDoFs");
    }
    else if (str == QStringLiteral("#malformed")) {
        Gui::Application::Instance->commandManager().runCommandByName("Sketcher_SelectMalformedConstraints");
    }
    else if (str == QStringLiteral("#partiallyredundant")) {
        Gui::Application::Instance->commandManager().runCommandByName("Sketcher_SelectPartiallyRedundantConstraints");
    }
}

void TaskSketcherMessages::createSettingsButtonActions()
{
    QToolButton* btn = getSettingsButton();
    btn->show();

    // Set Auto Update in the 'Manual Update' button menu.
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher");
    bool state = hGrp->GetBool("AutoRecompute", false);

    sketchView->getSketchObject()->noRecomputes = !state;

    auto* autoUpdateAction = new QWidgetAction(this);
    auto* containerWidget = new QWidget();
    auto* layout = new QGridLayout(containerWidget);
    auto* checkbox = new QCheckBox(tr("Auto-update"));
    checkbox->setToolTip(tr("Executes a recomputation of active document after every sketch action"));
    checkbox->setChecked(state);
    layout->addWidget(checkbox, 0, 0, 1, 2);
    containerWidget->setLayout(layout);
    autoUpdateAction->setDefaultWidget(containerWidget);

    connect(checkbox, &QCheckBox::toggled, this, [this](bool checked) {
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Sketcher");
        hGrp->SetBool("AutoRecompute", checked);
        sketchView->getSketchObject()->noRecomputes = !checked;
    });

    auto* gridAction = new GridSpaceAction(this);
    auto* snapAction = new SnapSpaceAction(this);
    auto* renderingAction = new RenderingOrderAction(this);

    QMenu* myMenu = new QMenu(this);
    myMenu->addAction(autoUpdateAction);
    myMenu->addSeparator();
    myMenu->addAction(gridAction);
    myMenu->addSeparator();
    myMenu->addAction(snapAction);
    myMenu->addSeparator();
    myMenu->addAction(renderingAction);
    btn->setMenu(myMenu);

    QObject::connect(myMenu, &QMenu::aboutToShow, [gridAction, snapAction, renderingAction]() {
        gridAction->updateWidget();
        snapAction->updateWidget(true);
        renderingAction->updateWidget();
    });

    QObject::connect(btn, &QToolButton::clicked, btn, &QToolButton::showMenu);
}

#include "moc_TaskSketcherMessages.cpp"
// clang-format on
