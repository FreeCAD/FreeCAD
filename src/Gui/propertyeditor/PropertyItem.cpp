/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#include "PreCompiled.h"

#ifndef _PreComp_
# include <algorithm>
# include <QComboBox>
# include <QFontDatabase>
# include <QLayout>
# include <QLocale>
# include <QPixmap>
# include <QSpinBox>
# include <QTextStream>
#endif

#include <Base/Tools.h>
#include <Base/UnitsApi.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/PropertyGeo.h>
#include <App/PropertyFile.h>
#include <App/PropertyUnits.h>
#include <Gui/Application.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Gui/ViewProviderDocumentObject.h>
#include <Gui/Placement.h>
#include <Gui/FileDialog.h>

#include "PropertyItem.h"

using namespace Gui::PropertyEditor;

TYPESYSTEM_SOURCE(Gui::PropertyEditor::PropertyItem, Base::BaseClass);

PropertyItem::PropertyItem() : parentItem(0), readonly(false)
{
    precision = Base::UnitsApi::getDecimals();
}

PropertyItem::~PropertyItem()
{
    qDeleteAll(childItems);
}

void PropertyItem::reset()
{
    qDeleteAll(childItems);
    childItems.clear();
}

void PropertyItem::setPropertyData(const std::vector<App::Property*>& items)
{
    propertyItems = items;
    bool ro = true;
    for (std::vector<App::Property*>::const_iterator it = items.begin();
        it != items.end(); ++it) {
        App::PropertyContainer* parent = (*it)->getContainer();
        if (parent)
            ro &= (parent->isReadOnly(*it) || (*it)->StatusBits.test(2));
    }
    this->setReadOnly(ro);
}

const std::vector<App::Property*>& PropertyItem::getPropertyData() const
{
    return propertyItems;
}

void PropertyItem::setParent(PropertyItem* parent)
{
    parentItem = parent;
}

PropertyItem *PropertyItem::parent() const
{
    return parentItem;
}

void PropertyItem::appendChild(PropertyItem *item)
{
    childItems.append(item);
}

PropertyItem *PropertyItem::child(int row)
{
    return childItems.value(row);
}

int PropertyItem::childCount() const
{
    return childItems.count();
}

int PropertyItem::columnCount() const
{
    return 2;
}

void PropertyItem::setReadOnly(bool ro)
{
    readonly = ro;
    for (QList<PropertyItem*>::iterator it = childItems.begin(); it != childItems.end(); ++it)
        (*it)->setReadOnly(ro);
}

bool PropertyItem::isReadOnly() const
{
    return readonly;
}

void PropertyItem::setDecimals(int prec)
{
    precision = prec;
}

int PropertyItem::decimals() const
{
    return precision;
}

QVariant PropertyItem::toolTip(const App::Property* prop) const
{
    return QVariant(QString::fromUtf8(prop->getDocumentation()));
}

QVariant PropertyItem::decoration(const App::Property* /*prop*/) const
{
    return QVariant();
}

QVariant PropertyItem::toString(const QVariant& prop) const
{
    return prop;
}

QVariant PropertyItem::value(const App::Property* /*prop*/) const
{
    return QVariant();
}

void PropertyItem::setValue(const QVariant& /*value*/)
{
}

QString PropertyItem::pythonIdentifier(const App::Property* prop) const
{
    App::PropertyContainer* parent = prop->getContainer();
    if (parent->getTypeId() == App::Document::getClassTypeId()) {
        App::Document* doc = static_cast<App::Document*>(parent);
        QString docName = QString::fromAscii(App::GetApplication().getDocumentName(doc));
        QString propName = QString::fromAscii(parent->getName(prop));
        return QString::fromAscii("FreeCAD.getDocument(\"%1\").%2").arg(docName).arg(propName);
    }
    if (parent->getTypeId().isDerivedFrom(App::DocumentObject::getClassTypeId())) {
        App::DocumentObject* obj = static_cast<App::DocumentObject*>(parent);
        App::Document* doc = obj->getDocument();
        QString docName = QString::fromAscii(App::GetApplication().getDocumentName(doc));
        QString objName = QString::fromAscii(obj->getNameInDocument());
        QString propName = QString::fromAscii(parent->getName(prop));
        return QString::fromAscii("FreeCAD.getDocument(\"%1\").getObject(\"%2\").%3")
            .arg(docName).arg(objName).arg(propName);
    }
    if (parent->getTypeId().isDerivedFrom(Gui::ViewProviderDocumentObject::getClassTypeId())) {
        App::DocumentObject* obj = static_cast<Gui::ViewProviderDocumentObject*>(parent)->getObject();
        App::Document* doc = obj->getDocument();
        QString docName = QString::fromAscii(App::GetApplication().getDocumentName(doc));
        QString objName = QString::fromAscii(obj->getNameInDocument());
        QString propName = QString::fromAscii(parent->getName(prop));
        return QString::fromAscii("FreeCADGui.getDocument(\"%1\").getObject(\"%2\").%3")
            .arg(docName).arg(objName).arg(propName);
    }
    return QString();
}

QWidget* PropertyItem::createEditor(QWidget* /*parent*/, const QObject* /*receiver*/, const char* /*method*/) const
{
    return 0;
}

void PropertyItem::setEditorData(QWidget * /*editor*/, const QVariant& /*data*/) const
{
}

QVariant PropertyItem::editorData(QWidget * /*editor*/) const
{
    return QVariant();
}

QString PropertyItem::propertyName() const
{
    if (propName.isEmpty())
        return QLatin1String("<empty>");
    return propName;
}

void PropertyItem::setPropertyName(const QString& name)
{
    setObjectName(name);
    QString display;
    bool upper = false;
    for (int i=0; i<name.length(); i++) {
        if (name[i].isUpper() && !display.isEmpty()) {
            // if there is a sequence of capital letters do not insert spaces
            if (!upper)
                display += QLatin1String(" ");
        }
        upper = name[i].isUpper();
        display += name[i];
    }

    propName = display;
}

void PropertyItem::setPropertyValue(const QString& value)
{
    for (std::vector<App::Property*>::const_iterator it = propertyItems.begin();
        it != propertyItems.end(); ++it) {
        App::PropertyContainer* parent = (*it)->getContainer();
        if (parent && !parent->isReadOnly(*it) && !(*it)->StatusBits.test(2)) {
            QString cmd = QString::fromAscii("%1 = %2").arg(pythonIdentifier(*it)).arg(value);
            Gui::Application::Instance->runPythonCode((const char*)cmd.toUtf8());
        }
    }
}

QVariant PropertyItem::data(int column, int role) const
{
    // property name
    if (column == 0) {
        if (role == Qt::DisplayRole)
            return propertyName();
        // no properties set
        if (propertyItems.empty())
            return QVariant();
        else if (role == Qt::ToolTipRole)
            return toolTip(propertyItems[0]);
        else
            return QVariant();
    }
    else {
        // no properties set
        if (propertyItems.empty()) {
            PropertyItem* parent = this->parent();
            if (!parent || !parent->parent())
                return QVariant();
            if (role == Qt::EditRole)
                return parent->property(qPrintable(objectName()));
            else if (role == Qt::DisplayRole) {
                QVariant val = parent->property(qPrintable(objectName()));
                return toString(val);

            }
            else
                return QVariant();
        }
        if (role == Qt::EditRole)
            return value(propertyItems[0]);
        else if (role == Qt::DecorationRole)
            return decoration(propertyItems[0]);
        else if (role == Qt::DisplayRole)
            return toString(value(propertyItems[0]));
        else if (role == Qt::ToolTipRole)
            return toolTip(propertyItems[0]);
        else
            return QVariant();
    }
}

bool PropertyItem::setData (const QVariant& value)
{
    // This is the basic mechanism to set the value to
    // a property and if no property is set for this item
    // it delegates it to its parent which sets then the
    // property or delegates again to its parent...
    if (propertyItems.empty()) {
        PropertyItem* parent = this->parent();
        if (!parent || !parent->parent())
            return false;
        parent->setProperty(qPrintable(objectName()),value);
        return true;
    }
    else {
        setValue(value);
        return true;
    }
}

Qt::ItemFlags PropertyItem::flags(int column) const
{
    Qt::ItemFlags basicFlags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if (column == 1 && !isReadOnly())
        return basicFlags | Qt::ItemIsEditable;
    else
        return basicFlags;
}

int PropertyItem::row() const
{
    if (parentItem)
        return parentItem->childItems.indexOf(const_cast<PropertyItem*>(this));

    return 0;
}

// --------------------------------------------------------------------

TYPESYSTEM_SOURCE(Gui::PropertyEditor::PropertyStringItem, Gui::PropertyEditor::PropertyItem);

PropertyStringItem::PropertyStringItem()
{
}

QVariant PropertyStringItem::value(const App::Property* prop) const
{
    assert(prop && prop->getTypeId().isDerivedFrom(App::PropertyString::getClassTypeId()));

    std::string value = static_cast<const App::PropertyString*>(prop)->getValue();
    return QVariant(QString::fromUtf8(value.c_str()));
}

void PropertyStringItem::setValue(const QVariant& value)
{
    if (!value.canConvert(QVariant::String))
        return;
    QString val = value.toString();
    QString data = QString::fromAscii("\"%1\"").arg(val);
    setPropertyValue(data);
}

QWidget* PropertyStringItem::createEditor(QWidget* parent, const QObject* receiver, const char* method) const
{
    QLineEdit *le = new QLineEdit(parent);
    le->setFrame(false);
    QObject::connect(le, SIGNAL(textChanged(const QString&)), receiver, method);
    return le;
}

