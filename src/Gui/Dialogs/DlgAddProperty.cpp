// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2024 Ondsel <development@ondsel.com>                     *
 *   Copyright (c) 2025 Pieter Hijma <info@pieterhijma.net>                 *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include <QMessageBox>
#include <QString>
#include <QCompleter>
#include <algorithm>
#include <memory>
#include <array>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/ExpressionParser.h>
#include <App/PropertyUnits.h>
#include <App/PropertyFile.h>
#include <App/PropertyGeo.h>
#include <Base/Tools.h>

#include "Dialogs/DlgAddProperty.h"
#include "Application.h"
#include "Macro.h"
#include "ui_DlgAddProperty.h"
#include "ViewProviderVarSet.h"
#include "propertyeditor/PropertyItem.h"

FC_LOG_LEVEL_INIT("DlgAddProperty", true, true)

using namespace Gui;
using namespace Gui::Dialog;
using namespace Gui::PropertyEditor;

const std::string DlgAddProperty::GroupBase = "Base";

/*
 * This dialog has quite complex logic, so we will document it here.
 *
 * The design goals of this dialog are:
 * - support transactions (undo/redo),
 * - provide a value field as soon as possible (see #16189),
 *   - keep the value if the name of the property is changed,
 * - support units (see #15557),
 * - support enumerations (see #15553),
 * - make OK available as soon as there is a valid property (see #17474),
 * - useful Python console commands (see #23760),
 * - support expressions (see #19716).
 *
 * Especially supporting expressions in the value field makes the logic
 * complex.  Editors for value fields are created from PropertyItems.  An
 * editor has two modes: One without the possibility to add an expression and
 * one with the possibility to add an expression.  This depends on whether the
 * PropertyItem is bound.  A PropertyItem can only be bound if a valid property
 * exists, which means the name of the property and the type should be known.
 *
 * As one of the goals of this dialog is to show an editor as soon as possible,
 * so also when there is no property name known yet, this means that the editor
 * won't have the expression button at first.
 *
 * To show the expression button as soon as possible, we create the property as
 * soon as a valid type and name are known.  This allows us to bind the
 * PropertyItem which results in having the expression button.
 *
 * However, since we also want to support transactions, this means that we need
 * to open a transaction as well.  This means that this dialog juggles the
 * following things:
 *
 * Given a valid property name and a property type we open a transaction and
 * create the property.  As soon as the property name or type is changed, we
 * abort the transaction and start a new transaction with a recreated property
 * with the new name or type.
 *
 * If the property name or type is invalid, we clear the transaction and as
 * soon as the name or type become valid again, we start a new transaction.
 *
 * If the type is changed, we need to clear the current expression and value to
 * the default value.  If only the name is changed, we keep the value as much
 * as possible with two exceptions: having a value based on an expression or
 * being a value for a property link.
 *
 * Expressions and links are bound to the property and changing the name of a
 * property prompts us to remove the old property and create a new one.  This
 * to make sure that the transaction for adding a property does not keep a
 * history of old property name changes.  So, because we want to have a new
 * transaction and expressions and links are bound to a property, the
 * expression or link becomes invalid when changing the property name.
 *
 * For expressions there are two choices: 1) we leave the outcome of the
 * expression in the value field (which is possible) but without it being based
 * on an expression or 2) we reset the value to the default value of the type.
 *
 * I chose the latter option because it is easy to miss that on a property name
 * change, the expression is invalidated, so the user may think the value is
 * the result of an expression, whereas in reality, the expression is lost.
 *
 * All in all, this leads to the following entities that need to be managed:
 * - transaction
 * - property item
 *   - property
 *   - editor
 *   - value of the editor
 *
 * We have to react on a name change and on a type change.  For each of these
 * changes, we need to take three cases into account:
 * - the name and type are valid
 * - only the type is valid
 * - neither the name nor the type is valid
 *
 * This has been encoded in the code below as onNameFieldChanged() and
 * onTypeChanged() and it works in two phases: clearing the transaction,
 * property item, and related, and building it up again depending on the
 * situation.
 */

