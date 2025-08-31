/***************************************************************************
 *   Copyright (c) 2024 David Carter <dcarter@david.carter.ca>             *
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
#include <QClipboard>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QTreeView>
#endif

#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Document.h>
#include <Gui/Selection/Selection.h>
#include <Gui/ViewProvider.h>
#include <Gui/ViewProviderDocumentObject.h>

#include <Mod/Material/App/MaterialLibrary.h>
#include <Mod/Material/App/PropertyMaterial.h>

#include "DlgInspectMaterial.h"
#include "ui_DlgInspectMaterial.h"


using namespace MatGui;

/* TRANSLATOR MatGui::DlgInspectMaterial */

DlgInspectMaterial::DlgInspectMaterial(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui_DlgInspectMaterial)
{
    ui->setupUi(this);

    auto tree = ui->treeMaterials;
    auto model = new QStandardItemModel();
    tree->setModel(model);

    tree->setHeaderHidden(true);


    std::vector<Gui::ViewProvider*> views = getSelection();
    update(views);

    connect(ui->buttonClipboard, &QPushButton::clicked, this, &DlgInspectMaterial::onClipboard);

    Gui::Selection().Attach(this);
}

DlgInspectMaterial::~DlgInspectMaterial()
{
    Gui::Selection().Detach(this);
}

bool DlgInspectMaterial::accept()
{
    return true;
}

void DlgInspectMaterial::onClipboard(bool checked)
{
    Q_UNUSED(checked)

    QApplication::clipboard()->setText(clipboardText);
}

std::vector<Gui::ViewProvider*> DlgInspectMaterial::getSelection() const
{
    std::vector<Gui::ViewProvider*> views;

    // get a single selection
    std::vector<Gui::SelectionSingleton::SelObj> sel =
        Gui::Selection().getSelection(nullptr, Gui::ResolveMode::OldStyleElement, true);
    for (const auto& it : sel) {
        Gui::ViewProvider* view =
            Gui::Application::Instance->getDocument(it.pDoc)->getViewProvider(it.pObject);
        views.push_back(view);
    }

    return views;
}

/// @cond DOXERR
void DlgInspectMaterial::OnChange(Gui::SelectionSingleton::SubjectType& rCaller,
                                  Gui::SelectionSingleton::MessageType Reason)
{
    Q_UNUSED(rCaller);

    if (Reason.Type == Gui::SelectionChanges::AddSelection
        || Reason.Type == Gui::SelectionChanges::RmvSelection
        || Reason.Type == Gui::SelectionChanges::SetSelection
        || Reason.Type == Gui::SelectionChanges::ClrSelection) {
        std::vector<Gui::ViewProvider*> views = getSelection();
        update(views);
    }
}
/// @endcond

void DlgInspectMaterial::appendClip(QString text)
{
    // Need to add indent
    QString indent(clipboardIndent * 4, QLatin1Char(' '));
    clipboardText += indent + text + QStringLiteral("\n");
}

QStandardItem* DlgInspectMaterial::clipItem(QString text)
{
    appendClip(text);
    auto item = new QStandardItem(text);
    return item;
}

void DlgInspectMaterial::indent()
{
    clipboardIndent += 1;
}

void DlgInspectMaterial::unindent()
{
    if (clipboardIndent > 0) {
        clipboardIndent -= 1;
    }
}

