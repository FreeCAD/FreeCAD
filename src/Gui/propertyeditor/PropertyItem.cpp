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
# include <QTextStream>
# include <QTimer>
# include <QApplication>
# include <QPalette>
# include <QtGlobal>
#endif

#include <Base/Tools.h>
#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/PropertyGeo.h>
#include <App/PropertyFile.h>
#include <App/PropertyUnits.h>
#include <Gui/Application.h>
#include <Gui/Control.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Gui/ViewProviderDocumentObject.h>
#include <Gui/Placement.h>
#include <Gui/FileDialog.h>
#include <Gui/DlgPropertyLink.h>
#include <Gui/QuantitySpinBox.h>

#include "PropertyItem.h"
#include <Gui/SpinBox.h>

using namespace Gui::PropertyEditor;

Gui::PropertyEditor::PropertyItemFactory* Gui::PropertyEditor::PropertyItemFactory::_singleton = NULL;

PropertyItemFactory& PropertyItemFactory::instance()
{
    if (_singleton == NULL)
        _singleton = new PropertyItemFactory;
    return *_singleton;
}

void PropertyItemFactory::destruct ()
{
    delete _singleton;
    _singleton = 0;
}

PropertyItem* PropertyItemFactory::createPropertyItem (const char* sName) const
{
    PropertyItem* w = static_cast<PropertyItem*>(Produce(sName));
    return w;
}

// ----------------------------------------------------

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyItem)

PropertyItem::PropertyItem() : parentItem(0), readonly(false), cleared(false)
{
    precision = Base::UnitsApi::getDecimals();
    setAutoApply(true);
}

PropertyItem::~PropertyItem()
{
    qDeleteAll(childItems);
}

void PropertyItem::initialize()
{
}

void PropertyItem::reset()
{
    qDeleteAll(childItems);
    childItems.clear();
}

void PropertyItem::setPropertyData(const std::vector<App::Property*>& items)
{
    //if we have a single property we can bind it for expression handling
    if (items.size() == 1) {
        const App::Property& p = *items.front();

        try {
            // Check for 'DocumentObject' as parent because otherwise 'ObjectIdentifier' raises an exception
            App::DocumentObject * docObj = Base::freecad_dynamic_cast<App::DocumentObject>(p.getContainer());
            if (docObj && !docObj->isReadOnly(&p)) {
                App::ObjectIdentifier id(p);
                std::vector<App::ObjectIdentifier> paths;
                p.getPaths(paths);

                //there may be no paths available in this property (for example an empty constraint list)
                if (id.getProperty() && !paths.empty())
                    bind(id);
            }
        }
        //it may happen that setting properties is not possible
        catch (...) {
        }
    }

    propertyItems = items;
    updateData();
    this->initialize();
}

void PropertyItem::updateData()
{
    bool ro = true;
    for (std::vector<App::Property*>::const_iterator it = propertyItems.begin();
        it != propertyItems.end(); ++it) {
        App::PropertyContainer* parent = (*it)->getContainer();
        if (parent)
            ro &= (parent->isReadOnly(*it) || (*it)->testStatus(App::Property::ReadOnly));
    }
    this->setReadOnly(ro);
}

const std::vector<App::Property*>& PropertyItem::getPropertyData() const
{
    return propertyItems;
}

bool PropertyItem::hasProperty(const App::Property* prop) const
{
    std::vector<App::Property*>::const_iterator it = std::find(propertyItems.begin(), propertyItems.end(), prop);
    if (it != propertyItems.end())
        return true;
    else
        return false;
}

void PropertyItem::assignProperty(const App::Property*)
{
}

bool PropertyItem::removeProperty(const App::Property* prop)
{
    std::vector<App::Property*>::iterator it = std::find(propertyItems.begin(), propertyItems.end(), prop);
    if (it != propertyItems.end()) {
        propertyItems.erase(it);
    }

    return propertyItems.empty();
}

App::Property* PropertyItem::getFirstProperty()
{
    if (propertyItems.empty())
        return 0;
    return propertyItems.front();
}

const App::Property* PropertyItem::getFirstProperty() const
{
    if (propertyItems.empty())
        return 0;
    return propertyItems.front();
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

void PropertyItem::insertChild(int index, PropertyItem *child)
{
    childItems.insert(index, child);
}

/*!
 * \brief PropertyItem::removeChildren
 * Deletes the children in the range of [from, to]
 */
void PropertyItem::removeChildren(int from, int to)
{
    int count = to-from+1;
    for (int i=0; i<count; i++) {
        PropertyItem* child = childItems.takeAt(from);
        delete child;
    }
}

/*!
 * \brief PropertyItem::takeChild
 * Removes the child at index row but doesn't delete it
 */
PropertyItem *PropertyItem::takeChild(int row)
{
    PropertyItem* child = childItems.takeAt(row);
    child->setParent(nullptr);
    return child;
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

bool PropertyItem::testStatus(App::Property::Status pos) const
{
    std::vector<App::Property*>::const_iterator it;
    for (it = propertyItems.begin(); it != propertyItems.end(); ++it) {
        if ((*it)->testStatus(pos))
            return true;
    }
    return false;
}

void PropertyItem::setDecimals(int prec)
{
    precision = prec;
}

int PropertyItem::decimals() const
{
    return precision;
}

QVariant PropertyItem::displayName() const
{
    return QVariant(displayText);
}

QVariant PropertyItem::toolTip(const App::Property* prop) const
{
    QString str = QApplication::translate("App::Property",
                                          prop->getDocumentation());
    return QVariant(str);
}

QVariant PropertyItem::decoration(const QVariant&) const
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
        QString docName = QString::fromLatin1(App::GetApplication().getDocumentName(doc));
        QString propName = QString::fromLatin1(parent->getPropertyName(prop));
        return QString::fromLatin1("FreeCAD.getDocument(\"%1\").%2").arg(docName).arg(propName);
    }
    if (parent->getTypeId().isDerivedFrom(App::DocumentObject::getClassTypeId())) {
        App::DocumentObject* obj = static_cast<App::DocumentObject*>(parent);
        App::Document* doc = obj->getDocument();
        QString docName = QString::fromLatin1(App::GetApplication().getDocumentName(doc));
        QString objName = QString::fromLatin1(obj->getNameInDocument());
        QString propName = QString::fromLatin1(parent->getPropertyName(prop));
        return QString::fromLatin1("FreeCAD.getDocument(\"%1\").getObject(\"%2\").%3")
            .arg(docName).arg(objName).arg(propName);
    }
    auto* vp = dynamic_cast<Gui::ViewProviderDocumentObject*>(parent);
    if (vp) {
        App::DocumentObject* obj = vp->getObject();
        App::Document* doc = obj->getDocument();
        QString docName = QString::fromLatin1(App::GetApplication().getDocumentName(doc));
        QString objName = QString::fromLatin1(obj->getNameInDocument());
        QString propName = QString::fromLatin1(parent->getPropertyName(prop));
        return QString::fromLatin1("FreeCADGui.getDocument(\"%1\").getObject(\"%2\").%3")
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
        return QLatin1String(QT_TRANSLATE_NOOP("App::Property", "<empty>"));
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
            if (!upper) {
                QChar last = display.at(display.length()-1);
                if (!last.isSpace())
                    display += QLatin1String(" ");
            }
        }
        upper = name[i].isUpper();
        display += name[i];
    }

    propName = display;

    QString str = QApplication::translate("App::Property", propName.toLatin1());
    displayText = str;
}

void PropertyItem::setPropertyValue(const QString& value)
{
    for (std::vector<App::Property*>::const_iterator it = propertyItems.begin();
        it != propertyItems.end(); ++it) {
        App::PropertyContainer* parent = (*it)->getContainer();
        if (parent && !parent->isReadOnly(*it) && !(*it)->testStatus(App::Property::ReadOnly)) {
            QString cmd = QString::fromLatin1("%1 = %2").arg(pythonIdentifier(*it)).arg(value);
            try {
                Gui::Command::runCommand(Gui::Command::App, cmd.toUtf8());
            }
            catch (Base::PyException &e) {
                e.ReportException();
                Base::Console().Error("Stack Trace: %s\n",e.getStackTrace().c_str());
            }
            catch (Base::Exception &e) {
                e.ReportException();
            }
            catch (...) {
                Base::Console().Error("Unknown C++ exception in PropertyItem::setPropertyValue thrown\n");
            }
        }
    }
}

QVariant PropertyItem::data(int column, int role) const
{
    // property name
    if (column == 0) {
        if (role == Qt::DisplayRole)
            return displayName();
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
            if (role == Qt::EditRole) {
                return parent->property(qPrintable(objectName()));
            }
            else if (role == Qt::DecorationRole) {
                QVariant val = parent->property(qPrintable(objectName()));
                return decoration(val);
            }
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
            return decoration(value(propertyItems[0]));
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
    cleared = false;

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

void PropertyItem::bind(const App::ObjectIdentifier& _path)
{
    Gui::ExpressionBinding::bind(_path);
    propertyBound();
}

void PropertyItem::bind(const App::Property& prop)
{
    Gui::ExpressionBinding::bind(prop);
    propertyBound();
}

QString PropertyItem::expressionAsString() const
{
    if (hasExpression()) {
        try {
            std::unique_ptr<App::Expression> result(getExpression()->eval());
            return QString::fromStdString(result->toString());
        }
        catch (const Base::Exception& e) {
            e.ReportException();
        }
    }

    return QString();
}

// --------------------------------------------------------------------

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyStringItem)

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
    val = QString::fromUtf8(Base::Interpreter().strToPython(val.toUtf8()).c_str());
    QString data = QString::fromLatin1("\"%1\"").arg(val);
    setPropertyValue(data);
}

QWidget* PropertyStringItem::createEditor(QWidget* parent, const QObject* receiver, const char* method) const
{
    QLineEdit *le = new QLineEdit(parent);
    le->setFrame(false);
    le->setReadOnly(isReadOnly());
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

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyFontItem)

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
    QString data = QString::fromLatin1("\"%1\"").arg(val);
    setPropertyValue(data);
}

