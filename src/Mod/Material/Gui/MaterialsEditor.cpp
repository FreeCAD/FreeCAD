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
# include <QColorDialog>
#endif

#include <limits>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QIODevice>
#include <QDesktopServices>
#include <QVariant>

#include <App/Application.h>
#include <Base/Interpreter.h>
#include <Base/Quantity.h>
#include <Gui/Command.h>
#include <Gui/InputField.h>
#include <Gui/WaitCursor.h>
#include <Gui/Application.h>
#include <Gui/PrefWidgets.h>
#include <Gui/SpinBox.h>
// #include <Gui/FileDialog.h>

#include <QItemSelectionModel>

#include <Mod/Material/App/Exceptions.h>
#include <Mod/Material/App/ModelManager.h>
#include "MaterialsEditor.h"
#include "ui_MaterialsEditor.h"
#include "ModelSelect.h"
#include "MaterialSave.h"
#include "MaterialDelegate.h"


using namespace MatGui;

/* TRANSLATOR MatGui::MaterialsEditor */

MaterialsEditor::MaterialsEditor(QWidget* parent)
  : QDialog(parent), ui(new Ui_MaterialsEditor), _edited(false)
{
    ui->setupUi(this);

    getFavorites();
    getRecents();

    createMaterialTree();
    createPhysicalTree();
    createAppearanceTree();
    createPreviews();

    ui->buttonURL->setIcon(QIcon(QString::fromStdString(":/icons/internet-web-browser.svg")));

    connect(ui->standardButtons->button(QDialogButtonBox::Ok), &QPushButton::clicked,
            this, &MaterialsEditor::onOk);
    connect(ui->standardButtons->button(QDialogButtonBox::Cancel), &QPushButton::clicked,
            this, &MaterialsEditor::onCancel);
    connect(ui->standardButtons->button(QDialogButtonBox::Save), &QPushButton::clicked,
            this, &MaterialsEditor::onSave);

    connect(ui->buttonURL, &QPushButton::clicked,
            this, &MaterialsEditor::onURL);
    connect(ui->buttonPhysicalAdd, &QPushButton::clicked,
            this, &MaterialsEditor::onPhysicalAdd);
    connect(ui->buttonAppearanceAdd, &QPushButton::clicked,
            this, &MaterialsEditor::onAppearanceAdd);
    connect(ui->buttonFavorite, &QPushButton::clicked,
            this, &MaterialsEditor::onFavourite);

    QItemSelectionModel* selectionModel = ui->treeMaterials->selectionModel();
    connect(selectionModel, &QItemSelectionModel::selectionChanged,
            this, &MaterialsEditor::onSelectMaterial);
}

/*
 *  Destroys the object and frees any allocated resources
 */
MaterialsEditor::~MaterialsEditor()
{
    // no need to delete child widgets, Qt does it all for us
}

void MaterialsEditor::getFavorites()
{
    _favorites.clear();

    auto param =
        App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Material/Favorites");
    int count = param->GetInt("Favorites", 0);
    for (int i = 0; i < count; i++)
    {
        QString key = QString::fromLatin1("FAV%1").arg(i);
        QString uuid = QString::fromStdString(param->GetASCII(key.toStdString().c_str(), ""));
        _favorites.push_back(uuid);
    }
}

void MaterialsEditor::saveFavorites()
{
    auto param =
        App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Material/Favorites");

    // Clear out the existing favorites
    int count = param->GetInt("Favorites", 0);
    for (int i = 0; i < count; i++)
    {
        QString key = QString::fromLatin1("FAV%1").arg(i);
        param->RemoveASCII(key.toStdString().c_str());
    }

    // Add the current values
    param->SetInt("Favorites", _favorites.size());
    int j = 0;
    for (auto favorite: _favorites)
    {
        QString key = QString::fromLatin1("FAV%1").arg(j);
        param->SetASCII(key.toStdString().c_str(), favorite.toStdString());

        j++;
    }
}

