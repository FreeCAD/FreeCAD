/***************************************************************************
 *   Copyright (c) 2013 Jürgen Riegel (FreeCAD@juergen-riegel.net)         *
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


#ifndef FEM_ViewProviderFemMeshShapeNetgen_H
#define FEM_ViewProviderFemMeshShapeNetgen_H

#include "ViewProviderFemMeshShape.h"
#include <Gui/ViewProviderBuilder.h>

class SoCoordinate3;
class SoDrawStyle;  
class SoIndexedFaceSet; 
class SoIndexedLineSet; 
class SoShapeHints;
class SoMaterialBinding;

namespace FemGui
{


class FemGuiExport ViewProviderFemMeshShapeNetgen : public ViewProviderFemMeshShape
{
    PROPERTY_HEADER(FemGui::ViewProviderFemMeshShapeNetgen);

public:
    /// constructor.
    ViewProviderFemMeshShapeNetgen();

    /// destructor.
    ~ViewProviderFemMeshShapeNetgen();

     virtual void updateData(const App::Property*);

protected:
    virtual void setupContextMenu(QMenu* menu, QObject* receiver, const char* member);
    virtual bool setEdit(int ModNum);

};

} //namespace FemGui


#endif // FEM_ViewProviderFemMeshShapeNetgen_H