QWidget* PropertyFontItem::createEditor(QWidget* parent, const QObject* receiver, const char* method) const
{
    QComboBox *cb = new QComboBox(parent);
    cb->setFrame(false);
    cb->setDisabled(isReadOnly());
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

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertySeparatorItem)

QWidget* PropertySeparatorItem::createEditor(QWidget* parent, const QObject* receiver, const char* method) const
{
    Q_UNUSED(parent); 
    Q_UNUSED(receiver); 
    Q_UNUSED(method); 
    return 0;
}

// --------------------------------------------------------------------

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyIntegerItem)

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
    //if the item has an expression it issues the python code
    if (!hasExpression()) {
        if (!value.canConvert(QVariant::Int))
            return;
        int val = value.toInt();
        QString data = QString::fromLatin1("%1").arg(val);
        setPropertyValue(data);
    }
}

QWidget* PropertyIntegerItem::createEditor(QWidget* parent, const QObject* receiver, const char* method) const
{
    Gui::IntSpinBox *sb = new Gui::IntSpinBox(parent);
    sb->setFrame(false);
    sb->setReadOnly(isReadOnly());
    QObject::connect(sb, SIGNAL(valueChanged(int)), receiver, method);

    if (isBound()) {
        sb->bind(getPath());
        sb->setAutoApply(autoApply());
    }

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

QVariant PropertyIntegerItem::toString(const QVariant& v) const
{
    QString string(PropertyItem::toString(v).toString());

    if (hasExpression())
        string += QString::fromLatin1("  ( %1 )").arg(QString::fromStdString(getExpressionString()));

    return QVariant(string);
}


// --------------------------------------------------------------------

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyIntegerConstraintItem)

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
    //if the item has an expression it issues the python code
    if (!hasExpression()) {
        if (!value.canConvert(QVariant::Int))
            return;
        int val = value.toInt();
        QString data = QString::fromLatin1("%1").arg(val);
        setPropertyValue(data);
    }
}

QWidget* PropertyIntegerConstraintItem::createEditor(QWidget* parent, const QObject* receiver, const char* method) const
{
    Gui::IntSpinBox *sb = new Gui::IntSpinBox(parent);
    sb->setFrame(false);
    sb->setReadOnly(isReadOnly());
    QObject::connect(sb, SIGNAL(valueChanged(int)), receiver, method);

    if (isBound()) {
        sb->bind(getPath());
        sb->setAutoApply(autoApply());
    }

    return sb;
}

void PropertyIntegerConstraintItem::setEditorData(QWidget *editor, const QVariant& data) const
{
    const App::PropertyIntegerConstraint* prop = static_cast
        <const App::PropertyIntegerConstraint*>(getFirstProperty());

    const App::PropertyIntegerConstraint::Constraints* c = 0;
    if (prop) {
        c = prop->getConstraints();
    }

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
    sb->setValue(data.toInt());
}

QVariant PropertyIntegerConstraintItem::editorData(QWidget *editor) const
{
    QSpinBox *sb = qobject_cast<QSpinBox*>(editor);
    return QVariant(sb->value());
}

QVariant PropertyIntegerConstraintItem::toString(const QVariant& v) const
{
    QString string(PropertyItem::toString(v).toString());

    if (hasExpression())
        string += QString::fromLatin1("  ( %1 )").arg(QString::fromStdString(getExpressionString()));

    return QVariant(string);
}


// --------------------------------------------------------------------

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyFloatItem)

PropertyFloatItem::PropertyFloatItem()
{
}

QVariant PropertyFloatItem::toString(const QVariant& prop) const
{
    double value = prop.toDouble();
    QString data = QLocale::system().toString(value, 'f', decimals());

    if (hasExpression())
        data += QString::fromLatin1("  ( %1 )").arg(QString::fromStdString(getExpressionString()));

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
    //if the item has an expression it issues the python code
    if (!hasExpression()) {
        if (!value.canConvert(QVariant::Double))
            return;
        double val = value.toDouble();
        QString data = QString::fromLatin1("%1").arg(val,0,'f',decimals());
        setPropertyValue(data);
    }
}

QWidget* PropertyFloatItem::createEditor(QWidget* parent, const QObject* receiver, const char* method) const
{
    Gui::DoubleSpinBox *sb = new Gui::DoubleSpinBox(parent);
    sb->setFrame(false);
    sb->setDecimals(decimals());
    sb->setReadOnly(isReadOnly());
    QObject::connect(sb, SIGNAL(valueChanged(double)), receiver, method);

    if (isBound()) {
        sb->bind(getPath());
        sb->setAutoApply(autoApply());
    }

    return sb;
}

void PropertyFloatItem::setEditorData(QWidget *editor, const QVariant& data) const
{
    QDoubleSpinBox *sb = qobject_cast<QDoubleSpinBox*>(editor);
    sb->setRange((double)INT_MIN, (double)INT_MAX);
    sb->setValue(data.toDouble());
}

QVariant PropertyFloatItem::editorData(QWidget *editor) const
{
    QDoubleSpinBox *sb = qobject_cast<QDoubleSpinBox*>(editor);
    return QVariant(sb->value());
}

// --------------------------------------------------------------------


PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyUnitItem)

PropertyUnitItem::PropertyUnitItem()
{
}

QVariant PropertyUnitItem::toString(const QVariant& prop) const
{
    const Base::Quantity& unit = prop.value<Base::Quantity>();
    QString string = unit.getUserString();
    if (hasExpression())
        string += QString::fromLatin1("  ( %1 )").arg(QString::fromStdString(getExpressionString()));

    return QVariant(string);
}

QVariant PropertyUnitItem::value(const App::Property* prop) const
{
    assert(prop && prop->getTypeId().isDerivedFrom(App::PropertyQuantity::getClassTypeId()));

    Base::Quantity value = static_cast<const App::PropertyQuantity*>(prop)->getQuantityValue();
    return QVariant::fromValue<Base::Quantity>(value);
}

void PropertyUnitItem::setValue(const QVariant& value)
{
    //if the item has an expression it handles the python code
    if (!hasExpression()) {
        if (!value.canConvert<Base::Quantity>())
            return;
        const Base::Quantity& val = value.value<Base::Quantity>();

        QString unit = QString::fromLatin1("'%1 %2'").arg(val.getValue()).arg(val.getUnit().getString()); 
        setPropertyValue(unit);
    }
}

QWidget* PropertyUnitItem::createEditor(QWidget* parent, const QObject* receiver, const char* method) const
{
    Gui::QuantitySpinBox *infield = new Gui::QuantitySpinBox(parent);
    infield->setFrame(false);
    infield->setMinimumHeight(0);
    infield->setReadOnly(isReadOnly());
    
    //if we are bound to an expression we need to bind it to the input field
    if (isBound()) {
        infield->bind(getPath());
        infield->setAutoApply(autoApply());
    }

    
    QObject::connect(infield, SIGNAL(valueChanged(double)), receiver, method);
    return infield;
}

void PropertyUnitItem::setEditorData(QWidget *editor, const QVariant& data) const
{
    const Base::Quantity& value = data.value<Base::Quantity>();

    Gui::QuantitySpinBox *infield = qobject_cast<Gui::QuantitySpinBox*>(editor);
    infield->setValue(value);
    infield->selectAll();
}

QVariant PropertyUnitItem::editorData(QWidget *editor) const
{
    Gui::QuantitySpinBox *infield = qobject_cast<Gui::QuantitySpinBox*>(editor);
    Base::Quantity value = infield->value();
    return QVariant::fromValue<Base::Quantity>(value);
}

// --------------------------------------------------------------------


PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyUnitConstraintItem)

PropertyUnitConstraintItem::PropertyUnitConstraintItem()
{

}

void PropertyUnitConstraintItem::setEditorData(QWidget *editor, const QVariant& data) const
{
    const Base::Quantity& value = data.value<Base::Quantity>();

    Gui::QuantitySpinBox *infield = qobject_cast<Gui::QuantitySpinBox*>(editor);
    infield->setValue(value);
    infield->selectAll();

    const App::PropertyQuantityConstraint* prop = static_cast
        <const App::PropertyQuantityConstraint*>(getFirstProperty());

    const App::PropertyQuantityConstraint::Constraints* c = 0;
    if (prop) {
        c = prop->getConstraints();
    }

    if (c) {
        infield->setMinimum(c->LowerBound);
        infield->setMaximum(c->UpperBound);
        infield->setSingleStep(c->StepSize);
    }
    else {
        infield->setMinimum((double)INT_MIN);
        infield->setMaximum((double)INT_MAX);
    }
}

// --------------------------------------------------------------------

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyFloatConstraintItem)

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
    //if the item has an expression it issues the python code
    if (!hasExpression()) {
        if (!value.canConvert(QVariant::Double))
            return;
        double val = value.toDouble();
        QString data = QString::fromLatin1("%1").arg(val,0,'f',decimals());
        setPropertyValue(data);
    }
}

QWidget* PropertyFloatConstraintItem::createEditor(QWidget* parent, const QObject* receiver, const char* method) const
{
    Gui::DoubleSpinBox *sb = new Gui::DoubleSpinBox(parent);
    sb->setDecimals(decimals());
    sb->setFrame(false);
    sb->setReadOnly(isReadOnly());
    QObject::connect(sb, SIGNAL(valueChanged(double)), receiver, method);

    if (isBound()) {
        sb->bind(getPath());
        sb->setAutoApply(autoApply());
    }

    return sb;
}

void PropertyFloatConstraintItem::setEditorData(QWidget *editor, const QVariant& data) const
{
    const App::PropertyFloatConstraint* prop = static_cast
        <const App::PropertyFloatConstraint*>(getFirstProperty());

    const App::PropertyFloatConstraint::Constraints* c = 0;
    if (prop) {
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
        sb->setSingleStep(0.1);
    }
    sb->setValue(data.toDouble());
}