void MaterialsEditor::addFavorite(const QString &uuid)
{
    // Ensure it is a material. New, unsaved materials will not be
    try
    {
        auto material = _materialManager.getMaterial(uuid);
    }
    catch(const Materials::MaterialNotFound&)
    {
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
    for (auto it: _favorites)
    {
        if (it == uuid)
            return true;
    }
    return false;
}


void MaterialsEditor::getRecents()
{
    _recents.clear();

    auto param =
        App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Material/Recent");
    _recentMax = param->GetInt("RecentMax", 5);
    int count = param->GetInt("Recent", 0);
    for (int i = 0; i < count; i++)
    {
        QString key = QString::fromLatin1("MRU%1").arg(i);
        QString uuid = QString::fromStdString(param->GetASCII(key.toStdString().c_str(), ""));
        _recents.push_back(uuid);
    }
}

void MaterialsEditor::saveRecents()
{
    auto param =
        App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Material/Recent");

    // Clear out the existing favorites
    int count = param->GetInt("Recent", 0);
    for (int i = 0; i < count; i++)
    {
        QString key = QString::fromLatin1("MRU%1").arg(i);
        param->RemoveASCII(key.toStdString().c_str());
    }

    // Add the current values
    int size = _recents.size();
    if (size > _recentMax)
        size = _recentMax;
    param->SetInt("Recent", size);
    int j = 0;
    for (auto recent: _recents)
    {
        QString key = QString::fromLatin1("MRU%1").arg(j);
        param->SetASCII(key.toStdString().c_str(), recent.toStdString());

        j++;
        if (j >= size)
            break;
    }
}

void MaterialsEditor::addRecent(const QString& uuid)
{
    // Ensure it is a material. New, unsaved materials will not be
    try
    {
        auto material = _materialManager.getMaterial(uuid);
    }
    catch(const Materials::MaterialNotFound&)
    {
        return;
    }
    
    // Ensure no duplicates
    if (isRecent(uuid))
        _recents.remove(uuid);

    _recents.push_front(uuid);
    while (_recents.size() > _recentMax)
        _recents.pop_back();

    saveRecents();
}

bool MaterialsEditor::isRecent(const QString& uuid) const
{
    for (auto it: _recents)
    {
        if (it == uuid)
            return true;
    }
    return false;
}

void MaterialsEditor::propertyChange(const QString &property, const QString value)
{
    Base::Console().Log("MaterialsEditor::propertyChange(%s) = '%s'\n", 
        property.toStdString().c_str(),
        value.toStdString().c_str());
    if (_material.hasPhysicalProperty(property))
    {
        _material.setPhysicalValue(property, value);
    } else if (_material.hasAppearanceProperty(property)) {
        _material.setAppearanceValue(property, value);
        updatePreview();
    }
    _edited = true;
}

void MaterialsEditor::onURL(bool checked)
{
    Q_UNUSED(checked)

    Base::Console().Log("URL\n");
    QString url = ui->editSourceURL->text();
    if (url.length() > 0)
        QDesktopServices::openUrl(QUrl(url, QUrl::TolerantMode));
}

void MaterialsEditor::onPhysicalAdd(bool checked)
{
    Q_UNUSED(checked)
    
    ModelSelect dialog(this, Materials::ModelManager::ModelFilter_Physical);
    dialog.setModal(true);
    if(dialog.exec() == QDialog::Accepted)
    {
        QString selected = dialog.selectedModel();
        Base::Console().Log("Selected model '%s'\n", selected.toStdString().c_str());
        _material.addPhysical(selected);
        updateMaterial();
    } else {
        Base::Console().Log("No model selected\n");
    }
}

void MaterialsEditor::onAppearanceAdd(bool checked)
{
    Q_UNUSED(checked)
    
    ModelSelect dialog(this, Materials::ModelManager::ModelFilter_Appearance);
    dialog.setModal(true);
    if(dialog.exec() == QDialog::Accepted)
    {
        QString selected = dialog.selectedModel();
        Base::Console().Log("Selected model '%s'\n", selected.toStdString().c_str());
        _material.addAppearance(selected);
        updateMaterial();
    } else {
        Base::Console().Log("No model selected\n");
    }
}

void MaterialsEditor::onFavourite(bool checked)
{
    Q_UNUSED(checked)

    Base::Console().Log("Favorite\n");
    auto selected = _material.getUUID();
    if (isFavorite(selected))
        removeFavorite(selected);
    else
        addFavorite(selected);
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

    MaterialSave dialog(&_material, this);
    dialog.setModal(true);
    if(dialog.exec() == QDialog::Accepted)
    {
    //     QString selected = dialog.selectedModel();
    //     Base::Console().Log("Selected model '%s'\n", selected.c_str());
    //     _material.addAppearance(selected);
    //     updateMaterial();
    // } else {
    //     Base::Console().Log("No model selected\n");
    }
}

void MaterialsEditor::accept()
{
    addRecent(_material.getUUID());
    QDialog::accept();
}

void MaterialsEditor::reject()
{
    QDialog::reject();
}

// QIcon MaterialsEditor::errorIcon(const QIcon &icon) const {
//     auto pixmap = icon.pixmap();
// }

void MaterialsEditor::addMaterials(QStandardItem &parent, const QString &top, const QString &folder, const QIcon &icon)
{
    auto tree = ui->treeMaterials;
    for (const auto& mod : fs::directory_iterator(folder.toStdString())) {
        if (fs::is_directory(mod)) {
            auto node = new QStandardItem(QString::fromStdString(mod.path().filename().string()));
            addExpanded(tree, &parent, node);
            node->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);

            addMaterials(*node, top, QString::fromStdString(mod.path().string()), icon);
        }
        else if (isMaterial(mod)) {
            auto card = new QStandardItem(icon, QString::fromStdString(mod.path().filename().string()));
            card->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled
                        | Qt::ItemIsDropEnabled);
            try {
                auto material = getMaterialManager().getMaterialByPath(QString::fromStdString(mod.path().string()));
                card->setData(QVariant(material.getUUID()), Qt::UserRole);
            } catch (Materials::ModelNotFound &e) {
                Base::Console().Log("Model not found error\n");
            } catch (Materials::MaterialNotFound &e) {
                // Base::Console().Log("Material not found error\n");
            } catch (std::exception &e) {
                Base::Console().Log("Exception '%s'\n", e.what());
            }
            addExpanded(tree, &parent, card);
        }
    }
}