void PropertyStringItem::setEditorData(QWidget *editor, const QVariant& data) const
{
    QLineEdit *le = qobject_cast<QLineEdit*>(editor);
    le->setText(data.toString());
}

QVariant PropertyStringItem::editorData(QWidget *editor) const
{
    QLineEdit *le = qobject_cast<QLineEdit*>(editor);
    return QVariant(le->text());
}

// --------------------------------------------------------------------

TYPESYSTEM_SOURCE(Gui::PropertyEditor::PropertyFontItem, Gui::PropertyEditor::PropertyItem);

PropertyFontItem::PropertyFontItem()
{
}

QVariant PropertyFontItem::value(const App::Property* prop) const
{
    assert(prop && prop->getTypeId().isDerivedFrom(App::PropertyFont::getClassTypeId()));

    std::string value = static_cast<const App::PropertyFont*>(prop)->getValue();
    return QVariant(QString::fromUtf8(value.c_str()));
}

void PropertyFontItem::setValue(const QVariant& value)
{
    if (!value.canConvert(QVariant::String))
        return;
    QString val = value.toString();
    QString data = QString::fromAscii("\"%1\"").arg(val);
    setPropertyValue(data);
}

QWidget* PropertyFontItem::createEditor(QWidget* parent, const QObject* receiver, const char* method) const
{
    QComboBox *cb = new QComboBox(parent);
    cb->setFrame(false);
    QObject::connect(cb, SIGNAL(activated(const QString&)), receiver, method);
    return cb;
}

void PropertyFontItem::setEditorData(QWidget *editor, const QVariant& data) const
{
    QComboBox *cb = qobject_cast<QComboBox*>(editor);
    QFontDatabase fdb;
    QStringList familyNames = fdb.families(QFontDatabase::Any);
    cb->addItems(familyNames);
    int index = familyNames.indexOf(data.toString());
    cb->setCurrentIndex(index);
}

QVariant PropertyFontItem::editorData(QWidget *editor) const
{
    QComboBox *cb = qobject_cast<QComboBox*>(editor);
    return QVariant(cb->currentText());
}

// --------------------------------------------------------------------

TYPESYSTEM_SOURCE(Gui::PropertyEditor::PropertySeparatorItem, Gui::PropertyEditor::PropertyItem);

QWidget* PropertySeparatorItem::createEditor(QWidget* parent, const QObject* receiver, const char* method) const
{
    return 0;
}

// --------------------------------------------------------------------

TYPESYSTEM_SOURCE(Gui::PropertyEditor::PropertyIntegerItem, Gui::PropertyEditor::PropertyItem);

PropertyIntegerItem::PropertyIntegerItem()
{
}

QVariant PropertyIntegerItem::value(const App::Property* prop) const
{
    assert(prop && prop->getTypeId().isDerivedFrom(App::PropertyInteger::getClassTypeId()));

    int value = (int)static_cast<const App::PropertyInteger*>(prop)->getValue();
    return QVariant(value);
}

void PropertyIntegerItem::setValue(const QVariant& value)
{
    if (!value.canConvert(QVariant::Int))
        return;
    int val = value.toInt();
    QString data = QString::fromAscii("%1").arg(val);
    setPropertyValue(data);
}

QWidget* PropertyIntegerItem::createEditor(QWidget* parent, const QObject* receiver, const char* method) const
{
    QSpinBox *sb = new QSpinBox(parent);
    sb->setFrame(false);
    QObject::connect(sb, SIGNAL(valueChanged(int)), receiver, method);
    return sb;
}

void PropertyIntegerItem::setEditorData(QWidget *editor, const QVariant& data) const
{
    QSpinBox *sb = qobject_cast<QSpinBox*>(editor);
    sb->setRange(INT_MIN, INT_MAX);
    sb->setValue(data.toInt());
}

QVariant PropertyIntegerItem::editorData(QWidget *editor) const
{
    QSpinBox *sb = qobject_cast<QSpinBox*>(editor);
    return QVariant(sb->value());
}

// --------------------------------------------------------------------

TYPESYSTEM_SOURCE(Gui::PropertyEditor::PropertyIntegerConstraintItem, Gui::PropertyEditor::PropertyItem);

PropertyIntegerConstraintItem::PropertyIntegerConstraintItem()
{
}

QVariant PropertyIntegerConstraintItem::value(const App::Property* prop) const
{
    assert(prop && prop->getTypeId().isDerivedFrom(App::PropertyIntegerConstraint::getClassTypeId()));

    int value = (int)static_cast<const App::PropertyIntegerConstraint*>(prop)->getValue();
    return QVariant(value);
}

void PropertyIntegerConstraintItem::setValue(const QVariant& value)
{
    if (!value.canConvert(QVariant::Int))
        return;
    int val = value.toInt();
    QString data = QString::fromAscii("%1").arg(val);
    setPropertyValue(data);
}

QWidget* PropertyIntegerConstraintItem::createEditor(QWidget* parent, const QObject* receiver, const char* method) const
{
    QSpinBox *sb = new QSpinBox(parent);
    sb->setFrame(false);
    QObject::connect(sb, SIGNAL(valueChanged(int)), receiver, method);
    return sb;
}

void PropertyIntegerConstraintItem::setEditorData(QWidget *editor, const QVariant& /*data*/) const
{
    const std::vector<App::Property*>& items = getPropertyData();
    App::PropertyIntegerConstraint* prop = (App::PropertyIntegerConstraint*)items[0];

    const App::PropertyIntegerConstraint::Constraints* c = 
        ((App::PropertyIntegerConstraint*)prop)->getConstraints();
    QSpinBox *sb = qobject_cast<QSpinBox*>(editor);
    if (c) {
        sb->setMinimum(c->LowerBound);
        sb->setMaximum(c->UpperBound);
        sb->setSingleStep(c->StepSize);
    }
    else {
        sb->setMinimum(INT_MIN);
        sb->setMaximum(INT_MAX);
    }
    sb->setValue(prop->getValue());
}

QVariant PropertyIntegerConstraintItem::editorData(QWidget *editor) const
{
    QSpinBox *sb = qobject_cast<QSpinBox*>(editor);
    return QVariant(sb->value());
}

// --------------------------------------------------------------------

TYPESYSTEM_SOURCE(Gui::PropertyEditor::PropertyFloatItem, Gui::PropertyEditor::PropertyItem);

PropertyFloatItem::PropertyFloatItem()
{
}

QVariant PropertyFloatItem::toString(const QVariant& prop) const
{
    double value = prop.toDouble();
    QString data = QLocale::system().toString(value, 'f', decimals());
    const std::vector<App::Property*>& props = getPropertyData();
    if (!props.empty()) {
        if (props.front()->getTypeId().isDerivedFrom(App::PropertyDistance::getClassTypeId())) {
            QString unit = QString::fromAscii("mm");
            unit.prepend(QLatin1String(" "));
            data += unit;
        }
        else if (props.front()->getTypeId().isDerivedFrom(App::PropertyLength::getClassTypeId())) {
            QString unit = QString::fromAscii("mm");
            unit.prepend(QLatin1String(" "));
            data += unit;
        }
        else if (props.front()->getTypeId().isDerivedFrom(App::PropertySpeed::getClassTypeId())) {
            //QString unit = Base::UnitsApi::getPrefUnitOf(Base::Acceleration);
            //unit.prepend(QLatin1String(" "));
            //data += unit;
        }
        else if (props.front()->getTypeId().isDerivedFrom(App::PropertyAcceleration::getClassTypeId())) {
            QString unit = QString::fromAscii("mm/s^2");
            unit.prepend(QLatin1String(" "));
            data += unit;
        }
    }

    return QVariant(data);
}

QVariant PropertyFloatItem::value(const App::Property* prop) const
{
    assert(prop && prop->getTypeId().isDerivedFrom(App::PropertyFloat::getClassTypeId()));

    double value = static_cast<const App::PropertyFloat*>(prop)->getValue();
    return QVariant(value);
}

void PropertyFloatItem::setValue(const QVariant& value)
{
    if (!value.canConvert(QVariant::Double))
        return;
    double val = value.toDouble();
    QString data = QString::fromAscii("%1").arg(val,0,'f',decimals());
    setPropertyValue(data);
}

QWidget* PropertyFloatItem::createEditor(QWidget* parent, const QObject* receiver, const char* method) const
{
    QDoubleSpinBox *sb = new QDoubleSpinBox(parent);
    sb->setFrame(false);
    sb->setDecimals(decimals());
    QObject::connect(sb, SIGNAL(valueChanged(double)), receiver, method);
    return sb;
}

void PropertyFloatItem::setEditorData(QWidget *editor, const QVariant& data) const
{
    QDoubleSpinBox *sb = qobject_cast<QDoubleSpinBox*>(editor);
    sb->setRange((double)INT_MIN, (double)INT_MAX);
    sb->setValue(data.toDouble());
    const std::vector<App::Property*>& prop = getPropertyData();
    if (prop.empty())
        return;
    else if (prop.front()->getTypeId().isDerivedFrom(App::PropertyDistance::getClassTypeId())) {
        QString unit = QString::fromAscii("mm");
        unit.prepend(QLatin1String(" "));
        sb->setSuffix(unit);
    }
    else if (prop.front()->getTypeId().isDerivedFrom(App::PropertyLength::getClassTypeId())) {
        sb->setMinimum(0.0);
        QString unit = QString::fromAscii("mm");
        unit.prepend(QLatin1String(" "));
        sb->setSuffix(unit);
    }
    else if (prop.front()->getTypeId().isDerivedFrom(App::PropertySpeed::getClassTypeId())) {
        //sb->setMinimum(0.0);
        //QString unit = Base::UnitsApi::getPrefUnitOf(Base::Acceleration);
        //unit.prepend(QLatin1String(" "));
        //sb->setSuffix(unit);
    }
    else if (prop.front()->getTypeId().isDerivedFrom(App::PropertyAcceleration::getClassTypeId())) {
        sb->setMinimum(0.0);
        QString unit = QString::fromAscii("mm/s^2");
        unit.prepend(QLatin1String(" "));
        sb->setSuffix(unit);
    }
}

