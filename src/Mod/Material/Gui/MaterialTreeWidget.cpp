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
#include <Mod/Material/App/MaterialFilterPy.h>
#include <Mod/Material/App/ModelUuids.h>

#include "MaterialTreeWidget.h"
#include "MaterialsEditor.h"
#include "ui_MaterialsEditor.h"

Q_DECLARE_METATYPE(Materials::MaterialFilterPy*)

using Base::Console;
using namespace MatGui;

/** Constructs a Material tree widget.
 */

TYPESYSTEM_SOURCE(MatGui::MaterialTreeWidget, Base::BaseClass)

MaterialTreeWidget::MaterialTreeWidget(const std::shared_ptr<Materials::MaterialFilter>& filter,
                                       QWidget* parent)
    : QWidget(parent)
    , m_expanded(false)
    , m_treeSizeHint(minimumTreeWidth, minimumTreeHeight)
    , _filter(filter)
    , _recentMax(defaultRecents)
{
    setup();
}

MaterialTreeWidget::MaterialTreeWidget(
    const std::shared_ptr<std::list<std::shared_ptr<Materials::MaterialFilter>>>& filterList,
    QWidget* parent)
    : QWidget(parent)
    , m_expanded(false)
    , m_treeSizeHint(minimumTreeWidth, minimumTreeHeight)
    , _filter(std::make_shared<Materials::MaterialFilter>())
    , _filterList(filterList)
    , _recentMax(defaultRecents)
{
    setup();
}

MaterialTreeWidget::MaterialTreeWidget(QWidget* parent)
    : QWidget(parent)
    , m_expanded(false)
    , m_treeSizeHint(minimumTreeWidth, minimumTreeHeight)
    , _filter(std::make_shared<Materials::MaterialFilter>())
    , _recentMax(defaultRecents)
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
MaterialTreeWidget::~MaterialTreeWidget()
{
    addRecent(m_uuid);
    saveWidgetSettings();
    saveMaterialTree();
}

QSize MaterialTreeWidget::sizeHint() const
{
    if (!m_expanded) {
        // When not expanded, the size height is the same as m_material
        QSize size = m_material->sizeHint();
        size.setWidth(minimumWidth);
        return size;
    }
    return QWidget::sizeHint();
}

QSize MaterialTreeWidget::treeSizeHint() const
{
    return m_treeSizeHint;
}

void MaterialTreeWidget::setTreeSizeHint(const QSize& hint)
{
    m_treeSizeHint = hint;
    m_materialTree->setMinimumSize(m_treeSizeHint);
    m_materialTree->adjustSize();
    adjustSize();
}

void MaterialTreeWidget::createLayout()
{
    m_material = new QLineEdit(this);
    m_expand = new QPushButton(this);
    m_expand->setIcon(style()->standardIcon(QStyle::SP_TitleBarUnshadeButton));
    m_materialTree = new QTreeView(this);
    m_filterCombo = new QComboBox(this);
    m_editor = new QPushButton(tr("Launch editor"), this);

    m_materialTree->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    m_materialTree->setMinimumSize(m_treeSizeHint);
    m_materialTree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_materialTree->setSelectionBehavior(QAbstractItemView::SelectItems);

    auto materialLayout = new QHBoxLayout();
    materialLayout->addWidget(m_material);
    materialLayout->addWidget(m_expand);
    // materialLayout->setSizeConstraint(QLayout::SetMinimumSize);

    auto treeLayout = new QHBoxLayout();
    treeLayout->addWidget(m_materialTree);

    auto buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(m_filterCombo);
    buttonLayout->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Preferred));
    buttonLayout->addWidget(m_editor);

    auto layout = new QVBoxLayout();
    layout->setContentsMargins(0, 9, 0, 9);
    layout->addItem(materialLayout);
    layout->addItem(treeLayout);
    layout->addItem(buttonLayout);
    setLayout(layout);

    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);

    // Set the filter if using a filter list
    if (hasMultipleFilters()) {
        _filter = _filterList->front();
    }

    fillFilterCombo();

    // Start in the previous expanded state
    auto param = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/TreeWidget");
    auto expanded = param->GetBool("WidgetExpanded", false);
    setExpanded(expanded);

    connect(m_expand, &QPushButton::clicked, this, &MaterialTreeWidget::expandClicked);
    connect(m_editor, &QPushButton::clicked, this, &MaterialTreeWidget::editorClicked);
    connect(m_filterCombo,
            &QComboBox::currentTextChanged,
            this,
            &MaterialTreeWidget::onFilter);
}

