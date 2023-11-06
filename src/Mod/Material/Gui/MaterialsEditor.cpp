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
#include <QColorDialog>
#include <QDesktopServices>
#include <QIODevice>
#include <QItemSelectionModel>
#include <QMenu>
#include <QMessageBox>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QVariant>
#endif

#include <limits>

#include <App/Application.h>
#include <App/License.h>
#include <Base/Interpreter.h>
#include <Base/Quantity.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/InputField.h>
#include <Gui/PrefWidgets.h>
#include <Gui/SpinBox.h>
#include <Gui/WaitCursor.h>

#include <Mod/Material/App/Exceptions.h>
#include <Mod/Material/App/ModelManager.h>

#include "MaterialDelegate.h"
#include "MaterialSave.h"
#include "MaterialsEditor.h"
#include "ModelSelect.h"
#include "ui_MaterialsEditor.h"


using namespace MatGui;

/* TRANSLATOR MatGui::MaterialsEditor */

MaterialsEditor::MaterialsEditor(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui_MaterialsEditor)
    , _material(std::make_shared<Materials::Material>())
    , _edited(false)
{
    ui->setupUi(this);

    getFavorites();
    getRecents();

    createMaterialTree();
    createPhysicalTree();
    createAppearanceTree();
    createPreviews();
    setMaterialDefaults();

    ui->buttonURL->setIcon(QIcon(QString::fromStdString(":/icons/internet-web-browser.svg")));

    connect(ui->standardButtons->button(QDialogButtonBox::Ok),
            &QPushButton::clicked,
            this,
            &MaterialsEditor::onOk);
    connect(ui->standardButtons->button(QDialogButtonBox::Cancel),
            &QPushButton::clicked,
            this,
            &MaterialsEditor::onCancel);
    connect(ui->standardButtons->button(QDialogButtonBox::Save),
            &QPushButton::clicked,
            this,
            &MaterialsEditor::onSave);

    connect(ui->editName, &QLineEdit::textEdited, this, &MaterialsEditor::onName);
    connect(ui->editAuthor, &QLineEdit::textEdited, this, &MaterialsEditor::onAuthor);
    connect(ui->editLicense, &QLineEdit::textEdited, this, &MaterialsEditor::onLicense);
    connect(ui->editSourceURL, &QLineEdit::textEdited, this, &MaterialsEditor::onSourceURL);
    connect(ui->editSourceReference,
            &QLineEdit::textEdited,
            this,
            &MaterialsEditor::onSourceReference);
    connect(ui->editDescription, &QTextEdit::textChanged, this, &MaterialsEditor::onDescription);

    connect(ui->buttonURL, &QPushButton::clicked, this, &MaterialsEditor::onURL);
    connect(ui->buttonPhysicalAdd, &QPushButton::clicked, this, &MaterialsEditor::onPhysicalAdd);
    connect(ui->buttonPhysicalRemove,
            &QPushButton::clicked,
            this,
            &MaterialsEditor::onPhysicalRemove);
    connect(ui->buttonAppearanceAdd,
            &QPushButton::clicked,
            this,
            &MaterialsEditor::onAppearanceAdd);
    connect(ui->buttonAppearanceRemove,
            &QPushButton::clicked,
            this,
            &MaterialsEditor::onAppearanceRemove);
    connect(ui->buttonInheritNew,
            &QPushButton::clicked,
            this,
            &MaterialsEditor::onInheritNewMaterial);
    connect(ui->buttonNew, &QPushButton::clicked, this, &MaterialsEditor::onNewMaterial);
    connect(ui->buttonFavorite, &QPushButton::clicked, this, &MaterialsEditor::onFavourite);

    QItemSelectionModel* selectionModel = ui->treeMaterials->selectionModel();
    connect(selectionModel,
            &QItemSelectionModel::selectionChanged,
            this,
            &MaterialsEditor::onSelectMaterial);
    connect(ui->treeMaterials, &QTreeView::doubleClicked, this, &MaterialsEditor::onDoubleClick);

    ui->treeMaterials->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->treeMaterials,
            &QWidget::customContextMenuRequested,
            this,
            &MaterialsEditor::onContextMenu);
}

