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
#include <QDesktopServices>
#include <QItemSelectionModel>
#include <QPushButton>
#include <QString>
#endif

#include <App/Application.h>
#include <Base/Interpreter.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/WaitCursor.h>

#include "ModelSelect.h"
#include "ui_ModelSelect.h"


using namespace MatGui;

/* TRANSLATOR MatGui::ModelSelect */

ModelSelect::ModelSelect(QWidget* parent, Materials::ModelManager::ModelFilter filter)
    : QDialog(parent)
    , _filter(filter)
    , ui(new Ui_ModelSelect)
{
    ui->setupUi(this);

    getFavorites();
    getRecents();

    createModelTree();
    createModelProperties();

    ui->buttonURL->setIcon(QIcon(QString::fromStdString(":/icons/internet-web-browser.svg")));
    ui->buttonDOI->setIcon(QIcon(QString::fromStdString(":/icons/internet-web-browser.svg")));

    connect(ui->standardButtons, &QDialogButtonBox::accepted, this, &ModelSelect::accept);
    connect(ui->standardButtons, &QDialogButtonBox::rejected, this, &ModelSelect::reject);

    QItemSelectionModel* selectionModel = ui->treeModels->selectionModel();
    connect(selectionModel,
            &QItemSelectionModel::selectionChanged,
            this,
            &ModelSelect::onSelectModel);

    connect(ui->buttonURL, &QPushButton::clicked, this, &ModelSelect::onURL);
    connect(ui->buttonDOI, &QPushButton::clicked, this, &ModelSelect::onDOI);
    connect(ui->buttonFavorite, &QPushButton::clicked, this, &ModelSelect::onFavourite);

    ui->standardButtons->button(QDialogButtonBox::Ok)->setEnabled(false);
    ui->buttonFavorite->setEnabled(false);
}

void ModelSelect::getFavorites()
{
    _favorites.clear();

    auto param = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/Models/Favorites");
    int count = param->GetInt("Favorites", 0);
    for (int i = 0; i < count; i++) {
        QString key = QString::fromLatin1("FAV%1").arg(i);
        QString uuid = QString::fromStdString(param->GetASCII(key.toStdString().c_str(), ""));
        _favorites.push_back(uuid);
    }
}

void ModelSelect::saveFavorites()
{
    auto param = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/Models/Favorites");

    // Clear out the existing favorites
    int count = param->GetInt("Favorites", 0);
    for (int i = 0; i < count; i++) {
        QString key = QString::fromLatin1("FAV%1").arg(i);
        param->RemoveASCII(key.toStdString().c_str());
    }

    // Add the current values
    param->SetInt("Favorites", _favorites.size());
    int j = 0;
    for (auto favorite : _favorites) {
        QString key = QString::fromLatin1("FAV%1").arg(j);
        param->SetASCII(key.toStdString().c_str(), favorite.toStdString());

        j++;
    }
}

void ModelSelect::addFavorite(const QString& uuid)
{
    if (!isFavorite(uuid)) {
        _favorites.push_back(uuid);
        saveFavorites();
        refreshModelTree();
    }
}

void ModelSelect::removeFavorite(const QString& uuid)
{
    if (isFavorite(uuid)) {
        _favorites.remove(uuid);
        saveFavorites();
        refreshModelTree();
    }
}

bool ModelSelect::isFavorite(const QString& uuid) const
{
    for (auto it : _favorites) {
        if (it == uuid) {
            return true;
        }
    }
    return false;
}


void ModelSelect::getRecents()
{
    _recents.clear();

    auto param = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/Models/Recent");
    _recentMax = param->GetInt("RecentMax", 5);
    int count = param->GetInt("Recent", 0);
    for (int i = 0; i < count; i++) {
        QString key = QString::fromLatin1("MRU%1").arg(i);
        QString uuid = QString::fromStdString(param->GetASCII(key.toStdString().c_str(), ""));
        _recents.push_back(uuid);
    }
}

