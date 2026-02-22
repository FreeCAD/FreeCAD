// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef GUI_DIALOGS_DLGANNOTATION_H
#define GUI_DIALOGS_DLGANNOTATION_H

#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <App/DocumentObserver.h>
#include <QDialog>
#include <memory>

namespace Gui
{
namespace Dialog
{

class Ui_DlgAnnotation;
class GuiExport DlgAnnotation: public QDialog
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(DlgAnnotation)

public:
    explicit DlgAnnotation(App::Document* doc, QWidget* parent = nullptr);
    ~DlgAnnotation() override;
    void accept() override;
    void reject() override;

protected:
    void changeEvent(QEvent* event) override;

private:
    void ensureTransaction();
    void createAnnotation();
    struct Position
    {
        Base::Vector3d base;
        Base::Vector3d text;
    };
    Position getPosition() const;

private:
    std::unique_ptr<Ui_DlgAnnotation> ui;
    App::DocumentWeakPtrT document;
};

class TaskAnnotation: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    explicit TaskAnnotation(App::Document* doc);
    ~TaskAnnotation() override;

public:
    bool accept() override;
    bool reject() override;

    QDialogButtonBox::StandardButtons getStandardButtons() const override
    {
        return QDialogButtonBox::Ok | QDialogButtonBox::Cancel;
    }

private:
    DlgAnnotation* dialog;
};

}  // namespace Dialog
}  // namespace Gui

#endif  // GUI_DIALOGS_DLGANNOTATION_H
