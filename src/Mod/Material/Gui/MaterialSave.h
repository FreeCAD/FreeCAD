/***************************************************************************
 *   Copyright (c) 2023 David Carter <dcarter@david.carter.ca>             *
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

#ifndef MATGUI_MATERIALSAVE_H
#define MATGUI_MATERIALSAVE_H

// #include <boost/filesystem.hpp>

#include <QDialog>
#include <QItemSelection>
#include <QStandardItem>
#include <QTreeView>

#include <Mod/Material/App/MaterialManager.h>

namespace MatGui
{

class Ui_MaterialSave;

class MaterialSave: public QDialog
{
    Q_OBJECT

public:
    explicit MaterialSave(Materials::Material* material, QWidget* parent = nullptr);
    ~MaterialSave() override;

    void setLibraries();
    void createModelTree();
    void addExpanded(QTreeView* tree, QStandardItem* parent, QStandardItem* child);
    void addExpanded(QTreeView* tree, QStandardItemModel* parent, QStandardItem* child);
    void
    addMaterials(QStandardItem& parent,
                 const std::shared_ptr<std::map<QString, Materials::MaterialTreeNode*>> modelTree,
                 const QIcon& folderIcon,
                 const QIcon& icon);
    void showSelectedTree();

    void onSelectModel(const QItemSelection& selected, const QItemSelection& deselected);
    void currentTextChanged(const QString& value);
    void onNewFolder(bool checked);
    void onFilename(const QString& text);
    void onOk(bool checked);
    void onCancel(bool checked);
    void accept() override;
    void reject() override;

private:
    std::unique_ptr<Ui_MaterialSave> ui;
    Materials::MaterialManager _manager;
    Materials::Material* _material;
    QString _selectedPath;
    QString _selectedUUID;
    QString _libraryName;
    QString _filename;

    QString getPath(const QStandardItem* item) const;
};

}  // namespace MatGui

#endif  // MATGUI_MATERIALSAVE_H
