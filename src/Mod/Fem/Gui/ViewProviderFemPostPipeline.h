/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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


#ifndef FEM_VIEWPROVIDERFEMPOSTPIPELINE_H
#define FEM_VIEWPROVIDERFEMPOSTPIPELINE_H

#include "ViewProviderFemPostObject.h"

namespace FemGui
{

class FemGuiExport ViewProviderFemPostPipeline : public ViewProviderFemPostObject {

    PROPERTY_HEADER(FemGui::ViewProviderFemPostPipeline);

public:
    /// constructor.
    ViewProviderFemPostPipeline();
    ~ViewProviderFemPostPipeline();

    virtual std::vector< App::DocumentObject* > claimChildren(void) const;
    virtual std::vector< App::DocumentObject* > claimChildren3D(void) const;
    virtual void updateData(const App::Property* prop);

protected:
    void updateFunctionSize();

};

} //namespace FemGui


#endif // FEM_VIEWPROVIDERFEMPOSTPIPELINE_H
