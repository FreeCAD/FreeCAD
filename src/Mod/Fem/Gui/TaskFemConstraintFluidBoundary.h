/***************************************************************************
 *   Copyright (c) 2016 Qingfeng Xia <qingfeng.xia    iesensor.com>        *
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

#ifndef GUI_TASKVIEW_TaskFemConstraintFluidBoundary_H
#define GUI_TASKVIEW_TaskFemConstraintFluidBoundary_H

#include <QObject>

#include <App/PropertyStandard.h>
#include <Mod/Fem/App/FemSolverObject.h>

#include "TaskFemConstraintOnBoundary.h"
#include "ViewProviderFemConstraintFluidBoundary.h"


class Ui_TaskFemConstraintFluidBoundary;

namespace App {
class Property;
}

namespace Gui {
class ViewProvider;
}

namespace FemGui {

class TaskFemConstraintFluidBoundary : public TaskFemConstraintOnBoundary
{
    Q_OBJECT

public:
    TaskFemConstraintFluidBoundary(ViewProviderFemConstraintFluidBoundary *ConstraintView,QWidget *parent = nullptr);
    virtual ~TaskFemConstraintFluidBoundary();

    const Fem::FemSolverObject* getFemSolver(void) const;

    std::string getBoundaryType(void) const;
    std::string getSubtype(void) const;
    double getBoundaryValue(void) const;

    std::string getTurbulenceModel(void) const;
    std::string getTurbulenceSpecification(void) const;
    double getTurbulentIntensityValue(void) const;
    double getTurbulentLengthValue(void) const;

    bool getHeatTransferring(void) const;
    std::string getThermalBoundaryType(void) const;
    double getTemperatureValue(void) const;
    double getHeatFluxValue(void) const;
    double getHTCoeffValue(void) const;

    const std::string getReferences() const;
    const std::string getDirectionName(void) const;
    const std::string getDirectionObject(void) const;
    bool getReverse(void) const;

private Q_SLOTS:
    void onBoundaryTypeChanged(void);
    void onSubtypeChanged(void);
    void onBoundaryValueChanged(double);
    void onTurbulenceSpecificationChanged(void);
    void onThermalBoundaryTypeChanged(void);
    void onReferenceDeleted(void);
    void onButtonDirection(const bool pressed = true);
    void onCheckReverse(bool); // consider removing this slot as the UI is hidden
    void addToSelection();
    void removeFromSelection();

protected:
    bool event(QEvent *e);
    virtual void changeEvent(QEvent *e);
    void clearButtons(const SelectionChangeModes notThis) override;

private:
    void updateBoundaryTypeUI();
    void updateSubtypeUI();
    void updateThermalBoundaryUI();
    void updateTurbulenceUI();
    void updateUI();

private:
    Ui_TaskFemConstraintFluidBoundary* ui;
    int dimension;  // -1: unknown, 2 for 2D and 3 for 3D
    Fem::FemSolverObject* pcSolver;
    App::PropertyBool* pHeatTransferring;
    App::PropertyEnumeration* pTurbulenceModel;
};

/// simulation dialog for the TaskView
class TaskDlgFemConstraintFluidBoundary : public TaskDlgFemConstraint
{
    Q_OBJECT

public:
    TaskDlgFemConstraintFluidBoundary(ViewProviderFemConstraintFluidBoundary *ConstraintView);

    /// is called by the framework if the dialog is accepted (Ok)
    virtual void open();
    virtual bool accept();
    virtual bool reject();

};

} //namespace FemGui

#endif // GUI_TASKVIEW_TaskFemConstraintFluidBoundary_H