void MaterialsEditor::addExpanded(QTreeView *tree, QStandardItem *parent, QStandardItem *child)
{
    parent->appendRow(child);
    tree->setExpanded(child->index(), true);
}

void MaterialsEditor::addExpanded(QTreeView *tree, QStandardItemModel *parent, QStandardItem *child)
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
    headers.append(QString::fromStdString("Property"));
    headers.append(QString::fromStdString("Value"));
    headers.append(QString::fromStdString("Type"));
    model->setHorizontalHeaderLabels(headers);

    tree->setColumnWidth(0, 250);
    tree->setColumnWidth(1, 250);
    tree->setColumnHidden(2, true);

    tree->setHeaderHidden(false);
    tree->setUniformRowHeights(true);
    MaterialDelegate* delegate = new MaterialDelegate(this);
    tree->setItemDelegateForColumn(1, delegate);

    connect(delegate, &MaterialDelegate::propertyChange,
            this, &MaterialsEditor::propertyChange);
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
    headers.append(QString::fromStdString("Property"));
    headers.append(QString::fromStdString("Value"));
    headers.append(QString::fromStdString("Type"));
    model->setHorizontalHeaderLabels(headers);

    tree->setColumnWidth(0, 250);
    tree->setColumnWidth(1, 250);
    tree->setColumnHidden(2, true);

    tree->setHeaderHidden(false);
    tree->setUniformRowHeights(false);
    MaterialDelegate* delegate = new MaterialDelegate(this);
    tree->setItemDelegateForColumn(1, delegate);

    connect(delegate, &MaterialDelegate::propertyChange,
            this, &MaterialsEditor::propertyChange);
}

void MaterialsEditor::addRecents(QStandardItem *parent)
{
    auto tree = ui->treeMaterials;
    for (auto& uuid : _recents) {
        try
        {
            const Materials::Material &material = getMaterialManager().getMaterial(uuid);

            QIcon icon = QIcon(material.getLibrary().getIconPath());
            auto card = new QStandardItem(icon, material.getName());
            card->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled
                        | Qt::ItemIsDropEnabled);
            card->setData(QVariant(uuid), Qt::UserRole);

            addExpanded(tree, parent, card);
        }
        catch(const Materials::MaterialNotFound& )
        {}
    }
}

void MaterialsEditor::addFavorites(QStandardItem *parent)
{
    auto tree = ui->treeMaterials;
    for (auto& uuid : _favorites) {
        try
        {
            const Materials::Material &material = getMaterialManager().getMaterial(uuid);

            QIcon icon = QIcon(material.getLibrary().getIconPath());
            auto card = new QStandardItem(icon, material.getName());
            card->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled
                        | Qt::ItemIsDropEnabled);
            card->setData(QVariant(uuid), Qt::UserRole);

            addExpanded(tree, parent, card);
        }
        catch(const Materials::MaterialNotFound& )
        {}
    }
}

