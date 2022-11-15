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

#ifndef PARTGUI_DLGPRIMITIVES_H
#define PARTGUI_DLGPRIMITIVES_H

#include <memory>
#include <QEventLoop>
#include <QPointer>
#include <App/DocumentObserver.h>
#include <Gui/TaskView/TaskDialog.h>

class gp_Ax2;
class SoPickedPoint;
class SoEventCallback;
class QSignalMapper;

namespace App { class Document; }
namespace Gui { class Document; }
namespace Part {
class Feature;
class Primitive;
}
namespace PartGui {

class Picker
{
public:
    Picker() : exitCode(-1)
    {
    }
    virtual ~Picker()
    {
    }

    virtual bool pickedPoint(const SoPickedPoint * point) = 0;
    virtual QString command(App::Document*) const = 0;
    void createPrimitive(QWidget* widget, const QString&, Gui::Document*);
    QString toPlacement(const gp_Ax2&) const;

    int exitCode;
    QEventLoop loop;
};

class Ui_DlgPrimitives;
class DlgPrimitives : public QWidget
{
    Q_OBJECT

public:
    explicit DlgPrimitives(QWidget* parent = nullptr, Part::Primitive* feature = nullptr);
    ~DlgPrimitives() override;
    void createPrimitive(const QString&);
    void accept(const QString&);
    void reject();

private Q_SLOTS:
    void on_buttonCircleFromThreePoints_clicked();
    void onChangePlane(QWidget*);
    void onChangeBox(QWidget*);
    void onChangeCylinder(QWidget*);
    void onChangeCone(QWidget*);
    void onChangeSphere(QWidget*);
    void onChangeEllipsoid(QWidget*);
    void onChangeTorus(QWidget*);
    void onChangePrism(QWidget*);
    void onChangeWedge(QWidget*);
    void onChangeHelix(QWidget*);
    void onChangeSpiral(QWidget*);
    void onChangeCircle(QWidget*);
    void onChangeEllipse(QWidget*);
    void onChangeVertex(QWidget*);
    void onChangeLine(QWidget*);
    void onChangeRegularPolygon(QWidget*);

private:
    QString createPlane(const QString& objectName, const QString& placement) const;
    QString createBox(const QString& objectName, const QString& placement) const;
    QString createCylinder(const QString& objectName, const QString& placement) const;
    QString createCone(const QString& objectName, const QString& placement) const;
    QString createSphere(const QString& objectName, const QString& placement) const;
    QString createEllipsoid(const QString& objectName, const QString& placement) const;
    QString createTorus(const QString& objectName, const QString& placement) const;
    QString createPrism(const QString& objectName, const QString& placement) const;
    QString createWedge(const QString& objectName, const QString& placement) const;
    QString createHelix(const QString& objectName, const QString& placement) const;
    QString createSpiral(const QString& objectName, const QString& placement) const;
    QString createCircle(const QString& objectName, const QString& placement) const;
    QString createEllipse(const QString& objectName, const QString& placement) const;
    QString createVertex(const QString& objectName, const QString& placement) const;
    QString createLine(const QString& objectName, const QString& placement) const;
    QString createRegularPolygon(const QString& objectName, const QString& placement) const;

    QString changePlane(const QString& objectName, const QString& placement) const;
    QString changeBox(const QString& objectName, const QString& placement) const;
    QString changeCylinder(const QString& objectName, const QString& placement) const;
    QString changeCone(const QString& objectName, const QString& placement) const;
    QString changeSphere(const QString& objectName, const QString& placement) const;
    QString changeEllipsoid(const QString& objectName, const QString& placement) const;
    QString changeTorus(const QString& objectName, const QString& placement) const;
    QString changePrism(const QString& objectName, const QString& placement) const;
    QString changeWedge(const QString& objectName, const QString& placement) const;
    QString changeHelix(const QString& objectName, const QString& placement) const;
    QString changeSpiral(const QString& objectName, const QString& placement) const;
    QString changeCircle(const QString& objectName, const QString& placement) const;
    QString changeEllipse(const QString& objectName, const QString& placement) const;
    QString changeVertex(const QString& objectName, const QString& placement) const;
    QString changeLine(const QString& objectName, const QString& placement) const;
    QString changeRegularPolygon(const QString& objectName, const QString& placement) const;

    static void pickCallback(void * ud, SoEventCallback * n);
    void executeCallback(Picker*);
    void connectSignalMapper(QWidget *sender, const char *signal, QSignalMapper* mapper);

private:
    std::unique_ptr<Ui_DlgPrimitives> ui;
    App::DocumentObjectWeakPtrT featurePtr;
};

class Ui_Location;
class Location : public QWidget
{
    Q_OBJECT

public:
    explicit Location(QWidget* parent = nullptr, Part::Feature* feature = nullptr);
    ~Location() override;
    QString toPlacement() const;

private Q_SLOTS:
    void onChangePosRot();
    void on_viewPositionButton_clicked();

private:
    static void pickCallback(void * ud, SoEventCallback * n);
    int mode;
    QPointer<QWidget> activeView;
    std::unique_ptr<Ui_Location> ui;
    App::DocumentObjectWeakPtrT featurePtr;
};

class TaskPrimitives : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskPrimitives();
    ~TaskPrimitives() override;

public:
    bool accept() override;
    bool reject() override;
    QDialogButtonBox::StandardButtons getStandardButtons() const override;
    void modifyStandardButtons(QDialogButtonBox*) override;

private:
    DlgPrimitives* widget;
    Location* location;
};

class TaskPrimitivesEdit : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    explicit TaskPrimitivesEdit(Part::Primitive* feature);
    ~TaskPrimitivesEdit() override;

public:
    bool accept() override;
    bool reject() override;
    QDialogButtonBox::StandardButtons getStandardButtons() const override;

private:
    DlgPrimitives* widget;
    Location* location;
};

} // namespace PartGui

#endif // PARTGUI_DLGPRIMITIVES_H
