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

#ifndef MATGUI_MATERIALSEDITOR_H
#define MATGUI_MATERIALSEDITOR_H

#include <boost/filesystem.hpp>

#include <QDialog>
#include <QDir>
#include <QStandardItem>
#include <QStyledItemDelegate>
#include <QSvgWidget>
#include <QTreeView>

#include <Mod/Material/App/MaterialManager.h>
#include <Mod/Material/App/Materials.h>
#include <Mod/Material/App/ModelManager.h>

namespace fs = boost::filesystem;

namespace MatGui
{

class Ui_MaterialsEditor;

class MaterialsEditor: public QDialog
{
    Q_OBJECT

public:
    explicit MaterialsEditor(QWidget* parent = nullptr);
    ~MaterialsEditor() override = default;

    void propertyChange(const QString& property, const QString value);
    void onFavourite(bool checked);
    void onURL(bool checked);
    void onPhysicalAdd(bool checked);
    void onAppearanceAdd(bool checked);
    void onOk(bool checked);
    void onCancel(bool checked);
    void onSave(bool checked);
    void accept() override;
    void reject() override;

    Materials::MaterialManager& getMaterialManager()
    {
        return _materialManager;
    }
    Materials::ModelManager& getModelManager()
    {
        return _modelManager;
    }

    void updateMaterialAppearance();
    void updateMaterialProperties();
    void updateMaterial();
    void onSelectMaterial(const QItemSelection& selected, const QItemSelection& deselected);

protected:
    int confirmSave(QWidget* parent);
    void saveMaterial();

private:
    std::unique_ptr<Ui_MaterialsEditor> ui;
    Materials::MaterialManager _materialManager;
    Materials::ModelManager _modelManager;
    Materials::Material _material;
    QSvgWidget* _rendered;
    QSvgWidget* _vectored;
    bool _edited;
    std::list<QString> _favorites;
    std::list<QString> _recents;
    int _recentMax;

    void getFavorites();
    void saveFavorites();
    void addFavorite(const QString& uuid);
    void removeFavorite(const QString& uuid);
    bool isFavorite(const QString& uuid) const;

    void getRecents();
    void saveRecents();
    void addRecent(const QString& uuid);
    bool isRecent(const QString& uuid) const;

    void updatePreview() const;
    QString getColorHash(const QString& colorString, int colorRange = 255) const;

    void addExpanded(QTreeView* tree, QStandardItem* parent, QStandardItem* child);
    void addExpanded(QTreeView* tree, QStandardItemModel* parent, QStandardItem* child);
    void addRecents(QStandardItem* parent);
    void addFavorites(QStandardItem* parent);
    void createPreviews();
    void createAppearanceTree();
    void createPhysicalTree();
    void createMaterialTree();
    void fillMaterialTree();
    void refreshMaterialTree();
    void
    addMaterials(QStandardItem& parent,
                 const std::shared_ptr<std::map<QString, Materials::MaterialTreeNode*>> modelTree,
                 const QIcon& folderIcon,
                 const QIcon& icon);
    bool isMaterial(const fs::path& p) const
    {
        return Materials::MaterialManager::isMaterial(p);
    }
};

}  // namespace MatGui

#endif  // MATGUI_MATERIALSEDITOR_H
