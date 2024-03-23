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
# include <Inventor/nodes/SoSeparator.h>
# include <QHeaderView>
# include <QTextStream>
# include <QFileSystemModel>
# include <QCompleter>
# include <QMessageBox>
# include <QValidator>
#include <memory>
#endif

#include <Base/Tools.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>

#include "BitmapFactory.h"
#include "PropertyManager.h"
#include "ui_PropertyManager.h"
#include "BitmapFactory.h"

#include "propertymanager/PropertyEditor2.h"


FC_LOG_LEVEL_INIT("PropertyManager", true, true)

using namespace Gui::Dialog;
using namespace Gui::PropertyEditor;

static ParameterGrp::handle _GetParam() {
    static ParameterGrp::handle hGrp;
    if(!hGrp) {
        hGrp = App::GetApplication().GetParameterGroupByPath(
                "User parameter:BaseApp/Preferences/PropertyView");
    }
    return hGrp;
}



std::unique_ptr<QPixmap>  DlgPropertyManager::documentPixmap;
std::unique_ptr<QPixmap>  DlgPropertyManager::documentPartialPixmap;

DlgPropertyManager::DlgPropertyManager(QWidget* parent, Qt::WindowFlags fl)
    : QDialog(parent, fl), ui(new Ui_PropertyManager()),
      comboBoxGroupAdd(this),
      completerDocument(ui->comboBoxDocument),
      completerTypeObject(ui->comboBoxTypeObject),
      completerNameObject(ui->comboBoxNameObject),
      completerGroup(ui->comboBoxGroup),
      completerTypeProperty(ui->comboBoxTypeProp),
      completerNameProperty(ui->comboBoxNameProp),
      editor(nullptr)
{
    ui->setupUi(this);
    setWindowTitle(tr("Property Manager"));

    setupWidgets();

    setupPropertyEditor();

    initializeFilters();

    if (!documentPixmap) {
        documentPixmap = std::make_unique<QPixmap>(Gui::BitmapFactory().pixmap("Document"));
        QIcon icon(*documentPixmap);
        documentPartialPixmap = std::make_unique<QPixmap>(icon.pixmap(documentPixmap->size(), QIcon::Disabled));
    }
    setupScopes();

    initializePropertyAdd();
    showAddPropertyPanel();
}

void DlgPropertyManager::setupScopes()
{
    auto model = getModel();
    QComboBox* comboBox = ui->comboBoxNameScope;
    connect(comboBox, &QComboBox::textActivated,
            this, &DlgPropertyManager::onChoosingNameScope);

   comboBox->blockSignals(true);
   comboBox->clear();
   UniqueVector<QString> names = model->getNamesScope();
   for (const auto& name : names) {
        comboBox->addItem(name);
    }
   comboBox->blockSignals(false);
}

void DlgPropertyManager::setupWidgets()
{

    connect(&comboBoxGroupAdd, &EditFinishedComboBox::editFinished,
            this, &DlgPropertyManager::onGroupDetermined);
    comboBoxGroupAdd.setInsertPolicy(QComboBox::InsertAtTop);
    comboBoxGroupAdd.setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    comboBoxGroupAdd.setEditable(true);
    auto gridLayout = qobject_cast<QGridLayout*>(ui->groupBoxAddProperty->layout());
    gridLayout->addWidget(&comboBoxGroupAdd, 1, 1);

    ui->comboBoxTypePropertyAdd->setInsertPolicy(QComboBox::NoInsert);

    // can't get this to work properly
    // QWidget::setTabOrder(ui->lineEditNamePropertyAdd, &comboBoxGroupAdd);
    // QWidget::setTabOrder(&comboBoxGroupAdd, ui->comboBoxTypePropertyAdd);
    
    connect(ui->refreshButton, &QPushButton::clicked,
            this, &DlgPropertyManager::onRefreshButtonClicked);
    connect(ui->lineEditNamePropertyAdd, &QLineEdit::editingFinished,
            this, &DlgPropertyManager::onNamePropertyDetermined);
    connect(ui->comboBoxTypePropertyAdd, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &DlgPropertyManager::onTypePropertyDetermined);


}

