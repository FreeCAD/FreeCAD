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

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QContextMenuEvent>
#include <QMenu>
#endif

#include <cstring>

#include <QHBoxLayout>
#include <QSpacerItem>
#include <QVBoxLayout>

#include <App/Color.h>
#include <Base/Console.h>
#include <Base/Tools.h>
#include <Gui/Command.h>

#include <Mod/Material/App/Exceptions.h>
#include <Mod/Material/App/MaterialFilter.h>
#include <Mod/Material/App/ModelUuids.h>

#include "MaterialTreeWidget.h"
#include "MaterialsEditor.h"
#include "ui_MaterialsEditor.h"


using Base::Console;
using namespace MatGui;

/** Constructs a Material tree widget.
 */

MaterialTreeWidget::MaterialTreeWidget(std::shared_ptr<Materials::MaterialFilter> filter,
                                       QWidget* parent)
    : QWidget(parent)
    , m_expanded(false)
    , _filter(filter)
{
    setup();
}

MaterialTreeWidget::MaterialTreeWidget(QWidget* parent)
    : QWidget(parent)
    , m_expanded(false)
    , _filter(nullptr)
{
    setup();
}

void MaterialTreeWidget::setup()
{
    getFavorites();
    getRecents();

    createLayout();
    createMaterialTree();
}

/**
 * Destroys the widget and detaches it from its parameter group.
 */
MaterialTreeWidget::~MaterialTreeWidget() = default;

void MaterialTreeWidget::createLayout()
{
    m_material = new QLineEdit(this);
    m_expand = new QPushButton(this);
    m_expand->setIcon(style()->standardIcon(QStyle::SP_TitleBarUnshadeButton));
    m_materialTree = new QTreeView(this);
    m_editor = new QPushButton(tr("Launch editor"), this);

    // m_materialTree->setSelectionModel(QAbstractItemView::SingleSelection);
    m_materialTree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_materialTree->setSelectionBehavior(QAbstractItemView::SelectItems);

    auto materialLayout = new QHBoxLayout();
    materialLayout->addWidget(m_material);
    materialLayout->addWidget(m_expand);

    auto treeLayout = new QHBoxLayout();
    treeLayout->addWidget(m_materialTree);

    auto buttonLayout = new QHBoxLayout();
    buttonLayout->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Preferred));
    buttonLayout->addWidget(m_editor);

    auto layout = new QVBoxLayout();
    layout->setContentsMargins(0, 9, 0, 9);
    layout->addItem(materialLayout);
    layout->addItem(treeLayout);
    layout->addItem(buttonLayout);
    setLayout(layout);

    // Start in an unexpanded state. Store the state?
    openWidgetState(false);

    connect(m_expand, &QPushButton::clicked, this, &MaterialTreeWidget::expandClicked);
    connect(m_editor, &QPushButton::clicked, this, &MaterialTreeWidget::editorClicked);
}

void MaterialTreeWidget::openWidgetState(bool open)
{
    m_materialTree->setVisible(open);
    m_editor->setVisible(open);

    m_expanded = open;

    if (open) {
        m_expand->setIcon(style()->standardIcon(QStyle::SP_TitleBarShadeButton));
    }
    else {
        m_expand->setIcon(style()->standardIcon(QStyle::SP_TitleBarUnshadeButton));
    }
}

void MaterialTreeWidget::expandClicked(bool checked)
{
    Q_UNUSED(checked)

    // Toggle the open state
    openWidgetState(!m_expanded);
}

void MaterialTreeWidget::editorClicked(bool checked)
{
    Q_UNUSED(checked)

    MaterialsEditor dialog(_filter, this);
    dialog.setModal(true);
    if (dialog.exec() == QDialog::Accepted) {
        // updateMaterialGeneral();
        // _material->resetEditState();
        // refreshMaterialTree();
        // _materialSelected = true;
        auto material = dialog.getMaterial();
        updateMaterialTree();
        setMaterial(material->getUUID());
    }

    // Gui::Application::Instance->commandManager().runCommandByName("Materials_Edit");
    // Toggle the open state
    // openWidgetState(!m_expanded);
}

void MaterialTreeWidget::updateMaterial(const QString& uuid)
{
    if (uuid.isEmpty() || uuid == m_uuid) {
        return;
    }

    m_uuid = uuid;

    // Fetch the material from the manager
    auto material = std::make_shared<Materials::Material>();
    try {
        material = std::make_shared<Materials::Material>(*getMaterialManager().getMaterial(uuid));
    }
    catch (Materials::ModelNotFound const&) {
        Base::Console().Log("*** Unable to load material '%s'\n", uuid.toStdString().c_str());
    }

    m_materialDisplay = material->getName();
    m_material->setText(m_materialDisplay);
}

