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

#include <QItemSelectionModel>

#include <Mod/Material/App/Exceptions.h>
#include <Mod/Material/App/ModelManager.h>
#include "MaterialsEditor.h"
#include "ui_MaterialsEditor.h"
#include "ModelSelect.h"
#include "MaterialSave.h"


using namespace MatGui;

/* TRANSLATOR MatGui::MaterialsEditor */

MaterialsEditor::MaterialsEditor(QWidget* parent)
  : QDialog(parent), ui(new Ui_MaterialsEditor), _edited(false)
{
    ui->setupUi(this);

    createMaterialTree();
    createPhysicalTree();
    createAppearanceTree();
    createPreviews();

    ui->buttonURL->setIcon(QIcon(QString::fromStdString(":/icons/internet-web-browser.svg")));

    connect(ui->standardButtons, &QDialogButtonBox::accepted,
            this, &MaterialsEditor::accept);
    connect(ui->standardButtons, &QDialogButtonBox::rejected,
            this, &MaterialsEditor::reject);
    connect(ui->standardButtons->button(QDialogButtonBox::Save), &QPushButton::clicked,
            this, &MaterialsEditor::onSave);

    connect(ui->buttonURL, &QPushButton::clicked,
            this, &MaterialsEditor::onURL);
    connect(ui->buttonPhysicalAdd, &QPushButton::clicked,
            this, &MaterialsEditor::onPhysicalAdd);
    connect(ui->buttonAppearanceAdd, &QPushButton::clicked,
            this, &MaterialsEditor::onAppearanceAdd);

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

void MaterialsEditor::tryPython()
{
    Base::Console().Log("MaterialsEditor::tryPython()\n");

    // from materialtools.cardutils import get_material_template
    // template_data = get_material_template(True)
    Gui::WaitCursor wc;
    App::GetApplication().setActiveTransaction(QT_TRANSLATE_NOOP("Command", "Python test"));

    Base::PyGILStateLocker lock;
    try {
        PyObject* module = PyImport_ImportModule("materialtools.cardutils");
        if (!module) {
            throw Py::Exception();
        }
        Py::Module utils(module, true);

        // Py::Tuple args(2);
        // args.setItem(0, Py::asObject(it->getPyObject()));
        // args.setItem(1, Py::Float(distance));
        auto ret = utils.callMemberFunction("get_material_template");
        // if (ret != nullptr)
        //     Base::Console().Log("MaterialsEditor::tryPython() - null return\n");
        // else
        //     Base::Console().Log("MaterialsEditor::tryPython() - data!\n");

        Base::Console().Log("MaterialsEditor::tryPython() - call complete\n");

    }
    catch (Py::Exception&) {
        Base::PyException e;
        e.ReportException();
    }

    App::GetApplication().closeActiveTransaction();
    Base::Console().Log("MaterialsEditor::tryPython() - finished\n");
}

void MaterialsEditor::propertyChange(const QString &property, const QString value)
{
    Base::Console().Log("MaterialsEditor::propertyChange()\n");
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
        std::string selected = dialog.selectedModel();
        Base::Console().Log("Selected model '%s'\n", selected.c_str());
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
        std::string selected = dialog.selectedModel();
        Base::Console().Log("Selected model '%s'\n", selected.c_str());
        _material.addAppearance(selected);
        updateMaterial();
    } else {
        Base::Console().Log("No model selected\n");
    }
}

void MaterialsEditor::onSave(bool checked)
{
    Q_UNUSED(checked)
    
    MaterialSave dialog(this);
    dialog.setModal(true);
    if(dialog.exec() == QDialog::Accepted)
    {
    //     std::string selected = dialog.selectedModel();
    //     Base::Console().Log("Selected model '%s'\n", selected.c_str());
    //     _material.addAppearance(selected);
    //     updateMaterial();
    // } else {
    //     Base::Console().Log("No model selected\n");
    }
}

void MaterialsEditor::accept()
{
    QDialog::accept();
}

void MaterialsEditor::reject()
{
    QDialog::reject();
    // auto dw = qobject_cast<QDockWidget*>(parent());
    // if (dw) {
    //     dw->deleteLater();
    // }
}

// QIcon MaterialsEditor::errorIcon(const QIcon &icon) const {
//     auto pixmap = icon.pixmap();
// }

