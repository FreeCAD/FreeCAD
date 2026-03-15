// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2011 Jürgen Riegel <juergen.riegel@web.de>              *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
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

#pragma once

#include <fastsignals/signal.h>

#include <Gui/TaskView/TaskView.h>

class Ui_TaskSolverMessages;
using Connection = fastsignals::connection;

namespace App
{
class Property;
}

namespace Gui
{

class GuiExport TaskSolverMessages: public Gui::TaskView::TaskBox
{
    Q_OBJECT

public:
    explicit TaskSolverMessages(const QPixmap& icon, const QString& title);
    ~TaskSolverMessages() override;
    FC_DISABLE_COPY_MOVE(TaskSolverMessages)

    void slotSetUp(const QString& state, const QString& msg, const QString& link, const QString& linkText);

private:
    void setupConnections();
    virtual void createSettingsButtonActions();
    virtual void onLabelStatusLinkClicked(const QString&) = 0;
    void onManualUpdateClicked(bool checked);

    virtual void updateToolTip(const QString&) = 0;

protected:
    void setLinkTooltip(const QString& tooltip);
    QToolButton* getSettingsButton();

    Connection connectionSetUp;

private:
    std::unique_ptr<Ui_TaskSolverMessages> ui;
};

}  // namespace Gui