void ModelSelect::saveRecents()
{
    auto param = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/Models/Recent");

    // Clear out the existing favorites
    int count = param->GetInt("Recent", 0);
    for (int i = 0; i < count; i++) {
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
    for (auto recent : _recents) {
        QString key = QString::fromLatin1("MRU%1").arg(j);
        param->SetASCII(key.toStdString().c_str(), recent.toStdString());

        j++;
        if (j >= size) {
            break;
        }
    }
}

void ModelSelect::addRecent(const QString& uuid)
{
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

bool ModelSelect::isRecent(const QString& uuid) const
{
    for (auto it : _recents) {
        if (it == uuid) {
            return true;
        }
    }
    return false;
}

/*
 *  Destroys the object and frees any allocated resources
 */
ModelSelect::~ModelSelect()
{
    // no need to delete child widgets, Qt does it all for us
}

void ModelSelect::addExpanded(QTreeView* tree, QStandardItem* parent, QStandardItem* child)
{
    parent->appendRow(child);
    tree->setExpanded(child->index(), true);
}

void ModelSelect::addExpanded(QTreeView* tree, QStandardItemModel* parent, QStandardItem* child)
{
    parent->appendRow(child);
    tree->setExpanded(child->index(), true);
}

void ModelSelect::addModels(
    QStandardItem& parent,
    const std::shared_ptr<std::map<QString, Materials::ModelTreeNode*>> modelTree,
    const QIcon& icon)
{
    auto tree = ui->treeModels;
    for (auto& mod : *modelTree) {
        Materials::ModelTreeNode* nodePtr = mod.second;
        if (nodePtr->getType() == Materials::ModelTreeNode::DataNode) {
            const Materials::Model* model = nodePtr->getData();
            QString uuid = model->getUUID();

            auto card = new QStandardItem(icon, model->getName());
            card->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled
                           | Qt::ItemIsDropEnabled);
            card->setData(QVariant(uuid), Qt::UserRole);

            addExpanded(tree, &parent, card);
        }
        else {
            auto node = new QStandardItem(mod.first);
            addExpanded(tree, &parent, node);
            node->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
            auto treeMap = nodePtr->getFolder();
            addModels(*node, treeMap, icon);
        }
    }
}

void ModelSelect::addRecents(QStandardItem* parent)
{
    auto tree = ui->treeModels;
    for (auto& uuid : _recents) {
        try {
            const Materials::Model& model = getModelManager().getModel(uuid);

            if (getModelManager().passFilter(_filter, model.getType())) {
                QIcon icon = QIcon(model.getLibrary().getIconPath());
                auto card = new QStandardItem(icon, model.getName());
                card->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled
                               | Qt::ItemIsDropEnabled);
                card->setData(QVariant(uuid), Qt::UserRole);

                addExpanded(tree, parent, card);
            }
        }
        catch (const Materials::ModelNotFound&) {
        }
    }
}

void ModelSelect::addFavorites(QStandardItem* parent)
{
    auto tree = ui->treeModels;
    for (auto& uuid : _favorites) {
        try {
            const Materials::Model& model = getModelManager().getModel(uuid);

            if (getModelManager().passFilter(_filter, model.getType())) {
                QIcon icon = QIcon(model.getLibrary().getIconPath());
                auto card = new QStandardItem(icon, model.getName());
                card->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled
                               | Qt::ItemIsDropEnabled);
                card->setData(QVariant(uuid), Qt::UserRole);

                addExpanded(tree, parent, card);
            }
        }
        catch (const Materials::ModelNotFound&) {
        }
    }
}

void ModelSelect::createModelTree()
{
    // Materials::ModelManager modelManager;

    auto tree = ui->treeModels;
    auto model = new QStandardItemModel();
    tree->setModel(model);
    tree->setHeaderHidden(true);

    fillTree();
}

void ModelSelect::refreshModelTree()
{
    auto tree = ui->treeModels;
    auto model = static_cast<QStandardItemModel*>(tree->model());
    model->clear();

    fillTree();
}

void ModelSelect::fillTree()
{
    // Materials::ModelManager modelManager;

    auto tree = ui->treeModels;
    auto model = static_cast<QStandardItemModel*>(tree->model());

    auto lib = new QStandardItem(QString::fromStdString("Favorites"));
    lib->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
    addExpanded(tree, model, lib);
    addFavorites(lib);

    lib = new QStandardItem(QString::fromStdString("Recent"));
    lib->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
    addExpanded(tree, model, lib);
    addRecents(lib);

    auto libraries = getModelManager().getModelLibraries();
    for (auto library : *libraries) {
        lib = new QStandardItem(library->getName());
        lib->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
        addExpanded(tree, model, lib);

        auto modelTree = getModelManager().getModelTree(*library, _filter);
        addModels(*lib, modelTree, QIcon(library->getIconPath()));
    }
}

void ModelSelect::setHeaders(QStandardItemModel* model)
{
    QStringList headers;
    headers.append(QString::fromStdString("Inherited"));
    headers.append(QString::fromStdString("Property"));
    headers.append(QString::fromStdString("Units"));
    headers.append(QString::fromStdString("Description"));
    headers.append(QString::fromStdString("URL"));

    model->setHorizontalHeaderLabels(headers);
}

void ModelSelect::setColumnWidths(QTableView* table)
{
    table->setColumnWidth(0, 75);
    table->setColumnWidth(1, 200);
    table->setColumnWidth(2, 200);
    table->setColumnWidth(3, 200);
    table->setColumnWidth(4, 200);
}

void ModelSelect::createModelProperties()
{
    auto table = ui->tableProperties;
    auto model = new QStandardItemModel();
    table->setModel(model);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    setHeaders(model);
    setColumnWidths(table);

    // table->setHeaderHidden(false);
    // table->setUniformRowHeights(true);
    // table->setItemDelegate(new MaterialDelegate(this));
}