void MaterialTreeWidget::setExpanded(bool open)
{
    m_materialTree->setVisible(open);
    m_editor->setVisible(open);

    setFilterVisible(open);

    m_expanded = open;

    if (open) {
        m_expand->setIcon(style()->standardIcon(QStyle::SP_TitleBarShadeButton));
    }
    else {
        m_expand->setIcon(style()->standardIcon(QStyle::SP_TitleBarUnshadeButton));
    }

    // m_materialTree->adjustSize();
    adjustSize();
    Q_EMIT onExpanded(m_expanded);
}

void MaterialTreeWidget::setFilterVisible(bool open)
{
    if (open && hasMultipleFilters()) {
        m_filterCombo->setVisible(true);
    }
    else {
        m_filterCombo->setVisible(false);
    }
}

void MaterialTreeWidget::fillFilterCombo()
{
    m_filterCombo->clear();
    if (hasMultipleFilters()) {
        for (auto const& filter : *_filterList) {
            m_filterCombo->addItem(filter->name());
        }
    }
}


void MaterialTreeWidget::expandClicked(bool checked)
{
    Q_UNUSED(checked)

    // Toggle the open state
    setExpanded(!m_expanded);
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
    // setExpanded(!m_expanded);
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
    catch (Materials::MaterialNotFound const&) {
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
    // Find the original item, not the reference in favourites or recents
    for (int i = 0; i < root->rowCount(); i++) {
        auto child = root->child(i);
        if (child->text() != tr("Favorites") && child->text() != tr("Recent")) {
            if (findInTree(*child, &index, uuid)) {
                return index;
            }
        }
    }

    return {};
}

void MaterialTreeWidget::setMaterial(const QString& uuid)
{
    if (uuid.isEmpty()) {
        // Nothing is selected
        QItemSelectionModel* selectionModel = m_materialTree->selectionModel();
        selectionModel->clear();
        m_material->clear();

        return;
    }

    updateMaterial(uuid);

    // Now select the material in the tree
    auto index = findInTree(uuid);
    if (index.isValid()) {
        QItemSelectionModel* selectionModel = m_materialTree->selectionModel();
        selectionModel->select(index, QItemSelectionModel::SelectCurrent);
        m_materialTree->scrollTo(index);
    }
}

QString MaterialTreeWidget::getMaterialUUID() const
{
    return m_uuid;
}

void MaterialTreeWidget::setFilter(const std::shared_ptr<Materials::MaterialFilter>& filter)
{
    if (_filter) {
        _filter.reset();
    }
    if (_filterList) {
        _filterList.reset();
    }

    _filter = filter;

    fillFilterCombo();
    setFilterVisible(m_expanded);

    updateMaterialTree();
}

void MaterialTreeWidget::setFilter(
    const std::shared_ptr<std::list<std::shared_ptr<Materials::MaterialFilter>>>& filterList)
{
    _filter.reset();
    if (_filterList) {
        _filterList.reset();
    }

    _filterList = filterList;
    if (hasMultipleFilters()) {
        _filter = _filterList->front();
    }

    fillFilterCombo();
    setFilterVisible(m_expanded);

    updateMaterialTree();
}

void MaterialTreeWidget::setActiveFilter(const QString& name)
{
    if (_filterList) {
        for (auto const& filter : *_filterList) {
            if (filter->name() == name) {
                _filter.reset();

                _filter = filter;

                // Save the library/folder expansion state
                saveMaterialTree();

                updateMaterialTree();
                return;
            }
        }
    }
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
    _recentMax = static_cast<int>(param->GetInt("RecentMax", defaultRecents));
    auto count = param->GetInt("Recent", 0);
    for (int i = 0; static_cast<long>(i) < count; i++) {
        QString key = QString::fromLatin1("MRU%1").arg(i);
        QString uuid = QString::fromStdString(param->GetASCII(key.toStdString().c_str(), ""));
        if (!_filter || _filter->modelIncluded(uuid)) {
            _recents.push_back(uuid);
        }
    }
}

void MaterialTreeWidget::saveRecents()
{
    auto param = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/Recent");

    // Clear out the existing favorites
    int count = param->GetInt("Recent", 0);
    for (int i = 0; static_cast<long>(i) < count; i++) {
        QString key = QString::fromLatin1("MRU%1").arg(i);
        param->RemoveASCII(key.toStdString().c_str());
    }

    // Add the current values
    int size = _recents.size();
    if (size > _recentMax) {
        size = _recentMax;
    }
    param->SetInt("Recent", size);
    int j = 0;
    for (auto& recent : _recents) {
        QString key = QString::fromLatin1("MRU%1").arg(j);
        param->SetASCII(key.toStdString().c_str(), recent.toStdString());

        j++;
        if (j >= size) {
            break;
        }
    }
}

void MaterialTreeWidget::addRecent(const QString& uuid)
{
    if (uuid.isEmpty()) {
        return;
    }
    // Ensure it is a material. New, unsaved materials will not be
    try {
        auto material = _materialManager.getMaterial(uuid);
        Q_UNUSED(material)
    }
    catch (const Materials::MaterialNotFound&) {
        return;
    }

    // Ensure no duplicates
    if (isRecent(uuid)) {
        _recents.remove(uuid);
    }

    _recents.push_front(uuid);
    while (_recents.size() > static_cast<std::size_t>(_recentMax)) {
        _recents.pop_back();
    }

    saveRecents();
}

bool MaterialTreeWidget::isRecent(const QString& uuid) const
{
    for (auto& it : _recents) {
        if (it == uuid) {
            return true;
        }
    }
    return false;
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
    auto param = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/TreeWidget/MaterialTree");

    auto model = dynamic_cast<QStandardItemModel*>(m_materialTree->model());

    if (_filterOptions.includeFavorites()) {
        auto lib = new QStandardItem(tr("Favorites"));
        lib->setFlags(Qt::ItemIsEnabled);
        addExpanded(model, lib, param);
        addFavorites(lib);
    }

    if (_filterOptions.includeRecent()) {
        auto lib = new QStandardItem(tr("Recent"));
        lib->setFlags(Qt::ItemIsEnabled);
        addExpanded(model, lib, param);
        addRecents(lib);
    }

    auto libraries = _materialManager.getMaterialLibraries();
    for (const auto& library : *libraries) {
        auto modelTree = _materialManager.getMaterialTree(library, _filter, _filterOptions);

        bool showLibraries = _filterOptions.includeEmptyLibraries();
        if (!_filterOptions.includeEmptyLibraries() && modelTree->size() > 0) {
            showLibraries = true;
        }

        if (showLibraries) {
            auto lib = new QStandardItem(library->getName());
            lib->setFlags(Qt::ItemIsEnabled);
            addExpanded(model, lib, param);

            QIcon icon(library->getIconPath());
            QIcon folderIcon(QString::fromStdString(":/icons/folder.svg"));

            addMaterials(*lib, modelTree, folderIcon, icon, param);
        }
    }
}

void MaterialTreeWidget::addExpanded(QStandardItem* parent, QStandardItem* child)
{
    parent->appendRow(child);
    m_materialTree->setExpanded(child->index(), true);
}

void MaterialTreeWidget::addExpanded(QStandardItem* parent,
                                     QStandardItem* child,
                                     const Base::Reference<ParameterGrp>& param)
{
    parent->appendRow(child);

    // Restore to any previous expansion state
    auto expand = param->GetBool(child->text().toStdString().c_str(), true);
    m_materialTree->setExpanded(child->index(), expand);
}

void MaterialTreeWidget::addExpanded(QStandardItemModel* model, QStandardItem* child)
{
    model->appendRow(child);
    m_materialTree->setExpanded(child->index(), true);
}

void MaterialTreeWidget::addExpanded(QStandardItemModel* model,
                                     QStandardItem* child,
                                     const Base::Reference<ParameterGrp>& param)
{
    model->appendRow(child);

    // Restore to any previous expansion state
    auto expand = param->GetBool(child->text().toStdString().c_str(), true);
    m_materialTree->setExpanded(child->index(), expand);
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
    const QIcon& icon,
    const Base::Reference<ParameterGrp>& param)
{
    auto childParam = param->GetGroup(parent.text().toStdString().c_str());
    for (auto& mat : *modelTree) {
        auto nodePtr = mat.second;
        if (nodePtr->getType() == Materials::MaterialTreeNode::DataNode) {
            auto material = nodePtr->getData();
            QString uuid = material->getUUID();

            auto card = new QStandardItem(icon, mat.first);
            card->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            card->setData(QVariant(uuid), Qt::UserRole);

            addExpanded(&parent, card);
        }
        else {
            auto node = new QStandardItem(folderIcon, mat.first);
            addExpanded(&parent, node, childParam);
            node->setFlags(Qt::ItemIsEnabled);
            auto treeMap = nodePtr->getFolder();
            addMaterials(*node, treeMap, folderIcon, icon, childParam);
        }
    }
}

void MaterialTreeWidget::onSelectMaterial(const QItemSelection& selected,
                                          const QItemSelection& deselected)
{
    Q_UNUSED(deselected);

    if (selected.isEmpty()) {
        m_uuid.clear();
        return;
    }

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

    if (!uuid.isEmpty()) {
        Q_EMIT materialSelected(getMaterialManager().getMaterial(uuid));
        Q_EMIT onMaterial(uuid);
    }
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

void MaterialTreeWidget::onFilter(const QString& text)
{
    setActiveFilter(text);
}

void MaterialTreeWidget::saveWidgetSettings()
{
    auto param = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/TreeWidget");
    param->SetBool("WidgetExpanded", m_expanded);
}

void MaterialTreeWidget::saveMaterialTree()
{
    auto param = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/TreeWidget/MaterialTree");
    param->Clear();

    auto tree = m_materialTree;
    auto model = dynamic_cast<QStandardItemModel*>(tree->model());

    auto root = model->invisibleRootItem();
    for (int i = 0; i < root->rowCount(); i++) {
        auto child = root->child(i);
        saveMaterialTreeChildren(param, tree, model, child);
    }
}

void MaterialTreeWidget::saveMaterialTreeChildren(const Base::Reference<ParameterGrp>& param,
                                                  QTreeView* tree,
                                                  QStandardItemModel* model,
                                                  QStandardItem* item)
{
    if (item->hasChildren()) {
        param->SetBool(item->text().toStdString().c_str(), tree->isExpanded(item->index()));

        auto treeParam = param->GetGroup(item->text().toStdString().c_str());
        for (int i = 0; i < item->rowCount(); i++) {
            auto child = item->child(i);

            saveMaterialTreeChildren(treeParam, tree, model, child);
        }
    }
}

// --------------------------------------------------------------------

PrefMaterialTreeWidget::PrefMaterialTreeWidget(QWidget* parent)
    : MaterialTreeWidget(parent)
    , PrefWidget()
{}

PrefMaterialTreeWidget::~PrefMaterialTreeWidget() = default;

void PrefMaterialTreeWidget::restorePreferences()
{
    if (getWindowParameter().isNull()) {
        failedToRestore(objectName());
        return;
    }

    const char* defaultUuid = "7f9fd73b-50c9-41d8-b7b2-575a030c1eeb";
    QString uuid = QString::fromStdString(getWindowParameter()->GetASCII(entryName(), defaultUuid));
    setMaterial(uuid);
}

void PrefMaterialTreeWidget::savePreferences()
{
    if (getWindowParameter().isNull()) {
        failedToSave(objectName());
        return;
    }

    getWindowParameter()->SetASCII(entryName(), getMaterialUUID().toStdString());
}
