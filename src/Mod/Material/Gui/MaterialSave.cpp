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
#include <QMessageBox>
#include <QTreeView>
#endif

#include <Gui/MainWindow.h>

#include <Mod/Material/App/MaterialLibrary.h>

#include "MaterialSave.h"
#include "ui_MaterialSave.h"

using namespace MatGui;

/* TRANSLATOR MatGui::MaterialsEditor */

MaterialSave::MaterialSave(Materials::Material* material, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui_MaterialSave)
    , _material(material)
    , _selectedPath(QString::fromStdString("/"))
    , _selectedUUID(QString())
{
    ui->setupUi(this);

    setLibraries();
    createModelTree();
    showSelectedTree();

    if (_material->getName().length() > 0) {
        ui->editFilename->setText(_material->getName() + QString::fromStdString(".FCMat"));
    }
    else {
        ui->editFilename->setText(QString::fromStdString("NewMaterial.FCMat"));
    }
    _filename = QString(ui->editFilename->text());  // No filename by default

    connect(ui->standardButtons->button(QDialogButtonBox::Ok),
            &QPushButton::clicked,
            this,
            &MaterialSave::onOk);
    connect(ui->standardButtons->button(QDialogButtonBox::Cancel),
            &QPushButton::clicked,
            this,
            &MaterialSave::onCancel);

    connect(ui->comboLibrary,
            &QComboBox::currentTextChanged,
            this,
            &MaterialSave::currentTextChanged);
    connect(ui->buttonNewFolder, &QPushButton::clicked, this, &MaterialSave::onNewFolder);
    connect(ui->editFilename, &QLineEdit::textEdited, this, &MaterialSave::onFilename);

    QItemSelectionModel* selectionModel = ui->treeMaterials->selectionModel();
    connect(selectionModel,
            &QItemSelectionModel::selectionChanged,
            this,
            &MaterialSave::onSelectModel);
}

/*
 *  Destroys the object and frees any allocated resources
 */
MaterialSave::~MaterialSave()
{
    // no need to delete child widgets, Qt does it all for us
}

void MaterialSave::onOk(bool checked)
{
    Q_UNUSED(checked)

    QString name = _filename.remove(QString::fromStdString(".FCMat"), Qt::CaseInsensitive);
    Base::Console().Log("name '%s'\n", _filename.toStdString().c_str());
    if (name != _material->getName()) {
        _material->setName(name);
        _material->setEditStateAlter();  // ? Does a name change count?
    }

    auto variant = ui->comboLibrary->currentData();
    auto library = variant.value<Materials::MaterialLibrary>();
    QFileInfo filepath(_selectedPath + QString::fromStdString("/") + name
                       + QString::fromStdString(".FCMat"));
    Base::Console().Log("saveMaterial(library(%s), material(%s), path(%s))\n",
                        library.getName().toStdString().c_str(),
                        _material->getName().toStdString().c_str(),
                        filepath.filePath().toStdString().c_str());
    _manager.saveMaterial(&library, *_material, filepath.filePath());

    accept();
}

void MaterialSave::onCancel(bool checked)
{
    Q_UNUSED(checked)

    reject();
}

void MaterialSave::accept()
{
    QDialog::accept();
}

void MaterialSave::reject()
{
    QDialog::reject();
}

void MaterialSave::setLibraries()
{
    auto libraries = _manager.getMaterialLibraries();
    for (auto library : *libraries) {
        if (!library->isReadOnly()) {
            QVariant libraryVariant;
            libraryVariant.setValue(*library);
            ui->comboLibrary->addItem(library->getName(), libraryVariant);
        }
    }
}

void MaterialSave::createModelTree()
{
    auto tree = ui->treeMaterials;
    auto model = new QStandardItemModel();
    tree->setModel(model);
    tree->setHeaderHidden(true);
}

void MaterialSave::addExpanded(QTreeView* tree, QStandardItem* parent, QStandardItem* child)
{
    parent->appendRow(child);
    tree->setExpanded(child->index(), true);
}

void MaterialSave::addExpanded(QTreeView* tree, QStandardItemModel* parent, QStandardItem* child)
{
    parent->appendRow(child);
    tree->setExpanded(child->index(), true);
}