QVariant PropertyFloatItem::editorData(QWidget *editor) const
{
    QDoubleSpinBox *sb = qobject_cast<QDoubleSpinBox*>(editor);
    return QVariant(sb->value());
}

// --------------------------------------------------------------------


TYPESYSTEM_SOURCE(Gui::PropertyEditor::PropertyUnitItem, Gui::PropertyEditor::PropertyItem);

PropertyUnitItem::PropertyUnitItem()
{
}

QVariant PropertyUnitItem::toString(const QVariant& Value) const
{
    double val = Value.toDouble();

    QString unit;
    const std::vector<App::Property*>& prop = getPropertyData();
    if (!prop.empty() && prop.front()->getTypeId().isDerivedFrom(App::PropertyQuantity::getClassTypeId())) {
        Base::Quantity value = static_cast<const App::PropertyQuantity*>(prop.front())->getQuantityValue();
        unit = value.getUserString();        
    }

    return QVariant(unit);
}

QVariant PropertyUnitItem::value(const App::Property* prop) const
{
    assert(prop && prop->getTypeId().isDerivedFrom(App::PropertyQuantity::getClassTypeId()));
    Base::Quantity value = static_cast<const App::PropertyQuantity*>(prop)->getQuantityValue();
    QString unitString;
    return QVariant(value.getValue());
}

void PropertyUnitItem::setValue(const QVariant& value)
{
    if (!value.canConvert(QVariant::Double))
        return;
    double val = value.toDouble();

    QString unit;
    const std::vector<App::Property*>& prop = getPropertyData();
    if (prop.empty())
        return;
    else if (prop.front()->getTypeId().isDerivedFrom(App::PropertyQuantity::getClassTypeId())) {
        Base::Quantity value = static_cast<const App::PropertyQuantity*>(prop.front())->getQuantityValue();
        unit = value.getUserString();
    }

    setPropertyValue(unit);
}

QWidget* PropertyUnitItem::createEditor(QWidget* parent, const QObject* receiver, const char* method) const
{
    QDoubleSpinBox *sb = new QDoubleSpinBox(parent);
    sb->setFrame(false);
    sb->setDecimals(decimals());
    QObject::connect(sb, SIGNAL(valueChanged(double)), receiver, method);
    return sb;
}

void PropertyUnitItem::setEditorData(QWidget *editor, const QVariant& data) const
{
    QDoubleSpinBox *sb = qobject_cast<QDoubleSpinBox*>(editor);
    sb->setRange((double)INT_MIN, (double)INT_MAX);
    sb->setValue(data.toDouble());
    const std::vector<App::Property*>& prop = getPropertyData();
    if (prop.empty())
        return;
    else if (prop.front()->getTypeId().isDerivedFrom(App::PropertyQuantity::getClassTypeId())) {
        Base::Quantity value = static_cast<const App::PropertyQuantity*>(prop.front())->getQuantityValue();
        QString unitString;
        //double factor;
        //value.getUserString(unitString);
        //unitString.prepend(QLatin1String(" "));
        //sb->setSuffix(unitString);
    }
}

QVariant PropertyUnitItem::editorData(QWidget *editor) const
{
    QDoubleSpinBox *sb = qobject_cast<QDoubleSpinBox*>(editor);
    return QVariant(sb->value());
}

// --------------------------------------------------------------------

TYPESYSTEM_SOURCE(Gui::PropertyEditor::PropertyFloatConstraintItem, Gui::PropertyEditor::PropertyItem);

PropertyFloatConstraintItem::PropertyFloatConstraintItem()
{
}

QVariant PropertyFloatConstraintItem::toString(const QVariant& prop) const
{
    double value = prop.toDouble();
    QString data = QLocale::system().toString(value, 'f', decimals());
    return QVariant(data);
}

QVariant PropertyFloatConstraintItem::value(const App::Property* prop) const
{
    assert(prop && prop->getTypeId().isDerivedFrom(App::PropertyFloatConstraint::getClassTypeId()));

    double value = static_cast<const App::PropertyFloatConstraint*>(prop)->getValue();
    return QVariant(value);
}

void PropertyFloatConstraintItem::setValue(const QVariant& value)
{
    if (!value.canConvert(QVariant::Double))
        return;
    double val = value.toDouble();
    QString data = QString::fromAscii("%1").arg(val,0,'f',decimals());
    setPropertyValue(data);
}

QWidget* PropertyFloatConstraintItem::createEditor(QWidget* parent, const QObject* receiver, const char* method) const
{
    QDoubleSpinBox *sb = new QDoubleSpinBox(parent);
    sb->setDecimals(decimals());
    sb->setFrame(false);
    QObject::connect(sb, SIGNAL(valueChanged(double)), receiver, method);
    return sb;
}

void PropertyFloatConstraintItem::setEditorData(QWidget *editor, const QVariant& /*data*/) const
{
    const std::vector<App::Property*>& items = getPropertyData();
    App::PropertyFloatConstraint* prop = (App::PropertyFloatConstraint*)items[0];

    const App::PropertyFloatConstraint::Constraints* c = ((App::PropertyFloatConstraint*)prop)->getConstraints();
    QDoubleSpinBox *sb = qobject_cast<QDoubleSpinBox*>(editor);
    if (c) {
        sb->setMinimum(c->LowerBound);
        sb->setMaximum(c->UpperBound);
        sb->setSingleStep(c->StepSize);
    }
    else {
        sb->setMinimum((double)INT_MIN);
        sb->setMaximum((double)INT_MAX);
        sb->setSingleStep(0.1);
    }
    sb->setValue(prop->getValue());
}

QVariant PropertyFloatConstraintItem::editorData(QWidget *editor) const
{
    QDoubleSpinBox *sb = qobject_cast<QDoubleSpinBox*>(editor);
    return QVariant(sb->value());
}

// --------------------------------------------------------------------

TYPESYSTEM_SOURCE(Gui::PropertyEditor::PropertyAngleItem, Gui::PropertyEditor::PropertyFloatItem);

PropertyAngleItem::PropertyAngleItem()
{
}

void PropertyAngleItem::setEditorData(QWidget *editor, const QVariant& data) const
{
    const App::PropertyFloatConstraint::Constraints* c = 0;
    const std::vector<App::Property*>& items = getPropertyData();
    if (!items.empty()) {
        App::PropertyAngle* prop = static_cast<App::PropertyAngle*>(items[0]);
        c = prop->getConstraints();
    }

    QDoubleSpinBox *sb = qobject_cast<QDoubleSpinBox*>(editor);
    if (c) {
        sb->setMinimum(c->LowerBound);
        sb->setMaximum(c->UpperBound);
        sb->setSingleStep(c->StepSize);
    }
    else {
        sb->setMinimum((double)INT_MIN);
        sb->setMaximum((double)INT_MAX);
        sb->setSingleStep(1.0);
    }

    sb->setValue(data.toDouble());
    sb->setSuffix(QString::fromUtf8(" \xc2\xb0"));
}

QVariant PropertyAngleItem::toString(const QVariant& prop) const
{
    double value = prop.toDouble();
    QString data = QString::fromUtf8("%1 \xc2\xb0")
        .arg(QLocale::system().toString(value, 'f', decimals()));
    return QVariant(data);
}

// --------------------------------------------------------------------

TYPESYSTEM_SOURCE(Gui::PropertyEditor::PropertyBoolItem, Gui::PropertyEditor::PropertyItem);

PropertyBoolItem::PropertyBoolItem()
{
}

QVariant PropertyBoolItem::value(const App::Property* prop) const
{
    assert(prop && prop->getTypeId().isDerivedFrom(App::PropertyBool::getClassTypeId()));
    
    bool value = ((App::PropertyBool*)prop)->getValue();
    return QVariant(value);
}

void PropertyBoolItem::setValue(const QVariant& value)
{
    if (!value.canConvert(QVariant::Bool))
        return;
    bool val = value.toBool();
    QString data = (val ? QLatin1String("True") : QLatin1String("False"));
    setPropertyValue(data);
}

QWidget* PropertyBoolItem::createEditor(QWidget* parent, const QObject* receiver, const char* method) const
{
    QComboBox *cb = new QComboBox(parent);
    cb->setFrame(false);
    cb->addItem(QLatin1String("false"));
    cb->addItem(QLatin1String("true"));
    QObject::connect(cb, SIGNAL(activated(int)), receiver, method);
    return cb;
}

void PropertyBoolItem::setEditorData(QWidget *editor, const QVariant& data) const
{
    QComboBox *cb = qobject_cast<QComboBox*>(editor);
    cb->setCurrentIndex(cb->findText(data.toString()));
}

QVariant PropertyBoolItem::editorData(QWidget *editor) const
{
    QComboBox *cb = qobject_cast<QComboBox*>(editor);
    return QVariant(cb->currentText());
}

// ---------------------------------------------------------------

TYPESYSTEM_SOURCE(Gui::PropertyEditor::PropertyVectorItem, Gui::PropertyEditor::PropertyItem);

