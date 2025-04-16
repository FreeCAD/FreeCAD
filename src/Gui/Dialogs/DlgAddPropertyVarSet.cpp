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

const std::string DlgAddPropertyVarSet::Group_Base = "Base";

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

void DlgAddPropertyVarSet::initializeGroup()
{
    comboBoxGroup.setObjectName(QStringLiteral("comboBoxGroup"));
    comboBoxGroup.setInsertPolicy(QComboBox::InsertAtTop);
    comboBoxGroup.setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    auto formLayout = qobject_cast<QFormLayout*>(layout());
    formLayout->setWidget(1, QFormLayout::FieldRole, &comboBoxGroup);

    std::vector<App::Property*> properties;
    varSet->getPropertyList(properties);

    std::unordered_set<std::string> groupNames;
    for (const auto* prop : properties) {
        const char* groupName = varSet->getPropertyGroup(prop);
        groupNames.insert(groupName ? groupName : Group_Base);
    }

    std::vector<std::string> groupNamesSorted(groupNames.begin(), groupNames.end());
    std::ranges::sort(groupNamesSorted, [](const std::string& a, const std::string& b) {
        // prefer anything else other than Base, so move it to the back
        if (a == Group_Base) {
            return false;
        }
        if (b == Group_Base) {
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
    if(lastType.isBad()) {
        lastType = App::PropertyLength::getClassTypeId();
    }

    std::vector<Base::Type> types = getSupportedTypes();

    for(const auto& type : types) {
        ui->comboBoxType->addItem(QString::fromLatin1(type.getName()));
        if(type == lastType) {
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

/*
// keep some debugging code for debugging tab order
static void printFocusChain(QWidget *widget) {
    FC_ERR("Focus Chain:");
    QWidget* start = widget;
    int i = 0;
    do {
        FC_ERR(" " << widget->objectName().toStdString());
        widget = widget->nextInFocusChain();
        i++;
    } while (widget != nullptr && i < 30 && start != widget);
    QWidget *currentWidget = QApplication::focusWidget();
    FC_ERR("  Current focus widget:" << (currentWidget ? currentWidget->objectName().toStdString() :
"None") << std::endl << std::endl);
}
*/

void DlgAddPropertyVarSet::addEditor(PropertyEditor::PropertyItem* propertyItem)
{
    editor.reset(propertyItem->createEditor(this, [this]() {
        this->valueChanged();
    }, PropertyEditor::FrameOption::WithFrame));
    editor->blockSignals(true);
    propertyItem->setEditorData(
            editor.get(),
            propertyItem->data(PropertyEditor::PropertyItem::ValueColumn, Qt::EditRole));
    editor->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    editor->setObjectName(QStringLiteral("editor"));
    auto formLayout = qobject_cast<QFormLayout*>(layout());
    formLayout->setWidget(3, QFormLayout::FieldRole, editor.get());

    QWidget::setTabOrder(ui->comboBoxType, editor.get());
    QWidget::setTabOrder(editor.get(), ui->checkBoxAdd);

    removeSelectionEditor();
    editor->blockSignals(false);

    // FC_ERR("add editor");
    // printFocusChain(editor.get());
}

bool DlgAddPropertyVarSet::isTypeWithEditor(const Base::Type& type)
{
    static const std::array<Base::Type, 5> subTypesWithEditor = {
        // These types and their subtypes have editors.
        App::PropertyBool::getClassTypeId(),
        App::PropertyColor::getClassTypeId(),
        App::PropertyFileIncluded::getClassTypeId(),
        App::PropertyFloat::getClassTypeId(),
        App::PropertyInteger::getClassTypeId()
    };

    static const std::array<Base::Type, 9> typesWithEditor = {
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

    return std::ranges::find(typesWithEditor, type) != typesWithEditor.end() ||
        std::ranges::any_of(subTypesWithEditor,
                            [&type](const Base::Type& t) {
                                return type.isDerivedFrom(t);
                            });
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
    if (!editor || std::strlen(editor) == 0) {
        return nullptr;
    }
    return static_cast<PropertyEditor::PropertyItem*>(
            PropertyEditor::PropertyItemFactory::instance().createPropertyItem(editor));
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

    // FC_ERR("Initialize widgets");
    // printFocusChain(ui->lineEditName);
}

bool DlgAddPropertyVarSet::propertyExists(const std::string& name)
{
    App::Property* prop = varSet->getPropertyByName(name.c_str());
    return prop && prop->getContainer() == varSet;
}

bool DlgAddPropertyVarSet::isNameValid()
{
    std::string name = ui->lineEditName->text().toStdString();

    return !(name.empty() ||
             name != Base::Tools::getIdentifier(name) ||
             App::ExpressionParser::isTokenAConstant(name) ||
             App::ExpressionParser::isTokenAUnit(name) ||
             propertyExists(name));
}

bool DlgAddPropertyVarSet::isGroupValid()
{
    std::string group = comboBoxGroup.currentText().toStdString();
    return !(group.empty() || group != Base::Tools::getIdentifier(group));
}

bool DlgAddPropertyVarSet::isTypeValid()
{
    std::string type = ui->comboBoxType->currentText().toStdString();

    return !Base::Type::fromName(type.c_str()).isBad();
}

bool DlgAddPropertyVarSet::areFieldsValid()
{
    return isNameValid() && isGroupValid() && isTypeValid();
}

void DlgAddPropertyVarSet::onTextFieldChanged([[maybe_unused]] const QString& text)
{
    setOkEnabled(areFieldsValid());
}

void DlgAddPropertyVarSet::removeEditor()
{
    if (editor) {
        layout()->removeWidget(editor.get());
        QWidget::setTabOrder(ui->comboBoxType, ui->checkBoxAdd);
        editor = nullptr;

        // FC_ERR("remove editor");
        // printFocusChain(ui->comboBoxType);
    }
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
    QVariant data;
    data = propertyItem->editorData(editor.get());
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

    App::Property* prop;
    try {
        prop = varSet->addDynamicProperty(type.c_str(), name.c_str(),
                                          group.c_str(), doc.c_str());
    }
    catch (Base::Exception& e) {
        e.ReportException();
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

        QVariant data;
        data = propertyItem->editorData(editor.get());
        propertyItem->setData(data);
    }

    return true;
}

void DlgAddPropertyVarSet::closeTransaction(TransactionOption option)
{
    if (transactionID != 0) {
        App::GetApplication().closeActiveTransaction(static_cast<bool>(option), transactionID);
        transactionID = 0;
    }
}

void DlgAddPropertyVarSet::clearFields()
{
    bool beforeBlocked = ui->lineEditName->blockSignals(true);
    ui->lineEditName->clear();
    ui->lineEditName->blockSignals(beforeBlocked);
    ui->lineEditToolTip->clear();
    initializeValue();
    setOkEnabled(false);
}

void DlgAddPropertyVarSet::accept()
{
    openTransaction();
    if (createProperty()) {
        closeTransaction(TransactionOption::COMMIT);
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
        closeTransaction(TransactionOption::ABORT);
    }
}

void DlgAddPropertyVarSet::reject()
{
    disconnect(connComboBoxGroup);
    disconnect(connComboBoxType);
    disconnect(connLineEditNameTextChanged);

    QDialog::reject();
}














#if 0









// unused
void DlgAddPropertyVarSet::onNamePropertyChanged([[maybe_unused]] const QString& text)
{
    setOkEnabled(areFieldsValid());

    // if (!namePropertyToAdd.empty() && text.toStdString() != namePropertyToAdd) {
    //     // The user decided to change the name of the property.  This
    //     // invalidates the editor that is strictly associated with the property.
    //     clearCurrentProperty();
    // }
}

// unused

// unused
App::Property* DlgAddPropertyVarSet::getPropertyToAdd() {
    // This function should be called only if it is certain the property exists.
    // It will throw a runtime error if not.
    App::Property* prop = varSet->getPropertyByName(namePropertyToAdd.c_str());
    if (prop == nullptr) {
        FC_THROWM(Base::RuntimeError, "A property with name '" << namePropertyToAdd << "' does not exist.");
    }

    return prop;
}

// unused
void DlgAddPropertyVarSet::changePropertyToAdd() {
    // we were already adding a new property, the only option to get here
    // is a change of type or group.

    std::string name = ui->lineEditName->text().toStdString();
    assert(name == namePropertyToAdd);

    // performs a check for nullptr
    App::Property* prop = getPropertyToAdd();

    std::string group = comboBoxGroup.currentText().toStdString();
    std::string doc = ui->lineEditToolTip->text().toStdString();
    if (prop->getGroup() != group) {
        varSet->changeDynamicProperty(prop, group.c_str(), doc.c_str());
    }

    std::string type = ui->comboBoxType->currentText().toStdString();
    if (prop->getTypeId() != Base::Type::fromName(type.c_str())) {
        // the property should have a different type
        varSet->removeDynamicProperty(namePropertyToAdd.c_str());
        createProperty();
    }
}

// unused
bool DlgAddPropertyVarSet::hasPendingTransaction()
{
    return transactionID != 0;
}


// unused
void DlgAddPropertyVarSet::clearCurrentProperty()
{
    removeEditor();
    varSet->removeDynamicProperty(namePropertyToAdd.c_str());
    if (hasPendingTransaction()) {
        closeTransaction(ABORT);
    }
    setOkEnabled(false);
    namePropertyToAdd.clear();
}

// unused
class CreatePropertyException : public std::exception {
public:
    explicit CreatePropertyException(const std::string& message) : msg(message) {}

    const char* what() const noexcept override {
        return msg.c_str();
    }

private:
    std::string msg;
};

// unused
void DlgAddPropertyVarSet::checkName() {
    std::string name = ui->lineEditName->text().toStdString();
    if(name.empty() || name != Base::Tools::getIdentifier(name)) {
        // QMessageBox::critical(getMainWindow(),
        //                       QObject::tr("Invalid name"),
        //                       QObject::tr("The property name must only contain alpha numericals, "
        //                                   "underscore, and must not start with a digit."));
        //clearEditors(!CLEAR_NAME);
        throw CreatePropertyException("Invalid name");
    }

    if(App::ExpressionParser::isTokenAUnit(name) || App::ExpressionParser::isTokenAConstant(name)) {
        // critical(QObject::tr("Invalid name"),
        //          QObject::tr("The property name is a reserved word."));
        // clearEditors(!CLEAR_NAME);
        throw CreatePropertyException("Invalid name");
    }

    // if (namePropertyToAdd.empty()) {
    //     // we are adding a new property, check whether it doesn't already exist
    //     auto prop = varSet->getPropertyByName(name.c_str());
    //     if(prop && prop->getContainer() == varSet) {
    //         critical(QObject::tr("Invalid name"),
    //                  QObject::tr("The property '%1' already exists in '%2'").arg(
    //                          QString::fromLatin1(name.c_str()),
    //                          QString::fromLatin1(varSet->getFullName().c_str())));
    //         clearEditors(!CLEAR_NAME);
    //         throw CreatePropertyException("Invalid name");
    //     }
    // }
}

// unused
void DlgAddPropertyVarSet::checkGroup() {
    std::string group = comboBoxGroup.currentText().toStdString();
    if (group.empty() || group != Base::Tools::getIdentifier(group)) {
        // critical(QObject::tr("Invalid name"),
        //          QObject::tr("The group name must only contain alpha numericals,\n"
        //                      "underscore, and must not start with a digit."));
        comboBoxGroup.setEditText(QStringLiteral("Base"));
        throw CreatePropertyException("Invalid name");
    }
}

// unused
void DlgAddPropertyVarSet::checkType() {
    std::string type = ui->comboBoxType->currentText().toStdString();

    if (Base::Type::fromName(type.c_str()).isBad()) {
        throw CreatePropertyException("Invalid name");
    }
}

// void DlgAddPropertyVarSet::onChanged()
// {
//     // try {
//     //     checkName();
//     //     checkGroup();
//     //     checkType();
//     //     // no check for tooltip, we accept any string
//     // }
//     // catch (const CreatePropertyException&) {
//     //     if (!namePropertyToAdd.empty()) {
//     //         clearCurrentProperty();
//     //     }
//     //     return;
//     // }
    
// }

// unused
void DlgAddPropertyVarSet::onEditFinished() {
    /* The editor for the value is dynamically created if 1) the name has been
     * determined and 2) if the type of the property has been determined.  The
     * group of the property is important too, but it is not essential, because
     * we can change the group after the property has been created.
     *
     * In this function we check whether we can create a property and therefore
     * an editor.
     */

    // try {
    //     checkName();
    //     checkGroup();
    //     checkType();
    //     // no check for tooltip, we accept any string
    // }
    // catch (const CreatePropertyException&) {
    //     if (!namePropertyToAdd.empty()) {
    //         clearCurrentProperty();
    //     }
    //     return;
    // }

    // if (namePropertyToAdd.empty()) {
    //     // we are adding a new property
    //     openTransaction();
    //     createProperty();
    // }
    // else {
    //     // we were already adding a new property that should now be changed
    //     changePropertyToAdd();
    // }
}

// unused
void DlgAddPropertyVarSet::addDocumentation() {
    /* Add the documentation to an existing property.
     * Note that this method assumes the property exists.
     *
     * Since there is no check on documentation (we accept any string), there
     * is no signal handler for the documentation field.  This method updates
     * the property that is being added with the text inserted as
     * documentation/tooltip.
     *
     * This function should be called at a late stage, before doing the accept.
     */

    std::string group = comboBoxGroup.currentText().toStdString();
    std::string doc = ui->lineEditToolTip->text().toStdString();

    // performs a check for nullptr
    App::Property* prop = getPropertyToAdd();
    varSet->changeDynamicProperty(prop, group.c_str(), doc.c_str());
}


#endif

#include "moc_DlgAddPropertyVarSet.cpp"
