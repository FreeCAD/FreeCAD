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


#pragma once

#include <QItemEditorFactory>
#include <QObject>
#include <QPointer>
#include <QPushButton>
#include <QItemEditorFactory>
#include <vector>

#include <App/PropertyStandard.h>
#include <Base/Factory.h>
#include <Base/Matrix.h>
#include <Base/Placement.h>
#include <Base/Quantity.h>
#include <Base/Vector3D.h>
#include <Base/UnitsApi.h>
#include <Gui/ExpressionBinding.h>
#include <Gui/MetaTypes.h>
#include <Gui/Widgets.h>

#include <FCGlobal.h>

#ifdef Q_MOC_RUN
Q_DECLARE_METATYPE(Base::Vector3f)
Q_DECLARE_METATYPE(Base::Vector3d)
Q_DECLARE_METATYPE(QList<Base::Vector3d>)
Q_DECLARE_METATYPE(Base::Matrix4D)
Q_DECLARE_METATYPE(Base::Placement)
Q_DECLARE_METATYPE(Base::Rotation)
Q_DECLARE_METATYPE(Base::Quantity)
Q_DECLARE_METATYPE(QList<Base::Quantity>)
#endif


#define PROPERTYITEM_HEADER \
public: \
    static void* create(void); \
    static void init(void);

