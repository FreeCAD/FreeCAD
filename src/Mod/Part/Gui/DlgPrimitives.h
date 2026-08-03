// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2007 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <memory>
#include <QEventLoop>
#include <QPointer>
#include <App/DocumentObserver.h>
#include <Gui/TaskView/TaskDialog.h>

class gp_Ax2;
class SoPickedPoint;
class SoEventCallback;
class QSignalMapper;

namespace App
{
class Document;
}
namespace Gui
{
class Document;
}
namespace Part
{
class Feature;
class Primitive;
class Plane;
class Box;
class Cylinder;
class Cone;
class Sphere;
class Ellipsoid;
class Torus;
class Prism;
class Wedge;
class Helix;
class Spiral;
class Circle;
class Ellipse;
class Vertex;
class Line;
class RegularPolygon;
}  // namespace Part
namespace PartGui
{

class Picker
{
public:
    virtual ~Picker() = default;

    virtual bool pickedPoint(const SoPickedPoint* point) = 0;
    virtual QString command(App::Document*) const = 0;
    void createPrimitive(QWidget* widget, const QString&, Gui::Document*);
    QString toPlacement(const gp_Ax2&) const;

    int exitCode {-1};
    QEventLoop loop;
};

class Ui_DlgPrimitives;

class AbstractPrimitive: public QObject
{
    Q_OBJECT

public:
    AbstractPrimitive(Part::Primitive* feature = nullptr);
    ~AbstractPrimitive() override = default;

    bool hasValidPrimitive() const;
    virtual const char* getDefaultName() const = 0;
    virtual QString create(const QString& objectName, const QString& placement) const = 0;
    virtual QString change(const QString& objectName, const QString& placement) const = 0;
    virtual void changeValue(QObject*) = 0;

protected:
    void connectSignalMapper(QSignalMapper* mapper);

protected:
    App::DocumentObjectWeakPtrT featurePtr;
};

// ----------------------------------------------------------------------------

class PlanePrimitive: public AbstractPrimitive
{
    Q_OBJECT

public:
    PlanePrimitive(std::shared_ptr<Ui_DlgPrimitives> ui, Part::Plane* feature = nullptr);

    const char* getDefaultName() const override;
    QString create(const QString& objectName, const QString& placement) const override;
    QString change(const QString& objectName, const QString& placement) const override;
    void changeValue(QObject*) override;

private:
    std::shared_ptr<Ui_DlgPrimitives> ui;
};

// ----------------------------------------------------------------------------

class BoxPrimitive: public AbstractPrimitive
{
    Q_OBJECT

public:
    BoxPrimitive(std::shared_ptr<Ui_DlgPrimitives> ui, Part::Box* feature = nullptr);

    const char* getDefaultName() const override;
    QString create(const QString& objectName, const QString& placement) const override;
    QString change(const QString& objectName, const QString& placement) const override;
    void changeValue(QObject*) override;

private:
    std::shared_ptr<Ui_DlgPrimitives> ui;
};

// ----------------------------------------------------------------------------

class CylinderPrimitive: public AbstractPrimitive
{
    Q_OBJECT

public:
    CylinderPrimitive(std::shared_ptr<Ui_DlgPrimitives> ui, Part::Cylinder* feature = nullptr);

    const char* getDefaultName() const override;
    QString create(const QString& objectName, const QString& placement) const override;
    QString change(const QString& objectName, const QString& placement) const override;
    void changeValue(QObject*) override;

private:
    std::shared_ptr<Ui_DlgPrimitives> ui;
};

// ----------------------------------------------------------------------------

class ConePrimitive: public AbstractPrimitive
{
    Q_OBJECT

public:
    ConePrimitive(std::shared_ptr<Ui_DlgPrimitives> ui, Part::Cone* feature = nullptr);

    const char* getDefaultName() const override;
    QString create(const QString& objectName, const QString& placement) const override;
    QString change(const QString& objectName, const QString& placement) const override;
    void changeValue(QObject*) override;

private:
    std::shared_ptr<Ui_DlgPrimitives> ui;
};

// ----------------------------------------------------------------------------

class SpherePrimitive: public AbstractPrimitive
{
    Q_OBJECT

public:
    SpherePrimitive(std::shared_ptr<Ui_DlgPrimitives> ui, Part::Sphere* feature = nullptr);