void DlgInspectMaterial::update(std::vector<Gui::ViewProvider*>& views)
{
    clipboardText = QStringLiteral("");
    clipboardIndent = 0;
    App::Document* doc = App::GetApplication().getActiveDocument();
    if (doc) {
        appendClip(tr("Document: ") + QString::fromUtf8(doc->Label.getValue()));
        ui->editDocument->setText(QString::fromUtf8(doc->Label.getValue()));

        if (views.size() == 1) {
            auto view = dynamic_cast<Gui::ViewProviderDocumentObject*>(views[0]);
            if (!view) {
                return;
            }
            auto* obj = view->getObject();
            if (!obj) {
                return;
            }
            auto* labelProp = dynamic_cast<App::PropertyString*>(obj->getPropertyByName("Label"));
            if (labelProp) {
                ui->editObjectLabel->setText(QString::fromUtf8(labelProp->getValue()));
                appendClip(tr("Label: ") + QString::fromUtf8(labelProp->getValue()));
            }
            else {
                ui->editObjectLabel->setText(QStringLiteral(""));
            }
            ui->editObjectName->setText(QLatin1String(obj->getNameInDocument()));
            appendClip(tr("Internal name: ") + QString::fromUtf8(obj->getNameInDocument()));

            auto subElement = Gui::Selection().getSelectionEx();
            if (subElement.size() > 0) {
                auto& subObject = subElement[0];
                if (subObject.getSubNames().size() > 0) {
                    ui->editSubShape->setText(QString::fromStdString(subObject.getSubNames()[0]));
                }
                else {
                    ui->editSubShape->setText(QStringLiteral(""));
                }
            }
            else {
                ui->editSubShape->setText(QStringLiteral(""));
            }

            auto subShapeType = QString::fromUtf8(obj->getTypeId().getName());
            subShapeType.remove(subShapeType.indexOf(QStringLiteral("::")), subShapeType.size());
            appendClip(tr("Type: ") + subShapeType);
            ui->editSubShapeType->setText(subShapeType);
            appendClip(tr("TypeID: ") + QString::fromUtf8(obj->getTypeId().getName()));
            ui->editShapeType->setText(QString::fromUtf8(obj->getTypeId().getName()));

            if (labelProp && QString::fromUtf8(labelProp->getValue()).size() > 0) {
                auto* prop = dynamic_cast<Materials::PropertyMaterial*>(
                    obj->getPropertyByName("ShapeMaterial"));
                if (prop) {
                    updateMaterialTree(prop->getValue());
                }
            }
        }
    }
}

void DlgInspectMaterial::updateMaterialTree(const Materials::Material& material)
{
    Base::Console().log("Material '%s'\n", material.getName().toStdString().c_str());

    auto tree = ui->treeMaterials;
    auto model = qobject_cast<QStandardItemModel*>(tree->model());
    model->clear();

    addMaterial(tree, model, material);
}

void DlgInspectMaterial::addMaterial(QTreeView* tree,
                                     QStandardItemModel* parent,
                                     const Materials::Material& material)
{
    auto card = clipItem(tr("Name: ") + material.getName());
    addExpanded(tree, parent, card);

    indent();
    addMaterialDetails(tree, card, material);
    unindent();
}

void DlgInspectMaterial::addMaterial(QTreeView* tree,
                                     QStandardItem* parent,
                                     const Materials::Material& material)
{
    auto card = clipItem(tr("Name: ") + material.getName());
    addExpanded(tree, parent, card);

    indent();
    addMaterialDetails(tree, card, material);
    unindent();
}

void DlgInspectMaterial::addModels(QTreeView* tree,
                                   QStandardItem* parent,
                                   const QSet<QString>* models)
{
    if (models->isEmpty()) {
        auto none = clipItem(tr("None"));
        addExpanded(tree, parent, none);
    }
    else {
        for (const QString& uuid : *models) {
            auto model = Materials::ModelManager::getManager().getModel(uuid);
            auto name = clipItem(tr("Name: ") + model->getName());
            addExpanded(tree, parent, name);

            indent();
            addModelDetails(tree, name, model);
            unindent();
        }
    }
}

void DlgInspectMaterial::addModelDetails(QTreeView* tree,
                                         QStandardItem* parent,
                                         std::shared_ptr<Materials::Model>& model)
{
    auto uuid = clipItem(tr("UUID: ") + model->getUUID());
    addExpanded(tree, parent, uuid);

    auto library = clipItem(tr("Library: ") + model->getLibrary()->getName());
    addExpanded(tree, parent, library);

    auto libraryPath =
        clipItem(tr("Library directory: ") + model->getLibrary()->getDirectoryPath());
    addExpanded(tree, parent, libraryPath);

    auto directory = clipItem(tr("Subdirectory: ") + model->getDirectory());
    addExpanded(tree, parent, directory);

    auto inherits = clipItem(tr("Inherits:"));
    addExpanded(tree, parent, inherits);

    auto& inheritedUuids = model->getInheritance();
    indent();
    if (inheritedUuids.isEmpty()) {
        auto none = clipItem(tr("None"));
        addExpanded(tree, inherits, none);
    }
    else {
        for (const QString& inherited : inheritedUuids) {
            auto inheritedModel = Materials::ModelManager::getManager().getModel(inherited);

            auto name = clipItem(tr("Name: ") + inheritedModel->getName());
            addExpanded(tree, inherits, name);

            indent();
            addModelDetails(tree, name, inheritedModel);
            unindent();
        }
    }
    unindent();
}