QVariant PropertyFloatConstraintItem::editorData(QWidget *editor) const
{
    QDoubleSpinBox *sb = qobject_cast<QDoubleSpinBox*>(editor);
    return QVariant(sb->value());
}

// --------------------------------------------------------------------

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyPrecisionItem)

PropertyPrecisionItem::PropertyPrecisionItem()
{
    setDecimals(16);
}

// --------------------------------------------------------------------

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyAngleItem)

PropertyAngleItem::PropertyAngleItem()
{
}

void PropertyAngleItem::setEditorData(QWidget *editor, const QVariant& data) const
{
    PropertyUnitConstraintItem::setEditorData(editor, data);
}

QVariant PropertyAngleItem::toString(const QVariant& prop) const
{
    return PropertyUnitConstraintItem::toString(prop);
}

// --------------------------------------------------------------------

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyBoolItem)

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
    cb->setDisabled(isReadOnly());
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

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyVectorItem)

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
    QString data = QString::fromLatin1("[%1 %2 %3]")
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
    QString data = QString::fromLatin1("(%1, %2, %3)")
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
    QString text = QString::fromLatin1("[%1 %2 %3]")
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

void PropertyVectorItem::propertyBound()
{
    m_x->bind(App::ObjectIdentifier(getPath())<<App::ObjectIdentifier::String("x"));
    m_y->bind(App::ObjectIdentifier(getPath())<<App::ObjectIdentifier::String("y"));
    m_z->bind(App::ObjectIdentifier(getPath())<<App::ObjectIdentifier::String("z"));
}

// ---------------------------------------------------------------

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyVectorDistanceItem)

PropertyVectorDistanceItem::PropertyVectorDistanceItem()
{
    m_x = static_cast<PropertyUnitItem*>(PropertyUnitItem::create());
    m_x->setParent(this);
    m_x->setPropertyName(QLatin1String("x"));
    this->appendChild(m_x);
    m_y = static_cast<PropertyUnitItem*>(PropertyUnitItem::create());
    m_y->setParent(this);
    m_y->setPropertyName(QLatin1String("y"));
    this->appendChild(m_y);
    m_z = static_cast<PropertyUnitItem*>(PropertyUnitItem::create());
    m_z->setParent(this);
    m_z->setPropertyName(QLatin1String("z"));
    this->appendChild(m_z);
}

QVariant PropertyVectorDistanceItem::toString(const QVariant& prop) const
{
    const Base::Vector3d& value = prop.value<Base::Vector3d>();
    QString data = QString::fromLatin1("[") + 
           Base::Quantity(value.x, Base::Unit::Length).getUserString() + QString::fromLatin1("  ") +
           Base::Quantity(value.y, Base::Unit::Length).getUserString() + QString::fromLatin1("  ") +
           Base::Quantity(value.z, Base::Unit::Length).getUserString() + QString::fromLatin1("]");
    return QVariant(data);
}


QVariant PropertyVectorDistanceItem::value(const App::Property* prop) const
{
    assert(prop && prop->getTypeId().isDerivedFrom(App::PropertyVector::getClassTypeId()));

    const Base::Vector3d& value = static_cast<const App::PropertyVector*>(prop)->getValue();
    return QVariant::fromValue<Base::Vector3d>(value);
}

void PropertyVectorDistanceItem::setValue(const QVariant& variant)
{
    if (!variant.canConvert<Base::Vector3d>())
        return;
    const Base::Vector3d& value = variant.value<Base::Vector3d>();

    Base::Quantity x = Base::Quantity(value.x, Base::Unit::Length);
    Base::Quantity y = Base::Quantity(value.y, Base::Unit::Length);
    Base::Quantity z = Base::Quantity(value.z, Base::Unit::Length);
    QString data = QString::fromLatin1("(%1, %2, %3)")
                    .arg(x.getValue())
                    .arg(y.getValue())
                    .arg(z.getValue());
    setPropertyValue(data);
}

void PropertyVectorDistanceItem::setEditorData(QWidget *editor, const QVariant& data) const
{
    QLineEdit* le = qobject_cast<QLineEdit*>(editor);
    le->setText(toString(data).toString());
}

QWidget* PropertyVectorDistanceItem::createEditor(QWidget* parent, const QObject* /*receiver*/, const char* /*method*/) const
{
    QLineEdit *le = new QLineEdit(parent);
    le->setFrame(false);
    le->setReadOnly(true);
    return le;
}

QVariant PropertyVectorDistanceItem::editorData(QWidget *editor) const
{
    QLineEdit *le = qobject_cast<QLineEdit*>(editor);
    return QVariant(le->text());
}

Base::Quantity PropertyVectorDistanceItem::x() const
{
    return Base::Quantity(data(1,Qt::EditRole).value<Base::Vector3d>().x, Base::Unit::Length);
}

void PropertyVectorDistanceItem::setX(Base::Quantity x)
{
    setData(QVariant::fromValue(Base::Vector3d(x.getValue(), y().getValue(), z().getValue())));
}

Base::Quantity PropertyVectorDistanceItem::y() const
{
    return Base::Quantity(data(1,Qt::EditRole).value<Base::Vector3d>().y, Base::Unit::Length);
}

void PropertyVectorDistanceItem::setY(Base::Quantity y)
{
    setData(QVariant::fromValue(Base::Vector3d(x().getValue(), y.getValue(), z().getValue())));
}

Base::Quantity PropertyVectorDistanceItem::z() const
{
    return Base::Quantity(data(1,Qt::EditRole).value<Base::Vector3d>().z, Base::Unit::Length);
}

void PropertyVectorDistanceItem::setZ(Base::Quantity z)
{
    setData(QVariant::fromValue(Base::Vector3d(x().getValue(), y().getValue(), z.getValue())));
}

void PropertyVectorDistanceItem::propertyBound()
{
    if (isBound()) {
        m_x->bind(App::ObjectIdentifier(getPath())<<App::ObjectIdentifier::String("x"));
        m_y->bind(App::ObjectIdentifier(getPath())<<App::ObjectIdentifier::String("y"));
        m_z->bind(App::ObjectIdentifier(getPath())<<App::ObjectIdentifier::String("z"));
    };
}

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyPositionItem)

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyDirectionItem)

// ---------------------------------------------------------------

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyMatrixItem)

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
    QString text = QString::fromLatin1("[%1 %2 %3 %4 %5 %6 %7 %8 %9 %10 %11 %12 %13 %14 %15 %16]")
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
    QString data = QString::fromLatin1("FreeCAD.Matrix(%1, %2, %3, %4, %5, %6, %7, %8, %9, %10, %11, %12, %13, %14, %15, %16)")
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
    QString text = QString::fromLatin1("[%1 %2 %3 %4 %5 %6 %7 %8 %9 %10 %11 %12 %13 %14 %15 %16]")
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
    p.getRotation().getRawValue(dir, angle);
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

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyPlacementItem)

PropertyPlacementItem::PropertyPlacementItem() : init_axis(false), changed_value(false), rot_angle(0), rot_axis(0,0,1)
{
    m_a = static_cast<PropertyUnitItem*>(PropertyUnitItem::create());
    m_a->setParent(this);
    m_a->setPropertyName(QLatin1String(QT_TRANSLATE_NOOP("App::Property", "Angle")));
    this->appendChild(m_a);
    m_d = static_cast<PropertyVectorItem*>(PropertyVectorItem::create());
    m_d->setParent(this);
    m_d->setPropertyName(QLatin1String(QT_TRANSLATE_NOOP("App::Property", "Axis")));
    m_d->setReadOnly(true);
    this->appendChild(m_d);
    m_p = static_cast<PropertyVectorDistanceItem*>(PropertyVectorDistanceItem::create());
    m_p->setParent(this);
    m_p->setPropertyName(QLatin1String(QT_TRANSLATE_NOOP("App::Property", "Position")));
    m_p->setReadOnly(true);
    this->appendChild(m_p);
}

PropertyPlacementItem::~PropertyPlacementItem()
{
}

Base::Quantity PropertyPlacementItem::getAngle() const
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<Base::Placement>())
        return Base::Quantity(0.0);
    const Base::Placement& val = value.value<Base::Placement>();
    double angle;
    Base::Vector3d dir;
    val.getRotation().getRawValue(dir, angle);
    if (dir * this->rot_axis < 0.0)
        angle = -angle;
    return Base::Quantity(Base::toDegrees<double>(angle), Base::Unit::Angle);
}

void PropertyPlacementItem::setAngle(Base::Quantity angle)
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<Base::Placement>())
        return;

    Base::Placement val = value.value<Base::Placement>();
    Base::Rotation rot;
    rot.setValue(this->rot_axis, Base::toRadians<double>(angle.getValue()));
    val.setRotation(rot);
    changed_value = true;
    rot_angle = angle.getValue();
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

void PropertyPlacementItem::assignProperty(const App::Property* prop)
{
    // Choose an adaptive epsilon to avoid changing the axis when they are considered to
    // be equal. See https://forum.freecadweb.org/viewtopic.php?f=10&t=24662&start=10
    double eps = std::pow(10.0, -2*(decimals()+1));
    if (prop->getTypeId().isDerivedFrom(App::PropertyPlacement::getClassTypeId())) {
        const Base::Placement& value = static_cast<const App::PropertyPlacement*>(prop)->getValue();
        double angle;
        Base::Vector3d dir;
        value.getRotation().getRawValue(dir, angle);
        Base::Vector3d cross = this->rot_axis.Cross(dir);
        double len2 = cross.Sqr();
        if (angle != 0) {
            // vectors are not parallel
            if (len2 > eps)
                this->rot_axis = dir;
            // vectors point into opposite directions
            else if (this->rot_axis.Dot(dir) < 0)
                this->rot_axis = -this->rot_axis;
        }
        this->rot_angle = Base::toDegrees(angle);
    }
}

