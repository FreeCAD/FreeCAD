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
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Base/Placement.h>

#include <boost/signals.hpp>
#include <boost/bind.hpp>

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
    Placement(QWidget* parent = 0, Qt::WFlags fl = 0);
    ~Placement();
    void accept();
    void reject();

    Base::Vector3d getDirection() const;
    void setPlacement(const Base::Placement&);
    Base::Placement getPlacement() const;
    void showDefaultButtons(bool);

protected:
    void changeEvent(QEvent *e);

private Q_SLOTS:
    void on_applyButton_clicked();
    void on_applyIncrementalPlacement_toggled(bool);
    void onPlacementChanged(int);
    void on_resetButton_clicked();

private:
    void setPlacementData(const Base::Placement&);
    Base::Placement getPlacementData() const;
    void directionActivated(int);
    void applyPlacement(const Base::Placement& p, bool incremental, bool data);
    void revertTransformation();
    void slotActiveDocument(const Gui::Document&);

Q_SIGNALS:
    void placementChanged(const QVariant &, bool, bool);
    void directionChanged();

private:
    typedef Gui::LocationInterfaceComp<Ui_Placement> Ui_PlacementComp;
    typedef boost::BOOST_SIGNALS_NAMESPACE::connection Connection;
    Ui_PlacementComp* ui;
    QSignalMapper* signalMapper;
    Connection connectAct;
    Base::Placement ref;
    std::string propertyName; // the name of the placement property
    std::set<std::string> documents;

    friend class TaskPlacement;
};

class GuiExport DockablePlacement : public Placement
{
    Q_OBJECT

public:
    DockablePlacement(QWidget* parent = 0, Qt::WFlags fl = 0);
    ~DockablePlacement();

    void accept();
    void reject();
};

class TaskPlacement : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskPlacement();
    ~TaskPlacement();

public:
    void setPropertyName(const QString&);
    void setPlacement(const Base::Placement&);
    bool accept();
    bool reject();
    void clicked(int id);

    bool isAllowedAlterDocument(void) const
    { return true; }
    bool isAllowedAlterView(void) const
    { return true; }
    bool isAllowedAlterSelection(void) const
    { return true; }
    QDialogButtonBox::StandardButtons getStandardButtons() const;

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
