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

#ifndef GUI_DIALOG_DLG_ADD_PROPERTY_H
#define GUI_DIALOG_DLG_ADD_PROPERTY_H

#include <qcompleter.h>

#include <QDialog>
#include <QComboBox>
#include <QFormLayout>

#include <FCGlobal.h>

#include <App/PropertyContainer.h>

#include "propertyeditor/PropertyItem.h"

namespace Gui {

class ViewProviderVarSet;

namespace Dialog {

class EditFinishedComboBox : public QComboBox {
    Q_OBJECT
public:
    explicit EditFinishedComboBox(QWidget *parent = nullptr) : QComboBox(parent) {
        setEditable(true);
        connect(this, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &EditFinishedComboBox::onIndexChanged);
        connect(this->lineEdit(), &QLineEdit::editingFinished, this, &EditFinishedComboBox::onEditingFinished);
    }

Q_SIGNALS:
    void editFinished();

private:
    void onEditingFinished() {
        Q_EMIT editFinished();
    }

    void onIndexChanged() {
        Q_EMIT editFinished();
    }
};

class Ui_DlgAddProperty;

class GuiExport DlgAddProperty : public QDialog
{
    Q_OBJECT

public:
    static const std::string GroupBase;

public:
    DlgAddProperty(QWidget* parent, ViewProviderVarSet* viewProvider);
    DlgAddProperty(QWidget* parent, App::PropertyContainer* container);

    DlgAddProperty(const DlgAddProperty&) = delete;
    DlgAddProperty(DlgAddProperty&&) = delete;
    DlgAddProperty& operator=(const DlgAddProperty&) = delete;
    DlgAddProperty& operator=(DlgAddProperty&&) = delete;

    ~DlgAddProperty() override;

    void changeEvent(QEvent* e) override;
    void accept() override;
    void reject() override;
    static void populateGroup(EditFinishedComboBox& comboBox,
                              const App::PropertyContainer* container);
    static void setWidgetForLabel(const char* labelName, QWidget* widget,
                                  QLayout* layout);

public Q_SLOTS:
    void valueChanged();
    void valueChangedEnum();

private:
    enum class TransactionOption : bool {
        Commit = false,
        Abort = true
    };

    enum class FieldChange : std::uint8_t {
        Name,
        Type
    };

    DlgAddProperty(QWidget* parent, App::PropertyContainer* container,
                         ViewProviderVarSet* viewProvider);

    void initializeGroup();

    std::vector<Base::Type> getSupportedTypes();
    void initializeTypes();

    void removeSelectionEditor();
    QVariant getEditorData() const;
    void setEditorData(const QVariant& data);
    bool isEnumPropertyItem() const;
    void addEnumEditor(PropertyEditor::PropertyItem* propertyItem);
    void addNormalEditor(PropertyEditor::PropertyItem* propertyItem);
    void addEditor(PropertyEditor::PropertyItem* propertyItem);
    bool isTypeWithEditor(const Base::Type& type);
    bool isTypeWithEditor(const std::string& type);
    void createEditorForType(const Base::Type& type);
    void initializeValue();

    void setTitle();
    void setAddEnabled(bool enabled);
    void initializeWidgets(ViewProviderVarSet* viewProvider);

    bool isDocument() const;
    bool isDocumentObject() const;
    bool propertyExists(const std::string& name);
    bool isNameValid();
    bool isGroupValid();
    bool isTypeValid();
    bool areFieldsValid();

    void setEditor(bool valueNeedsReset);
    void buildForUnbound(bool valueNeedsReset);
    void setPropertyItem(App::Property* prop, bool supportsExpressions);
    void buildForBound(bool valueNeedsReset, bool supportsExpressions);
    bool clearBoundProperty();
    bool clear(FieldChange fieldChange);
    void onNameChanged(const QString& text);
    void onGroupFinished();
    void onTypeChanged(const QString& text);

    void showStatusMessage();

    void removeEditor();

    void openTransaction();
    void critical(const QString& title, const QString& text);
    App::Property* createProperty();
    void closeTransaction(TransactionOption option);
    void clearFields();
    void addDocumentation();

    static void removeExistingWidget(QFormLayout* layout, int labelRow);
    static int findLabelRow(const char* labelName, QFormLayout* layout);

private:
    App::PropertyContainer* container;
    std::unique_ptr<Ui_DlgAddProperty> ui;

    EditFinishedComboBox comboBoxGroup;
    QCompleter completerType;

    std::unique_ptr<QWidget> editor;
    std::unique_ptr<PropertyEditor::PropertyItem> propertyItem;
    std::unique_ptr<App::ObjectIdentifier> objectIdentifier;

    // a transactionID of 0 means that there is no active transaction.
    int transactionID;

    QMetaObject::Connection connComboBoxGroup;
    QMetaObject::Connection connComboBoxType;
    QMetaObject::Connection connLineEditNameTextChanged;
};

} // namespace Dialog
} // namespace Gui

#endif // GUI_DIALOG_DLG_ADD_PROPERTY_H