QVariant PropertyPlacementItem::value(const App::Property* prop) const
{
    assert(prop && prop->getTypeId().isDerivedFrom(App::PropertyPlacement::getClassTypeId()));

    const Base::Placement& value = static_cast<const App::PropertyPlacement*>(prop)->getValue();
    double angle;
    Base::Vector3d dir;
    value.getRotation().getRawValue(dir, angle);
    if (!init_axis) {
        if (m_a->hasExpression()) {
            QString str = m_a->expressionAsString();
            const_cast<PropertyPlacementItem*>(this)->rot_angle = str.toDouble();
        }
        else {
            const_cast<PropertyPlacementItem*>(this)->rot_angle = Base::toDegrees(angle);
        }

        PropertyItem* x = m_d->child(0);
        PropertyItem* y = m_d->child(1);
        PropertyItem* z = m_d->child(2);
        if (x->hasExpression()) {
            QString str = x->expressionAsString();
            dir.x = str.toDouble();
        }
        if (y->hasExpression()) {
            QString str = y->expressionAsString();
            dir.y = str.toDouble();
        }
        if (z->hasExpression()) {
            QString str = z->expressionAsString();
            dir.z = str.toDouble();
        }
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
    p.getRotation().getRawValue(dir, angle);
    angle = Base::toDegrees<double>(angle);
    pos = p.getPosition();
    QString data = QString::fromUtf8("Axis: (%1 %2 %3)\n"
                                     "Angle: %4\n"
                                     "Position: (%5  %6  %7)")
                    .arg(QLocale::system().toString(dir.x,'f',decimals()))
                    .arg(QLocale::system().toString(dir.y,'f',decimals()))
                    .arg(QLocale::system().toString(dir.z,'f',decimals()))
                    .arg(Base::Quantity(angle, Base::Unit::Angle).getUserString())
                    .arg(Base::Quantity(pos.x, Base::Unit::Length).getUserString())
                    .arg(Base::Quantity(pos.y, Base::Unit::Length).getUserString())
                    .arg(Base::Quantity(pos.z, Base::Unit::Length).getUserString());
    return QVariant(data);
}

QVariant PropertyPlacementItem::toString(const QVariant& prop) const
{
    const Base::Placement& p = prop.value<Base::Placement>();
    double angle;
    Base::Vector3d dir, pos;
    p.getRotation().getRawValue(dir, angle);
    angle = Base::toDegrees<double>(angle);
    pos = p.getPosition();
    QString data = QString::fromUtf8("[(%1 %2 %3); %4; (%5  %6  %7)]")
                    .arg(QLocale::system().toString(dir.x,'f',2))
                    .arg(QLocale::system().toString(dir.y,'f',2))
                    .arg(QLocale::system().toString(dir.z,'f',2))
                    .arg(Base::Quantity(angle, Base::Unit::Angle).getUserString())
                    .arg(Base::Quantity(pos.x, Base::Unit::Length).getUserString())
                    .arg(Base::Quantity(pos.y, Base::Unit::Length).getUserString())
                    .arg(Base::Quantity(pos.z, Base::Unit::Length).getUserString());
    return QVariant(data);
}

void PropertyPlacementItem::setValue(const QVariant& value)
{
    if (!value.canConvert<Base::Placement>())
        return;
    // Accept this only if the user changed the axis, angle or position but
    // not if >this< item loses focus
    if (!changed_value)
        return;
    changed_value = false;
    const Base::Placement& val = value.value<Base::Placement>();
    Base::Vector3d pos = val.getPosition();

    QString data = QString::fromLatin1("App.Placement("
                                      "App.Vector(%1,%2,%3),"
                                      "App.Rotation(App.Vector(%4,%5,%6),%7))")
                    .arg(pos.x)
                    .arg(pos.y)
                    .arg(pos.z)
                    .arg(rot_axis.x)
                    .arg(rot_axis.y)
                    .arg(rot_axis.z)
                    .arg(rot_angle,0);
    setPropertyValue(data);
}

QWidget* PropertyPlacementItem::createEditor(QWidget* parent, const QObject* receiver, const char* method) const
{
    PlacementEditor *pe = new PlacementEditor(this->propertyName(), parent);
    QObject::connect(pe, SIGNAL(valueChanged(const QVariant &)), receiver, method);
    pe->setDisabled(isReadOnly());
    return pe;
}

void PropertyPlacementItem::setEditorData(QWidget *editor, const QVariant& data) const
{
    PlacementEditor *pe = qobject_cast<PlacementEditor*>(editor);
    pe->setValue(data);
}

QVariant PropertyPlacementItem::editorData(QWidget *editor) const
{
    PlacementEditor *pe = qobject_cast<PlacementEditor*>(editor);
    return pe->value();
}

void PropertyPlacementItem::propertyBound()
{
    if (isBound()) {
        m_a->bind(App::ObjectIdentifier(getPath())<<App::ObjectIdentifier::String("Rotation")
                                                  <<App::ObjectIdentifier::String("Angle"));
   
        m_d->bind(App::ObjectIdentifier(getPath())<<App::ObjectIdentifier::String("Rotation")
                                                  <<App::ObjectIdentifier::String("Axis"));
        
        m_p->bind(App::ObjectIdentifier(getPath())<<App::ObjectIdentifier::String("Base"));
    }
}


// ---------------------------------------------------------------

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyEnumItem)

PropertyEnumItem::PropertyEnumItem()
{
}

QVariant PropertyEnumItem::value(const App::Property* prop) const
{
    assert(prop && prop->getTypeId().isDerivedFrom(App::PropertyEnumeration::getClassTypeId()));

    const App::PropertyEnumeration* prop_enum = static_cast<const App::PropertyEnumeration*>(prop);
    const std::vector<std::string>& value = prop_enum->getEnumVector();
    long currentItem = prop_enum->getValue();

    if (currentItem < 0 || currentItem >= static_cast<long>(value.size()))
        return QVariant(QString());
    return QVariant(QString::fromUtf8(value[currentItem].c_str()));
}

void PropertyEnumItem::setValue(const QVariant& value)
{
    if (!value.canConvert(QVariant::StringList))
        return;
    QStringList items = value.toStringList();
    if (!items.isEmpty()) {
        QByteArray val = items.front().toUtf8();
        std::string str = Base::Tools::escapedUnicodeFromUtf8(val);
        QString data = QString::fromLatin1("u\"%1\"").arg(QString::fromStdString(str));
        setPropertyValue(data);
    }
}

QWidget* PropertyEnumItem::createEditor(QWidget* parent, const QObject* receiver, const char* method) const
{
    QComboBox *cb = new QComboBox(parent);
    cb->setFrame(false);
    cb->setDisabled(isReadOnly());
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
                    commonModes << QString::fromUtf8(jt->c_str());
            }
            else {
                for (std::vector<std::string>::const_iterator jt = value.begin(); jt != value.end(); ++jt) {
                    if (commonModes.contains(QString::fromUtf8(jt->c_str())))
                        modes << QString::fromUtf8(jt->c_str());
                }

                commonModes = modes;
                modes.clear();
            }
        }
    }

    QComboBox *cb = qobject_cast<QComboBox*>(editor);
    if (!commonModes.isEmpty()) {
        cb->clear();
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

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyStringListItem)

PropertyStringListItem::PropertyStringListItem()
{
}

QWidget* PropertyStringListItem::createEditor(QWidget* parent, const QObject* receiver, const char* method) const
{
    Gui::LabelEditor* le = new Gui::LabelEditor(parent);
    le->setAutoFillBackground(true);
    le->setDisabled(isReadOnly());
    QObject::connect(le, SIGNAL(textChanged(const QString&)), receiver, method);
    return le;
}

void PropertyStringListItem::setEditorData(QWidget *editor, const QVariant& data) const
{
    Gui::LabelEditor *le = qobject_cast<Gui::LabelEditor*>(editor);
    QStringList list = data.toStringList();
    le->setText(list.join(QChar::fromLatin1('\n')));
}

QVariant PropertyStringListItem::editorData(QWidget *editor) const
{
    Gui::LabelEditor *le = qobject_cast<Gui::LabelEditor*>(editor);
    QString complete = le->text();
    QStringList list = complete.split(QChar::fromLatin1('\n'));
    return QVariant(list);
}

QVariant PropertyStringListItem::toString(const QVariant& prop) const
{
    QStringList list = prop.toStringList();
    if (list.size() > 10) {
        list = list.mid(0, 10);
        list.append(QLatin1String("..."));
    }

    QString text = QString::fromUtf8("[%1]").arg(list.join(QLatin1String(",")));
    text.replace(QString::fromUtf8("'"),QString::fromUtf8("\\'"));

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
        QString text(*it);
        text.replace(QString::fromUtf8("'"),QString::fromUtf8("\\'"));

        std::string pystr = Base::Tools::escapedUnicodeFromUtf8(text.toUtf8());
        str << "u\"" << pystr.c_str() << "\", ";
    }
    str << "]";
    setPropertyValue(data);
}

// ---------------------------------------------------------------

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyFloatListItem)

PropertyFloatListItem::PropertyFloatListItem()
{
}

QWidget* PropertyFloatListItem::createEditor(QWidget* parent, const QObject* receiver, const char* method) const
{
    Gui::LabelEditor* le = new Gui::LabelEditor(parent);
    le->setAutoFillBackground(true);
    le->setInputType(Gui::LabelEditor::Float);
    le->setDisabled(isReadOnly());
    QObject::connect(le, SIGNAL(textChanged(const QString&)), receiver, method);
    return le;
}

void PropertyFloatListItem::setEditorData(QWidget *editor, const QVariant& data) const
{
    Gui::LabelEditor *le = qobject_cast<Gui::LabelEditor*>(editor);
    QStringList list = data.toStringList();
    le->setText(list.join(QChar::fromLatin1('\n')));
}

QVariant PropertyFloatListItem::editorData(QWidget *editor) const
{
    Gui::LabelEditor *le = qobject_cast<Gui::LabelEditor*>(editor);
    QString complete = le->text();
    QStringList list = complete.split(QChar::fromLatin1('\n'));
    return QVariant(list);
}

