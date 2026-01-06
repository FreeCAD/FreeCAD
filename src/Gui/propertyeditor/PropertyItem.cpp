// SPDX-License-Identifier: LGPL-2.1-or-later
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


#include <algorithm>
#include <iomanip>
#include <limits>
#include <QApplication>
#include <QComboBox>
#include <QCheckBox>
#include <QFontDatabase>
#include <QLocale>
#include <QMessageBox>
#include <QPalette>
#include <QPixmap>
#include <QTextStream>
#include <QTimer>
#include <QtGlobal>
#include <QMenu>

#include "PropertyItem.h"
#include "PropertyView.h"

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Expression.h>
#include <App/PropertyGeo.h>
#include <App/PropertyFile.h>
#include <App/PropertyUnits.h>
#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/Tools.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Dialogs/DlgPropertyLink.h>
#include <Gui/FileDialog.h>
#include <Gui/MainWindow.h>
#include <Gui/Placement.h>
#include <Gui/QuantitySpinBox.h>
#include <Gui/Selection/Selection.h>
#include <Gui/SpinBox.h>
#include <Gui/VectorListEditor.h>
#include <Gui/ViewProviderDocumentObject.h>

// NOLINTBEGIN(cppcoreguidelines-pro-*,cppcoreguidelines-prefer-member-initializer)
using namespace Gui::PropertyEditor;
using namespace Gui::Dialog;

namespace
{
constexpr const int lowPrec = 2;
constexpr const int highPrec = 16;
}  // namespace


PropertyItemFactory& PropertyItemFactory::instance()
{
    static PropertyItemFactory inst;
    return inst;
}

PropertyItem* PropertyItemFactory::createPropertyItem(const char* sName) const
{
    return static_cast<PropertyItem*>(Produce(sName));
}

// ----------------------------------------------------

QString PropertyItemAttorney::toString(PropertyItem* item, const QVariant& value)
{
    return item->toString(value);
}

// ----------------------------------------------------

Q_DECLARE_METATYPE(Py::Object)  // NOLINT

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyItem)

PropertyItem::PropertyItem()
    : parentItem(nullptr)
    , readonly(false)
    , precision(Base::UnitsApi::getDecimals())
    , linked(false)
    , expanded(false)
{
    setAutoApply(true);
}

PropertyItem::~PropertyItem()
{
    qDeleteAll(childItems);
}

void PropertyItem::initialize()
{}

void PropertyItem::reset()
{
    qDeleteAll(childItems);
    childItems.clear();
}

void PropertyItem::onChange()
{
    if (hasExpression()) {
        for (auto child : std::as_const(childItems)) {
            if (child && child->hasExpression()) {
                child->setExpression(std::shared_ptr<App::Expression>());
            }
        }
        for (auto item = parentItem; item; item = item->parentItem) {
            if (item->hasExpression()) {
                item->setExpression(std::shared_ptr<App::Expression>());
            }
        }
    }
}

bool PropertyItem::hasAnyExpression() const
{
    if (ExpressionBinding::hasExpression()) {
        return true;
    }
    if (parentItem) {
        return parentItem->hasExpression();
    }
    return false;
}

void PropertyItem::setPropertyData(const std::vector<App::Property*>& items)
{
    // if we have a single property we can bind it for expression handling
    if (items.size() == 1) {
        const App::Property& prop = *items.front();

        try {
            // Check for 'DocumentObject' as parent because otherwise 'ObjectIdentifier' raises an
            // exception
            auto* docObj = freecad_cast<App::DocumentObject*>(prop.getContainer());
            if (docObj && !docObj->isReadOnly(&prop)) {
                App::ObjectIdentifier id(prop);
                std::vector<App::ObjectIdentifier> paths;
                prop.getPaths(paths);

                // there may be no paths available in this property (for example an empty constraint
                // list)
                if (id.getProperty() && !paths.empty()) {
                    bind(id);
                }
            }
        }
        // it may happen that setting properties is not possible
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
    for (auto it : propertyItems) {
        App::PropertyContainer* parent = it->getContainer();
        if (parent) {
            ro &= (parent->isReadOnly(it) || it->testStatus(App::Property::ReadOnly));
        }
    }
    this->setReadOnly(ro);
}

const std::vector<App::Property*>& PropertyItem::getPropertyData() const
{
    return propertyItems;
}

bool PropertyItem::hasProperty(const App::Property* prop) const
{
    auto it = std::ranges::find(propertyItems, prop);
    return (it != propertyItems.end());
}

void PropertyItem::assignProperty(const App::Property* prop)
{
    Q_UNUSED(prop)
}

bool PropertyItem::removeProperty(const App::Property* prop)
{
    auto it = std::ranges::find(propertyItems, prop);
    if (it != propertyItems.end()) {
        propertyItems.erase(it);
    }

    return propertyItems.empty();
}

bool PropertyItem::renameProperty(const App::Property* prop)
{
    setPropertyData({const_cast<App::Property*>(prop)});
    QString name = QString::fromLatin1(prop->getName());
    setPropertyName(name, name);
    return true;
}

App::Property* PropertyItem::getFirstProperty()
{
    if (propertyItems.empty()) {
        return nullptr;
    }
    return propertyItems.front();
}

const App::Property* PropertyItem::getFirstProperty() const
{
    if (propertyItems.empty()) {
        return nullptr;
    }
    return propertyItems.front();
}

void PropertyItem::setParent(PropertyItem* parent)
{
    parentItem = parent;
}

PropertyItem* PropertyItem::parent() const
{
    return parentItem;
}

void PropertyItem::appendChild(PropertyItem* item)
{
    childItems.append(item);
}

void PropertyItem::insertChild(int index, PropertyItem* child)
{
    childItems.insert(index, child);
}

/*!
 * \brief PropertyItem::removeChildren
 * Deletes the children in the range of [from, to]
 */
void PropertyItem::removeChildren(int from, int to)
{
    int count = to - from + 1;
    for (int i = 0; i < count; i++) {
        PropertyItem* child = childItems.takeAt(from);
        delete child;
    }
}

void PropertyItem::moveChild(int from, int to)
{
    childItems.move(from, to);
}

/*!
 * \brief PropertyItem::takeChild
 * Removes the child at index row but doesn't delete it
 */
PropertyItem* PropertyItem::takeChild(int row)
{
    PropertyItem* child = childItems.takeAt(row);
    child->setParent(nullptr);
    return child;
}

PropertyItem* PropertyItem::child(int row)
{
    return childItems.value(row);
}

int PropertyItem::childCount() const
{
    return childItems.count();
}

int PropertyItem::columnCount() const
{
    return PropertyItem::ColumnCount;
}

void PropertyItem::setReadOnly(bool ro)
{
    readonly = ro;
    for (auto it : std::as_const(childItems)) {
        it->setReadOnly(ro);
    }
}

bool PropertyItem::isReadOnly() const
{
    return readonly;
}

void PropertyItem::setLinked(bool value)
{
    linked = value;
    for (auto it : std::as_const(childItems)) {
        it->setLinked(value);
    }
}

bool PropertyItem::isLinked() const
{
    return linked;
}

void PropertyItem::setExpanded(bool enable)
{
    expanded = enable;
}

bool PropertyItem::isExpanded() const
{
    return expanded;
}

bool PropertyItem::testStatus(App::Property::Status pos) const
{
    std::vector<App::Property*>::const_iterator it;
    for (it = propertyItems.begin(); it != propertyItems.end(); ++it) {
        if ((*it)->testStatus(pos)) {
            return true;
        }
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
    return {displayText};
}

QVariant PropertyItem::toolTip(const App::Property* prop) const
{
    QString str = QApplication::translate("App::Property", prop->getDocumentation());
    return {str};
}

QVariant PropertyItem::decoration(const QVariant& value) const
{
    Q_UNUSED(value)
    return {};
}

QString PropertyItem::asNone(const Py::Object& pyobj) const
{
    Q_UNUSED(pyobj)
    return QStringLiteral("<None>");
}

QString PropertyItem::asString(const Py::Object& pyobj) const
{
    return QString::fromStdString(pyobj.as_string());
}

QString PropertyItem::asSequence(const Py::Object& pyobj) const
{
    std::ostringstream ss;
    ss << '[';
    Py::Sequence seq(pyobj);
    bool first = true;
    Py_ssize_t i = 0;
    for (i = 0; i < 2 && i < seq.size(); ++i) {
        if (first) {
            first = false;
        }
        else {
            ss << ", ";
        }
        ss << Py::Object(seq[i]).as_string();
    }

    if (i < seq.size()) {
        ss << "...";
    }
    ss << ']';
    return QString::fromUtf8(ss.str().c_str());
}

QString PropertyItem::asMapping(const Py::Object& pyobj) const
{
    std::ostringstream ss;
    ss << '{';
    Py::Mapping map(pyobj);
    bool first = true;
    auto it = map.begin();
    for (int i = 0; i < 2 && it != map.end(); ++it, ++i) {
        if (first) {
            first = false;
        }
        else {
            ss << ", ";
        }
        const auto& v = *it;
        ss << Py::Object(v.first).as_string() << ':' << Py::Object(v.second).as_string();
    }

    if (it != map.end()) {
        ss << "...";
    }
    ss << '}';
    return QString::fromUtf8(ss.str().c_str());
}

QString PropertyItem::toString(const Py::Object& pyobj) const
{
    if (pyobj.isNone()) {
        return asNone(pyobj);
    }
    if (pyobj.isSequence()) {
        return asSequence(pyobj);
    }
    if (pyobj.isMapping()) {
        return asMapping(pyobj);
    }

    return asString(pyobj);
}

QString PropertyItem::toString(const QVariant& prop) const
{
    if (prop != QVariant() || propertyItems.size() != 1) {
        return prop.toString();
    }

    std::ostringstream ss;
    Base::PyGILStateLocker lock;
    try {
        Py::Object pyobj(propertyItems[0]->getPyObject(), true);
        return toString(pyobj);
    }
    catch (Py::Exception&) {
        Base::PyException e;
        ss.str("");
        ss << "ERR: " << e.what();
    }
    catch (Base::Exception& e) {
        ss.str("");
        ss << "ERR: " << e.what();
    }
    catch (std::exception& e) {
        ss.str("");
        ss << "ERR: " << e.what();
    }
    catch (...) {
        ss.str("");
        ss << "ERR!";
    }

    return {QString::fromUtf8(ss.str().c_str())};
}

QVariant PropertyItem::value(const App::Property* /*prop*/) const
{
    return {};
}

void PropertyItem::setValue(const QVariant& /*value*/)
{}

QWidget* PropertyItem::
    createEditor(QWidget* /*parent*/, const std::function<void()>& /*method*/, FrameOption /*frameOption*/) const
{
    return nullptr;
}

void PropertyItem::setEditorData(QWidget* /*editor*/, const QVariant& /*data*/) const
{}

QVariant PropertyItem::editorData(QWidget* /*editor*/) const
{
    return {};
}

QWidget* PropertyItem::createExpressionEditor(QWidget* parent, const std::function<void()>& method) const
{
    if (!isBound()) {
        return nullptr;
    }
    auto le = new ExpLineEdit(parent, true);
    le->setFrame(false);
    le->setReadOnly(true);
    QObject::connect(le, &ExpLineEdit::textChanged, method);
    le->bind(getPath());
    le->setAutoApply(autoApply());
    return le;
}

void PropertyItem::setExpressionEditorData(QWidget* editor, const QVariant& data) const
{
    auto le = qobject_cast<QLineEdit*>(editor);
    if (le) {
        le->setText(data.toString());
    }
}

QVariant PropertyItem::expressionEditorData(QWidget* editor) const
{
    auto le = qobject_cast<QLineEdit*>(editor);
    if (le) {
        return {le->text()};
    }
    return {};
}

PropertyEditorWidget* PropertyItem::createPropertyEditorWidget(QWidget* parent) const
{
    auto editor = new PropertyEditorWidget(parent);
    connect(editor, &PropertyEditorWidget::buttonClick, this, [this]() {
        const auto& props = this->getPropertyData();
        if (!props.empty() && props[0]->getName() && props[0]->testStatus(App::Property::UserEdit)
            && props[0]->getContainer()) {
            props[0]->getContainer()->editProperty(props[0]->getName());
        }
    });
    return editor;
}

QString PropertyItem::propertyName() const
{
    if (propName.isEmpty()) {
        return QLatin1String(QT_TRANSLATE_NOOP("App::Property", "<empty>"));
    }
    return propName;
}

void PropertyItem::setPropertyName(const QString& name, const QString& realName)
{
    if (realName.size()) {
        propName = realName;
    }
    else {
        propName = name;
    }

    setObjectName(propName);

    QString display;
    bool upper = false;
    for (auto&& i : name) {
        if (i.isUpper() && !display.isEmpty()) {
            // if there is a sequence of capital letters do not insert spaces
            if (!upper) {
                QChar last = display.at(display.length() - 1);
                if (!last.isSpace()) {
                    display += QLatin1String(" ");
                }
            }
        }
        upper = i.isUpper();
        display += i;
    }

    propName = display;

    QString str = QApplication::translate("App::Property", propName.toUtf8());
    displayText = str;
}

void PropertyItem::setPropertyValue(const std::string& value)
{
    // Construct command for property assignment in one go, in case of any
    // intermediate changes caused by property change that may potentially
    // invalidate the current property array.
    std::ostringstream ss;
    for (auto prop : propertyItems) {
        App::PropertyContainer* parent = prop->getContainer();
        if (!parent || parent->isReadOnly(prop) || prop->testStatus(App::Property::ReadOnly)) {
            continue;
        }

        if (parent->isDerivedFrom<App::Document>()) {
            auto doc = static_cast<App::Document*>(parent);
            ss << "FreeCAD.getDocument('" << doc->getName() << "').";
        }
        else if (parent->isDerivedFrom<App::DocumentObject>()) {
            auto obj = static_cast<App::DocumentObject*>(parent);
            App::Document* doc = obj->getDocument();
            ss << "FreeCAD.getDocument('" << doc->getName() << "').getObject('"
               << obj->getNameInDocument() << "').";
        }
        else if (parent->isDerivedFrom<ViewProviderDocumentObject>()) {
            App::DocumentObject* obj = static_cast<ViewProviderDocumentObject*>(parent)->getObject();
            App::Document* doc = obj->getDocument();
            ss << "FreeCADGui.getDocument('" << doc->getName() << "').getObject('"
               << obj->getNameInDocument() << "').";
        }
        else {
            continue;
        }

        ss << parent->getPropertyPrefix() << prop->getName() << " = " << value << '\n';
    }

    std::string cmd = ss.str();
    if (cmd.empty()) {
        return;
    }

    try {
        Gui::Command::runCommand(Gui::Command::App, cmd.c_str());
    }
    catch (Base::PyException& e) {
        e.reportException();
        Base::Console().error("Stack Trace: %s\n", e.getStackTrace().c_str());
    }
    catch (Base::Exception& e) {
        e.reportException();
    }
    catch (...) {
        Base::Console().error("Unknown C++ exception in PropertyItem::setPropertyValue thrown\n");
    }
}

void PropertyItem::setNameToolTipOverride(const QString& name)
{
    nameToolTipOverride = name;
}

void PropertyItem::setPropertyValue(const QString& value)
{
    setPropertyValue(value.toStdString());
}

QVariant PropertyItem::dataPropertyName(int role) const
{
    if (role == Qt::ForegroundRole && linked) {
        return QVariant::fromValue(QColor(0x20, 0xaa, 0x20));  // NOLINT
    }

    if (role == Qt::BackgroundRole || role == Qt::ForegroundRole) {
        if (PropertyView::showAll() && propertyItems.size() == 1
            && propertyItems.front()->testStatus(App::Property::PropDynamic)
            && !propertyItems.front()->testStatus(App::Property::LockDynamic)) {
            return role == Qt::BackgroundRole
                ? QVariant::fromValue(QColor(0xFF, 0xFF, 0x99))  // NOLINT
                : QVariant::fromValue(QColor(0, 0, 0));
        }
        return {};
    }
    if (role == Qt::DisplayRole) {
        return displayName();
    }
    // no properties set
    if (propertyItems.empty()) {
        if (role == Qt::ToolTipRole && nameToolTipOverride.size()) {
            return nameToolTipOverride;
        }
        return {};
    }
    if (role == Qt::ToolTipRole) {
        QString type
            = QStringLiteral("Type: %1\nName: %2")
                  .arg(QString::fromLatin1(propertyItems[0]->getTypeId().getName()), objectName());

        QString doc = PropertyItem::toolTip(propertyItems[0]).toString();
        if (doc.isEmpty()) {
            doc = toolTip(propertyItems[0]).toString();
        }
        if (doc.size()) {
            return type + QLatin1String("\n\n") + doc;
        }
        return type;
    }

    return {};
}

QVariant PropertyItem::dataValue(int role) const
{
    // no properties set
    if (propertyItems.empty()) {
        PropertyItem* parent = this->parent();
        if (!parent || !parent->parent()) {
            return {};
        }
        if (role == Qt::EditRole) {
            return parent->property(qPrintable(objectName()));
        }
        if (role == Qt::DecorationRole) {
            QVariant val = parent->property(qPrintable(objectName()));
            return decoration(val);
        }
        if (role == Qt::DisplayRole) {
            QVariant val = parent->property(qPrintable(objectName()));
            return toString(val);
        }
        if (role == Qt::ForegroundRole) {
            if (hasExpression()) {
                return QVariant::fromValue(QApplication::palette().color(QPalette::Link));
            }
            return {};
        }

        return {};
    }
    if (role == Qt::EditRole) {
        return value(propertyItems[0]);
    }
    if (role == Qt::DecorationRole) {
        return decoration(value(propertyItems[0]));
    }
    if (role == Qt::DisplayRole) {
        return toString(value(propertyItems[0]));
    }
    if (role == Qt::ToolTipRole) {
        return toolTip(propertyItems[0]);
    }
    if (role == Qt::ForegroundRole) {
        if (hasExpression()) {
            return QVariant::fromValue(QApplication::palette().color(QPalette::Link));
        }
        return {};
    }

    return {};
}

QVariant PropertyItem::data(int column, int role) const
{
    if (column == PropertyItem::NameColumn) {
        return dataPropertyName(role);
    }

    return dataValue(role);
}

bool PropertyItem::setData(const QVariant& value)
{
    // This is the basic mechanism to set the value to
    // a property and if no property is set for this item
    // it delegates it to its parent which sets then the
    // property or delegates again to its parent...
    if (propertyItems.empty()) {
        PropertyItem* parent = this->parent();
        if (!parent || !parent->parent() || hasAnyExpression()) {
            return false;
        }

        parent->setProperty(qPrintable(objectName()), value);
        return true;
    }

    setValue(value);
    return true;
}

Qt::ItemFlags PropertyItem::flags(int column) const
{
    Qt::ItemFlags basicFlags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if (column == 1 && !isReadOnly()) {
        return basicFlags | Qt::ItemIsEditable;
    }

    return basicFlags;
}

int PropertyItem::row() const
{
    if (parentItem) {
        return parentItem->childItems.indexOf(const_cast<PropertyItem*>(this));  // NOLINT
    }

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
            e.reportException();
        }
    }

    return {};
}

