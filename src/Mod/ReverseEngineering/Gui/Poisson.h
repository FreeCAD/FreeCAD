// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   © 2015 Werner Mayer <wmayer@users.sourceforge.net>                       *
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/


#ifndef REENGUI_POISSON_H
#define REENGUI_POISSON_H

#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>


namespace ReenGui
{

class PoissonWidget: public QWidget
{
    Q_OBJECT

public:
    explicit PoissonWidget(const App::DocumentObjectT&, QWidget* parent = nullptr);
    ~PoissonWidget() override;

    bool accept();

private:
    void changeEvent(QEvent* e) override;

private:
    class Private;
    Private* d;
};

class TaskPoisson: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    explicit TaskPoisson(const App::DocumentObjectT&);
    ~TaskPoisson() override;

public:
    void open() override;
    bool accept() override;

    QDialogButtonBox::StandardButtons getStandardButtons() const override
    {
        return QDialogButtonBox::Ok | QDialogButtonBox::Cancel;
    }

private:
    PoissonWidget* widget;
};

}  // namespace ReenGui

#endif  // REENGUI_POISSON_H
