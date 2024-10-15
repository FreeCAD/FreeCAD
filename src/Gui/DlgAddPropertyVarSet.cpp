/****************************************************************************
 *   Copyright (c) 2024 Ondsel <development@ondsel.com>                     *
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
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/ExpressionParser.h>
#include <App/PropertyUnits.h>
#include <Base/Tools.h>

#include "DlgAddPropertyVarSet.h"
#include "ui_DlgAddPropertyVarSet.h"
#include "MainWindow.h"
#include "ViewProviderVarSet.h"

FC_LOG_LEVEL_INIT("DlgAddPropertyVarSet", true, true)

using namespace Gui;
using namespace Gui::Dialog;

const std::string DlgAddPropertyVarSet::GROUP_BASE = "Base";

const bool CLEAR_NAME = true;

DlgAddPropertyVarSet::DlgAddPropertyVarSet(QWidget* parent,
                                           ViewProviderVarSet* viewProvider)
    : QDialog(parent),
      varSet(dynamic_cast<App::VarSet*>(viewProvider->getObject())),
      ui(new Ui_DlgAddPropertyVarSet),
      comboBoxGroup(this),
      completerType(this),
      editor(nullptr)
{
    ui->setupUi(this);

    initializeWidgets(viewProvider);
}

DlgAddPropertyVarSet::~DlgAddPropertyVarSet() = default;

void DlgAddPropertyVarSet::initializeGroup()
{
    comboBoxGroup.setObjectName(QString::fromUtf8("comboBoxGroup"));
    comboBoxGroup.setInsertPolicy(QComboBox::InsertAtTop);
    comboBoxGroup.setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    auto formLayout = qobject_cast<QFormLayout*>(layout());
    formLayout->setWidget(1, QFormLayout::FieldRole, &comboBoxGroup);

    std::vector<App::Property*> properties;
    varSet->getPropertyList(properties);
    std::unordered_set<std::string> groupNames;
    for (auto prop : properties) {
        const char* groupName = varSet->getPropertyGroup(prop);
        groupNames.insert(groupName ? groupName : GROUP_BASE);
    }
    std::vector<std::string> groupNamesSorted(groupNames.begin(), groupNames.end());
    std::sort(groupNamesSorted.begin(), groupNamesSorted.end(), [](std::string& a, std::string& b) {
        // prefer anything else other than Base, so move it to the back
        if (a == GROUP_BASE) {
            return false;
        }
        else if (b == GROUP_BASE) {
            return true;
        }
        else {
            return a < b;
        }
    });

    for (const auto& groupName : groupNamesSorted) {
        comboBoxGroup.addItem(QString::fromStdString(groupName));
    }

    comboBoxGroup.setEditText(QString::fromStdString(groupNamesSorted[0]));
    connComboBoxGroup = connect(&comboBoxGroup, &EditFinishedComboBox::editFinished,
                                this, &DlgAddPropertyVarSet::onEditFinished);
}

void DlgAddPropertyVarSet::getSupportedTypes(std::vector<Base::Type>& types)
{
    std::vector<Base::Type> proptypes;
    Base::Type::getAllDerivedFrom(Base::Type::fromName("App::Property"), proptypes);
    std::copy_if(proptypes.begin(), proptypes.end(), std::back_inserter(types), [](const Base::Type& type) {
        return type.canInstantiate();
    });
    std::sort(types.begin(), types.end(), [](Base::Type a, Base::Type b) {
        return strcmp(a.getName(), b.getName()) < 0;
    });
}

void DlgAddPropertyVarSet::initializeTypes()
{
    auto paramGroup = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/PropertyView");
    auto lastType = Base::Type::fromName(
            paramGroup->GetASCII("NewPropertyType","App::PropertyLength").c_str());
    if(lastType.isBad()) {
        lastType = App::PropertyLength::getClassTypeId();
    }

    std::vector<Base::Type> types;
    getSupportedTypes(types);

    for(const auto& type : types) {
        ui->comboBoxType->addItem(QString::fromLatin1(type.getName()));
        if(type == lastType)
            ui->comboBoxType->setCurrentIndex(ui->comboBoxType->count()-1);
    }

    completerType.setModel(ui->comboBoxType->model());
    completerType.setCaseSensitivity(Qt::CaseInsensitive);
    completerType.setFilterMode(Qt::MatchContains);
    ui->comboBoxType->setCompleter(&completerType);
    ui->comboBoxType->setInsertPolicy(QComboBox::NoInsert);

    connComboBoxType = connect(ui->comboBoxType, &QComboBox::currentTextChanged,
                               this, &DlgAddPropertyVarSet::onEditFinished);
}

/*
// keep some debugging code for debugging tab order
static void printFocusChain(QWidget *widget) {
    FC_ERR("Focus Chain:");
    QWidget* start = widget;
    int i = 0;
    do {
        FC_ERR(" " << widget->objectName().toStdString();
        widget = widget->nextInFocusChain();
        i++;
    } while (widget != nullptr && i < 30 && start != widget);
    QWidget *currentWidget = QApplication::focusWidget();
    FC_ERR("  Current focus widget:" << (currentWidget ? currentWidget->objectName().toStdString() : "None") << std::endl << std::endl);
}
*/