PropertyVectorItem::PropertyVectorItem()
{
    m_x = static_cast<PropertyFloatItem*>(PropertyFloatItem::create());
    m_x->setParent(this);
    m_x->setPropertyName(QLatin1String("x"));
    this->appendChild(m_x);
    m_y = static_cast<PropertyFloatItem*>(PropertyFloatItem::create());
    m_y->setParent(this);
    m_y->setPropertyName(QLatin1String("y"));
    this->appendChild(m_y);
    m_z = static_cast<PropertyFloatItem*>(PropertyFloatItem::create());
    m_z->setParent(this);
    m_z->setPropertyName(QLatin1String("z"));
    this->appendChild(m_z);
}

QVariant PropertyVectorItem::toString(const QVariant& prop) const
{
    const Base::Vector3d& value = prop.value<Base::Vector3d>();
    QString data = QString::fromAscii("[%1 %2 %3]")
        .arg(QLocale::system().toString(value.x, 'f', 2))
        .arg(QLocale::system().toString(value.y, 'f', 2))
        .arg(QLocale::system().toString(value.z, 'f', 2));
    return QVariant(data);
}

QVariant PropertyVectorItem::value(const App::Property* prop) const
{
    assert(prop && prop->getTypeId().isDerivedFrom(App::PropertyVector::getClassTypeId()));

    const Base::Vector3d& value = static_cast<const App::PropertyVector*>(prop)->getValue();
    return QVariant::fromValue<Base::Vector3d>(value);
}

void PropertyVectorItem::setValue(const QVariant& value)
{
    if (!value.canConvert<Base::Vector3d>())
        return;
    const Base::Vector3d& val = value.value<Base::Vector3d>();
    QString data = QString::fromAscii("(%1, %2, %3)")
                    .arg(val.x,0,'f',decimals())
                    .arg(val.y,0,'f',decimals())
                    .arg(val.z,0,'f',decimals());
    setPropertyValue(data);
}

QWidget* PropertyVectorItem::createEditor(QWidget* parent, const QObject* /*receiver*/, const char* /*method*/) const
{
    QLineEdit *le = new QLineEdit(parent);
    le->setFrame(false);
    le->setReadOnly(true);
    return le;
}

void PropertyVectorItem::setEditorData(QWidget *editor, const QVariant& data) const
{
    QLineEdit* le = qobject_cast<QLineEdit*>(editor);
    const Base::Vector3d& value = data.value<Base::Vector3d>();
    QString text = QString::fromAscii("[%1 %2 %3]")
        .arg(QLocale::system().toString(value.x, 'f', 2))
        .arg(QLocale::system().toString(value.y, 'f', 2))
        .arg(QLocale::system().toString(value.z, 'f', 2));
    le->setText(text);
}

QVariant PropertyVectorItem::editorData(QWidget *editor) const
{
    QLineEdit *le = qobject_cast<QLineEdit*>(editor);
    return QVariant(le->text());
}

double PropertyVectorItem::x() const
{
    return data(1,Qt::EditRole).value<Base::Vector3d>().x;
}

void PropertyVectorItem::setX(double x)
{
    setData(QVariant::fromValue(Base::Vector3d(x, y(), z())));
}

double PropertyVectorItem::y() const
{
    return data(1,Qt::EditRole).value<Base::Vector3d>().y;
}

void PropertyVectorItem::setY(double y)
{
    setData(QVariant::fromValue(Base::Vector3d(x(), y, z())));
}

double PropertyVectorItem::z() const
{
    return data(1,Qt::EditRole).value<Base::Vector3d>().z;
}

void PropertyVectorItem::setZ(double z)
{
    setData(QVariant::fromValue(Base::Vector3d(x(), y(), z)));
}

// ---------------------------------------------------------------

TYPESYSTEM_SOURCE(Gui::PropertyEditor::PropertyMatrixItem, Gui::PropertyEditor::PropertyItem);

PropertyMatrixItem::PropertyMatrixItem()
{
    const int decimals=16;
    m_a11 = static_cast<PropertyFloatItem*>(PropertyFloatItem::create());
    m_a11->setParent(this);
    m_a11->setPropertyName(QLatin1String("A11"));
    m_a11->setDecimals(decimals);
    this->appendChild(m_a11);
    m_a12 = static_cast<PropertyFloatItem*>(PropertyFloatItem::create());
    m_a12->setParent(this);
    m_a12->setPropertyName(QLatin1String("A12"));
    m_a12->setDecimals(decimals);
    this->appendChild(m_a12);
    m_a13 = static_cast<PropertyFloatItem*>(PropertyFloatItem::create());
    m_a13->setParent(this);
    m_a13->setPropertyName(QLatin1String("A13"));
    m_a13->setDecimals(decimals);
    this->appendChild(m_a13);
    m_a14 = static_cast<PropertyFloatItem*>(PropertyFloatItem::create());
    m_a14->setParent(this);
    m_a14->setPropertyName(QLatin1String("A14"));
    m_a14->setDecimals(decimals);
    this->appendChild(m_a14);
    m_a21 = static_cast<PropertyFloatItem*>(PropertyFloatItem::create());
    m_a21->setParent(this);
    m_a21->setPropertyName(QLatin1String("A21"));
    m_a21->setDecimals(decimals);
    this->appendChild(m_a21);
    m_a22 = static_cast<PropertyFloatItem*>(PropertyFloatItem::create());
    m_a22->setParent(this);
    m_a22->setPropertyName(QLatin1String("A22"));
    m_a22->setDecimals(decimals);
    this->appendChild(m_a22);
    m_a23 = static_cast<PropertyFloatItem*>(PropertyFloatItem::create());
    m_a23->setParent(this);
    m_a23->setPropertyName(QLatin1String("A23"));
    m_a23->setDecimals(decimals);
    this->appendChild(m_a23);
    m_a24 = static_cast<PropertyFloatItem*>(PropertyFloatItem::create());
    m_a24->setParent(this);
    m_a24->setPropertyName(QLatin1String("A24"));
    m_a24->setDecimals(decimals);
    this->appendChild(m_a24);
    m_a31 = static_cast<PropertyFloatItem*>(PropertyFloatItem::create());
    m_a31->setParent(this);
    m_a31->setPropertyName(QLatin1String("A31"));
    m_a31->setDecimals(decimals);
    this->appendChild(m_a31);
    m_a32 = static_cast<PropertyFloatItem*>(PropertyFloatItem::create());
    m_a32->setParent(this);
    m_a32->setPropertyName(QLatin1String("A32"));
    m_a32->setDecimals(decimals);
    this->appendChild(m_a32);
    m_a33 = static_cast<PropertyFloatItem*>(PropertyFloatItem::create());
    m_a33->setParent(this);
    m_a33->setPropertyName(QLatin1String("A33"));
    m_a33->setDecimals(decimals);
    this->appendChild(m_a33);
    m_a34 = static_cast<PropertyFloatItem*>(PropertyFloatItem::create());
    m_a34->setParent(this);
    m_a34->setPropertyName(QLatin1String("A34"));
    m_a34->setDecimals(decimals);
    this->appendChild(m_a34);
    m_a41 = static_cast<PropertyFloatItem*>(PropertyFloatItem::create());
    m_a41->setParent(this);
    m_a41->setPropertyName(QLatin1String("A41"));
    m_a41->setDecimals(decimals);
    this->appendChild(m_a41);
    m_a42 = static_cast<PropertyFloatItem*>(PropertyFloatItem::create());
    m_a42->setParent(this);
    m_a42->setPropertyName(QLatin1String("A42"));
    m_a42->setDecimals(decimals);
    this->appendChild(m_a42);
    m_a43 = static_cast<PropertyFloatItem*>(PropertyFloatItem::create());
    m_a43->setParent(this);
    m_a43->setPropertyName(QLatin1String("A43"));
    m_a43->setDecimals(decimals);
    this->appendChild(m_a43);
    m_a44 = static_cast<PropertyFloatItem*>(PropertyFloatItem::create());
    m_a44->setParent(this);
    m_a44->setPropertyName(QLatin1String("A44"));
    m_a44->setDecimals(decimals);
    this->appendChild(m_a44);
}

QVariant PropertyMatrixItem::toString(const QVariant& prop) const
{
    const Base::Matrix4D& value = prop.value<Base::Matrix4D>();
    QString text = QString::fromAscii("[%1 %2 %3 %4 %5 %6 %7 %8 %9 %10 %11 %12 %13 %14 %15 %16]")
        .arg(QLocale::system().toString(value[0][0], 'f', 2)) //(unsigned short usNdx)
        .arg(QLocale::system().toString(value[0][1], 'f', 2))
        .arg(QLocale::system().toString(value[0][2], 'f', 2))
        .arg(QLocale::system().toString(value[0][3], 'f', 2))
        .arg(QLocale::system().toString(value[1][0], 'f', 2))
        .arg(QLocale::system().toString(value[1][1], 'f', 2))
        .arg(QLocale::system().toString(value[1][2], 'f', 2))
        .arg(QLocale::system().toString(value[1][3], 'f', 2))
        .arg(QLocale::system().toString(value[2][0], 'f', 2))
        .arg(QLocale::system().toString(value[2][1], 'f', 2))
        .arg(QLocale::system().toString(value[2][2], 'f', 2))
        .arg(QLocale::system().toString(value[2][3], 'f', 2))
        .arg(QLocale::system().toString(value[3][0], 'f', 2))
        .arg(QLocale::system().toString(value[3][1], 'f', 2))
        .arg(QLocale::system().toString(value[3][2], 'f', 2))
        .arg(QLocale::system().toString(value[3][3], 'f', 2));
    return QVariant(text);
}

QVariant PropertyMatrixItem::value(const App::Property* prop) const
{
    assert(prop && prop->getTypeId().isDerivedFrom(App::PropertyMatrix::getClassTypeId()));

    const Base::Matrix4D& value = static_cast<const App::PropertyMatrix*>(prop)->getValue();
    return QVariant::fromValue<Base::Matrix4D>(value);
}