DlgAddProperty::DlgAddProperty(QWidget* parent, App::PropertyContainer* container)
    : DlgAddProperty(parent, container, nullptr)
{}


DlgAddProperty::DlgAddProperty(QWidget* parent, ViewProviderVarSet* viewProvider)
    : DlgAddProperty(
          parent,
          viewProvider ? viewProvider->getObject<App::PropertyContainer>() : nullptr,
          viewProvider
      )
{}

DlgAddProperty::DlgAddProperty(
    QWidget* parent,
    App::PropertyContainer* container,
    ViewProviderVarSet* viewProvider
)
    : QDialog(parent)
    , container(container)
    , ui(new Ui_DlgAddProperty)
    , comboBoxGroup(this)
    , completerType(this)
    , editor(nullptr)
    , transactionID(0)
{
    ui->setupUi(this);
    setupMacroRedirector();
    initializeWidgets(viewProvider);
}

DlgAddProperty::~DlgAddProperty() = default;

void DlgAddProperty::setupMacroRedirector()
{
    setValueRedirector = std::make_unique<MacroManager::MacroRedirector>(
        [this](MacroManager::LineType /*type*/, const char* line) { this->setValueCommand = line; }
    );
}

int DlgAddProperty::findLabelRow(const char* labelName, QFormLayout* layout)
{
    for (int row = 0; row < layout->rowCount(); ++row) {
        QLayoutItem* labelItem = layout->itemAt(row, QFormLayout::LabelRole);
        if (labelItem == nullptr) {
            continue;
        }

        if (auto label = qobject_cast<QLabel*>(labelItem->widget())) {
            if (label->objectName() == QString::fromLatin1(labelName)) {
                return row;
            }
        }
    }
    return -1;
}

void DlgAddProperty::removeExistingWidget(QFormLayout* formLayout, int labelRow)
{
    if (QLayoutItem* existingItem = formLayout->itemAt(labelRow, QFormLayout::FieldRole)) {
        if (QWidget* existingWidget = existingItem->widget()) {
            formLayout->removeWidget(existingWidget);
            existingWidget->deleteLater();
        }
    }
}


void DlgAddProperty::setWidgetForLabel(const char* labelName, QWidget* widget, QLayout* layout)
{
    auto formLayout = qobject_cast<QFormLayout*>(layout);
    if (formLayout == nullptr) {
        FC_ERR("Form layout not found");
        return;
    }

    int labelRow = findLabelRow(labelName, formLayout);
    if (labelRow < 0) {
        FC_ERR("Could not find row for '" << labelName << "'");
        return;
    }

    removeExistingWidget(formLayout, labelRow);
    formLayout->setWidget(labelRow, QFormLayout::FieldRole, widget);
}

void DlgAddProperty::populateGroup(EditFinishedComboBox& comboBox, const App::PropertyContainer* container)
{
    auto paramGroup = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/PropertyView"
    );
    std::string lastGroup = paramGroup->GetASCII("NewPropertyGroup");

    std::vector<App::Property*> properties;
    container->getPropertyList(properties);

    std::unordered_set<std::string> groupNames;
    for (const auto* prop : properties) {
        const char* groupName = container->getPropertyGroup(prop);
        groupNames.insert(groupName ? groupName : GroupBase);
    }

    std::vector<std::string> groupNamesSorted(groupNames.begin(), groupNames.end());
    std::ranges::sort(groupNamesSorted, [](const std::string& a, const std::string& b) {
        // prefer anything else other than Base, so move it to the back
        if (a == GroupBase) {
            return false;
        }
        if (b == GroupBase) {
            return true;
        }
        return a < b;
    });

    for (const auto& groupName : groupNamesSorted) {
        comboBox.addItem(QString::fromStdString(groupName));
    }

    if (!lastGroup.empty() && std::ranges::find(groupNames, lastGroup) != groupNames.end()) {
        comboBox.setEditText(QString::fromStdString(lastGroup));
    }
    else {
        comboBox.setEditText(QString::fromStdString(groupNamesSorted[0]));
    }
}