void MaterialsEditor::getFavorites()
{
    _favorites.clear();

    auto param = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/Favorites");
    int count = param->GetInt("Favorites", 0);
    for (int i = 0; i < count; i++) {
        QString key = QString::fromLatin1("FAV%1").arg(i);
        QString uuid = QString::fromStdString(param->GetASCII(key.toStdString().c_str(), ""));
        _favorites.push_back(uuid);
    }
}

void MaterialsEditor::saveFavorites()
{
    auto param = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/Favorites");

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

void MaterialsEditor::addFavorite(const QString& uuid)
{
    // Ensure it is a material. New, unsaved materials will not be
    try {
        auto material = _materialManager.getMaterial(uuid);
        Q_UNUSED(material)
    }
    catch (const Materials::MaterialNotFound&) {
        return;
    }

    if (!isFavorite(uuid)) {
        _favorites.push_back(uuid);
        saveFavorites();
        refreshMaterialTree();
    }
}

void MaterialsEditor::removeFavorite(const QString& uuid)
{
    if (isFavorite(uuid)) {
        _favorites.remove(uuid);
        saveFavorites();
        refreshMaterialTree();
    }
}

bool MaterialsEditor::isFavorite(const QString& uuid) const
{
    for (auto it : _favorites) {
        if (it == uuid) {
            return true;
        }
    }
    return false;
}


void MaterialsEditor::getRecents()
{
    _recents.clear();

    auto param = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/Recent");
    _recentMax = param->GetInt("RecentMax", 5);
    int count = param->GetInt("Recent", 0);
    for (int i = 0; i < count; i++) {
        QString key = QString::fromLatin1("MRU%1").arg(i);
        QString uuid = QString::fromStdString(param->GetASCII(key.toStdString().c_str(), ""));
        _recents.push_back(uuid);
    }
}

void MaterialsEditor::saveRecents()
{
    auto param = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/Recent");

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

void MaterialsEditor::addRecent(const QString& uuid)
{
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

bool MaterialsEditor::isRecent(const QString& uuid) const
{
    for (auto it : _recents) {
        if (it == uuid) {
            return true;
        }
    }
    return false;
}

void MaterialsEditor::onName(const QString& text)
{
    _material->setName(text);
}

void MaterialsEditor::onAuthor(const QString& text)
{
    _material->setAuthor(text);
}

void MaterialsEditor::onLicense(const QString& text)
{
    _material->setLicense(text);
}

void MaterialsEditor::onSourceURL(const QString& text)
{
    _material->setURL(text);
}

void MaterialsEditor::onSourceReference(const QString& text)
{
    _material->setReference(text);
}

void MaterialsEditor::onDescription()
{
    _material->setDescription(ui->editDescription->toPlainText());
}

void MaterialsEditor::propertyChange(const QString& property, const QString value)
{
    Base::Console().Log("MaterialsEditor::propertyChange(%s) = '%s'\n",
                        property.toStdString().c_str(),
                        value.toStdString().c_str());
    if (_material->hasPhysicalProperty(property)) {
        _material->setPhysicalValue(property, value);
    }
    else if (_material->hasAppearanceProperty(property)) {
        _material->setAppearanceValue(property, value);
        updatePreview();
    }
    _edited = true;
}

void MaterialsEditor::onURL(bool checked)
{
    Q_UNUSED(checked)

    Base::Console().Log("URL\n");
    QString url = ui->editSourceURL->text();
    if (url.length() > 0) {
        QDesktopServices::openUrl(QUrl(url, QUrl::TolerantMode));
    }
}

void MaterialsEditor::onPhysicalAdd(bool checked)
{
    Q_UNUSED(checked)

    ModelSelect dialog(this, Materials::ModelFilter_Physical);
    dialog.setModal(true);
    if (dialog.exec() == QDialog::Accepted) {
        QString selected = dialog.selectedModel();
        Base::Console().Log("Selected model '%s'\n", selected.toStdString().c_str());
        _material->addPhysical(selected);
        updateMaterial();
    }
    else {
        Base::Console().Log("No model selected\n");
    }
}

void MaterialsEditor::onPhysicalRemove(bool checked)
{
    Q_UNUSED(checked)

    QItemSelectionModel* selectionModel = ui->treePhysicalProperties->selectionModel();
    if (selectionModel->hasSelection()) {
        const QModelIndex index = selectionModel->currentIndex().siblingAtColumn(0);

        const QStandardItemModel* treeModel = static_cast<const QStandardItemModel*>(index.model());

        // Check we're the material model root.
        auto item = treeModel->itemFromIndex(index);
        auto group = item->parent();
        if (!group) {
            QString propertyName = index.data().toString();

            QString uuid = _material->getModelByName(propertyName);
            _material->removePhysical(uuid);
            updateMaterial();
        }
    }
}

void MaterialsEditor::onAppearanceAdd(bool checked)
{
    Q_UNUSED(checked)

    ModelSelect dialog(this, Materials::ModelFilter_Appearance);
    dialog.setModal(true);
    if (dialog.exec() == QDialog::Accepted) {
        QString selected = dialog.selectedModel();
        Base::Console().Log("Selected model '%s'\n", selected.toStdString().c_str());
        _material->addAppearance(selected);
        updateMaterial();
    }
    else {
        Base::Console().Log("No model selected\n");
    }
}

void MaterialsEditor::onAppearanceRemove(bool checked)
{
    Q_UNUSED(checked)

    QItemSelectionModel* selectionModel = ui->treeAppearance->selectionModel();
    if (selectionModel->hasSelection()) {
        const QModelIndex index = selectionModel->currentIndex().siblingAtColumn(0);

        const QStandardItemModel* treeModel = static_cast<const QStandardItemModel*>(index.model());

        // Check we're the material model root.
        auto item = treeModel->itemFromIndex(index);
        auto group = item->parent();
        if (!group) {
            QString propertyName = index.data().toString();

            QString uuid = _material->getModelByName(propertyName);
            _material->removeAppearance(uuid);
            updateMaterial();
        }
    }
}

void MaterialsEditor::onFavourite(bool checked)
{
    Q_UNUSED(checked)

    Base::Console().Log("Favorite\n");
    auto selected = _material->getUUID();
    if (isFavorite(selected)) {
        removeFavorite(selected);
    }
    else {
        addFavorite(selected);
    }
}

void MaterialsEditor::setMaterialDefaults()
{
    _material->setName(tr("Unnamed"));
    std::string Author = App::GetApplication()
                             .GetParameterGroupByPath("User parameter:BaseApp/Preferences/Document")
                             ->GetASCII("prefAuthor", "");
    _material->setAuthor(QString::fromStdString(Author));

    // license stuff
    auto paramGrp {App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Document")};
    auto index = static_cast<int>(paramGrp->GetInt("prefLicenseType", 0));
    const char* name = App::licenseItems.at(index).at(App::posnOfFullName);
    // const char* url = App::licenseItems.at(index).at(App::posnOfUrl);
    // std::string licenseUrl = (paramGrp->GetASCII("prefLicenseUrl", url));
    _material->setLicense(QString::fromStdString(name));

    // Empty materials will have no parent
    _materialManager.dereference(_material);

    updateMaterial();
    _material->resetEditState();
}

void MaterialsEditor::onNewMaterial(bool checked)
{
    Q_UNUSED(checked)

    Base::Console().Log("New Material\n");

    // Ensure data is saved (or discarded) before changing materials
    if (_material->getEditState() != Materials::Material::ModelEdit_None) {
        // Prompt the user to save or discard changes
        Base::Console().Log("*** Material edited!!!\n");
        int res = confirmSave(this);
        if (res == QMessageBox::Cancel) {
            return;
        }
    }

    // Create a new material
    _material = std::make_shared<Materials::Material>();
    setMaterialDefaults();
}

void MaterialsEditor::onInheritNewMaterial(bool checked)
{
    Q_UNUSED(checked)

    Base::Console().Log("Inherit New Material\n");

    // Save the current UUID to use as out parent
    auto parent = _material->getUUID();

    // Ensure data is saved (or discarded) before changing materials
    if (_material->getEditState() != Materials::Material::ModelEdit_None) {
        // Prompt the user to save or discard changes
        Base::Console().Log("*** Material edited!!!\n");
        int res = confirmSave(this);
        if (res == QMessageBox::Cancel) {
            return;
        }
    }

    // Create a new material
    _material = std::make_shared<Materials::Material>();
    _material->setParentUUID(parent);
    setMaterialDefaults();
}

void MaterialsEditor::onOk(bool checked)
{
    Q_UNUSED(checked)

    accept();
}

void MaterialsEditor::onCancel(bool checked)
{
    Q_UNUSED(checked)

    reject();
}

void MaterialsEditor::onSave(bool checked)
{
    Q_UNUSED(checked)

    saveMaterial();
}

void MaterialsEditor::saveMaterial()
{
    MaterialSave dialog(_material, this);
    dialog.setModal(true);
    if (dialog.exec() == QDialog::Accepted) {
        updateMaterialGeneral();
        _material->resetEditState();
        refreshMaterialTree();
    }
}

void MaterialsEditor::accept()
{
    addRecent(_material->getUUID());
    QDialog::accept();
}

void MaterialsEditor::reject()
{
    QDialog::reject();
}

// QIcon MaterialsEditor::errorIcon(const QIcon &icon) const {
//     auto pixmap = icon.pixmap();
// }

void MaterialsEditor::addMaterials(
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
            auto material = nodePtr->getData();
            QString uuid = material->getUUID();
            // Base::Console().Log("Material path '%s'\n",
            //                     material->getDirectory().toStdString().c_str());

            // auto card = new QStandardItem(icon, material->getName());
            auto card = new QStandardItem(icon, mat.first);
            card->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled
                           | Qt::ItemIsDropEnabled);
            card->setData(QVariant(uuid), Qt::UserRole);

            addExpanded(tree, &parent, card);
        }
        else {
            auto node = new QStandardItem(folderIcon, mat.first);
            addExpanded(tree, &parent, node);
            node->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
            auto treeMap = nodePtr->getFolder();
            addMaterials(*node, treeMap, folderIcon, icon);
        }
    }
}

void MaterialsEditor::addExpanded(QTreeView* tree, QStandardItem* parent, QStandardItem* child)
{
    parent->appendRow(child);
    tree->setExpanded(child->index(), true);
}

void MaterialsEditor::addExpanded(QTreeView* tree, QStandardItemModel* parent, QStandardItem* child)
{
    parent->appendRow(child);
    tree->setExpanded(child->index(), true);
}

void MaterialsEditor::createPhysicalTree()
{
    auto tree = ui->treePhysicalProperties;
    auto model = new QStandardItemModel();
    tree->setModel(model);

    QStringList headers;
    headers.append(tr("Property"));
    headers.append(tr("Value"));
    headers.append(tr("Type"));
    model->setHorizontalHeaderLabels(headers);

    tree->setColumnWidth(0, 250);
    tree->setColumnWidth(1, 250);
    tree->setColumnHidden(2, true);

    tree->setHeaderHidden(false);
    tree->setUniformRowHeights(true);
    MaterialDelegate* delegate = new MaterialDelegate(this);
    tree->setItemDelegateForColumn(1, delegate);

    connect(delegate, &MaterialDelegate::propertyChange, this, &MaterialsEditor::propertyChange);

    // QItemSelectionModel* selectionModel = ui->treePhysicalProperties->selectionModel();
    // connect(selectionModel,
    //         &QItemSelectionModel::selectionChanged,
    //         this,
    //         &MaterialsEditor::onSelectPhysicalProperty);
}

void MaterialsEditor::createPreviews()
{
    _rendered = new QSvgWidget(QString::fromStdString(":/icons/preview-rendered.svg"));
    _rendered->setMaximumWidth(64);
    _rendered->setMinimumHeight(64);
    ui->layoutAppearance->addWidget(_rendered);

    _vectored = new QSvgWidget(QString::fromStdString(":/icons/preview-vector.svg"));
    _vectored->setMaximumWidth(64);
    _vectored->setMinimumHeight(64);
    ui->layoutAppearance->addWidget(_vectored);

    updatePreview();
}

void MaterialsEditor::createAppearanceTree()
{
    auto tree = ui->treeAppearance;
    auto model = new QStandardItemModel();
    tree->setModel(model);

    QStringList headers;
    headers.append(tr("Property"));
    headers.append(tr("Value"));
    headers.append(tr("Type"));
    model->setHorizontalHeaderLabels(headers);

    tree->setColumnWidth(0, 250);
    tree->setColumnWidth(1, 250);
    tree->setColumnHidden(2, true);

    tree->setHeaderHidden(false);
    tree->setUniformRowHeights(false);
    MaterialDelegate* delegate = new MaterialDelegate(this);
    tree->setItemDelegateForColumn(1, delegate);

    connect(delegate, &MaterialDelegate::propertyChange, this, &MaterialsEditor::propertyChange);
}

void MaterialsEditor::addRecents(QStandardItem* parent)
{
    auto tree = ui->treeMaterials;
    for (auto& uuid : _recents) {
        try {
            auto material = getMaterialManager().getMaterial(uuid);

            QIcon icon = QIcon(material->getLibrary()->getIconPath());
            auto card = new QStandardItem(icon, libraryPath(material));
            card->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled
                           | Qt::ItemIsDropEnabled);
            card->setData(QVariant(uuid), Qt::UserRole);

            addExpanded(tree, parent, card);
        }
        catch (const Materials::MaterialNotFound&) {
        }
    }
}

