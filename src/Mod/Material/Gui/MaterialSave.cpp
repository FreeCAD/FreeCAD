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

#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/MainWindow.h>
#include <Gui/Tools.h>

#include <Mod/Material/App/MaterialLibrary.h>

#include "MaterialsEditor.h"
#include "MaterialSave.h"
#include "ui_MaterialSave.h"

using namespace MatGui;

/* TRANSLATOR MatGui::MaterialsEditor */

MaterialSave::MaterialSave(const std::shared_ptr<Materials::Material>& material, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui_MaterialSave)
    , _material(material)
    , _saveInherited(true)
    , _selectedPath(QStringLiteral("/"))
    , _selectedFull(QStringLiteral("/"))
    , _selectedUUID()
    , _deleteAction(this)
{
    ui->setupUi(this);

    setLibraries();
    createModelTree();
    showSelectedTree();

    if (_material->getName().length() > 0) {
        ui->editFilename->setText(_material->getName() + QStringLiteral(".FCMat"));
    }
    else {
        ui->editFilename->setText(QStringLiteral("NewMaterial.FCMat"));
    }
    _filename = QString(ui->editFilename->text());  // No filename by default

    ui->checkDerived->setChecked(_saveInherited);
#if QT_VERSION >= QT_VERSION_CHECK(6,7,0)
    connect(ui->checkDerived, &QCheckBox::checkStateChanged, this, &MaterialSave::onInherited);
#else
    connect(ui->checkDerived, &QCheckBox::stateChanged, this, &MaterialSave::onInherited);
#endif
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
    _deleteAction.setShortcut(Gui::QtTools::deleteKeySequence());

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

    QString name = _filename.remove(QStringLiteral(".FCMat"), Qt::CaseInsensitive);
    if (name != _material->getName()) {
        _material->setName(name);
        _material->setEditStateAlter();  // ? Does a name change count?
    }

    auto variant = ui->comboLibrary->currentData();
    auto library = variant.value<std::shared_ptr<Materials::MaterialLibrary>>();
    QFileInfo filepath(_selectedPath + QStringLiteral("/") + name
                       + QStringLiteral(".FCMat"));

    /*if (library->fileExists(filepath.filePath()))*/ {
        // confirm overwrite
        auto res = confirmOverwrite(_filename);
        if (res == QMessageBox::Cancel) {
            return;
        }

        Materials::MaterialManager::getManager()
            .saveMaterial(library, _material, filepath.filePath(), true, false, _saveInherited);
        accept();
        return;
    }

    bool saveAsCopy = false;
    if (Materials::MaterialManager::getManager().exists(_material->getUUID())) {
        // Does it already exist in this library?
        if (Materials::MaterialManager::getManager().exists(*library, _material->getUUID())) {
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
            if (res == QMessageBox::Save) {
                // QMessageBox::Save saves as normal, a duplicate
                saveAsCopy = true;
            }
            // QMessageBox::Ok saves a new material
        }
    }

    Materials::MaterialManager::getManager()
        .saveMaterial(library, _material, filepath.filePath(), false, saveAsCopy, _saveInherited);

    accept();
}

