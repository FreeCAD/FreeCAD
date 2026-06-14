/***************************************************************************
 *   Copyright (c) 2013 Jürgen Riegel <FreeCAD@juergen-riegel.net>         *
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

#include <Gui/TaskView/TaskView.h>


class Ui_TaskTetParameter;
class SoEventCallback;

namespace Base
{
class Polygon2d;
}
namespace App
{
class Property;
}

namespace Gui
{
class ViewProvider;
class ViewVolumeProjection;
}  // namespace Gui
namespace Fem
{
class FemMeshShapeNetgenObject;
}

namespace FemGui
{

class ViewProviderFemMeshShapeNetgen;


class TaskTetParameter: public Gui::TaskView::TaskBox
{
    Q_OBJECT

public:
    explicit TaskTetParameter(Fem::FemMeshShapeNetgenObject* pcObject, QWidget* parent = nullptr);
    ~TaskTetParameter() override;

    void setInfo();
    bool touched;

private Q_SLOTS:
    void SwitchMethod(int Value);
    void maxSizeValueChanged(double Value);
    void minSizeValueChanged(double Value);
    void setQuadric(int s);
    void setGrowthRate(double v);
    void setSegsPerEdge(int v);
    void setSegsPerRadius(int v);
    void setOptimize(int v);

protected:
    Fem::FemMeshShapeNetgenObject* pcObject;

private:
    QWidget* proxy;
    std::unique_ptr<Ui_TaskTetParameter> ui;
};

}  // namespace FemGui