void DlgAddProperty::initializeGroup()
{
    comboBoxGroup.setObjectName(QStringLiteral("comboBoxGroup"));
    comboBoxGroup.setInsertPolicy(QComboBox::InsertAtTop);
    comboBoxGroup.setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    setWidgetForLabel("labelGroup", &comboBoxGroup, layout());
    populateGroup(comboBoxGroup, container);

    connComboBoxGroup = connect(
        &comboBoxGroup,
        &EditFinishedComboBox::editFinished,
        this,
        &DlgAddProperty::onGroupFinished
    );
}

std::vector<Base::Type> DlgAddProperty::getSupportedTypes()
{
    std::vector<Base::Type> supportedTypes;
    std::vector<Base::Type> allTypes;
    Base::Type::getAllDerivedFrom(Base::Type::fromName("App::Property"), allTypes);

    std::ranges::copy_if(allTypes, std::back_inserter(supportedTypes), [&](const Base::Type& type) {
        return type.canInstantiate() && isTypeWithEditor(type);
    });

    std::ranges::sort(supportedTypes, [](Base::Type a, Base::Type b) {
        return strcmp(a.getName(), b.getName()) < 0;
    });

    return supportedTypes;
}

void DlgAddProperty::initializeTypes()
{
    auto paramGroup = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/PropertyView"
    );
    auto lastType = Base::Type::fromName(
        paramGroup->GetASCII("NewPropertyType", "App::PropertyLength").c_str()
    );
    if (lastType.isBad()) {
        lastType = App::PropertyLength::getClassTypeId();
    }

    std::vector<Base::Type> types = getSupportedTypes();

    for (const auto& type : types) {
        ui->comboBoxType->addItem(QString::fromLatin1(type.getName()));
        if (type == lastType) {
            ui->comboBoxType->setCurrentIndex(ui->comboBoxType->count() - 1);
        }
    }

    completerType.setModel(ui->comboBoxType->model());
    completerType.setCaseSensitivity(Qt::CaseInsensitive);
    completerType.setFilterMode(Qt::MatchContains);
    ui->comboBoxType->setCompleter(&completerType);
    ui->comboBoxType->setInsertPolicy(QComboBox::NoInsert);

    connComboBoxType = connect(
        ui->comboBoxType,
        &QComboBox::currentTextChanged,
        this,
        &DlgAddProperty::onTypeChanged
    );
}

void DlgAddProperty::removeSelectionEditor()
{
    // If the editor has a lineedit, then Qt selects the string inside it when
    // the editor is created.  This interferes with the editor getting focus.
    // For example, units will then be selected as well, whereas this is not
    // the behavior we want.  We therefore deselect the text in the lineedit.
    if (auto lineEdit = editor->findChild<QLineEdit*>()) {
        lineEdit->deselect();
    }
}

void DlgAddProperty::addEnumEditor(PropertyItem* propertyItem)
{
    auto* values = static_cast<PropertyStringListItem*>(PropertyStringListItem::create());
    values->setParent(propertyItem);
    values->setPropertyName(QLatin1String(QT_TRANSLATE_NOOP("App::Property", "Enum")));
    if (propertyItem->childCount() > 0) {
        auto* child = propertyItem->takeChild(0);
        delete child;
    }
    propertyItem->appendChild(values);
    editor.reset(
        values->createEditor(this, [this]() { this->valueChangedEnum(); }, FrameOption::WithFrame)
    );
}

void DlgAddProperty::addNormalEditor(PropertyItem* propertyItem)
{
    editor.reset(
        propertyItem->createEditor(this, [this]() { this->valueChanged(); }, FrameOption::WithFrame)
    );
}

