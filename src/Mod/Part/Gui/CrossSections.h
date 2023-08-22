/***************************************************************************
 *   Copyright (c) 2010 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef PARTGUI_CROSSSECTIONS_H
#define PARTGUI_CROSSSECTIONS_H

#include <QDialog>
#include <QPointer>

#include <Base/BoundBox.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>


namespace Gui {
class View3DInventor;
}

namespace PartGui {

class ViewProviderCrossSections;
class Ui_CrossSections;
class CrossSections : public QDialog
{
    Q_OBJECT

    enum Plane { XY, XZ, YZ };

public:
    explicit CrossSections(const Base::BoundBox3d& bb, QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());
    ~CrossSections() override;
    void accept() override;
    void apply();

protected:
    void changeEvent(QEvent *e) override;
    void keyPressEvent(QKeyEvent*) override;

private:
    void setupConnections();
    void xyPlaneClicked();
    void xzPlaneClicked();
    void yzPlaneClicked();
    void positionValueChanged(double);
    void distanceValueChanged(double);
    void countSectionsValueChanged(int);
    void checkBothSidesToggled(bool);
    void sectionsBoxToggled(bool);

private:
    std::vector<double> getPlanes() const;
    void calcPlane(Plane, double);
    void calcPlanes(Plane/*, double, bool, int*/);
    void makePlanes(Plane, const std::vector<double>&, double[4]);
    Plane plane() const;

private:
    std::unique_ptr<Ui_CrossSections> ui;
    Base::BoundBox3d bbox;
    ViewProviderCrossSections* vp;
    QPointer<Gui::View3DInventor> view;
};

class TaskCrossSections : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    explicit TaskCrossSections(const Base::BoundBox3d& bb);

public:
    bool accept() override;
    void clicked(int id) override;

    QDialogButtonBox::StandardButtons getStandardButtons() const override
    { return QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel; }

private:
    CrossSections* widget;
    Gui::TaskView::TaskBox* taskbox;
};

} // namespace PartGui

#endif // PARTGUI_CROSSSECTIONS_H
