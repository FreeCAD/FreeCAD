/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef MESHGUI_REMOVECOMPONENTS_H
#define MESHGUI_REMOVECOMPONENTS_H

#include <QDialog>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Mod/Mesh/MeshGlobal.h>
#include "MeshSelection.h"

namespace MeshGui
{
class Ui_RemoveComponents;

/**
 * Non-modal dialog to de/select components, regions, the complete or single faces
 * of a mesh and delete them.
 * @author Werner Mayer
 */
class MeshGuiExport RemoveComponents: public QWidget
{
    Q_OBJECT

public:
    explicit RemoveComponents(QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());
    ~RemoveComponents() override;
    void reject();
    void deleteSelection();
    void invertSelection();

public:
    void onSelectRegionClicked();
    void onSelectAllClicked();
    void onSelectComponentsClicked();
    void onSelectTriangleClicked();
    void onDeselectRegionClicked();
    void onDeselectAllClicked();
    void onDeselectComponentsClicked();
    void onDeselectTriangleClicked();
    void onVisibleTrianglesToggled(bool);
    void onScreenTrianglesToggled(bool);
    void onSelectCompToggled(bool);
    void onDeselectCompToggled(bool);

protected:
    void changeEvent(QEvent* e) override;

private:
    void setupConnections();

private:
    Ui_RemoveComponents* ui;
    MeshSelection meshSel;

    Q_DISABLE_COPY_MOVE(RemoveComponents)
};

/**
 * Embed the panel into a dialog.
 */
class MeshGuiExport RemoveComponentsDialog: public QDialog
{
    Q_OBJECT

public:
    explicit RemoveComponentsDialog(QWidget* parent = nullptr,
                                    Qt::WindowFlags fl = Qt::WindowFlags());
    ~RemoveComponentsDialog() override;
    void reject() override;

private Q_SLOTS:
    void clicked(QAbstractButton* btn);

private:
    RemoveComponents* widget;

    Q_DISABLE_COPY_MOVE(RemoveComponentsDialog)
};

/**
 * Embed the panel into a task dialog.
 */
class TaskRemoveComponents: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskRemoveComponents();

public:
    bool accept() override;
    void clicked(int) override;

    QDialogButtonBox::StandardButtons getStandardButtons() const override
    {
        return QDialogButtonBox::Ok | QDialogButtonBox::Close;
    }
    bool isAllowedAlterDocument() const override
    {
        return true;
    }
    void modifyStandardButtons(QDialogButtonBox*) override;

private:
    RemoveComponents* widget;
    Gui::TaskView::TaskBox* taskbox;
};

}  // namespace MeshGui

#endif  // MESHGUI_REMOVECOMPONENTS_H
