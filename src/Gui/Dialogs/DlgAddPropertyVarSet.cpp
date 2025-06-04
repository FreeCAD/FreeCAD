/****************************************************************************
 *   Copyright (c) 2024 Ondsel <development@ondsel.com>                     *
 *   Copyright (c) 2025 Pieter Hijma <info@pieterhijma.net>                 *
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
# include <QMessageBox>
# include <QString>
# include <QCompleter>
# include <algorithm>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/ExpressionParser.h>
#include <App/PropertyUnits.h>
#include <App/PropertyFile.h>
#include <App/PropertyGeo.h>
#include <Base/Tools.h>

#include "Dialogs/DlgAddPropertyVarSet.h"
#include "ui_DlgAddPropertyVarSet.h"
#include "ViewProviderVarSet.h"

FC_LOG_LEVEL_INIT("DlgAddPropertyVarSet", true, true)

using namespace Gui;
using namespace Gui::Dialog;

const std::string DlgAddPropertyVarSet::GroupBase = "Base";

DlgAddPropertyVarSet::DlgAddPropertyVarSet(QWidget* parent,
                                           ViewProviderVarSet* viewProvider)
    : QDialog(parent),
      varSet(viewProvider->getObject<App::VarSet>()),
      ui(new Ui_DlgAddPropertyVarSet),
      comboBoxGroup(this),
      completerType(this),
      editor(nullptr),
      transactionID(0)
{
    ui->setupUi(this);

    initializeWidgets(viewProvider);
}

DlgAddPropertyVarSet::~DlgAddPropertyVarSet() = default;

int DlgAddPropertyVarSet::findLabelRow(const char* labelName, QFormLayout* layout)
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

void DlgAddPropertyVarSet::setWidgetForLabel(const char* labelName, QWidget* widget)
{
    auto formLayout = qobject_cast<QFormLayout*>(layout());
    if (formLayout == nullptr) {
        FC_ERR("Form layout not found");
        return;
    }

    int labelRow = findLabelRow(labelName, formLayout);
    if (labelRow < 0) {
        FC_ERR("Couldn't find row for '" << labelName << "'");
        return;
    }

    formLayout->setWidget(labelRow, QFormLayout::FieldRole, widget);
}

void DlgAddPropertyVarSet::initializeGroup()
{
    comboBoxGroup.setObjectName(QStringLiteral("comboBoxGroup"));
    comboBoxGroup.setInsertPolicy(QComboBox::InsertAtTop);
    comboBoxGroup.setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    setWidgetForLabel("labelGroup", &comboBoxGroup);

    std::vector<App::Property*> properties;
    varSet->getPropertyList(properties);

    std::unordered_set<std::string> groupNames;
    for (const auto* prop : properties) {
        const char* groupName = varSet->getPropertyGroup(prop);
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
        comboBoxGroup.addItem(QString::fromStdString(groupName));
    }

    comboBoxGroup.setEditText(QString::fromStdString(groupNamesSorted[0]));
    connComboBoxGroup = connect(&comboBoxGroup, &EditFinishedComboBox::currentTextChanged,
                                this, &DlgAddPropertyVarSet::onTextFieldChanged);
}

std::vector<Base::Type> DlgAddPropertyVarSet::getSupportedTypes()
{
    std::vector<Base::Type> supportedTypes;
    std::vector<Base::Type> allTypes;
    Base::Type::getAllDerivedFrom(Base::Type::fromName("App::Property"), allTypes);

    std::ranges::copy_if(allTypes, std::back_inserter(supportedTypes),
                         [](const Base::Type& type) {
                             return type.canInstantiate();
                         });

    std::ranges::sort(supportedTypes, [](Base::Type a, Base::Type b) {
        return strcmp(a.getName(), b.getName()) < 0;
    });

    return supportedTypes;
}

void DlgAddPropertyVarSet::initializeTypes()
{
    auto paramGroup = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/PropertyView");
    auto lastType = Base::Type::fromName(
            paramGroup->GetASCII("NewPropertyType", "App::PropertyLength").c_str());
    if (lastType.isBad()) {
        lastType = App::PropertyLength::getClassTypeId();
    }

    std::vector<Base::Type> types = getSupportedTypes();

    for(const auto& type : types) {
        ui->comboBoxType->addItem(QString::fromLatin1(type.getName()));
        if (type == lastType) {
            ui->comboBoxType->setCurrentIndex(ui->comboBoxType->count()-1);
        }
    }

    completerType.setModel(ui->comboBoxType->model());
    completerType.setCaseSensitivity(Qt::CaseInsensitive);
    completerType.setFilterMode(Qt::MatchContains);
    ui->comboBoxType->setCompleter(&completerType);
    ui->comboBoxType->setInsertPolicy(QComboBox::NoInsert);

    connComboBoxType = connect(ui->comboBoxType, &QComboBox::currentTextChanged,
                               this, &DlgAddPropertyVarSet::onTypeChanged);
}

void DlgAddPropertyVarSet::removeSelectionEditor()
{
    // If the editor has a lineedit, then Qt selects the string inside it when
    // the editor is created.  This interferes with the editor getting focus.
    // For example, units will then be selected as well, whereas this is not
    // the behavior we want.  We therefore deselect the text in the lineedit.
    if (auto lineEdit = editor->findChild<QLineEdit*>()) {
        lineEdit->deselect();
    }
}

void DlgAddPropertyVarSet::addEditor(PropertyEditor::PropertyItem* propertyItem)
{
    editor.reset(propertyItem->createEditor(this, [this]() {
        this->valueChanged();
    }, PropertyEditor::FrameOption::WithFrame));
    if (editor == nullptr) {
        return;
    }

    QSignalBlocker blockSignals(editor.get());

    // To set the data in the editor, we need to set the data in the
    // propertyItem.  The propertyItem needs to have a property set to make
    // sure that we get a correct value and the unit.
    propertyItem->setEditorData(
            editor.get(),
            propertyItem->data(PropertyEditor::PropertyItem::ValueColumn, Qt::EditRole));
    editor->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    editor->setObjectName(QStringLiteral("editor"));

    setWidgetForLabel("labelValue", editor.get());

    QWidget::setTabOrder(ui->comboBoxType, editor.get());
    QWidget::setTabOrder(editor.get(), ui->checkBoxAdd);

    removeSelectionEditor();
}

bool DlgAddPropertyVarSet::isTypeWithEditor(const Base::Type& type)
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
        App::PropertyFile::getClassTypeId(),
        App::PropertyFloatList::getClassTypeId(),
        App::PropertyFont::getClassTypeId(),
        App::PropertyIntegerList::getClassTypeId(),
        App::PropertyMaterialList::getClassTypeId(),
        App::PropertyPath::getClassTypeId(),
        App::PropertyString::getClassTypeId(),
        App::PropertyStringList::getClassTypeId(),
        App::PropertyVectorList::getClassTypeId()
    };

    const auto isDerivedFromType = [&type](const Base::Type& t) {
        return type.isDerivedFrom(t);
    };

    return std::ranges::find(typesWithEditor, type) != typesWithEditor.end() ||
        std::ranges::any_of(subTypesWithEditor, isDerivedFromType);
}

bool DlgAddPropertyVarSet::isTypeWithEditor(const std::string& type)
{
    Base::Type propType =
        Base::Type::getTypeIfDerivedFrom(type.c_str(), App::Property::getClassTypeId(), true);
    return isTypeWithEditor(propType);
}

static PropertyEditor::PropertyItem *createPropertyItem(App::Property *prop)
{
    const char *editor = prop->getEditorName();
    if (Base::Tools::isNullOrEmpty(editor)) {
        return nullptr;
    }
    return PropertyEditor::PropertyItemFactory::instance().createPropertyItem(editor);
}

void DlgAddPropertyVarSet::createEditorForType(const Base::Type& type)
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
    std::unique_ptr<App::Property, void(*)(App::Property*)> prop(
            static_cast<App::Property*>(propInstance),
            [](App::Property* p) { delete p; });
    prop->setContainer(varSet);

    propertyItem.reset(createPropertyItem(prop.get()));

    if (propertyItem && isTypeWithEditor(type)) {
        propertyItem->setPropertyData({prop.get()});
        addEditor(propertyItem.get());
        propertyItem->removeProperty(prop.get());
    }
}

void DlgAddPropertyVarSet::initializeValue()
{
    std::string type = ui->comboBoxType->currentText().toStdString();

    Base::Type propType =
        Base::Type::getTypeIfDerivedFrom(type.c_str(), App::Property::getClassTypeId(), true);
    if (propType.isBad()) {
        FC_THROWM(Base::TypeError, "Invalid type " << type << " for property");
    }

    if (isTypeWithEditor(propType)) {
        createEditorForType(propType);
    }
}

void DlgAddPropertyVarSet::setTitle()
{
    setWindowTitle(QObject::tr("Add a property to %1").arg(QString::fromStdString(varSet->getFullName())));
}

void DlgAddPropertyVarSet::setOkEnabled(bool enabled)
{
    QPushButton *okButton = ui->buttonBox->button(QDialogButtonBox::Ok);
    okButton->setEnabled(enabled);
}

void DlgAddPropertyVarSet::initializeWidgets(ViewProviderVarSet* viewProvider)
{
    initializeGroup();
    initializeTypes();
    initializeValue();

    connect(this, &QDialog::finished,
            this, [viewProvider](int result) { viewProvider->onFinished(result); });
    connLineEditNameTextChanged = connect(ui->lineEditName, &QLineEdit::textChanged,
            this, &DlgAddPropertyVarSet::onTextFieldChanged);

    setTitle();
    setOkEnabled(false);

    ui->lineEditName->setFocus();

    QWidget::setTabOrder(ui->lineEditName, &comboBoxGroup);
    QWidget::setTabOrder(&comboBoxGroup, ui->comboBoxType);
}

bool DlgAddPropertyVarSet::propertyExists(const std::string& name)
{
    App::Property* prop = varSet->getPropertyByName(name.c_str());
    return prop && prop->getContainer() == varSet;
}

bool DlgAddPropertyVarSet::isNameValid()
{
    std::string name = ui->lineEditName->text().toStdString();

    return !name.empty() &&
        name == Base::Tools::getIdentifier(name) &&
        !App::ExpressionParser::isTokenAConstant(name) &&
        !App::ExpressionParser::isTokenAUnit(name) &&
        !propertyExists(name);
}

bool DlgAddPropertyVarSet::isGroupValid()
{
    std::string group = comboBoxGroup.currentText().toStdString();
    return !group.empty() && group == Base::Tools::getIdentifier(group);
}

bool DlgAddPropertyVarSet::isTypeValid()
{
    std::string type = ui->comboBoxType->currentText().toStdString();
    return Base::Type::fromName(type.c_str()).isDerivedFrom(App::Property::getClassTypeId());
}

bool DlgAddPropertyVarSet::areFieldsValid()
{
    return isNameValid() && isGroupValid() && isTypeValid();
}

void DlgAddPropertyVarSet::onTextFieldChanged([[maybe_unused]]const QString& text)
{
    setOkEnabled(areFieldsValid());
    showStatusMessage();
}

void DlgAddPropertyVarSet::showStatusMessage()
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

void DlgAddPropertyVarSet::removeEditor()
{
    if (editor == nullptr) {
        return;
    }

    layout()->removeWidget(editor.get());
    QWidget::setTabOrder(ui->comboBoxType, ui->checkBoxAdd);
    editor = nullptr;
}

void DlgAddPropertyVarSet::onTypeChanged(const QString& text)
{
    std::string type = text.toStdString();

    if (Base::Type::fromName(type.c_str()).isBad() || !isTypeWithEditor(type)) {
        propertyItem = nullptr;
        removeEditor();
    }
    else {
        initializeValue();
    }

    setOkEnabled(areFieldsValid());
    showStatusMessage();
}

void DlgAddPropertyVarSet::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        setTitle();
    }
    QDialog::changeEvent(e);
}

void DlgAddPropertyVarSet::valueChanged()
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
void DlgAddPropertyVarSet::openTransaction()
{
    transactionID = App::GetApplication().setActiveTransaction("Add property VarSet");
}

void DlgAddPropertyVarSet::critical(const QString& title, const QString& text) {
    static bool criticalDialogShown = false;
    if (!criticalDialogShown) {
        criticalDialogShown = true;
        QMessageBox::critical(this, title, text);
        criticalDialogShown = false;
    }
}

bool DlgAddPropertyVarSet::createProperty()
{
    std::string name = ui->lineEditName->text().toStdString();
    std::string group = comboBoxGroup.currentText().toStdString();
    std::string type = ui->comboBoxType->currentText().toStdString();
    std::string doc = ui->lineEditToolTip->text().toStdString();

    App::Property* prop = nullptr;
    try {
        prop = varSet->addDynamicProperty(type.c_str(), name.c_str(),
                                          group.c_str(), doc.c_str());
    }
    catch (Base::Exception& e) {
        e.reportException();
        critical(QObject::tr("Add property"),
                 QObject::tr("Failed to add property to '%1': %2").arg(
                         QString::fromLatin1(varSet->getFullName().c_str()),
                         QString::fromUtf8(e.what())));
        return false;
    }

    if (propertyItem) {
        objectIdentifier = std::make_unique<App::ObjectIdentifier>(*prop);
        propertyItem->setPropertyData({prop});
        propertyItem->bind(*objectIdentifier);

        QVariant data = propertyItem->editorData(editor.get());
        propertyItem->setData(data);
    }

    return true;
}

void DlgAddPropertyVarSet::closeTransaction(TransactionOption option)
{
    if (transactionID == 0) {
        return;
    }

    App::GetApplication().closeActiveTransaction(static_cast<bool>(option), transactionID);
    transactionID = 0;
}

void DlgAddPropertyVarSet::clearFields()
{
    {
        QSignalBlocker blocker(ui->lineEditName);
        ui->lineEditName->clear();
    }
    ui->lineEditToolTip->clear();
    initializeValue();
    setOkEnabled(false);
}

void DlgAddPropertyVarSet::accept()
{
    openTransaction();
    if (createProperty()) {
        closeTransaction(TransactionOption::Commit);
        std::string group = comboBoxGroup.currentText().toStdString();
        std::string type = ui->comboBoxType->currentText().toStdString();
        auto paramGroup = App::GetApplication().GetParameterGroupByPath(
                "User parameter:BaseApp/Preferences/PropertyView");
        paramGroup->SetASCII("NewPropertyType", type.c_str());
        paramGroup->SetASCII("NewPropertyGroup", group.c_str());

        if (ui->checkBoxAdd->isChecked()) {
            clearFields();
            ui->lineEditName->setFocus();
        }
        else {
            // we are done, close the dialog
            QDialog::accept();
        }
    }
    else {
        closeTransaction(TransactionOption::Abort);
    }
}

void DlgAddPropertyVarSet::reject()
{
    disconnect(connComboBoxGroup);
    disconnect(connComboBoxType);
    disconnect(connLineEditNameTextChanged);

    QDialog::reject();
}

#include "moc_DlgAddPropertyVarSet.cpp"