    const char* getDefaultName() const override;
    QString create(const QString& objectName, const QString& placement) const override;
    QString change(const QString& objectName, const QString& placement) const override;
    void changeValue(QObject*) override;

private:
    std::shared_ptr<Ui_DlgPrimitives> ui;
};

// ----------------------------------------------------------------------------

class EllipsoidPrimitive: public AbstractPrimitive
{
    Q_OBJECT

public:
    EllipsoidPrimitive(std::shared_ptr<Ui_DlgPrimitives> ui, Part::Ellipsoid* feature = nullptr);

    const char* getDefaultName() const override;
    QString create(const QString& objectName, const QString& placement) const override;
    QString change(const QString& objectName, const QString& placement) const override;
    void changeValue(QObject*) override;

private:
    std::shared_ptr<Ui_DlgPrimitives> ui;
};

// ----------------------------------------------------------------------------

class TorusPrimitive: public AbstractPrimitive
{
    Q_OBJECT

public:
    TorusPrimitive(std::shared_ptr<Ui_DlgPrimitives> ui, Part::Torus* feature = nullptr);

    const char* getDefaultName() const override;
    QString create(const QString& objectName, const QString& placement) const override;
    QString change(const QString& objectName, const QString& placement) const override;
    void changeValue(QObject*) override;

private:
    std::shared_ptr<Ui_DlgPrimitives> ui;
};

// ----------------------------------------------------------------------------

class PrismPrimitive: public AbstractPrimitive
{
    Q_OBJECT

public:
    PrismPrimitive(std::shared_ptr<Ui_DlgPrimitives> ui, Part::Prism* feature = nullptr);

    const char* getDefaultName() const override;
    QString create(const QString& objectName, const QString& placement) const override;
    QString change(const QString& objectName, const QString& placement) const override;
    void changeValue(QObject*) override;

private:
    std::shared_ptr<Ui_DlgPrimitives> ui;
};

// ----------------------------------------------------------------------------

class WedgePrimitive: public AbstractPrimitive
{
    Q_OBJECT

public:
    WedgePrimitive(std::shared_ptr<Ui_DlgPrimitives> ui, Part::Wedge* feature = nullptr);

    const char* getDefaultName() const override;
    QString create(const QString& objectName, const QString& placement) const override;
    QString change(const QString& objectName, const QString& placement) const override;
    void changeValue(QObject*) override;

private:
    std::shared_ptr<Ui_DlgPrimitives> ui;
};

// ----------------------------------------------------------------------------

class HelixPrimitive: public AbstractPrimitive
{
    Q_OBJECT

public:
    HelixPrimitive(std::shared_ptr<Ui_DlgPrimitives> ui, Part::Helix* feature = nullptr);

    const char* getDefaultName() const override;
    QString create(const QString& objectName, const QString& placement) const override;
    QString change(const QString& objectName, const QString& placement) const override;
    void changeValue(QObject*) override;

private:
    std::shared_ptr<Ui_DlgPrimitives> ui;
};

// ----------------------------------------------------------------------------

class SpiralPrimitive: public AbstractPrimitive
{
    Q_OBJECT

public:
    SpiralPrimitive(std::shared_ptr<Ui_DlgPrimitives> ui, Part::Spiral* feature = nullptr);

    const char* getDefaultName() const override;
    QString create(const QString& objectName, const QString& placement) const override;
    QString change(const QString& objectName, const QString& placement) const override;
    void changeValue(QObject*) override;

private:
    std::shared_ptr<Ui_DlgPrimitives> ui;
};

// ----------------------------------------------------------------------------

class CirclePrimitive: public AbstractPrimitive
{
    Q_OBJECT

public:
    CirclePrimitive(std::shared_ptr<Ui_DlgPrimitives> ui, Part::Circle* feature = nullptr);

    const char* getDefaultName() const override;
    QString create(const QString& objectName, const QString& placement) const override;
    QString change(const QString& objectName, const QString& placement) const override;
    void changeValue(QObject*) override;

private:
    std::shared_ptr<Ui_DlgPrimitives> ui;
};

// ----------------------------------------------------------------------------

class EllipsePrimitive: public AbstractPrimitive
{
    Q_OBJECT

public:
    EllipsePrimitive(std::shared_ptr<Ui_DlgPrimitives> ui, Part::Ellipse* feature = nullptr);