QVariant PropertyMatrixItem::toolTip(const App::Property* prop) const
{
    assert(prop && prop->getTypeId().isDerivedFrom(App::PropertyMatrix::getClassTypeId()));

    const Base::Matrix4D& value = static_cast<const App::PropertyMatrix*>(prop)->getValue();
    return QVariant(QString::fromStdString(value.analyse()));
}

void PropertyMatrixItem::setValue(const QVariant& value)
{
    if (!value.canConvert<Base::Matrix4D>())
        return;
    const Base::Matrix4D& val = value.value<Base::Matrix4D>();
    const int decimals=16;
    QString data = QString::fromAscii("FreeCAD.Matrix(%1, %2, %3, %4, %5, %6, %7, %8, %9, %10, %11, %12, %13, %14, %15, %16)")
        .arg(val[0][0],0, 'f', decimals)
        .arg(val[0][1],0, 'f', decimals)
        .arg(val[0][2],0, 'f', decimals)
        .arg(val[0][3],0, 'f', decimals)
        .arg(val[1][0],0, 'f', decimals)
        .arg(val[1][1],0, 'f', decimals)
        .arg(val[1][2],0, 'f', decimals)
        .arg(val[1][3],0, 'f', decimals)
        .arg(val[2][0],0, 'f', decimals)
        .arg(val[2][1],0, 'f', decimals)
        .arg(val[2][2],0, 'f', decimals)
        .arg(val[2][3],0, 'f', decimals)
        .arg(val[3][0],0, 'f', decimals)
        .arg(val[3][1],0, 'f', decimals)
        .arg(val[3][2],0, 'f', decimals)
        .arg(val[3][3],0, 'f', decimals);
    setPropertyValue(data);
}

QWidget* PropertyMatrixItem::createEditor(QWidget* parent, const QObject* /*receiver*/, const char* /*method*/) const
{
    QLineEdit *le = new QLineEdit(parent);
    le->setFrame(false);
    le->setReadOnly(true);
    return le;
}

void PropertyMatrixItem::setEditorData(QWidget *editor, const QVariant& data) const
{
    QLineEdit* le = qobject_cast<QLineEdit*>(editor);
    const Base::Matrix4D& value = data.value<Base::Matrix4D>();
    QString text = QString::fromAscii("[%1 %2 %3 %4 %5 %6 %7 %8 %9 %10 %11 %12 %13 %14 %15 %16]")
        .arg(QLocale::system().toString(value[0][0], 'f', 2)) //(unsigned short usNdx)
        .arg(QLocale::system().toString(value[0][1], 'f', 2))
        .arg(QLocale::system().toString(value[0][2], 'f', 2))
        .arg(QLocale::system().toString(value[0][3], 'f', 2))
        .arg(QLocale::system().toString(value[1][0], 'f', 2))
        .arg(QLocale::system().toString(value[1][1], 'f', 2))
        .arg(QLocale::system().toString(value[1][2], 'f', 2))
        .arg(QLocale::system().toString(value[1][3], 'f', 2))
        .arg(QLocale::system().toString(value[2][0], 'f', 2))
        .arg(QLocale::system().toString(value[2][1], 'f', 2))
        .arg(QLocale::system().toString(value[2][2], 'f', 2))
        .arg(QLocale::system().toString(value[2][3], 'f', 2))
        .arg(QLocale::system().toString(value[3][0], 'f', 2))
        .arg(QLocale::system().toString(value[3][1], 'f', 2))
        .arg(QLocale::system().toString(value[3][2], 'f', 2))
        .arg(QLocale::system().toString(value[3][3], 'f', 2));
    le->setText(text);
}

QVariant PropertyMatrixItem::editorData(QWidget *editor) const
{
    QLineEdit *le = qobject_cast<QLineEdit*>(editor);
    return QVariant(le->text());
}

double PropertyMatrixItem::getA11() const
{
    return data(1,Qt::EditRole).value<Base::Matrix4D>()[0][0];
}

void PropertyMatrixItem::setA11(double A11)
{
    setData(QVariant::fromValue(Base::Matrix4D(A11,getA12(),getA13(),getA14(),getA21(),getA22(),getA23(),getA24(),getA31(),getA32(),getA33(),getA34(),getA41(),getA42(),getA43(),getA44() )));
}

double PropertyMatrixItem::getA12() const
{
    return data(1,Qt::EditRole).value<Base::Matrix4D>()[0][1];
}

void PropertyMatrixItem::setA12(double A12)
{
    setData(QVariant::fromValue(Base::Matrix4D(getA11(),A12,getA13(),getA14(),getA21(),getA22(),getA23(),getA24(),getA31(),getA32(),getA33(),getA34(),getA41(),getA42(),getA43(),getA44() )));
}

double PropertyMatrixItem::getA13() const
{
    return data(1,Qt::EditRole).value<Base::Matrix4D>()[0][2];
}

void PropertyMatrixItem::setA13(double A13)
{
    setData(QVariant::fromValue(Base::Matrix4D(getA11(),getA12(),A13,getA14(),getA21(),getA22(),getA23(),getA24(),getA31(),getA32(),getA33(),getA34(),getA41(),getA42(),getA43(),getA44() )));
}

double PropertyMatrixItem::getA14() const
{
    return data(1,Qt::EditRole).value<Base::Matrix4D>()[0][3];
}

void PropertyMatrixItem::setA14(double A14)
{
    setData(QVariant::fromValue(Base::Matrix4D(getA11(),getA12(),getA13(),A14,getA21(),getA22(),getA23(),getA24(),getA31(),getA32(),getA33(),getA34(),getA41(),getA42(),getA43(),getA44() )));
}

double PropertyMatrixItem::getA21() const
{
    return data(1,Qt::EditRole).value<Base::Matrix4D>()[1][0];
}

void PropertyMatrixItem::setA21(double A21)
{
    setData(QVariant::fromValue(Base::Matrix4D(getA11(),getA12(),getA13(),getA14(),A21,getA22(),getA23(),getA24(),getA31(),getA32(),getA33(),getA34(),getA41(),getA42(),getA43(),getA44() )));
}

double PropertyMatrixItem::getA22() const
{
    return data(1,Qt::EditRole).value<Base::Matrix4D>()[1][1];
}

void PropertyMatrixItem::setA22(double A22)
{
    setData(QVariant::fromValue(Base::Matrix4D(getA11(),getA12(),getA13(),getA14(),getA21(),A22,getA23(),getA24(),getA31(),getA32(),getA33(),getA34(),getA41(),getA42(),getA43(),getA44() )));
}

double PropertyMatrixItem::getA23() const
{
    return data(1,Qt::EditRole).value<Base::Matrix4D>()[1][2];
}

void PropertyMatrixItem::setA23(double A23)
{
    setData(QVariant::fromValue(Base::Matrix4D(getA11(),getA12(),getA13(),getA14(),getA21(),getA22(),A23,getA24(),getA31(),getA32(),getA33(),getA34(),getA41(),getA42(),getA43(),getA44() )));
}

double PropertyMatrixItem::getA24() const
{
    return data(1,Qt::EditRole).value<Base::Matrix4D>()[1][3];
}

void PropertyMatrixItem::setA24(double A24)
{
    setData(QVariant::fromValue(Base::Matrix4D(getA11(),getA12(),getA13(),getA14(),getA21(),getA22(),getA23(),A24,getA31(),getA32(),getA33(),getA34(),getA41(),getA42(),getA43(),getA44() )));
}

double PropertyMatrixItem::getA31() const
{
    return data(1,Qt::EditRole).value<Base::Matrix4D>()[2][0];
}

void PropertyMatrixItem::setA31(double A31)
{
    setData(QVariant::fromValue(Base::Matrix4D(getA11(),getA12(),getA13(),getA14(),getA21(),getA22(),getA23(),getA24(),A31,getA32(),getA33(),getA34(),getA41(),getA42(),getA43(),getA44() )));
}

double PropertyMatrixItem::getA32() const
{
    return data(1,Qt::EditRole).value<Base::Matrix4D>()[2][1];
}

void PropertyMatrixItem::setA32(double A32)
{
    setData(QVariant::fromValue(Base::Matrix4D(getA11(),getA12(),getA13(),getA14(),getA21(),getA22(),getA23(),getA24(),getA31(),A32,getA33(),getA34(),getA41(),getA42(),getA43(),getA44() )));
}

double PropertyMatrixItem::getA33() const
{
    return data(1,Qt::EditRole).value<Base::Matrix4D>()[2][2];
}

void PropertyMatrixItem::setA33(double A33)
{
    setData(QVariant::fromValue(Base::Matrix4D(getA11(),getA12(),getA13(),getA14(),getA21(),getA22(),getA23(),getA24(),getA31(),getA32(),A33,getA34(),getA41(),getA42(),getA43(),getA44() )));
}

double PropertyMatrixItem::getA34() const
{
    return data(1,Qt::EditRole).value<Base::Matrix4D>()[2][3];
}

void PropertyMatrixItem::setA34(double A34)
{
    setData(QVariant::fromValue(Base::Matrix4D(getA11(),getA12(),getA13(),getA14(),getA21(),getA22(),getA23(),getA24(),getA31(),getA32(),getA33(),A34,getA41(),getA42(),getA43(),getA44() )));
}

double PropertyMatrixItem::getA41() const
{
    return data(1,Qt::EditRole).value<Base::Matrix4D>()[3][0];
}

void PropertyMatrixItem::setA41(double A41)
{
    setData(QVariant::fromValue(Base::Matrix4D(getA11(),getA12(),getA13(),getA14(),getA21(),getA22(),getA23(),getA24(),getA31(),getA32(),getA33(),getA34(),A41,getA42(),getA43(),getA44() )));
}