bool MaterialTreeWidget::findInTree(const QStandardItem& node,
                                    QModelIndex* index,
                                    const QString& uuid)
{
    auto vv = node.data(Qt::UserRole);
    if (vv.isValid() && vv == uuid) {
        *index = node.index();
        return true;
    }

    if (node.hasChildren()) {
        int rows = node.rowCount();
        for (int i = 0; i < node.rowCount(); i++) {
            auto child = node.child(i);
            if (findInTree(*child, index, uuid)) {
                return true;
            }
        }
    }

    return false;
}

QModelIndex MaterialTreeWidget::findInTree(const QString& uuid)
{
    auto model = dynamic_cast<QStandardItemModel*>(m_materialTree->model());
    auto root = model->invisibleRootItem();

    QModelIndex index;
    if (findInTree(*root, &index, uuid)) {
        return index;
    }

    return {};
}

void MaterialTreeWidget::setMaterial(const QString& uuid)
{
    if (uuid.isEmpty() || uuid == m_uuid) {
        return;
    }
    updateMaterial(uuid);

    // Now select the material in the tree
    auto index = findInTree(uuid);
    if (index.isValid()) {
        QItemSelectionModel* selectionModel = m_materialTree->selectionModel();
        selectionModel->select(index, QItemSelectionModel::SelectCurrent);
    }
}

QString MaterialTreeWidget::getMaterialUUID() const
{
    return m_uuid;
}

void MaterialTreeWidget::setFilter(std::shared_ptr<Materials::MaterialFilter> filter)
{
    _filter.reset();
    _filter = filter;

    updateMaterialTree();
}

void MaterialTreeWidget::updateMaterialTree()
{
    _favorites.clear();
    _recents.clear();

    auto model = dynamic_cast<QStandardItemModel*>(m_materialTree->model());
    model->clear();

    getFavorites();
    getRecents();
    fillMaterialTree();
}

void MaterialTreeWidget::getFavorites()
{
    _favorites.clear();

    auto param = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/Favorites");
    auto count = param->GetInt("Favorites", 0);
    for (int i = 0; static_cast<long>(i) < count; i++) {
        QString key = QString::fromLatin1("FAV%1").arg(i);
        QString uuid = QString::fromStdString(param->GetASCII(key.toStdString().c_str(), ""));
        if (!_filter || _filter->modelIncluded(uuid)) {
            _favorites.push_back(uuid);
        }
    }
}

void MaterialTreeWidget::getRecents()
{
    _recents.clear();

    auto param = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/Recent");
    _recentMax = static_cast<int>(param->GetInt("RecentMax", 5));
    auto count = param->GetInt("Recent", 0);
    for (int i = 0; static_cast<long>(i) < count; i++) {
        QString key = QString::fromLatin1("MRU%1").arg(i);
        QString uuid = QString::fromStdString(param->GetASCII(key.toStdString().c_str(), ""));
        if (!_filter || _filter->modelIncluded(uuid)) {
            _recents.push_back(uuid);
        }
    }
}

void MaterialTreeWidget::createMaterialTree()
{
    auto model = new QStandardItemModel(this);
    m_materialTree->setModel(model);
    m_materialTree->setHeaderHidden(true);

    // This needs to be done after the model is set
    QItemSelectionModel* selectionModel = m_materialTree->selectionModel();
    connect(selectionModel,
            &QItemSelectionModel::selectionChanged,
            this,
            &MaterialTreeWidget::onSelectMaterial);
    connect(m_materialTree, &QTreeView::doubleClicked, this, &MaterialTreeWidget::onDoubleClick);

    fillMaterialTree();
}