void DlgAddProperty::addEditor(PropertyItem* propertyItem)
{
    if (isSubLinkPropertyItem()) {
        // Since sublinks need the 3D view to select an object and the dialog
        // is modal, we do not provide an editor for sublinks.  It is possible
        // to create a property of this type though and the property can be set
        // in the property view later which does give access to the 3D view.
        return;
    }

    if (isEnumPropertyItem()) {
        addEnumEditor(propertyItem);
    }
    else {
        addNormalEditor(propertyItem);
    }
    if (editor == nullptr) {
        return;
    }

    // Make sure that the editor has the same height as the
    // other widgets in the dialog.
    editor->setMinimumHeight(comboBoxGroup.height());

    QSignalBlocker blockSignals(editor.get());

    // To set the data in the editor, we need to set the data in the
    // propertyItem.  The propertyItem needs to have a property set to make
    // sure that we get a correct value and the unit.
    setEditorData(propertyItem->data(PropertyItem::ValueColumn, Qt::EditRole));
    editor->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    editor->setObjectName(QStringLiteral("editor"));

    setWidgetForLabel("labelValue", editor.get(), layout());

    QWidget::setTabOrder(ui->comboBoxType, editor.get());
    QWidget::setTabOrder(editor.get(), ui->lineEditToolTip);

    removeSelectionEditor();
}

bool DlgAddProperty::isTypeWithEditor(const Base::Type& type)
{
    static const std::initializer_list<Base::Type> subTypesWithEditor = {
        // These types and their subtypes have editors.
        App::PropertyBool::getClassTypeId(),
        App::PropertyColor::getClassTypeId(),
        App::PropertyFileIncluded::getClassTypeId(),
        App::PropertyFloat::getClassTypeId(),
        App::PropertyInteger::getClassTypeId()
    };

    static const std::initializer_list<Base::Type> typesWithEditor = {
        // These types have editors but not necessarily their subtypes.
        App::PropertyEnumeration::getClassTypeId(),
        App::PropertyFile::getClassTypeId(),
        App::PropertyFloatList::getClassTypeId(),
        App::PropertyFont::getClassTypeId(),
        App::PropertyIntegerList::getClassTypeId(),
        App::PropertyLink::getClassTypeId(),
        App::PropertyLinkSub::getClassTypeId(),
        App::PropertyLinkList::getClassTypeId(),
        App::PropertyLinkSubList::getClassTypeId(),
        App::PropertyXLink::getClassTypeId(),
        App::PropertyXLinkSub::getClassTypeId(),
        App::PropertyXLinkList::getClassTypeId(),
        App::PropertyXLinkSubList::getClassTypeId(),
        App::PropertyMaterialList::getClassTypeId(),
        App::PropertyPath::getClassTypeId(),
        App::PropertyString::getClassTypeId(),
        App::PropertyStringList::getClassTypeId(),
        App::PropertyVectorList::getClassTypeId()
    };

    const auto isDerivedFromType = [&type](const Base::Type& t) {
        return type.isDerivedFrom(t);
    };

    return std::ranges::find(typesWithEditor, type) != typesWithEditor.end()
        || std::ranges::any_of(subTypesWithEditor, isDerivedFromType);
}

bool DlgAddProperty::isTypeWithEditor(const std::string& type)
{
    Base::Type propType
        = Base::Type::getTypeIfDerivedFrom(type.c_str(), App::Property::getClassTypeId(), true);
    return isTypeWithEditor(propType);
}

static PropertyItem* createPropertyItem(App::Property* prop)
{
    const char* editor = prop->getEditorName();
    if (Base::Tools::isNullOrEmpty(editor)) {
        return nullptr;
    }
    return PropertyItemFactory::instance().createPropertyItem(editor);
}

void DlgAddProperty::createEditorForType(const Base::Type& type)
{
    // Temporarily create a property for two reasons:
    // - to acquire the editor name from the instance, and
    // - to acquire an initial value from the instance possibly with the correct unit.
    void* propInstance = type.createInstance();
    if (!propInstance) {
        FC_THROWM(Base::RuntimeError, "Failed to create a property of type " << type.getName());
    }

    // When prop goes out of scope, it can be deleted because we obtained the
    // propertyItem (if applicable) and we initialized the editor with the data
    // from the property.
    std::unique_ptr<App::Property, void (*)(App::Property*)> prop(
        static_cast<App::Property*>(propInstance),
        [](App::Property* p) { delete p; }
    );
    prop->setContainer(container);

    propertyItem.reset(createPropertyItem(prop.get()));

    if (propertyItem && isTypeWithEditor(type)) {
        propertyItem->setPropertyData({prop.get()});
        addEditor(propertyItem.get());
        propertyItem->removeProperty(prop.get());
    }
}

