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

#ifndef GUI_PROPERTY_MANAGER_H
#define GUI_PROPERTY_MANAGER_H
#include <QComboBox>
#include <QDialog>
#include <QHash>
#include <QStandardItemModel>
#include <QAbstractItemModel>
#include <functional>
#include <QCompleter>

#include "PropertyManagerModel.h"
#include "Filter.h"
#include "propertyeditor/PropertyItem.h"

class SoNode;
class PropertyEditor2;


namespace Gui {
class Document;

namespace PropertyEditor {
    class PropertyEditor2;
}
    
namespace Dialog {

class Ui_PropertyManager;


    
class EditFinishedComboBox : public QComboBox {
    Q_OBJECT
public:
    explicit EditFinishedComboBox(QWidget *parent = nullptr) : QComboBox(parent) {}

Q_SIGNALS:
    void editFinished();

protected:
    void focusOutEvent(QFocusEvent *event) override {
        QComboBox::focusOutEvent(event);
        Q_EMIT editFinished();
    }
};


/// Dialog window to display scenegraph model as a tree
class DlgPropertyManager : public QDialog
{
    Q_OBJECT

public:
    DlgPropertyManager(QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());
    ~DlgPropertyManager() override;

    Gui::PropertyEditor::PropertyEditor2* propertyEditor;
    
    static std::unique_ptr<QPixmap> documentPixmap;
    static std::unique_ptr<QPixmap> documentPartialPixmap;



private:
    void onRefreshButtonClicked();
    void onNamePropertyDetermined();
    void onGroupDetermined();
    void onTypePropertyDetermined();
    void onModelReset();

    void onChoosingNameScope(QString name);

    void reset();
    void setupPropertyEditor();
    void setupWidgets();
    void setupScopes();


    void initializeFilter(QComboBox* comboBox, QCompleter &completer);
    void initializeFilters();
    
    void setupFilter(QComboBox* comboBox,
                     std::function<Gui::PropertyEditor::UniqueVector<QString>(void)> getFilterValuesFunc);
    void setupFilters();
    PropertyEditor::PropertyManagerModel* getModel();
    void filterSelected(int index);

    void addEditor(PropertyEditor::PropertyItem* propertyItem, std::string& type);
    void removeEditor();


    void initializePropertyAdd();

    void showAddPropertyPanel();

    void clearPropertyAdd();

    void recomputeDocument();
    
    void createProperty(App::DocumentObject* obj, std::string& name, std::string& group);

    void setFirstInTabOrder(QWidget* widget);
    void setNextInTabOrder(QWidget* widget);

    void setTabOrderAddProperty();
    void setTabOrderFilter();

public Q_SLOTS:
    void valueChanged();
    void onAddProperty();

protected:
    void changeEvent(QEvent *e) override;

private:
    Ui_PropertyManager* ui;

    EditFinishedComboBox comboBoxGroupAdd;

    std::unordered_set<std::string> unsupportedTypes = {
        "App::PropertyVector", "App::PropertyVectorDistance", "App::PropertyMatrix",
        "App::PropertyRotation", "App::PropertyPlacement", "App::PropertyEnumeration"};

    QCompleter completerDocument;
    QCompleter completerTypeObject;
    QCompleter completerNameObject;
    QCompleter completerGroup;
    QCompleter completerTypeProperty;
    QCompleter completerNameProperty;

    QCompleter completerGroupAdd;
    QCompleter completerTypePropertyAdd;

    QWidget* widgetLastTabOrder;


    // state between adding properties
    std::unique_ptr<QWidget> editor;
    std::unique_ptr<QWidget> expressionEditor;
    std::string namePropertyToAdd;
    std::unique_ptr<PropertyEditor::PropertyItem> propertyItem;
    std::unique_ptr<App::ObjectIdentifier> objectIdentifier;
};

} // namespace Dialog
} // namespace Gui

#endif
