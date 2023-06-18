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

#include "PreCompiled.h"
#ifndef _PreComp_
#endif

#include <QString>
#include <QPushButton>

#include <App/Application.h>
#include <Base/Interpreter.h>
#include <Gui/Command.h>
#include <Gui/WaitCursor.h>
#include <Gui/Application.h>

#include <QItemSelectionModel>

#include <Mod/Material/App/ModelManager.h>
#include "ModelSelect.h"
#include "ui_ModelSelect.h"


using namespace MatGui;

/* TRANSLATOR MatGui::ModelSelect */

ModelSelect::ModelSelect(QWidget* parent, Materials::ModelManager::ModelFilter filter)
  : QDialog(parent), _filter(filter), ui(new Ui_ModelSelect)
{
    ui->setupUi(this);

    createModelTree();
    createModelProperties();

    connect(ui->standardButtons, &QDialogButtonBox::accepted,
            this, &ModelSelect::accept);
    connect(ui->standardButtons, &QDialogButtonBox::rejected,
            this, &ModelSelect::reject);

    QItemSelectionModel* selectionModel = ui->treeModels->selectionModel();
    connect(selectionModel, &QItemSelectionModel::selectionChanged,
            this, &ModelSelect::onSelectModel);

    ui->standardButtons->button(QDialogButtonBox::Ok)->setEnabled(false);
}

/*
 *  Destroys the object and frees any allocated resources
 */
ModelSelect::~ModelSelect()
{
    // no need to delete child widgets, Qt does it all for us
}

void ModelSelect::addExpanded(QTreeView *tree, QStandardItem *parent, QStandardItem *child)
{
    parent->appendRow(child);
    tree->setExpanded(child->index(), true);
}

void ModelSelect::addExpanded(QTreeView *tree, QStandardItemModel *parent, QStandardItem *child)
{
    parent->appendRow(child);
    tree->setExpanded(child->index(), true);
}

void ModelSelect::addModels(QStandardItem &parent, const std::map<std::string, Materials::ModelTreeNode*>* modelTree, const QIcon &icon)
{
    auto tree = ui->treeModels;
    for (auto& mod : *modelTree) {
        Materials::ModelTreeNode *nodePtr = mod.second;
        if (nodePtr->getType() == Materials::ModelTreeNode::ModelNode)
        {
            const Materials::Model *model = nodePtr->getModel();
            std::string uuid = model->getUUID();

            auto card = new QStandardItem(icon, QString::fromStdString(mod.first));
            card->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled
                        | Qt::ItemIsDropEnabled);
            card->setData(QVariant(QString::fromStdString(uuid)), Qt::UserRole);

            addExpanded(tree, &parent, card);
        } else {
            auto node = new QStandardItem(QString::fromStdString(mod.first));
            addExpanded(tree, &parent, node);
            node->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
            const std::map<std::string, Materials::ModelTreeNode*>* treeMap = nodePtr->getFolder();
            addModels(*node, treeMap, icon);
        }
        // if (fs::is_directory(mod)) {
        //     auto node = new QStandardItem(QString::fromStdString(mod.path().filename().string()));
        //     addExpanded(tree, &parent, node);
        //     node->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);

        //     addModels(*node, top, mod.path().string(), icon);
        // }
        // else if (Materials::ModelManager::isModel(mod)) {
        //     auto card = new QStandardItem(icon, QString::fromStdString(mod.path().filename().string()));
        //     card->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled
        //                 | Qt::ItemIsDropEnabled);
        //     try {
        //         auto model = getModelManager().getModelByPath(mod.path().string());
        //         card->setData(QVariant(QString::fromStdString(model.getUUID())), Qt::UserRole);
        //     } catch (Materials::ModelNotFound &e) {
        //         Base::Console().Log("Model not found error\n");
        //     } catch (std::exception &e) {
        //         Base::Console().Log("Exception '%s'\n", e.what());
        //     }
        //     addExpanded(tree, &parent, card);
        // }
    }
}

void ModelSelect::createModelTree()
{
    // Materials::ModelManager modelManager;

    auto tree = ui->treeModels;
    auto model = new QStandardItemModel();
    tree->setModel(model);

    tree->setHeaderHidden(true);

    auto lib = new QStandardItem(QString::fromStdString("Favorites"));
    lib->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
    addExpanded(tree, model, lib);

    lib = new QStandardItem(QString::fromStdString("Recent"));
    lib->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
    addExpanded(tree, model, lib);

    std::list<Materials::ModelLibrary*> *libraries = getModelManager().getModelLibraries();
    for (Materials::ModelLibrary *library : *libraries)
    {
        lib = new QStandardItem(QString::fromStdString(library->getName()));
        lib->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
        addExpanded(tree, model, lib);

        // auto path = library->getDirectoryPath();
        std::map<std::string, Materials::ModelTreeNode*>* modelTree= getModelManager().getModelTree(*library, _filter);
        // delete modelTree;
        addModels(*lib, modelTree, QIcon(QString::fromStdString(library->getIconPath())));
    }
}