void DlgAddProperty::initializeValue()
{
    std::string type = ui->comboBoxType->currentText().toStdString();

    Base::Type propType
        = Base::Type::getTypeIfDerivedFrom(type.c_str(), App::Property::getClassTypeId(), true);
    if (propType.isBad()) {
        return;
    }

    if (isTypeWithEditor(propType)) {
        createEditorForType(propType);
    }
    else {
        removeEditor();
    }
}

void DlgAddProperty::setTitle()
{
    setWindowTitle(tr("Add Property"));
}

void DlgAddProperty::setAddEnabled(bool enabled)
{
    QPushButton* addButton = ui->buttonBox->button(QDialogButtonBox::Ok);
    QPushButton* closeButton = ui->buttonBox->button(QDialogButtonBox::Close);
    closeButton->setDefault(!enabled);
    addButton->setDefault(enabled);
    addButton->setEnabled(enabled);
}

void DlgAddProperty::initializeWidgets(ViewProviderVarSet* viewProvider)
{
    initializeGroup();
    initializeTypes();
    initializeValue();

    if (viewProvider) {
        connect(this, &QDialog::finished, this, [viewProvider](int result) {
            viewProvider->onFinished(result);
        });
    }
    connLineEditNameTextChanged
        = connect(ui->lineEditName, &QLineEdit::textChanged, this, &DlgAddProperty::onNameChanged);

    setTitle();
    QPushButton* addButton = ui->buttonBox->button(QDialogButtonBox::Ok);
    addButton->setText(tr("Add"));
    setAddEnabled(false);

    ui->lineEditName->setFocus();

    QWidget::setTabOrder(ui->lineEditName, &comboBoxGroup);
    QWidget::setTabOrder(&comboBoxGroup, ui->comboBoxType);

    adjustSize();
}

bool DlgAddProperty::propertyExists(const std::string& name)
{
    App::Property* prop = container->getPropertyByName(name.c_str());
    return prop && prop->getContainer() == container
        && !(propertyItem && propertyItem->getFirstProperty() == prop);
}

bool DlgAddProperty::isNameValid()
{
    std::string name = ui->lineEditName->text().toStdString();

    return !name.empty() && name == Base::Tools::getIdentifier(name)
        && !App::ExpressionParser::isTokenAConstant(name)
        && !App::ExpressionParser::isTokenAUnit(name) && !propertyExists(name);
}

bool DlgAddProperty::isGroupValid()
{
    std::string group = comboBoxGroup.currentText().toStdString();
    return !group.empty() && group == Base::Tools::getIdentifier(group);
}

bool DlgAddProperty::isTypeValid()
{
    std::string type = ui->comboBoxType->currentText().toStdString();
    return Base::Type::fromName(type.c_str()).isDerivedFrom(App::Property::getClassTypeId())
        && type != "App::Property";
}

bool DlgAddProperty::isDocument() const
{
    return container->isDerivedFrom<App::Document>();
}

bool DlgAddProperty::isDocumentObject() const
{
    return container->isDerivedFrom<App::DocumentObject>();
}

bool DlgAddProperty::areFieldsValid()
{
    return isNameValid() && isGroupValid() && isTypeValid();
}

void DlgAddProperty::showStatusMessage()
{
    QString error;
    QString text = ui->lineEditName->text();
    std::string name = text.toStdString();

    if (!isGroupValid()) {
        error = tr("Invalid group name");
    }
    else if (!isTypeValid()) {
        error = tr("Invalid type name");
    }
    else if (name.empty()) {
        error.clear();
    }
    else if (name != Base::Tools::getIdentifier(name)) {
        error = tr("Invalid property name '%1'").arg(text);
    }
    else if (propertyExists(name)) {
        error = tr("Property '%1' already exists").arg(text);
    }
    else if (App::ExpressionParser::isTokenAConstant(name)) {
        error = tr("'%1' is a constant").arg(text);
    }
    else if (App::ExpressionParser::isTokenAUnit(name)) {
        error = tr("'%1' is a unit").arg(text);
    }

    ui->labelError->setText(error);
}

