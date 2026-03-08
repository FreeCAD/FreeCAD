// SPDX-License-Identifier: LGPL-2.1-or-later

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

#pragma once

#include <memory>

#include <QDialog>
#include <QDir>
#include <QIcon>
#include <QPoint>
#include <QStandardItem>
#include <QStyledItemDelegate>
#include <QSvgWidget>
#include <QTreeView>

#include <Base/Handle.h>
#include <Base/Parameter.h>

#include <Mod/Material/App/MaterialFilter.h>
#include <Mod/Material/App/MaterialManager.h>
#include <Mod/Material/App/Materials.h>
#include <Mod/Material/App/ModelManager.h>

#include "AppearancePreview.h"

namespace MatGui
{

class Ui_MaterialsEditor;

class MaterialsEditor: public QDialog
{
    Q_OBJECT

public:
    explicit MaterialsEditor(Materials::MaterialFilter filter,
                             QWidget* parent = nullptr);
    explicit MaterialsEditor(QWidget* parent = nullptr);
    ~MaterialsEditor() override = default;

    void onName(const QString& text);
    void onAuthor(const QString& text);
    void onLicense(const QString& text);
    void onSourceURL(const QString& text);
    void onSourceReference(const QString& text);
    void onDescription();

    void propertyChange(const QString& property, const QVariant& value);
    void onInheritNewMaterial(bool checked);
    void onNewMaterial(bool checked);
    void onFavourite(bool checked);
    void onURL(bool checked);
    void onPhysicalAdd(bool checked);
    void onPhysicalRemove(bool checked);
    void onAppearanceAdd(bool checked);
    void onAppearanceRemove(bool checked);
    void onOk(bool checked);
    void onCancel(bool checked);
    void onSave(bool checked);
    void accept() override;
    void reject() override;

    Materials::MaterialManager& getMaterialManager()
    {
        return Materials::MaterialManager::getManager();
    }

    static QString libraryPath(const std::shared_ptr<Materials::Material>& material);

    static QIcon getIcon(const std::shared_ptr<Materials::MaterialLibrary>& library);
    static QIcon getIcon(const std::shared_ptr<Materials::ModelLibrary>& library);
    static QIcon getIcon(const std::shared_ptr<Materials::Library>& library);

    void updateMaterialAppearance();
    void updateMaterialProperties();
    void updateMaterialGeneral();
    void updateMaterial();
    void onSelectMaterial(const QItemSelection& selected, const QItemSelection& deselected);
    void onDoubleClick(const QModelIndex& index);
    void onContextMenu(const QPoint& pos);

    bool isMaterialSelected() const
    {
        return _materialSelected;
    }
    std::shared_ptr<Materials::Material> getMaterial()
    {
        return _material;
    }

protected:
    int confirmSave(QWidget* parent);
    void saveMaterial();

private:
    std::unique_ptr<Ui_MaterialsEditor> ui;
    std::shared_ptr<Materials::Material> _material;
    AppearancePreview* _rendered;
    bool _materialSelected;
    std::list<QString> _favorites;
    std::list<QString> _recents;
    int _recentMax;
    QIcon _warningIcon;
    Materials::MaterialFilter _filter;
    Materials::MaterialFilterOptions _filterOptions;

    void setup();

    void saveWindow();
    void saveMaterialTreeChildren(const Base::Reference<ParameterGrp>& param,
                                  QTreeView* tree,
                                  QStandardItemModel* model,
                                  QStandardItem* item);
    void saveMaterialTree(const Base::Reference<ParameterGrp>& param);

    void oldFormatError();

    void getFavorites();
    void saveFavorites();
    void addFavorite(const QString& uuid);
    void removeFavorite(const QString& uuid);
    bool isFavorite(const QString& uuid) const;

    void getRecents();
    void saveRecents();
    void addRecent(const QString& uuid);
    bool isRecent(const QString& uuid) const;

    void onInherit(bool checked);
    void onInheritNew(bool checked);

    void setMaterialDefaults();
    bool updateTexturePreview() const;
    bool updateMaterialPreview() const;
    void updatePreview() const;
    static QString getColorHash(const QString& colorString);

    static void addExpanded(QTreeView* tree, QStandardItem* parent, QStandardItem* child);
    static void addExpanded(QTreeView* tree,
                            QStandardItem* parent,
                            QStandardItem* child,
                            const Base::Reference<ParameterGrp>& param);
    static void addExpanded(QTreeView* tree, QStandardItemModel* parent, QStandardItem* child);
    static void addExpanded(QTreeView* tree,
                            QStandardItemModel* parent,
                            QStandardItem* child,
                            const Base::Reference<ParameterGrp>& param);
    void addRecents(QStandardItem* parent);
    void addFavorites(QStandardItem* parent);
    void createPreviews();
    void createAppearanceTree();
    void createPhysicalTree();
    void createMaterialTree();
    void fillMaterialTree();
    void refreshMaterialTree();
    void addMaterials(
        QStandardItem& parent,
        const std::shared_ptr<std::map<QString, std::shared_ptr<Materials::MaterialTreeNode>>>
            modelTree,
        const QIcon& folderIcon,
        const QIcon& icon,
        const Base::Reference<ParameterGrp>& param);

    /* Indicates if we should show favourite materials
     */
    bool includeFavorites() const
    {
        return _filterOptions.includeFavorites();
    }
    void setIncludeFavorites(bool value)
    {
        _filterOptions.setIncludeFavorites(value);
    }

    /* Indicates if we should show recent materials
     */
    bool includeRecent() const
    {
        return _filterOptions.includeRecent();
    }
    void setIncludeRecent(bool value)
    {
        _filterOptions.setIncludeRecent(value);
    }

    /* Indicates if we should include empty folders
     */
    bool includeEmptyFolders() const
    {
        return _filterOptions.includeEmptyFolders();
    }
    void setIncludeEmptyFolders(bool value)
    {
        _filterOptions.setIncludeEmptyFolders(value);
    }

    /* Indicates if we should include empty libraries
     */
    bool includeEmptyLibraries() const
    {
        return _filterOptions.includeEmptyLibraries();
    }
    void setIncludeEmptyLibraries(bool value)
    {
        Base::Console().log("setIncludeEmptyLibraries(%s)\n", (value ? "true" : "false"));
        _filterOptions.setIncludeEmptyLibraries(value);
    }

    /* Indicates if we should include materials in the older format
     */
    bool includeLegacy() const
    {
        return _filterOptions.includeLegacy();
    }
    void setIncludeLegacy(bool legacy)
    {
        _filterOptions.setIncludeLegacy(legacy);
    }
};

}  // namespace MatGui