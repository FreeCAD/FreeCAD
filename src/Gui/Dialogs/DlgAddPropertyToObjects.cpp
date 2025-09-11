#include "PreCompiled.h"
#ifndef _PreComp_
#include <QMessageBox>
#include <QString>
#include <QCompleter>
#include <algorithm>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/ExpressionParser.h>
#include <App/PropertyUnits.h>
#include <App/PropertyFile.h>
#include <App/PropertyGeo.h>
#include <Base/Tools.h>

#include "Dialogs/DlgAddPropertyToObjects.h"
#include "Dialogs/DlgAddPropertyVarSet.h"
#include "Selection.h"
#include "ui_DlgAddPropertyVarSet.h"

FC_LOG_LEVEL_INIT("DlgAddPropertyToObjects", true, true)

using namespace Gui;
using namespace Gui::Dialog;

const std::string DlgAddPropertyToObjects::GroupBase = "Base";

DlgAddPropertyToObjects::DlgAddPropertyToObjects(QWidget* parent, const std::vector<App::DocumentObject*>& objects)
    : QDialog(parent),
      objects(objects),
      ui(new Ui_DlgAddPropertyVarSet),
      comboBoxGroup(new EditFinishedComboBox(this)),
      completerType(this),
      editor(nullptr),
      transactionID(0)
{
    ui->setupUi(this);
    initializeWidgets();
}

DlgAddPropertyToObjects::~DlgAddPropertyToObjects(){
    delete comboBoxGroup;
}

void DlgAddPropertyToObjects::initializeWidgets()
{
    initializeGroup();
    initializeTypes();
    initializeValue();

    connLineEditNameTextChanged = connect(ui->lineEditName, &QLineEdit::textChanged,
        this, &DlgAddPropertyToObjects::onTextFieldChanged);

    setWindowTitle(tr("Add Property to Selected Objects"));
    setOkEnabled(false);
    ui->lineEditName->setFocus();

    QWidget::setTabOrder(ui->lineEditName, comboBoxGroup);
    QWidget::setTabOrder(comboBoxGroup, ui->comboBoxType);
}

int DlgAddPropertyToObjects::findLabelRow(const char* labelName, QFormLayout* layout)
{
    for (int row = 0; row < layout->rowCount(); ++row) {
        QLayoutItem* labelItem = layout->itemAt(row, QFormLayout::LabelRole);
        if (!labelItem) continue;

        if (auto label = qobject_cast<QLabel*>(labelItem->widget())) {
            if (label->objectName() == QString::fromLatin1(labelName))
                return row;
        }
    }
    return -1;
}