void MaterialsEditor::addFavorites(QStandardItem* parent)
{
    auto tree = ui->treeMaterials;
    for (auto& uuid : _favorites) {
        try {
            auto material = getMaterialManager().getMaterial(uuid);

            QIcon icon = QIcon(material->getLibrary()->getIconPath());
            auto card = new QStandardItem(icon, libraryPath(material));
            card->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled
                           | Qt::ItemIsDropEnabled);
            card->setData(QVariant(uuid), Qt::UserRole);

            addExpanded(tree, parent, card);
        }
        catch (const Materials::MaterialNotFound&) {
        }
    }
}

void MaterialsEditor::fillMaterialTree()
{
    auto tree = ui->treeMaterials;
    auto model = static_cast<QStandardItemModel*>(tree->model());

    auto lib = new QStandardItem(tr("Favorites"));
    lib->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
    addExpanded(tree, model, lib);
    addFavorites(lib);

    lib = new QStandardItem(tr("Recent"));
    lib->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
    addExpanded(tree, model, lib);
    addRecents(lib);

    auto libraries = Materials::MaterialManager::getMaterialLibraries();
    for (const auto& library : *libraries) {
        lib = new QStandardItem(library->getName());
        lib->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
        addExpanded(tree, model, lib);

        QIcon icon(library->getIconPath());
        QIcon folderIcon(QString::fromStdString(":/icons/folder.svg"));

        auto modelTree = _materialManager.getMaterialTree(library);
        addMaterials(*lib, modelTree, folderIcon, icon);
    }
}