void DlgPropertyManager::initializePropertyAdd()
{
    completerGroupAdd.setModel(comboBoxGroupAdd.model());
    completerGroupAdd.setCaseSensitivity(Qt::CaseInsensitive);
    completerGroupAdd.setFilterMode(Qt::MatchContains);
    comboBoxGroupAdd.setCompleter(&completerGroupAdd);

    completerTypePropertyAdd.setModel(ui->comboBoxTypePropertyAdd->model());
    completerTypePropertyAdd.setCaseSensitivity(Qt::CaseInsensitive);
    completerTypePropertyAdd.setFilterMode(Qt::MatchContains);
    ui->comboBoxTypePropertyAdd->setCompleter(&completerTypePropertyAdd);
    
    auto hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/PropertyView");
    auto defType = Base::Type::fromName(
            hGrp->GetASCII("NewPropertyType","App::PropertyString").c_str());
    if(defType.isBad())
        defType = App::PropertyString::getClassTypeId();

    std::vector<Base::Type> types;
    Base::Type::getAllDerivedFrom(Base::Type::fromName("App::Property"),types);
    std::sort(types.begin(), types.end(), [](Base::Type a, Base::Type b) { return strcmp(a.getName(), b.getName()) < 0; });

    for(const auto& type : types) {
        ui->comboBoxTypePropertyAdd->addItem(QString::fromLatin1(type.getName()));
        if (type == defType) {
            ui->comboBoxTypePropertyAdd->setCurrentIndex(ui->comboBoxTypePropertyAdd->count()-1);
        }
    }

    ui->buttonAddProperty->setEnabled(false);
    connect(ui->buttonAddProperty, &QPushButton::clicked, this, &DlgPropertyManager::onAddProperty);
    
}

void DlgPropertyManager::initializeFilter(QComboBox* comboBox, QCompleter &completer)
                                          
{
    completer.setModel(comboBox->model());
    completer.setCaseSensitivity(Qt::CaseInsensitive);
    completer.setFilterMode(Qt::MatchContains);
    comboBox->setCompleter(&completer);
    comboBox->setInsertPolicy(QComboBox::NoInsert);
    connect(comboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, &DlgPropertyManager::filterSelected);
}

void DlgPropertyManager::initializeFilters()
{
    initializeFilter(ui->comboBoxDocument, completerDocument);
    initializeFilter(ui->comboBoxTypeObject, completerTypeObject);
    initializeFilter(ui->comboBoxNameObject, completerNameObject);
    initializeFilter(ui->comboBoxGroup, completerGroup);
    initializeFilter(ui->comboBoxTypeProp, completerTypeProperty);
    initializeFilter(ui->comboBoxNameProp, completerNameProperty);

    // doesn't work
    // setFirstInTabOrder(ui->comboBoxDocument);
    // setNextInTabOrder(ui->comboBoxTypeObject);
    // setNextInTabOrder(ui->comboBoxNameObject);
    // setNextInTabOrder(ui->comboBoxGroup);
    // setNextInTabOrder(ui->comboBoxTypeProp);
    // setNextInTabOrder(ui->comboBoxNameProp);
    // setFirstInTabOrder(nullptr);

    setupFilters();
}

void DlgPropertyManager::removeEditor()
{
    if (editor) {
        ui->groupBoxAddProperty->layout()->removeWidget(editor.get());
    }
}

void DlgPropertyManager::addEditor(PropertyItem* propertyItem, std::string& type)
{
    editor.reset(propertyItem->createEditor(this, this, SLOT(valueChanged())));
    editor->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    auto gridLayout = qobject_cast<QGridLayout*>(ui->groupBoxAddProperty->layout());
    gridLayout->addWidget(editor.get(), 1, 3);

    // doesn't work as I want
    QWidget::setTabOrder(ui->lineEditNamePropertyAdd, &comboBoxGroupAdd);
    QWidget::setTabOrder(&comboBoxGroupAdd, ui->comboBoxTypePropertyAdd);
    QWidget::setTabOrder(ui->comboBoxTypePropertyAdd, editor.get());
    QWidget::setTabOrder(editor.get(), ui->buttonAddProperty);
}

/*
 *  Destroys the object and frees any allocated resources
 */
DlgPropertyManager::~DlgPropertyManager()
{
    // no need to delete child widgets, Qt does it all for us
    delete ui;
}


void DlgPropertyManager::setupPropertyEditor()
{
    propertyEditor = new Gui::PropertyEditor::PropertyEditor2();
    propertyEditor->setObjectName(QStringLiteral("propertyEditor"));
    propertyEditor->setAutomaticDocumentUpdate(_GetParam()->GetBool("AutoTransactionView", false));
    propertyEditor->setAutomaticExpand(_GetParam()->GetBool("AutoExpandView", false));
    propertyEditor->init();

    connect(getModel(), &QAbstractItemModel::modelReset,
            this, &DlgPropertyManager::onModelReset);
    
    ui->gridLayout->addWidget(propertyEditor);
}

