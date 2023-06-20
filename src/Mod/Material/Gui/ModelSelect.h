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
    explicit ModelSelect(QWidget* parent = nullptr, Materials::ModelManager::ModelFilter filter=Materials::ModelManager::ModelFilter_None);
    ~ModelSelect() override;

    void onURL(bool checked);
    void onDOI(bool checked);
    void onFavourite(bool checked);
    void onSelectModel(const QItemSelection& selected, const QItemSelection& deselected);
    const std::string &selectedModel() const { return _selected; }
    void accept() override;
    void reject() override;

private:
    void getFavorites();
    void saveFavorites();
    void addFavorite(const std::string& uuid);
    void removeFavorite(const std::string& uuid);
    bool isFavorite(const std::string& uuid) const;
    
    void getRecents();
    void saveRecents();
    void addRecent(const std::string& uuid);
    bool isRecent(const std::string& uuid) const;

    void addExpanded(QTreeView* tree, QStandardItem* parent, QStandardItem* child);
    void addExpanded(QTreeView *tree, QStandardItemModel *parent, QStandardItem *child);
    void addRecents(QStandardItem* parent);
    void addFavorites(QStandardItem *parent);
    void addModels(QStandardItem& parent, const std::map<std::string, Materials::ModelTreeNode*>* modelTree,
                   const QIcon& icon);
    void updateMaterialModel(const std::string& uuid);
    void clearMaterialModel(void);
    void createModelTree();
    void refreshModelTree();
    void fillTree();

    void setHeaders(QStandardItemModel *model);
    void setColumnWidths(QTableView *table);
    void updateModelProperties(const Materials::Model& model);
    void createModelProperties();
    Materials::ModelManager &getModelManager() { return *Materials::ModelManager::getManager(); }

    Materials::ModelManager::ModelFilter _filter;
    std::unique_ptr<Ui_ModelSelect> ui;
    std::string _selected;
    std::list<std::string> _favorites;
    std::list<std::string> _recents;
    int _recentMax;
};

} // namespace MatGui

#endif // MATGUI_MODELSELECT_H
