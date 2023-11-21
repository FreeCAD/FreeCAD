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
#include <QMenu>
#include <QMessageBox>
#include <QTreeView>
#endif

#include <Gui/MainWindow.h>

#include <Mod/Material/App/MaterialLibrary.h>

#include "MaterialSave.h"
#include "ui_MaterialSave.h"

using namespace MatGui;

/* TRANSLATOR MatGui::MaterialsEditor */

MaterialSave::MaterialSave(std::shared_ptr<Materials::Material> material, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui_MaterialSave)
    , _material(material)
    , _saveInherited(true)
    , _selectedPath(QString::fromStdString("/"))
    , _selectedFull(QString::fromStdString("/"))
    , _selectedUUID(QString())
    , _deleteAction(this)
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

    ui->checkDerived->setChecked(_saveInherited);
    connect(ui->checkDerived, &QCheckBox::stateChanged, this, &MaterialSave::onInherited);

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

    ui->treeMaterials->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->treeMaterials,
            &QWidget::customContextMenuRequested,
            this,
            &MaterialSave::onContextMenu);

    _deleteAction.setText(tr("Delete"));
    _deleteAction.setShortcut(Qt::Key_Delete);
    connect(&_deleteAction, &QAction::triggered, this, &MaterialSave::onDelete);
    ui->treeMaterials->addAction(&_deleteAction);

    QItemSelectionModel* selectionModel = ui->treeMaterials->selectionModel();
    connect(selectionModel,
            &QItemSelectionModel::selectionChanged,
            this,
            &MaterialSave::onSelectModel);

    auto model = static_cast<QStandardItemModel*>(ui->treeMaterials->model());
    connect(model, &QStandardItemModel::itemChanged, this, &MaterialSave::onItemChanged);
}

/*
 *  Destroys the object and frees any allocated resources
 */
MaterialSave::~MaterialSave()
{
    // no need to delete child widgets, Qt does it all for us
}

void MaterialSave::onInherited(int state)
{
    Q_UNUSED(state)

    _saveInherited = ui->checkDerived->isChecked();
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
    auto library = variant.value<std::shared_ptr<Materials::MaterialLibrary>>();
    QFileInfo filepath(_selectedPath + QString::fromStdString("/") + name
                       + QString::fromStdString(".FCMat"));
    Base::Console().Log("saveMaterial(library(%s), material(%s), path(%s))\n",
                        library->getName().toStdString().c_str(),
                        _material->getName().toStdString().c_str(),
                        filepath.filePath().toStdString().c_str());

    if (library->fileExists(filepath.filePath())) {
        // confirm overwrite
        auto res = confirmOverwrite(_filename);
        if (res == QMessageBox::Cancel) {
            return;
        }

        _manager.saveMaterial(library, _material, filepath.filePath(), true, false, _saveInherited);
        accept();
        return;
    }

    bool saveAsCopy = false;
    if (_manager.exists(_material->getUUID())) {
        // Does it already exist in this library?
        if (_manager.exists(library, _material->getUUID())) {
            // Confirm saving a new material
            auto res = confirmNewMaterial();
            if (res == QMessageBox::Cancel) {
                return;
            }
            // saveAsCopy = false = already done
        }
        else {
            // Copy or new
            auto res = confirmCopy();
            if (res == QMessageBox::Cancel) {
                return;
            }
            else if (res == QMessageBox::Save) {
                // QMessageBox::Save saves as normal, a duplicate
                saveAsCopy = true;
            }
            // QMessageBox::Ok saves a new material
        }
    }

    _manager
        .saveMaterial(library, _material, filepath.filePath(), false, saveAsCopy, _saveInherited);

    accept();
}

int MaterialSave::confirmOverwrite(const QString& filename)
{
    QMessageBox box(this);
    box.setIcon(QMessageBox::Question);
    box.setWindowTitle(QObject::tr("Confirm Overwrite"));

    QFileInfo info(_selectedFull);
    QString prompt = tr("Are you sure you want to save over '%1'?").arg(filename);
    box.setText(prompt);

    box.setInformativeText(tr("Saving over the original file may cause other documents to break. "
                              "This is not recommended."));

    box.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    box.setDefaultButton(QMessageBox::Cancel);
    box.setEscapeButton(QMessageBox::Cancel);

    int res = QMessageBox::Cancel;
    box.adjustSize();  // Silence warnings from Qt on Windows
    switch (box.exec()) {
        case QMessageBox::Ok:
            res = QMessageBox::Ok;
            break;
    }

    return res;
}

int MaterialSave::confirmNewMaterial()
{
    QMessageBox box(this);
    box.setIcon(QMessageBox::Question);
    box.setWindowTitle(tr("Confirm Save As New Material"));

    QString prompt = tr("Save as new material");
    box.setText(prompt);

    box.setInformativeText(tr(
        "This material already exists in this library. Would you like to save as a new material?"));

    box.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    box.setDefaultButton(QMessageBox::Cancel);
    box.setEscapeButton(QMessageBox::Cancel);

    int res = QMessageBox::Cancel;
    box.adjustSize();  // Silence warnings from Qt on Windows
    switch (box.exec()) {
        case QMessageBox::Ok:
            res = QMessageBox::Ok;
            break;
    }

    return res;
}

