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


#ifndef PROPERTYEDITORITEM_H
#define PROPERTYEDITORITEM_H

#include <QObject>
#include <QPointer>
#include <QItemEditorFactory>
#include <vector>

#include <Base/Type.h>
#include <Base/Vector3D.h>
#include <Base/Matrix.h>
#include <Base/Placement.h>
#include <Base/UnitsApi.h>
#include <App/PropertyStandard.h>
#include <Gui/Widgets.h>

Q_DECLARE_METATYPE(Base::Vector3f)
Q_DECLARE_METATYPE(Base::Vector3d)
Q_DECLARE_METATYPE(Base::Matrix4D)
Q_DECLARE_METATYPE(Base::Placement)

namespace Gui {
namespace Dialog { class TaskPlacement; }
namespace PropertyEditor {

class GuiExport PropertyItem : virtual public QObject, public Base::BaseClass
{
    Q_OBJECT

    TYPESYSTEM_HEADER();

public:
    ~PropertyItem();

    /** Sets the current property objects. */
    void setPropertyData( const std::vector<App::Property*>& );
    const std::vector<App::Property*>& getPropertyData() const;

    /** Creates the appropriate editor for this item and sets the editor to the value of overrideValue(). */
    virtual QWidget* createEditor(QWidget* parent, const QObject* receiver, const char* method) const;
    virtual void setEditorData(QWidget *editor, const QVariant& data) const;
    virtual QVariant editorData(QWidget *editor) const;
    virtual bool isSeparator() const { return false; }

    void setParent(PropertyItem* parent);
    PropertyItem *parent() const;
    void appendChild(PropertyItem *child);

    void setReadOnly(bool);
    bool isReadOnly() const;
    void setDecimals(int);
    int decimals() const;

    PropertyItem *child(int row);
    int childCount() const;
    int columnCount() const;
    QString propertyName() const;
    void setPropertyName(const QString&);
    void setPropertyValue(const QString&);
    QVariant data(int column, int role) const;
    bool setData (const QVariant& value);
    Qt::ItemFlags flags(int column) const;
    int row() const;
    void reset();

protected:
    PropertyItem();

    virtual QVariant decoration(const App::Property*) const;
    virtual QVariant toolTip(const App::Property*) const;
    virtual QVariant toString(const QVariant&) const;
    virtual QVariant value(const App::Property*) const;
    virtual void setValue(const QVariant&);
    QString pythonIdentifier(const App::Property*) const;

private:
    QString propName;
    QVariant propData;
    std::vector<App::Property*> propertyItems;
    PropertyItem *parentItem;
    QList<PropertyItem*> childItems;
    bool readonly;
    int precision;
};

/**
 * Change a string property.
 * \author Werner Mayer
 */
class GuiExport PropertyStringItem: public PropertyItem
{
    TYPESYSTEM_HEADER();

    virtual QWidget* createEditor(QWidget* parent, const QObject* receiver, const char* method) const;
    virtual void setEditorData(QWidget *editor, const QVariant& data) const;
    virtual QVariant editorData(QWidget *editor) const;

protected:
    virtual QVariant value(const App::Property*) const;
    virtual void setValue(const QVariant&);

protected:
    PropertyStringItem();
};

/**
 * Change a font property.
 * \author Werner Mayer
 */
class GuiExport PropertyFontItem: public PropertyItem
{
    TYPESYSTEM_HEADER();

    virtual QWidget* createEditor(QWidget* parent, const QObject* receiver, const char* method) const;
    virtual void setEditorData(QWidget *editor, const QVariant& data) const;
    virtual QVariant editorData(QWidget *editor) const;

protected:
    virtual QVariant value(const App::Property*) const;
    virtual void setValue(const QVariant&);

protected:
    PropertyFontItem();
};

/**
 * Dummy property to separate groups of properties.
 * \author Werner Mayer
 */
class GuiExport PropertySeparatorItem : public PropertyItem
{
    TYPESYSTEM_HEADER();

    bool isSeparator() const { return true; }
    QWidget* createEditor(QWidget* parent, const QObject* receiver, const char* method) const;
};

/**
 * Change a number.
 * \author Werner Mayer
 */
class GuiExport PropertyIntegerItem: public PropertyItem
{
    TYPESYSTEM_HEADER();

    virtual QWidget* createEditor(QWidget* parent, const QObject* receiver, const char* method) const;
    virtual void setEditorData(QWidget *editor, const QVariant& data) const;
    virtual QVariant editorData(QWidget *editor) const;

protected:
    virtual QVariant value(const App::Property*) const;
    virtual void setValue(const QVariant&);

protected:
    PropertyIntegerItem();
};

/**
 * Change a number with constraints.
 * \author Werner Mayer
 */
class GuiExport PropertyIntegerConstraintItem: public PropertyItem
{
    TYPESYSTEM_HEADER();

