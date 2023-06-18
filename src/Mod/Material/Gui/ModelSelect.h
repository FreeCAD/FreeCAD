/***************************************************************************
 *   Copyright (c) 2023 David Carter <dcarter@david.carter.ca>             *
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

#ifndef MATGUI_MODELSELECT_H
#define MATGUI_MODELSELECT_H

#include <boost/filesystem.hpp>

#include <QDialog>
#include <QDir>
#include <QStandardItem>
#include <QTableView>
#include <QTreeView>

#include <Mod/Material/App/Materials.h>

namespace fs = boost::filesystem;

namespace MatGui {

class Ui_ModelSelect;

class ModelSelect : public QDialog
{
    Q_OBJECT

public:
    explicit ModelSelect(QWidget* parent = nullptr);
    ~ModelSelect() override;

    void onSelectModel(const QItemSelection& selected, const QItemSelection& deselected);
    void accept() override;
    void reject() override;

private:
    void addExpanded(QTreeView* tree, QStandardItem* parent, QStandardItem* child);
    void addExpanded(QTreeView *tree, QStandardItemModel *parent, QStandardItem *child);
    void addModels(QStandardItem& parent, std::map<std::string, void*>* modelTree,
                   const QIcon& icon);
    void updateMaterialModel(const std::string& uuid);
    void clearMaterialModel(void);
    void createModelTree();
    void setHeaders(QStandardItemModel *model);
    void setColumnWidths(QTableView *table);
    void updateModelProperties(const Materials::Model& model);
    void createModelProperties();
    Materials::ModelManager &getModelManager() { return _modelManager; }

    std::unique_ptr<Ui_ModelSelect> ui;
    Materials::ModelManager _modelManager;
};

} // namespace MatGui

#endif // MATGUI_MODELSELECT_H