void DlgAddProperty::removeEditor()
{
    if (editor == nullptr) {
        return;
    }

    // Create a placeholder widget to keep the layout intact.
    auto* placeholder = new QWidget(this);
    placeholder->setObjectName(QStringLiteral("placeholder"));
    placeholder->setMinimumHeight(comboBoxGroup.height());
    setWidgetForLabel("labelValue", placeholder, layout());

    QWidget::setTabOrder(ui->comboBoxType, ui->lineEditToolTip);
    editor = nullptr;
}

bool DlgAddProperty::isEnumPropertyItem() const
{
    return ui->comboBoxType->currentText()
        == QString::fromLatin1(App::PropertyEnumeration::getClassTypeId().getName());
}

bool DlgAddProperty::isSubLinkPropertyItem() const
{
    const QString type = ui->comboBoxType->currentText();
    static const std::array<const char*, 4> sublinkTypes = {
        App::PropertyLinkSub::getClassTypeId().getName(),
        App::PropertyLinkSubList::getClassTypeId().getName(),
        App::PropertyXLinkSub::getClassTypeId().getName(),
        App::PropertyXLinkSubList::getClassTypeId().getName()
    };
    return std::ranges::any_of(sublinkTypes, [&type](const char* subLinkType) {
        return type == QString::fromLatin1(subLinkType);
    });
}

QVariant DlgAddProperty::getEditorData() const
{
    if (isEnumPropertyItem()) {
        PropertyItem* child = propertyItem->child(0);
        if (child == nullptr) {
            return {};
        }
        return child->editorData(editor.get());
    }

    return propertyItem->editorData(editor.get());
}

void DlgAddProperty::setEditorData(const QVariant& data)
{
    if (isEnumPropertyItem()) {
        PropertyItem* child = propertyItem->child(0);
        if (child == nullptr) {
            return;
        }
        child->setEditorData(editor.get(), data);
    }
    else {
        propertyItem->setEditorData(editor.get(), data);
    }
}

void DlgAddProperty::setEditor(bool valueNeedsReset)
{
    if (editor && !valueNeedsReset) {
        QVariant data = getEditorData();
        addEditor(propertyItem.get());
        if (editor == nullptr) {
            return;
        }
        setEditorData(data);
        removeSelectionEditor();
    }
    else if (propertyItem) {
        addEditor(propertyItem.get());
    }
    else {
        initializeValue();
    }

    if (editor) {
        QVariant data = propertyItem->editorData(editor.get());
        propertyItem->setData(data);
    }
}

void DlgAddProperty::setPropertyItem(App::Property* prop, bool supportsExpressions)
{
    if (prop == nullptr) {
        return;
    }

    if (propertyItem == nullptr) {
        propertyItem.reset(createPropertyItem(prop));
    }

    if (propertyItem == nullptr) {
        return;
    }

    propertyItem->setAutoApply(true);
    propertyItem->setPropertyData({prop});
    if (supportsExpressions) {
        objectIdentifier = std::make_unique<App::ObjectIdentifier>(*prop);
        propertyItem->bind(*objectIdentifier);
    }
}

void DlgAddProperty::buildForUnbound(bool valueNeedsReset)
{
    setEditor(valueNeedsReset);
}

void DlgAddProperty::buildForBound(bool valueNeedsReset, bool supportsExpressions)
{
    openTransaction();
    App::Property* prop = createProperty();
    setPropertyItem(prop, supportsExpressions);
    setEditor(valueNeedsReset);
}

bool DlgAddProperty::clearBoundProperty()
{
    // Both a property link and an expression are bound to a property and as a
    // result need the value to be reset.
    bool isPropertyLinkItem = qobject_cast<PropertyLinkItem*>(propertyItem.get()) != nullptr;
    bool valueNeedsReset = isPropertyLinkItem || propertyItem->hasExpression();

    if (App::Property* prop = propertyItem->getFirstProperty()) {
        propertyItem->unbind();
        propertyItem->removeProperty(prop);
        container->removeDynamicProperty(prop->getName());
        closeTransaction(TransactionOption::Abort);
    }
    return valueNeedsReset;
}