PropertyManagerModel* DlgPropertyManager::getModel()
{
    return static_cast<PropertyManagerModel*>(propertyEditor->model());
}


void DlgPropertyManager::setupFilter(QComboBox* comboBox,
                                     std::function<UniqueVector<QString>(void)> getFilterValuesFunc)
{
    comboBox->blockSignals(true);
    comboBox->clear();
    UniqueVector<QString> filterValues = getFilterValuesFunc();
    for (auto filterValue : filterValues) {
        comboBox->addItem(filterValue);
    }
    comboBox->blockSignals(false);
}

void DlgPropertyManager::setupFilters()
{
    auto *model = getModel();
    FC_ERR("setupFilters");

    setupFilter(ui->comboBoxDocument,
                [&model]() {
                    return model->getFilterValuesDocument();
                });
    setupFilter(ui->comboBoxTypeObject,
                [&model]() {
                    return model->getFilterValuesTypeObject();
                });
    setupFilter(ui->comboBoxNameObject,
                [&model]() {
                    return model->getFilterValuesNameObject();
                });
    setupFilter(ui->comboBoxGroup,
                [&model]() {
                    return model->getFilterValuesGroup();
                });
    setupFilter(ui->comboBoxTypeProp,
                [&model]() {
                    return model->getFilterValuesTypeProperty();
                });
    setupFilter(ui->comboBoxNameProp,
                [&model]() {
                    return model->getFilterValuesNameProperty();
                });
}

void DlgPropertyManager::filterSelected(int index)
{
    FC_ERR("filterSelected: " << index);
    QString selectedDocument = ui->comboBoxDocument->currentText();
    QString selectedTypeObject = ui->comboBoxTypeObject->currentText();
    QString selectedNameObject = ui->comboBoxNameObject->currentText();
    QString selectedGroup = ui->comboBoxGroup->currentText();
    QString selectedTypeProperty = ui->comboBoxTypeProp->currentText();
    QString selectedNameProperty = ui->comboBoxNameProp->currentText();
    
    auto *model = getModel();
    model->filterSelected(selectedDocument,
                          selectedTypeObject, selectedNameObject,
                          selectedGroup,
                          selectedTypeProperty, selectedNameProperty);

    ui->comboBoxNameScope->clearEditText();
}

// TODO: not yet implemented
void DlgPropertyManager::changeEvent(QEvent *e)
{
    // if (e->type() == QEvent::LanguageChange) {
    //     ui->retranslateUi(this);
    //     setWindowTitle(tr("Property Manager"));
    // }
    // QDialog::changeEvent(e);
}


// doesn't work as I expected need to debug more
void DlgPropertyManager::setNextInTabOrder(QWidget *widget)
{
    FC_ERR("setnext");
    FC_ERR("  last: " << widgetLastTabOrder->objectName().toUtf8().constData());
    FC_ERR("  next: " << widget->objectName().toUtf8().constData());
    // QWidget::setTabOrder(widgetLastTabOrder, widget);
    // widgetLastTabOrder = widget;
}

// idem
void DlgPropertyManager::setFirstInTabOrder(QWidget *widget)
{
    FC_ERR("First: " << widget->objectName().toUtf8().constData());
    // widgetLastTabOrder = widget;
}

// debug code
// static void printFocusChain(QWidget *widget) {
//     FC_ERR("Focus Chain:");
//     for (int i = 0; i < 30; i++) {
//         FC_ERR(" " << widget->objectName().toUtf8().constData());
//         widget = widget->nextInFocusChain();
//     }
//     QWidget *currentWidget = QApplication::focusWidget();
//     FC_ERR("  Current focus widget:" << (currentWidget ? currentWidget->objectName().toUtf8().constData() : "None"));
// }

// doesn't work as I expected
void DlgPropertyManager::setTabOrderAddProperty()
{
    QWidget::setTabOrder(ui->lineEditNamePropertyAdd, &comboBoxGroupAdd);
    QWidget::setTabOrder(&comboBoxGroupAdd, ui->comboBoxTypePropertyAdd);
    QWidget::setTabOrder(ui->comboBoxTypePropertyAdd, ui->buttonAddProperty);
}