void MaterialsEditor::createMaterialTree()
{
    // Materials::ModelManager &modelManager = getModelManager();
    // Q_UNUSED(modelManager)

    auto tree = ui->treeMaterials;
    auto model = new QStandardItemModel();
    tree->setModel(model);

    tree->setHeaderHidden(true);
    fillMaterialTree();
}

void MaterialsEditor::refreshMaterialTree()
{
    auto tree = ui->treeMaterials;
    auto model = static_cast<QStandardItemModel*>(tree->model());
    model->clear();

    fillMaterialTree();
}

void MaterialsEditor::updatePreview() const
{
    QString diffuseColor;
    QString highlightColor;
    QString sectionColor;

    if (_material->hasAppearanceProperty(QString::fromStdString("DiffuseColor"))) {
        diffuseColor = _material->getAppearanceValueString(QString::fromStdString("DiffuseColor"));
    }
    else if (_material->hasAppearanceProperty(QString::fromStdString("ViewColor"))) {
        diffuseColor = _material->getAppearanceValueString(QString::fromStdString("ViewColor"));
    }
    else if (_material->hasAppearanceProperty(QString::fromStdString("Color"))) {
        diffuseColor = _material->getAppearanceValueString(QString::fromStdString("Color"));
    }

    if (_material->hasAppearanceProperty(QString::fromStdString("SpecularColor"))) {
        highlightColor =
            _material->getAppearanceValueString(QString::fromStdString("SpecularColor"));
    }

    if (_material->hasAppearanceProperty(QString::fromStdString("SectionColor"))) {
        sectionColor = _material->getAppearanceValueString(QString::fromStdString("SectionColor"));
    }

    if ((diffuseColor.length() + highlightColor.length()) > 0) {
        auto file = QFile(QString::fromStdString(":/icons/preview-rendered.svg"));
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString svg = QTextStream(&file).readAll();
            file.close();
            if (diffuseColor.length() > 0) {
                svg =
                    svg.replace(QString::fromStdString("#d3d7cf"), getColorHash(diffuseColor, 255));
                svg =
                    svg.replace(QString::fromStdString("#555753"), getColorHash(diffuseColor, 125));
            }
            if (highlightColor.length() > 0) {
                svg = svg.replace(QString::fromStdString("#fffffe"),
                                  getColorHash(highlightColor, 255));
            }
            _rendered->load(svg.toUtf8());
        }
    }

    if ((diffuseColor.length() + sectionColor.length()) > 0) {
        auto file = QFile(QString::fromStdString(":/icons/preview-vector.svg"));
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString svg = QTextStream(&file).readAll();
            file.close();
            if (diffuseColor.length() > 0) {
                svg =
                    svg.replace(QString::fromStdString("#d3d7cf"), getColorHash(diffuseColor, 255));
                svg =
                    svg.replace(QString::fromStdString("#555753"), getColorHash(diffuseColor, 125));
            }
            if (sectionColor.length() > 0) {
                svg =
                    svg.replace(QString::fromStdString("#ffffff"), getColorHash(sectionColor, 255));
            }
            _vectored->load(svg.toUtf8());
        }
    }
}