bool DlgAddProperty::clear(FieldChange fieldChange)
{
    if (propertyItem == nullptr) {
        return true;
    }

    bool valueNeedsReset = clearBoundProperty();

    if (fieldChange == FieldChange::Type) {
        valueNeedsReset = true;
        removeEditor();
        propertyItem = nullptr;
    }
    return valueNeedsReset;
}

void DlgAddProperty::onNameChanged([[maybe_unused]] const QString& text)
{
    bool valueNeedsReset = clear(FieldChange::Name);
    if (isNameValid() && isTypeValid()) {
        buildForBound(valueNeedsReset, isDocumentObject());
    }
    else if (isTypeValid()) {
        buildForUnbound(valueNeedsReset);
    }
    else {
        removeEditor();
        propertyItem = nullptr;
    }

    setAddEnabled(areFieldsValid());
    showStatusMessage();
}

void DlgAddProperty::onGroupFinished()
{
    if (isGroupValid() && propertyItem) {
        std::string group = comboBoxGroup.currentText().toStdString();
        std::string doc = ui->lineEditToolTip->text().toStdString();
        if (App::Property* prop = propertyItem->getFirstProperty();
            prop && prop->getGroup() != group) {
            container->changeDynamicProperty(prop, group.c_str(), doc.c_str());
        }
    }

    setAddEnabled(areFieldsValid());
    showStatusMessage();
}

void DlgAddProperty::onTypeChanged([[maybe_unused]] const QString& text)
{
    bool valueNeedsReset = clear(FieldChange::Type);
    if (isNameValid() && isTypeValid()) {
        buildForBound(valueNeedsReset, isDocumentObject());
    }
    else if (isTypeValid()) {
        buildForUnbound(valueNeedsReset);
    }
    // nothing if both name and type are invalid

    setAddEnabled(areFieldsValid());
    showStatusMessage();
}

void DlgAddProperty::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        setTitle();
    }
    QDialog::changeEvent(e);
}

void DlgAddProperty::valueChangedEnum()
{
    auto* propEnum = static_cast<App::PropertyEnumeration*>(propertyItem->getFirstProperty());
    if (propEnum == nullptr || propertyItem->childCount() == 0) {
        return;
    }

    auto* values = static_cast<PropertyStringListItem*>(propertyItem->child(0));
    QVariant data = values->editorData(editor.get());
    QStringList enumValues = data.toStringList();
    // convert to std::vector<std::string>
    std::vector<std::string> enumValuesVec;
    std::ranges::transform(enumValues, std::back_inserter(enumValuesVec), [](const QString& value) {
        return value.toStdString();
    });
    propEnum->setEnums(enumValuesVec);
}

void DlgAddProperty::valueChanged()
{
    QVariant data = propertyItem->editorData(editor.get());
    propertyItem->setData(data);
}

/* We use these functions rather than the functions provided by App::Document
 * because this dialog may be opened when another transaction is in progress.
 * An example is opening a sketch.  If this dialog uses the functions provided
 * by App::Document, a reject of the dialog would close that transaction.  By
 * checking whether the transaction ID is "our" transaction ID, we prevent this
 * behavior.
 */
void DlgAddProperty::openTransaction()
{
    transactionID = App::GetApplication().setActiveTransaction("Add property");
}

void DlgAddProperty::critical(const QString& title, const QString& text)
{
    static bool criticalDialogShown = false;
    if (!criticalDialogShown) {
        Base::StateLocker locker(criticalDialogShown);
        QMessageBox::critical(this, title, text);
    }
}

void DlgAddProperty::recordMacroAdd(
    const App::PropertyContainer* container,
    const std::string& type,
    const std::string& name,
    const std::string& group,
    const std::string& doc
) const
{
    std::ostringstream command;
    command << "App.getDocument('";
    const App::Document* document = freecad_cast<App::Document*>(container);
    const App::DocumentObject* object = freecad_cast<App::DocumentObject*>(container);
    if (document) {
        command << document->getName() << "')";
    }
    else if (object) {
        command << object->getDocument()->getName() << "')." << object->getNameInDocument();
    }
    else {
        FC_ERR("Cannot record macro for container of type " << container->getTypeId().getName());
        return;
    }
    command << ".addProperty('" << type << "', '" << name << "', '" << group << "', '" << doc + "')";
    Application::Instance->macroManager()->addLine(Gui::MacroManager::App, command.str().c_str());
}

