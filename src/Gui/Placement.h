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

#include <Gui/InputVector.h>
#include <Gui/SelectionObject.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>


class QSignalMapper;

namespace Gui {
class Document;

namespace Dialog {

class Ui_Placement;
class TaskPlacement;
class GuiExport Placement : public Gui::LocationDialog
{
    Q_OBJECT

public:
    explicit Placement(QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());
    ~Placement() override;
    void accept() override;
    void reject() override;

    void bindObject();
    Base::Vector3d getDirection() const override;
    void setPlacement(const Base::Placement&);
    Base::Placement getPlacement() const;
    void showDefaultButtons(bool);

protected:
    void open() override;
    void changeEvent(QEvent *e) override;
    void keyPressEvent(QKeyEvent*) override;

private Q_SLOTS:
    void openTransaction();
    void on_applyButton_clicked();
    void on_applyIncrementalPlacement_toggled(bool);
    void onPlacementChanged(int);
    void on_resetButton_clicked();
    void on_centerOfMass_toggled(bool);
    void on_selectedVertex_clicked();
    void on_applyAxial_clicked();

private:
    bool onApply();
    void setPlacementData(const Base::Placement&);
    Base::Placement getPlacementData() const;
    Base::Vector3d getCenterData() const;
    QString getPlacementString() const;
    void directionActivated(int) override;
    void applyPlacement(const Base::Placement& p, bool incremental);
    void applyPlacement(const QString& p, bool incremental);
    void revertTransformation();
    void slotActiveDocument(const Gui::Document&);
    QWidget* getInvalidInput() const;

Q_SIGNALS:
    void placementChanged(const QVariant &, bool, bool);
    void directionChanged();

private:
    typedef Gui::LocationUi<Ui_Placement> Ui_PlacementComp;
    typedef boost::signals2::connection Connection;
    Ui_PlacementComp* ui;
    QSignalMapper* signalMapper;
    Connection connectAct;
    Base::Placement ref;
    Base::Vector3d cntOfMass;
    std::string propertyName; // the name of the placement property
    std::set<std::string> documents;
    /**
     * store these so we can reselect original object
     * after user selects points and clicks Selected point(s)
     */
    std::vector<SelectionObject> selectionObjects;
    /** If false apply the placement directly to the transform nodes,
     * otherwise change the placement property.
     */
    bool changeProperty;

    friend class TaskPlacement;
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