void ModelSelect::setHeaders(QStandardItemModel *model)
{
    QStringList headers;
    headers.append(QString::fromStdString("Inherited"));
    headers.append(QString::fromStdString("Property"));
    headers.append(QString::fromStdString("Units"));
    headers.append(QString::fromStdString("Description"));
    headers.append(QString::fromStdString("URL"));

    model->setHorizontalHeaderLabels(headers);
}

void ModelSelect::setColumnWidths(QTableView *table)
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

void ModelSelect::updateModelProperties(const Materials::Model &model)
{
    QTableView *table = ui->tableProperties;
    QStandardItemModel *tableModel = static_cast<QStandardItemModel *>(table->model());
    tableModel->clear();

    setHeaders(tableModel);
    setColumnWidths(table);

    for (auto itp = model.begin(); itp != model.end(); itp++)
    {
        QList<QStandardItem*> items;

        std::string key = itp->first;
        Materials::ModelProperty modelProperty = itp->second;
        
        auto inherited = new QStandardItem(QString::fromStdString(modelProperty.isInherited() ? "*" : ""));
        // inherited->setToolTip(QString::fromStdString(modelProperty.getDescription()));
        items.append(inherited);
        
        auto propertyItem = new QStandardItem(QString::fromStdString(key));
        items.append(propertyItem);

        auto unitsItem = new QStandardItem(QString::fromStdString(modelProperty.getUnits()));
        items.append(unitsItem);

        auto descriptionItem = new QStandardItem(QString::fromStdString(modelProperty.getDescription()));
        items.append(descriptionItem);

        auto urlItem = new QStandardItem(QString::fromStdString(modelProperty.getURL()));
        items.append(urlItem);

        // addExpanded(tree, modelRoot, propertyItem);
        tableModel->appendRow(items);
    }
}

void ModelSelect::updateMaterialModel(const std::string &uuid)
{
    Materials::Model model = getModelManager().getModel(uuid);

    // Update the general information
    ui->editName->setText(QString::fromStdString(model.getName()));
    ui->editURL->setText(QString::fromStdString(model.getURL()));
    ui->editDOI->setText(QString::fromStdString(model.getDOI()));
    ui->editDescription->setText(QString::fromStdString(model.getDescription()));

    if (model.getType() == Materials::Model::ModelType_Physical) {
        ui->tabWidget->setTabText(1, QString::fromStdString("Properties"));
    } else {
        ui->tabWidget->setTabText(1, QString::fromStdString("Appearance"));
    }
    updateModelProperties(model);
}

void ModelSelect::clearMaterialModel(void)
{
    // Update the general information
    ui->editName->setText(QString::fromStdString(""));
    ui->editURL->setText(QString::fromStdString(""));
    ui->editDOI->setText(QString::fromStdString(""));
    ui->editDescription->setText(QString::fromStdString(""));

    ui->tabWidget->setTabText(1, QString::fromStdString("Properties"));

    QTableView *table = ui->tableProperties;
    QStandardItemModel *tableModel = static_cast<QStandardItemModel *>(table->model());
    tableModel->clear();

    setHeaders(tableModel);
    setColumnWidths(table);
}

void ModelSelect::onSelectModel(const QItemSelection& selected, const QItemSelection& deselected)
{
    Q_UNUSED(deselected);

    QStandardItemModel *model = static_cast<QStandardItemModel *>(ui->treeModels->model());
    QModelIndexList indexes = selected.indexes();
    for (auto it = indexes.begin(); it != indexes.end(); it++)
    {
        QStandardItem* item = model->itemFromIndex(*it);
        Base::Console().Log("%s\n", item->text().toStdString().c_str());
        if (item) {
            try
            {
                _selected = item->data(Qt::UserRole).toString().toStdString();
                Base::Console().Log("\t%s\n", _selected.c_str());
                updateMaterialModel(_selected);
                ui->standardButtons->button(QDialogButtonBox::Ok)->setEnabled(true);
            }
            catch(const std::exception& e)
            {
                _selected = "";
                Base::Console().Log("Error %s\n", e.what());
                clearMaterialModel();
                ui->standardButtons->button(QDialogButtonBox::Ok)->setEnabled(false);
            }

        }
    }
}

void ModelSelect::accept()
{
    QDialog::accept();
}

void ModelSelect::reject()
{
    QDialog::reject();
    // auto dw = qobject_cast<QDockWidget*>(parent());
    // if (dw) {
    //     dw->deleteLater();
    // }
}

#include "moc_ModelSelect.cpp"
