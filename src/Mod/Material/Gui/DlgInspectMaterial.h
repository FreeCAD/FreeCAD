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

#include <QStandardItem>
#include <QStandardItemModel>
#include <QTreeView>

#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>

#include <Mod/Material/App/MaterialManager.h>
#include <Mod/Material/App/ModelManager.h>

namespace Gui
{
class ViewProvider;
}

namespace MatGui
{
class Ui_DlgInspectMaterial;

class DlgInspectMaterial: public QWidget, public Gui::SelectionSingleton::ObserverType
{
    Q_OBJECT

public:
    explicit DlgInspectMaterial(QWidget* parent = nullptr);
    ~DlgInspectMaterial() override;

    bool accept();
    void onClipboard(bool checked);

    /// Observer message from the Selection
    void OnChange(Gui::SelectionSingleton::SubjectType& rCaller,
                  Gui::SelectionSingleton::MessageType Reason) override;

private:
    std::unique_ptr<Ui_DlgInspectMaterial> ui;
    QString clipboardText;
    int clipboardIndent;

    void appendClip(QString text);
    QStandardItem* clipItem(QString text);
    void indent();
    void unindent();

    std::vector<Gui::ViewProvider*> getSelection() const;
    void update(std::vector<Gui::ViewProvider*>& views);
    void updateMaterialTree(const Materials::Material& material);
    void
    addMaterial(QTreeView* tree, QStandardItemModel* parent, const Materials::Material& material);
    void addMaterial(QTreeView* tree, QStandardItem* parent, const Materials::Material& material);
    void
    addMaterialDetails(QTreeView* tree, QStandardItem* parent, const Materials::Material& material);
    void addModels(QTreeView* tree, QStandardItem* parent, const QSet<QString>* models);
    void addModelDetails(QTreeView* tree,
                         QStandardItem* parent,
                         std::shared_ptr<Materials::Model>& model);
    void addProperties(
        QTreeView* tree,
        QStandardItem* parent,
        const std::map<QString, std::shared_ptr<Materials::MaterialProperty>>& properties);
    void addPropertyDetails(QTreeView* tree,
                            QStandardItem* parent,
                            const std::shared_ptr<Materials::MaterialProperty>& property);

    void addExpanded(QTreeView* tree, QStandardItemModel* parent, QStandardItem* child);
    void addExpanded(QTreeView* tree, QStandardItem* parent, QStandardItem* child);
};


class TaskInspectMaterial: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskInspectMaterial();
    ~TaskInspectMaterial() override;

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
    DlgInspectMaterial* widget;
};

}  // namespace MatGui