double PropertyMatrixItem::getA42() const
{
    return data(1,Qt::EditRole).value<Base::Matrix4D>()[3][1];
}

void PropertyMatrixItem::setA42(double A42)
{
    setData(QVariant::fromValue(Base::Matrix4D(getA11(),getA12(),getA13(),getA14(),getA21(),getA22(),getA23(),getA24(),getA31(),getA32(),getA33(),getA34(),getA41(),A42,getA43(),getA44() )));
}

double PropertyMatrixItem::getA43() const
{
    return data(1,Qt::EditRole).value<Base::Matrix4D>()[3][2];
}

void PropertyMatrixItem::setA43(double A43)
{
    setData(QVariant::fromValue(Base::Matrix4D(getA11(),getA12(),getA13(),getA14(),getA21(),getA22(),getA23(),getA24(),getA31(),getA32(),getA33(),getA34(),getA41(),getA42(),A43,getA44() )));
}

double PropertyMatrixItem::getA44() const
{
    return data(1,Qt::EditRole).value<Base::Matrix4D>()[3][3];
}

void PropertyMatrixItem::setA44(double A44)
{
    setData(QVariant::fromValue(Base::Matrix4D(getA11(),getA12(),getA13(),getA14(),getA21(),getA22(),getA23(),getA24(),getA31(),getA32(),getA33(),getA34(),getA41(),getA42(),getA43(),A44 )));
}

// --------------------------------------------------------------------

PlacementEditor::PlacementEditor(const QString& name, QWidget * parent)
    : LabelButton(parent), _task(0)
{
    propertyname = name;
    propertyname.replace(QLatin1String(" "), QLatin1String(""));
}

PlacementEditor::~PlacementEditor()
{
}

void PlacementEditor::browse()
{
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    Gui::Dialog::TaskPlacement* task;
    task = qobject_cast<Gui::Dialog::TaskPlacement*>(dlg);
    if (dlg && !task) {
        // there is already another task dialog which must be closed first
        Gui::Control().showDialog(dlg);
        return;
    }
    if (!task) {
        task = new Gui::Dialog::TaskPlacement();
    }
    if (!_task) {
        _task = task;
        connect(task, SIGNAL(placementChanged(const QVariant &, bool, bool)),
                this, SLOT(updateValue(const QVariant&, bool, bool)));
    }
    task->setPlacement(value().value<Base::Placement>());
    task->setPropertyName(propertyname);
    Gui::Control().showDialog(task);
}

void PlacementEditor::showValue(const QVariant& d)
{
    const Base::Placement& p = d.value<Base::Placement>();
    double angle;
    Base::Vector3d dir, pos;
    p.getRotation().getValue(dir, angle);
    angle = Base::toDegrees<double>(angle);
    pos = p.getPosition();
    QString data = QString::fromUtf8("[(%1 %2 %3);%4 \xc2\xb0;(%5 %6 %7)]")
                    .arg(QLocale::system().toString(dir.x,'f',2))
                    .arg(QLocale::system().toString(dir.y,'f',2))
                    .arg(QLocale::system().toString(dir.z,'f',2))
                    .arg(QLocale::system().toString(angle,'f',2))
                    .arg(QLocale::system().toString(pos.x,'f',2))
                    .arg(QLocale::system().toString(pos.y,'f',2))
                    .arg(QLocale::system().toString(pos.z,'f',2));
    getLabel()->setText(data);
}

void PlacementEditor::updateValue(const QVariant& v, bool incr, bool data)
{
    if (data) {
        if (incr) {
            QVariant u = value();
            const Base::Placement& plm = u.value<Base::Placement>();
            const Base::Placement& rel = v.value<Base::Placement>();
            Base::Placement data = rel * plm;
            setValue(QVariant::fromValue<Base::Placement>(data));
        }
        else {
            setValue(v);
        }
    }
}

TYPESYSTEM_SOURCE(Gui::PropertyEditor::PropertyPlacementItem, Gui::PropertyEditor::PropertyItem);

PropertyPlacementItem::PropertyPlacementItem() : init_axis(false), changed_value(false), rot_axis(0,0,1)
{
    m_a = static_cast<PropertyAngleItem*>(PropertyAngleItem::create());
    m_a->setParent(this);
    m_a->setPropertyName(QLatin1String("Angle"));
    this->appendChild(m_a);
    m_d = static_cast<PropertyVectorItem*>(PropertyVectorItem::create());
    m_d->setParent(this);
    m_d->setPropertyName(QLatin1String("Axis"));
    m_d->setReadOnly(true);
    this->appendChild(m_d);
    m_p = static_cast<PropertyVectorItem*>(PropertyVectorItem::create());
    m_p->setParent(this);
    m_p->setPropertyName(QLatin1String("Position"));
    m_p->setReadOnly(true);
    this->appendChild(m_p);
}

PropertyPlacementItem::~PropertyPlacementItem()
{
}

double PropertyPlacementItem::getAngle() const
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<Base::Placement>())
        return 0.0;
    const Base::Placement& val = value.value<Base::Placement>();
    double angle;
    Base::Vector3d dir;
    val.getRotation().getValue(dir, angle);
    if (dir * this->rot_axis < 0.0)
        angle = -angle;
    return Base::toDegrees<double>(angle);
}

void PropertyPlacementItem::setAngle(double angle)
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<Base::Placement>())
        return;

    Base::Placement val = value.value<Base::Placement>();
    Base::Rotation rot;
    rot.setValue(this->rot_axis, Base::toRadians<double>(angle));
    val.setRotation(rot);
    changed_value = true;
    setValue(QVariant::fromValue(val));
}

Base::Vector3d PropertyPlacementItem::getAxis() const
{
    // We must store the rotation axis in a member because
    // if we read the value from the property we would always
    // get a normalized vector which makes it quite unhandy
    // to work with
    return this->rot_axis;
}

void PropertyPlacementItem::setAxis(const Base::Vector3d& axis)
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<Base::Placement>())
        return;
    this->rot_axis = axis;
    Base::Placement val = value.value<Base::Placement>();
    Base::Rotation rot = val.getRotation();
    Base::Vector3d dummy; double angle;
    rot.getValue(dummy, angle);
    if (dummy * axis < 0.0)
        angle = -angle;
    rot.setValue(axis, angle);
    val.setRotation(rot);
    changed_value = true;
    setValue(QVariant::fromValue(val));
}

Base::Vector3d PropertyPlacementItem::getPosition() const
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<Base::Placement>())
        return Base::Vector3d(0,0,0);
    const Base::Placement& val = value.value<Base::Placement>();
    return val.getPosition();
}

void PropertyPlacementItem::setPosition(const Base::Vector3d& pos)
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<Base::Placement>())
        return;
    Base::Placement val = value.value<Base::Placement>();
    val.setPosition(pos);
    changed_value = true;
    setValue(QVariant::fromValue(val));
}

QVariant PropertyPlacementItem::value(const App::Property* prop) const
{
    assert(prop && prop->getTypeId().isDerivedFrom(App::PropertyPlacement::getClassTypeId()));

    const Base::Placement& value = static_cast<const App::PropertyPlacement*>(prop)->getValue();
    double angle;
    Base::Vector3d dir;
    value.getRotation().getValue(dir, angle);
    if (!init_axis) {
        const_cast<PropertyPlacementItem*>(this)->rot_axis = dir;
        const_cast<PropertyPlacementItem*>(this)->init_axis = true;
    }
    return QVariant::fromValue<Base::Placement>(value);
}

QVariant PropertyPlacementItem::toolTip(const App::Property* prop) const
{
    assert(prop && prop->getTypeId().isDerivedFrom(App::PropertyPlacement::getClassTypeId()));

    const Base::Placement& p = static_cast<const App::PropertyPlacement*>(prop)->getValue();
    double angle;
    Base::Vector3d dir, pos;
    p.getRotation().getValue(dir, angle);
    angle = Base::toDegrees<double>(angle);
    pos = p.getPosition();
    QString data = QString::fromUtf8("Axis: (%1 %2 %3)\n"
                                     "Angle: %4 \xc2\xb0\n"
                                     "Position: (%5 %6 %7)")
                    .arg(QLocale::system().toString(dir.x,'f',decimals()))
                    .arg(QLocale::system().toString(dir.y,'f',decimals()))
                    .arg(QLocale::system().toString(dir.z,'f',decimals()))
                    .arg(QLocale::system().toString(angle,'f',decimals()))
                    .arg(QLocale::system().toString(pos.x,'f',decimals()))
                    .arg(QLocale::system().toString(pos.y,'f',decimals()))
                    .arg(QLocale::system().toString(pos.z,'f',decimals()));
    return QVariant(data);
}

QVariant PropertyPlacementItem::toString(const QVariant& prop) const
{
    const Base::Placement& p = prop.value<Base::Placement>();
    double angle;
    Base::Vector3d dir, pos;
    p.getRotation().getValue(dir, angle);
    angle = Base::toDegrees<double>(angle);
    pos = p.getPosition();
    QString data = QString::fromUtf8("[(%1 %2 %3);%4 \xc2\xb0;(%5 %6 %7)]")
                    .arg(QLocale::system().toString(dir.x,'f',2))
                    .arg(QLocale::system().toString(dir.y,'f',2))
                    .arg(QLocale::system().toString(dir.z,'f',2))
                    .arg(QLocale::system().toString(angle,'f',2))
                    .arg(QLocale::system().toString(pos.x,'f',2))
                    .arg(QLocale::system().toString(pos.y,'f',2))
                    .arg(QLocale::system().toString(pos.z,'f',2));
    return QVariant(data);
}