void DlgAddPropertyVarSet::initializeWidgets(ViewProviderVarSet* viewProvider)
{
    initializeGroup();
    initializeTypes();

    connect(this, &QDialog::finished,
            this, [viewProvider](int result) { viewProvider->onFinished(result); });
    connLineEditNameEditFinished = connect(ui->lineEditName, &QLineEdit::editingFinished,
                                           this, &DlgAddPropertyVarSet::onEditFinished);
    connLineEditNameTextChanged = connect(ui->lineEditName, &QLineEdit::textChanged,
            this, &DlgAddPropertyVarSet::onNamePropertyChanged);

    std::string title = "Add a property to " + varSet->getFullName();
    setWindowTitle(QString::fromStdString(title));

    setOkEnabled(false);

    ui->lineEditName->setFocus();

    QWidget::setTabOrder(ui->lineEditName, &comboBoxGroup);
    QWidget::setTabOrder(&comboBoxGroup, ui->comboBoxType);

    // FC_ERR("Initialize widgets");
    // printFocusChain(ui->lineEditName);
}

void DlgAddPropertyVarSet::setOkEnabled(bool enabled)
{
    QPushButton *okButton = ui->buttonBox->button(QDialogButtonBox::Ok);
    okButton->setEnabled(enabled);
}

void DlgAddPropertyVarSet::clearEditors(bool clearName)
{
    if (clearName) {
        bool beforeBlocked = ui->lineEditName->blockSignals(true);
        ui->lineEditName->clear();
        ui->lineEditName->blockSignals(beforeBlocked);
    }
    removeEditor();
    setOkEnabled(false);
    namePropertyToAdd.clear();
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

static PropertyEditor::PropertyItem *createPropertyItem(App::Property *prop)
{
    const char *editor = prop->getEditorName();
    if (!editor || !editor[0]) {
        return nullptr;
    }
    auto item = static_cast<PropertyEditor::PropertyItem*>(
            PropertyEditor::PropertyItemFactory::instance().createPropertyItem(editor));
    if (!item) {
        qWarning("No property item for type %s found\n", editor);
    }
    return item;
}

void DlgAddPropertyVarSet::addEditor(PropertyEditor::PropertyItem* propertyItem, std::string& type)
{
    editor.reset(propertyItem->createEditor(this, [this]() {
        this->valueChanged();
    }));
    if (type == "App::PropertyFont") {
        propertyItem->setEditorData(editor.get(), QVariant());
    }
    editor->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    editor->setObjectName(QString::fromUtf8("editor"));
    auto formLayout = qobject_cast<QFormLayout*>(layout());
    formLayout->setWidget(3, QFormLayout::FieldRole, editor.get());

    QWidget::setTabOrder(ui->comboBoxType, editor.get());
    QWidget::setTabOrder(editor.get(), ui->checkBoxAdd);

    // FC_ERR("add editor");
    // printFocusChain(editor.get());
}

bool DlgAddPropertyVarSet::isTypeWithEditor(const std::string& type)
{
    return typesWithoutEditor.find(type) == typesWithoutEditor.end();
}

void DlgAddPropertyVarSet::createProperty()
{
    std::string name = ui->lineEditName->text().toStdString();
    std::string group = comboBoxGroup.currentText().toStdString();
    std::string type = ui->comboBoxType->currentText().toStdString();

    App::Property* prop;
    try {
        prop = varSet->addDynamicProperty(type.c_str(), name.c_str(),
                                          group.c_str());
    }
    catch (Base::Exception& e) {
        e.ReportException();
        critical(QObject::tr("Add property"),
                 QObject::tr("Failed to add property to '%1': %2").arg(
                         QString::fromLatin1(varSet->getFullName().c_str()),
                         QString::fromUtf8(e.what())));
        clearEditors();
        return;
    }

    namePropertyToAdd = name;

    objectIdentifier = std::make_unique<App::ObjectIdentifier>(*prop);
    // creating a propertyItem here because it has all kinds of logic for
    // editors that we can reuse
    removeEditor();
    propertyItem.reset(createPropertyItem(prop));
    if (propertyItem && isTypeWithEditor(type)) {
        propertyItem->setPropertyData({prop});
        propertyItem->bind(*objectIdentifier);
        addEditor(propertyItem.get(), type);
    }

    setOkEnabled(true);
}

void DlgAddPropertyVarSet::changePropertyToAdd() {
    // we were already adding a new property, the only option to get here
    // is a change of type or group.

    std::string name = ui->lineEditName->text().toStdString();
    assert(name == namePropertyToAdd);

    App::Property* prop = varSet->getPropertyByName(namePropertyToAdd.c_str());
    if (prop == nullptr) {
        // this should not happen because this method assumes the property exists
        FC_THROWM(Base::RuntimeError, "A property with name '" << name << "' does not exist.");
    }

    std::string group = comboBoxGroup.currentText().toStdString();
    if (prop->getGroup() != group) {
        varSet->changeDynamicProperty(prop, group.c_str(), nullptr);
    }

    std::string type = ui->comboBoxType->currentText().toStdString();
    if (prop->getTypeId() != Base::Type::fromName(type.c_str())) {
        // the property should have a different type
        varSet->removeDynamicProperty(namePropertyToAdd.c_str());
        createProperty();
    }
}


void DlgAddPropertyVarSet::clearCurrentProperty()
{
    removeEditor();
    varSet->removeDynamicProperty(namePropertyToAdd.c_str());
    App::Document* doc = varSet->getDocument();
    if (doc->hasPendingTransaction()) {
        doc->abortTransaction();
    }
    setOkEnabled(false);
    namePropertyToAdd.clear();
}

class CreatePropertyException : public std::exception {
public:
    explicit CreatePropertyException(const std::string& message) : msg(message) {}

    const char* what() const noexcept override {
        return msg.c_str();
    }

private:
    std::string msg;
};

void DlgAddPropertyVarSet::checkName() {
    std::string name = ui->lineEditName->text().toStdString();
    if(name.empty() || name != Base::Tools::getIdentifier(name)) {
        critical(QObject::tr("Invalid name"),
                 QObject::tr("The property name must only contain alpha numericals,\n"
                             "underscore, and must not start with a digit."));
        clearEditors(!CLEAR_NAME);
        throw CreatePropertyException("Invalid name");
    }

    if(App::ExpressionParser::isTokenAUnit(name) || App::ExpressionParser::isTokenAConstant(name)) {
        critical(QObject::tr("Invalid name"),
                 QObject::tr("The property name is a reserved word."));
        clearEditors(!CLEAR_NAME);
        throw CreatePropertyException("Invalid name");
    }

    if (namePropertyToAdd.empty()) {
        // we are adding a new property, check whether it doesn't already exist
        auto prop = varSet->getPropertyByName(name.c_str());
        if(prop && prop->getContainer() == varSet) {
            critical(QObject::tr("Invalid name"),
                     QObject::tr("The property '%1' already exists in '%2'").arg(
                             QString::fromLatin1(name.c_str()),
                             QString::fromLatin1(varSet->getFullName().c_str())));
            clearEditors(!CLEAR_NAME);
            throw CreatePropertyException("Invalid name");
        }
    }
}

void DlgAddPropertyVarSet::checkGroup() {
    std::string group = comboBoxGroup.currentText().toStdString();

    if (group.empty() || group != Base::Tools::getIdentifier(group)) {
        critical(QObject::tr("Invalid name"),
                 QObject::tr("The group name must only contain alpha numericals,\n"
                             "underscore, and must not start with a digit."));
        comboBoxGroup.setEditText(QString::fromUtf8("Base"));
        throw CreatePropertyException("Invalid name");
    }
}

void DlgAddPropertyVarSet::checkType() {
    std::string type = ui->comboBoxType->currentText().toStdString();

    if (Base::Type::fromName(type.c_str()) == Base::Type::badType()) {
        throw CreatePropertyException("Invalid name");
    }
}

void DlgAddPropertyVarSet::onEditFinished() {
    /* The editor for the value is dynamically created if 1) the name has been
     * determined and 2) if the type of the property has been determined.  The
     * group of the property is important too, but it is not essential, because
     * we can change the group after the property has been created.
     *
     * In this function we check whether we can create a property and therefore
     * an editor.
     */

    try {
        checkName();
        checkGroup();
        checkType();
    }
    catch (const CreatePropertyException&) {
        if (!namePropertyToAdd.empty()) {
            clearCurrentProperty();
        }
        return;
    }

    if (namePropertyToAdd.empty()) {
        // we are adding a new property
        App::Document* doc = varSet->getDocument();
        doc->openTransaction("Add property VarSet");
        createProperty();
    }
    else {
        // we were already adding a new property that should now be changed
        changePropertyToAdd();
    }
}

void DlgAddPropertyVarSet::onNamePropertyChanged(const QString& text)
{
    if (!namePropertyToAdd.empty() && text.toStdString() != namePropertyToAdd) {
        // The user decided to change the name of the property.  This
        // invalidates the editor that is strictly associated with the property.
        clearCurrentProperty();
    }
}

void DlgAddPropertyVarSet::critical(const QString& title, const QString& text) {
    static bool criticalDialogShown = false;
    if (!criticalDialogShown) {
        criticalDialogShown = true;
        QMessageBox::critical(this, title, text);
        criticalDialogShown = false;
    }
}

void DlgAddPropertyVarSet::valueChanged()
{
    QVariant data;
    data = propertyItem->editorData(editor.get());
    propertyItem->setData(data);
}

void DlgAddPropertyVarSet::accept()
{
    App::Document* doc = varSet->getDocument();
    doc->commitTransaction();

    if (ui->checkBoxAdd->isChecked()) {
        clearEditors();
        doc->openTransaction();
        ui->lineEditName->setFocus();
        return;
    }

    std::string group = comboBoxGroup.currentText().toStdString();
    std::string type = ui->comboBoxType->currentText().toStdString();
    auto paramGroup = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/PropertyView");
    paramGroup->SetASCII("NewPropertyType", type.c_str());
    paramGroup->SetASCII("NewPropertyGroup", group.c_str());
    QDialog::accept();
}

void DlgAddPropertyVarSet::reject()
{
    App::Document* doc = varSet->getDocument();
    // On reject we can disconnect the signal handlers because nothing useful
    // is to be done.  Otherwise, signals may activate the handlers that assume
    // that a new property has been created, an assumption that will be
    // violated by aborting the transaction because it will remove the newly
    // created property.
    disconnect(connComboBoxGroup);
    disconnect(connComboBoxType);
    disconnect(connLineEditNameEditFinished);
    disconnect(connLineEditNameTextChanged);

    // a transaction is not pending if a name has not been determined.
    if (doc->hasPendingTransaction()) {
        doc->abortTransaction();
    }
    QDialog::reject();
}


#include "moc_DlgAddPropertyVarSet.cpp"