App::Property* DlgAddProperty::createProperty()
{
    std::string name = ui->lineEditName->text().toStdString();
    std::string group = comboBoxGroup.currentText().toStdString();
    std::string type = ui->comboBoxType->currentText().toStdString();
    std::string doc = ui->lineEditToolTip->text().toStdString();

    auto recordAddCommand = [this](MacroManager::LineType, const char* line) {
        this->addCommand = line;
    };

    try {
        App::Property* prop
            = container->addDynamicProperty(type.c_str(), name.c_str(), group.c_str(), doc.c_str());
        MacroManager::MacroRedirector redirector(recordAddCommand);
        recordMacroAdd(container, type, name, group, doc);
        return prop;
    }
    catch (Base::Exception& e) {
        e.reportException();
        critical(
            QObject::tr("Add property"),
            QObject::tr("Failed to add property to '%1': %2")
                .arg(QString::fromLatin1(container->getFullName().c_str()), QString::fromUtf8(e.what()))
        );
        return nullptr;
    }
}

void DlgAddProperty::closeTransaction(TransactionOption option)
{
    if (transactionID == 0) {
        return;
    }

    App::GetApplication().closeActiveTransaction(static_cast<bool>(option), transactionID);
    transactionID = 0;
}

void DlgAddProperty::clearFields()
{
    {
        QSignalBlocker blocker(ui->lineEditName);
        ui->lineEditName->clear();
    }
    ui->lineEditToolTip->clear();
    initializeValue();
    setAddEnabled(false);
}

void DlgAddProperty::addDocumentation()
{
    /* Since there is no check on documentation (we accept any string), there
     * is no signal handler for the documentation field.  This method updates
     * the property that is being added with the text inserted as
     * documentation/tooltip.
     */

    std::string group = comboBoxGroup.currentText().toStdString();
    std::string doc = ui->lineEditToolTip->text().toStdString();

    if (propertyItem == nullptr) {
        // If there is no property item, we cannot add documentation.
        return;
    }

    App::Property* prop = propertyItem->getFirstProperty();
    if (prop == nullptr) {
        return;
    }

    container->changeDynamicProperty(prop, group.c_str(), doc.c_str());
}

void DlgAddProperty::accept()
{
    addDocumentation();
    auto* object = freecad_cast<App::DocumentObject*>(container);
    if (object) {
        object->ExpressionEngine.execute();
    }
    closeTransaction(TransactionOption::Commit);

    setValueRedirector = nullptr;
    Application::Instance->macroManager()->addLine(MacroManager::LineType::App, addCommand.c_str());
    Application::Instance->macroManager()->addLine(MacroManager::LineType::App, setValueCommand.c_str());
    setupMacroRedirector();

    std::string group = comboBoxGroup.currentText().toStdString();
    std::string type = ui->comboBoxType->currentText().toStdString();
    auto paramGroup = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/PropertyView"
    );
    paramGroup->SetASCII("NewPropertyType", type.c_str());
    paramGroup->SetASCII("NewPropertyGroup", group.c_str());

    clearFields();
    ui->lineEditName->setFocus();

    // Note that we don't call QDialog::accept() here to keep the dialog
    // open for adding more properties.
}

void DlgAddProperty::reject()
{
    if (propertyItem) {
        if (App::Property* prop = propertyItem->getFirstProperty()) {
            App::PropertyContainer* container = prop->getContainer();
            container->removeDynamicProperty(prop->getName());
            closeTransaction(TransactionOption::Abort);
        }
    }
    disconnect(connComboBoxGroup);
    disconnect(connComboBoxType);
    disconnect(connLineEditNameTextChanged);

    QDialog::reject();
}

#include "moc_DlgAddProperty.cpp"