void DlgInspectMaterial::addProperties(
    QTreeView* tree,
    QStandardItem* parent,
    const std::map<QString, std::shared_ptr<Materials::MaterialProperty>>& properties)
{
    if (properties.empty()) {
        auto none = clipItem(tr("None"));
        addExpanded(tree, parent, none);
    }
    else {
        for (auto& property : properties) {
            auto name = clipItem(tr("Name: ") + property.second->getName());
            addExpanded(tree, parent, name);

            indent();
            addPropertyDetails(tree, name, property.second);
            unindent();
        }
    }
}

void DlgInspectMaterial::addPropertyDetails(
    QTreeView* tree,
    QStandardItem* parent,
    const std::shared_ptr<Materials::MaterialProperty>& property)
{
    auto uuid = clipItem(tr("Model UUID: ") + property->getModelUUID());
    addExpanded(tree, parent, uuid);
    auto type = clipItem(tr("Type: ") + property->getPropertyType());
    addExpanded(tree, parent, type);
    auto hasValue = clipItem(tr("Has value: ") + (property->isNull() ? tr("No") : tr("Yes")));
    addExpanded(tree, parent, hasValue);
}

void DlgInspectMaterial::addMaterialDetails(QTreeView* tree,
                                            QStandardItem* parent,
                                            const Materials::Material& material)
{
    auto uuid = clipItem(tr("UUID: ") + material.getUUID());
    addExpanded(tree, parent, uuid);
    auto library =
        clipItem(tr("Library: ") + material.getLibrary()->getName());
    addExpanded(tree, parent, library);
    auto libraryPath = clipItem(tr("Library directory: ") + material.getLibrary()->getDirectoryPath());
    addExpanded(tree, parent, libraryPath);
    auto directory = clipItem(tr("Sub directory: ") + material.getDirectory());
    addExpanded(tree, parent, directory);
    auto inherits = clipItem(tr("Inherits:"));
    addExpanded(tree, parent, inherits);

    indent();
    auto parentUUID = material.getParentUUID();
    if (!parentUUID.isEmpty()) {
        auto parentMaterial = Materials::MaterialManager::getManager().getMaterial(material.getParentUUID());
        addMaterial(tree, inherits, *parentMaterial);
    }
    else {
        auto none = clipItem(tr("None"));
        addExpanded(tree, inherits, none);
    }
    unindent();

    auto appearance = clipItem(tr("Appearance models:"));
    addExpanded(tree, parent, appearance);
    indent();
    addModels(tree, appearance, material.getAppearanceModels());
    unindent();

    auto physical = clipItem(tr("Physical models:"));
    addExpanded(tree, parent, physical);
    indent();
    addModels(tree, physical, material.getPhysicalModels());
    unindent();

    auto appearanceProperties = clipItem(tr("Appearance properties:"));
    addExpanded(tree, parent, appearanceProperties);
    indent();
    addProperties(tree, appearanceProperties, material.getAppearanceProperties());
    unindent();

    auto physicalProperties = clipItem(tr("Physical properties:"));
    addExpanded(tree, parent, physicalProperties);
    indent();
    addProperties(tree, physicalProperties, material.getPhysicalProperties());
    unindent();
}

void DlgInspectMaterial::addExpanded(QTreeView* tree,
                                     QStandardItemModel* parent,
                                     QStandardItem* child)
{
    parent->appendRow(child);
    tree->setExpanded(child->index(), true);
}

void DlgInspectMaterial::addExpanded(QTreeView* tree, QStandardItem* parent, QStandardItem* child)
{
    parent->appendRow(child);
    tree->setExpanded(child->index(), true);
}

/* TRANSLATOR MatGui::TaskInspectMaterial */

TaskInspectMaterial::TaskInspectMaterial()
{
    widget = new DlgInspectMaterial();
    addTaskBox(Gui::BitmapFactory().pixmap("Part_Loft"), widget);
}

TaskInspectMaterial::~TaskInspectMaterial() = default;

void TaskInspectMaterial::open()
{}

void TaskInspectMaterial::clicked(int)
{}

bool TaskInspectMaterial::accept()
{
    return widget->accept();
}

#include "moc_DlgInspectMaterial.cpp"
