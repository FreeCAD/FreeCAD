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


#ifndef FEM_ViewProviderFemMeshShape_H
#define FEM_ViewProviderFemMeshShape_H

#include <Gui/ViewProviderFeaturePython.h>

#include "ViewProviderFemMesh.h"


namespace FemGui
{

class FemGuiExport ViewProviderFemMeshShapeBase: public ViewProviderFemMesh
{
    PROPERTY_HEADER_WITH_OVERRIDE(FemGui::ViewProviderFemMeshShapeBase);

public:
    /// constructor.
    ViewProviderFemMeshShapeBase();

    /// destructor.
    ~ViewProviderFemMeshShapeBase() override;
};


class FemGuiExport ViewProviderFemMeshShape: public ViewProviderFemMeshShapeBase
{
    PROPERTY_HEADER_WITH_OVERRIDE(FemGui::ViewProviderFemMeshShape);

public:
    /// constructor.
    ViewProviderFemMeshShape();

    /// destructor.
    ~ViewProviderFemMeshShape() override;
};

using ViewProviderFemMeshShapeBasePython =
    Gui::ViewProviderFeaturePythonT<ViewProviderFemMeshShapeBase>;

}  // namespace FemGui


#endif  // FEM_ViewProviderFemMeshShape_H