// --------------------------------------------------------------------

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyStringItem)

PropertyStringItem::PropertyStringItem() = default;

QVariant PropertyStringItem::value(const App::Property* prop) const
{
    assert(prop && prop->isDerivedFrom<App::PropertyString>());

    std::string value = static_cast<const App::PropertyString*>(prop)->getValue();
    return {QString::fromUtf8(value.c_str())};
}

void PropertyStringItem::setValue(const QVariant& value)
{
    if (!hasExpression() && value.canConvert<QString>()) {
        std::string val = Base::InterpreterSingleton::strToPython(value.toString().toStdString());
        setPropertyValue(Base::Tools::quoted(val));
    }
}

QWidget* PropertyStringItem::createEditor(
    QWidget* parent,
    const std::function<void()>& method,
    FrameOption frameOption
) const
{
    auto le = new ExpLineEdit(parent);
    le->setFrame(static_cast<bool>(frameOption));
    QObject::connect(le, &ExpLineEdit::textChanged, method);
    if (isBound()) {
        le->bind(getPath());
        le->setAutoApply(autoApply());
    }

    return le;
}

void PropertyStringItem::setEditorData(QWidget* editor, const QVariant& data) const
{
    auto le = qobject_cast<QLineEdit*>(editor);
    le->setText(data.toString());
}

QVariant PropertyStringItem::editorData(QWidget* editor) const
{
    auto le = qobject_cast<QLineEdit*>(editor);
    return {le->text()};
}

// --------------------------------------------------------------------

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyFontItem)

PropertyFontItem::PropertyFontItem() = default;

QVariant PropertyFontItem::value(const App::Property* prop) const
{
    assert(prop && prop->isDerivedFrom<App::PropertyFont>());

    std::string value = static_cast<const App::PropertyFont*>(prop)->getValue();
    return {QString::fromUtf8(value.c_str())};
}

void PropertyFontItem::setValue(const QVariant& value)
{
    if (!hasExpression() && value.canConvert<QString>()) {
        setPropertyValue(Base::Tools::quoted(value.toString().toStdString()));
    }
}

QWidget* PropertyFontItem::createEditor(
    QWidget* parent,
    const std::function<void()>& method,
    FrameOption frameOption
) const
{
    auto cb = new QComboBox(parent);
    cb->setFrame(static_cast<bool>(frameOption));
    QObject::connect(cb, &QComboBox::textActivated, method);
    return cb;
}

void PropertyFontItem::setEditorData(QWidget* editor, const QVariant& data) const
{
    auto cb = qobject_cast<QComboBox*>(editor);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QStringList familyNames = QFontDatabase().families(QFontDatabase::Any);
#else
    QStringList familyNames = QFontDatabase::families(QFontDatabase::Any);
#endif
    cb->addItems(familyNames);
    int index = familyNames.indexOf(data.toString());
    cb->setCurrentIndex(index);
}

QVariant PropertyFontItem::editorData(QWidget* editor) const
{
    auto cb = qobject_cast<QComboBox*>(editor);
    return {cb->currentText()};
}

// --------------------------------------------------------------------

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertySeparatorItem)

QWidget* PropertySeparatorItem::createEditor(
    QWidget* parent,
    const std::function<void()>& method,
    FrameOption frameOption
) const
{
    Q_UNUSED(parent);
    Q_UNUSED(method);
    Q_UNUSED(frameOption);
    return nullptr;
}

// --------------------------------------------------------------------

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyIntegerItem)

PropertyIntegerItem::PropertyIntegerItem() = default;

QVariant PropertyIntegerItem::value(const App::Property* prop) const
{
    assert(prop && prop->isDerivedFrom<App::PropertyInteger>());

    int value = (int)static_cast<const App::PropertyInteger*>(prop)->getValue();
    return {value};
}

void PropertyIntegerItem::setValue(const QVariant& value)
{
    // if the item has an expression it issues the python code
    if (!hasExpression() && value.canConvert<int>()) {
        int val = value.toInt();
        setPropertyValue(std::to_string(val));
    }
}

QWidget* PropertyIntegerItem::createEditor(
    QWidget* parent,
    const std::function<void()>& method,
    FrameOption frameOption
) const
{
    auto sb = new Gui::IntSpinBox(parent);
    sb->setFrame(static_cast<bool>(frameOption));
    QObject::connect(sb, qOverload<int>(&Gui::IntSpinBox::valueChanged), method);

    if (isBound()) {
        sb->bind(getPath());
        sb->setAutoApply(autoApply());
    }

    return sb;
}

void PropertyIntegerItem::setEditorData(QWidget* editor, const QVariant& data) const
{
    auto sb = qobject_cast<QSpinBox*>(editor);
    sb->setRange(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
    sb->setValue(data.toInt());
}

QVariant PropertyIntegerItem::editorData(QWidget* editor) const
{
    auto sb = qobject_cast<QSpinBox*>(editor);
    return {sb->value()};
}

QString PropertyIntegerItem::toString(const QVariant& v) const
{
    QString string(PropertyItem::toString(v));

    if (hasExpression()) {
        string += QStringLiteral("  ( %1 )").arg(QString::fromStdString(getExpressionString()));
    }

    return string;
}


// --------------------------------------------------------------------

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyIntegerConstraintItem)

PropertyIntegerConstraintItem::PropertyIntegerConstraintItem() = default;

QVariant PropertyIntegerConstraintItem::value(const App::Property* prop) const
{
    assert(prop && prop->isDerivedFrom<App::PropertyIntegerConstraint>());

    int value = (int)static_cast<const App::PropertyIntegerConstraint*>(prop)->getValue();
    return {value};
}

void PropertyIntegerConstraintItem::setValue(const QVariant& value)
{
    // if the item has an expression it issues the python code
    if (!hasExpression() && value.canConvert<int>()) {
        int val = value.toInt();
        setPropertyValue(std::to_string(val));
    }
}

QWidget* PropertyIntegerConstraintItem::createEditor(
    QWidget* parent,
    const std::function<void()>& method,
    FrameOption frameOption
) const
{
    auto sb = new Gui::IntSpinBox(parent);
    sb->setFrame(static_cast<bool>(frameOption));
    QObject::connect(sb, qOverload<int>(&Gui::IntSpinBox::valueChanged), method);

    if (isBound()) {
        sb->bind(getPath());
        sb->setAutoApply(autoApply());
    }

    return sb;
}

void PropertyIntegerConstraintItem::setEditorData(QWidget* editor, const QVariant& data) const
{
    const auto prop = static_cast<const App::PropertyIntegerConstraint*>(getFirstProperty());

    const App::PropertyIntegerConstraint::Constraints* c = nullptr;
    if (prop) {
        c = prop->getConstraints();
    }

    auto sb = qobject_cast<QSpinBox*>(editor);
    if (c) {
        sb->setMinimum(int(c->LowerBound));
        sb->setMaximum(int(c->UpperBound));
        sb->setSingleStep(int(c->StepSize));
    }
    else {
        sb->setMinimum(min);
        sb->setMaximum(max);
        sb->setSingleStep(steps);
    }

    sb->setValue(data.toInt());
}

QVariant PropertyIntegerConstraintItem::editorData(QWidget* editor) const
{
    auto sb = qobject_cast<QSpinBox*>(editor);
    return {sb->value()};
}

QString PropertyIntegerConstraintItem::toString(const QVariant& v) const
{
    QString string(PropertyItem::toString(v));

    if (hasExpression()) {
        string += QStringLiteral("  ( %1 )").arg(QString::fromStdString(getExpressionString()));
    }

    return string;
}


// --------------------------------------------------------------------

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyFloatItem)

PropertyFloatItem::PropertyFloatItem() = default;

QString PropertyFloatItem::toString(const QVariant& prop) const
{
    double value = prop.toDouble();
    QString data = QLocale().toString(value, 'f', decimals());

    if (hasExpression()) {
        data += QStringLiteral("  ( %1 )").arg(QString::fromStdString(getExpressionString()));
    }

    return data;
}

QVariant PropertyFloatItem::value(const App::Property* prop) const
{
    assert(prop && prop->isDerivedFrom<App::PropertyFloat>());

    double value = static_cast<const App::PropertyFloat*>(prop)->getValue();
    return {value};
}

void PropertyFloatItem::setValue(const QVariant& value)
{
    // if the item has an expression it issues the python code
    if (!hasExpression() && value.canConvert<double>()) {
        std::ostringstream ss;
        ss << std::setprecision(highPrec) << value.toDouble();
        setPropertyValue(ss.str());
    }
}

QWidget* PropertyFloatItem::createEditor(
    QWidget* parent,
    const std::function<void()>& method,
    FrameOption frameOption
) const
{
    auto sb = new Gui::DoubleSpinBox(parent);
    sb->setFrame(static_cast<bool>(frameOption));
    sb->setDecimals(decimals());
    QObject::connect(sb, qOverload<double>(&Gui::DoubleSpinBox::valueChanged), method);

    if (isBound()) {
        sb->bind(getPath());
        sb->setAutoApply(autoApply());
    }

    return sb;
}

void PropertyFloatItem::setEditorData(QWidget* editor, const QVariant& data) const
{
    auto sb = qobject_cast<QDoubleSpinBox*>(editor);
    sb->setRange(
        static_cast<double>(std::numeric_limits<int>::min()),
        static_cast<double>(std::numeric_limits<int>::max())
    );
    sb->setValue(data.toDouble());
}

QVariant PropertyFloatItem::editorData(QWidget* editor) const
{
    auto sb = qobject_cast<QDoubleSpinBox*>(editor);
    return {sb->value()};
}

// --------------------------------------------------------------------


PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyUnitItem)

PropertyUnitItem::PropertyUnitItem() = default;

QString PropertyUnitItem::toString(const QVariant& prop) const
{
    const Base::Quantity& unit = prop.value<Base::Quantity>();
    std::string str = unit.getUserString();
    if (hasExpression()) {
        str += fmt::format("  ( {} )", getExpressionString());
    }

    return QString::fromStdString(str);
}

QVariant PropertyUnitItem::value(const App::Property* prop) const
{
    assert(prop && prop->isDerivedFrom<App::PropertyQuantity>());

    Base::Quantity value = static_cast<const App::PropertyQuantity*>(prop)->getQuantityValue();
    return QVariant::fromValue<Base::Quantity>(value);
}

void PropertyUnitItem::setValue(const QVariant& value)
{
    // if the item has an expression it handles the python code
    if (!hasExpression() && value.canConvert<Base::Quantity>()) {
        const Base::Quantity& val = value.value<Base::Quantity>();
        Base::QuantityFormat format(Base::QuantityFormat::Default, highPrec);
        setPropertyValue(val.toString(format));
    }
}

QWidget* PropertyUnitItem::createEditor(
    QWidget* parent,
    const std::function<void()>& method,
    FrameOption frameOption
) const
{
    auto infield = new Gui::QuantitySpinBox(parent);
    infield->setFrame(static_cast<bool>(frameOption));
    infield->setMinimumHeight(0);

    // if we are bound to an expression we need to bind it to the input field
    if (isBound()) {
        infield->bind(getPath());
        infield->setAutoApply(autoApply());
    }

    QObject::connect(infield, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), method);
    return infield;
}

void PropertyUnitItem::setEditorData(QWidget* editor, const QVariant& data) const
{
    const Base::Quantity& value = data.value<Base::Quantity>();

    auto infield = qobject_cast<Gui::QuantitySpinBox*>(editor);
    infield->setValue(value);
    infield->selectAll();
}

QVariant PropertyUnitItem::editorData(QWidget* editor) const
{
    auto infield = qobject_cast<Gui::QuantitySpinBox*>(editor);
    Base::Quantity value = infield->value();
    return QVariant::fromValue<Base::Quantity>(value);
}

// --------------------------------------------------------------------


PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyUnitConstraintItem)

PropertyUnitConstraintItem::PropertyUnitConstraintItem() = default;

void PropertyUnitConstraintItem::setEditorData(QWidget* editor, const QVariant& data) const
{
    const Base::Quantity& value = data.value<Base::Quantity>();

    auto infield = qobject_cast<Gui::QuantitySpinBox*>(editor);
    infield->setValue(value);
    infield->selectAll();

    const auto prop = static_cast<const App::PropertyQuantityConstraint*>(getFirstProperty());

    const App::PropertyQuantityConstraint::Constraints* c = nullptr;
    if (prop) {
        c = prop->getConstraints();
    }

    if (c) {
        infield->setMinimum(c->LowerBound);
        infield->setMaximum(c->UpperBound);
        infield->setSingleStep(c->StepSize);
    }
    else {
        infield->setMinimum(min);
        infield->setMaximum(max);
        infield->setSingleStep(steps);
    }
}

// --------------------------------------------------------------------

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyFloatConstraintItem)

PropertyFloatConstraintItem::PropertyFloatConstraintItem() = default;

QString PropertyFloatConstraintItem::toString(const QVariant& prop) const
{
    double value = prop.toDouble();
    return QLocale().toString(value, 'f', decimals());
}

QVariant PropertyFloatConstraintItem::value(const App::Property* prop) const
{
    assert(prop && prop->isDerivedFrom<App::PropertyFloatConstraint>());

    double value = static_cast<const App::PropertyFloatConstraint*>(prop)->getValue();
    return {value};
}

void PropertyFloatConstraintItem::setValue(const QVariant& value)
{
    // if the item has an expression it issues the python code
    if (!hasExpression() && value.canConvert<double>()) {
        std::ostringstream ss;
        ss << std::setprecision(highPrec) << value.toDouble();
        setPropertyValue(ss.str());
    }
}

QWidget* PropertyFloatConstraintItem::createEditor(
    QWidget* parent,
    const std::function<void()>& method,
    FrameOption frameOption
) const
{
    auto sb = new Gui::DoubleSpinBox(parent);
    sb->setDecimals(decimals());
    sb->setFrame(static_cast<bool>(frameOption));
    QObject::connect(sb, qOverload<double>(&Gui::DoubleSpinBox::valueChanged), method);

    if (isBound()) {
        sb->bind(getPath());
        sb->setAutoApply(autoApply());
    }

    return sb;
}