void ModelSelect::updateModelProperties(const Materials::Model& model)
{
    QTableView* table = ui->tableProperties;
    QStandardItemModel* tableModel = static_cast<QStandardItemModel*>(table->model());
    tableModel->clear();

    setHeaders(tableModel);
    setColumnWidths(table);

    for (auto itp = model.begin(); itp != model.end(); itp++) {
        QList<QStandardItem*> items;

        QString key = itp->first;
        const Materials::ModelProperty modelProperty =
            static_cast<const Materials::ModelProperty>(itp->second);

        auto inherited =
            new QStandardItem(QString::fromStdString(modelProperty.isInherited() ? "*" : ""));
        // inherited->setToolTip(QString::fromStdString(modelProperty.getDescription()));
        items.append(inherited);

        auto propertyItem = new QStandardItem(key);
        items.append(propertyItem);

        auto unitsItem = new QStandardItem(modelProperty.getUnits());
        items.append(unitsItem);

        auto descriptionItem = new QStandardItem(modelProperty.getDescription());
        items.append(descriptionItem);

        auto urlItem = new QStandardItem(modelProperty.getURL());
        items.append(urlItem);

        // addExpanded(tree, modelRoot, propertyItem);
        tableModel->appendRow(items);
    }
}

void ModelSelect::updateMaterialModel(const QString& uuid)
{
    Materials::Model model = getModelManager().getModel(uuid);

    // Update the general information
    ui->editName->setText(model.getName());
    ui->editURL->setText(model.getURL());
    ui->editDOI->setText(model.getDOI());
    ui->editDescription->setText(model.getDescription());

    if (model.getType() == Materials::Model::ModelType_Physical) {
        ui->tabWidget->setTabText(1, QString::fromStdString("Properties"));
    }
    else {
        ui->tabWidget->setTabText(1, QString::fromStdString("Appearance"));
    }
    updateModelProperties(model);
}

void ModelSelect::clearMaterialModel()
{
    // Update the general information
    ui->editName->setText(QString::fromStdString(""));
    ui->editURL->setText(QString::fromStdString(""));
    ui->editDOI->setText(QString::fromStdString(""));
    ui->editDescription->setText(QString::fromStdString(""));

    ui->tabWidget->setTabText(1, QString::fromStdString("Properties"));

    QTableView* table = ui->tableProperties;
    QStandardItemModel* tableModel = static_cast<QStandardItemModel*>(table->model());
    tableModel->clear();

    setHeaders(tableModel);
    setColumnWidths(table);
}

void ModelSelect::onSelectModel(const QItemSelection& selected, const QItemSelection& deselected)
{
    Q_UNUSED(deselected);

    QStandardItemModel* model = static_cast<QStandardItemModel*>(ui->treeModels->model());
    QModelIndexList indexes = selected.indexes();
    for (auto it = indexes.begin(); it != indexes.end(); it++) {
        QStandardItem* item = model->itemFromIndex(*it);
        Base::Console().Log("%s\n", item->text().toStdString().c_str());
        if (item) {
            try {
                _selected = item->data(Qt::UserRole).toString();
                Base::Console().Log("\t%s\n", _selected.toStdString().c_str());
                updateMaterialModel(_selected);
                ui->standardButtons->button(QDialogButtonBox::Ok)->setEnabled(true);
                ui->buttonFavorite->setEnabled(true);
            }
            catch (const std::exception& e) {
                _selected = QString::fromStdString("");
                Base::Console().Log("Error %s\n", e.what());
                clearMaterialModel();
                ui->standardButtons->button(QDialogButtonBox::Ok)->setEnabled(false);
                ui->buttonFavorite->setEnabled(false);
            }
        }
    }
}

void ModelSelect::onURL(bool checked)
{
    Q_UNUSED(checked)

    Base::Console().Log("URL\n");
    QString url = ui->editURL->text();
    if (url.length() > 0) {
        QDesktopServices::openUrl(QUrl(url, QUrl::TolerantMode));
    }
}

void ModelSelect::onDOI(bool checked)
{
    Q_UNUSED(checked)

    Base::Console().Log("DOI\n");
    QString url = QString::fromStdString("https://doi.org/") + ui->editDOI->text();
    if (url.length() > 0) {
        QDesktopServices::openUrl(QUrl(url, QUrl::TolerantMode));
    }
}

void ModelSelect::onFavourite(bool checked)
{
    Q_UNUSED(checked)

    Base::Console().Log("Favorite\n");
    if (isFavorite(_selected)) {
        removeFavorite(_selected);
    }
    else {
        addFavorite(_selected);
    }
}

void ModelSelect::accept()
{
    addRecent(_selected);
    QDialog::accept();
}

void ModelSelect::reject()
{
    QDialog::reject();
}

#include "moc_ModelSelect.cpp"
