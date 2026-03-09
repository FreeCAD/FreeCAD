// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2024 David Carter <dcarter@david.carter.ca>             *
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

#pragma once

#include <memory>

#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>

namespace App
{
class Color;
}

namespace Gui
{
class ViewProvider;
}

namespace MatGui
{
class Ui_DlgInspectAppearance;

class ColorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ColorWidget(const Base::Color& color, QWidget* parent = nullptr);
    ~ColorWidget() override = default;

    QSize sizeHint() const override { return {75,23}; }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QColor _color;
};

class DlgInspectAppearance: public QWidget, public Gui::SelectionSingleton::ObserverType
{
    Q_OBJECT

public:
    explicit DlgInspectAppearance(QWidget* parent = nullptr);
    ~DlgInspectAppearance() override;

    bool accept();
    // bool reject();

    /// Observer message from the Selection
    void OnChange(Gui::SelectionSingleton::SubjectType& rCaller,
                  Gui::SelectionSingleton::MessageType Reason) override;

protected:

private:
    std::unique_ptr<Ui_DlgInspectAppearance> ui;

    std::vector<Gui::ViewProvider*> getSelection() const;
    void update(std::vector<Gui::ViewProvider*>& views);
    QWidget* makeAppearanceTab(const App::Material& material);
};


class TaskInspectAppearance: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskInspectAppearance();
    ~TaskInspectAppearance() override;

public:
    void open() override;
    bool accept() override;
    // bool reject() override;
    void clicked(int) override;

    QDialogButtonBox::StandardButtons getStandardButtons() const override
    {
        return QDialogButtonBox::Ok;
    }

private:
    DlgInspectAppearance* widget;
};

}  // namespace MatGui