    virtual QWidget* createEditor(QWidget* parent, const QObject* receiver, const char* method) const;
    virtual void setEditorData(QWidget *editor, const QVariant& data) const;
    virtual QVariant editorData(QWidget *editor) const;

protected:
    virtual QVariant value(const App::Property*) const;
    virtual void setValue(const QVariant&);

protected:
    PropertyIntegerConstraintItem();
};

/**
 * Change a floating point number.
 * \author Werner Mayer
 */
class GuiExport PropertyFloatItem: public PropertyItem
{
    TYPESYSTEM_HEADER();

    virtual QWidget* createEditor(QWidget* parent, const QObject* receiver, const char* method) const;
    virtual void setEditorData(QWidget *editor, const QVariant& data) const;
    virtual QVariant editorData(QWidget *editor) const;

protected:
    virtual QVariant toString(const QVariant&) const;
    virtual QVariant value(const App::Property*) const;
    virtual void setValue(const QVariant&);

protected:
    PropertyFloatItem();
};

/**
 * Change a Unit based floating point number.
 * \author Juergen Riegel
 */
class GuiExport PropertyUnitItem: public PropertyItem
{
    TYPESYSTEM_HEADER();

    virtual QWidget* createEditor(QWidget* parent, const QObject* receiver, const char* method) const;
    virtual void setEditorData(QWidget *editor, const QVariant& data) const;
    virtual QVariant editorData(QWidget *editor) const;

protected:
    virtual QVariant toString(const QVariant&) const;
    virtual QVariant value(const App::Property*) const;
    virtual void setValue(const QVariant&);
    Base::Unit  _Unit;

    PropertyUnitItem();
};

/**
 * Change a floating point number with constraints.
 * \author Werner Mayer
 */
class GuiExport PropertyFloatConstraintItem: public PropertyItem
{
    TYPESYSTEM_HEADER();

    virtual QWidget* createEditor(QWidget* parent, const QObject* receiver, const char* method) const;
    virtual void setEditorData(QWidget *editor, const QVariant& data) const;
    virtual QVariant editorData(QWidget *editor) const;

protected:
    virtual QVariant toString(const QVariant&) const;
    virtual QVariant value(const App::Property*) const;
    virtual void setValue(const QVariant&);

protected:
    PropertyFloatConstraintItem();
};

/**
 * Change a floating point number.
 * \author Werner Mayer
 */
class GuiExport PropertyAngleItem : public PropertyFloatItem
{
    TYPESYSTEM_HEADER();

protected:
    virtual void setEditorData(QWidget *editor, const QVariant& data) const;
    virtual QVariant toString(const QVariant&) const;

protected:
    PropertyAngleItem();
};

/**
 * Edit properties of boolean type. 
 * \author Werner Mayer
 */
class GuiExport PropertyBoolItem: public PropertyItem
{
    TYPESYSTEM_HEADER();

    virtual QWidget* createEditor(QWidget* parent, const QObject* receiver, const char* method) const;
    virtual void setEditorData(QWidget *editor, const QVariant& data) const;
    virtual QVariant editorData(QWidget *editor) const;

protected:
    virtual QVariant value(const App::Property*) const;
    virtual void setValue(const QVariant&);

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
    Q_OBJECT
    Q_PROPERTY(double x READ x WRITE setX DESIGNABLE true USER true)
    Q_PROPERTY(double y READ y WRITE setY DESIGNABLE true USER true)
    Q_PROPERTY(double z READ z WRITE setZ DESIGNABLE true USER true)
    TYPESYSTEM_HEADER();

    virtual QWidget* createEditor(QWidget* parent, const QObject* receiver, const char* method) const;
    virtual void setEditorData(QWidget *editor, const QVariant& data) const;
    virtual QVariant editorData(QWidget *editor) const;