void MaterialsEditor::addMaterials(QStandardItem &parent, const std::string &top, const std::string &folder, const QIcon &icon)
{
    auto tree = ui->treeMaterials;
    for (const auto& mod : fs::directory_iterator(folder)) {
        if (fs::is_directory(mod)) {
            auto node = new QStandardItem(QString::fromStdString(mod.path().filename().string()));
            addExpanded(tree, &parent, node);
            node->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);

            addMaterials(*node, top, mod.path().string(), icon);
        }
        else if (isMaterial(mod)) {
            auto card = new QStandardItem(icon, QString::fromStdString(mod.path().filename().string()));
            card->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled
                        | Qt::ItemIsDropEnabled);
            try {
                auto material = getMaterialManager().getMaterialByPath(mod.path().string());
                card->setData(QVariant(QString::fromStdString(material.getUUID())), Qt::UserRole);
            } catch (Materials::ModelNotFound &e) {
                Base::Console().Log("Model not found error\n");
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
    tree->setItemDelegateForColumn(1, new MaterialDelegate(this));
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
    tree->setUniformRowHeights(true);
    MaterialDelegate* delegate = new MaterialDelegate(this);
    tree->setItemDelegateForColumn(1, delegate);

    connect(delegate, &MaterialDelegate::propertyChange,
            this, &MaterialsEditor::propertyChange);
}

void MaterialsEditor::createMaterialTree()
{
    Materials::ModelManager &modelManager = getModelManager();
    Q_UNUSED(modelManager)
    // Materials::MaterialManager materialManager;

    auto param =
        App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Material");
    if (!param)
        Base::Console().Log("Unable to find material parameter group\n");
    else
        Base::Console().Log("Found material parameter group\n");

    auto tree = ui->treeMaterials;
    auto model = new QStandardItemModel();
    tree->setModel(model);

    tree->setHeaderHidden(true);

    auto lib = new QStandardItem(QString::fromStdString("Favorites"));
    lib->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
    addExpanded(tree, model, lib);

    lib = new QStandardItem(QString::fromStdString("Recent"));
    lib->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
    addExpanded(tree, model, lib);

    auto libraries = Materials::MaterialManager::getMaterialLibraries();
    for (const auto &value : *libraries)
    {
        lib = new QStandardItem(QString::fromStdString(value->getName()));
        lib->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
        addExpanded(tree, model, lib);

        auto path = value->getDirectoryPath();
        addMaterials(*lib, path, path, QIcon(QString::fromStdString(value->getIconPath())));
    }
}

void MaterialsEditor::updatePreview() const
{
    std::string diffuseColor;
    std::string highlightColor;
    std::string sectionColor;

    if (_material.hasAppearanceProperty(static_cast<std::string>("DiffuseColor")))
        diffuseColor = _material.getAppearanceValue("DiffuseColor");
    else if (_material.hasAppearanceProperty(static_cast<std::string>("ViewColor")))
        diffuseColor = _material.getAppearanceValue("ViewColor");
    else if (_material.hasAppearanceProperty(static_cast<std::string>("Color")))
        diffuseColor = _material.getAppearanceValue("Color");

    if (_material.hasAppearanceProperty(static_cast<std::string>("SpecularColor")))
        highlightColor = _material.getAppearanceValue("SpecularColor");

    if (_material.hasAppearanceProperty(static_cast<std::string>("SectionColor")))
        sectionColor = _material.getAppearanceValue("SectionColor");

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

QString MaterialsEditor::getColorHash(const std::string &colorString, int colorRange) const
{
    /*
        returns a '#000000' string from a '(0.1,0.2,0.3)' string. Optionally the string
        has a fourth value for alpha (transparency)
    */
    std::stringstream stream(colorString);

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

    const std::vector<std::string> *models = _material.getAppearanceModels();
    if (models) {
        for (auto it = models->begin(); it != models->end(); it++)
        {
            std::string uuid = *it;
            try {
                const Materials::Model &model = getModelManager().getModel(uuid);
                std::string name = model.getName();

                auto modelRoot = new QStandardItem(QString::fromStdString(name));
                modelRoot->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
                addExpanded(tree, treeModel, modelRoot);
                for (auto itp = model.begin(); itp != model.end(); itp++)
                {
                    QList<QStandardItem*> items;

                    std::string key = itp->first;
                    auto propertyItem = new QStandardItem(QString::fromStdString(key));
                    propertyItem->setToolTip(QString::fromStdString(itp->second.getDescription()));
                    items.append(propertyItem);

                    auto valueItem = new QStandardItem(QString::fromStdString(_material.getAppearanceValue(key)));
                    valueItem->setToolTip(QString::fromStdString(itp->second.getDescription()));
                    items.append(valueItem);

                    auto typeItem = new QStandardItem(QString::fromStdString(itp->second.getPropertyType()));
                    items.append(typeItem);

                    auto unitsItem = new QStandardItem(QString::fromStdString(itp->second.getUnits()));
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

    const std::vector<std::string> *models = _material.getPhysicalModels();
    if (models) {
        for (auto it = models->begin(); it != models->end(); it++)
        {
            std::string uuid = *it;
            try {
                const Materials::Model &model = getModelManager().getModel(uuid);
                std::string name = model.getName();

                auto modelRoot = new QStandardItem(QString::fromStdString(name));
                modelRoot->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
                addExpanded(tree, treeModel, modelRoot);
                for (auto itp = model.begin(); itp != model.end(); itp++)
                {
                    QList<QStandardItem*> items;

                    std::string key = itp->first;
                    Materials::ModelProperty modelProperty = itp->second;
                    auto propertyItem = new QStandardItem(QString::fromStdString(key));
                    propertyItem->setToolTip(QString::fromStdString(modelProperty.getDescription()));
                    items.append(propertyItem);

                    auto valueItem = new QStandardItem(QString::fromStdString(_material.getPhysicalValue(key)));
                    valueItem->setToolTip(QString::fromStdString(modelProperty.getDescription()));
                    items.append(valueItem);

                    auto typeItem = new QStandardItem(QString::fromStdString(modelProperty.getPropertyType()));
                    items.append(typeItem);

                    auto unitsItem = new QStandardItem(QString::fromStdString(modelProperty.getUnits()));
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
    ui->editName->setText(QString::fromStdString(_material.getName()));
    ui->editAuthorLicense->setText(QString::fromStdString(_material.getAuthorAndLicense()));
    // ui->editParent->setText(QString::fromStdString(_material.getName()));
    ui->editSourceURL->setText(QString::fromStdString(_material.getURL()));
    ui->editSourceReference->setText(QString::fromStdString(_material.getReference()));
    // ui->editTags->setText(QString::fromStdString(_material.getName()));
    ui->editDescription->setText(QString::fromStdString(_material.getDescription()));

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
            std::string uuid = item->data(Qt::UserRole).toString().toStdString();
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

MaterialDelegate::MaterialDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

void MaterialDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, 
        const QModelIndex &index) const
{
    // Base::Console().Log("MaterialsEditor::paint()\n");
    const QStandardItemModel *treeModel = static_cast<const QStandardItemModel *>(index.model());

    // Check we're not the material model root. This is also used to access the entry columns
    auto item = treeModel->itemFromIndex(index);
    auto group = item->parent();
    if (!group)
    {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    int row = index.row();

    QString propertyName = group->child(row, 0)->text();
    QString propertyType = QString::fromStdString("String");
    if (group->child(row, 2))
        propertyType = group->child(row, 2)->text();
    QString propertyValue = QString::fromStdString("");
    if (group->child(row, 1))
        propertyValue = group->child(row, 1)->text();

    if (propertyType.toStdString() != "Color" || index.column() != 1)
    {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    painter->save();

    QColor color;
    color.setRgba(parseColor(propertyValue));
    painter->fillRect(option.rect.left() + 5, option.rect.top() + 5, option.rect.width() - 10, option.rect.height() - 10, QBrush(color));

    painter->restore();
}

QSize MaterialDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    const QStandardItemModel *treeModel = static_cast<const QStandardItemModel *>(index.model());

    // Check we're not the material model root. This is also used to access the entry columns
    auto item = treeModel->itemFromIndex(index);
    auto group = item->parent();
    if (!group)
    {
        return QStyledItemDelegate::sizeHint(option, index);
    }

    int row = index.row();

    QString propertyType = QString::fromStdString("String");
    if (group->child(row, 2))
        propertyType = group->child(row, 2)->text();

    if (propertyType.toStdString() != "Color" || index.column() != 1)
    {
        return QStyledItemDelegate::sizeHint(option, index);
    }

    return QSize(75, 50); // Standard QPushButton size
}

void MaterialDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    Base::Console().Log("MaterialsEditor::setEditorData()\n");
    QVariant propertyType = editor->property("Type");
    const QStandardItemModel *model = static_cast<const QStandardItemModel *>(index.model());
    QStandardItem *item = model->itemFromIndex(index);
    auto group = item->parent();
    if (!group)
        return;

    int row = index.row();
    QString propertyName = group->child(row, 0)->text();

    std::string type = propertyType.toString().toStdString();
    if (type == "Color")
    {
        QColor color = editor->property("color").value<QColor>();
        QString colorText = QString(QString::fromStdString("(%1,%2,%3,%4)"))
                                .arg(color.red()/255.0)
                                .arg(color.green()/255.0)
                                .arg(color.blue()/255.0)
                                .arg(color.alpha()/255.0);
        item->setText(colorText);
    } else if (type == "File")
    {
        Gui::FileChooser* chooser = static_cast<Gui::FileChooser*>(editor);
        item->setText(chooser->fileName());
    } else if (type == "Quantity")
    {
        Gui::InputField* input = static_cast<Gui::InputField*>(editor);
        item->setText(input->getQuantityString());
    } else
    {
        QStyledItemDelegate::setEditorData(editor, index);
    }

    Q_EMIT const_cast<MaterialDelegate *>(this)->propertyChange(propertyName, item->text());
}

void MaterialDelegate::setModelData(QWidget* editor, QAbstractItemModel* model,
                      const QModelIndex& index) const
{
    QStyledItemDelegate::setModelData(editor, model, index);
}

QWidget* MaterialDelegate::createEditor(
    QWidget* parent, const QStyleOptionViewItem&, const QModelIndex& index) const
{
    Base::Console().Log("MaterialsEditor::createEditor()\n");
    if (index.column() != 1)
        return nullptr;

    const QStandardItemModel *treeModel = static_cast<const QStandardItemModel *>(index.model());

    // Check we're not the material model root. This is also used to access the entry columns
    auto item = treeModel->itemFromIndex(index);
    auto group = item->parent();
    if (!group)
        return nullptr;

    int row = index.row();

    QString propertyName = group->child(row, 0)->text();
    QString propertyType = QString::fromStdString("String");
    if (group->child(row, 2))
        propertyType = group->child(row, 2)->text();

    QString propertyValue = QString::fromStdString("");
    if (group->child(row, 1))
        propertyValue = group->child(row, 1)->text();

    QString propertyUnits = QString::fromStdString("");
    if (group->child(row, 1))
        propertyUnits = group->child(row, 3)->text();

    QWidget* editor = createWidget(parent, propertyName, propertyType, propertyValue, propertyUnits);

    return editor;
}

QWidget* MaterialDelegate::createWidget(QWidget* parent, const QString &propertyName, const QString &propertyType,
    const QString &propertyValue, const QString &propertyUnits) const
{
    Q_UNUSED(propertyName);

    QWidget* widget = nullptr;

    std::string type = propertyType.toStdString();
    if (type == "String" || type == "URL" || type == "Vector")
    {
        widget = new Gui::PrefLineEdit(parent);

    } else if ((type == "Integer") || (type == "Int"))
    {
        Gui::UIntSpinBox *spinner = new Gui::UIntSpinBox(parent);
        spinner->setMinimum(0);
        spinner->setMaximum(UINT_MAX);
        spinner->setValue(propertyValue.toUInt());
        widget = spinner;
    } else if (type == "Float")
    {
        Gui::DoubleSpinBox *spinner = new Gui::DoubleSpinBox(parent);
        
        // the magnetic permeability is the parameter for which many decimals matter
        // the most however, even for this, 6 digits are sufficient
        spinner->setDecimals(6);

        // for almost all Float parameters of materials a step of 1 would be too large
        spinner->setSingleStep(0.1);

        spinner->setMinimum(std::numeric_limits<double>::min());
        spinner->setMaximum(std::numeric_limits<double>::max());
        spinner->setValue(propertyValue.toDouble());
        widget = spinner;
    } else if (type == "Boolean")
    {
        Gui::PrefComboBox *combo = new Gui::PrefComboBox(parent);
        combo->insertItem(0, QString::fromStdString(""));
        combo->insertItem(1, QString::fromStdString("False"));
        combo->insertItem(2, QString::fromStdString("True"));
        combo->setCurrentText(propertyValue);
        widget = combo;
    } else if (type == "Color")
    {
        Gui::PrefColorButton *button = new Gui::PrefColorButton();
        QColor color;
        color.setRgba(parseColor(propertyValue));
        button->setProperty("color", color);

        widget = button;
    } else if (type == "Quantity")
    {
        Gui::InputField *input = new Gui::InputField();
        input->setMinimum(std::numeric_limits<double>::min());
        input->setMaximum(std::numeric_limits<double>::max());
        input->setUnitText(propertyUnits); // TODO: Ensure this exists
        input->setPrecision(6);
        input->setQuantityString(propertyValue);

        widget = input;
    } else if (type == "File")
    {
        Gui::FileChooser *chooser = new Gui::FileChooser();
        if (propertyValue.length() > 0)
        {
            chooser->setFileName(propertyValue);
        }

        widget = chooser;
    } else
    {
        // Default editor
        widget = new QLineEdit(parent);
    }

    widget->setProperty("Type", propertyType);
    widget->setParent(parent);

    return widget;
}

QRgb MaterialDelegate::parseColor(const QString &color) const
{
    QString trimmed = color;
    trimmed.replace(QRegularExpression(QString::fromStdString("\\(([^<]*)\\)")),
            QString::fromStdString("\\1"));
    QStringList parts = trimmed.split(QString::fromStdString(","));
    int red = parts.at(0).toDouble() * 255;
    int green = parts.at(1).toDouble() * 255;
    int blue = parts.at(2).toDouble() * 255;
    int alpha = parts.at(3).toDouble() * 255;

    return qRgba(red, green, blue, alpha);
}

#include "moc_MaterialsEditor.cpp"