QString MaterialsEditor::getColorHash(const QString& colorString, int colorRange) const
{
    /*
        returns a '#000000' string from a '(0.1,0.2,0.3)' string. Optionally the string
        has a fourth value for alpha (transparency)
    */
    std::stringstream stream(colorString.toStdString());

    char c;
    stream >> c;  // read "("
    double red;
    stream >> red;
    stream >> c;  // ","
    double green;
    stream >> green;
    stream >> c;  // ","
    double blue;
    stream >> blue;
    stream >> c;  // ","
    double alpha = 1.0;
    if (c == ',') {
        stream >> alpha;
    }

    QColor color(static_cast<int>(red * colorRange),
                 static_cast<int>(green * colorRange),
                 static_cast<int>(blue * colorRange),
                 static_cast<int>(alpha * colorRange));
    return color.name();
}

void MaterialsEditor::updateMaterialAppearance()
{
    QTreeView* tree = ui->treeAppearance;
    QStandardItemModel* treeModel = static_cast<QStandardItemModel*>(tree->model());
    treeModel->clear();

    QStringList headers;
    headers.append(tr("Property"));
    headers.append(tr("Value"));
    headers.append(tr("Type"));
    treeModel->setHorizontalHeaderLabels(headers);

    tree->setColumnWidth(0, 250);
    tree->setColumnWidth(1, 250);
    tree->setColumnHidden(2, true);

    auto models = _material->getAppearanceModels();
    if (models) {
        for (auto it = models->begin(); it != models->end(); it++) {
            QString uuid = *it;
            try {
                auto model = getModelManager().getModel(uuid);
                QString name = model->getName();

                auto modelRoot = new QStandardItem(name);
                modelRoot->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDragEnabled
                                    | Qt::ItemIsDropEnabled);
                addExpanded(tree, treeModel, modelRoot);
                for (auto itp = model->begin(); itp != model->end(); itp++) {
                    QList<QStandardItem*> items;

                    QString key = itp->first;
                    auto propertyItem = new QStandardItem(key);
                    propertyItem->setToolTip(itp->second.getDescription());
                    items.append(propertyItem);

                    auto valueItem = new QStandardItem(_material->getAppearanceValueString(key));
                    valueItem->setToolTip(itp->second.getDescription());
                    QVariant variant;
                    variant.setValue(_material);
                    valueItem->setData(variant);
                    items.append(valueItem);

                    auto typeItem = new QStandardItem(itp->second.getPropertyType());
                    items.append(typeItem);

                    auto unitsItem = new QStandardItem(itp->second.getUnits());
                    items.append(unitsItem);

                    modelRoot->appendRow(items);
                    tree->setExpanded(modelRoot->index(), true);
                }
            }
            catch (Materials::ModelNotFound const&) {
            }
        }
    }
}