    double x() const;
    void setX(double x);
    double y() const;
    void setY(double y);
    double z() const;
    void setZ(double z);

protected:
    virtual QVariant toString(const QVariant&) const;
    virtual QVariant value(const App::Property*) const;
    virtual void setValue(const QVariant&);

protected:
    PropertyVectorItem();

private:
    PropertyFloatItem* m_x;
    PropertyFloatItem* m_y;
    PropertyFloatItem* m_z;
};

class GuiExport PropertyMatrixItem: public PropertyItem
{
    Q_OBJECT
    Q_PROPERTY(double A11 READ getA11 WRITE setA11 DESIGNABLE true USER true)
    Q_PROPERTY(double A12 READ getA12 WRITE setA12 DESIGNABLE true USER true)
    Q_PROPERTY(double A13 READ getA13 WRITE setA13 DESIGNABLE true USER true)
    Q_PROPERTY(double A14 READ getA14 WRITE setA14 DESIGNABLE true USER true)
    Q_PROPERTY(double A21 READ getA21 WRITE setA21 DESIGNABLE true USER true)
    Q_PROPERTY(double A22 READ getA22 WRITE setA22 DESIGNABLE true USER true)
    Q_PROPERTY(double A23 READ getA23 WRITE setA23 DESIGNABLE true USER true)
    Q_PROPERTY(double A24 READ getA24 WRITE setA24 DESIGNABLE true USER true)
    Q_PROPERTY(double A31 READ getA31 WRITE setA31 DESIGNABLE true USER true)
    Q_PROPERTY(double A32 READ getA32 WRITE setA32 DESIGNABLE true USER true)
    Q_PROPERTY(double A33 READ getA33 WRITE setA33 DESIGNABLE true USER true)
    Q_PROPERTY(double A34 READ getA34 WRITE setA34 DESIGNABLE true USER true)
    Q_PROPERTY(double A41 READ getA41 WRITE setA41 DESIGNABLE true USER true)
    Q_PROPERTY(double A42 READ getA42 WRITE setA42 DESIGNABLE true USER true)
    Q_PROPERTY(double A43 READ getA43 WRITE setA43 DESIGNABLE true USER true)
    Q_PROPERTY(double A44 READ getA44 WRITE setA44 DESIGNABLE true USER true)
    TYPESYSTEM_HEADER();

    virtual QWidget* createEditor(QWidget* parent, const QObject* receiver, const char* method) const;
    virtual void setEditorData(QWidget *editor, const QVariant& data) const;
    virtual QVariant editorData(QWidget *editor) const;

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
    virtual QVariant toString(const QVariant&) const;
    virtual QVariant value(const App::Property*) const;
    virtual void setValue(const QVariant&);

protected:
    PropertyMatrixItem();
    virtual QVariant toolTip(const App::Property*) const;

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

class PlacementEditor : public Gui::LabelButton
{
    Q_OBJECT

public:
    PlacementEditor(const QString& name, QWidget * parent = 0);
    ~PlacementEditor();

private Q_SLOTS:
    void updateValue(const QVariant& v, bool, bool);

private:
    void browse();
    void showValue(const QVariant& d);

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
    Q_OBJECT
    Q_PROPERTY(double Angle READ getAngle WRITE setAngle DESIGNABLE true USER true)
    Q_PROPERTY(Base::Vector3d Axis READ getAxis WRITE setAxis DESIGNABLE true USER true)
    Q_PROPERTY(Base::Vector3d Position READ getPosition WRITE setPosition DESIGNABLE true USER true)
    TYPESYSTEM_HEADER();

    virtual QWidget* createEditor(QWidget* parent, const QObject* receiver, const char* method) const;
    virtual void setEditorData(QWidget *editor, const QVariant& data) const;
    virtual QVariant editorData(QWidget *editor) const;

    double getAngle() const;
    void setAngle(double);
    Base::Vector3d getAxis() const;
    void setAxis(const Base::Vector3d&);
    Base::Vector3d getPosition() const;
    void setPosition(const Base::Vector3d&);

protected:
    PropertyPlacementItem();
    ~PropertyPlacementItem();
    virtual QVariant toolTip(const App::Property*) const;
    virtual QVariant toString(const QVariant&) const;
    virtual QVariant value(const App::Property*) const;
    virtual void setValue(const QVariant&);

private:
    bool init_axis;
    bool changed_value;
    Base::Vector3d rot_axis;
    PropertyAngleItem * m_a;
    PropertyVectorItem* m_d;
    PropertyVectorItem* m_p;
};

/**
 * Edit properties of enum type. 
 * \author Werner Mayer
 */
class GuiExport PropertyEnumItem: public PropertyItem
{
    TYPESYSTEM_HEADER();

    virtual QWidget* createEditor(QWidget* parent, const QObject* receiver, const char* method) const;
    virtual void setEditorData(QWidget *editor, const QVariant& data) const;
    virtual QVariant editorData(QWidget *editor) const;

protected:
    virtual QVariant value(const App::Property*) const;
    virtual void setValue(const QVariant&);

protected:
    PropertyEnumItem();
};

/**
 * Edit properties of enum type. 
 * \author Werner Mayer
 */
class GuiExport PropertyStringListItem: public PropertyItem
{
    TYPESYSTEM_HEADER();