void MaterialsEditor::fillMaterialTree()
{
    auto tree = ui->treeMaterials;
    auto model = static_cast<QStandardItemModel *>(tree->model());

    auto lib = new QStandardItem(QString::fromStdString("Favorites"));
    lib->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
    addExpanded(tree, model, lib);
    addFavorites(lib);

    lib = new QStandardItem(QString::fromStdString("Recent"));
    lib->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
    addExpanded(tree, model, lib);
    addRecents(lib);

    auto libraries = Materials::MaterialManager::getMaterialLibraries();
    for (const auto &value : *libraries)
    {
        lib = new QStandardItem(value->getName());
        lib->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
        addExpanded(tree, model, lib);

        auto path = value->getDirectoryPath();
        addMaterials(*lib, path, path, QIcon(value->getIconPath()));
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
    auto model = static_cast<QStandardItemModel *>(tree->model());
    model->clear();

    fillMaterialTree();
}

void MaterialsEditor::updatePreview() const
{
    QString diffuseColor;
    QString highlightColor;
    QString sectionColor;

    if (_material.hasAppearanceProperty(QString::fromStdString("DiffuseColor")))
        diffuseColor = _material.getAppearanceValueString(QString::fromStdString("DiffuseColor"));
    else if (_material.hasAppearanceProperty(QString::fromStdString("ViewColor")))
        diffuseColor = _material.getAppearanceValueString(QString::fromStdString("ViewColor"));
    else if (_material.hasAppearanceProperty(QString::fromStdString("Color")))
        diffuseColor = _material.getAppearanceValueString(QString::fromStdString("Color"));

    if (_material.hasAppearanceProperty(QString::fromStdString("SpecularColor")))
        highlightColor = _material.getAppearanceValueString(QString::fromStdString("SpecularColor"));

    if (_material.hasAppearanceProperty(QString::fromStdString("SectionColor")))
        sectionColor = _material.getAppearanceValueString(QString::fromStdString("SectionColor"));

    if ((diffuseColor.length() + highlightColor.length()) > 0)
    {
        auto file = QFile(QString::fromStdString(":/icons/preview-rendered.svg"));
        if (file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QString svg = QTextStream(&file).readAll();
            file.close();
            if (diffuseColor.length() > 0)
            {
                svg = svg.replace(QString::fromStdString("#d3d7cf"), getColorHash(diffuseColor, 255));
                svg = svg.replace(QString::fromStdString("#555753"), getColorHash(diffuseColor, 125));
            }
            if (highlightColor.length() > 0)
            {
                svg = svg.replace(QString::fromStdString("#fffffe"), getColorHash(highlightColor, 255));
            }
            _rendered->load(svg.toUtf8());
        }
    }

    if ((diffuseColor.length() + sectionColor.length()) > 0)
    {
        auto file = QFile(QString::fromStdString(":/icons/preview-vector.svg"));
        if (file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QString svg = QTextStream(&file).readAll();
            file.close();
            if (diffuseColor.length() > 0)
            {
                svg = svg.replace(QString::fromStdString("#d3d7cf"), getColorHash(diffuseColor, 255));
                svg = svg.replace(QString::fromStdString("#555753"), getColorHash(diffuseColor, 125));
            }
            if (sectionColor.length() > 0)
            {
                svg = svg.replace(QString::fromStdString("#ffffff"), getColorHash(sectionColor, 255));
            }
            _vectored->load(svg.toUtf8());
        }
    }
}

QString MaterialsEditor::getColorHash(const QString &colorString, int colorRange) const
{
    /*
        returns a '#000000' string from a '(0.1,0.2,0.3)' string. Optionally the string
        has a fourth value for alpha (transparency)
    */
    std::stringstream stream(colorString.toStdString());

    char c; stream >> c; // read "("
    double red; stream >> red;
    stream >> c; // ","
    double green; stream >> green;
    stream >> c; // ","
    double blue; stream >> blue;
    stream >> c; // ","
    double alpha = 1.0;
    if (c == ',')
        stream >> alpha;

    QColor color(static_cast<int>(red * colorRange),
                 static_cast<int>(green * colorRange),
                 static_cast<int>(blue * colorRange),
                 static_cast<int>(alpha * colorRange));
    return color.name();
}

void MaterialsEditor::updateMaterialAppearance()
{
    QTreeView *tree = ui->treeAppearance;
    QStandardItemModel *treeModel = static_cast<QStandardItemModel *>(tree->model());
    treeModel->clear();

    QStringList headers;
    headers.append(QString::fromStdString("Property"));
    headers.append(QString::fromStdString("Value"));
    headers.append(QString::fromStdString("Type"));
    treeModel->setHorizontalHeaderLabels(headers);

    tree->setColumnWidth(0, 250);
    tree->setColumnWidth(1, 250);
    tree->setColumnHidden(2, true);

    const std::vector<QString> *models = _material.getAppearanceModels();
    if (models) {
        for (auto it = models->begin(); it != models->end(); it++)
        {
            QString uuid = *it;
            try {
                const Materials::Model &model = getModelManager().getModel(uuid);
                QString name = model.getName();

                auto modelRoot = new QStandardItem(name);
                modelRoot->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
                addExpanded(tree, treeModel, modelRoot);
                for (auto itp = model.begin(); itp != model.end(); itp++)
                {
                    QList<QStandardItem*> items;

                    QString key = itp->first;
                    auto propertyItem = new QStandardItem(key);
                    propertyItem->setToolTip(itp->second.getDescription());
                    items.append(propertyItem);

                    auto valueItem = new QStandardItem(_material.getAppearanceValueString(key));
                    valueItem->setToolTip(itp->second.getDescription());
                    QVariant variant;
                    variant.setValue(&_material);
                    valueItem->setData(variant);
                    items.append(valueItem);

                    auto typeItem = new QStandardItem(itp->second.getPropertyType());
                    items.append(typeItem);

                    auto unitsItem = new QStandardItem(itp->second.getUnits());
                    items.append(unitsItem);

                    modelRoot->appendRow(items);
                    tree->setExpanded(modelRoot->index(), true);
                }
            } catch (Materials::ModelNotFound const &) {
            }
        }
    }
}

void MaterialsEditor::updateMaterialProperties()
{
    QTreeView *tree = ui->treePhysicalProperties;
    QStandardItemModel *treeModel = static_cast<QStandardItemModel *>(tree->model());
    treeModel->clear();

    QStringList headers;
    headers.append(QString::fromStdString("Property"));
    headers.append(QString::fromStdString("Value"));
    headers.append(QString::fromStdString("Type"));
    headers.append(QString::fromStdString("Units"));
    treeModel->setHorizontalHeaderLabels(headers);

    tree->setColumnWidth(0, 250);
    tree->setColumnWidth(1, 250);
    tree->setColumnHidden(2, true);
    tree->setColumnHidden(3, true);

    const std::vector<QString> *models = _material.getPhysicalModels();
    if (models) {
        for (auto it = models->begin(); it != models->end(); it++)
        {
            QString uuid = *it;
            try {
                const Materials::Model &model = getModelManager().getModel(uuid);
                QString name = model.getName();

                auto modelRoot = new QStandardItem(name);
                modelRoot->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
                addExpanded(tree, treeModel, modelRoot);
                for (auto itp = model.begin(); itp != model.end(); itp++)
                {
                    QList<QStandardItem*> items;

                    QString key = itp->first;
                    Materials::ModelProperty modelProperty = itp->second;
                    auto propertyItem = new QStandardItem(key);
                    propertyItem->setToolTip(modelProperty.getDescription());
                    items.append(propertyItem);

                    auto valueItem = new QStandardItem(_material.getPhysicalValueString(key));
                    valueItem->setToolTip(modelProperty.getDescription());
                    QVariant variant;
                    variant.setValue(&_material);
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
            } catch (Materials::ModelNotFound const &) {
            }
        }
    }
}

void MaterialsEditor::updateMaterial()
{
    // Update the general information
    ui->editName->setText(_material.getName());
    ui->editAuthorLicense->setText(_material.getAuthorAndLicense());
    // ui->editParent->setText(_material.getName());
    ui->editSourceURL->setText(_material.getURL());
    ui->editSourceReference->setText(_material.getReference());
    // ui->editTags->setText(_material.getName());
    ui->editDescription->setText(_material.getDescription());

    updateMaterialProperties();
    updateMaterialAppearance();
    
    updatePreview();
}

void MaterialsEditor::onSelectMaterial(const QItemSelection& selected, const QItemSelection& deselected)
{
    Q_UNUSED(deselected);

    if (_edited)
    {
        // Prompt the user to save or discard changes
    }

    QStandardItemModel *model = static_cast<QStandardItemModel *>(ui->treeMaterials->model());
    QModelIndexList indexes = selected.indexes();
    for (auto it = indexes.begin(); it != indexes.end(); it++)
    {
        QStandardItem* item = model->itemFromIndex(*it);
        Base::Console().Log("%s\n", item->text().toStdString().c_str());
        if (item) {
            QString uuid = item->data(Qt::UserRole).toString();
            try
            {
                _material = getMaterialManager().getMaterial(uuid);
            }
            catch(Materials::ModelNotFound const&)
            {
                Materials::Material empty;
                _material = empty;
            }

            updateMaterial();
            _edited = false;
        }
    }
}

#include "moc_MaterialsEditor.cpp"