void MaterialsEditor::updateMaterialProperties()
{
    QTreeView* tree = ui->treePhysicalProperties;
    QStandardItemModel* treeModel = static_cast<QStandardItemModel*>(tree->model());
    treeModel->clear();

    QStringList headers;
    headers.append(tr("Property"));
    headers.append(tr("Value"));
    headers.append(tr("Type"));
    headers.append(tr("Units"));
    treeModel->setHorizontalHeaderLabels(headers);

    tree->setColumnWidth(0, 250);
    tree->setColumnWidth(1, 250);
    tree->setColumnHidden(2, true);
    tree->setColumnHidden(3, true);

    auto models = _material->getPhysicalModels();
    if (models) {
        for (auto it = models->begin(); it != models->end(); it++) {
            QString uuid = *it;
            try {
                auto model = getModelManager().getModel(uuid);
                QString name = model->getName();

                auto modelRoot = new QStandardItem(name);
                modelRoot->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDragEnabled
                                    | Qt::ItemIsDropEnabled);
                addExpanded(tree, treeModel, modelRoot);
                for (auto itp = model->begin(); itp != model->end(); itp++) {
                    QList<QStandardItem*> items;

                    QString key = itp->first;
                    Materials::ModelProperty modelProperty =
                        static_cast<Materials::ModelProperty>(itp->second);
                    auto propertyItem = new QStandardItem(key);
                    propertyItem->setToolTip(modelProperty.getDescription());
                    items.append(propertyItem);

                    auto valueItem = new QStandardItem(_material->getPhysicalValueString(key));
                    valueItem->setToolTip(modelProperty.getDescription());
                    QVariant variant;
                    variant.setValue(_material);
                    valueItem->setData(variant);
                    items.append(valueItem);

                    auto typeItem = new QStandardItem(modelProperty.getPropertyType());
                    items.append(typeItem);

                    auto unitsItem = new QStandardItem(modelProperty.getUnits());
                    items.append(unitsItem);

                    // addExpanded(tree, modelRoot, propertyItem);
                    modelRoot->appendRow(items);
                    tree->setExpanded(modelRoot->index(), true);
                }
            }
            catch (Materials::ModelNotFound const&) {
            }
        }
    }
}