int MaterialSave::confirmOverwrite(const QString& filename)
{
    QMessageBox box(this);
    box.setIcon(QMessageBox::Question);
    box.setWindowTitle(QObject::tr("Confirm Overwrite"));

    QFileInfo info(_selectedFull);
    QString prompt = tr("Save over '%1'?").arg(filename);
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
    box.setWindowTitle(tr("Confirm Save as New Material"));

    QString prompt = tr("Save as new material");
    box.setText(prompt);

    box.setInformativeText(tr(
        "This material already exists in this library. Save as a new material?"));

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
    box.setWindowTitle(tr("Confirm Save as Copy"));

    QString prompt = tr("Save as copy");
    box.setText(prompt);

    box.setInformativeText(tr("Saving a copy is not recommended as it can break other documents. "
                              "It is recommended to save as a new material."));

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
    auto libraries = Materials::MaterialManager::getManager().getLibraries();
    for (auto& library : *libraries) {
        if (library->isLocal()) {
            if (!library->isReadOnly()) {
                QVariant libraryVariant;
                libraryVariant.setValue(library);
                ui->comboLibrary->addItem(library->getName(), libraryVariant);
            }
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
        if (nodePtr->getType() == Materials::MaterialTreeNode::NodeType::DataNode) {
            QString uuid = nodePtr->getUUID();

            auto card = new QStandardItem(icon, mat.first);
            card->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled
                           | Qt::ItemIsDropEnabled);
            card->setData(QVariant(uuid), Qt::UserRole);

            addExpanded(tree, &parent, card);
        }
        else {
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
        auto icon = MaterialsEditor::getIcon(library);
        QIcon folderIcon(QStringLiteral(":/icons/folder.svg"));
        _libraryName = library->getName();
        _selectedPath = QStringLiteral("/") + _libraryName;
        _selectedFull = _selectedPath;

        auto lib = new QStandardItem(library->getName());
        lib->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
        addExpanded(tree, model, lib);

        auto modelTree = Materials::MaterialManager::getManager().getMaterialTree(*library);
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
    QString path = QStringLiteral("/");
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
    auto model = static_cast<QStandardItemModel*>(ui->treeMaterials->model());
    QModelIndexList indexes = selected.indexes();
    if (indexes.isEmpty()) {
        _selectedPath = QStringLiteral("/") + _libraryName;
        _selectedFull = _selectedPath;
        _selectedUUID = QString();
        return;
    }
    for (auto it = indexes.begin(); it != indexes.end(); it++) {
        QStandardItem* item = model->itemFromIndex(*it);
        if (item) {
            auto _selected = item->data(Qt::UserRole);
            if (_selected.isValid()) {
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

    Materials::MaterialManager::getManager().createFolder(library, path);
}

void MaterialSave::renameFolder(const QString& oldPath, const QString& newPath)
{
    auto library = currentLibrary();

    Materials::MaterialManager::getManager().renameFolder(library, oldPath, newPath);
}

void MaterialSave::deleteRecursive(const QString& path)
{
    // This will delete files, folders, and any children
    auto library = currentLibrary();

    Materials::MaterialManager::getManager().deleteRecursive(library, path);
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
            if (child->text().startsWith(tr("New folder"))) {
                newCount++;
            }
        }
    }

    // Folders have no associated data
    if (item->data(Qt::UserRole).isNull()) {
        QIcon folderIcon(QStringLiteral(":/icons/folder.svg"));

        QString folderName = tr("New folder");
        if (newCount > 0) {
            folderName += QString::number(newCount);
        }
        auto node = new QStandardItem(folderIcon, folderName);
        node->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled
                       | Qt::ItemIsDropEnabled | Qt::ItemIsEditable);
        addExpanded(tree, item, node);

        QItemSelectionModel* selectionModel = ui->treeMaterials->selectionModel();
        selectionModel->select(node->index(),
                               QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Current);

        createFolder(getPath(node));
    }
}

void MaterialSave::onItemChanged(QStandardItem* item)
{
    QString oldPath = _selectedPath;
    _selectedPath = getPath(item);
    _selectedFull = _selectedPath;

    renameFolder(oldPath, _selectedPath);
}

void MaterialSave::onFilename(const QString& text)
{
    _filename = text;
}

QString MaterialSave::pathFromIndex(const QModelIndex& index) const
{
    auto model = dynamic_cast<const QStandardItemModel*>(index.model());
    auto item = model->itemFromIndex(index);
    return getPath(item);
}

void MaterialSave::onContextMenu(const QPoint& pos)
{
    QMenu contextMenu(tr("Context Menu"), this);

    contextMenu.addAction(&_deleteAction);

    contextMenu.exec(ui->treeMaterials->mapToGlobal(pos));
}

void MaterialSave::onDelete(bool checked)
{
    Q_UNUSED(checked)

    QItemSelectionModel* selectionModel = ui->treeMaterials->selectionModel();
    if (!selectionModel->hasSelection()) {
        return;
    }

    confirmDelete(this);
}

int MaterialSave::confirmDelete(QWidget* parent)
{
    auto library = currentLibrary();

    // if (library->isRoot(_selectedFull)) {
    //     return QMessageBox::Cancel;
    // }

    QMessageBox box(parent ? parent : this);
    box.setIcon(QMessageBox::Question);
    box.setWindowTitle(QObject::tr("Confirm Delete"));

    QFileInfo info(_selectedFull);
    QString prompt = QObject::tr("Delete '%1'?").arg(info.fileName());
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
    auto library = currentLibrary();

    // if (!library->isRoot(_selectedFull)) {
    //     Materials::MaterialManager::getManager().deleteRecursive(library, _selectedFull);
    //     removeSelectedFromTree();
    // }
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