void PropertyPlacementItem::setValue(const QVariant& value)
{
    if (!value.canConvert<Base::Placement>())
        return;
    // Accept this only if the user changed the axis, angle or position but
    // not if >this< item looses focus
    if (!changed_value)
        return;
    changed_value = false;
    const Base::Placement& val = value.value<Base::Placement>();
    Base::Vector3d pos = val.getPosition();
    const Base::Rotation& rt = val.getRotation();
    QString data = QString::fromAscii("App.Placement("
                                      "App.Vector(%1,%2,%3),"
                                      "App.Rotation(%4,%5,%6,%7))")
                    .arg(pos.x,0,'g',6)
                    .arg(pos.y,0,'g',6)
                    .arg(pos.z,0,'g',6)
                    .arg(rt[0],0,'g',6)
                    .arg(rt[1],0,'g',6)
                    .arg(rt[2],0,'g',6)
                    .arg(rt[3],0,'g',6);
    setPropertyValue(data);
}

QWidget* PropertyPlacementItem::createEditor(QWidget* parent, const QObject* receiver, const char* method) const
{
    PlacementEditor *pe = new PlacementEditor(this->propertyName(), parent);
    QObject::connect(pe, SIGNAL(valueChanged(const QVariant &)), receiver, method);
    return pe;
}

void PropertyPlacementItem::setEditorData(QWidget *editor, const QVariant& data) const
{
    Gui::LabelButton *pe = qobject_cast<Gui::LabelButton*>(editor);
    pe->setValue(data);
}

QVariant PropertyPlacementItem::editorData(QWidget *editor) const
{
    Gui::LabelButton *pe = qobject_cast<Gui::LabelButton*>(editor);
    return pe->value();
}

// ---------------------------------------------------------------

TYPESYSTEM_SOURCE(Gui::PropertyEditor::PropertyEnumItem, Gui::PropertyEditor::PropertyItem);

PropertyEnumItem::PropertyEnumItem()
{
}

QVariant PropertyEnumItem::value(const App::Property* prop) const
{
    assert(prop && prop->getTypeId().isDerivedFrom(App::PropertyEnumeration::getClassTypeId()));

    const App::PropertyEnumeration* prop_enum = static_cast<const App::PropertyEnumeration*>(prop);
    if (prop_enum->getEnums() == 0) {
        return QVariant(QString());
    }
    else {
        const std::vector<std::string>& value = prop_enum->getEnumVector();
        long currentItem = prop_enum->getValue();
        return QVariant(QString::fromUtf8(value[currentItem].c_str()));
    }
}

void PropertyEnumItem::setValue(const QVariant& value)
{
    if (!value.canConvert(QVariant::StringList))
        return;
    QStringList items = value.toStringList();
    if (!items.isEmpty()) {
        QString val = items.front();
        QString data = QString::fromAscii("\"%1\"").arg(val);
        setPropertyValue(data);
    }
}

QWidget* PropertyEnumItem::createEditor(QWidget* parent, const QObject* receiver, const char* method) const
{
    QComboBox *cb = new QComboBox(parent);
    cb->setFrame(false);
    QObject::connect(cb, SIGNAL(activated(int)), receiver, method);
    return cb;
}

void PropertyEnumItem::setEditorData(QWidget *editor, const QVariant& data) const
{
    const std::vector<App::Property*>& items = getPropertyData();

    QStringList commonModes, modes;
    for (std::vector<App::Property*>::const_iterator it = items.begin(); it != items.end(); ++it) {
        if ((*it)->getTypeId() == App::PropertyEnumeration::getClassTypeId()) {
            App::PropertyEnumeration* prop = static_cast<App::PropertyEnumeration*>(*it);
            if (prop->getEnums() == 0) {
                commonModes.clear();
                break;
            }
            const std::vector<std::string>& value = prop->getEnumVector();
            if (it == items.begin()) {
                for (std::vector<std::string>::const_iterator jt = value.begin(); jt != value.end(); ++jt)
                    commonModes << QLatin1String(jt->c_str());
            }
            else {
                for (std::vector<std::string>::const_iterator jt = value.begin(); jt != value.end(); ++jt) {
                    if (commonModes.contains(QLatin1String(jt->c_str())))
                        modes << QLatin1String(jt->c_str());
                }

                commonModes = modes;
                modes.clear();
            }
        }
    }

    QComboBox *cb = qobject_cast<QComboBox*>(editor);
    if (!commonModes.isEmpty()) {
        cb->addItems(commonModes);
        cb->setCurrentIndex(cb->findText(data.toString()));
    }
}

QVariant PropertyEnumItem::editorData(QWidget *editor) const
{
    QComboBox *cb = qobject_cast<QComboBox*>(editor);
    return QVariant(cb->currentText());
}

// ---------------------------------------------------------------

TYPESYSTEM_SOURCE(Gui::PropertyEditor::PropertyStringListItem, Gui::PropertyEditor::PropertyItem);

PropertyStringListItem::PropertyStringListItem()
{
}

QWidget* PropertyStringListItem::createEditor(QWidget* parent, const QObject* receiver, const char* method) const
{
    Gui::LabelEditor* le = new Gui::LabelEditor(parent);
    le->setAutoFillBackground(true);
    QObject::connect(le, SIGNAL(textChanged(const QString&)), receiver, method);
    return le;
}

void PropertyStringListItem::setEditorData(QWidget *editor, const QVariant& data) const
{
    Gui::LabelEditor *le = qobject_cast<Gui::LabelEditor*>(editor);
    QStringList list = data.toStringList();
    le->setText(list.join(QChar::fromAscii('\n')));
}

QVariant PropertyStringListItem::editorData(QWidget *editor) const
{
    Gui::LabelEditor *le = qobject_cast<Gui::LabelEditor*>(editor);
    QString complete = le->text();
    QStringList list = complete.split(QChar::fromAscii('\n'));
    return QVariant(list);
}

QVariant PropertyStringListItem::toString(const QVariant& prop) const
{
    QStringList list = prop.toStringList();
    QString text = QString::fromUtf8("[%1]").arg(list.join(QLatin1String(",")));

    return QVariant(text);
}

QVariant PropertyStringListItem::value(const App::Property* prop) const
{
    assert(prop && prop->getTypeId().isDerivedFrom(App::PropertyStringList::getClassTypeId()));

    QStringList list;
    const std::vector<std::string>& value = ((App::PropertyStringList*)prop)->getValues();
    for ( std::vector<std::string>::const_iterator jt = value.begin(); jt != value.end(); ++jt ) {
        list << QString::fromUtf8(jt->c_str());
    }

    return QVariant(list);
}

void PropertyStringListItem::setValue(const QVariant& value)
{
    if (!value.canConvert(QVariant::StringList))
        return;
    QStringList values = value.toStringList();
    QString data;
    QTextStream str(&data);
    str << "[";
    for (QStringList::Iterator it = values.begin(); it != values.end(); ++it) {
        str << "unicode('" << *it << "', 'utf-8'),";
    }
    str << "]";
    setPropertyValue(data);
}

// --------------------------------------------------------------------

TYPESYSTEM_SOURCE(Gui::PropertyEditor::PropertyColorItem, Gui::PropertyEditor::PropertyItem);

PropertyColorItem::PropertyColorItem()
{
}

QVariant PropertyColorItem::decoration(const App::Property* prop) const
{
    App::Color value = ((App::PropertyColor*)prop)->getValue();
    QColor color((int)(255.0f*value.r),(int)(255.0f*value.g),(int)(255.0f*value.b));

    int size = QApplication::style()->pixelMetric(QStyle::PM_ListViewIconSize);
    QPixmap p(size, size);
    p.fill(color);

    return QVariant(p);
}

QVariant PropertyColorItem::toString(const QVariant& prop) const
{
    QColor value = prop.value<QColor>();
    QString color = QString::fromAscii("[%1, %2, %3]")
        .arg(value.red()).arg(value.green()).arg(value.blue());
    return QVariant(color);
}

QVariant PropertyColorItem::value(const App::Property* prop) const
{
    assert(prop && prop->getTypeId().isDerivedFrom(App::PropertyColor::getClassTypeId()));

    App::Color value = ((App::PropertyColor*)prop)->getValue();
    return QVariant(QColor((int)(255.0f*value.r),(int)(255.0f*value.g),(int)(255.0f*value.b)));
}

void PropertyColorItem::setValue(const QVariant& value)
{
    if (!value.canConvert<QColor>())
        return;
    QColor col = value.value<QColor>();
    App::Color val;
    val.r = (float)col.red()/255.0f;
    val.g = (float)col.green()/255.0f;
    val.b = (float)col.blue()/255.0f;
    QString data = QString::fromAscii("(%1,%2,%3)")
                    .arg(val.r,0,'f',decimals())
                    .arg(val.g,0,'f',decimals())
                    .arg(val.b,0,'f',decimals());
    setPropertyValue(data);
}

QWidget* PropertyColorItem::createEditor(QWidget* parent, const QObject* receiver, const char* method) const
{
    Gui::ColorButton* cb = new Gui::ColorButton( parent );
    QObject::connect(cb, SIGNAL(changed()), receiver, method);
    return cb;
}

void PropertyColorItem::setEditorData(QWidget *editor, const QVariant& data) const
{
    Gui::ColorButton *cb = qobject_cast<Gui::ColorButton*>(editor);
    QColor color = data.value<QColor>();
    cb->setColor(color);
}

QVariant PropertyColorItem::editorData(QWidget *editor) const
{
    Gui::ColorButton *cb = qobject_cast<Gui::ColorButton*>(editor);
    QVariant var;
    var.setValue(cb->color());
    return var;
}

// --------------------------------------------------------------------