QString MaterialsEditor::libraryPath(std::shared_ptr<Materials::Material> material) const
{
    QString path;
    auto library = material->getLibrary();
    if (library) {
        path = QString::fromStdString("/%1/%2")
                   .arg(material->getLibrary()->getName())
                   .arg(material->getDirectory());
    }
    else {
        path = QString::fromStdString("%1").arg(material->getDirectory());
    }

    return path;
}

void MaterialsEditor::updateMaterialGeneral()
{
    QString parentString;
    try {
        auto parent = _materialManager.getParent(_material);
        parentString = libraryPath(parent);
    }
    catch (const Materials::MaterialNotFound&) {
    }

    // Update the general information
    ui->editName->setText(_material->getName());
    ui->editAuthor->setText(_material->getAuthor());
    ui->editLicense->setText(_material->getLicense());
    ui->editParent->setText(parentString);
    ui->editParent->setReadOnly(true);
    ui->editSourceURL->setText(_material->getURL());
    ui->editSourceReference->setText(_material->getReference());
    // ui->editTags->setText(_material->getName());
    ui->editDescription->setText(_material->getDescription());
}

void MaterialsEditor::updateMaterial()
{
    updateMaterialGeneral();
    updateMaterialProperties();
    updateMaterialAppearance();

    updatePreview();
}

void MaterialsEditor::onSelectMaterial(const QItemSelection& selected,
                                       const QItemSelection& deselected)
{
    Q_UNUSED(deselected);

    // Get the UUID before changing the underlying data model
    QString uuid;
    QStandardItemModel* model = static_cast<QStandardItemModel*>(ui->treeMaterials->model());
    QModelIndexList indexes = selected.indexes();
    for (auto it = indexes.begin(); it != indexes.end(); it++) {
        QStandardItem* item = model->itemFromIndex(*it);

        Base::Console().Log("%s\n", item->text().toStdString().c_str());
        if (item) {
            uuid = item->data(Qt::UserRole).toString();
            break;
        }
    }

    if (uuid.isEmpty() || uuid == _material->getUUID()) {
        Base::Console().Log("*** Unchanged material '%s'\n", uuid.toStdString().c_str());
        return;
    }

    // Ensure data is saved (or discarded) before changing materials
    if (_material->getEditState() != Materials::Material::ModelEdit_None) {
        // Prompt the user to save or discard changes
        Base::Console().Log("*** Material edited!!!\n");
        int res = confirmSave(this);
        if (res == QMessageBox::Cancel) {
            return;
        }
    }

    // Get the selected material
    try {
        _material = std::make_shared<Materials::Material>(*getMaterialManager().getMaterial(uuid));
    }
    catch (Materials::ModelNotFound const&) {
        Base::Console().Log("*** Unable to load material '%s'\n", uuid.toStdString().c_str());
        _material = std::make_shared<Materials::Material>();
    }

    updateMaterial();
    _material->resetEditState();
}