    virtual QWidget* createEditor(QWidget* parent, const QObject* receiver, const char* method) const;
    virtual void setEditorData(QWidget *editor, const QVariant& data) const;
    virtual QVariant editorData(QWidget *editor) const;

protected:
    QVariant toString(const QVariant&) const;
    virtual QVariant value(const App::Property*) const;
    virtual void setValue(const QVariant&);

protected:
    PropertyStringListItem();
};

/**
 * Change a color property.
 * \author Werner Mayer
 */
class GuiExport PropertyColorItem: public PropertyItem
{
    TYPESYSTEM_HEADER();

    virtual QWidget* createEditor(QWidget* parent, const QObject* receiver, const char* method) const;
    virtual void setEditorData(QWidget *editor, const QVariant& data) const;
    virtual QVariant editorData(QWidget *editor) const;

protected:
    virtual QVariant decoration(const App::Property*) const;
    virtual QVariant toString(const QVariant&) const;
    virtual QVariant value(const App::Property*) const;
    virtual void setValue(const QVariant&);

protected:
    PropertyColorItem();
};

/**
 * Change a file.
 * \author Werner Mayer
 */
class GuiExport PropertyFileItem: public PropertyItem
{
    TYPESYSTEM_HEADER();

    virtual QWidget* createEditor(QWidget* parent, const QObject* receiver, const char* method) const;
    virtual void setEditorData(QWidget *editor, const QVariant& data) const;
    virtual QVariant editorData(QWidget *editor) const;

protected:
    virtual QVariant value(const App::Property*) const;
    virtual void setValue(const QVariant&);

protected:
    PropertyFileItem();
    virtual QVariant toolTip(const App::Property*) const;
};

/**
 * Change a path.
 * \author Werner Mayer
 */
class GuiExport PropertyPathItem: public PropertyItem
{
    TYPESYSTEM_HEADER();

    virtual QWidget* createEditor(QWidget* parent, const QObject* receiver, const char* method) const;
    virtual void setEditorData(QWidget *editor, const QVariant& data) const;
    virtual QVariant editorData(QWidget *editor) const;

protected:
    virtual QVariant value(const App::Property*) const;
    virtual void setValue(const QVariant&);

protected:
    PropertyPathItem();
    virtual QVariant toolTip(const App::Property*) const;
};

/**
 * Show path of included file.
 * \author Werner Mayer
 */
class GuiExport PropertyTransientFileItem: public PropertyItem
{
    TYPESYSTEM_HEADER();

    virtual QWidget* createEditor(QWidget* parent, const QObject* receiver, const char* method) const;
    virtual void setEditorData(QWidget *editor, const QVariant& data) const;
    virtual QVariant editorData(QWidget *editor) const;

protected:
    virtual QVariant value(const App::Property*) const;
    virtual void setValue(const QVariant&);

protected:
    PropertyTransientFileItem();
    virtual QVariant toolTip(const App::Property*) const;
};

class LinkLabel : public QLabel
{
    Q_OBJECT

public:
    LinkLabel (QWidget * parent = 0);
    virtual ~LinkLabel();
    void setPropertyLink(const QStringList& o);
    QStringList propertyLink() const;

protected Q_SLOTS:
    void onLinkActivated(const QString&);

private:
    QStringList object;
};

/**
 * Edit properties of link type. 
 * \author Werner Mayer
 */
class GuiExport PropertyLinkItem: public PropertyItem
{
    TYPESYSTEM_HEADER();

    virtual QWidget* createEditor(QWidget* parent, const QObject* receiver, const char* method) const;
    virtual void setEditorData(QWidget *editor, const QVariant& data) const;
    virtual QVariant editorData(QWidget *editor) const;

protected:
    virtual QVariant toString(const QVariant&) const;
    virtual QVariant value(const App::Property*) const;
    virtual void setValue(const QVariant&);

protected:
    PropertyLinkItem();
};

class PropertyItemEditorFactory : public QItemEditorFactory
{
public:
    PropertyItemEditorFactory();
    virtual ~PropertyItemEditorFactory();

    virtual QWidget * createEditor ( QVariant::Type type, QWidget * parent ) const;
    virtual QByteArray valuePropertyName ( QVariant::Type type ) const;
};

} // namespace PropertyEditor
} // namespace Gui

#endif // PROPERTYEDITORITEM_H