// doesn't work as I expected
void DlgPropertyManager::setTabOrderFilter()
{
    QWidget::setTabOrder(ui->comboBoxDocument, ui->comboBoxTypeObject);
    QWidget::setTabOrder(ui->comboBoxTypeObject, ui->comboBoxNameObject);
    QWidget::setTabOrder(ui->comboBoxNameObject, ui->comboBoxGroup);
    QWidget::setTabOrder(ui->comboBoxGroup, ui->comboBoxTypeProp);
    QWidget::setTabOrder(ui->comboBoxTypeProp, ui->comboBoxNameProp);
}
    


void DlgPropertyManager::showAddPropertyPanel()
{
    PropertyManagerModel* model = getModel();
    App::DocumentObject* obj = model->getUniqueObject();
    FC_ERR("onModelReset");
    if (obj) {
        setTabOrderAddProperty();
        ui->groupBoxAddProperty->setVisible(true);
        ui->lineEditNamePropertyAdd->setFocus();

        comboBoxGroupAdd.clear();
        for (const auto& groupName : model->getGroupNames()) {
            comboBoxGroupAdd.addItem(groupName);
        }
        
        QString group = model->getUniqueGroup();
        if (!group.isEmpty()) {
            comboBoxGroupAdd.setEditText(group);
        }

        QString type = model->getUniqueTypeProperty();
        if (!type.isEmpty()) {
            ui->comboBoxTypePropertyAdd->setEditText(type);
        }

        // FC_ERR("showAddPropertyPanel");
        // printFocusChain(ui->lineEditNamePropertyAdd);
    }
    else {
        setTabOrderFilter();
        ui->groupBoxAddProperty->setVisible(false);
        ui->comboBoxDocument->setFocus();
    }
}

void DlgPropertyManager::onModelReset()
{
    FC_ERR("onModelReset");
    showAddPropertyPanel();
    setupFilters();
}

void DlgPropertyManager::clearPropertyAdd()
{
    ui->lineEditNamePropertyAdd->clear();
    removeEditor();
    ui->buttonAddProperty->setEnabled(false);
    namePropertyToAdd.clear();
    editor = nullptr;
    
}

static PropertyItem *createPropertyItem(App::Property *prop)
{
    const char *editor = prop->getEditorName();
    if (!editor || !editor[0]) {
        return nullptr;
    }
    auto item = static_cast<PropertyItem*>(
            PropertyItemFactory::instance().createPropertyItem(editor));
    if (!item) {
        qWarning("No property item for type %s found\n", editor);
    }
    return item;
}

void DlgPropertyManager::createProperty(App::DocumentObject* obj, std::string& name, std::string& group)
{
    std::string type = ui->comboBoxTypePropertyAdd->currentText().toUtf8().constData();

    auto prop = obj->addDynamicProperty(type.c_str(), name.c_str(),
                                        group.c_str());
    namePropertyToAdd = name;

    // FC_ERR("type: " << prop->getTypeId().getName());
    // FC_ERR("type class: " << App::PropertyString::getClassTypeId().getName());

    objectIdentifier = std::make_unique<App::ObjectIdentifier>(*prop);
    // creating a propertyItem here because it has all kinds of logic for
    // editors that we can reuse
    propertyItem.reset(createPropertyItem(prop));
    propertyItem->setPropertyData({prop});
    propertyItem->bind(*objectIdentifier);

    ui->buttonAddProperty->setEnabled(true);

    removeEditor();
    if (unsupportedTypes.find(type) == unsupportedTypes.end()) {
        addEditor(propertyItem.get(), type);
    }
}


void DlgPropertyManager::onNamePropertyDetermined()
{
    PropertyManagerModel* model = getModel();
    App::DocumentObject* obj = model->getUniqueObject();

    // FC_ERR("onNamePropertyDetermined");
    // printFocusChain(ui->lineEditNamePropertyAdd);

    if (!namePropertyToAdd.empty()) {
        // we were already adding a name, so remove this property
        obj->removeDynamicProperty(namePropertyToAdd.c_str());
    }
    
    QString nameProperty = ui->lineEditNamePropertyAdd->text();
    std::string name = nameProperty.toUtf8().constData();
    std::string group = comboBoxGroupAdd.currentText().toUtf8().constData();
    if(name.empty() || group.empty()
            || name != Base::Tools::getIdentifier(name)
            || group != Base::Tools::getIdentifier(group))
    {
        QMessageBox::critical(this,
            QObject::tr("Invalid name"),
            QObject::tr("The property name or group name must only contain alpha numericals,\n"
                        "underscore, and must not start with a digit."));
        clearPropertyAdd();
        return;
    }

    auto prop = obj->getPropertyByName(name.c_str());
    if(prop && prop->getContainer() == obj) {
        QMessageBox::critical(this,
                              QObject::tr("Invalid name"),
                              QObject::tr("The property '%1' already exists in '%2'").arg(
                                      QString::fromLatin1(name.c_str()),
                                      QString::fromLatin1(obj->getFullName().c_str())));
        clearPropertyAdd();
        return;
    }

    createProperty(obj, name, group);
}