QVariant PropertyFloatListItem::toString(const QVariant& prop) const
{
    QStringList list = prop.toStringList();
    if (list.size() > 10) {
        list = list.mid(0, 10);
        list.append(QLatin1String("..."));
    }
    QString text = QString::fromUtf8("[%1]").arg(list.join(QLatin1String(",")));
    return QVariant(text);
}

QVariant PropertyFloatListItem::value(const App::Property* prop) const
{
    assert(prop && prop->getTypeId().isDerivedFrom(App::PropertyFloatList::getClassTypeId()));

    QStringList list;
    const std::vector<double>& value = static_cast<const App::PropertyFloatList*>(prop)->getValues();
    for (std::vector<double>::const_iterator jt = value.begin(); jt != value.end(); ++jt) {
        list << QString::number(*jt, 'f', decimals());
    }

    return QVariant(list);
}

void PropertyFloatListItem::setValue(const QVariant& value)
{
    if (!value.canConvert(QVariant::StringList))
        return;
    QStringList values = value.toStringList();
    QString data;
    QTextStream str(&data);
    str << "[";
    for (QStringList::Iterator it = values.begin(); it != values.end(); ++it) {
        str << *it << ",";
    }
    str << "]";
    if (data == QString::fromUtf8("[,]"))
        data = QString::fromUtf8("[]");
    setPropertyValue(data);
}

// ---------------------------------------------------------------

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyIntegerListItem)

PropertyIntegerListItem::PropertyIntegerListItem()
{
}

QWidget* PropertyIntegerListItem::createEditor(QWidget* parent, const QObject* receiver, const char* method) const
{
    Gui::LabelEditor* le = new Gui::LabelEditor(parent);
    le->setAutoFillBackground(true);
    le->setInputType(Gui::LabelEditor::Integer);
    le->setDisabled(isReadOnly());
    QObject::connect(le, SIGNAL(textChanged(const QString&)), receiver, method);
    return le;
}

void PropertyIntegerListItem::setEditorData(QWidget *editor, const QVariant& data) const
{
    Gui::LabelEditor *le = qobject_cast<Gui::LabelEditor*>(editor);
    QStringList list = data.toStringList();
    le->setText(list.join(QChar::fromLatin1('\n')));
}

QVariant PropertyIntegerListItem::editorData(QWidget *editor) const
{
    Gui::LabelEditor *le = qobject_cast<Gui::LabelEditor*>(editor);
    QString complete = le->text();
    QStringList list = complete.split(QChar::fromLatin1('\n'));
    return QVariant(list);
}

QVariant PropertyIntegerListItem::toString(const QVariant& prop) const
{
    QStringList list = prop.toStringList();
    if (list.size() > 10) {
        list = list.mid(0, 10);
        list.append(QLatin1String("..."));
    }
    QString text = QString::fromUtf8("[%1]").arg(list.join(QLatin1String(",")));

    return QVariant(text);
}

QVariant PropertyIntegerListItem::value(const App::Property* prop) const
{
    assert(prop && prop->getTypeId().isDerivedFrom(App::PropertyIntegerList::getClassTypeId()));

    QStringList list;
    const std::vector<long>& value = static_cast<const App::PropertyIntegerList*>(prop)->getValues();
    for (std::vector<long>::const_iterator jt = value.begin(); jt != value.end(); ++jt) {
        list << QString::number(*jt);
    }

    return QVariant(list);
}

void PropertyIntegerListItem::setValue(const QVariant& value)
{
    if (!value.canConvert(QVariant::StringList))
        return;
    QStringList values = value.toStringList();
    QString data;
    QTextStream str(&data);
    str << "[";
    for (QStringList::Iterator it = values.begin(); it != values.end(); ++it) {
        str << *it << ",";
    }
    str << "]";
    if (data == QString::fromUtf8("[,]"))
        data = QString::fromUtf8("[]");
    setPropertyValue(data);
}

// --------------------------------------------------------------------

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyColorItem)

PropertyColorItem::PropertyColorItem()
{
}

QVariant PropertyColorItem::decoration(const QVariant& value) const
{
    QColor color = value.value<QColor>();

    int size = QApplication::style()->pixelMetric(QStyle::PM_ListViewIconSize);
    QPixmap p(size, size);
    p.fill(color);

    return QVariant(p);
}

QVariant PropertyColorItem::toString(const QVariant& prop) const
{
    QColor value = prop.value<QColor>();
    QString color = QString::fromLatin1("[%1, %2, %3]")
        .arg(value.red()).arg(value.green()).arg(value.blue());
    return QVariant(color);
}

QVariant PropertyColorItem::value(const App::Property* prop) const
{
    assert(prop && prop->getTypeId().isDerivedFrom(App::PropertyColor::getClassTypeId()));

    App::Color value = ((App::PropertyColor*)prop)->getValue();
    return QVariant(value.asValue<QColor>());
}

void PropertyColorItem::setValue(const QVariant& value)
{
    if (!value.canConvert<QColor>())
        return;
    QColor col = value.value<QColor>();
    App::Color val; val.setValue<QColor>(col);
    QString data = QString::fromLatin1("(%1,%2,%3)")
                    .arg(val.r,0,'f',decimals())
                    .arg(val.g,0,'f',decimals())
                    .arg(val.b,0,'f',decimals());
    setPropertyValue(data);
}

QWidget* PropertyColorItem::createEditor(QWidget* parent, const QObject* receiver, const char* method) const
{
    Gui::ColorButton* cb = new Gui::ColorButton( parent );
    cb->setDisabled(isReadOnly());
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

namespace Gui { namespace PropertyEditor {
    class Material
    {
    public:
        QColor diffuseColor;
        QColor ambientColor;
        QColor specularColor;
        QColor emissiveColor;
        float shininess;
        float transparency;
    };
}
}

Q_DECLARE_METATYPE(Gui::PropertyEditor::Material)

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyMaterialItem)

PropertyMaterialItem::PropertyMaterialItem()
{
    diffuse = static_cast<PropertyColorItem*>(PropertyColorItem::create());
    diffuse->setParent(this);
    diffuse->setPropertyName(QLatin1String("DiffuseColor"));
    this->appendChild(diffuse);

    ambient = static_cast<PropertyColorItem*>(PropertyColorItem::create());
    ambient->setParent(this);
    ambient->setPropertyName(QLatin1String("AmbientColor"));
    this->appendChild(ambient);

    specular = static_cast<PropertyColorItem*>(PropertyColorItem::create());
    specular->setParent(this);
    specular->setPropertyName(QLatin1String("SpecularColor"));
    this->appendChild(specular);

    emissive = static_cast<PropertyColorItem*>(PropertyColorItem::create());
    emissive->setParent(this);
    emissive->setPropertyName(QLatin1String("EmissiveColor"));
    this->appendChild(emissive);

    shininess = static_cast<PropertyFloatItem*>(PropertyFloatItem::create());
    shininess->setParent(this);
    shininess->setPropertyName(QLatin1String("Shininess"));
    this->appendChild(shininess);

    transparency = static_cast<PropertyFloatItem*>(PropertyFloatItem::create());
    transparency->setParent(this);
    transparency->setPropertyName(QLatin1String("Transparency"));
    this->appendChild(transparency);
}

PropertyMaterialItem::~PropertyMaterialItem()
{
}

void PropertyMaterialItem::propertyBound()
{
}

QColor PropertyMaterialItem::getDiffuseColor() const
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<Material>())
        return QColor();

    Material val = value.value<Material>();
    return val.diffuseColor;
}

void PropertyMaterialItem::setDiffuseColor(const QColor& color)
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<Material>())
        return;

    Material mat = value.value<Material>();
    mat.diffuseColor = color;
    setValue(QVariant::fromValue<Material>(mat));
}

QColor PropertyMaterialItem::getAmbientColor() const
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<Material>())
        return QColor();

    Material val = value.value<Material>();
    return val.ambientColor;
}

void PropertyMaterialItem::setAmbientColor(const QColor& color)
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<Material>())
        return;

    Material mat = value.value<Material>();
    mat.ambientColor = color;
    setValue(QVariant::fromValue<Material>(mat));
}

QColor PropertyMaterialItem::getSpecularColor() const
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<Material>())
        return QColor();

    Material val = value.value<Material>();
    return val.specularColor;
}

void PropertyMaterialItem::setSpecularColor(const QColor& color)
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<Material>())
        return;

    Material mat = value.value<Material>();
    mat.specularColor = color;
    setValue(QVariant::fromValue<Material>(mat));
}

QColor PropertyMaterialItem::getEmissiveColor() const
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<Material>())
        return QColor();

    Material val = value.value<Material>();
    return val.emissiveColor;
}

void PropertyMaterialItem::setEmissiveColor(const QColor& color)
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<Material>())
        return;

    Material mat = value.value<Material>();
    mat.emissiveColor = color;
    setValue(QVariant::fromValue<Material>(mat));
}

float PropertyMaterialItem::getShininess() const
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<Material>())
        return 0;

    Material val = value.value<Material>();
    return val.shininess;
}

void PropertyMaterialItem::setShininess(float s)
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<Material>())
        return;

    Material mat = value.value<Material>();
    mat.shininess = s;
    setValue(QVariant::fromValue<Material>(mat));
}

float PropertyMaterialItem::getTransparency() const
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<Material>())
        return 0;

    Material val = value.value<Material>();
    return val.transparency;
}

void PropertyMaterialItem::setTransparency(float t)
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<Material>())
        return;

    Material mat = value.value<Material>();
    mat.transparency = t;
    setValue(QVariant::fromValue<Material>(mat));
}

QVariant PropertyMaterialItem::decoration(const QVariant& value) const
{
    // use the diffuse color
    Material val = value.value<Material>();
    QColor color = val.diffuseColor;

    int size = QApplication::style()->pixelMetric(QStyle::PM_ListViewIconSize);
    QPixmap p(size, size);
    p.fill(color);

    return QVariant(p);
}

