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
#include <App/PropertyUnits.h>
#include <Base/Tools.h>

#include "DlgAddPropertyVarSet.h"
#include "ui_DlgAddPropertyVarSet.h"
#include "MainWindow.h"
#include "ViewProviderDocumentObject.h"
#include "ViewProviderVarSet.h"

FC_LOG_LEVEL_INIT("DlgAddPropertyVarSet", true, true)

using namespace Gui;
using namespace Gui::Dialog;

const std::string DlgAddPropertyVarSet::GROUP_BASE = "Base";

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
    connect(&comboBoxGroup, &EditFinishedComboBox::editFinished,
            this, &DlgAddPropertyVarSet::onGroupDetermined);
    comboBoxGroup.setObjectName(QString::fromUtf8("comboBoxGroup"));
    comboBoxGroup.setInsertPolicy(QComboBox::InsertAtTop);
    comboBoxGroup.setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    comboBoxGroup.setEditable(true);
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
    Base::Type::getAllDerivedFrom(Base::Type::fromName("App::Property"),types);
    std::sort(types.begin(), types.end(), [](Base::Type a, Base::Type b) { return strcmp(a.getName(), b.getName()) < 0; });

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

    connect(ui->comboBoxType, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &DlgAddPropertyVarSet::onTypePropertyDetermined);
}

/*
// keep some debugging code for debugging tab order
static void printFocusChain(QWidget *widget) {
    FC_ERR("Focus Chain:");
    QWidget* start = widget;
    int i = 0;
    do {
        FC_ERR(" " << widget->objectName().toUtf8().constData());
        widget = widget->nextInFocusChain();
        i++;
    } while (widget != nullptr && i < 30 && start != widget);
    QWidget *currentWidget = QApplication::focusWidget();
    FC_ERR("  Current focus widget:" << (currentWidget ? currentWidget->objectName().toUtf8().constData() : "None") << std::endl << std::endl);
}
*/