void PropertyFloatConstraintItem::setEditorData(QWidget* editor, const QVariant& data) const
{
    const auto prop = static_cast<const App::PropertyFloatConstraint*>(getFirstProperty());

    const App::PropertyFloatConstraint::Constraints* c = nullptr;
    if (prop) {
        c = prop->getConstraints();
    }

    auto sb = qobject_cast<QDoubleSpinBox*>(editor);
    if (c) {
        sb->setMinimum(c->LowerBound);
        sb->setMaximum(c->UpperBound);
        sb->setSingleStep(c->StepSize);
    }
    else {
        sb->setMinimum(min);
        sb->setMaximum(max);
        sb->setSingleStep(steps);
    }

    sb->setValue(data.toDouble());
}

QVariant PropertyFloatConstraintItem::editorData(QWidget* editor) const
{
    auto sb = qobject_cast<QDoubleSpinBox*>(editor);
    return {sb->value()};
}

// --------------------------------------------------------------------

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyPrecisionItem)

PropertyPrecisionItem::PropertyPrecisionItem()
{
    setDecimals(highPrec);
}

// --------------------------------------------------------------------

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyAngleItem)

PropertyAngleItem::PropertyAngleItem() = default;

void PropertyAngleItem::setEditorData(QWidget* editor, const QVariant& data) const
{
    PropertyUnitConstraintItem::setEditorData(editor, data);
}

QString PropertyAngleItem::toString(const QVariant& prop) const
{
    return PropertyUnitConstraintItem::toString(prop);
}

// --------------------------------------------------------------------

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyBoolItem)

PropertyBoolItem::PropertyBoolItem() = default;

QVariant PropertyBoolItem::value(const App::Property* prop) const
{
    assert(prop && prop->isDerivedFrom<App::PropertyBool>());

    bool value = static_cast<const App::PropertyBool*>(prop)->getValue();
    return {value};
}

void PropertyBoolItem::setValue(const QVariant& value)
{
    if (!hasExpression() && value.canConvert<bool>()) {
        std::string val = value.toBool() ? "True" : "False";
        setPropertyValue(val);
    }
}

QWidget* PropertyBoolItem::
    createEditor(QWidget* parent, const std::function<void()>& method, FrameOption /*frameOption*/) const
{
    auto checkbox = new QCheckBox(parent);
#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
    QObject::connect(checkbox, &QCheckBox::stateChanged, method);
#else
    QObject::connect(checkbox, &QCheckBox::checkStateChanged, method);
#endif

    return checkbox;
}

void PropertyBoolItem::setEditorData(QWidget* editor, const QVariant& data) const
{
    if (auto checkbox = qobject_cast<QCheckBox*>(editor)) {
        checkbox->setChecked(data.toBool());
    }
}

QVariant PropertyBoolItem::editorData(QWidget* editor) const
{
    if (auto checkbox = qobject_cast<QCheckBox*>(editor)) {
        return checkbox->isChecked();
    }
    return false;
}

// ---------------------------------------------------------------

namespace Gui::PropertyEditor
{
class VectorLineEdit: public Gui::ExpLineEdit
{
    int decimals;

public:
    explicit VectorLineEdit(int decimals, QWidget* parent = nullptr, bool expressionOnly = false)
        : Gui::ExpLineEdit(parent, expressionOnly)
        , decimals(decimals)
    {}

    bool apply(const std::string& propName) override
    {
        // NOLINTNEXTLINE
        if (!ExpressionBinding::apply(propName)) {  // clazy:exclude=skipped-base-method
            QVariant data = property("coords");
            if (data.canConvert<Base::Vector3d>()) {
                const Base::Vector3d& value = data.value<Base::Vector3d>();

                QString str = QStringLiteral("(%1, %2, %3)")
                                  .arg(value.x, 0, 'f', decimals)
                                  .arg(value.y, 0, 'f', decimals)
                                  .arg(value.z, 0, 'f', decimals);

                Gui::Command::doCommand(
                    Gui::Command::Doc,
                    "%s = %s",
                    propName.c_str(),
                    str.toLatin1().constData()
                );
                return true;
            }
        }

        return false;
    }
};
}  // namespace Gui::PropertyEditor

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

QString PropertyVectorItem::toString(const QVariant& prop) const
{
    QLocale loc;
    const Base::Vector3d& value = prop.value<Base::Vector3d>();
    QString data = QStringLiteral("[%1 %2 %3]")
                       .arg(
                           loc.toString(value.x, 'f', lowPrec),
                           loc.toString(value.y, 'f', lowPrec),
                           loc.toString(value.z, 'f', lowPrec)
                       );
    if (hasExpression()) {
        data += QStringLiteral("  ( %1 )").arg(QString::fromStdString(getExpressionString()));
    }
    return data;
}

QVariant PropertyVectorItem::value(const App::Property* prop) const
{
    assert(prop && prop->isDerivedFrom<App::PropertyVector>());

    const Base::Vector3d& value = static_cast<const App::PropertyVector*>(prop)->getValue();
    return QVariant::fromValue<Base::Vector3d>(value);
}

void PropertyVectorItem::setValue(const QVariant& value)
{
    if (hasExpression() || !value.canConvert<Base::Vector3d>()) {
        return;
    }
    const Base::Vector3d& val = value.value<Base::Vector3d>();
    QString data = QStringLiteral("(%1, %2, %3)")
                       .arg(val.x, 0, 'g', highPrec)
                       .arg(val.y, 0, 'g', highPrec)
                       .arg(val.z, 0, 'g', highPrec);
    setPropertyValue(data);
}

QWidget* PropertyVectorItem::createEditor(
    QWidget* parent,
    const std::function<void()>& /*method*/,
    FrameOption frameOption
) const
{
    auto le = new VectorLineEdit(decimals(), parent);
    le->setFrame(static_cast<bool>(frameOption));
    le->setReadOnly(true);

    if (isBound()) {
        le->bind(getPath());
        le->setAutoApply(autoApply());
    }

    return le;
}

void PropertyVectorItem::setEditorData(QWidget* editor, const QVariant& data) const
{
    QLocale loc;
    auto le = qobject_cast<QLineEdit*>(editor);
    const Base::Vector3d& value = data.value<Base::Vector3d>();
    QString text = QStringLiteral("[%1 %2 %3]")
                       .arg(
                           loc.toString(value.x, 'f', lowPrec),
                           loc.toString(value.y, 'f', lowPrec),
                           loc.toString(value.z, 'f', lowPrec)
                       );
    le->setProperty("coords", data);
    le->setText(text);
}

QVariant PropertyVectorItem::editorData(QWidget* editor) const
{
    auto le = qobject_cast<QLineEdit*>(editor);
    return {le->text()};
}

double PropertyVectorItem::x() const
{
    return data(1, Qt::EditRole).value<Base::Vector3d>().x;
}

void PropertyVectorItem::setX(double x)
{
    setData(QVariant::fromValue(Base::Vector3d(x, y(), z())));
}

double PropertyVectorItem::y() const
{
    return data(1, Qt::EditRole).value<Base::Vector3d>().y;
}

void PropertyVectorItem::setY(double y)
{
    setData(QVariant::fromValue(Base::Vector3d(x(), y, z())));
}

double PropertyVectorItem::z() const
{
    return data(1, Qt::EditRole).value<Base::Vector3d>().z;
}

void PropertyVectorItem::setZ(double z)
{
    setData(QVariant::fromValue(Base::Vector3d(x(), y(), z)));
}

void PropertyVectorItem::propertyBound()
{
    m_x->bind(App::ObjectIdentifier(getPath()) << App::ObjectIdentifier::String("x"));
    m_y->bind(App::ObjectIdentifier(getPath()) << App::ObjectIdentifier::String("y"));
    m_z->bind(App::ObjectIdentifier(getPath()) << App::ObjectIdentifier::String("z"));
}

// ---------------------------------------------------------------

PropertyEditorWidget::PropertyEditorWidget(QWidget* parent)
    : QWidget(parent)
{
    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);

    lineEdit = new QLineEdit(this);
    lineEdit->setReadOnly(true);
    layout->addWidget(lineEdit);

    button = new QPushButton(QStringLiteral("…"), this);
#if defined(Q_OS_MACOS)
    button->setAttribute(Qt::WA_LayoutUsesWidgetRect);  // layout size from QMacStyle was not correct
#endif
    layout->addWidget(button);

    connect(button, &QPushButton::clicked, this, &PropertyEditorWidget::buttonClick);

    // QAbstractItemView will call selectAll() if a QLineEdit is the focus
    // proxy. Since the QLineEdit here is read-only and not meant for editing,
    // do not set it as focus proxy. Otherwise, the text won't even shown for
    // most stylesheets (which contain a trick to hide the content of a selected
    // read-only/disabled editor widgets).
    //
    // setFocusProxy(lineEdit);
}

PropertyEditorWidget::~PropertyEditorWidget() = default;

void PropertyEditorWidget::resizeEvent(QResizeEvent* e)
{
    button->setFixedWidth(e->size().height());
    button->setFixedHeight(e->size().height());
}

void PropertyEditorWidget::showValue(const QVariant& d)
{
    lineEdit->setText(d.toString());
}

QVariant PropertyEditorWidget::value() const
{
    return variant;
}

void PropertyEditorWidget::setValue(const QVariant& val)
{
    variant = val;
    showValue(variant);
    Q_EMIT valueChanged(variant);
}

// ---------------------------------------------------------------

VectorListWidget::VectorListWidget(int decimals, QWidget* parent)
    : PropertyEditorWidget(parent)
    , decimals(decimals)
{
    connect(button, &QPushButton::clicked, this, &VectorListWidget::buttonClicked);
}

void VectorListWidget::buttonClicked()
{
    auto dlg = new VectorListEditor(decimals, this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setValues(value().value<QList<Base::Vector3d>>());
    QPoint p(0, 0);
    p = this->mapToGlobal(p);
    dlg->move(p);
    connect(dlg, &VectorListEditor::accepted, this, [this, dlg] {
        QVariant data = QVariant::fromValue<QList<Base::Vector3d>>(dlg->getValues());
        setValue(data);
    });

    Gui::adjustDialogPosition(dlg);
    dlg->exec();
}

void VectorListWidget::showValue(const QVariant& d)
{
    QLocale loc;
    QString data;
    const QList<Base::Vector3d>& value = d.value<QList<Base::Vector3d>>();
    if (value.isEmpty()) {
        data = QStringLiteral("[]");
    }
    else {
        data = QStringLiteral("[%1 %2 %3], ...")
                   .arg(
                       loc.toString(value[0].x, 'f', lowPrec),
                       loc.toString(value[0].y, 'f', lowPrec),
                       loc.toString(value[0].z, 'f', lowPrec)
                   );
    }
    lineEdit->setText(data);
}
// ---------------------------------------------------------------

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyVectorListItem)

PropertyVectorListItem::PropertyVectorListItem() = default;

QString PropertyVectorListItem::toString(const QVariant& prop) const
{
    const QList<Base::Vector3d>& value = prop.value<QList<Base::Vector3d>>();
    if (value.isEmpty()) {
        return QStringLiteral("[]");
    }
    QLocale loc;
    QString data = QStringLiteral("[%1 %2 %3], ...")
                       .arg(
                           loc.toString(value[0].x, 'f', lowPrec),
                           loc.toString(value[0].y, 'f', lowPrec),
                           loc.toString(value[0].z, 'f', lowPrec)
                       );

    if (hasExpression()) {
        data += QStringLiteral("  ( %1 )").arg(QString::fromStdString(getExpressionString()));
    }
    return data;
}

QVariant PropertyVectorListItem::value(const App::Property* prop) const
{
    assert(prop && prop->isDerivedFrom<App::PropertyVectorList>());

    const std::vector<Base::Vector3d>& value
        = static_cast<const App::PropertyVectorList*>(prop)->getValue();
    QList<Base::Vector3d> list;
    std::copy(value.begin(), value.end(), std::back_inserter(list));
    return QVariant::fromValue<QList<Base::Vector3d>>(list);
}

void PropertyVectorListItem::setValue(const QVariant& value)
{
    if (!value.canConvert<QList<Base::Vector3d>>()) {
        return;
    }
    const QList<Base::Vector3d>& val = value.value<QList<Base::Vector3d>>();
    QString data;
    QTextStream str(&data);
    str << "[";
    for (const auto& it : val) {
        str << QStringLiteral("(%1, %2, %3), ")
                   .arg(it.x, 0, 'g', highPrec)
                   .arg(it.y, 0, 'g', highPrec)
                   .arg(it.z, 0, 'g', highPrec);
    }
    str << "]";
    setPropertyValue(data);
}

QWidget* PropertyVectorListItem::
    createEditor(QWidget* parent, const std::function<void()>& method, FrameOption /*frameOption*/) const
{
    auto pe = new VectorListWidget(decimals(), parent);
    QObject::connect(pe, &VectorListWidget::valueChanged, method);
    return pe;
}

void PropertyVectorListItem::setEditorData(QWidget* editor, const QVariant& data) const
{
    auto pe = qobject_cast<VectorListWidget*>(editor);
    pe->setValue(data);
}

QVariant PropertyVectorListItem::editorData(QWidget* editor) const
{
    auto pe = qobject_cast<VectorListWidget*>(editor);
    return pe->value();
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

QString PropertyVectorDistanceItem::toString(const QVariant& prop) const
{
    const Base::Vector3d& value = prop.value<Base::Vector3d>();
    std::string str = fmt::format(
        "[{} {} {}]",
        Base::Quantity(value.x, Base::Unit::Length).getUserString(),
        Base::Quantity(value.y, Base::Unit::Length).getUserString(),
        Base::Quantity(value.z, Base::Unit::Length).getUserString()
    );
    if (hasExpression()) {
        str += fmt::format("  ( {} )", getExpressionString());
    }
    return QString::fromStdString(str);
}


QVariant PropertyVectorDistanceItem::value(const App::Property* prop) const
{
    assert(prop && prop->isDerivedFrom<App::PropertyVector>());

    const Base::Vector3d& value = static_cast<const App::PropertyVector*>(prop)->getValue();
    return QVariant::fromValue<Base::Vector3d>(value);
}

void PropertyVectorDistanceItem::setValue(const QVariant& variant)
{
    if (hasExpression() || !variant.canConvert<Base::Vector3d>()) {
        return;
    }
    const Base::Vector3d& value = variant.value<Base::Vector3d>();
    std::string val = fmt::format(
        "({:.{}g}, {:.{}g}, {:.{}g})",
        value.x,
        highPrec,
        value.y,
        highPrec,
        value.z,
        highPrec
    );
    setPropertyValue(val);
}

void PropertyVectorDistanceItem::setEditorData(QWidget* editor, const QVariant& data) const
{
    auto le = qobject_cast<QLineEdit*>(editor);
    le->setProperty("coords", data);
    le->setText(toString(data));
}

QWidget* PropertyVectorDistanceItem::createEditor(
    QWidget* parent,
    const std::function<void()>& /*method*/,
    FrameOption frameOption
) const
{
    auto le = new VectorLineEdit(decimals(), parent);
    le->setFrame(static_cast<bool>(frameOption));
    le->setReadOnly(true);

    if (isBound()) {
        le->bind(getPath());
        le->setAutoApply(autoApply());
    }

    return le;
}

QVariant PropertyVectorDistanceItem::editorData(QWidget* editor) const
{
    auto le = qobject_cast<QLineEdit*>(editor);
    return {le->text()};
}

Base::Quantity PropertyVectorDistanceItem::x() const
{
    return Base::Quantity(data(1, Qt::EditRole).value<Base::Vector3d>().x, Base::Unit::Length);
}

void PropertyVectorDistanceItem::setX(Base::Quantity x)
{
    setData(QVariant::fromValue(Base::Vector3d(x.getValue(), y().getValue(), z().getValue())));
}

Base::Quantity PropertyVectorDistanceItem::y() const
{
    return Base::Quantity(data(1, Qt::EditRole).value<Base::Vector3d>().y, Base::Unit::Length);
}

void PropertyVectorDistanceItem::setY(Base::Quantity y)
{
    setData(QVariant::fromValue(Base::Vector3d(x().getValue(), y.getValue(), z().getValue())));
}

Base::Quantity PropertyVectorDistanceItem::z() const
{
    return Base::Quantity(data(1, Qt::EditRole).value<Base::Vector3d>().z, Base::Unit::Length);
}

void PropertyVectorDistanceItem::setZ(Base::Quantity z)
{
    setData(QVariant::fromValue(Base::Vector3d(x().getValue(), y().getValue(), z.getValue())));
}

void PropertyVectorDistanceItem::propertyBound()
{
    if (isBound()) {
        m_x->bind(App::ObjectIdentifier(getPath()) << App::ObjectIdentifier::String("x"));
        m_y->bind(App::ObjectIdentifier(getPath()) << App::ObjectIdentifier::String("y"));
        m_z->bind(App::ObjectIdentifier(getPath()) << App::ObjectIdentifier::String("z"));
    };
}

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyPositionItem)

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyDirectionItem)