int MaterialSave::confirmCopy()
{
    QMessageBox box(this);
    box.setIcon(QMessageBox::Question);
    box.setWindowTitle(tr("Confirm Save As Copy"));

    QString prompt = tr("Save as Copy");
    box.setText(prompt);

    box.setInformativeText(tr("Saving a copy is not recommended as it can break other documents. "
                              "We recommend you save as a new material."));

    QPushButton* duplicateButton = box.addButton(tr("Save Copy"), QMessageBox::AcceptRole);
    QPushButton* newButton = box.addButton(tr("Save As New"), QMessageBox::ActionRole);
    QPushButton* cancelButton = box.addButton(QMessageBox::Cancel);

    box.setDefaultButton(cancelButton);
    box.setEscapeButton(cancelButton);

    box.adjustSize();  // Silence warnings from Qt on Windows
    box.exec();

    int res = QMessageBox::Cancel;
    if (box.clickedButton() == duplicateButton) {
        res = QMessageBox::Save;
    }
    else if (box.clickedButton() == newButton) {
        res = QMessageBox::Ok;
    }

    return res;
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
    for (auto& library : *libraries) {
        if (!library->isReadOnly()) {
            QVariant libraryVariant;
            libraryVariant.setValue(library);
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
    const std::shared_ptr<std::map<QString, std::shared_ptr<Materials::MaterialTreeNode>>>
        modelTree,
    const QIcon& folderIcon,
    const QIcon& icon)
{
    auto tree = ui->treeMaterials;
    for (auto& mat : *modelTree) {
        std::shared_ptr<Materials::MaterialTreeNode> nodePtr = mat.second;
        if (nodePtr->getType() == Materials::MaterialTreeNode::DataNode) {
            std::shared_ptr<Materials::Material> material = nodePtr->getData();
            QString uuid = material->getUUID();
            Base::Console().Log("Material path '%s'\n",
                                material->getDirectory().toStdString().c_str());

            auto card = new QStandardItem(icon, mat.first);
            card->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled
                           | Qt::ItemIsDropEnabled);
            card->setData(QVariant(uuid), Qt::UserRole);

            addExpanded(tree, &parent, card);
        }
        else {
            Base::Console().Log("Material folder path '%s'\n", mat.first.toStdString().c_str());
            auto node = new QStandardItem(folderIcon, mat.first);
            node->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled
                           | Qt::ItemIsDropEnabled | Qt::ItemIsEditable);
            addExpanded(tree, &parent, node);
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
        auto library = variant.value<std::shared_ptr<Materials::MaterialLibrary>>();
        QIcon icon(library->getIconPath());
        QIcon folderIcon(QString::fromStdString(":/icons/folder.svg"));
        _libraryName = library->getName();
        _selectedPath = QString::fromStdString("/") + _libraryName;
        _selectedFull = _selectedPath;

        auto lib = new QStandardItem(library->getName());
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
        _selectedFull = _selectedPath;
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
                _selectedFull = getPath(item);
                _selectedUUID = _selected.toString();
                _filename = item->text();
            }
            else {
                _selectedPath = getPath(item);
                _selectedFull = _selectedPath;
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

std::shared_ptr<Materials::MaterialLibrary> MaterialSave::currentLibrary()
{
    auto variant = ui->comboLibrary->currentData();
    return variant.value<std::shared_ptr<Materials::MaterialLibrary>>();
}

void MaterialSave::createFolder(const QString& path)
{
    auto library = currentLibrary();

    _manager.createFolder(library, path);
}

void MaterialSave::renameFolder(const QString& oldPath, const QString& newPath)
{
    auto library = currentLibrary();

    _manager.renameFolder(library, oldPath, newPath);
}

void MaterialSave::deleteRecursive(const QString& path)
{
    // This will delete files, folders, and any children
    auto library = currentLibrary();

    _manager.deleteRecursive(library, path);
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

    // Check for existing folders starting "New Folder" to prevent duplicates
    int newCount = 0;
    if (item->hasChildren()) {
        for (auto i = 0; i < item->rowCount(); i++) {
            auto child = item->child(i);
            if (child->text().startsWith(tr("New Folder"))) {
                newCount++;
            }
        }
    }

    // Folders have no associated data
    if (item->data(Qt::UserRole).isNull()) {
        Base::Console().Log("Add new folder to '%s'\n", item->text().toStdString().c_str());
        QIcon folderIcon(QString::fromStdString(":/icons/folder.svg"));

        QString folderName = tr("New Folder");
        if (newCount > 0) {
            folderName += QString::number(newCount);
        }
        auto node = new QStandardItem(folderIcon, folderName);
        node->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled
                       | Qt::ItemIsDropEnabled | Qt::ItemIsEditable);
        addExpanded(tree, item, node);

        Base::Console().Log("New folder index valid: %s\n",
                            node->index().isValid() ? "true" : "false");

        QItemSelectionModel* selectionModel = ui->treeMaterials->selectionModel();
        selectionModel->select(node->index(),
                               QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Current);

        createFolder(getPath(node));
    }
}

void MaterialSave::onItemChanged(QStandardItem* item)
{
    Base::Console().Log("MaterialSave::onItemChanged('%s')\n", item->text().toStdString().c_str());
    QString oldPath = _selectedPath;
    _selectedPath = getPath(item);
    _selectedFull = _selectedPath;
    Base::Console().Log("\tSelected path '%s'\n", _selectedPath.toStdString().c_str());
    renameFolder(oldPath, _selectedPath);
}

void MaterialSave::onFilename(const QString& text)
{
    Base::Console().Log("MaterialSave::onFilename('%s')\n", text.toStdString().c_str());

    _filename = text;
}

QString MaterialSave::pathFromIndex(const QModelIndex& index) const
{
    auto model = static_cast<const QStandardItemModel*>(index.model());
    auto item = model->itemFromIndex(index);
    return getPath(item);
}

void MaterialSave::onContextMenu(const QPoint& pos)
{
    Base::Console().Log("MaterialSave::onContextMenu(%d,%d)\n", pos.x(), pos.y());
    QModelIndex index = ui->treeMaterials->indexAt(pos);
    QString path = pathFromIndex(index);
    Base::Console().Log("\tindex at (%d,%d)->'%s'\n",
                        index.row(),
                        index.column(),
                        path.toStdString().c_str());


    QMenu contextMenu(tr("Context menu"), this);

    // QAction action1(tr("Delete"), this);
    // action1.setShortcut(Qt::Key_Delete);
    // connect(&action1, &QAction::triggered, this, &MaterialSave::onDelete);
    contextMenu.addAction(&_deleteAction);

    contextMenu.exec(ui->treeMaterials->mapToGlobal(pos));
}

void MaterialSave::onDelete(bool checked)
{
    Q_UNUSED(checked)

    Base::Console().Log("MaterialSave::onDelete()\n");
    QItemSelectionModel* selectionModel = ui->treeMaterials->selectionModel();
    if (!selectionModel->hasSelection()) {
        Base::Console().Log("\tNothing selected\n");
        return;
    }

    Base::Console().Log("\tSelected path '%s'\n", _selectedFull.toStdString().c_str());
    int res = confirmDelete(this);
    if (res == QMessageBox::Cancel) {
        return;
    }
}

int MaterialSave::confirmDelete(QWidget* parent)
{
    auto library = currentLibrary();

    if (library->isRoot(_selectedFull)) {
        return QMessageBox::Cancel;
    }

    QMessageBox box(parent ? parent : this);
    box.setIcon(QMessageBox::Question);
    box.setWindowTitle(QObject::tr("Confirm Delete"));

    QFileInfo info(_selectedFull);
    QString prompt = QObject::tr("Are you sure you want to delete '%1'?").arg(info.fileName());
    box.setText(prompt);

    if (selectedHasChildren()) {
        box.setInformativeText(QObject::tr("Removing this will also remove all contents."));
    }
    box.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    box.setDefaultButton(QMessageBox::Cancel);
    box.setEscapeButton(QMessageBox::Cancel);

    int res = QMessageBox::Cancel;
    box.adjustSize();  // Silence warnings from Qt on Windows
    switch (box.exec()) {
        case QMessageBox::Ok:
            deleteSelected();
            res = QMessageBox::Ok;
            break;
    }

    return res;
}

bool MaterialSave::selectedHasChildren()
{
    auto tree = ui->treeMaterials;
    auto model = static_cast<QStandardItemModel*>(tree->model());
    auto current = tree->currentIndex();
    if (!current.isValid()) {
        current = model->index(0, 0);
    }
    auto item = model->itemFromIndex(current);

    return item->hasChildren();
}

void MaterialSave::deleteSelected()
{
    Base::Console().Log("\tDelete selected path '%s'\n", _selectedFull.toStdString().c_str());
    auto library = currentLibrary();

    if (!library->isRoot(_selectedFull)) {
        _manager.deleteRecursive(library, _selectedFull);
        removeSelectedFromTree();
    }
}

void MaterialSave::removeChildren(QStandardItem* item)
{
    while (item->rowCount() > 0) {
        auto child = item->child(0);
        removeChildren(child);
        item->removeRow(0);
    }
}

void MaterialSave::removeSelectedFromTree()
{
    auto tree = ui->treeMaterials;
    auto model = static_cast<QStandardItemModel*>(tree->model());
    auto current = tree->currentIndex();
    if (current.row() >= 0) {
        auto item = model->itemFromIndex(current);

        // Remove the children
        removeChildren(item);
        item->parent()->removeRow(item->row());
    }

    QItemSelectionModel* selectionModel = ui->treeMaterials->selectionModel();
    selectionModel->clear();
}

#include "moc_MaterialSave.cpp"
