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

#ifndef GUI_DIALOG_DLG_ADD_PROPERTY_VARSET_H
#define GUI_DIALOG_DLG_ADD_PROPERTY_VARSET_H

#include <qcompleter.h>
#include <unordered_set>
#include <QDialog>
#include <QComboBox>
#include <FCGlobal.h>

#include <App/VarSet.h>

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

class Ui_DlgAddPropertyVarSet;

class GuiExport DlgAddPropertyVarSet : public QDialog
{
    Q_OBJECT

public:
    static const std::string GROUP_BASE;

public:
    DlgAddPropertyVarSet(QWidget *parent, ViewProviderVarSet* viewProvider);
    ~DlgAddPropertyVarSet() override;

    void changeEvent(QEvent* e) override;
    void accept() override;
    void reject() override;

public Q_SLOTS:
    void valueChanged();

private:
    void initializeGroup();
    void initializeTypes();
    void initializeWidgets(ViewProviderVarSet* viewProvider);

    void setTitle();
    void setOkEnabled(bool enabled);
    void clearEditors(bool clearName = true);
    void clearCurrentProperty();

    void removeEditor();
    void addEditor(PropertyEditor::PropertyItem* propertyItem, std::string& type);

    bool isTypeWithEditor(const std::string& type);
    void createProperty();
    void changePropertyToAdd();

    void openTransaction();
    bool hasPendingTransaction();
    void abortTransaction();
    void closeTransaction(bool abort);

    void checkName();
    void checkGroup();
    void checkType();
    void onEditFinished();
    void onNamePropertyChanged(const QString& text);
    void critical(const QString& title, const QString& text);

    void getSupportedTypes(std::vector<Base::Type>& types);
    App::Property* getPropertyToAdd();
    void addDocumentation();

private:
    std::unordered_set<std::string> typesWithoutEditor = {
        "App::PropertyVector", "App::PropertyVectorDistance", "App::PropertyMatrix",
        "App::PropertyRotation", "App::PropertyPlacement", "App::PropertyEnumeration",
        "App::PropertyDirection", "App::PropertyPlacementList", "App::PropertyPosition",
        "App::PropertyExpressionEngine", "App::PropertyIntegerSet",
        "Sketcher::PropertyConstraintList"};

    App::VarSet* varSet;
    std::unique_ptr<Ui_DlgAddPropertyVarSet> ui;

    EditFinishedComboBox comboBoxGroup;
    QCompleter completerType;

    // state between adding properties
    std::unique_ptr<QWidget> editor;
    std::unique_ptr<QWidget> expressionEditor;
    std::string namePropertyToAdd;
    std::unique_ptr<PropertyEditor::PropertyItem> propertyItem;
    std::unique_ptr<App::ObjectIdentifier> objectIdentifier;

    // a transactionID of 0 means that there is no active transaction.
    int transactionID;

    // connections
    QMetaObject::Connection connComboBoxGroup;
    QMetaObject::Connection connComboBoxType;
    QMetaObject::Connection connLineEditNameEditFinished;
    QMetaObject::Connection connLineEditNameTextChanged;
};

} // namespace Dialog
} // namespace Gui

#endif // GUI_DIALOG_DLG_ADD_PROPERTY_VARSET_H