// ---------------------------------------------------------------

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyMatrixItem)

PropertyMatrixItem::PropertyMatrixItem()
{
    const int decimals = highPrec;
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

QString PropertyMatrixItem::toString(const QVariant& prop) const
{
    QLocale loc;
    const Base::Matrix4D& value = prop.value<Base::Matrix4D>();
    // NOLINTBEGIN
    QString text = QStringLiteral("[%1 %2 %3 %4 %5 %6 %7 %8 %9 %10 %11 %12 %13 %14 %15 %16]")
                       .arg(
                           loc.toString(value[0][0], 'f', lowPrec),  //(unsigned short usNdx)
                           loc.toString(value[0][1], 'f', lowPrec),
                           loc.toString(value[0][2], 'f', lowPrec),
                           loc.toString(value[0][3], 'f', lowPrec),
                           loc.toString(value[1][0], 'f', lowPrec),
                           loc.toString(value[1][1], 'f', lowPrec),
                           loc.toString(value[1][2], 'f', lowPrec),
                           loc.toString(value[1][3], 'f', lowPrec),
                           loc.toString(value[2][0], 'f', lowPrec)
                       )
                       .arg(
                           loc.toString(value[2][1], 'f', lowPrec),
                           loc.toString(value[2][2], 'f', lowPrec),
                           loc.toString(value[2][3], 'f', lowPrec),
                           loc.toString(value[3][0], 'f', lowPrec),
                           loc.toString(value[3][1], 'f', lowPrec),
                           loc.toString(value[3][2], 'f', lowPrec),
                           loc.toString(value[3][3], 'f', lowPrec)
                       );
    // NOLINTEND
    return text;
}

QVariant PropertyMatrixItem::value(const App::Property* prop) const
{
    assert(prop && prop->isDerivedFrom<App::PropertyMatrix>());

    const Base::Matrix4D& value = static_cast<const App::PropertyMatrix*>(prop)->getValue();
    return QVariant::fromValue<Base::Matrix4D>(value);
}

QVariant PropertyMatrixItem::toolTip(const App::Property* prop) const
{
    assert(prop && prop->isDerivedFrom<App::PropertyMatrix>());

    const Base::Matrix4D& value = static_cast<const App::PropertyMatrix*>(prop)->getValue();
    return {QString::fromStdString(value.analyse())};
}

void PropertyMatrixItem::setValue(const QVariant& value)
{
    if (hasExpression() || !value.canConvert<Base::Matrix4D>()) {
        return;
    }
    const Base::Matrix4D& val = value.value<Base::Matrix4D>();
    // NOLINTBEGIN
    QString data
        = QStringLiteral("FreeCAD.Matrix(%1, %2, %3, %4, %5, %6, %7, %8, %9, %10, %11, %12, %13, %14, %15, %16)")
              .arg(val[0][0], 0, 'g', highPrec)
              .arg(val[0][1], 0, 'g', highPrec)
              .arg(val[0][2], 0, 'g', highPrec)
              .arg(val[0][3], 0, 'g', highPrec)
              .arg(val[1][0], 0, 'g', highPrec)
              .arg(val[1][1], 0, 'g', highPrec)
              .arg(val[1][2], 0, 'g', highPrec)
              .arg(val[1][3], 0, 'g', highPrec)
              .arg(val[2][0], 0, 'g', highPrec)
              .arg(val[2][1], 0, 'g', highPrec)
              .arg(val[2][2], 0, 'g', highPrec)
              .arg(val[2][3], 0, 'g', highPrec)
              .arg(val[3][0], 0, 'g', highPrec)
              .arg(val[3][1], 0, 'g', highPrec)
              .arg(val[3][2], 0, 'g', highPrec)
              .arg(val[3][3], 0, 'g', highPrec);
    // NOLINTEND
    setPropertyValue(data);
}

QWidget* PropertyMatrixItem::createEditor(
    QWidget* parent,
    const std::function<void()>& /*method*/,
    FrameOption frameOption
) const
{
    auto le = new QLineEdit(parent);
    le->setFrame(static_cast<bool>(frameOption));
    le->setReadOnly(true);
    return le;
}

void PropertyMatrixItem::setEditorData(QWidget* editor, const QVariant& data) const
{
    auto le = qobject_cast<QLineEdit*>(editor);
    le->setText(toString(data));
}

QVariant PropertyMatrixItem::editorData(QWidget* editor) const
{
    auto le = qobject_cast<QLineEdit*>(editor);
    return {le->text()};
}

// clang-format off
double PropertyMatrixItem::getA11() const
{
    return data(1, Qt::EditRole).value<Base::Matrix4D>()[0][0];
}

void PropertyMatrixItem::setA11(double A11)
{
    setData(QVariant::fromValue(Base::Matrix4D(A11, getA12(), getA13(),getA14(),
                                               getA21(), getA22(), getA23(), getA24(),
                                               getA31(), getA32(), getA33(), getA34(),
                                               getA41(), getA42(), getA43(), getA44())));
}

double PropertyMatrixItem::getA12() const
{
    return data(1, Qt::EditRole).value<Base::Matrix4D>()[0][1];
}

void PropertyMatrixItem::setA12(double A12)
{
    setData(QVariant::fromValue(Base::Matrix4D(getA11(), A12, getA13(), getA14(),
                                               getA21(), getA22(), getA23(), getA24(),
                                               getA31(), getA32(), getA33(), getA34(),
                                               getA41(), getA42(), getA43(), getA44())));
}

double PropertyMatrixItem::getA13() const
{
    return data(1, Qt::EditRole).value<Base::Matrix4D>()[0][2];
}

void PropertyMatrixItem::setA13(double A13)
{
    setData(QVariant::fromValue(Base::Matrix4D(getA11(), getA12(), A13, getA14(),
                                               getA21(), getA22(), getA23(), getA24(),
                                               getA31(), getA32(), getA33(), getA34(),
                                               getA41(), getA42(), getA43(), getA44())));
}

double PropertyMatrixItem::getA14() const
{
    return data(1, Qt::EditRole).value<Base::Matrix4D>()[0][3];
}

void PropertyMatrixItem::setA14(double A14)
{
    setData(QVariant::fromValue(Base::Matrix4D(getA11(), getA12(), getA13(), A14,
                                               getA21(), getA22(), getA23(), getA24(),
                                               getA31(), getA32(), getA33(), getA34(),
                                               getA41(), getA42(), getA43(), getA44())));
}

double PropertyMatrixItem::getA21() const
{
    return data(1, Qt::EditRole).value<Base::Matrix4D>()[1][0];
}

void PropertyMatrixItem::setA21(double A21)
{
    setData(QVariant::fromValue(Base::Matrix4D(getA11(), getA12(), getA13(), getA14(),
                                               A21, getA22(), getA23(), getA24(),
                                               getA31(), getA32(), getA33(), getA34(),
                                               getA41(), getA42(), getA43(), getA44())));
}

double PropertyMatrixItem::getA22() const
{
    return data(1, Qt::EditRole).value<Base::Matrix4D>()[1][1];
}

void PropertyMatrixItem::setA22(double A22)
{
    setData(QVariant::fromValue(Base::Matrix4D(getA11(), getA12(), getA13(), getA14(),
                                               getA21(), A22, getA23(), getA24(),
                                               getA31(), getA32(), getA33(), getA34(),
                                               getA41(), getA42(), getA43(), getA44())));
}

double PropertyMatrixItem::getA23() const
{
    return data(1, Qt::EditRole).value<Base::Matrix4D>()[1][2];
}

void PropertyMatrixItem::setA23(double A23)
{
    setData(QVariant::fromValue(Base::Matrix4D(getA11(), getA12(), getA13(), getA14(),
                                               getA21(), getA22(), A23, getA24(),
                                               getA31(), getA32(), getA33(), getA34(),
                                               getA41(), getA42(), getA43(), getA44())));
}

double PropertyMatrixItem::getA24() const
{
    return data(1, Qt::EditRole).value<Base::Matrix4D>()[1][3];
}

void PropertyMatrixItem::setA24(double A24)
{
    setData(QVariant::fromValue(Base::Matrix4D(getA11(), getA12(), getA13(), getA14(),
                                               getA21(), getA22(), getA23(), A24,
                                               getA31(), getA32(), getA33(), getA34(),
                                               getA41(), getA42(), getA43(), getA44())));
}

double PropertyMatrixItem::getA31() const
{
    return data(1, Qt::EditRole).value<Base::Matrix4D>()[2][0];
}

void PropertyMatrixItem::setA31(double A31)
{
    setData(QVariant::fromValue(Base::Matrix4D(getA11(), getA12(), getA13(), getA14(),
                                               getA21(), getA22(), getA23(), getA24(),
                                               A31, getA32(), getA33(), getA34(),
                                               getA41(), getA42(), getA43(), getA44())));
}

double PropertyMatrixItem::getA32() const
{
    return data(1, Qt::EditRole).value<Base::Matrix4D>()[2][1];
}

void PropertyMatrixItem::setA32(double A32)
{
    setData(QVariant::fromValue(Base::Matrix4D(getA11(), getA12(), getA13(), getA14(),
                                               getA21(), getA22(), getA23(), getA24(),
                                               getA31(), A32, getA33(), getA34(),
                                               getA41(), getA42(), getA43(), getA44())));
}

double PropertyMatrixItem::getA33() const
{
    return data(1, Qt::EditRole).value<Base::Matrix4D>()[2][2];
}

void PropertyMatrixItem::setA33(double A33)
{
    setData(QVariant::fromValue(Base::Matrix4D(getA11(), getA12(), getA13(), getA14(),
                                               getA21(), getA22(), getA23(), getA24(),
                                               getA31(), getA32(), A33, getA34(),
                                               getA41(), getA42(), getA43(), getA44())));
}

double PropertyMatrixItem::getA34() const
{
    return data(1, Qt::EditRole).value<Base::Matrix4D>()[2][3];
}

void PropertyMatrixItem::setA34(double A34)
{
    setData(QVariant::fromValue(Base::Matrix4D(getA11(), getA12(), getA13(), getA14(),
                                               getA21(), getA22(), getA23(), getA24(),
                                               getA31(), getA32(), getA33(), A34,
                                               getA41(), getA42(), getA43(), getA44())));
}

double PropertyMatrixItem::getA41() const
{
    return data(1, Qt::EditRole).value<Base::Matrix4D>()[3][0];
}

void PropertyMatrixItem::setA41(double A41)
{
    setData(QVariant::fromValue(Base::Matrix4D(getA11(), getA12(), getA13(), getA14(),
                                               getA21(), getA22(), getA23(), getA24(),
                                               getA31(), getA32(), getA33(), getA34(),
                                               A41, getA42(), getA43(), getA44())));
}

double PropertyMatrixItem::getA42() const
{
    return data(1, Qt::EditRole).value<Base::Matrix4D>()[3][1];
}

void PropertyMatrixItem::setA42(double A42)
{
    setData(QVariant::fromValue(Base::Matrix4D(getA11(), getA12(), getA13(), getA14(),
                                               getA21(), getA22(), getA23(), getA24(),
                                               getA31(), getA32(), getA33(), getA34(),
                                               getA41(), A42, getA43(), getA44())));
}

double PropertyMatrixItem::getA43() const
{
    return data(1, Qt::EditRole).value<Base::Matrix4D>()[3][2];
}

void PropertyMatrixItem::setA43(double A43)
{
    setData(QVariant::fromValue(Base::Matrix4D(getA11(), getA12(), getA13(), getA14(),
                                               getA21(), getA22(), getA23(), getA24(),
                                               getA31(), getA32(), getA33(), getA34(),
                                               getA41(), getA42(), A43, getA44())));
}

double PropertyMatrixItem::getA44() const
{
    return data(1, Qt::EditRole).value<Base::Matrix4D>()[3][3];
}

void PropertyMatrixItem::setA44(double A44)
{
    setData(QVariant::fromValue(Base::Matrix4D(getA11(), getA12(), getA13(), getA14(),
                                               getA21(), getA22(), getA23(), getA24(),
                                               getA31(), getA32(), getA33(), getA34(),
                                               getA41(), getA42(), getA43(), A44)));
}
// clang-format on

// ---------------------------------------------------------------

RotationHelper::RotationHelper()
    : init_axis(false)
    , changed_value(false)
    , rot_angle(0)
    , rot_axis(0, 0, 1)
{}

void RotationHelper::setChanged(bool value)
{
    changed_value = value;
}

bool RotationHelper::hasChangedAndReset()
{
    if (!changed_value) {
        return false;
    }

    changed_value = false;
    return true;
}

bool RotationHelper::isAxisInitialized() const
{
    return init_axis;
}

void RotationHelper::setValue(const Base::Vector3d& axis, double angle)
{
    rot_axis = axis;
    rot_angle = angle;
    init_axis = true;
}

void RotationHelper::getValue(Base::Vector3d& axis, double& angle) const
{
    axis = rot_axis;
    angle = rot_angle;
}

double RotationHelper::getAngle(const Base::Rotation& val) const
{
    double angle {};
    Base::Vector3d dir;
    val.getRawValue(dir, angle);
    if (dir * this->rot_axis < 0.0) {
        angle = -angle;
    }
    return angle;
}

Base::Rotation RotationHelper::setAngle(double angle)
{
    Base::Rotation rot;
    rot.setValue(this->rot_axis, Base::toRadians<double>(angle));
    changed_value = true;
    rot_angle = angle;
    return rot;
}

Base::Vector3d RotationHelper::getAxis() const
{
    // We must store the rotation axis in a member because
    // if we read the value from the property we would always
    // get a normalized vector which makes it quite unhandy
    // to work with
    return this->rot_axis;
}

Base::Rotation RotationHelper::setAxis(const Base::Rotation& value, const Base::Vector3d& axis)
{
    this->rot_axis = axis;
    Base::Rotation rot = value;
    Base::Vector3d dummy;
    double angle {};
    rot.getValue(dummy, angle);
    if (dummy * axis < 0.0) {
        angle = -angle;
    }
    rot.setValue(axis, angle);
    changed_value = true;
    return rot;
}

void RotationHelper::assignProperty(const Base::Rotation& value, double eps)
{
    double angle {};
    Base::Vector3d dir;
    value.getRawValue(dir, angle);
    Base::Vector3d cross = this->rot_axis.Cross(dir);
    double len2 = cross.Sqr();
    if (angle != 0) {
        // vectors are not parallel
        if (len2 > eps) {
            this->rot_axis = dir;
        }
        // vectors point into opposite directions
        else if (this->rot_axis.Dot(dir) < 0) {
            this->rot_axis = -this->rot_axis;
        }
    }
    this->rot_angle = Base::toDegrees(angle);
}

// ---------------------------------------------------------------

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyRotationItem)

PropertyRotationItem::PropertyRotationItem()
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
}

PropertyRotationItem::~PropertyRotationItem() = default;

Base::Quantity PropertyRotationItem::getAngle() const
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<Base::Rotation>()) {
        return Base::Quantity(0.0);
    }

    const Base::Rotation& val = value.value<Base::Rotation>();
    double angle = h.getAngle(val);
    return Base::Quantity(Base::toDegrees<double>(angle), Base::Unit::Angle);
}

void PropertyRotationItem::setAngle(Base::Quantity angle)
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<Base::Rotation>()) {
        return;
    }

    Base::Rotation rot = h.setAngle(angle.getValue());
    setValue(QVariant::fromValue(rot));
}

Base::Vector3d PropertyRotationItem::getAxis() const
{
    return h.getAxis();
}

void PropertyRotationItem::setAxis(const Base::Vector3d& axis)
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<Base::Rotation>()) {
        return;
    }

    auto rot = value.value<Base::Rotation>();
    rot = h.setAxis(rot, axis);
    setValue(QVariant::fromValue(rot));
}

void PropertyRotationItem::assignProperty(const App::Property* prop)
{
    // Choose an adaptive epsilon to avoid changing the axis when they are considered to
    // be equal. See https://forum.freecad.org/viewtopic.php?f=10&t=24662&start=10
    double eps = std::pow(10.0, -2 * (decimals() + 1));  // NOLINT
    if (prop->isDerivedFrom<App::PropertyRotation>()) {
        const Base::Rotation& value = static_cast<const App::PropertyRotation*>(prop)->getValue();
        h.assignProperty(value, eps);
    }
}

