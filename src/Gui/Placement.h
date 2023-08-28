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

#ifndef GUI_PLACEMENT_H
#define GUI_PLACEMENT_H

#include <QDialog>
#include <Base/Placement.h>
#include <Gui/SelectionObject.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>


class QSignalMapper;

namespace App {
class DocumentObject;
}
namespace Gui {
class Document;

namespace Dialog {

class Ui_Placement;
class TaskPlacement;

class GuiExport PlacementHandler : public QObject
{
    Q_OBJECT

public:
    PlacementHandler();
    void openTransactionIfNeeded();
    void setPropertyName(const std::string&);
    const std::string& getPropertyName() const;
    void appendDocument(const std::string&);
    void activatedDocument(const std::string&);
    void revertTransformation();
    void applyPlacement(const Base::Placement& p, bool incremental);
    void applyPlacement(const QString& p, bool incremental);

private:
    std::vector<App::DocumentObject*> getObjects(Gui::Document*) const;
    std::vector<App::DocumentObject*> getSelectedObjects(Gui::Document*) const;
    void revertTransformationOfViewProviders(Gui::Document*);
    void tryRecompute(Gui::Document*);
    void applyPlacement(Gui::Document*, App::DocumentObject*, const Base::Placement& p, bool incremental);
    void applyPlacement(App::DocumentObject*, const QString& p, bool incremental);
    QString getIncrementalPlacement(App::DocumentObject*, const QString&) const;
    QString getSimplePlacement(App::DocumentObject*, const QString&) const;

private Q_SLOTS:
    void openTransaction();

private:
    std::string propertyName; // the name of the placement property
    std::set<std::string> documents;
    /** If false apply the placement directly to the transform nodes,
     * otherwise change the placement property.
     */
    bool changeProperty;
};

class GuiExport Placement : public QDialog
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
    Base::Vector3d getDirection() const;
    void setPlacement(const Base::Placement&);
    Base::Placement getPlacement() const;
    void showDefaultButtons(bool);

protected:
    void changeEvent(QEvent *e) override;
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
    void setupDocument();
    void setupRotationMethod();

    bool onApply();
    void setPlacementData(const Base::Placement&);
    Base::Placement getPlacementData() const;
    Base::Rotation getRotationData() const;
    Base::Vector3d getPositionData() const;
    Base::Vector3d getAnglesData() const;
    Base::Vector3d getCenterData() const;
    Base::Vector3d getCenterOfMass() const;
    QString getPlacementString() const;
    QString getPlacementFromEulerAngles() const;
    QString getPlacementFromAxisWithAngle() const;
    void slotActiveDocument(const Gui::Document&);
    QWidget* getInvalidInput() const;
    void showErrorMessage();

Q_SIGNALS:
    void placementChanged(const QVariant &, bool, bool);

private:
    using Connection = boost::signals2::connection;
    Ui_Placement* ui;
    QSignalMapper* signalMapper;
    Connection connectAct;
    PlacementHandler handler;
    Base::Placement ref;
    Base::Vector3d cntOfMass;
    /**
     * store these so we can reselect original object
     * after user selects points and clicks Selected point(s)
     */
    std::vector<SelectionObject> selectionObjects;
};

class GuiExport DockablePlacement : public Placement
{
    Q_OBJECT

public:
    explicit DockablePlacement(QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());
    ~DockablePlacement() override;

    void accept() override;
    void reject() override;
};

class TaskPlacement : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskPlacement();
    ~TaskPlacement() override;

public:
    void setPropertyName(const QString&);
    void setPlacement(const Base::Placement&);
    void setSelection(const std::vector<SelectionObject>&);
    void bindObject();
    bool accept() override;
    bool reject() override;
    void clicked(int id) override;

    void open() override;
    bool isAllowedAlterDocument() const override
    { return true; }
    bool isAllowedAlterView() const override
    { return true; }
    bool isAllowedAlterSelection() const override
    { return true; }
    QDialogButtonBox::StandardButtons getStandardButtons() const override;

public Q_SLOTS:
    void slotPlacementChanged(const QVariant &, bool, bool);

Q_SIGNALS:
    void placementChanged(const QVariant &, bool, bool);

private:
    Placement* widget;
    Gui::TaskView::TaskBox* taskbox;
};

} // namespace Dialog
} // namespace Gui

#endif // GUI_PLACEMENT_H
