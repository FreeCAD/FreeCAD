/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <QDialog>
#include <Base/Placement.h>
#include <Gui/Selection/SelectionObject.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>


class QSignalMapper;

namespace App
{
class DocumentObject;
}
namespace Gui
{
class Document;

namespace Dialog
{

class Ui_Placement;
class TaskPlacement;

class GuiExport PlacementHandler: public QObject
{
    Q_OBJECT

public:
    PlacementHandler();
    void openTransactionIfNeeded();
    void setPropertyName(const std::string&);
    void setIgnoreTransactions(bool value);
    void setSelection(const std::vector<SelectionObject>&);
    void reselectObjects();
    const App::DocumentObject* getFirstOfSelection() const;
    const std::string& getPropertyName() const;
    void appendDocument(const std::string&);
    void activatedDocument(const std::string&);
    void revertTransformation();
    void setRefPlacement(const Base::Placement& plm);
    const Base::Placement& getRefPlacement() const;
    void applyPlacement(const Base::Placement& p, bool incremental);
    void applyPlacement(const QString& p, bool incremental);
    Base::Vector3d computeCenterOfMass() const;
    void setCenterOfMass(const Base::Vector3d& pnt);
    Base::Vector3d getCenterOfMass() const;
    std::tuple<Base::Vector3d, std::vector<Base::Vector3d>> getSelectedPoints() const;

private:
    std::vector<const App::DocumentObject*> getObjects(const Gui::Document*) const;
    std::vector<const App::DocumentObject*> getSelectedObjects(const Gui::Document*) const;
    void revertTransformationOfViewProviders(Gui::Document*);
    void tryRecompute(Gui::Document*);
    void applyPlacement(
        const Gui::Document*,
        const App::DocumentObject*,
        const Base::Placement& p,
        bool incremental
    );
    void applyPlacement(const App::DocumentObject*, const QString& p, bool incremental);
    QString getIncrementalPlacement(const App::DocumentObject*, const QString&) const;
    QString getSimplePlacement(const App::DocumentObject*, const QString&) const;
    void setupDocument();
    void slotActiveDocument(const Gui::Document&);
    void openCommandIfActive(Gui::Document*);
    void commitCommandIfActive(Gui::Document*);
    void abortCommandIfActive(Gui::Document*);

private Q_SLOTS:
    void openTransaction();

private:
    using Connection = fastsignals::scoped_connection;
    std::string propertyName;  // the name of the placement property
    std::set<std::string> documents;
    /** If false apply the placement directly to the transform nodes,
     * otherwise change the placement property.
     */
    bool changeProperty;
    /** If true do not open or commit transactions. In this case it's expected
     *  that it's done by the calling instance.
     */
    bool ignoreTransaction;
    Connection connectAct;
    /**
     * store these so we can reselect original object
     * after user selects points and clicks Selected point(s)
     */
    std::vector<SelectionObject> selectionObjects;
    Base::Placement ref;
    Base::Vector3d cntOfMass;
};

class GuiExport Placement: public QDialog
{
    Q_OBJECT

public:
    explicit Placement(QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());
    ~Placement() override;
    void open() override;
    void accept() override;
    void reject() override;

    void setPropertyName(const std::string&);
    void setSelection(const std::vector<SelectionObject>&);
    void bindObject();
    void setPlacementAndBindObject(const App::DocumentObject* obj, const std::string& propertyName);
    void setIgnoreTransactions(bool value);
    Base::Vector3d getDirection() const;
    void setPlacement(const Base::Placement&);
    Base::Placement getPlacement() const;
    void showDefaultButtons(bool);

protected:
    void changeEvent(QEvent* e) override;
    void keyPressEvent(QKeyEvent*) override;

public Q_SLOTS:
    void onApplyButtonClicked();

private Q_SLOTS:
    void onApplyIncrementalPlacementToggled(bool);
    void onPlacementChanged(int);
    void onResetButtonClicked();
    void onCenterOfMassToggled(bool);
    void onSelectedVertexClicked();
    void onApplyAxialClicked();

private:
    void setupUi();
    void setupConnections();
    void setupUnits();
    void setupSignalMapper();
    void setupRotationMethod();
    void bindProperty(const App::DocumentObject* obj, const std::string& propertyName);

    bool onApply();
    void setPlacementData(const Base::Placement&);
    Base::Placement getPlacementData() const;
    Base::Rotation getRotationData() const;
    Base::Vector3d getPositionData() const;
    Base::Vector3d getAnglesData() const;
    Base::Vector3d getCenterData() const;
    QString getPlacementString() const;
    QString getPlacementFromEulerAngles() const;
    QString getPlacementFromAxisWithAngle() const;
    QWidget* getInvalidInput() const;
    void showErrorMessage();

Q_SIGNALS:
    void placementChanged(const QVariant&, bool, bool);

private:
    Ui_Placement* ui;
    QSignalMapper* signalMapper;
    PlacementHandler handler;
};

class GuiExport DockablePlacement: public Placement
{
    Q_OBJECT

public:
    explicit DockablePlacement(QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());
    ~DockablePlacement() override;

    void accept() override;
    void reject() override;
};

class TaskPlacement: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskPlacement();
    ~TaskPlacement() override;

public:
    void setPropertyName(const QString&);
    void setPlacement(const Base::Placement&);
    void setSelection(const std::vector<SelectionObject>&);
    void clearSelection();
    void bindObject();
    void setPlacementAndBindObject(const App::DocumentObject* obj, const std::string& propertyName);
    bool accept() override;
    bool reject() override;
    void clicked(int id) override;

    void open() override;
    bool isAllowedAlterDocument() const override
    {
        return true;
    }
    bool isAllowedAlterView() const override
    {
        return true;
    }
    bool isAllowedAlterSelection() const override
    {
        return true;
    }
    QDialogButtonBox::StandardButtons getStandardButtons() const override;

public Q_SLOTS:
    void slotPlacementChanged(const QVariant&, bool, bool);

Q_SIGNALS:
    void placementChanged(const QVariant&, bool, bool);

private:
    Placement* widget;
};

class TaskPlacementPy: public Py::PythonExtension<TaskPlacementPy>
{
public:
    using BaseType = Py::PythonExtension<TaskPlacementPy>;
    static void init_type();

    TaskPlacementPy();
    ~TaskPlacementPy() override;

    Py::Object repr() override;
    Py::Object getattr(const char* name) override;
    int setattr(const char* name, const Py::Object&) override;

    Py::Object setPropertyName(const Py::Tuple&);
    Py::Object setPlacement(const Py::Tuple&);
    Py::Object setSelection(const Py::Tuple&);
    Py::Object bindObject(const Py::Tuple&);
    Py::Object setPlacementAndBindObject(const Py::Tuple&);
    Py::Object setIgnoreTransactions(const Py::Tuple&);

    Py::Object showDefaultButtons(const Py::Tuple&);
    Py::Object accept(const Py::Tuple&);
    Py::Object reject(const Py::Tuple&);
    Py::Object clicked(const Py::Tuple&);
    Py::Object open(const Py::Tuple&);
    Py::Object isAllowedAlterDocument(const Py::Tuple&);
    Py::Object isAllowedAlterView(const Py::Tuple&);
    Py::Object isAllowedAlterSelection(const Py::Tuple&);
    Py::Object getStandardButtons(const Py::Tuple&);

private:
    static PyObject* PyMake(struct _typeobject*, PyObject*, PyObject*);

private:
    QPointer<Placement> widget;
};

}  // namespace Dialog
}  // namespace Gui