QVariant PropertyRotationItem::value(const App::Property* prop) const
{
    assert(prop && prop->isDerivedFrom<App::PropertyRotation>());

    const Base::Rotation& value = static_cast<const App::PropertyRotation*>(prop)->getValue();
    double angle {};
    Base::Vector3d dir;
    value.getRawValue(dir, angle);
    if (!h.isAxisInitialized()) {
        if (m_a->hasExpression()) {
            QString str = m_a->expressionAsString();
            angle = str.toDouble();
        }
        else {
            angle = Base::toDegrees(angle);
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
        h.setValue(dir, angle);
    }
    return QVariant::fromValue<Base::Rotation>(value);
}

QVariant PropertyRotationItem::toolTip(const App::Property* prop) const
{
    assert(prop && prop->isDerivedFrom<App::PropertyRotation>());

    const Base::Rotation& p = static_cast<const App::PropertyRotation*>(prop)->getValue();
    double angle {};
    Base::Vector3d dir;
    p.getRawValue(dir, angle);
    angle = Base::toDegrees<double>(angle);

    QLocale loc;
    QString data
        = QStringLiteral(
              "Axis: (%1 %2 %3)\n"
              "Angle: %4"
        )
              .arg(
                  loc.toString(dir.x, 'f', decimals()),
                  loc.toString(dir.y, 'f', decimals()),
                  loc.toString(dir.z, 'f', decimals()),
                  QString::fromStdString(Base::Quantity(angle, Base::Unit::Angle).getUserString())
              );
    return {data};
}

QString PropertyRotationItem::toString(const QVariant& prop) const
{
    const Base::Rotation& p = prop.value<Base::Rotation>();
    double angle {};
    Base::Vector3d dir;
    p.getRawValue(dir, angle);
    angle = Base::toDegrees<double>(angle);

    QLocale loc;
    QString data
        = QStringLiteral("[(%1 %2 %3); %4]")
              .arg(
                  loc.toString(dir.x, 'f', lowPrec),
                  loc.toString(dir.y, 'f', lowPrec),
                  loc.toString(dir.z, 'f', lowPrec),
                  QString::fromStdString(Base::Quantity(angle, Base::Unit::Angle).getUserString())
              );
    return data;
}

void PropertyRotationItem::setValue(const QVariant& value)
{
    if (!value.canConvert<Base::Rotation>()) {
        return;
    }
    // Accept this only if the user changed the axis, angle or position but
    // not if >this< item loses focus
    if (!h.hasChangedAndReset()) {
        return;
    }

    Base::Vector3d axis;
    double angle {};
    h.getValue(axis, angle);
    std::string val = fmt::format(
        "App.Rotation(App.Vector({:.{}g},{:.{}g},{:.{}g}),{:.{}g})",
        axis.x,
        highPrec,
        axis.y,
        highPrec,
        axis.z,
        highPrec,
        angle,
        highPrec
    );
    setPropertyValue(val);
}

QWidget* PropertyRotationItem::createEditor(
    QWidget* parent,
    const std::function<void()>& method,
    FrameOption frameOption
) const
{
    Q_UNUSED(parent)
    Q_UNUSED(method)
    Q_UNUSED(frameOption)
    return nullptr;
}

void PropertyRotationItem::setEditorData(QWidget* editor, const QVariant& data) const
{
    Q_UNUSED(editor)
    Q_UNUSED(data)
}

QVariant PropertyRotationItem::editorData(QWidget* editor) const
{
    Q_UNUSED(editor)
    return {};
}

void PropertyRotationItem::propertyBound()
{
    if (isBound()) {
        m_a->bind(App::ObjectIdentifier(getPath()) << App::ObjectIdentifier::String("Angle"));

        m_d->bind(App::ObjectIdentifier(getPath()) << App::ObjectIdentifier::String("Axis"));
    }
}

// --------------------------------------------------------------------

PlacementEditor::PlacementEditor(QString name, QWidget* parent)
    : LabelButton(parent)
    , _task(nullptr)
    , propertyname {std::move(name)}
{
    propertyname.replace(QLatin1String(" "), QLatin1String(""));
}

PlacementEditor::~PlacementEditor() = default;

void PlacementEditor::browse()
{
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    Gui::Dialog::TaskPlacement* task {};
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
        connect(task, &TaskPlacement::placementChanged, this, &PlacementEditor::updateValue);
    }
    task->setPlacement(value().value<Base::Placement>());
    task->setPropertyName(propertyname);
    task->setSelection(Gui::Selection().getSelectionEx());
    task->bindObject();
    Gui::Control().showDialog(task);
}

void PlacementEditor::showValue(const QVariant& d)
{
    const Base::Placement& p = d.value<Base::Placement>();
    double angle {};
    Base::Vector3d dir;
    Base::Vector3d pos;
    p.getRotation().getRawValue(dir, angle);
    angle = Base::toDegrees<double>(angle);
    pos = p.getPosition();

    QLocale loc;
    QString data = QString::fromUtf8("[(%1 %2 %3);%4 \xc2\xb0;(%5 %6 %7)]")
                       .arg(
                           loc.toString(dir.x, 'f', lowPrec),
                           loc.toString(dir.y, 'f', lowPrec),
                           loc.toString(dir.z, 'f', lowPrec),
                           loc.toString(angle, 'f', lowPrec),
                           loc.toString(pos.x, 'f', lowPrec),
                           loc.toString(pos.y, 'f', lowPrec),
                           loc.toString(pos.z, 'f', lowPrec)
                       );
    getLabel()->setText(data);
}

void PlacementEditor::updateValue(const QVariant& v, bool incr, bool data)
{
    if (data) {
        if (incr) {
            QVariant u = value();
            const Base::Placement& plm = u.value<Base::Placement>();
            const Base::Placement& rel = v.value<Base::Placement>();
            Base::Placement newp = rel * plm;
            setValue(QVariant::fromValue<Base::Placement>(newp));
        }
        else {
            setValue(v);
        }
    }
}

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyPlacementItem)

PropertyPlacementItem::PropertyPlacementItem()
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

PropertyPlacementItem::~PropertyPlacementItem() = default;

Base::Quantity PropertyPlacementItem::getAngle() const
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<Base::Placement>()) {
        return Base::Quantity(0.0);
    }

    const Base::Placement& val = value.value<Base::Placement>();
    double angle = h.getAngle(val.getRotation());
    return Base::Quantity(Base::toDegrees<double>(angle), Base::Unit::Angle);
}

void PropertyPlacementItem::setAngle(Base::Quantity angle)
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<Base::Placement>()) {
        return;
    }

    auto val = value.value<Base::Placement>();
    Base::Rotation rot = h.setAngle(angle.getValue());
    val.setRotation(rot);
    setValue(QVariant::fromValue(val));
}

Base::Vector3d PropertyPlacementItem::getAxis() const
{
    return h.getAxis();
}

void PropertyPlacementItem::setAxis(const Base::Vector3d& axis)
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<Base::Placement>()) {
        return;
    }

    auto val = value.value<Base::Placement>();
    Base::Rotation rot = val.getRotation();
    rot = h.setAxis(rot, axis);
    val.setRotation(rot);
    setValue(QVariant::fromValue(val));
}

Base::Vector3d PropertyPlacementItem::getPosition() const
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<Base::Placement>()) {
        return Base::Vector3d(0, 0, 0);
    }
    const Base::Placement& val = value.value<Base::Placement>();
    return val.getPosition();
}

void PropertyPlacementItem::setPosition(const Base::Vector3d& pos)
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<Base::Placement>()) {
        return;
    }

    auto val = value.value<Base::Placement>();
    val.setPosition(pos);
    h.setChanged(true);
    setValue(QVariant::fromValue(val));
}

void PropertyPlacementItem::assignProperty(const App::Property* prop)
{
    // Choose an adaptive epsilon to avoid changing the axis when they are considered to
    // be equal. See https://forum.freecad.org/viewtopic.php?f=10&t=24662&start=10
    double eps = std::pow(10.0, -2 * (decimals() + 1));  // NOLINT
    if (prop->isDerivedFrom<App::PropertyPlacement>()) {
        const Base::Placement& value = static_cast<const App::PropertyPlacement*>(prop)->getValue();
        h.assignProperty(value.getRotation(), eps);
    }
}

QVariant PropertyPlacementItem::value(const App::Property* prop) const
{
    assert(prop && prop->isDerivedFrom<App::PropertyPlacement>());

    const Base::Placement& value = static_cast<const App::PropertyPlacement*>(prop)->getValue();
    double angle {};
    Base::Vector3d dir;
    value.getRotation().getRawValue(dir, angle);
    if (!h.isAxisInitialized()) {
        if (m_a->hasExpression()) {
            QString str = m_a->expressionAsString();
            angle = str.toDouble();
        }
        else {
            angle = Base::toDegrees(angle);
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
        h.setValue(dir, angle);
    }
    return QVariant::fromValue<Base::Placement>(value);
}

QVariant PropertyPlacementItem::toolTip(const App::Property* prop) const
{
    assert(prop && prop->isDerivedFrom<App::PropertyPlacement>());

    const Base::Placement& p = static_cast<const App::PropertyPlacement*>(prop)->getValue();
    double angle {};
    Base::Vector3d dir;
    Base::Vector3d pos;
    p.getRotation().getRawValue(dir, angle);
    angle = Base::toDegrees<double>(angle);
    pos = p.getPosition();

    QLocale loc;
    QString data
        = QStringLiteral(
              "Axis: (%1 %2 %3)\n"
              "Angle: %4\n"
              "Position: (%5  %6  %7)"
        )
              .arg(
                  loc.toString(dir.x, 'f', decimals()),
                  loc.toString(dir.y, 'f', decimals()),
                  loc.toString(dir.z, 'f', decimals()),
                  QString::fromStdString(Base::Quantity(angle, Base::Unit::Angle).getUserString()),
                  QString::fromStdString(Base::Quantity(pos.x, Base::Unit::Length).getUserString()),
                  QString::fromStdString(Base::Quantity(pos.y, Base::Unit::Length).getUserString()),
                  QString::fromStdString(Base::Quantity(pos.z, Base::Unit::Length).getUserString())
              );
    return {data};
}

QString PropertyPlacementItem::toString(const QVariant& prop) const
{
    const Base::Placement& p = prop.value<Base::Placement>();
    double angle {};
    Base::Vector3d dir;
    Base::Vector3d pos;
    p.getRotation().getRawValue(dir, angle);
    angle = Base::toDegrees<double>(angle);
    pos = p.getPosition();

    QLocale loc;
    QString data
        = QStringLiteral("[(%1 %2 %3); %4; (%5  %6  %7)]")
              .arg(
                  loc.toString(dir.x, 'f', lowPrec),
                  loc.toString(dir.y, 'f', lowPrec),
                  loc.toString(dir.z, 'f', lowPrec),
                  QString::fromStdString(Base::Quantity(angle, Base::Unit::Angle).getUserString()),
                  QString::fromStdString(Base::Quantity(pos.x, Base::Unit::Length).getUserString()),
                  QString::fromStdString(Base::Quantity(pos.y, Base::Unit::Length).getUserString()),
                  QString::fromStdString(Base::Quantity(pos.z, Base::Unit::Length).getUserString())
              );
    return data;
}

void PropertyPlacementItem::setValue(const QVariant& value)
{
    if (hasExpression() || !value.canConvert<Base::Placement>()) {
        return;
    }
    // Accept this only if the user changed the axis, angle or position but
    // not if >this< item loses focus
    if (!h.hasChangedAndReset()) {
        return;
    }

    const Base::Placement& val = value.value<Base::Placement>();
    Base::Vector3d pos = val.getPosition();

    Base::Vector3d axis;
    double angle {};
    h.getValue(axis, angle);
    std::string str = fmt::format(
        "App.Placement("
        "App.Vector({:.{}g},{:.{}g},{:.{}g}),"
        "App.Rotation(App.Vector({:.{}g},{:.{}g},{:.{}g}),{:.{}g}))",
        pos.x,
        highPrec,
        pos.y,
        highPrec,
        pos.z,
        highPrec,
        axis.x,
        highPrec,
        axis.y,
        highPrec,
        axis.z,
        highPrec,
        angle,
        highPrec
    );
    setPropertyValue(str);
}

QWidget* PropertyPlacementItem::
    createEditor(QWidget* parent, const std::function<void()>& method, FrameOption /*frameOption*/) const
{
    auto pe = new PlacementEditor(this->propertyName(), parent);
    QObject::connect(pe, &PlacementEditor::valueChanged, method);

    // The Placement dialog only works if property is part of a DocumentObject
    bool readonly = isReadOnly();
    if (auto prop = getFirstProperty()) {
        readonly |= (!prop->getContainer()->isDerivedFrom<App::DocumentObject>());
    }
    pe->setDisabled(readonly);
    return pe;
}

void PropertyPlacementItem::setEditorData(QWidget* editor, const QVariant& data) const
{
    auto pe = qobject_cast<PlacementEditor*>(editor);
    pe->setValue(data);
}

QVariant PropertyPlacementItem::editorData(QWidget* editor) const
{
    auto pe = qobject_cast<PlacementEditor*>(editor);
    return pe->value();
}

void PropertyPlacementItem::propertyBound()
{
    if (isBound()) {
        m_a->bind(
            App::ObjectIdentifier(getPath())
            << App::ObjectIdentifier::String("Rotation") << App::ObjectIdentifier::String("Angle")
        );

        m_d->bind(
            App::ObjectIdentifier(getPath())
            << App::ObjectIdentifier::String("Rotation") << App::ObjectIdentifier::String("Axis")
        );

        m_p->bind(App::ObjectIdentifier(getPath()) << App::ObjectIdentifier::String("Base"));
    }
}


// ---------------------------------------------------------------

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyEnumItem)

PropertyEnumItem::PropertyEnumItem()
    : m_enum(nullptr)
{
    if (PropertyView::showAll()) {
        m_enum = static_cast<PropertyStringListItem*>(PropertyStringListItem::create());
        m_enum->setParent(this);
        m_enum->setPropertyName(QLatin1String(QT_TRANSLATE_NOOP("App::Property", "Enum")));
        this->appendChild(m_enum);
    }
}

void PropertyEnumItem::propertyBound()
{
    if (m_enum && isBound()) {
        m_enum->bind(App::ObjectIdentifier(getPath()) << App::ObjectIdentifier::String("Enum"));
    }
}

void PropertyEnumItem::setEnum(const QStringList& values)
{
    setData(values);
}

QStringList PropertyEnumItem::getEnum() const
{
    QStringList res;
    auto prop = getFirstProperty();
    if (prop && prop->isDerivedFrom<App::PropertyEnumeration>()) {
        const auto prop_enum = static_cast<const App::PropertyEnumeration*>(prop);
        std::vector<std::string> enums = prop_enum->getEnumVector();
        for (const auto& it : enums) {
            res.push_back(QString::fromStdString(it));
        }
    }
    return res;
}

QVariant PropertyEnumItem::value(const App::Property* prop) const
{
    assert(prop && prop->isDerivedFrom<App::PropertyEnumeration>());

    const auto prop_enum = static_cast<const App::PropertyEnumeration*>(prop);
    if (!prop_enum->isValid()) {
        return {QString()};
    }
    return {QString::fromUtf8(prop_enum->getValueAsString())};
}

void PropertyEnumItem::setValue(const QVariant& value)
{
    if (hasExpression()) {
        return;
    }

    std::ostringstream ss;

    if (value.userType() == QMetaType::QStringList) {
        QStringList values = value.toStringList();
        ss << "[";
        for (const auto& it : values) {
            QString text(it);
            text.replace(QStringLiteral("'"), QStringLiteral("\\'"));

            std::string str = Base::Tools::escapedUnicodeFromUtf8(text.toUtf8());
            str = Base::InterpreterSingleton::strToPython(str);
            ss << "u\"" << str << "\", ";
        }
        ss << "]";
        setPropertyValue(ss.str());
    }
    else if (value.canConvert<QString>()) {
        std::string str = Base::Tools::escapedUnicodeFromUtf8(value.toString().toUtf8());
        ss << "u\"" << str << "\"";
        setPropertyValue(ss.str());
    }
}