#define PROPERTYITEM_SOURCE(_class_) \
    void* _class_::create(void) \
    { \
        return new _class_(); \
    } \
    void _class_::init(void) \
    { \
        (void)new Gui::PropertyEditor::PropertyItemProducer<_class_>(#_class_); \
    }

namespace Gui
{

namespace Dialog
{
class TaskPlacement;
class DlgPropertyLink;
}  // namespace Dialog

namespace PropertyEditor
{

class PropertyItem;
class PropertyModel;
class PropertyEditorWidget;

enum class FrameOption : bool
{
    NoFrame = false,
    WithFrame = true
};
/**
 * The PropertyItemFactory provides methods for the dynamic creation of property items.
 * \author Werner Mayer
 */
class GuiExport PropertyItemFactory: public Base::Factory
{
public:
    static PropertyItemFactory& instance();

    PropertyItem* createPropertyItem(const char* sName) const;

private:
    PropertyItemFactory() = default;
    ~PropertyItemFactory() override = default;
};

template<class CLASS>
class PropertyItemProducer: public Base::AbstractProducer
{
public:
    explicit PropertyItemProducer(const char* className)
    {
        PropertyItemFactory::instance().AddProducer(className, this);
    }
    ~PropertyItemProducer() override = default;
    void* Produce() const override
    {
        return CLASS::create();
    }
};

class PropertyItemAttorney
{
public:
    static QString toString(PropertyItem* item, const QVariant& v);
};

class GuiExport PropertyItem: public QObject, public ExpressionBinding
{
    Q_OBJECT
    PROPERTYITEM_HEADER

public:
    enum Column
    {
        NameColumn = 0,
        ValueColumn = 1,
        ColumnCount
    };

    ~PropertyItem() override;

    /** Sets the current property objects. */
    void setPropertyData(const std::vector<App::Property*>&);
    void updateData();
    const std::vector<App::Property*>& getPropertyData() const;
    bool hasProperty(const App::Property*) const;
    virtual void assignProperty(const App::Property*);
    bool removeProperty(const App::Property*);
    bool renameProperty(const App::Property*);
    App::Property* getFirstProperty();
    const App::Property* getFirstProperty() const;

    /** Creates the appropriate editor for this item and sets the editor to the value of
     * overrideValue(). */
    virtual QWidget* createEditor(
        QWidget* parent,
        const std::function<void()>& method,
        FrameOption frameOption = FrameOption::NoFrame
    ) const;
    virtual void setEditorData(QWidget* editor, const QVariant& data) const;
    virtual QVariant editorData(QWidget* editor) const;
    virtual bool isSeparator() const
    {
        return false;
    }

    QWidget* createExpressionEditor(QWidget* parent, const std::function<void()>& method) const;
    void setExpressionEditorData(QWidget* editor, const QVariant& data) const;
    QVariant expressionEditorData(QWidget* editor) const;

    PropertyEditorWidget* createPropertyEditorWidget(QWidget* parent) const;

    /**override the bind functions to ensure we issue the propertyBound() call, which is then
       overloaded by childs which like to be informed of a binding*/
    void bind(const App::Property& prop) override;
    void bind(const App::ObjectIdentifier& _path) override;
    virtual void propertyBound()
    {}
    QString expressionAsString() const;

    void setParent(PropertyItem* parent);
    PropertyItem* parent() const;
    void appendChild(PropertyItem* child);
    void insertChild(int, PropertyItem* child);
    void moveChild(int from, int to);
    void removeChildren(int from, int to);
    PropertyItem* takeChild(int);

    void setReadOnly(bool);
    bool isReadOnly() const;
    bool testStatus(App::Property::Status pos) const;
    void setDecimals(int);
    int decimals() const;

    void setLinked(bool);
    bool isLinked() const;

    bool isExpanded() const;
    void setExpanded(bool e);

    PropertyItem* child(int row);
    int childCount() const;
    int columnCount() const;
    QString propertyName() const;
    void setPropertyName(const QString& name, const QString& realName = QString());
    void setPropertyValue(const QString&);
    void setNameToolTipOverride(const QString& tooltip);
    virtual QVariant data(int column, int role) const;
    bool setData(const QVariant& value);
    Qt::ItemFlags flags(int column) const;
    virtual int row() const;
    void reset();

    bool hasAnyExpression() const;

protected:
    PropertyItem();

    void setPropertyValue(const std::string& value);
    virtual QVariant displayName() const;
    virtual QVariant decoration(const QVariant&) const;
    virtual QVariant toolTip(const App::Property*) const;
    virtual QString toString(const QVariant&) const;
    virtual QVariant value(const App::Property*) const;
    virtual void setValue(const QVariant&);
    virtual void initialize();

    // gets called when the bound expression is changed
    void onChange() override;

private:
    QVariant dataPropertyName(int role) const;
    QVariant dataValue(int role) const;
    QString toString(const Py::Object&) const;
    QString asNone(const Py::Object&) const;
    QString asString(const Py::Object&) const;
    QString asSequence(const Py::Object&) const;
    QString asMapping(const Py::Object&) const;

protected:
    QString propName;
    QString displayText;
    std::vector<App::Property*> propertyItems;
    PropertyItem* parentItem;
    QList<PropertyItem*> childItems;
    bool readonly;
    int precision;
    bool linked;
    bool expanded;
    QString nameToolTipOverride;

    friend class PropertyItemAttorney;
};

/**
 * Change a string property.
 * \author Werner Mayer
 */
class GuiExport PropertyStringItem: public PropertyItem
{
    Q_OBJECT
    PROPERTYITEM_HEADER

    QWidget* createEditor(
        QWidget* parent,
        const std::function<void()>& method,
        FrameOption frameOption = FrameOption::NoFrame
    ) const override;
    void setEditorData(QWidget* editor, const QVariant& data) const override;
    QVariant editorData(QWidget* editor) const override;
    QVariant toolTip(const App::Property*) const override;

protected:
    QVariant value(const App::Property*) const override;
    void setValue(const QVariant&) override;

protected:
    PropertyStringItem();
};

/**
 * Change a font property.
 * \author Werner Mayer
 */
class GuiExport PropertyFontItem: public PropertyItem
{
    Q_OBJECT
    PROPERTYITEM_HEADER

    QWidget* createEditor(
        QWidget* parent,
        const std::function<void()>& method,
        FrameOption frameOption = FrameOption::NoFrame
    ) const override;
    void setEditorData(QWidget* editor, const QVariant& data) const override;
    QVariant editorData(QWidget* editor) const override;

protected:
    QVariant value(const App::Property*) const override;
    void setValue(const QVariant&) override;

protected:
    PropertyFontItem();
};

/**
 * Dummy property to separate groups of properties.
 * \author Werner Mayer
 */
class GuiExport PropertySeparatorItem: public PropertyItem
{
    Q_OBJECT
    PROPERTYITEM_HEADER

    bool isSeparator() const override
    {
        return true;
    }
    QWidget* createEditor(
        QWidget* parent,
        const std::function<void()>& method,
        FrameOption frameOption = FrameOption::NoFrame
    ) const override;

    int row() const override
    {
        return _row < 0 ? PropertyItem::row() : _row;
    }

private:
    friend PropertyModel;
    int _row = -1;
};

/**
 * Change a number.
 * \author Werner Mayer
 */
class GuiExport PropertyIntegerItem: public PropertyItem
{
    Q_OBJECT
    PROPERTYITEM_HEADER

    QWidget* createEditor(
        QWidget* parent,
        const std::function<void()>& method,
        FrameOption frameOption = FrameOption::NoFrame
    ) const override;
    void setEditorData(QWidget* editor, const QVariant& data) const override;
    QVariant editorData(QWidget* editor) const override;

protected:
    QString toString(const QVariant&) const override;
    QVariant value(const App::Property*) const override;
    void setValue(const QVariant&) override;

protected:
    PropertyIntegerItem();
};

/**
 * Change a number with constraints.
 * \author Werner Mayer
 */
class GuiExport PropertyIntegerConstraintItem: public PropertyItem
{
    Q_OBJECT
    PROPERTYITEM_HEADER

    QWidget* createEditor(
        QWidget* parent,
        const std::function<void()>& method,
        FrameOption frameOption = FrameOption::NoFrame
    ) const override;
    void setEditorData(QWidget* editor, const QVariant& data) const override;
    QVariant editorData(QWidget* editor) const override;

    void setRange(int min, int max)
    {
        this->min = min;
        this->max = max;
    }

    void setStepSize(int steps)
    {
        this->steps = steps;
    }

protected:
    QString toString(const QVariant&) const override;
    QVariant value(const App::Property*) const override;
    void setValue(const QVariant&) override;

protected:
    PropertyIntegerConstraintItem();

private:
    int min = std::numeric_limits<int>::min();
    int max = std::numeric_limits<int>::max();
    int steps = 1;
};

/**
 * Change a floating point number.
 * \author Werner Mayer
 */
class GuiExport PropertyFloatItem: public PropertyItem
{
    Q_OBJECT
    PROPERTYITEM_HEADER

    QWidget* createEditor(
        QWidget* parent,
        const std::function<void()>& method,
        FrameOption frameOption = FrameOption::NoFrame
    ) const override;
    void setEditorData(QWidget* editor, const QVariant& data) const override;
    QVariant editorData(QWidget* editor) const override;

protected:
    QString toString(const QVariant&) const override;
    QVariant value(const App::Property*) const override;
    void setValue(const QVariant&) override;

protected:
    PropertyFloatItem();
};

/**
 * Change a Unit based floating point number.
 * \author Juergen Riegel
 */
class GuiExport PropertyUnitItem: public PropertyItem
{
    Q_OBJECT
    PROPERTYITEM_HEADER

    QWidget* createEditor(
        QWidget* parent,
        const std::function<void()>& method,
        FrameOption frameOption = FrameOption::NoFrame
    ) const override;
    void setEditorData(QWidget* editor, const QVariant& data) const override;
    QVariant editorData(QWidget* editor) const override;

protected:
    QString toString(const QVariant&) const override;
    QVariant value(const App::Property*) const override;
    void setValue(const QVariant&) override;

    PropertyUnitItem();
};

/**
 * Change a Unit based floating point number within constraints.
 * \author Stefan Troeger
 */
class GuiExport PropertyUnitConstraintItem: public PropertyUnitItem
{
    Q_OBJECT
    PROPERTYITEM_HEADER

    void setEditorData(QWidget* editor, const QVariant& data) const override;

    void setRange(double min, double max)
    {
        this->min = min;
        this->max = max;
    }

    void setStepSize(double steps)
    {
        this->steps = steps;
    }

protected:
    PropertyUnitConstraintItem();

private:
    double min = static_cast<double>(std::numeric_limits<int>::min());
    double max = static_cast<double>(std::numeric_limits<int>::max());
    double steps = 0.1;
};

/**
 * Change a floating point number with constraints.
 * \author Werner Mayer
 */
class GuiExport PropertyFloatConstraintItem: public PropertyItem
{
    Q_OBJECT
    PROPERTYITEM_HEADER

    QWidget* createEditor(
        QWidget* parent,
        const std::function<void()>& method,
        FrameOption frameOption = FrameOption::NoFrame
    ) const override;
    void setEditorData(QWidget* editor, const QVariant& data) const override;
    QVariant editorData(QWidget* editor) const override;

    void setRange(double min, double max)
    {
        this->min = min;
        this->max = max;
    }

    void setStepSize(double steps)
    {
        this->steps = steps;
    }

protected:
    QString toString(const QVariant&) const override;
    QVariant value(const App::Property*) const override;
    void setValue(const QVariant&) override;

protected:
    PropertyFloatConstraintItem();

private:
    double min = static_cast<double>(std::numeric_limits<int>::min());
    double max = static_cast<double>(std::numeric_limits<int>::max());
    double steps = 0.1;
};

/**
 * Change a floating point number with many decimal points (hard coded as 16)
 */
class GuiExport PropertyPrecisionItem: public PropertyFloatConstraintItem
{
    Q_OBJECT
    PROPERTYITEM_HEADER
protected:
    PropertyPrecisionItem();
};

/**
 * Change a floating point number.
 * \author Werner Mayer
 */
class GuiExport PropertyAngleItem: public PropertyUnitConstraintItem
{
    Q_OBJECT
    PROPERTYITEM_HEADER

protected:
    void setEditorData(QWidget* editor, const QVariant& data) const override;
    QString toString(const QVariant&) const override;

protected:
    PropertyAngleItem();
};

/**
 * Edit properties of boolean type.
 * \author Werner Mayer
 */
class GuiExport PropertyBoolItem: public PropertyItem
{
    Q_OBJECT
    PROPERTYITEM_HEADER

    QWidget* createEditor(
        QWidget* parent,
        const std::function<void()>& method,
        FrameOption frameOption = FrameOption::NoFrame
    ) const override;
    void setEditorData(QWidget* editor, const QVariant& data) const override;
    QVariant editorData(QWidget* editor) const override;

protected:
    QVariant value(const App::Property*) const override;
    void setValue(const QVariant&) override;

protected:
    PropertyBoolItem();
};

/**
 * Edit properties of vector type.
 * \author Werner Mayer
 */
class PropertyFloatItem;
class GuiExport PropertyVectorItem: public PropertyItem
{
    // clang-format off
    Q_OBJECT
    Q_PROPERTY(double x READ x WRITE setX DESIGNABLE true USER true)  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(double y READ y WRITE setY DESIGNABLE true USER true)  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(double z READ z WRITE setZ DESIGNABLE true USER true)  // clazy:exclude=qproperty-without-notify
    PROPERTYITEM_HEADER
    // clang-format on

    QWidget* createEditor(
        QWidget* parent,
        const std::function<void()>& method,
        FrameOption frameOption = FrameOption::NoFrame
    ) const override;
    void setEditorData(QWidget* editor, const QVariant& data) const override;
    QVariant editorData(QWidget* editor) const override;

    double x() const;
    void setX(double x);
    double y() const;
    void setY(double y);
    double z() const;
    void setZ(double z);

protected:
    QString toString(const QVariant&) const override;
    QVariant value(const App::Property*) const override;
    void setValue(const QVariant&) override;

protected:
    PropertyVectorItem();
    void propertyBound() override;

private:
    PropertyFloatItem* m_x;
    PropertyFloatItem* m_y;
    PropertyFloatItem* m_z;
};

class PropertyEditorWidget: public QWidget
{
    Q_OBJECT

public:
    explicit PropertyEditorWidget(QWidget* parent = nullptr);
    ~PropertyEditorWidget() override;

    QVariant value() const;

public Q_SLOTS:
    void setValue(const QVariant&);

protected:
    virtual void showValue(const QVariant& data);
    void resizeEvent(QResizeEvent*) override;

Q_SIGNALS:
    void buttonClick();
    void valueChanged(const QVariant&);

protected:
    QVariant variant;
    QLineEdit* lineEdit;
    QPushButton* button;
};

class VectorListWidget: public PropertyEditorWidget
{
    Q_OBJECT

public:
    explicit VectorListWidget(int decimals, QWidget* parent = nullptr);

protected:
    void showValue(const QVariant& data) override;

private Q_SLOTS:
    void buttonClicked();

private:
    int decimals;
};

/**
 * Edit properties of vector list type.
 * \author Werner Mayer
 */
class GuiExport PropertyVectorListItem: public PropertyItem
{
    Q_OBJECT
    PROPERTYITEM_HEADER

    QWidget* createEditor(
        QWidget* parent,
        const std::function<void()>& method,
        FrameOption frameOption = FrameOption::NoFrame
    ) const override;
    void setEditorData(QWidget* editor, const QVariant& data) const override;
    QVariant editorData(QWidget* editor) const override;

protected:
    QString toString(const QVariant&) const override;
    QVariant value(const App::Property*) const override;
    void setValue(const QVariant&) override;

protected:
    PropertyVectorListItem();
};

/**
 * Edit properties of vector type which hold distances.
 * \author Stefan Troeger
 */
class PropertyUnitItem;
class GuiExport PropertyVectorDistanceItem: public PropertyItem
{
    // clang-format off
    Q_OBJECT
    Q_PROPERTY(Base::Quantity x READ x WRITE setX DESIGNABLE true USER true)  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(Base::Quantity y READ y WRITE setY DESIGNABLE true USER true)  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(Base::Quantity z READ z WRITE setZ DESIGNABLE true USER true)  // clazy:exclude=qproperty-without-notify
    PROPERTYITEM_HEADER
    // clang-format on

    QWidget* createEditor(
        QWidget* parent,
        const std::function<void()>& method,
        FrameOption frameOption = FrameOption::NoFrame
    ) const override;
    void setEditorData(QWidget* editor, const QVariant& data) const override;
    QVariant editorData(QWidget* editor) const override;

    void propertyBound() override;

    Base::Quantity x() const;
    void setX(Base::Quantity x);
    Base::Quantity y() const;
    void setY(Base::Quantity y);
    Base::Quantity z() const;
    void setZ(Base::Quantity z);

protected:
    QString toString(const QVariant&) const override;
    QVariant value(const App::Property*) const override;
    void setValue(const QVariant&) override;

    PropertyVectorDistanceItem();

private:
    PropertyUnitItem* m_x;
    PropertyUnitItem* m_y;
    PropertyUnitItem* m_z;
};

class GuiExport PropertyPositionItem: public PropertyVectorDistanceItem
{
    Q_OBJECT
    PROPERTYITEM_HEADER
};

class GuiExport PropertyDirectionItem: public PropertyVectorDistanceItem
{
    Q_OBJECT
    PROPERTYITEM_HEADER
};

class GuiExport PropertyMatrixItem: public PropertyItem
{
    // clang-format off
    Q_OBJECT
    Q_PROPERTY(double A11 READ getA11 WRITE setA11 DESIGNABLE true USER true)  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(double A12 READ getA12 WRITE setA12 DESIGNABLE true USER true)  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(double A13 READ getA13 WRITE setA13 DESIGNABLE true USER true)  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(double A14 READ getA14 WRITE setA14 DESIGNABLE true USER true)  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(double A21 READ getA21 WRITE setA21 DESIGNABLE true USER true)  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(double A22 READ getA22 WRITE setA22 DESIGNABLE true USER true)  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(double A23 READ getA23 WRITE setA23 DESIGNABLE true USER true)  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(double A24 READ getA24 WRITE setA24 DESIGNABLE true USER true)  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(double A31 READ getA31 WRITE setA31 DESIGNABLE true USER true)  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(double A32 READ getA32 WRITE setA32 DESIGNABLE true USER true)  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(double A33 READ getA33 WRITE setA33 DESIGNABLE true USER true)  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(double A34 READ getA34 WRITE setA34 DESIGNABLE true USER true)  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(double A41 READ getA41 WRITE setA41 DESIGNABLE true USER true)  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(double A42 READ getA42 WRITE setA42 DESIGNABLE true USER true)  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(double A43 READ getA43 WRITE setA43 DESIGNABLE true USER true)  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(double A44 READ getA44 WRITE setA44 DESIGNABLE true USER true)  // clazy:exclude=qproperty-without-notify
    PROPERTYITEM_HEADER
    // clang-format on

    QWidget* createEditor(
        QWidget* parent,
        const std::function<void()>& method,
        FrameOption frameOption = FrameOption::NoFrame
    ) const override;
    void setEditorData(QWidget* editor, const QVariant& data) const override;
    QVariant editorData(QWidget* editor) const override;

    double getA11() const;
    void setA11(double A11);
    double getA12() const;
    void setA12(double A12);
    double getA13() const;
    void setA13(double A13);
    double getA14() const;
    void setA14(double A14);
    double getA21() const;
    void setA21(double A21);
    double getA22() const;
    void setA22(double A22);
    double getA23() const;
    void setA23(double A23);
    double getA24() const;
    void setA24(double A24);
    double getA31() const;
    void setA31(double A31);
    double getA32() const;
    void setA32(double A32);
    double getA33() const;
    void setA33(double A33);
    double getA34() const;
    void setA34(double A34);
    double getA41() const;
    void setA41(double A41);
    double getA42() const;
    void setA42(double A42);
    double getA43() const;
    void setA43(double A43);
    double getA44() const;
    void setA44(double A44);

protected:
    QString toString(const QVariant&) const override;
    QVariant value(const App::Property*) const override;
    void setValue(const QVariant&) override;

protected:
    PropertyMatrixItem();
    QVariant toolTip(const App::Property*) const override;

private:
    PropertyFloatItem* m_a11;
    PropertyFloatItem* m_a12;
    PropertyFloatItem* m_a13;
    PropertyFloatItem* m_a14;
    PropertyFloatItem* m_a21;
    PropertyFloatItem* m_a22;
    PropertyFloatItem* m_a23;
    PropertyFloatItem* m_a24;
    PropertyFloatItem* m_a31;
    PropertyFloatItem* m_a32;
    PropertyFloatItem* m_a33;
    PropertyFloatItem* m_a34;
    PropertyFloatItem* m_a41;
    PropertyFloatItem* m_a42;
    PropertyFloatItem* m_a43;
    PropertyFloatItem* m_a44;
};

class RotationHelper
{
public:
    RotationHelper();
    void setChanged(bool);
    bool hasChangedAndReset();
    bool isAxisInitialized() const;
    void setValue(const Base::Vector3d& axis, double angle);
    void getValue(Base::Vector3d& axis, double& angle) const;
    double getAngle(const Base::Rotation& val) const;
    Base::Rotation setAngle(double);
    Base::Vector3d getAxis() const;
    Base::Rotation setAxis(const Base::Rotation& value, const Base::Vector3d& axis);
    void assignProperty(const Base::Rotation& value, double eps);

private:
    bool init_axis;
    bool changed_value;
    double rot_angle;
    Base::Vector3d rot_axis;
};

/**
 * Edit properties of rotation type.
 * \author Werner Mayer
 */
class GuiExport PropertyRotationItem: public PropertyItem
{
    // clang-format off
    Q_OBJECT
    Q_PROPERTY(Base::Quantity Angle READ getAngle WRITE setAngle DESIGNABLE true USER true)  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(Base::Vector3d Axis  READ getAxis  WRITE setAxis  DESIGNABLE true USER true)  // clazy:exclude=qproperty-without-notify
    PROPERTYITEM_HEADER
    // clang-format on

    QWidget* createEditor(
        QWidget* parent,
        const std::function<void()>& method,
        FrameOption frameOption = FrameOption::NoFrame
    ) const override;
    void setEditorData(QWidget* editor, const QVariant& data) const override;
    QVariant editorData(QWidget* editor) const override;

    void propertyBound() override;
    void assignProperty(const App::Property*) override;

    Base::Quantity getAngle() const;
    void setAngle(Base::Quantity);
    Base::Vector3d getAxis() const;
    void setAxis(const Base::Vector3d&);

protected:
    PropertyRotationItem();
    ~PropertyRotationItem() override;
    QVariant toolTip(const App::Property*) const override;
    QString toString(const QVariant&) const override;
    QVariant value(const App::Property*) const override;
    void setValue(const QVariant&) override;

private:
    mutable RotationHelper h;
    PropertyUnitItem* m_a;
    PropertyVectorItem* m_d;
};

class PlacementEditor: public Gui::LabelButton
{
    Q_OBJECT

public:
    explicit PlacementEditor(QString name, QWidget* parent = nullptr);
    ~PlacementEditor() override;

private Q_SLOTS:
    void updateValue(const QVariant& v, bool, bool);

private:
    void browse() override;
    void showValue(const QVariant& d) override;

private:
    QPointer<Gui::Dialog::TaskPlacement> _task;
    QString propertyname;
};

/**
 * Edit properties of placement type.
 * \author Werner Mayer
 */
class GuiExport PropertyPlacementItem: public PropertyItem
{
    // clang-format off
    Q_OBJECT
    Q_PROPERTY(Base::Quantity Angle    READ getAngle    WRITE setAngle    DESIGNABLE true USER true)  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(Base::Vector3d Axis     READ getAxis     WRITE setAxis     DESIGNABLE true USER true)  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(Base::Vector3d Position READ getPosition WRITE setPosition DESIGNABLE true USER true)  // clazy:exclude=qproperty-without-notify
    PROPERTYITEM_HEADER
    // clang-format on

    QWidget* createEditor(
        QWidget* parent,
        const std::function<void()>& method,
        FrameOption frameOption = FrameOption::NoFrame
    ) const override;
    void setEditorData(QWidget* editor, const QVariant& data) const override;
    QVariant editorData(QWidget* editor) const override;

    void propertyBound() override;
    void assignProperty(const App::Property*) override;

    Base::Quantity getAngle() const;
    void setAngle(Base::Quantity);
    Base::Vector3d getAxis() const;
    void setAxis(const Base::Vector3d&);
    Base::Vector3d getPosition() const;
    void setPosition(const Base::Vector3d&);

protected:
    PropertyPlacementItem();
    ~PropertyPlacementItem() override;
    QVariant toolTip(const App::Property*) const override;
    QString toString(const QVariant&) const override;
    QVariant value(const App::Property*) const override;
    void setValue(const QVariant&) override;

private:
    mutable RotationHelper h;
    PropertyUnitItem* m_a;
    PropertyVectorItem* m_d;
    PropertyVectorDistanceItem* m_p;
};

class PropertyStringListItem;

/**
 * Edit properties of enum type.
 * \author Werner Mayer
 */
class GuiExport PropertyEnumItem: public PropertyItem
{
    // clang-format off
    Q_OBJECT
    Q_PROPERTY(QStringList Enum READ getEnum WRITE setEnum DESIGNABLE true USER true)  // clazy:exclude=qproperty-without-notify
    PROPERTYITEM_HEADER
    // clang-format on

    QWidget* createEditor(
        QWidget* parent,
        const std::function<void()>& method,
        FrameOption frameOption = FrameOption::NoFrame
    ) const override;
    void setEditorData(QWidget* editor, const QVariant& data) const override;
    QVariant editorData(QWidget* editor) const override;

    QStringList getEnum() const;
    void setEnum(const QStringList&);

private:
    QStringList getCommonModes() const;

protected:
    QVariant value(const App::Property*) const override;
    void setValue(const QVariant&) override;
    void propertyBound() override;

protected:
    PropertyEnumItem();

private:
    PropertyStringListItem* m_enum;
};

class PropertyEnumButton: public QPushButton
{
    Q_OBJECT
public:
    explicit PropertyEnumButton(QWidget* parent = nullptr)
        : QPushButton(parent)
    {}

Q_SIGNALS:
    void picked();
};

/**
 * Edit properties of string list type.
 * \author Werner Mayer
 */
class GuiExport PropertyStringListItem: public PropertyItem
{
    Q_OBJECT
    PROPERTYITEM_HEADER

    QWidget* createEditor(
        QWidget* parent,
        const std::function<void()>& method,
        FrameOption frameOption = FrameOption::NoFrame
    ) const override;
    void setEditorData(QWidget* editor, const QVariant& data) const override;
    QVariant editorData(QWidget* editor) const override;

protected:
    QString toString(const QVariant&) const override;
    QVariant value(const App::Property*) const override;
    void setValue(const QVariant&) override;

protected:
    PropertyStringListItem();
};

/**
 * Edit properties of float list type.
 * \author Werner Mayer
 */
class GuiExport PropertyFloatListItem: public PropertyItem
{
    Q_OBJECT
    PROPERTYITEM_HEADER

    QWidget* createEditor(
        QWidget* parent,
        const std::function<void()>& method,
        FrameOption frameOption = FrameOption::NoFrame
    ) const override;
    void setEditorData(QWidget* editor, const QVariant& data) const override;
    QVariant editorData(QWidget* editor) const override;

protected:
    QString toString(const QVariant&) const override;
    QVariant value(const App::Property*) const override;
    void setValue(const QVariant&) override;

protected:
    PropertyFloatListItem();
};

/**
 * Edit properties of float list type.
 * \author Werner Mayer
 */
class GuiExport PropertyIntegerListItem: public PropertyItem
{
    Q_OBJECT
    PROPERTYITEM_HEADER

    QWidget* createEditor(
        QWidget* parent,
        const std::function<void()>& method,
        FrameOption frameOption = FrameOption::NoFrame
    ) const override;
    void setEditorData(QWidget* editor, const QVariant& data) const override;
    QVariant editorData(QWidget* editor) const override;

protected:
    QString toString(const QVariant&) const override;
    QVariant value(const App::Property*) const override;
    void setValue(const QVariant&) override;

protected:
    PropertyIntegerListItem();
};

/**
 * Change a color property.
 * \author Werner Mayer
 */
class GuiExport PropertyColorItem: public PropertyItem
{
    Q_OBJECT
    PROPERTYITEM_HEADER

    QWidget* createEditor(
        QWidget* parent,
        const std::function<void()>& method,
        FrameOption frameOption = FrameOption::NoFrame
    ) const override;
    void setEditorData(QWidget* editor, const QVariant& data) const override;
    QVariant editorData(QWidget* editor) const override;

protected:
    QVariant decoration(const QVariant&) const override;
    QString toString(const QVariant&) const override;
    QVariant value(const App::Property*) const override;
    void setValue(const QVariant&) override;

protected:
    PropertyColorItem();
};

/**
 * Change a material property.
 * \author Werner Mayer
 */
class GuiExport PropertyMaterialItem: public PropertyItem
{
    // clang-format off
    Q_OBJECT
    Q_PROPERTY(QColor AmbientColor  READ getAmbientColor  WRITE setAmbientColor  DESIGNABLE true USER true)  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(QColor DiffuseColor  READ getDiffuseColor  WRITE setDiffuseColor  DESIGNABLE true USER true)  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(QColor SpecularColor READ getSpecularColor WRITE setSpecularColor DESIGNABLE true USER true)  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(QColor EmissiveColor READ getEmissiveColor WRITE setEmissiveColor DESIGNABLE true USER true)  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(float Shininess      READ getShininess     WRITE setShininess     DESIGNABLE true USER true)  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(float Transparency   READ getTransparency  WRITE setTransparency  DESIGNABLE true USER true)  // clazy:exclude=qproperty-without-notify
    PROPERTYITEM_HEADER
    // clang-format on

    QWidget* createEditor(
        QWidget* parent,
        const std::function<void()>& method,
        FrameOption frameOption = FrameOption::NoFrame
    ) const override;
    void setEditorData(QWidget* editor, const QVariant& data) const override;
    QVariant editorData(QWidget* editor) const override;

    void propertyBound() override;

    QColor getAmbientColor() const;
    void setAmbientColor(const QColor&);
    QColor getDiffuseColor() const;
    void setDiffuseColor(const QColor&);
    QColor getSpecularColor() const;
    void setSpecularColor(const QColor&);
    QColor getEmissiveColor() const;
    void setEmissiveColor(const QColor&);
    int getShininess() const;
    void setShininess(int);
    int getTransparency() const;
    void setTransparency(int);

protected:
    PropertyMaterialItem();
    ~PropertyMaterialItem() override;

    QVariant decoration(const QVariant&) const override;
    QVariant toolTip(const App::Property*) const override;
    QString toString(const QVariant&) const override;
    QVariant value(const App::Property*) const override;
    void setValue(const QVariant&) override;

private:
    PropertyColorItem* ambient;
    PropertyColorItem* diffuse;
    PropertyColorItem* specular;
    PropertyColorItem* emissive;
    PropertyIntegerConstraintItem* shininess;
    PropertyIntegerConstraintItem* transparency;
};

class GuiExport PropertyMaterialListItem: public PropertyItem
{
    // clang-format off
    Q_OBJECT
    Q_PROPERTY(QColor AmbientColor  READ getAmbientColor  WRITE setAmbientColor  DESIGNABLE true USER true)  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(QColor DiffuseColor  READ getDiffuseColor  WRITE setDiffuseColor  DESIGNABLE true USER true)  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(QColor SpecularColor READ getSpecularColor WRITE setSpecularColor DESIGNABLE true USER true)  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(QColor EmissiveColor READ getEmissiveColor WRITE setEmissiveColor DESIGNABLE true USER true)  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(float Shininess      READ getShininess     WRITE setShininess     DESIGNABLE true USER true)  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(float Transparency   READ getTransparency  WRITE setTransparency  DESIGNABLE true USER true)  // clazy:exclude=qproperty-without-notify
    PROPERTYITEM_HEADER
    // clang-format on

    QWidget* createEditor(
        QWidget* parent,
        const std::function<void()>& method,
        FrameOption frameOption = FrameOption::NoFrame
    ) const override;
    void setEditorData(QWidget* editor, const QVariant& data) const override;
    QVariant editorData(QWidget* editor) const override;

    void propertyBound() override;

    QColor getAmbientColor() const;
    void setAmbientColor(const QColor&);
    QColor getDiffuseColor() const;
    void setDiffuseColor(const QColor&);
    QColor getSpecularColor() const;
    void setSpecularColor(const QColor&);
    QColor getEmissiveColor() const;
    void setEmissiveColor(const QColor&);
    int getShininess() const;
    void setShininess(int);
    int getTransparency() const;
    void setTransparency(int);

protected:
    PropertyMaterialListItem();
    ~PropertyMaterialListItem() override;

    QVariant decoration(const QVariant&) const override;
    QVariant toolTip(const App::Property*) const override;
    QString toString(const QVariant&) const override;
    QVariant value(const App::Property*) const override;
    void setValue(const QVariant&) override;

private:
    PropertyColorItem* ambient;
    PropertyColorItem* diffuse;
    PropertyColorItem* specular;
    PropertyColorItem* emissive;
    PropertyIntegerConstraintItem* shininess;
    PropertyIntegerConstraintItem* transparency;
};

/**
 * Change a file.
 * \author Werner Mayer
 */
class GuiExport PropertyFileItem: public PropertyItem
{
    Q_OBJECT
    PROPERTYITEM_HEADER

    QWidget* createEditor(
        QWidget* parent,
        const std::function<void()>& method,
        FrameOption frameOption = FrameOption::NoFrame
    ) const override;
    void setEditorData(QWidget* editor, const QVariant& data) const override;
    QVariant editorData(QWidget* editor) const override;

protected:
    QVariant value(const App::Property*) const override;
    void setValue(const QVariant&) override;

protected:
    PropertyFileItem();
    QVariant toolTip(const App::Property*) const override;
};

/**
 * Change a path.
 * \author Werner Mayer
 */
class GuiExport PropertyPathItem: public PropertyItem
{
    Q_OBJECT
    PROPERTYITEM_HEADER

    QWidget* createEditor(
        QWidget* parent,
        const std::function<void()>& method,
        FrameOption frameOption = FrameOption::NoFrame
    ) const override;
    void setEditorData(QWidget* editor, const QVariant& data) const override;
    QVariant editorData(QWidget* editor) const override;

protected:
    QVariant value(const App::Property*) const override;
    void setValue(const QVariant&) override;

protected:
    PropertyPathItem();
    QVariant toolTip(const App::Property*) const override;
};

/**
 * Show path of included file.
 * \author Werner Mayer
 */
class GuiExport PropertyTransientFileItem: public PropertyItem
{
    Q_OBJECT
    PROPERTYITEM_HEADER

    QWidget* createEditor(
        QWidget* parent,
        const std::function<void()>& method,
        FrameOption frameOption = FrameOption::NoFrame
    ) const override;
    void setEditorData(QWidget* editor, const QVariant& data) const override;
    QVariant editorData(QWidget* editor) const override;

protected:
    QVariant value(const App::Property*) const override;
    void setValue(const QVariant&) override;

protected:
    PropertyTransientFileItem();
    QVariant toolTip(const App::Property*) const override;
};

class LinkSelection: public QObject
{
    Q_OBJECT

public:
    explicit LinkSelection(App::SubObjectT);
    ~LinkSelection() override;

public Q_SLOTS:
    void select();

private:
    App::SubObjectT link;
};


class LinkLabel: public QWidget
{
    Q_OBJECT

public:
    LinkLabel(QWidget* parent, const App::Property* prop);
    ~LinkLabel() override;
    void updatePropertyLink();
    QVariant propertyLink() const;

protected:
    void resizeEvent(QResizeEvent*) override;

protected Q_SLOTS:
    void onLinkActivated(const QString&);
    void onEditClicked();
    void onLinkChanged();

Q_SIGNALS:
    void linkChanged(const QVariant&);

private:
    QLabel* label;
    QPushButton* editButton;
    QVariant link;
    App::DocumentObjectT objProp;

    Gui::Dialog::DlgPropertyLink* dlg;
};

/**
 * Edit properties of link type.
 * \author Werner Mayer
 */
class GuiExport PropertyLinkItem: public PropertyItem
{
    Q_OBJECT
    PROPERTYITEM_HEADER

    QWidget* createEditor(
        QWidget* parent,
        const std::function<void()>& method,
        FrameOption frameOption = FrameOption::NoFrame
    ) const override;
    void setEditorData(QWidget* editor, const QVariant& data) const override;
    QVariant editorData(QWidget* editor) const override;

protected:
    QString toString(const QVariant&) const override;
    QVariant value(const App::Property*) const override;
    void setValue(const QVariant&) override;
    QVariant data(int column, int role) const override;

protected:
    PropertyLinkItem();
};

/**
 * Edit properties of link list type.
 * \author Werner Mayer
 */
class GuiExport PropertyLinkListItem: public PropertyLinkItem
{
    Q_OBJECT
    PROPERTYITEM_HEADER

protected:
    PropertyLinkListItem();
};

class PropertyItemEditorFactory: public QItemEditorFactory
{
public:
    PropertyItemEditorFactory();
    ~PropertyItemEditorFactory() override;

    QWidget* createEditor(int userType, QWidget* parent) const override;
    QByteArray valuePropertyName(int userType) const override;
};

}  // namespace PropertyEditor
}  // namespace Gui