void DlgPropertyManager::onGroupDetermined()
{
    FC_ERR("onGroupDetermined:");

    std::string group = comboBoxGroupAdd.currentText().toUtf8().constData();

    FC_ERR("group: \"" << group << "\"");
    
    if (group.empty() || group != Base::Tools::getIdentifier(group)) {
        QMessageBox::critical(this,
            QObject::tr("Invalid name"),
            QObject::tr("The group name must only contain alpha numericals,\n"
                        "underscore, and must not start with a digit."));
        PropertyManagerModel* model = getModel();
        QString group = model->getUniqueGroup();
        if (group.isEmpty()) {
            comboBoxGroupAdd.setEditText(Filter::BASE_GROUP);
        }
        else {
            comboBoxGroupAdd.setEditText(group);
        }
    }

    PropertyManagerModel* model = getModel();
    App::DocumentObject* obj = model->getUniqueObject();

    if (!namePropertyToAdd.empty()) {
        // we were already adding a property
        App::Property* prop = obj->getPropertyByName(namePropertyToAdd.c_str());
        if (prop->getGroup() != group) {
            obj->changeDynamicProperty(prop, group.c_str(), nullptr);
        }
    }

    ui->comboBoxTypePropertyAdd->setFocus();
}

void DlgPropertyManager::onTypePropertyDetermined()
{
    FC_ERR("onTypePropertyDetermined");

    PropertyManagerModel* model = getModel();
    App::DocumentObject* obj = model->getUniqueObject();

    std::string type = ui->comboBoxTypePropertyAdd->currentText().toUtf8().constData();
    
    if (!namePropertyToAdd.empty()) {
        // we were already adding a name, so check this property
        App::Property* prop = obj->getPropertyByName(namePropertyToAdd.c_str());

        if (prop->getTypeId() != Base::Type::fromName(type.c_str())) {
            // the property should have a different type
            std::string group = prop->getGroup();
            obj->removeDynamicProperty(namePropertyToAdd.c_str());
            createProperty(obj, namePropertyToAdd, group);
        }
    }
}

void DlgPropertyManager::recomputeDocument()
{
    App::Property* prop = propertyItem->getFirstProperty();
    App::DocumentObject* obj = objectIdentifier->getDocumentObject();
    App::Document* doc = obj->getDocument();

    try {
        if (doc /* && !doc->isTransactionEmpty() */) { // no transaction support yet
            // Between opening and committing a transaction a recompute
            // could already have been done
            if (doc->isTouched()) {
                doc->recompute();
            }
        }
    }
    // do not re-throw
    catch (const Base::Exception& e) {
        e.ReportException();
    }
    catch (const std::exception& e) {
        Base::Console().Error("Unhandled std::exception caught in PropertyManager::recomputeDocument.\n"
                            "The error message is: %s\n", e.what());
    }
    catch (...) {
        Base::Console().Error("Unhandled unknown exception caught in PropertyManager::recomputeDocument.\n");
    }
}

void DlgPropertyManager::onAddProperty()
{
    clearPropertyAdd();
    recomputeDocument();
    auto *model = getModel();
    model->refresh();
    setupFilters();
    ui->lineEditNamePropertyAdd->setFocus();
}
    


void DlgPropertyManager::valueChanged()
{
    FC_ERR("valueChanged");
    QWidget* editor = qobject_cast<QWidget*>(sender());
    QVariant data;
    data = propertyItem->editorData(editor);

    propertyItem->setData(data);
}

void DlgPropertyManager::onRefreshButtonClicked()
{
    auto *model = getModel();
    model->refresh();
    setupFilters();
}

void DlgPropertyManager::onChoosingNameScope(QString name)
{
    FC_ERR("onChoosingNameScope");
    auto model = getModel();
    if (model->scopeExists(name)) {
        FC_ERR("select scope " << name.toUtf8().constData());
        model->selectScope(name);
        model->refresh();
        setupFilters();
    }
    else {
        FC_ERR("inserting scope " << name.toUtf8().constData());
        model->insertScope(name);
        setupScopes();
    }
}

#include "moc_PropertyManager.cpp"