namespace
{

class EnumItems;

struct EnumItem
{
    QString text;
    QString fullText;
    std::shared_ptr<EnumItems> children;
    explicit EnumItem(QString t = QString(), QString f = QString())
        : text(std::move(t))
        , fullText(std::move(f))
    {}
    void populate(QMenu* menu);
};

class EnumItems: public std::vector<EnumItem>
{
};

void EnumItem::populate(QMenu* menu)
{
    if (!children || children->empty()) {
        auto action = menu->addAction(text);
        action->setData(fullText);
        return;
    }
    auto subMenu = menu->addMenu(text);
    for (auto& item : *children) {
        item.populate(subMenu);
    }
}

std::shared_ptr<EnumItems> getEnumItems(const QStringList& commonModes)  // NOLINT
{
    int index = -1;
    std::shared_ptr<EnumItems> enumItems;
    for (auto& mode : commonModes) {
        ++index;
        auto fields = mode.split(QStringLiteral("|"));
        if (!enumItems && fields.size() <= 1) {
            continue;
        }
        if (!enumItems) {
            enumItems = std::make_shared<EnumItems>();
            for (int i = 0; i < index; ++i) {
                enumItems->emplace_back(commonModes[i], mode);
            }
        }
        auto children = enumItems;
        int j = -1;
        for (auto& field : fields) {
            ++j;
            field = field.trimmed();
            auto it = children->end();
            if (field.isEmpty()) {
                if (!children->empty()) {
                    --it;
                }
                else {
                    continue;
                }
            }
            else {
                it = std::find_if(children->begin(), children->end(), [&field](const EnumItem& item) {
                    return item.text == field;
                });
                if (it == children->end()) {
                    it = children->emplace(children->end(), field, mode);
                }
            }
            if (j + 1 == (int)fields.size()) {
                break;
            }
            if (!it->children) {
                it->children = std::make_shared<EnumItems>();
            }
            children = it->children;
        }
    }

    return enumItems;
}

}  // anonymous namespace

QStringList PropertyEnumItem::getCommonModes() const
{
    const std::vector<App::Property*>& items = getPropertyData();

    QStringList commonModes;
    QStringList modes;
    for (auto it = items.begin(); it != items.end(); ++it) {
        if ((*it)->is<App::PropertyEnumeration>()) {
            auto prop = static_cast<App::PropertyEnumeration*>(*it);
            if (!prop->hasEnums()) {
                commonModes.clear();
                return {};
            }
            const std::vector<std::string>& value = prop->getEnumVector();
            if (it == items.begin()) {
                for (const auto& jt : value) {
                    commonModes << QString::fromUtf8(jt.c_str());
                }
            }
            else {
                for (const auto& jt : value) {
                    if (commonModes.contains(QString::fromUtf8(jt.c_str()))) {
                        modes << QString::fromUtf8(jt.c_str());
                    }
                }

                commonModes = modes;
                modes.clear();
            }
        }
    }

    return commonModes;
}

QWidget* PropertyEnumItem::createEditor(
    QWidget* parent,
    const std::function<void()>& method,
    FrameOption frameOption
) const
{
    QStringList commonModes = getCommonModes();
    if (commonModes.isEmpty()) {
        return nullptr;
    }

    std::shared_ptr<EnumItems> enumItems = getEnumItems(commonModes);

    if (!enumItems) {
        auto cb = new QComboBox(parent);
        cb->setFrame(static_cast<bool>(frameOption));
        cb->addItems(commonModes);
        QObject::connect(cb, qOverload<int>(&QComboBox::activated), method);
        return cb;
    }

    auto button = new PropertyEnumButton(parent);
    button->setDisabled(isReadOnly());
    auto menu = new QMenu(button);
    for (auto& item : *enumItems) {
        item.populate(menu);
    }
    button->setMenu(menu);
    QObject::connect(menu, &QMenu::aboutToShow, this, [=]() {
        menu->setMinimumWidth(button->width());
    });
    QObject::connect(menu, &QMenu::triggered, this, [=](QAction* action) {
        button->setText(action->data().toString());
        Q_EMIT button->picked();
    });
    QObject::connect(button, &PropertyEnumButton::picked, method);
    return button;
}

void PropertyEnumItem::setEditorData(QWidget* editor, const QVariant& data) const
{
    if (auto cb = qobject_cast<QComboBox*>(editor)) {
        cb->setEditable(false);
        cb->setCurrentIndex(cb->findText(data.toString()));
    }
    else if (auto btn = qobject_cast<QPushButton*>(editor)) {
        btn->setText(data.toString());
    }
}

QVariant PropertyEnumItem::editorData(QWidget* editor) const
{
    if (auto cb = qobject_cast<QComboBox*>(editor)) {
        return {cb->currentText()};
    }
    if (auto btn = qobject_cast<QPushButton*>(editor)) {
        return btn->text();
    }
    return {};
}

// ---------------------------------------------------------------

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyStringListItem)

PropertyStringListItem::PropertyStringListItem() = default;

QWidget* PropertyStringListItem::
    createEditor(QWidget* parent, const std::function<void()>& method, FrameOption /*frameOption*/) const
{
    auto le = new Gui::LabelEditor(parent);
    le->setAutoFillBackground(true);
    QObject::connect(le, &Gui::LabelEditor::textChanged, method);
    return le;
}

void PropertyStringListItem::setEditorData(QWidget* editor, const QVariant& data) const
{
    auto le = qobject_cast<Gui::LabelEditor*>(editor);
    QStringList list = data.toStringList();
    le->setText(list.join(QChar::fromLatin1('\n')));
}

QVariant PropertyStringListItem::editorData(QWidget* editor) const
{
    auto le = qobject_cast<Gui::LabelEditor*>(editor);
    QString complete = le->text();
    QStringList list = complete.split(QChar::fromLatin1('\n'));
    return {list};
}

QString PropertyStringListItem::toString(const QVariant& prop) const
{
    QStringList list = prop.toStringList();
    const int size = 10;
    if (list.size() > size) {
        list = list.mid(0, size);
        list.append(QStringLiteral("..."));
    }

    return QStringLiteral("[%1]").arg(list.join(QLatin1Char(',')));
}

QVariant PropertyStringListItem::value(const App::Property* prop) const
{
    assert(prop && prop->isDerivedFrom<App::PropertyStringList>());
    QStringList list;
    const std::vector<std::string>& value
        = (static_cast<const App::PropertyStringList*>(prop))->getValues();
    for (const auto& jt : value) {
        list << QString::fromUtf8(jt.c_str());
    }

    return {list};
}

void PropertyStringListItem::setValue(const QVariant& value)
{
    if (hasExpression() || !value.canConvert<QStringList>()) {
        return;
    }

    QStringList values = value.toStringList();
    std::ostringstream ss;
    ss << "[";
    for (const auto& it : values) {
        QString text(it);
        std::string pystr = Base::InterpreterSingleton::strToPython(text.toUtf8().constData());
        ss << "\"" << pystr << "\", ";
    }
    ss << "]";
    setPropertyValue(ss.str());
}

// ---------------------------------------------------------------

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyFloatListItem)

PropertyFloatListItem::PropertyFloatListItem() = default;

QWidget* PropertyFloatListItem::
    createEditor(QWidget* parent, const std::function<void()>& method, FrameOption /*frameOption*/) const
{
    auto le = new Gui::LabelEditor(parent);
    le->setAutoFillBackground(true);
    le->setInputType(Gui::LabelEditor::Float);
    QObject::connect(le, &Gui::LabelEditor::textChanged, method);
    return le;
}

void PropertyFloatListItem::setEditorData(QWidget* editor, const QVariant& data) const
{
    auto le = qobject_cast<Gui::LabelEditor*>(editor);
    QStringList list = data.toStringList();
    le->setText(list.join(QChar::fromLatin1('\n')));
}

QVariant PropertyFloatListItem::editorData(QWidget* editor) const
{
    auto le = qobject_cast<Gui::LabelEditor*>(editor);
    QString complete = le->text();
    QStringList list = complete.split(QChar::fromLatin1('\n'));
    return {list};
}

QString PropertyFloatListItem::toString(const QVariant& prop) const
{
    QStringList list = prop.toStringList();
    const int size = 10;
    if (list.size() > size) {
        list = list.mid(0, size);
        list.append(QStringLiteral("..."));
    }
    return QStringLiteral("[%1]").arg(list.join(QLatin1Char(',')));
}

QVariant PropertyFloatListItem::value(const App::Property* prop) const
{
    assert(prop && prop->isDerivedFrom<App::PropertyFloatList>());

    QStringList list;
    const std::vector<double>& value = static_cast<const App::PropertyFloatList*>(prop)->getValues();
    for (double jt : value) {
        list << QString::number(jt, 'f', decimals());
    }

    return {list};
}

void PropertyFloatListItem::setValue(const QVariant& value)
{
    if (hasExpression() || !value.canConvert<QStringList>()) {
        return;
    }

    QStringList values = value.toStringList();
    QString data;
    QTextStream str(&data);
    str << "[";
    for (auto it = values.begin(); it != values.end(); ++it) {
        if (it != values.begin()) {
            str << ",";
        }
        str << *it;
    }
    str << "]";
    setPropertyValue(data);
}

// ---------------------------------------------------------------

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyIntegerListItem)

PropertyIntegerListItem::PropertyIntegerListItem() = default;

QWidget* PropertyIntegerListItem::
    createEditor(QWidget* parent, const std::function<void()>& method, FrameOption /*frameOption*/) const
{
    auto le = new Gui::LabelEditor(parent);
    le->setAutoFillBackground(true);
    le->setInputType(Gui::LabelEditor::Integer);
    QObject::connect(le, &Gui::LabelEditor::textChanged, method);
    return le;
}

void PropertyIntegerListItem::setEditorData(QWidget* editor, const QVariant& data) const
{
    auto le = qobject_cast<Gui::LabelEditor*>(editor);
    QStringList list = data.toStringList();
    le->setText(list.join(QChar::fromLatin1('\n')));
}

QVariant PropertyIntegerListItem::editorData(QWidget* editor) const
{
    auto le = qobject_cast<Gui::LabelEditor*>(editor);
    QString complete = le->text();
    QStringList list = complete.split(QChar::fromLatin1('\n'));
    return {list};
}

QString PropertyIntegerListItem::toString(const QVariant& prop) const
{
    QStringList list = prop.toStringList();
    const int size = 10;
    if (list.size() > size) {
        list = list.mid(0, size);
        list.append(QStringLiteral("..."));
    }
    QString text = QStringLiteral("[%1]").arg(list.join(QLatin1Char(',')));

    return {text};
}

QVariant PropertyIntegerListItem::value(const App::Property* prop) const
{
    assert(prop && prop->isDerivedFrom<App::PropertyIntegerList>());

    QStringList list;
    const std::vector<long>& value = static_cast<const App::PropertyIntegerList*>(prop)->getValues();
    for (long jt : value) {
        list << QString::number(jt);
    }

    return {list};
}

void PropertyIntegerListItem::setValue(const QVariant& value)
{
    if (hasExpression() || !value.canConvert<QStringList>()) {
        return;
    }

    QStringList values = value.toStringList();
    QString data;
    QTextStream str(&data);
    str << "[";
    for (auto it = values.begin(); it != values.end(); ++it) {
        if (it != values.begin()) {
            str << ",";
        }
        str << *it;
    }
    str << "]";
    setPropertyValue(data);
}

// --------------------------------------------------------------------

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyColorItem)

PropertyColorItem::PropertyColorItem() = default;

QVariant PropertyColorItem::decoration(const QVariant& value) const
{
    auto color = value.value<QColor>();

    int size = QApplication::style()->pixelMetric(QStyle::PM_ListViewIconSize);
    QPixmap p(size, size);
    p.fill(color);

    return QVariant(p);
}

QString PropertyColorItem::toString(const QVariant& prop) const
{
    auto value = prop.value<QColor>();
    return QStringLiteral("[%1, %2, %3]").arg(value.red()).arg(value.green()).arg(value.blue());
}

QVariant PropertyColorItem::value(const App::Property* prop) const
{
    assert(prop && prop->isDerivedFrom<App::PropertyColor>());

    Base::Color value = static_cast<const App::PropertyColor*>(prop)->getValue();
    return QVariant(value.asValue<QColor>());
}

void PropertyColorItem::setValue(const QVariant& value)
{
    if (hasExpression() || !value.canConvert<QColor>()) {
        return;
    }
    auto col = value.value<QColor>();
    QString data = QStringLiteral("(%1,%2,%3)").arg(col.red()).arg(col.green()).arg(col.blue());
    setPropertyValue(data);
}

QWidget* PropertyColorItem::
    createEditor(QWidget* parent, const std::function<void()>& method, FrameOption /*frameOption*/) const
{
    auto cb = new Gui::ColorButton(parent);
    QObject::connect(cb, &Gui::ColorButton::changed, method);
    return cb;
}

void PropertyColorItem::setEditorData(QWidget* editor, const QVariant& data) const
{
    auto cb = qobject_cast<Gui::ColorButton*>(editor);
    auto color = data.value<QColor>();
    cb->setColor(color);
}

QVariant PropertyColorItem::editorData(QWidget* editor) const
{
    auto cb = qobject_cast<Gui::ColorButton*>(editor);
    QVariant var;
    var.setValue(cb->color());
    return var;
}

// --------------------------------------------------------------------

namespace Gui::PropertyEditor
{
class Material
{
public:
    QColor diffuseColor;
    QColor ambientColor;
    QColor specularColor;
    QColor emissiveColor;
    float shininess {};
    float transparency {};
};
}  // namespace Gui::PropertyEditor

Q_DECLARE_METATYPE(Gui::PropertyEditor::Material)  // NOLINT

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyMaterialItem)

PropertyMaterialItem::PropertyMaterialItem()
{
    const int min = 0;
    const int max = 100;
    const int steps = 5;
    diffuse = static_cast<PropertyColorItem*>(PropertyColorItem::create());
    diffuse->setParent(this);
    diffuse->setPropertyName(QLatin1String("DiffuseColor"));
    diffuse->setNameToolTipOverride(
        tr("Defines the base color of a surface when illuminated by light. It represents how the "
           "object scatters light evenly in all directions, independent of the viewer’s angle. "
           "This property will influence the material color the most.")
    );
    this->appendChild(diffuse);

    ambient = static_cast<PropertyColorItem*>(PropertyColorItem::create());
    ambient->setParent(this);
    ambient->setPropertyName(QLatin1String("AmbientColor"));
    ambient->setNameToolTipOverride(
        tr("Defines the color of a surface under indirect, uniform lighting, representing how it "
           "appears when illuminated only by ambient light in a scene, without directional light, "
           "shading, or highlights")
    );
    this->appendChild(ambient);

    specular = static_cast<PropertyColorItem*>(PropertyColorItem::create());
    specular->setParent(this);
    specular->setPropertyName(QLatin1String("SpecularColor"));
    specular->setNameToolTipOverride(
        tr("Defines the color and intensity of the bright, mirror-like highlights that appear on "
           "shiny or reflective surfaces when light hits them directly. Set to bright colors for "
           "shiny objects.")
    );
    this->appendChild(specular);

    emissive = static_cast<PropertyColorItem*>(PropertyColorItem::create());
    emissive->setParent(this);
    emissive->setPropertyName(QLatin1String("EmissiveColor"));
    emissive->setNameToolTipOverride(
        tr("Defines the color of a surface that appears to emit as if it were a light source, "
           "independent of external lighting, making the object look self-illuminated. Set to "
           "black to have no emissive color.")
    );
    this->appendChild(emissive);

    shininess = static_cast<PropertyIntegerConstraintItem*>(PropertyIntegerConstraintItem::create());
    shininess->setRange(min, max);
    shininess->setStepSize(steps);
    shininess->setParent(this);
    shininess->setPropertyName(QLatin1String("Shininess"));
    shininess->setNameToolTipOverride(
        tr("Defines the size and sharpness of specular highlights on a surface. Higher values "
           "produce small, sharp highlights, while lower values create broad, soft highlights. "
           "Note that the highlight intensity is defined by specular color.")
    );
    this->appendChild(shininess);

    transparency = static_cast<PropertyIntegerConstraintItem*>(PropertyIntegerConstraintItem::create());
    transparency->setRange(min, max);
    transparency->setStepSize(steps);
    transparency->setParent(this);
    transparency->setPropertyName(QLatin1String("Transparency"));
    transparency->setNameToolTipOverride(
        tr("Defines how much light passes through an object, making it "
           "partially or fully see-through")
    );
    this->appendChild(transparency);
}

PropertyMaterialItem::~PropertyMaterialItem() = default;

void PropertyMaterialItem::propertyBound()
{}

QColor PropertyMaterialItem::getDiffuseColor() const
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<Material>()) {
        return {};
    }

    auto val = value.value<Material>();
    return val.diffuseColor;
}