QVariant PropertyMaterialItem::toString(const QVariant& prop) const
{
    // use the diffuse color
    Material val = prop.value<Material>();
    QColor value = val.diffuseColor;
    QString color = QString::fromLatin1("[%1, %2, %3]")
        .arg(value.red()).arg(value.green()).arg(value.blue());
    return QVariant(color);
}

QVariant PropertyMaterialItem::toolTip(const App::Property* prop) const
{
    assert(prop && prop->getTypeId().isDerivedFrom(App::PropertyMaterial::getClassTypeId()));

    const App::Material& value = static_cast<const App::PropertyMaterial*>(prop)->getValue();
    QColor dc = value.diffuseColor.asValue<QColor>();
    QColor ac = value.ambientColor.asValue<QColor>();
    QColor sc = value.specularColor.asValue<QColor>();
    QColor ec = value.emissiveColor.asValue<QColor>();

    QString data = QString::fromUtf8(
        "Diffuse color: [%1, %2, %3]\n"
        "Ambient color: [%4, %5, %6]\n"
        "Specular color: [%7, %8, %9]\n"
        "Emissive color: [%10, %11, %12]\n"
        "Shininess: %13\n"
        "Transparency: %14"
        )
        .arg(dc.red()).arg(dc.green()).arg(dc.blue())
        .arg(ac.red()).arg(ac.green()).arg(ac.blue())
        .arg(sc.red()).arg(sc.green()).arg(sc.blue())
        .arg(ec.red()).arg(ec.green()).arg(ec.blue())
        .arg(value.shininess)
        .arg(value.transparency)
        ;

    return QVariant(data);
}

QVariant PropertyMaterialItem::value(const App::Property* prop) const
{
    assert(prop && prop->getTypeId().isDerivedFrom(App::PropertyMaterial::getClassTypeId()));

    const App::Material& value = static_cast<const App::PropertyMaterial*>(prop)->getValue();
    Material mat;

    mat.diffuseColor = value.diffuseColor.asValue<QColor>();
    mat.ambientColor = value.ambientColor.asValue<QColor>();
    mat.specularColor = value.specularColor.asValue<QColor>();
    mat.emissiveColor = value.emissiveColor.asValue<QColor>();
    mat.shininess = value.shininess;
    mat.transparency = value.transparency;

    return QVariant::fromValue<Material>(mat);
}

void PropertyMaterialItem::setValue(const QVariant& value)
{
    if (!value.canConvert<Material>())
        return;

    Material mat = value.value<Material>();
    App::Color dc; dc.setValue<QColor>(mat.diffuseColor);
    App::Color ac; ac.setValue<QColor>(mat.ambientColor);
    App::Color sc; sc.setValue<QColor>(mat.specularColor);
    App::Color ec; ec.setValue<QColor>(mat.emissiveColor);
    float s = mat.shininess;
    float t = mat.transparency;

    QString data = QString::fromLatin1(
        "App.Material("
        "DiffuseColor=(%1,%2,%3),"
        "AmbientColor=(%4,%5,%6),"
        "SpecularColor=(%7,%8,%9),"
        "EmissiveColor=(%10,%11,%12),"
        "Shininess=(%13),"
        "Transparency=(%14),"
        ")"
    )
    .arg(dc.r, 0, 'f', decimals())
    .arg(dc.g, 0, 'f', decimals())
    .arg(dc.b, 0, 'f', decimals())
    .arg(ac.r, 0, 'f', decimals())
    .arg(ac.g, 0, 'f', decimals())
    .arg(ac.b, 0, 'f', decimals())
    .arg(sc.r, 0, 'f', decimals())
    .arg(sc.g, 0, 'f', decimals())
    .arg(sc.b, 0, 'f', decimals())
    .arg(ec.r, 0, 'f', decimals())
    .arg(ec.g, 0, 'f', decimals())
    .arg(ec.b, 0, 'f', decimals())
    .arg(s, 0, 'f', decimals())
    .arg(t, 0, 'f', decimals())
    ;

    setPropertyValue(data);
}

QWidget* PropertyMaterialItem::createEditor(QWidget* parent, const QObject* receiver, const char* method) const
{
    Gui::ColorButton* cb = new Gui::ColorButton(parent);
    cb->setDisabled(isReadOnly());
    QObject::connect(cb, SIGNAL(changed()), receiver, method);
    return cb;
}

void PropertyMaterialItem::setEditorData(QWidget *editor, const QVariant& data) const
{
    if (!data.canConvert<Material>())
        return;

    Material val = data.value<Material>();
    Gui::ColorButton *cb = qobject_cast<Gui::ColorButton*>(editor);
    cb->setColor(val.diffuseColor);
}

QVariant PropertyMaterialItem::editorData(QWidget *editor) const
{
    Gui::ColorButton *cb = qobject_cast<Gui::ColorButton*>(editor);
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<Material>())
        return QVariant();

    Material val = value.value<Material>();
    val.diffuseColor = cb->color();
    return QVariant::fromValue<Material>(val);
}

// --------------------------------------------------------------------

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyMaterialListItem)

PropertyMaterialListItem::PropertyMaterialListItem()
{
    // This editor gets a list of materials but it only edits the first item.
    diffuse = static_cast<PropertyColorItem*>(PropertyColorItem::create());
    diffuse->setParent(this);
    diffuse->setPropertyName(QLatin1String("DiffuseColor"));
    this->appendChild(diffuse);

    ambient = static_cast<PropertyColorItem*>(PropertyColorItem::create());
    ambient->setParent(this);
    ambient->setPropertyName(QLatin1String("AmbientColor"));
    this->appendChild(ambient);

    specular = static_cast<PropertyColorItem*>(PropertyColorItem::create());
    specular->setParent(this);
    specular->setPropertyName(QLatin1String("SpecularColor"));
    this->appendChild(specular);

    emissive = static_cast<PropertyColorItem*>(PropertyColorItem::create());
    emissive->setParent(this);
    emissive->setPropertyName(QLatin1String("EmissiveColor"));
    this->appendChild(emissive);

    shininess = static_cast<PropertyFloatItem*>(PropertyFloatItem::create());
    shininess->setParent(this);
    shininess->setPropertyName(QLatin1String("Shininess"));
    this->appendChild(shininess);

    transparency = static_cast<PropertyFloatItem*>(PropertyFloatItem::create());
    transparency->setParent(this);
    transparency->setPropertyName(QLatin1String("Transparency"));
    this->appendChild(transparency);
}

PropertyMaterialListItem::~PropertyMaterialListItem()
{
}

void PropertyMaterialListItem::propertyBound()
{
}

QColor PropertyMaterialListItem::getDiffuseColor() const
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<QVariantList>())
        return QColor();

    QVariantList list = value.toList();
    if (list.isEmpty())
        return QColor();

    if (!list[0].canConvert<Material>())
        return QColor();

    Material mat = list[0].value<Material>();
    return mat.diffuseColor;
}

void PropertyMaterialListItem::setDiffuseColor(const QColor& color)
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<QVariantList>())
        return;

    QVariantList list = value.toList();
    if (list.isEmpty())
        return;

    if (!list[0].canConvert<Material>())
        return;

    Material mat = list[0].value<Material>();
    mat.diffuseColor = color;
    list[0] = QVariant::fromValue<Material>(mat);
    setValue(list);
}

QColor PropertyMaterialListItem::getAmbientColor() const
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<QVariantList>())
        return QColor();

    QVariantList list = value.toList();
    if (list.isEmpty())
        return QColor();

    if (!list[0].canConvert<Material>())
        return QColor();

    Material mat = list[0].value<Material>();
    return mat.ambientColor;
}

void PropertyMaterialListItem::setAmbientColor(const QColor& color)
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<QVariantList>())
        return;

    QVariantList list = value.toList();
    if (list.isEmpty())
        return;

    if (!list[0].canConvert<Material>())
        return;

    Material mat = list[0].value<Material>();
    mat.ambientColor = color;
    list[0] = QVariant::fromValue<Material>(mat);
    setValue(list);
}

QColor PropertyMaterialListItem::getSpecularColor() const
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<QVariantList>())
        return QColor();

    QVariantList list = value.toList();
    if (list.isEmpty())
        return QColor();

    if (!list[0].canConvert<Material>())
        return QColor();

    Material mat = list[0].value<Material>();
    return mat.specularColor;
}

void PropertyMaterialListItem::setSpecularColor(const QColor& color)
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<QVariantList>())
        return;

    QVariantList list = value.toList();
    if (list.isEmpty())
        return;

    if (!list[0].canConvert<Material>())
        return;

    Material mat = list[0].value<Material>();
    mat.specularColor = color;
    list[0] = QVariant::fromValue<Material>(mat);
    setValue(list);
}

QColor PropertyMaterialListItem::getEmissiveColor() const
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<QVariantList>())
        return QColor();

    QVariantList list = value.toList();
    if (list.isEmpty())
        return QColor();

    if (!list[0].canConvert<Material>())
        return QColor();

    Material mat = list[0].value<Material>();
    return mat.emissiveColor;
}

void PropertyMaterialListItem::setEmissiveColor(const QColor& color)
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<QVariantList>())
        return;

    QVariantList list = value.toList();
    if (list.isEmpty())
        return;

    if (!list[0].canConvert<Material>())
        return;

    Material mat = list[0].value<Material>();
    mat.emissiveColor = color;
    list[0] = QVariant::fromValue<Material>(mat);
    setValue(list);
}

float PropertyMaterialListItem::getShininess() const
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<QVariantList>())
        return 0;

    QVariantList list = value.toList();
    if (list.isEmpty())
        return 0;

    if (!list[0].canConvert<Material>())
        return 0;

    Material mat = list[0].value<Material>();
    return mat.shininess;
}

void PropertyMaterialListItem::setShininess(float s)
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<QVariantList>())
        return;

    QVariantList list = value.toList();
    if (list.isEmpty())
        return;

    if (!list[0].canConvert<Material>())
        return;

    Material mat = list[0].value<Material>();
    mat.shininess = s;
    list[0] = QVariant::fromValue<Material>(mat);
    setValue(list);
}