TYPESYSTEM_SOURCE(Gui::PropertyEditor::PropertyFileItem, Gui::PropertyEditor::PropertyItem);

PropertyFileItem::PropertyFileItem()
{
}

QVariant PropertyFileItem::value(const App::Property* prop) const
{
    assert(prop && prop->getTypeId().isDerivedFrom(App::PropertyFile::getClassTypeId()));

    std::string value = static_cast<const App::PropertyFile*>(prop)->getValue();
    return QVariant(QString::fromUtf8(value.c_str()));
}

void PropertyFileItem::setValue(const QVariant& value)
{
    if (!value.canConvert(QVariant::String))
        return;
    QString val = value.toString();
    QString data = QString::fromAscii("\"%1\"").arg(val);
    setPropertyValue(data);
}

QVariant PropertyFileItem::toolTip(const App::Property* prop) const
{
    return value(prop);
}

QWidget* PropertyFileItem::createEditor(QWidget* parent, const QObject* receiver, const char* method) const
{
    Gui::FileChooser *fc = new Gui::FileChooser(parent);
    fc->setAutoFillBackground(true);
    QObject::connect(fc, SIGNAL(fileNameSelected(const QString&)), receiver, method);
    return fc;
}

void PropertyFileItem::setEditorData(QWidget *editor, const QVariant& data) const
{
    Gui::FileChooser *fc = qobject_cast<Gui::FileChooser*>(editor);
    fc->setFileName(data.toString());
}

QVariant PropertyFileItem::editorData(QWidget *editor) const
{
    Gui::FileChooser *fc = qobject_cast<Gui::FileChooser*>(editor);
    return QVariant(fc->fileName());
}

// --------------------------------------------------------------------

TYPESYSTEM_SOURCE(Gui::PropertyEditor::PropertyPathItem, Gui::PropertyEditor::PropertyItem);

PropertyPathItem::PropertyPathItem()
{
}

QVariant PropertyPathItem::value(const App::Property* prop) const
{
    assert(prop && prop->getTypeId().isDerivedFrom(App::PropertyPath::getClassTypeId()));

    std::string value = static_cast<const App::PropertyPath*>(prop)->getValue().string();
    return QVariant(QString::fromUtf8(value.c_str()));
}

void PropertyPathItem::setValue(const QVariant& value)
{
    if (!value.canConvert(QVariant::String))
        return;
    QString val = value.toString();
    QString data = QString::fromAscii("\"%1\"").arg(val);
    setPropertyValue(data);
}

QVariant PropertyPathItem::toolTip(const App::Property* prop) const
{
    return value(prop);
}

QWidget* PropertyPathItem::createEditor(QWidget* parent, const QObject* receiver, const char* method) const
{
    Gui::FileChooser *fc = new Gui::FileChooser(parent);
    fc->setAutoFillBackground(true);
    QObject::connect(fc, SIGNAL(fileNameSelected(const QString&)), receiver, method);
    return fc;
}

void PropertyPathItem::setEditorData(QWidget *editor, const QVariant& data) const
{
    Gui::FileChooser *fc = qobject_cast<Gui::FileChooser*>(editor);
    fc->setFileName(data.toString());
}

QVariant PropertyPathItem::editorData(QWidget *editor) const
{
    Gui::FileChooser *fc = qobject_cast<Gui::FileChooser*>(editor);
    return QVariant(fc->fileName());
}

// --------------------------------------------------------------------

TYPESYSTEM_SOURCE(Gui::PropertyEditor::PropertyTransientFileItem, Gui::PropertyEditor::PropertyItem);

PropertyTransientFileItem::PropertyTransientFileItem()
{
}

QVariant PropertyTransientFileItem::value(const App::Property* prop) const
{
    assert(prop && prop->getTypeId().isDerivedFrom(App::PropertyFileIncluded::getClassTypeId()));

    std::string value = static_cast<const App::PropertyFileIncluded*>(prop)->getValue();
    return QVariant(QString::fromUtf8(value.c_str()));
}

void PropertyTransientFileItem::setValue(const QVariant& value)
{
    if (!value.canConvert(QVariant::String))
        return;
    QString val = value.toString();
    QString data = QString::fromAscii("\"%1\"").arg(val);
    setPropertyValue(data);
}

QVariant PropertyTransientFileItem::toolTip(const App::Property* prop) const
{
    return value(prop);
}

QWidget* PropertyTransientFileItem::createEditor(QWidget* parent, const QObject* receiver, const char* method) const
{
    Gui::FileChooser *fc = new Gui::FileChooser(parent);
    fc->setAutoFillBackground(true);
    QObject::connect(fc, SIGNAL(fileNameSelected(const QString&)), receiver, method);
    return fc;
}

void PropertyTransientFileItem::setEditorData(QWidget *editor, const QVariant& data) const
{
    Gui::FileChooser *fc = qobject_cast<Gui::FileChooser*>(editor);
    fc->setFileName(data.toString());
}

QVariant PropertyTransientFileItem::editorData(QWidget *editor) const
{
    Gui::FileChooser *fc = qobject_cast<Gui::FileChooser*>(editor);
    return QVariant(fc->fileName());
}

// ---------------------------------------------------------------

LinkLabel::LinkLabel (QWidget * parent) : QLabel(parent)
{
    setTextFormat(Qt::RichText);
    connect(this, SIGNAL(linkActivated(const QString&)),
            this, SLOT(onLinkActivated(const QString&)));
}

LinkLabel::~LinkLabel()
{
}

void LinkLabel::setPropertyLink(const QStringList& o)
{
    object = o;
    QString text = QString::fromAscii(
        "<html><head><style type=\"text/css\">"
        "p, li { white-space: pre-wrap; }"
        "</style></head><body>"
        "<p>"
        "<a href=\"%1.%2\"><span style=\" text-decoration: underline; color:#0000ff;\">%3</span></a>"
        "<span>	</span>"
        "<a href=\"@__edit_link_prop__@\"><span style=\" text-decoration: underline; color:#0000ff;\">%4</span></a>"
        "</p></body></html>"
    ).arg(o[0]).arg(o[1]).arg(o[2]).arg(tr("Edit..."));
    setText(text);
}

QStringList LinkLabel::propertyLink() const
{
    return object;
}

void LinkLabel::onLinkActivated (const QString& s)
{
    if (s == QLatin1String("@__edit_link_prop__@"))
        QMessageBox::warning(this, QLatin1String("Not yet implemented"), QLatin1String("Not yet implemented"));
    else
        Gui::Selection().addSelection((const char*)object[0].toAscii(), (const char*)object[1].toAscii());
}

TYPESYSTEM_SOURCE(Gui::PropertyEditor::PropertyLinkItem, Gui::PropertyEditor::PropertyItem);

PropertyLinkItem::PropertyLinkItem()
{
}

QVariant PropertyLinkItem::toString(const QVariant& prop) const
{
    QStringList list = prop.toStringList();
    return QVariant(list.last());
}

QVariant PropertyLinkItem::value(const App::Property* prop) const
{
    assert(prop && prop->getTypeId().isDerivedFrom(App::PropertyLink::getClassTypeId()));

    const App::PropertyLink* prop_link = static_cast<const App::PropertyLink*>(prop);
    App::DocumentObject* obj = prop_link->getValue();
    QStringList list;
    if (obj) {
        list << QString::fromAscii(obj->getDocument()->getName());
        list << QString::fromAscii(obj->getNameInDocument());
        list << QString::fromUtf8(obj->Label.getValue());
    }
    else {
        App::PropertyContainer* c = prop_link->getContainer();
        if (c->getTypeId().isDerivedFrom(App::DocumentObject::getClassTypeId())) {
            App::DocumentObject* obj = static_cast<App::DocumentObject*>(c);
            list << QString::fromAscii(obj->getDocument()->getName());
        }
        else {
            list << QString::fromAscii("");
        }
        list << QString::fromAscii("Null");
        list << QString::fromAscii("");
    }

    return QVariant(list);
}

void PropertyLinkItem::setValue(const QVariant& value)
{
    if (!value.canConvert(QVariant::StringList))
        return;
    QStringList items = value.toStringList();
    if (items.size() > 1) {
        QString d = items[0];
        QString o = items[1];
        QString data = QString::fromAscii("App.getDocument('%1').getObject('%2')").arg(d).arg(o);
        setPropertyValue(data);
    }
}

QWidget* PropertyLinkItem::createEditor(QWidget* parent, const QObject* receiver, const char* method) const
{
    LinkLabel *ll = new LinkLabel(parent);
    ll->setAutoFillBackground(true);
    //QObject::connect(cb, SIGNAL(activated(int)), receiver, method);
    return ll;
}

void PropertyLinkItem::setEditorData(QWidget *editor, const QVariant& data) const
{
    QStringList list = data.toStringList();
    LinkLabel *ll = static_cast<LinkLabel*>(editor);
    ll->setPropertyLink(list);
}

QVariant PropertyLinkItem::editorData(QWidget *editor) const
{
    LinkLabel *ll = static_cast<LinkLabel*>(editor);
    return QVariant(ll->propertyLink());
}

// --------------------------------------------------------------------

PropertyItemEditorFactory::PropertyItemEditorFactory()
{
}

PropertyItemEditorFactory::~PropertyItemEditorFactory()
{
}

QWidget * PropertyItemEditorFactory::createEditor (QVariant::Type /*type*/, QWidget * /*parent*/) const
{
    // do not allow to create any editor widgets because we do that in subclasses of PropertyItem
    return 0;
}

QByteArray PropertyItemEditorFactory::valuePropertyName (QVariant::Type /*type*/) const
{
    // do not allow to set properties because we do that in subclasses of PropertyItem
    return "";
}

#include "moc_PropertyItem.cpp"