void PropertyMaterialItem::setDiffuseColor(const QColor& color)
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<Material>()) {
        return;
    }

    auto mat = value.value<Material>();
    mat.diffuseColor = color;
    setValue(QVariant::fromValue<Material>(mat));
}

QColor PropertyMaterialItem::getAmbientColor() const
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<Material>()) {
        return {};
    }

    auto val = value.value<Material>();
    return val.ambientColor;
}

void PropertyMaterialItem::setAmbientColor(const QColor& color)
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<Material>()) {
        return;
    }

    auto mat = value.value<Material>();
    mat.ambientColor = color;
    setValue(QVariant::fromValue<Material>(mat));
}

QColor PropertyMaterialItem::getSpecularColor() const
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<Material>()) {
        return {};
    }

    auto val = value.value<Material>();
    return val.specularColor;
}

void PropertyMaterialItem::setSpecularColor(const QColor& color)
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<Material>()) {
        return;
    }

    auto mat = value.value<Material>();
    mat.specularColor = color;
    setValue(QVariant::fromValue<Material>(mat));
}

QColor PropertyMaterialItem::getEmissiveColor() const
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<Material>()) {
        return {};
    }

    auto val = value.value<Material>();
    return val.emissiveColor;
}

void PropertyMaterialItem::setEmissiveColor(const QColor& color)
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<Material>()) {
        return;
    }

    auto mat = value.value<Material>();
    mat.emissiveColor = color;
    setValue(QVariant::fromValue<Material>(mat));
}

int PropertyMaterialItem::getShininess() const
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<Material>()) {
        return 0;
    }

    auto val = value.value<Material>();
    return Base::toPercent(val.shininess);
}

void PropertyMaterialItem::setShininess(int s)
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<Material>()) {
        return;
    }

    auto mat = value.value<Material>();
    mat.shininess = Base::fromPercent(s);
    setValue(QVariant::fromValue<Material>(mat));
}

int PropertyMaterialItem::getTransparency() const
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<Material>()) {
        return 0;
    }

    auto val = value.value<Material>();
    return Base::toPercent(val.transparency);
}

void PropertyMaterialItem::setTransparency(int t)
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<Material>()) {
        return;
    }

    auto mat = value.value<Material>();
    mat.transparency = Base::fromPercent(t);
    setValue(QVariant::fromValue<Material>(mat));
}

QVariant PropertyMaterialItem::decoration(const QVariant& value) const
{
    // use the diffuse color
    auto val = value.value<Material>();
    QColor color = val.diffuseColor;

    int size = QApplication::style()->pixelMetric(QStyle::PM_ListViewIconSize);
    QPixmap p(size, size);
    p.fill(color);

    return QVariant(p);
}

QString PropertyMaterialItem::toString(const QVariant& prop) const
{
    // use the diffuse color
    auto val = prop.value<Material>();
    QColor value = val.diffuseColor;
    return QStringLiteral("[%1, %2, %3]").arg(value.red()).arg(value.green()).arg(value.blue());
}

QVariant PropertyMaterialItem::toolTip(const App::Property* prop) const
{
    assert(prop && prop->isDerivedFrom<App::PropertyMaterial>());

    const App::Material& value = static_cast<const App::PropertyMaterial*>(prop)->getValue();
    auto dc = value.diffuseColor.asValue<QColor>();
    auto ac = value.ambientColor.asValue<QColor>();
    auto sc = value.specularColor.asValue<QColor>();
    auto ec = value.emissiveColor.asValue<QColor>();

    QString data = QStringLiteral(
                       "Diffuse color: [%1, %2, %3]\n"
                       "Ambient color: [%4, %5, %6]\n"
                       "Specular color: [%7, %8, %9]\n"
                       "Emissive color: [%10, %11, %12]\n"
                       "Shininess: %13\n"
                       "Transparency: %14"
    )
                       .arg(dc.red())
                       .arg(dc.green())
                       .arg(dc.blue())
                       .arg(ac.red())
                       .arg(ac.green())
                       .arg(ac.blue())
                       .arg(sc.red())
                       .arg(sc.green())
                       .arg(sc.blue())
                       .arg(ec.red())
                       .arg(ec.green())
                       .arg(ec.blue())
                       .arg(Base::toPercent(value.shininess))
                       .arg(Base::toPercent(value.transparency));

    return {data};
}

QVariant PropertyMaterialItem::value(const App::Property* prop) const
{
    assert(prop && prop->isDerivedFrom<App::PropertyMaterial>());

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
    if (hasExpression() || !value.canConvert<Material>()) {
        return;
    }

    auto mat = value.value<Material>();
    Base::Color dc;
    dc.setValue<QColor>(mat.diffuseColor);
    uint32_t dcp = dc.getPackedValue();
    Base::Color ac;
    ac.setValue<QColor>(mat.ambientColor);
    uint32_t acp = ac.getPackedValue();
    Base::Color sc;
    sc.setValue<QColor>(mat.specularColor);
    uint32_t scp = sc.getPackedValue();
    Base::Color ec;
    ec.setValue<QColor>(mat.emissiveColor);
    uint32_t ecp = ec.getPackedValue();
    float s = mat.shininess;
    float t = mat.transparency;

    QString data = QStringLiteral(
                       "App.Material("
                       "DiffuseColor = %1,"
                       "AmbientColor = %2,"
                       "SpecularColor = %3,"
                       "EmissiveColor = %4,"
                       "Shininess = %5,"
                       "Transparency = %6,"
                       ")"
    )
                       .arg(dcp)
                       .arg(acp)
                       .arg(scp)
                       .arg(ecp)
                       .arg(s, 0, 'f', 10)
                       .arg(t, 0, 'f', 10);

    setPropertyValue(data);
}

QWidget* PropertyMaterialItem::
    createEditor(QWidget* parent, const std::function<void()>& method, FrameOption /*frameOption*/) const
{
    auto cb = new Gui::ColorButton(parent);
    QObject::connect(cb, &Gui::ColorButton::changed, method);
    return cb;
}

void PropertyMaterialItem::setEditorData(QWidget* editor, const QVariant& data) const
{
    if (!data.canConvert<Material>()) {
        return;
    }

    auto val = data.value<Material>();
    auto cb = qobject_cast<Gui::ColorButton*>(editor);
    cb->setColor(val.diffuseColor);
}

QVariant PropertyMaterialItem::editorData(QWidget* editor) const
{
    auto cb = qobject_cast<Gui::ColorButton*>(editor);
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<Material>()) {
        return {};
    }

    auto val = value.value<Material>();
    val.diffuseColor = cb->color();
    return QVariant::fromValue<Material>(val);
}

// --------------------------------------------------------------------

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyMaterialListItem)

PropertyMaterialListItem::PropertyMaterialListItem()
{
    const int min = 0;
    const int max = 100;
    const int steps = 5;

    // This editor gets a list of materials but it only edits the first item.
    diffuse = static_cast<PropertyColorItem*>(PropertyColorItem::create());
    diffuse->setParent(this);
    diffuse->setPropertyName(QLatin1String("DiffuseColor"));
    diffuse->setNameToolTipOverride(
        tr("Defines the base color of a surface when illuminated by light. It represents how the "
           "object scatters light evenly in all directions, independent of the viewer’s angle. "
           "This property will influence the material color the most.")
    );
    this->appendChild(diffuse);

    ambient = static_cast<PropertyColorItem*>(PropertyColorItem::create());
    ambient->setParent(this);
    ambient->setPropertyName(QLatin1String("AmbientColor"));
    ambient->setNameToolTipOverride(
        tr("Defines the color of a surface under indirect, uniform lighting, representing how it "
           "appears when illuminated only by ambient light in a scene, without directional light, "
           "shading, or highlights")
    );
    this->appendChild(ambient);

    specular = static_cast<PropertyColorItem*>(PropertyColorItem::create());
    specular->setParent(this);
    specular->setPropertyName(QLatin1String("SpecularColor"));
    specular->setNameToolTipOverride(
        tr("Defines the color and intensity of the bright, mirror-like highlights that appear on "
           "shiny or reflective surfaces when light hits them directly. Set to bright colors for "
           "shiny objects.")
    );
    this->appendChild(specular);

    emissive = static_cast<PropertyColorItem*>(PropertyColorItem::create());
    emissive->setParent(this);
    emissive->setPropertyName(QLatin1String("EmissiveColor"));
    emissive->setNameToolTipOverride(
        tr("Defines the color of a surface that appears to emit as if it were a light source, "
           "independent of external lighting, making the object look self-illuminated. Set to "
           "black to have no emissive color.")
    );
    this->appendChild(emissive);

    shininess = static_cast<PropertyIntegerConstraintItem*>(PropertyIntegerConstraintItem::create());
    shininess->setRange(min, max);
    shininess->setStepSize(steps);
    shininess->setParent(this);
    shininess->setPropertyName(QLatin1String("Shininess"));
    shininess->setNameToolTipOverride(
        tr("Defines the size and sharpness of specular highlights on a surface. Higher values "
           "produce small, sharp highlights, while lower values create broad, soft highlights. "
           "Note that the highlight intensity is defined by specular color.")
    );
    this->appendChild(shininess);

    transparency = static_cast<PropertyIntegerConstraintItem*>(PropertyIntegerConstraintItem::create());
    transparency->setRange(min, max);
    transparency->setStepSize(steps);
    transparency->setParent(this);
    transparency->setPropertyName(QLatin1String("Transparency"));
    transparency->setNameToolTipOverride(
        tr("Defines how much light passes through an object, making it "
           "partially or fully see-through")
    );
    this->appendChild(transparency);
}

PropertyMaterialListItem::~PropertyMaterialListItem() = default;

void PropertyMaterialListItem::propertyBound()
{}

QColor PropertyMaterialListItem::getDiffuseColor() const
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<QVariantList>()) {
        return {};
    }

    QVariantList list = value.toList();
    if (list.isEmpty()) {
        return {};
    }

    if (!list[0].canConvert<Material>()) {
        return {};
    }

    auto mat = list[0].value<Material>();
    return mat.diffuseColor;
}

void PropertyMaterialListItem::setDiffuseColor(const QColor& color)
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<QVariantList>()) {
        return;
    }

    QVariantList list = value.toList();
    if (list.isEmpty()) {
        return;
    }

    if (!list[0].canConvert<Material>()) {
        return;
    }

    auto mat = list[0].value<Material>();
    mat.diffuseColor = color;
    list[0] = QVariant::fromValue<Material>(mat);
    setValue(list);
}

QColor PropertyMaterialListItem::getAmbientColor() const
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<QVariantList>()) {
        return {};
    }

    QVariantList list = value.toList();
    if (list.isEmpty()) {
        return {};
    }

    if (!list[0].canConvert<Material>()) {
        return {};
    }

    auto mat = list[0].value<Material>();
    return mat.ambientColor;
}

void PropertyMaterialListItem::setAmbientColor(const QColor& color)
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<QVariantList>()) {
        return;
    }

    QVariantList list = value.toList();
    if (list.isEmpty()) {
        return;
    }

    if (!list[0].canConvert<Material>()) {
        return;
    }

    auto mat = list[0].value<Material>();
    mat.ambientColor = color;
    list[0] = QVariant::fromValue<Material>(mat);
    setValue(list);
}

QColor PropertyMaterialListItem::getSpecularColor() const
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<QVariantList>()) {
        return {};
    }

    QVariantList list = value.toList();
    if (list.isEmpty()) {
        return {};
    }

    if (!list[0].canConvert<Material>()) {
        return {};
    }

    auto mat = list[0].value<Material>();
    return mat.specularColor;
}

void PropertyMaterialListItem::setSpecularColor(const QColor& color)
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<QVariantList>()) {
        return;
    }

    QVariantList list = value.toList();
    if (list.isEmpty()) {
        return;
    }

    if (!list[0].canConvert<Material>()) {
        return;
    }

    auto mat = list[0].value<Material>();
    mat.specularColor = color;
    list[0] = QVariant::fromValue<Material>(mat);
    setValue(list);
}

QColor PropertyMaterialListItem::getEmissiveColor() const
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<QVariantList>()) {
        return {};
    }

    QVariantList list = value.toList();
    if (list.isEmpty()) {
        return {};
    }

    if (!list[0].canConvert<Material>()) {
        return {};
    }

    auto mat = list[0].value<Material>();
    return mat.emissiveColor;
}

void PropertyMaterialListItem::setEmissiveColor(const QColor& color)
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<QVariantList>()) {
        return;
    }

    QVariantList list = value.toList();
    if (list.isEmpty()) {
        return;
    }

    if (!list[0].canConvert<Material>()) {
        return;
    }

    auto mat = list[0].value<Material>();
    mat.emissiveColor = color;
    list[0] = QVariant::fromValue<Material>(mat);
    setValue(list);
}

int PropertyMaterialListItem::getShininess() const
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<QVariantList>()) {
        return 0;
    }

    QVariantList list = value.toList();
    if (list.isEmpty()) {
        return 0;
    }

    if (!list[0].canConvert<Material>()) {
        return 0;
    }

    auto mat = list[0].value<Material>();
    return Base::toPercent(mat.shininess);
}

void PropertyMaterialListItem::setShininess(int s)
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<QVariantList>()) {
        return;
    }

    QVariantList list = value.toList();
    if (list.isEmpty()) {
        return;
    }

    if (!list[0].canConvert<Material>()) {
        return;
    }

    auto mat = list[0].value<Material>();
    mat.shininess = Base::fromPercent(s);
    list[0] = QVariant::fromValue<Material>(mat);
    setValue(list);
}

int PropertyMaterialListItem::getTransparency() const
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<QVariantList>()) {
        return 0;
    }

    QVariantList list = value.toList();
    if (list.isEmpty()) {
        return 0;
    }

    if (!list[0].canConvert<Material>()) {
        return 0;
    }

    auto mat = list[0].value<Material>();
    return Base::toPercent(mat.transparency);
}

void PropertyMaterialListItem::setTransparency(int t)
{
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<QVariantList>()) {
        return;
    }

    QVariantList list = value.toList();
    if (list.isEmpty()) {
        return;
    }

    if (!list[0].canConvert<Material>()) {
        return;
    }

    auto mat = list[0].value<Material>();
    mat.transparency = Base::fromPercent(t);
    list[0] = QVariant::fromValue<Material>(mat);
    setValue(list);
}

QVariant PropertyMaterialListItem::decoration(const QVariant& value) const
{
    if (!value.canConvert<QVariantList>()) {
        return {};
    }

    QVariantList list = value.toList();
    if (list.isEmpty()) {
        return {};
    }

    if (!list[0].canConvert<Material>()) {
        return {};
    }

    // use the diffuse color
    auto mat = list[0].value<Material>();
    QColor color = mat.diffuseColor;

    int size = QApplication::style()->pixelMetric(QStyle::PM_ListViewIconSize);
    QPixmap p(size, size);
    p.fill(color);

    return QVariant(p);
}

QString PropertyMaterialListItem::toString(const QVariant& prop) const
{
    if (!prop.canConvert<QVariantList>()) {
        return {};
    }

    QVariantList list = prop.toList();
    if (list.isEmpty()) {
        return {};
    }

    if (!list[0].canConvert<Material>()) {
        return {};
    }

    // use the diffuse color
    auto mat = list[0].value<Material>();
    QColor value = mat.diffuseColor;
    return QStringLiteral("[%1, %2, %3]").arg(value.red()).arg(value.green()).arg(value.blue());
}

QVariant PropertyMaterialListItem::toolTip(const App::Property* prop) const
{
    assert(prop && prop->isDerivedFrom<App::PropertyMaterialList>());

    const std::vector<App::Material>& values
        = static_cast<const App::PropertyMaterialList*>(prop)->getValues();
    if (values.empty()) {
        return {};
    }

    App::Material value = values.front();
    auto dc = value.diffuseColor.asValue<QColor>();
    auto ac = value.ambientColor.asValue<QColor>();
    auto sc = value.specularColor.asValue<QColor>();
    auto ec = value.emissiveColor.asValue<QColor>();

    QString data = QStringLiteral(
                       "Diffuse color: [%1, %2, %3]\n"
                       "Ambient color: [%4, %5, %6]\n"
                       "Specular color: [%7, %8, %9]\n"
                       "Emissive color: [%10, %11, %12]\n"
                       "Shininess: %13\n"
                       "Transparency: %14"
    )
                       .arg(dc.red())
                       .arg(dc.green())
                       .arg(dc.blue())
                       .arg(ac.red())
                       .arg(ac.green())
                       .arg(ac.blue())
                       .arg(sc.red())
                       .arg(sc.green())
                       .arg(sc.blue())
                       .arg(ec.red())
                       .arg(ec.green())
                       .arg(ec.blue())
                       .arg(Base::toPercent(value.shininess))
                       .arg(Base::toPercent(value.transparency));

    return {data};
}