void MaterialTreeWidget::fillMaterialTree()
{
    auto model = dynamic_cast<QStandardItemModel*>(m_materialTree->model());

    auto lib = new QStandardItem(tr("Favorites"));
    lib->setFlags(Qt::ItemIsEnabled);
    addExpanded(model, lib);
    addFavorites(lib);

    lib = new QStandardItem(tr("Recent"));
    lib->setFlags(Qt::ItemIsEnabled);
    addExpanded(model, lib);
    addRecents(lib);

    // // Create a filter to only include current format materials
    // // that contain the basic render model.
    // Materials::MaterialFilter filter;
    // filter.setIncludeEmptyFolders(false);
    // filter.setIncludeLegacy(false);
    // filter.addRequired(Materials::ModelUUIDs::ModelUUID_Rendering_Basic);

    auto libraries = _materialManager.getMaterialLibraries();
    for (const auto& library : *libraries) {
        lib = new QStandardItem(library->getName());
        lib->setFlags(Qt::ItemIsEnabled);
        addExpanded(model, lib);

        QIcon icon(library->getIconPath());
        QIcon folderIcon(QString::fromStdString(":/icons/folder.svg"));

        auto modelTree = _materialManager.getMaterialTree(library, _filter);
        addMaterials(*lib, modelTree, folderIcon, icon);
    }
}

void MaterialTreeWidget::addExpanded(QStandardItem* parent, QStandardItem* child)
{
    parent->appendRow(child);
    m_materialTree->setExpanded(child->index(), true);
}

void MaterialTreeWidget::addExpanded(QStandardItemModel* model, QStandardItem* child)
{
    model->appendRow(child);
    m_materialTree->setExpanded(child->index(), true);
}

void MaterialTreeWidget::addRecents(QStandardItem* parent)
{
    for (auto& uuid : _recents) {
        try {
            auto material = getMaterialManager().getMaterial(uuid);

            QIcon icon = QIcon(material->getLibrary()->getIconPath());
            auto card = new QStandardItem(icon, material->getName());
            card->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            card->setData(QVariant(uuid), Qt::UserRole);

            addExpanded(parent, card);
        }
        catch (const Materials::MaterialNotFound&) {
        }
    }
}

void MaterialTreeWidget::addFavorites(QStandardItem* parent)
{
    for (auto& uuid : _favorites) {
        try {
            auto material = getMaterialManager().getMaterial(uuid);

            QIcon icon = QIcon(material->getLibrary()->getIconPath());
            auto card = new QStandardItem(icon, material->getName());
            card->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            card->setData(QVariant(uuid), Qt::UserRole);

            addExpanded(parent, card);
        }
        catch (const Materials::MaterialNotFound&) {
        }
    }
}
void MaterialTreeWidget::addMaterials(
    QStandardItem& parent,
    const std::shared_ptr<std::map<QString, std::shared_ptr<Materials::MaterialTreeNode>>>&
        modelTree,
    const QIcon& folderIcon,
    const QIcon& icon)
{
    for (auto& mat : *modelTree) {
        auto nodePtr = mat.second;
        if (nodePtr->getType() == Materials::MaterialTreeNode::DataNode) {
            auto material = nodePtr->getData();
            QString uuid = material->getUUID();
            // Base::Console().Log("Material path '%s'\n",
            //                     material->getDirectory().toStdString().c_str());

            // auto card = new QStandardItem(icon, material->getName());
            auto card = new QStandardItem(icon, mat.first);
            card->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            card->setData(QVariant(uuid), Qt::UserRole);

            addExpanded(&parent, card);
        }
        else {
            auto node = new QStandardItem(folderIcon, mat.first);
            addExpanded(&parent, node);
            node->setFlags(Qt::ItemIsEnabled);
            auto treeMap = nodePtr->getFolder();
            addMaterials(*node, treeMap, folderIcon, icon);
        }
    }
}

void MaterialTreeWidget::onSelectMaterial(const QItemSelection& selected,
                                          const QItemSelection& deselected)
{
    Q_UNUSED(deselected);

    // Get the UUID before changing the underlying data model
    QString uuid;
    auto model = dynamic_cast<QStandardItemModel*>(m_materialTree->model());
    QModelIndexList indexes = selected.indexes();
    for (auto it = indexes.begin(); it != indexes.end(); it++) {
        QStandardItem* item = model->itemFromIndex(*it);

        if (item) {
            uuid = item->data(Qt::UserRole).toString();
            break;
        }
    }

    updateMaterial(uuid);
    std::string _uuid = uuid.toStdString();

    Q_EMIT materialSelected(getMaterialManager().getMaterial(uuid));
}

void MaterialTreeWidget::onDoubleClick(const QModelIndex& index)
{
    auto model = dynamic_cast<QStandardItemModel*>(m_materialTree->model());
    auto item = model->itemFromIndex(index);

    if (item) {
        auto uuid = item->data(Qt::UserRole).toString();
        updateMaterial(uuid);
    }
}