void DlgAddPropertyToObjects::setWidgetForLabel(const char* labelName, QWidget* widget)
{
    auto formLayout = qobject_cast<QFormLayout*>(layout());
    if (!formLayout) {
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

void DlgAddPropertyToObjects::initializeGroup()
{
    setWidgetForLabel("labelGroup", comboBoxGroup);

    std::unordered_set<std::string> groupNames;
    for (auto obj : objects) {
        std::vector<App::Property*> properties;
        obj->getPropertyList(properties);
        
        for (const auto* prop : properties) {
            const char* groupName = obj->getPropertyGroup(prop);
            groupNames.insert(groupName ? groupName : GroupBase);
        }
    }

    if (groupNames.empty()) {
        groupNames.insert(GroupBase);
    }

    std::vector<std::string> groupNamesSorted(groupNames.begin(), groupNames.end());
    std::sort(groupNamesSorted.begin(), groupNamesSorted.end(), [](const auto& a, const auto& b) {
        if (a == GroupBase) return false;
        if (b == GroupBase) return true;
        return a < b;
    });

    for (const auto& groupName : groupNamesSorted) {
        comboBoxGroup->addItem(QString::fromStdString(groupName));
    }

    comboBoxGroup->setEditText(QString::fromStdString(groupNamesSorted[0]));
    connComboBoxGroup = connect(comboBoxGroup, &EditFinishedComboBox::currentTextChanged,
                              this, &DlgAddPropertyToObjects::onTextFieldChanged);
}

std::vector<Base::Type> DlgAddPropertyToObjects::getSupportedTypes()
{
    std::vector<Base::Type> supportedTypes;
    std::vector<Base::Type> allTypes;
    Base::Type::getAllDerivedFrom(Base::Type::fromName("App::Property"), allTypes);

    std::copy_if(allTypes.begin(), allTypes.end(), std::back_inserter(supportedTypes),
        [](const Base::Type& type) { return type.canInstantiate(); });

    std::sort(supportedTypes.begin(), supportedTypes.end(), [](Base::Type a, Base::Type b) {
        return strcmp(a.getName(), b.getName()) < 0;
    });

    return supportedTypes;
}

void DlgAddPropertyToObjects::initializeTypes()
{
    auto paramGroup = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/PropertyView");
    auto lastType = Base::Type::fromName(
        paramGroup->GetASCII("NewPropertyType", "App::PropertyLength").c_str());
    if (lastType.isBad()) {
        lastType = App::PropertyLength::getClassTypeId();
    }

    std::vector<Base::Type> types = getSupportedTypes();

    for (const auto& type : types) {
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
                             this, &DlgAddPropertyToObjects::onTypeChanged);
}

void DlgAddPropertyToObjects::removeSelectionEditor()
{
    if (auto lineEdit = editor->findChild<QLineEdit*>()) {
        lineEdit->deselect();
    }
}

void DlgAddPropertyToObjects::addEditor(PropertyEditor::PropertyItem* propertyItem)
{
    editor.reset(propertyItem->createEditor(this, [this]() { this->valueChanged(); }, 
        PropertyEditor::FrameOption::WithFrame));
    if (!editor) return;

    QSignalBlocker blockSignals(editor.get());
    propertyItem->setEditorData(editor.get(),
        propertyItem->data(PropertyEditor::PropertyItem::ValueColumn, Qt::EditRole));
    editor->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    editor->setObjectName(QStringLiteral("editor"));

    setWidgetForLabel("labelValue", editor.get());
    QWidget::setTabOrder(ui->comboBoxType, editor.get());
    QWidget::setTabOrder(editor.get(), ui->checkBoxAdd);
    removeSelectionEditor();
}

bool DlgAddPropertyToObjects::isTypeWithEditor(const Base::Type& type)
{
    static const std::initializer_list<Base::Type> subTypesWithEditor = {
        App::PropertyBool::getClassTypeId(),
        App::PropertyColor::getClassTypeId(),
        App::PropertyFileIncluded::getClassTypeId(),
        App::PropertyFloat::getClassTypeId(),
        App::PropertyInteger::getClassTypeId()
    };

    static const std::initializer_list<Base::Type> typesWithEditor = {
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

    return std::find(typesWithEditor.begin(), typesWithEditor.end(), type) != typesWithEditor.end() ||
        std::any_of(subTypesWithEditor.begin(), subTypesWithEditor.end(),
                   [&type](const Base::Type& t) { return type.isDerivedFrom(t); });
}

PropertyEditor::PropertyItem* DlgAddPropertyToObjects::createPropertyItem(App::Property* prop)
{
    const char* editor = prop->getEditorName();
    return editor ? PropertyEditor::PropertyItemFactory::instance().createPropertyItem(editor) : nullptr;
}

void DlgAddPropertyToObjects::createEditorForType(const Base::Type& type)
{
    std::unique_ptr<App::Property, void(*)(App::Property*)> prop(
        static_cast<App::Property*>(type.createInstance()),
        [](App::Property* p) { delete p; });
    if (!prop) {
        FC_THROWM(Base::RuntimeError, "Failed to create property of type " << type.getName());
    }

    // Use first object as container for property validation
    prop->setContainer(objects.empty() ? nullptr : objects.front());

    propertyItem.reset(createPropertyItem(prop.get()));
    if (propertyItem && isTypeWithEditor(type)) {
        propertyItem->setPropertyData({prop.get()});
        addEditor(propertyItem.get());
        propertyItem->removeProperty(prop.get());
    }
}

void DlgAddPropertyToObjects::initializeValue()
{
    std::string type = ui->comboBoxType->currentText().toStdString();
    Base::Type propType = Base::Type::getTypeIfDerivedFrom(type.c_str(), App::Property::getClassTypeId(), true);
    if (propType.isBad()) {
        FC_THROWM(Base::TypeError, "Invalid type " << type << " for property");
    }

    if (isTypeWithEditor(propType)) {
        createEditorForType(propType);
    }
}

void DlgAddPropertyToObjects::setOkEnabled(bool enabled)
{
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(enabled);
}

bool DlgAddPropertyToObjects::propertyExistsInAny(const std::string& name, std::vector<App::DocumentObject*>& conflictingObjects)
{
    conflictingObjects.clear();
    for (auto obj : objects) {
        if (obj->getPropertyByName(name.c_str())) {
            conflictingObjects.push_back(obj);
        }
    }
    return !conflictingObjects.empty();
}

bool DlgAddPropertyToObjects::isNameValid()
{
    std::string name = ui->lineEditName->text().toStdString();
    if (name.empty()) return false;
    
    return name == Base::Tools::getIdentifier(name) &&
           !App::ExpressionParser::isTokenAConstant(name) &&
           !App::ExpressionParser::isTokenAUnit(name);
}

bool DlgAddPropertyToObjects::isGroupValid()
{
    std::string group = comboBoxGroup->currentText().toStdString();
    return !group.empty() && group == Base::Tools::getIdentifier(group);
}

bool DlgAddPropertyToObjects::isTypeValid()
{
    std::string type = ui->comboBoxType->currentText().toStdString();
    return !Base::Type::fromName(type.c_str()).isBad();
}

bool DlgAddPropertyToObjects::areFieldsValid()
{
    if (!isNameValid() || !isGroupValid() || !isTypeValid()) {
        return false;
    }

    std::vector<App::DocumentObject*> conflicts;
    return !propertyExistsInAny(ui->lineEditName->text().toStdString(), conflicts);
}

void DlgAddPropertyToObjects::onTextFieldChanged(const QString&)
{
    setOkEnabled(areFieldsValid());
}

void DlgAddPropertyToObjects::removeEditor()
{
    if (!editor) return;
    layout()->removeWidget(editor.get());
    QWidget::setTabOrder(ui->comboBoxType, ui->checkBoxAdd);
    editor = nullptr;
}

void DlgAddPropertyToObjects::onTypeChanged(const QString& text)
{
    std::string type = text.toStdString();
    Base::Type propType = Base::Type::fromName(type.c_str());

    if (propType.isBad() || !isTypeWithEditor(propType)) {
        propertyItem = nullptr;
        removeEditor();
    } else {
        initializeValue();
    }

    setOkEnabled(areFieldsValid());
}

void DlgAddPropertyToObjects::valueChanged()
{
    if (propertyItem && editor) {
        QVariant data = propertyItem->editorData(editor.get());
        propertyItem->setData(data);
    }
}

void DlgAddPropertyToObjects::openTransaction(const std::string& propName)
{
    std::string msg = "Add property";
    if (!propName.empty()) msg += " '" + propName + "'";
    transactionID = App::GetApplication().setActiveTransaction(msg.c_str());
}

void DlgAddPropertyToObjects::closeTransaction(TransactionOption option)
{
    if (transactionID == 0) return;
    App::GetApplication().closeActiveTransaction(static_cast<bool>(option), transactionID);
    transactionID = 0;
}

void DlgAddPropertyToObjects::clearFields()
{
    QSignalBlocker blocker(ui->lineEditName);
    ui->lineEditName->clear();
    ui->lineEditToolTip->clear();
    initializeValue();
    setOkEnabled(false);
}

bool DlgAddPropertyToObjects::addPropertyToObjects()
{
    std::string name = ui->lineEditName->text().toStdString();
    std::string group = comboBoxGroup->currentText().toStdString();
    std::string type = ui->comboBoxType->currentText().toStdString();
    std::string doc = ui->lineEditToolTip->text().toStdString();

    std::vector<App::DocumentObject*> conflicts;
    if (propertyExistsInAny(name, conflicts)) {
        QStringList conflictNames;
        for (auto obj : conflicts) {
            conflictNames << QString::fromUtf8(obj->getNameInDocument());
        }
        
        QMessageBox::warning(this, 
            QObject::tr("Property exists"),
            QObject::tr("The property '%1' already exists in:\n%2\n\nPlease choose a different name.")
                .arg(QString::fromUtf8(name.c_str()))
                .arg(conflictNames.join(QString::fromLatin1("\n"))));
        return false;
    }

    bool success = true;
    std::vector<App::DocumentObject*> successfulAdds;

    
    for (auto obj : objects) {
        try {
            App::Property* prop = obj->addDynamicProperty(
                type.c_str(), 
                name.c_str(),
                group.c_str(), 
                doc.c_str());
                
            if (prop) {
                successfulAdds.push_back(obj);
                
                if (propertyItem) {
                    auto oid = std::make_unique<App::ObjectIdentifier>(*prop);
                    propertyItem->setPropertyData({prop});
                    propertyItem->bind(*oid);
                    
                    QVariant data = propertyItem->editorData(editor.get());
                    propertyItem->setData(data);
                }
            }
        }
        catch (Base::Exception& e) {
            e.reportException();
            success = false;
            
            QMessageBox::warning(this,
                QObject::tr("Add property failed"),
                QObject::tr("Failed to add property to '%1':\n%2")
                    .arg(QString::fromUtf8(obj->getNameInDocument()))
                    .arg(QString::fromUtf8(e.what())));
        }
    }

    if (success) {
        auto paramGroup = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/PropertyView");
        paramGroup->SetASCII("NewPropertyType", type.c_str());
        paramGroup->SetASCII("NewPropertyGroup", group.c_str());
        
        return true;
    } else {
        auto reply = QMessageBox::question(this,
            QObject::tr("Partial success"),
            QObject::tr("Property was added to some objects but failed on others.\n"
               "Do you want to keep the changes?"),
            QMessageBox::Yes|QMessageBox::No);
            
        if (reply == QMessageBox::Yes) {
            return true;
        } else {
            return false;
        }
    }
}

void DlgAddPropertyToObjects::accept()
{
    const std::string name = ui->lineEditName->text().toStdString();
    openTransaction(name);
    if (addPropertyToObjects()) {
        closeTransaction(TransactionOption::Commit);
        if (ui->checkBoxAdd->isChecked()) {
            clearFields();
            ui->lineEditName->setFocus();
        } else {
            QDialog::accept();
        }
    }
    else{
        closeTransaction(TransactionOption::Abort);
    }
}

void DlgAddPropertyToObjects::reject()
{
    disconnect(connComboBoxGroup);
    disconnect(connComboBoxType);
    disconnect(connLineEditNameTextChanged);
    QDialog::reject();
}

void DlgAddPropertyToObjects::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        setWindowTitle(tr("Add Property to Selected Objects"));
    }
    QDialog::changeEvent(e);
}

#include "moc_DlgAddPropertyToObjects.cpp"