QVariant PropertyMaterialListItem::value(const App::Property* prop) const
{
    assert(prop && prop->isDerivedFrom<App::PropertyMaterialList>());

    const std::vector<App::Material>& value
        = static_cast<const App::PropertyMaterialList*>(prop)->getValues();
    QVariantList variantList;

    for (const auto& it : value) {
        Material mat;
        mat.diffuseColor = it.diffuseColor.asValue<QColor>();
        mat.ambientColor = it.ambientColor.asValue<QColor>();
        mat.specularColor = it.specularColor.asValue<QColor>();
        mat.emissiveColor = it.emissiveColor.asValue<QColor>();
        mat.shininess = it.shininess;
        mat.transparency = it.transparency;

        variantList << QVariant::fromValue<Material>(mat);
    }

    return variantList;
}

void PropertyMaterialListItem::setValue(const QVariant& value)
{
    if (hasExpression() || !value.canConvert<QVariantList>()) {
        return;
    }

    QVariantList list = value.toList();
    if (list.isEmpty()) {
        return;
    }

    // Setting an appearance using the property editor resets the
    // per-face appearance
    list = list.mid(0, 1);

    QString data;
    QTextStream str(&data);
    str << "(";

    auto mat = list[0].value<Material>();
    Base::Color dc;
    dc.setValue<QColor>(mat.diffuseColor);
    uint32_t dcp = dc.getPackedValue();
    Base::Color ac;
    ac.setValue<QColor>(mat.ambientColor);
    uint32_t acp = ac.getPackedValue();
    Base::Color sc;
    sc.setValue<QColor>(mat.specularColor);
    uint32_t scp = sc.getPackedValue();
    Base::Color ec;
    ec.setValue<QColor>(mat.emissiveColor);
    uint32_t ecp = ec.getPackedValue();
    float s = mat.shininess;
    float t = mat.transparency;

    QString item = QStringLiteral(
                       "App.Material("
                       "DiffuseColor = %1,"
                       "AmbientColor = %2,"
                       "SpecularColor = %3,"
                       "EmissiveColor = %4,"
                       "Shininess = %5,"
                       "Transparency = %6,"
                       ")"
    )
                       .arg(dcp)
                       .arg(acp)
                       .arg(scp)
                       .arg(ecp)
                       .arg(s, 0, 'f', 10)
                       .arg(t, 0, 'f', 10);
    str << item << ")";

    setPropertyValue(data);
}

QWidget* PropertyMaterialListItem::
    createEditor(QWidget* parent, const std::function<void()>& method, FrameOption /*frameOption*/) const
{
    auto cb = new Gui::ColorButton(parent);
    QObject::connect(cb, &Gui::ColorButton::changed, method);
    return cb;
}

void PropertyMaterialListItem::setEditorData(QWidget* editor, const QVariant& data) const
{
    if (!data.canConvert<QVariantList>()) {
        return;
    }

    QVariantList list = data.toList();
    if (list.isEmpty()) {
        return;
    }

    if (!list[0].canConvert<Material>()) {
        return;
    }

    // use the diffuse color
    auto mat = list[0].value<Material>();
    QColor color = mat.diffuseColor;

    auto cb = qobject_cast<Gui::ColorButton*>(editor);
    cb->setColor(color);
}

QVariant PropertyMaterialListItem::editorData(QWidget* editor) const
{
    auto cb = qobject_cast<Gui::ColorButton*>(editor);
    QVariant value = data(1, Qt::EditRole);
    if (!value.canConvert<QVariantList>()) {
        return {};
    }

    QVariantList list = value.toList();
    if (list.isEmpty()) {
        return {};
    }

    if (!list[0].canConvert<Material>()) {
        return {};
    }

    // use the diffuse color
    auto mat = list[0].value<Material>();
    mat.diffuseColor = cb->color();
    list[0] = QVariant::fromValue<Material>(mat);

    return list;
}

// --------------------------------------------------------------------

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyFileItem)

PropertyFileItem::PropertyFileItem() = default;

QVariant PropertyFileItem::value(const App::Property* prop) const
{
    assert(prop && prop->isDerivedFrom<App::PropertyFile>());

    std::string value = static_cast<const App::PropertyFile*>(prop)->getValue();
    return {QString::fromUtf8(value.c_str())};
}

void PropertyFileItem::setValue(const QVariant& value)
{
    if (!hasExpression() && value.canConvert<QString>()) {
        setPropertyValue(Base::Tools::quoted(value.toString().toStdString()));
    }
}

QVariant PropertyFileItem::toolTip(const App::Property* prop) const
{
    return value(prop);
}

QWidget* PropertyFileItem::
    createEditor(QWidget* parent, const std::function<void()>& method, FrameOption /*frameOption*/) const
{
    auto fc = new Gui::FileChooser(parent);
    fc->setAutoFillBackground(true);
    QObject::connect(fc, &Gui::FileChooser::fileNameSelected, method);
    return fc;
}

void PropertyFileItem::setEditorData(QWidget* editor, const QVariant& data) const
{
    const App::Property* prop = getFirstProperty();
    if (const auto propFile = dynamic_cast<const App::PropertyFile*>(prop)) {
        std::string filter = propFile->getFilter();
        auto fc = qobject_cast<Gui::FileChooser*>(editor);
        if (!filter.empty()) {
            fc->setFilter(QString::fromStdString(filter));
        }
        fc->setFileName(data.toString());
    }
}

QVariant PropertyFileItem::editorData(QWidget* editor) const
{
    auto fc = qobject_cast<Gui::FileChooser*>(editor);
    return {fc->fileName()};
}

// --------------------------------------------------------------------

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyPathItem)

PropertyPathItem::PropertyPathItem() = default;

QVariant PropertyPathItem::value(const App::Property* prop) const
{
    assert(prop && prop->isDerivedFrom<App::PropertyPath>());

    std::string value = static_cast<const App::PropertyPath*>(prop)->getValue().string();
    return {QString::fromUtf8(value.c_str())};
}

void PropertyPathItem::setValue(const QVariant& value)
{
    if (!hasExpression() && value.canConvert<QString>()) {
        setPropertyValue(Base::Tools::quoted(value.toString().toStdString()));
    }
}

QVariant PropertyPathItem::toolTip(const App::Property* prop) const
{
    return value(prop);
}

QWidget* PropertyPathItem::
    createEditor(QWidget* parent, const std::function<void()>& method, FrameOption /*frameOption*/) const
{
    auto fc = new Gui::FileChooser(parent);
    fc->setMode(FileChooser::Directory);
    fc->setAutoFillBackground(true);
    QObject::connect(fc, &Gui::FileChooser::fileNameSelected, method);
    return fc;
}

void PropertyPathItem::setEditorData(QWidget* editor, const QVariant& data) const
{
    auto fc = qobject_cast<Gui::FileChooser*>(editor);
    fc->setFileName(data.toString());
}

QVariant PropertyPathItem::editorData(QWidget* editor) const
{
    auto fc = qobject_cast<Gui::FileChooser*>(editor);
    return {fc->fileName()};
}

// --------------------------------------------------------------------

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyTransientFileItem)

PropertyTransientFileItem::PropertyTransientFileItem() = default;

QVariant PropertyTransientFileItem::value(const App::Property* prop) const
{
    assert(prop && prop->isDerivedFrom<App::PropertyFileIncluded>());

    std::string value = static_cast<const App::PropertyFileIncluded*>(prop)->getValue();
    return {QString::fromUtf8(value.c_str())};
}

void PropertyTransientFileItem::setValue(const QVariant& value)
{
    if (!hasExpression() && value.canConvert<QString>()) {
        setPropertyValue(Base::Tools::quoted(value.toString().toStdString()));
    }
}

QVariant PropertyTransientFileItem::toolTip(const App::Property* prop) const
{
    return value(prop);
}

QWidget* PropertyTransientFileItem::
    createEditor(QWidget* parent, const std::function<void()>& method, FrameOption /*frameOption*/) const
{
    auto fc = new Gui::FileChooser(parent);
    fc->setAutoFillBackground(true);
    QObject::connect(fc, &Gui::FileChooser::fileNameSelected, method);
    return fc;
}

void PropertyTransientFileItem::setEditorData(QWidget* editor, const QVariant& data) const
{
    auto fc = qobject_cast<Gui::FileChooser*>(editor);
    fc->setFileName(data.toString());

    const auto prop = dynamic_cast<const App::PropertyFileIncluded*>(getFirstProperty());

    if (prop) {
        std::string filter = prop->getFilter();
        if (!filter.empty()) {
            fc->setFilter(QString::fromStdString(filter));
        }
    }
}

QVariant PropertyTransientFileItem::editorData(QWidget* editor) const
{
    auto fc = qobject_cast<Gui::FileChooser*>(editor);
    return {fc->fileName()};
}

// ---------------------------------------------------------------

LinkSelection::LinkSelection(App::SubObjectT link)
    : link(std::move(link))
{}

LinkSelection::~LinkSelection() = default;

void LinkSelection::select()
{
    auto sobj = link.getSubObject();
    if (!sobj) {
        QMessageBox::critical(getMainWindow(), tr("Error"), tr("Object not found"));
        return;
    }
    Gui::Selection().selStackPush();
    Gui::Selection().clearSelection();
    Gui::Selection().addSelection(
        link.getDocumentName().c_str(),
        link.getObjectName().c_str(),
        link.getSubName().c_str()
    );
    this->deleteLater();
}

// ---------------------------------------------------------------

LinkLabel::LinkLabel(QWidget* parent, const App::Property* prop)
    : QWidget(parent)
    , objProp(prop)
    , dlg(nullptr)
{
    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(1);

    label = new QLabel(this);
    label->setAutoFillBackground(true);
    label->setTextFormat(Qt::RichText);
    // Below is necessary for the hytperlink to be clickable without losing focus
    label->setTextInteractionFlags(Qt::TextBrowserInteraction);
    layout->addWidget(label);

    editButton = new QPushButton(QStringLiteral("…"), this);
#if defined(Q_OS_MACOS)
    editButton->setAttribute(Qt::WA_LayoutUsesWidgetRect);  // layout size from QMacStyle was not correct
#endif
    editButton->setToolTip(tr("Changes the linked object"));
    layout->addWidget(editButton);

    this->setFocusPolicy(Qt::StrongFocus);
    this->setFocusProxy(label);

    // setLayout(layout);

    connect(label, &QLabel::linkActivated, this, &LinkLabel::onLinkActivated);
    connect(editButton, &QPushButton::clicked, this, &LinkLabel::onEditClicked);
}

LinkLabel::~LinkLabel() = default;

void LinkLabel::updatePropertyLink()
{
    QString text;
    auto owner = objProp.getObject();
    auto prop = freecad_cast<App::PropertyLinkBase*>(objProp.getProperty());

    link = QVariant();

    if (owner && prop) {
        auto links = DlgPropertyLink::getLinksFromProperty(prop);
        if (links.size() == 1) {
            auto& sobj = links.front();
            link = QVariant::fromValue(sobj);
            QString linkcolor = QApplication::palette().color(QPalette::Link).name();
            text = QStringLiteral(
                       "<html><head><style type=\"text/css\">"
                       "p, li { white-space: pre-wrap; }"
                       "</style></head><body>"
                       "<p>"
                       "<a href=\"%1#%2.%3\"><span style=\" text-decoration: "
                       "underline; color:%4;\">%5</span></a>"
                       "</p></body></html>"
            )
                       .arg(
                           QLatin1String(sobj.getDocumentName().c_str()),
                           QLatin1String(sobj.getObjectName().c_str()),
                           QString::fromUtf8(sobj.getSubName().c_str()),
                           linkcolor,
                           DlgPropertyLink::formatObject(
                               owner->getDocument(),
                               sobj.getObject(),
                               sobj.getSubName().c_str()
                           )
                       );
        }
        else if (!links.empty()) {
            text = DlgPropertyLink::formatLinks(owner->getDocument(), links);
        }
    }
    label->setText(text);
}

QVariant LinkLabel::propertyLink() const
{
    return link;
}

void LinkLabel::onLinkActivated(const QString& s)
{
    Q_UNUSED(s);
    auto select = new LinkSelection(qvariant_cast<App::SubObjectT>(link));
    QTimer::singleShot(50, select, &LinkSelection::select);  // NOLINT
}

void LinkLabel::onEditClicked()
{
    if (!dlg) {
        dlg = new DlgPropertyLink(this);
        dlg->init(objProp, true);
        connect(dlg, &DlgPropertyLink::accepted, this, &LinkLabel::onLinkChanged);
    }
    else {
        dlg->init(objProp, false);
    }

    dlg->show();
}

void LinkLabel::onLinkChanged()
{
    if (dlg) {
        auto links = dlg->currentLinks();
        if (links != dlg->originalLinks()) {
            link = QVariant::fromValue(links);
            Q_EMIT linkChanged(link);
            updatePropertyLink();
        }
    }
}

void LinkLabel::resizeEvent(QResizeEvent* e)
{
    editButton->setFixedWidth(e->size().height());
}

// --------------------------------------------------------------------

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyLinkItem)

PropertyLinkItem::PropertyLinkItem() = default;

QString PropertyLinkItem::toString(const QVariant& prop) const
{
    if (propertyItems.empty()) {
        return {};
    }
    App::DocumentObjectT owner(propertyItems[0]);
    return DlgPropertyLink::formatLinks(
        owner.getDocument(),
        qvariant_cast<QList<App::SubObjectT>>(prop)
    );
}

QVariant PropertyLinkItem::data(int column, int role) const
{
    if (!propertyItems.empty() && column == 1
        && (role == Qt::ForegroundRole || role == Qt::ToolTipRole)) {
        if (auto propLink = dynamic_cast<const App::PropertyLinkBase*>(propertyItems[0])) {
            if (role == Qt::ForegroundRole && propLink->checkRestore() > 1) {
                return QVariant::fromValue(QColor(0xff, 0, 0));  // NOLINT
            }
            if (role == Qt::ToolTipRole) {
                if (auto xlink = dynamic_cast<const App::PropertyXLink*>(propertyItems[0])) {
                    const char* filePath = xlink->getFilePath();
                    if (!Base::Tools::isNullOrEmpty(filePath)) {
                        return QVariant::fromValue(QString::fromUtf8(filePath));
                    }
                }
            }
        }
    }

    return PropertyItem::data(column, role);
}

QVariant PropertyLinkItem::value(const App::Property* prop) const
{
    auto propLink = freecad_cast<App::PropertyLinkBase*>(prop);
    if (!propLink) {
        return {};
    }

    auto links = DlgPropertyLink::getLinksFromProperty(propLink);
    if (links.empty()) {
        return {};
    }

    return QVariant::fromValue(links);
}

void PropertyLinkItem::setValue(const QVariant& value)
{
    auto links = qvariant_cast<QList<App::SubObjectT>>(value);
    setPropertyValue(DlgPropertyLink::linksToPython(links));
}

QWidget* PropertyLinkItem::
    createEditor(QWidget* parent, const std::function<void()>& method, FrameOption /*frameOption*/) const
{
    if (propertyItems.empty()) {
        return nullptr;
    }
    auto ll = new LinkLabel(parent, propertyItems.front());
    ll->setAutoFillBackground(true);
    QObject::connect(ll, &LinkLabel::linkChanged, method);
    return ll;
}

void PropertyLinkItem::setEditorData(QWidget* editor, const QVariant& data) const
{
    (void)data;
    auto ll = qobject_cast<LinkLabel*>(editor);
    return ll->updatePropertyLink();
}

QVariant PropertyLinkItem::editorData(QWidget* editor) const
{
    auto ll = qobject_cast<LinkLabel*>(editor);
    return ll->propertyLink();
}

// --------------------------------------------------------------------

PROPERTYITEM_SOURCE(Gui::PropertyEditor::PropertyLinkListItem)

PropertyLinkListItem::PropertyLinkListItem() = default;

PropertyItemEditorFactory::PropertyItemEditorFactory() = default;

PropertyItemEditorFactory::~PropertyItemEditorFactory() = default;

QWidget* PropertyItemEditorFactory::createEditor(int /*type*/, QWidget* /*parent*/) const
{
    // do not allow to create any editor widgets because we do that in subclasses of PropertyItem
    return nullptr;
}

QByteArray PropertyItemEditorFactory::valuePropertyName(int /*type*/) const
{
    // do not allow to set properties because we do that in subclasses of PropertyItem
    return "";
}
// NOLINTEND(cppcoreguidelines-pro-*,cppcoreguidelines-prefer-member-initializer)

#include "moc_PropertyItem.cpp"