float PropertyMaterialListItem::getTransparency() const
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<QVariantList>())
        return 0;

    QVariantList list = value.toList();
    if (list.isEmpty())
        return 0;

    if (!list[0].canConvert<Material>())
        return 0;

    Material mat = list[0].value<Material>();
    return mat.transparency;
}

void PropertyMaterialListItem::setTransparency(float t)
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<QVariantList>())
        return;

    QVariantList list = value.toList();
    if (list.isEmpty())
        return;

    if (!list[0].canConvert<Material>())
        return;

    Material mat = list[0].value<Material>();
    mat.transparency = t;
    list[0] = QVariant::fromValue<Material>(mat);
    setValue(list);
}

QVariant PropertyMaterialListItem::decoration(const QVariant& value) const
{
    if (!value.canConvert<QVariantList>())
        return QVariant();

    QVariantList list = value.toList();
    if (list.isEmpty())
        return QVariant();

    if (!list[0].canConvert<Material>())
        return QVariant();

    // use the diffuse color
    Material mat = list[0].value<Material>();
    QColor color = mat.diffuseColor;

    int size = QApplication::style()->pixelMetric(QStyle::PM_ListViewIconSize);
    QPixmap p(size, size);
    p.fill(color);

    return QVariant(p);
}

QVariant PropertyMaterialListItem::toString(const QVariant& prop) const
{
    if (!prop.canConvert<QVariantList>())
        return QVariant();

    QVariantList list = prop.toList();
    if (list.isEmpty())
        return QVariant();

    if (!list[0].canConvert<Material>())
        return QVariant();

    // use the diffuse color
    Material mat = list[0].value<Material>();
    QColor value = mat.diffuseColor;
    QString color = QString::fromLatin1("[%1, %2, %3]")
        .arg(value.red()).arg(value.green()).arg(value.blue());
    return QVariant(color);
}

QVariant PropertyMaterialListItem::toolTip(const App::Property* prop) const
{
    assert(prop && prop->getTypeId().isDerivedFrom(App::PropertyMaterialList::getClassTypeId()));

    const std::vector<App::Material>& values = static_cast<const App::PropertyMaterialList*>(prop)->getValues();
    if (values.empty())
        return QVariant();

    App::Material value = values.front();
    QColor dc = value.diffuseColor.asValue<QColor>();
    QColor ac = value.ambientColor.asValue<QColor>();
    QColor sc = value.specularColor.asValue<QColor>();
    QColor ec = value.emissiveColor.asValue<QColor>();

    QString data = QString::fromUtf8(
        "Diffuse color: [%1, %2, %3]\n"
        "Ambient color: [%4, %5, %6]\n"
        "Specular color: [%7, %8, %9]\n"
        "Emissive color: [%10, %11, %12]\n"
        "Shininess: %13\n"
        "Transparency: %14"
        )
        .arg(dc.red()).arg(dc.green()).arg(dc.blue())
        .arg(ac.red()).arg(ac.green()).arg(ac.blue())
        .arg(sc.red()).arg(sc.green()).arg(sc.blue())
        .arg(ec.red()).arg(ec.green()).arg(ec.blue())
        .arg(value.shininess)
        .arg(value.transparency)
        ;

    return QVariant(data);
}

QVariant PropertyMaterialListItem::value(const App::Property* prop) const
{
    assert(prop && prop->getTypeId().isDerivedFrom(App::PropertyMaterialList::getClassTypeId()));

    const std::vector<App::Material>& value = static_cast<const App::PropertyMaterialList*>(prop)->getValues();
    QVariantList variantList;

    for (std::vector<App::Material>::const_iterator it = value.begin(); it != value.end(); ++it) {
        Material mat;
        mat.diffuseColor = it->diffuseColor.asValue<QColor>();
        mat.ambientColor = it->ambientColor.asValue<QColor>();
        mat.specularColor = it->specularColor.asValue<QColor>();
        mat.emissiveColor = it->emissiveColor.asValue<QColor>();
        mat.shininess = it->shininess;
        mat.transparency = it->transparency;

        variantList << QVariant::fromValue<Material>(mat);
    }

    return variantList;
}

void PropertyMaterialListItem::setValue(const QVariant& value)
{
    if (!value.canConvert<QVariantList>())
        return;

    QVariantList list = value.toList();
    if (list.isEmpty())
        return;

    QString data;
    QTextStream str(&data);
    str << "(";

    for (QVariantList::iterator it = list.begin(); it != list.end(); ++it) {
        Material mat = it->value<Material>();
        App::Color dc; dc.setValue<QColor>(mat.diffuseColor);
        App::Color ac; ac.setValue<QColor>(mat.ambientColor);
        App::Color sc; sc.setValue<QColor>(mat.specularColor);
        App::Color ec; ec.setValue<QColor>(mat.emissiveColor);
        float s = mat.shininess;
        float t = mat.transparency;

        QString item = QString::fromLatin1(
            "App.Material("
            "DiffuseColor=(%1,%2,%3),"
            "AmbientColor=(%4,%5,%6),"
            "SpecularColor=(%7,%8,%9),"
            "EmissiveColor=(%10,%11,%12),"
            "Shininess=(%13),"
            "Transparency=(%14),"
            ")"
            )
            .arg(dc.r, 0, 'f', decimals())
            .arg(dc.g, 0, 'f', decimals())
            .arg(dc.b, 0, 'f', decimals())
            .arg(ac.r, 0, 'f', decimals())
            .arg(ac.g, 0, 'f', decimals())
            .arg(ac.b, 0, 'f', decimals())
            .arg(sc.r, 0, 'f', decimals())
            .arg(sc.g, 0, 'f', decimals())
            .arg(sc.b, 0, 'f', decimals())
            .arg(ec.r, 0, 'f', decimals())
            .arg(ec.g, 0, 'f', decimals())
            .arg(ec.b, 0, 'f', decimals())
            .arg(s, 0, 'f', decimals())
            .arg(t, 0, 'f', decimals())
            ;
        str << item << ", ";
    }

    str << ")";

    setPropertyValue(data);
}

QWidget* PropertyMaterialListItem::createEditor(QWidget* parent, const QObject* receiver, const char* method) const
{
    Gui::ColorButton* cb = new Gui::ColorButton(parent);
    cb->setDisabled(isReadOnly());
    QObject::connect(cb, SIGNAL(changed()), receiver, method);
    return cb;
}

void PropertyMaterialListItem::setEditorData(QWidget *editor, const QVariant& data) const
{
    if (!data.canConvert<QVariantList>())
        return;

    QVariantList list = data.toList();
    if (list.isEmpty())
        return;

    if (!list[0].canConvert<Material>())
        return;

    // use the diffuse color
    Material mat = list[0].value<Material>();
    QColor color = mat.diffuseColor;

    Gui::ColorButton *cb = qobject_cast<Gui::ColorButton*>(editor);
    cb->setColor(color);
}

QVariant PropertyMaterialListItem::editorData(QWidget *editor) const
{
    Gui::ColorButton *cb = qobject_cast<Gui::ColorButton*>(editor);
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<QVariantList>())
        return QVariant();

    QVariantList list = value.toList();
    if (list.isEmpty())
        return QVariant();

    if (!list[0].canConvert<Material>())
        return QVariant();

    // use the diffuse color
    Material mat = list[0].value<Material>();
    mat.diffuseColor = cb->color();
    list[0] = QVariant::fromValue<Material>(mat);

    return list;
}

// --------------------------------------------------------------------

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyFileItem)

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
    QString data = QString::fromLatin1("\"%1\"").arg(val);
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
    fc->setDisabled(isReadOnly());
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

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyPathItem)

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
    QString data = QString::fromLatin1("\"%1\"").arg(val);
    setPropertyValue(data);
}

QVariant PropertyPathItem::toolTip(const App::Property* prop) const
{
    return value(prop);
}

QWidget* PropertyPathItem::createEditor(QWidget* parent, const QObject* receiver, const char* method) const
{
    Gui::FileChooser *fc = new Gui::FileChooser(parent);
    fc->setMode(FileChooser::Directory);
    fc->setAutoFillBackground(true);
    fc->setDisabled(isReadOnly());
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

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyTransientFileItem)

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
    QString data = QString::fromLatin1("\"%1\"").arg(val);
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
    fc->setDisabled(isReadOnly());
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

LinkSelection::LinkSelection(const QStringList& list) : link(list)
{
}

LinkSelection::~LinkSelection()
{
}

void LinkSelection::select()
{
    Gui::Selection().clearSelection();
    Gui::Selection().addSelection((const char*)link[0].toLatin1(),
                                  (const char*)link[1].toLatin1());
    this->deleteLater();
}

// ---------------------------------------------------------------

LinkLabel::LinkLabel (QWidget * parent) : QWidget(parent)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(1);

    label = new QLabel(this);
    label->setAutoFillBackground(true);
    label->setTextFormat(Qt::RichText);
    layout->addWidget(label);

    editButton = new QPushButton(QLatin1String("..."), this);
#if defined (Q_OS_MAC)
    editButton->setAttribute(Qt::WA_LayoutUsesWidgetRect); // layout size from QMacStyle was not correct
#endif
    editButton->setToolTip(tr("Change the linked object"));
    layout->addWidget(editButton);
    
    // setLayout(layout);
    
    connect(label, SIGNAL(linkActivated(const QString&)),
            this, SLOT(onLinkActivated(const QString&)));
    connect(editButton, SIGNAL(clicked()),
            this, SLOT(onEditClicked()));
}

LinkLabel::~LinkLabel()
{
}

