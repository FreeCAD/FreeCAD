#ifndef GUI_DIALOG_DLG_ADD_PROPERTY_TO_OBJECTS_H
#define GUI_DIALOG_DLG_ADD_PROPERTY_TO_OBJECTS_H

#include <qcompleter.h>
#include <QDialog>
#include <QComboBox>
#include <QFormLayout>
#include <vector>

#include <FCGlobal.h>
#include <App/DocumentObject.h>
#include <App/Property.h>
#include "Gui/propertyeditor/PropertyItem.h"

namespace Gui {
namespace Dialog {

class EditFinishedComboBox;

class Ui_DlgAddPropertyVarSet;

class GuiExport DlgAddPropertyToObjects : public QDialog
{
    Q_OBJECT

public:
    static const std::string GroupBase;

public:
    DlgAddPropertyToObjects(QWidget* parent, const std::vector<App::DocumentObject*>& objects);
    
    DlgAddPropertyToObjects(const DlgAddPropertyToObjects&) = delete;
    DlgAddPropertyToObjects(DlgAddPropertyToObjects&&) = delete;
    DlgAddPropertyToObjects& operator=(const DlgAddPropertyToObjects&) = delete;
    DlgAddPropertyToObjects& operator=(DlgAddPropertyToObjects&&) = delete;

    ~DlgAddPropertyToObjects() override;

    void changeEvent(QEvent* e) override;
    void accept() override;
    void reject() override;

public Q_SLOTS:
    void valueChanged();

private:
    enum class TransactionOption : bool {
        Commit = false,
        Abort = true
    };

    // Initialization methods
    void initializeWidgets();
    int findLabelRow(const char* labelName, QFormLayout* layout);
    void setWidgetForLabel(const char* labelName, QWidget* widget);
    void initializeGroup();
    std::vector<Base::Type> getSupportedTypes();
    void initializeTypes();
    void initializeValue();

    // Editor handling
    void removeSelectionEditor();
    void addEditor(PropertyEditor::PropertyItem* propertyItem);
    bool isTypeWithEditor(const Base::Type& type);
    bool isTypeWithEditor(const std::string& type);
    static PropertyEditor::PropertyItem* createPropertyItem(App::Property* prop);
    void createEditorForType(const Base::Type& type);
    void removeEditor();

    // Validation methods
    bool propertyExistsInAny(const std::string& name, std::vector<App::DocumentObject*>& conflictingObjects);
    bool isNameValid();
    bool isGroupValid();
    bool isTypeValid();
    bool areFieldsValid();

    // UI state methods
    void setOkEnabled(bool enabled);
    void clearFields();

    // Transaction handling
    void openTransaction(const std::string& propName);
    void closeTransaction(TransactionOption option);

    // Core functionality
    bool addPropertyToObjects();

    // Slots
    void onTextFieldChanged(const QString& text);
    void onTypeChanged(const QString& text);

private:
    std::vector<App::DocumentObject*> objects;
    std::unique_ptr<Ui_DlgAddPropertyVarSet> ui;

    EditFinishedComboBox* comboBoxGroup;
    QCompleter completerType;

    std::unique_ptr<QWidget> editor;
    std::unique_ptr<PropertyEditor::PropertyItem> propertyItem;
    std::unique_ptr<App::ObjectIdentifier> objectIdentifier;

    int transactionID; // 0 means no active transaction

    QMetaObject::Connection connComboBoxGroup;
    QMetaObject::Connection connComboBoxType;
    QMetaObject::Connection connLineEditNameTextChanged;
};

} // namespace Dialog
} // namespace Gui

#endif // GUI_DIALOG_DLG_ADD_PROPERTY_TO_OBJECTS_H