void MaterialSave::addMaterials(
    QStandardItem& parent,
    const std::shared_ptr<std::map<QString, Materials::MaterialTreeNode*>> modelTree,
    const QIcon& folderIcon,
    const QIcon& icon)
{
    auto tree = ui->treeMaterials;
    for (auto& mat : *modelTree) {
        Materials::MaterialTreeNode* nodePtr = mat.second;
        if (nodePtr->getType() == Materials::MaterialTreeNode::DataNode) {
            const Materials::Material* material = nodePtr->getData();
            QString uuid = material->getUUID();
            Base::Console().Log("Material path '%s'\n",
                                material->getDirectory().toStdString().c_str());

            // auto card = new QStandardItem(icon, material->getName());
            auto card = new QStandardItem(icon, mat.first);
            // card->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled
            //             | Qt::ItemIsDropEnabled);
            card->setData(QVariant(uuid), Qt::UserRole);

            addExpanded(tree, &parent, card);
        }
        else {
            auto node = new QStandardItem(folderIcon, mat.first);
            addExpanded(tree, &parent, node);
            // node->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
            auto treeMap = nodePtr->getFolder();
            addMaterials(*node, treeMap, folderIcon, icon);
        }
    }
}

void MaterialSave::showSelectedTree()
{
    auto tree = ui->treeMaterials;
    auto model = static_cast<QStandardItemModel*>(tree->model());
    model->clear();

    if (ui->comboLibrary->count() > 0) {
        auto variant = ui->comboLibrary->currentData();
        auto library = variant.value<Materials::MaterialLibrary>();
        QIcon icon(library.getIconPath());
        QIcon folderIcon(QString::fromStdString(":/icons/folder.svg"));
        _libraryName = library.getName();
        _selectedPath = QString::fromStdString("/") + _libraryName;

        auto lib = new QStandardItem(library.getName());
        lib->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
        addExpanded(tree, model, lib);

        auto modelTree = _manager.getMaterialTree(library);
        addMaterials(*lib, modelTree, folderIcon, icon);
    }
    else {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("No writeable library"),
                             QObject::tr("No writeable library"));
    }
}

QString MaterialSave::getPath(const QStandardItem* item) const
{
    QString path = QString::fromStdString("/");
    if (item) {
        path = path + item->text();
        if (item->parent()) {
            return getPath(item->parent()) + path;
        }
    }

    return path;
}

void MaterialSave::onSelectModel(const QItemSelection& selected, const QItemSelection& deselected)
{
    // Q_UNUSED(selected);
    Q_UNUSED(deselected);

    _filename = QString(ui->editFilename->text());  // No filename by default
    QStandardItemModel* model = static_cast<QStandardItemModel*>(ui->treeMaterials->model());
    QModelIndexList indexes = selected.indexes();
    if (indexes.count() == 0) {
        Base::Console().Log("Nothing selected\n");
        _selectedPath = QString::fromStdString("/") + _libraryName;
        _selectedUUID = QString();
        Base::Console().Log("\tSelected path '%s'\n", _selectedPath.toStdString().c_str());
        return;
    }
    for (auto it = indexes.begin(); it != indexes.end(); it++) {
        QStandardItem* item = model->itemFromIndex(*it);
        Base::Console().Log("%s\n", item->text().toStdString().c_str());
        if (item) {
            auto _selected = item->data(Qt::UserRole);
            if (_selected.isValid()) {
                Base::Console().Log("\tuuid %s\n", _selected.toString().toStdString().c_str());
                _selectedPath = getPath(item->parent());
                _selectedUUID = _selected.toString();
                _filename = item->text();
            }
            else {
                _selectedPath = getPath(item);
                _selectedUUID = QString();
            }
        }
    }
    if (_filename.length() > 0) {
        ui->editFilename->setText(_filename);
    }
    Base::Console().Log("\tSelected path '%s', filename = '%s'\n",
                        _selectedPath.toStdString().c_str(),
                        _filename.toStdString().c_str());
}

void MaterialSave::currentTextChanged(const QString& value)
{
    Q_UNUSED(value)

    showSelectedTree();
}

void MaterialSave::onNewFolder(bool checked)
{
    Q_UNUSED(checked)

    auto tree = ui->treeMaterials;
    auto model = static_cast<QStandardItemModel*>(tree->model());
    auto current = tree->currentIndex();
    if (!current.isValid()) {
        current = model->index(0, 0);
    }
    auto item = model->itemFromIndex(current);
    if (item->hasChildren()) {
        Base::Console().Log("Add new folder to '%s'\n", item->text().toStdString().c_str());
        QIcon folderIcon(QString::fromStdString(":/icons/folder.svg"));

        auto node = new QStandardItem(folderIcon, QString::fromStdString("New Folder"));
        addExpanded(tree, item, node);
        // node->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
    }
}

void MaterialSave::onFilename(const QString& text)
{
    Base::Console().Log("MaterialSave::onFilename('%s')\n", text.toStdString().c_str());

    _filename = text;
}

#include "moc_MaterialSave.cpp"