void MaterialsEditor::onDoubleClick(const QModelIndex& index)
{
    Q_UNUSED(index)

    Base::Console().Log("MaterialsEditor::onDoubleClick()\n");
    accept();
}

void MaterialsEditor::onContextMenu(const QPoint& pos)
{
    Base::Console().Log("MaterialsEditor::onContextMenu(%d,%d)\n", pos.x(), pos.y());
    // QModelIndex index = ui->treeMaterials->indexAt(pos);
    // QString path = pathFromIndex(index);
    // Base::Console().Log("\tindex at (%d,%d)->'%s'\n",
    //                     index.row(),
    //                     index.column(),
    //                     path.toStdString().c_str());


    QMenu contextMenu(tr("Context menu"), this);

    QAction action1(tr("Inherit from"), this);
    // action1.setShortcut(Qt::Key_Delete);
    connect(&action1, &QAction::triggered, this, &MaterialsEditor::onInherit);
    contextMenu.addAction(&action1);

    QAction action2(tr("Inherit new material"), this);
    // action1.setShortcut(Qt::Key_Delete);
    connect(&action2, &QAction::triggered, this, &MaterialsEditor::onInheritNew);
    contextMenu.addAction(&action2);

    contextMenu.exec(ui->treeMaterials->mapToGlobal(pos));
}

void MaterialsEditor::onInherit(bool checked)
{
    Q_UNUSED(checked)

    Base::Console().Log("MaterialsEditor::onInherit()\n");
    // QItemSelectionModel* selectionModel = ui->treeMaterials->selectionModel();
    // if (!selectionModel->hasSelection()) {
    //     Base::Console().Log("\tNothing selected\n");
    //     return;
    // }

    // Base::Console().Log("\tSelected path '%s'\n", _selectedFull.toStdString().c_str());
    // int res = confirmDelete(this);
    // if (res == QMessageBox::Cancel) {
    //     return;
    // }
}

void MaterialsEditor::onInheritNew(bool checked)
{
    Q_UNUSED(checked)

    Base::Console().Log("MaterialsEditor::onInherit()\n");
    // QItemSelectionModel* selectionModel = ui->treeMaterials->selectionModel();
    // if (!selectionModel->hasSelection()) {
    //     Base::Console().Log("\tNothing selected\n");
    //     return;
    // }

    // Base::Console().Log("\tSelected path '%s'\n", _selectedFull.toStdString().c_str());
    // int res = confirmDelete(this);
    // if (res == QMessageBox::Cancel) {
    //     return;
    // }
}

int MaterialsEditor::confirmSave(QWidget* parent)
{
    QMessageBox box(parent ? parent : this);
    box.setIcon(QMessageBox::Question);
    box.setWindowTitle(QObject::tr("Unsaved Material"));
    box.setText(QObject::tr("Do you want to save your changes to the material before closing?"));

    box.setInformativeText(QObject::tr("If you don't save, your changes will be lost."));
    box.setStandardButtons(QMessageBox::Discard | QMessageBox::Cancel | QMessageBox::Save);
    box.setDefaultButton(QMessageBox::Save);
    box.setEscapeButton(QMessageBox::Cancel);

    // add shortcuts
    QAbstractButton* saveBtn = box.button(QMessageBox::Save);
    if (saveBtn->shortcut().isEmpty()) {
        QString text = saveBtn->text();
        text.prepend(QLatin1Char('&'));
        saveBtn->setShortcut(QKeySequence::mnemonic(text));
    }

    QAbstractButton* discardBtn = box.button(QMessageBox::Discard);
    if (discardBtn->shortcut().isEmpty()) {
        QString text = discardBtn->text();
        text.prepend(QLatin1Char('&'));
        discardBtn->setShortcut(QKeySequence::mnemonic(text));
    }

    int res = QMessageBox::Cancel;
    box.adjustSize();  // Silence warnings from Qt on Windows
    switch (box.exec()) {
        case QMessageBox::Save:
            saveMaterial();
            res = QMessageBox::Save;
            break;
        case QMessageBox::Discard:
            res = QMessageBox::Discard;
            break;
    }

    return res;
}

#include "moc_MaterialsEditor.cpp"