void LinkLabel::setPropertyLink(const QStringList& o)
{
    link = o;
    QString linkcolor = QApplication::palette().color(QPalette::Link).name();
    QString text = QString::fromLatin1(
        "<html><head><style type=\"text/css\">"
        "p, li { white-space: pre-wrap; }"
        "</style></head><body>"
        "<p>"
        "<a href=\"%1.%2\"><span style=\" text-decoration: underline; color:%3;\">%4</span></a>"
        "</p></body></html>"
    )
    .arg(link[0])
    .arg(link[1])
    .arg(linkcolor)
    .arg(link[2]);
    label->setText(text);
}

QStringList LinkLabel::propertyLink() const
{
    return link;
}

void LinkLabel::onLinkActivated (const QString& s)
{
    Q_UNUSED(s);
    LinkSelection* select = new LinkSelection(link);
    QTimer::singleShot(50, select, SLOT(select()));
}

void LinkLabel::onEditClicked ()
{
    Gui::Dialog::DlgPropertyLink dlg(link, this);
    if (dlg.exec() == QDialog::Accepted) {
        setPropertyLink(dlg.propertyLink());
        /*emit*/ linkChanged(link);
    }
}

void LinkLabel::resizeEvent(QResizeEvent* e)
{
    editButton->setFixedWidth(e->size().height());
    editButton->setFixedHeight(e->size().height());
}


PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyLinkItem)

PropertyLinkItem::PropertyLinkItem()
{
}

QVariant PropertyLinkItem::toString(const QVariant& prop) const
{
    QStringList list = prop.toStringList();
    return QVariant(list[2]);
}

QVariant PropertyLinkItem::value(const App::Property* prop) const
{
    assert(prop && prop->getTypeId().isDerivedFrom(App::PropertyLink::getClassTypeId()));

    const App::PropertyLink* prop_link = static_cast<const App::PropertyLink*>(prop);
    App::PropertyContainer* c = prop_link->getContainer();

    // the list has five elements:
    // [document name, internal name, label, internal name of container, property name]
    App::DocumentObject* obj = prop_link->getValue();
    QStringList list;
    if (obj) {
        list << QString::fromLatin1(obj->getDocument()->getName());
        list << QString::fromLatin1(obj->getNameInDocument());
        list << QString::fromUtf8(obj->Label.getValue());
    }
    else {
        // no object assigned
        // the document name
        if (c->getTypeId().isDerivedFrom(App::DocumentObject::getClassTypeId())) {
            App::DocumentObject* obj = static_cast<App::DocumentObject*>(c);
            list << QString::fromLatin1(obj->getDocument()->getName());
        }
        else {
            list << QString::fromLatin1("");
        }

        // the internal object name
        list << QString::fromLatin1("Null");
        // the object label
        list << QString::fromLatin1("");
    }

    // the name of this object
    if (c->getTypeId().isDerivedFrom(App::DocumentObject::getClassTypeId())) {
        App::DocumentObject* obj = static_cast<App::DocumentObject*>(c);
        list << QString::fromLatin1(obj->getNameInDocument());
    }
    else {
        list << QString::fromLatin1("Null");
    }

    list << QString::fromLatin1(prop->getName());

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
        QString data;
        if ( o.isEmpty() )
            data = QString::fromLatin1("None");
        else
            data = QString::fromLatin1("App.getDocument('%1').getObject('%2')").arg(d).arg(o);
        setPropertyValue(data);
    }
}

QWidget* PropertyLinkItem::createEditor(QWidget* parent, const QObject* receiver, const char* method) const
{
    LinkLabel *ll = new LinkLabel(parent);
    ll->setAutoFillBackground(true);
    ll->setDisabled(isReadOnly());
    QObject::connect(ll, SIGNAL(linkChanged(const QStringList&)), receiver, method);
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

LinkListLabel::LinkListLabel (QWidget * parent) : QWidget(parent)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(1);

    label = new QLabel(this);
    label->setAutoFillBackground(true);
    layout->addWidget(label);

    editButton = new QPushButton(QLatin1String("..."), this);
    editButton->setToolTip(tr("Change the linked objects"));
    layout->addWidget(editButton);

    // setLayout(layout);
    connect(editButton, SIGNAL(clicked()),
            this, SLOT(onEditClicked()));
}

LinkListLabel::~LinkListLabel()
{
}

void LinkListLabel::setPropertyLinkList(const QVariantList& o)
{
    links = o;
    if (links.isEmpty()) {
        label->clear();
    }
    else if (links.size() == 1) {
        QStringList s = links.front().toStringList();
        label->setText(s[2]);
    }
    else {
        QStringList obj;
        for (QVariantList::iterator it = links.begin(); it != links.end(); ++it)
            obj << it->toStringList()[2];
        label->setText(QString::fromLatin1("[%1]").arg(obj.join(QString::fromLatin1(", "))));
    }
}

QVariantList LinkListLabel::propertyLinkList() const
{
    return links;
}

void LinkListLabel::onEditClicked ()
{
    QStringList list = links.front().toStringList();
    Gui::Dialog::DlgPropertyLink dlg(list, this);
    dlg.setSelectionMode(QAbstractItemView::ExtendedSelection);
    if (dlg.exec() == QDialog::Accepted) {
        setPropertyLinkList(dlg.propertyLinkList());
        Q_EMIT linkChanged(links);
    }
}

void LinkListLabel::resizeEvent(QResizeEvent* e)
{
    editButton->setFixedWidth(e->size().height());
}


PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyLinkListItem)

PropertyLinkListItem::PropertyLinkListItem()
{
}

QVariant PropertyLinkListItem::toString(const QVariant& prop) const
{
    QVariantList list = prop.toList();
    if (list.empty()) {
        return QString();
    }
    else if (list.size() == 1) {
        QStringList item = list.front().toStringList();
        return QString::fromLatin1("%1").arg(item[2]);
    }
    else {
        QStringList obj;
        for (QVariantList::iterator it = list.begin(); it != list.end(); ++it)
            obj << it->toStringList()[2];
        return QString::fromLatin1("[%1]").arg(obj.join(QString::fromLatin1(", ")));
    }
}

QVariant PropertyLinkListItem::value(const App::Property* prop) const
{
    assert(prop && prop->getTypeId().isDerivedFrom(App::PropertyLinkList::getClassTypeId()));

    const App::PropertyLinkList* prop_link = static_cast<const App::PropertyLinkList*>(prop);
    App::PropertyContainer* c = prop_link->getContainer();

    // the name of this object
    QString objName;
    if (c->getTypeId().isDerivedFrom(App::DocumentObject::getClassTypeId())) {
        App::DocumentObject* obj = static_cast<App::DocumentObject*>(c);
        objName = QString::fromLatin1(obj->getNameInDocument());
    }
    else {
        objName = QString::fromLatin1("Null");
    }

    // each item is a list of five elements:
    //[document name, internal name, label, internal name of container, property name]
    // the variant list contains at least one item
    std::vector<App::DocumentObject*> obj = prop_link->getValues();
    QVariantList varList;
    if (!obj.empty()) {
        for (std::vector<App::DocumentObject*>::iterator it = obj.begin(); it != obj.end(); ++it) {
            QStringList list;
            list << QString::fromLatin1((*it)->getDocument()->getName());
            list << QString::fromLatin1((*it)->getNameInDocument());
            list << QString::fromUtf8((*it)->Label.getValue());
            list << objName;
            list << QString::fromLatin1(prop->getName());
            varList << list;
        }
    }
    else {
        QStringList list;
        // no object assigned
        // the document name
        if (c->getTypeId().isDerivedFrom(App::DocumentObject::getClassTypeId())) {
            App::DocumentObject* obj = static_cast<App::DocumentObject*>(c);
            list << QString::fromLatin1(obj->getDocument()->getName());
        }
        else {
            list << QString::fromLatin1("");
        }

        // the internal object name
        list << QString::fromLatin1("Null");
        // the object label
        list << QString::fromLatin1("");
        list << objName;
        list << QString::fromLatin1(prop->getName());
        varList << list;
    }

    return QVariant(varList);
}

void PropertyLinkListItem::setValue(const QVariant& value)
{
    if (!value.canConvert(QVariant::List))
        return;
    QVariantList items = value.toList();
    QStringList data;
    for (QVariantList::iterator it = items.begin(); it != items.end(); ++it) {
        QStringList list = it->toStringList();
        QString d = list[0];
        QString o = list[1];
        if (!o.isEmpty())
            data << QString::fromLatin1("App.getDocument('%1').getObject('%2')").arg(d).arg(o);
    }

    setPropertyValue(QString::fromLatin1("[%1]").arg(data.join(QString::fromLatin1(", "))));
}

QWidget* PropertyLinkListItem::createEditor(QWidget* parent, const QObject* receiver, const char* method) const
{
    LinkListLabel *ll = new LinkListLabel(parent);
    ll->setAutoFillBackground(true);
    ll->setDisabled(isReadOnly());
    QObject::connect(ll, SIGNAL(linkChanged(const QVariantList&)), receiver, method);
    return ll;
}

void PropertyLinkListItem::setEditorData(QWidget *editor, const QVariant& data) const
{
    QVariantList list = data.toList();
    LinkListLabel *ll = static_cast<LinkListLabel*>(editor);
    ll->setPropertyLinkList(list);
}

QVariant PropertyLinkListItem::editorData(QWidget *editor) const
{
    LinkListLabel *ll = static_cast<LinkListLabel*>(editor);
    return QVariant(ll->propertyLinkList());
}

// --------------------------------------------------------------------

PropertyItemEditorFactory::PropertyItemEditorFactory()
{
}

PropertyItemEditorFactory::~PropertyItemEditorFactory()
{
}

#if (QT_VERSION >= 0x050300)
QWidget * PropertyItemEditorFactory::createEditor (int /*type*/, QWidget * /*parent*/) const
{
    // do not allow to create any editor widgets because we do that in subclasses of PropertyItem
    return 0;
}

QByteArray PropertyItemEditorFactory::valuePropertyName (int /*type*/) const
{
    // do not allow to set properties because we do that in subclasses of PropertyItem
    return "";
}
#else
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
#endif

#include "moc_PropertyItem.cpp"