    const char* getDefaultName() const override;
    QString create(const QString& objectName, const QString& placement) const override;
    QString change(const QString& objectName, const QString& placement) const override;
    void changeValue(QObject*) override;

private:
    std::shared_ptr<Ui_DlgPrimitives> ui;
};

// ----------------------------------------------------------------------------

class PolygonPrimitive: public AbstractPrimitive
{
    Q_OBJECT

public:
    PolygonPrimitive(std::shared_ptr<Ui_DlgPrimitives> ui, Part::RegularPolygon* feature = nullptr);

    const char* getDefaultName() const override;
    QString create(const QString& objectName, const QString& placement) const override;
    QString change(const QString& objectName, const QString& placement) const override;
    void changeValue(QObject*) override;

private:
    std::shared_ptr<Ui_DlgPrimitives> ui;
};

// ----------------------------------------------------------------------------

class LinePrimitive: public AbstractPrimitive
{
    Q_OBJECT

public:
    LinePrimitive(std::shared_ptr<Ui_DlgPrimitives> ui, Part::Line* feature = nullptr);

    const char* getDefaultName() const override;
    QString create(const QString& objectName, const QString& placement) const override;
    QString change(const QString& objectName, const QString& placement) const override;
    void changeValue(QObject*) override;

private:
    std::shared_ptr<Ui_DlgPrimitives> ui;
};

// ----------------------------------------------------------------------------

class VertexPrimitive: public AbstractPrimitive
{
    Q_OBJECT

public:
    VertexPrimitive(std::shared_ptr<Ui_DlgPrimitives> ui, Part::Vertex* feature = nullptr);

    const char* getDefaultName() const override;
    QString create(const QString& objectName, const QString& placement) const override;
    QString change(const QString& objectName, const QString& placement) const override;
    void changeValue(QObject*) override;

private:
    std::shared_ptr<Ui_DlgPrimitives> ui;
};

// ----------------------------------------------------------------------------

class DlgPrimitives: public QWidget
{
    Q_OBJECT

public:
    explicit DlgPrimitives(QWidget* parent = nullptr, Part::Primitive* feature = nullptr);
    ~DlgPrimitives() override;
    void createPrimitive(const QString&);
    void accept(const QString&);
    void reject();

private:
    void buttonCircleFromThreePoints();

private:
    static void pickCallback(void* ud, SoEventCallback* n);
    void executeCallback(Picker*);
    void acceptChanges(const QString&);
    void tryCreatePrimitive(const QString&);

    void addPrimitive(std::shared_ptr<AbstractPrimitive>);
    std::shared_ptr<AbstractPrimitive> getPrimitive(int index) const;
    int findIndexOfValidPrimitive() const;
    void activatePage();

private:
    using AbstractPrimitivePtr = std::shared_ptr<AbstractPrimitive>;
    std::vector<AbstractPrimitivePtr> primitive;
    std::shared_ptr<Ui_DlgPrimitives> ui;
    App::DocumentObjectWeakPtrT featurePtr;
};

class Ui_Location;
class Location: public QWidget
{
    Q_OBJECT

public:
    explicit Location(QWidget* parent = nullptr, Part::Feature* feature = nullptr);
    ~Location() override;
    QString toPlacement() const;

private:
    void onPlacementChanged();
    void onViewPositionButton();

private:
    void setPlacement(Part::Feature* feature);
    void bindExpressions(Part::Feature* feature);
    void connectSignals();
    static void pickCallback(void* ud, SoEventCallback* n);

    int mode;
    QPointer<QWidget> activeView;
    std::unique_ptr<Ui_Location> ui;
    App::DocumentObjectWeakPtrT featurePtr;
};

class TaskPrimitives: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskPrimitives();

public:
    bool accept() override;
    bool reject() override;
    QDialogButtonBox::StandardButtons getStandardButtons() const override;
    void modifyStandardButtons(QDialogButtonBox*) override;

private:
    DlgPrimitives* widget;
    Location* location;
};

class TaskPrimitivesEdit: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    explicit TaskPrimitivesEdit(Part::Primitive* feature);

public:
    bool accept() override;
    bool reject() override;
    QDialogButtonBox::StandardButtons getStandardButtons() const override;

private:
    DlgPrimitives* widget;
    Location* location;
};

}  // namespace PartGui