void DlgAddPropertyVarSet::initializeWidgets(ViewProviderVarSet* viewProvider)
{
    initializeGroup();
    initializeTypes();

    connect(this, &QDialog::finished,
            this, [viewProvider](int result) { viewProvider->onFinished(result); });
    connect(ui->lineEditName, &QLineEdit::editingFinished,
            this, &DlgAddPropertyVarSet::onNamePropertyDetermined);

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

void DlgAddPropertyVarSet::clearEditors()
{
    ui->lineEditName->clear();
    removeEditor();
    setOkEnabled(false);
    namePropertyToAdd.clear();
    editor = nullptr;
}

void DlgAddPropertyVarSet::removeEditor()
{
    if (editor) {
        layout()->removeWidget(editor.get());
        QWidget::setTabOrder(ui->comboBoxType, ui->checkBoxAdd);

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

void DlgAddPropertyVarSet::addEditor(PropertyEditor::PropertyItem* propertyItem, std::string& /*type*/)
{
    editor.reset(propertyItem->createEditor(this, this, SLOT(valueChanged())));
    editor->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    editor->setObjectName(QString::fromUtf8("editor"));
    auto formLayout = qobject_cast<QFormLayout*>(layout());
    formLayout->setWidget(3, QFormLayout::FieldRole, editor.get());

    QWidget::setTabOrder(ui->comboBoxType, editor.get());
    QWidget::setTabOrder(editor.get(), ui->checkBoxAdd);

    // FC_ERR("add editor");
    // printFocusChain(editor.get());
}

bool DlgAddPropertyVarSet::isSupportedType(std::string& type)
{
    return unsupportedTypes.find(type) == unsupportedTypes.end();
}

void DlgAddPropertyVarSet::createProperty(std::string& name, std::string& group)
{
    std::string type = ui->comboBoxType->currentText().toUtf8().constData();

    App::Property* prop;
    try {
        prop = varSet->addDynamicProperty(type.c_str(), name.c_str(),
                                          group.c_str());
    }
    catch (Base::Exception& e) {
        e.ReportException();
        QMessageBox::critical(this,
                              QObject::tr("Add property"),
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
    if (propertyItem && isSupportedType(type)) {
        propertyItem->setPropertyData({prop});
        propertyItem->bind(*objectIdentifier);
             addEditor(propertyItem.get(), type);
    }

    setOkEnabled(true);
}

void DlgAddPropertyVarSet::onNamePropertyDetermined()
{
    if (!namePropertyToAdd.empty()) {
        // we were already adding a name, so remove that property
        varSet->removeDynamicProperty(namePropertyToAdd.c_str());
    }
    QString nameProperty = ui->lineEditName->text();
    std::string name = nameProperty.toUtf8().constData();
    std::string group = comboBoxGroup.currentText().toUtf8().constData();
    if(name.empty() || group.empty()
       || name != Base::Tools::getIdentifier(name)
       || group != Base::Tools::getIdentifier(group)) {
        QMessageBox::critical(getMainWindow(),
                              QObject::tr("Invalid name"),
                              QObject::tr("The property name or group name must only contain alpha numericals,\n"
                                          "underscore, and must not start with a digit."));
        clearEditors();
        return;
    }

    auto prop = varSet->getPropertyByName(name.c_str());
    if(prop && prop->getContainer() == varSet) {
        QMessageBox::critical(this,
                              QObject::tr("Invalid name"),
                              QObject::tr("The property '%1' already exists in '%2'").arg(
                                      QString::fromLatin1(name.c_str()),
                                      QString::fromLatin1(varSet->getFullName().c_str())));
        clearEditors();
        return;
    }

    App::Document* doc = varSet->getDocument();
    doc->openTransaction("Add property VarSet");
    createProperty(name, group);

    // FC_ERR("chain onNameDetermined");
    // printFocusChain(ui->lineEditName);
}

void DlgAddPropertyVarSet::onGroupDetermined()
{
    std::string group = comboBoxGroup.currentText().toUtf8().constData();

    if (group.empty() || group != Base::Tools::getIdentifier(group)) {
        QMessageBox::critical(this,
            QObject::tr("Invalid name"),
            QObject::tr("The group name must only contain alpha numericals,\n"
                        "underscore, and must not start with a digit."));
        comboBoxGroup.setEditText(QString::fromUtf8("Base"));
        return;
    }

    if (!namePropertyToAdd.empty()) {
        // we were already adding a property
        App::Property* prop = varSet->getPropertyByName(namePropertyToAdd.c_str());
        if (prop->getGroup() != group) {
            varSet->changeDynamicProperty(prop, group.c_str(), nullptr);
        }
    }

    // FC_ERR("chain onGroupDetermined");
    // printFocusChain(&comboBoxGroup);
    ui->comboBoxType->setFocus();
}

void DlgAddPropertyVarSet::onTypePropertyDetermined()
{
    std::string type = ui->comboBoxType->currentText().toUtf8().constData();

    if (!namePropertyToAdd.empty()) {
        // we were already adding a name, so check this property
        App::Property* prop = varSet->getPropertyByName(namePropertyToAdd.c_str());

        if (prop->getTypeId() != Base::Type::fromName(type.c_str())) {
            // the property should have a different type
            std::string group = prop->getGroup();
            varSet->removeDynamicProperty(namePropertyToAdd.c_str());
            createProperty(namePropertyToAdd, group);
        }
    }
}

void DlgAddPropertyVarSet::valueChanged()
{
    QWidget* editor = qobject_cast<QWidget*>(sender());
    QVariant data;
    data = propertyItem->editorData(editor);

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

    std::string group = comboBoxGroup.currentText().toUtf8().constData();
    std::string type = ui->comboBoxType->currentText().toUtf8().constData();
    auto paramGroup = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/PropertyView");
    paramGroup->SetASCII("NewPropertyType", type.c_str());
    paramGroup->SetASCII("NewPropertyGroup", group.c_str());
    QDialog::accept();
}

void DlgAddPropertyVarSet::reject()
{
    App::Document* doc = varSet->getDocument();
    // a transaction is not pending if a name has not been determined.
    if (doc->hasPendingTransaction()) {
        doc->abortTransaction();
    }
    QDialog::reject();
}


#include "moc_DlgAddPropertyVarSet.cpp"
