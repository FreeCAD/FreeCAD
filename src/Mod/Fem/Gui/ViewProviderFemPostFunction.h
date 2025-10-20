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

#ifndef FEM_VIEWPROVIDERFEMPOSTFUNCTION_H
#define FEM_VIEWPROVIDERFEMPOSTFUNCTION_H

#include <QWidget>
#include <boost/signals2.hpp>

#include <Gui/ViewProviderDocumentObjectGroup.h>
#include "ViewProviderShapeExtension.h"

namespace FemGui
{

class ViewProviderFemPostFunction;

class FemGuiExport ViewProviderFemPostFunctionProvider: public Gui::ViewProviderDocumentObjectGroup
{
    PROPERTY_HEADER_WITH_OVERRIDE(FemGui::ViewProviderFemPostFunctionProvider);

public:
    ViewProviderFemPostFunctionProvider();
    ~ViewProviderFemPostFunctionProvider() override;

    App::PropertyFloat SizeX;
    App::PropertyFloat SizeY;
    App::PropertyFloat SizeZ;

    // handling when object is deleted
    bool onDelete(const std::vector<std::string>&) override;
    /// asks view provider if the given object can be deleted
    bool canDelete(App::DocumentObject* obj) const override;

protected:
    void onChanged(const App::Property* prop) override;
    void updateData(const App::Property*) override;

    void updateSize();
};

class FemGuiExport ViewProviderFemPostFunction: public Gui::ViewProviderDocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(FemGui::ViewProviderFemPostFunction);

public:
    /// constructor.
    ViewProviderFemPostFunction();
    ~ViewProviderFemPostFunction() override;

    App::PropertyFloat AutoScaleFactorX;
    App::PropertyFloat AutoScaleFactorY;
    App::PropertyFloat AutoScaleFactorZ;

    bool doubleClicked() override;
    std::vector<std::string> getDisplayModes() const override;

    virtual ShapeWidget* createControlWidget();

protected:
    bool setEdit(int ModNum) override;
    void unsetEdit(int ModNum) override;
    void onChanged(const App::Property* prop) override;

    void setAutoScale(bool value)
    {
        m_autoscale = value;
    }
    bool autoScale()
    {
        return m_autoscale;
    }

private:
    bool m_autoscale;
};

// ***************************************************************************
class FemGuiExport ViewProviderFemPostBoxFunction: public ViewProviderFemPostFunction, public ViewProviderBoxExtension
{
    PROPERTY_HEADER_WITH_EXTENSIONS(FemGui::ViewProviderFemPostBoxFunction);

public:
    ViewProviderFemPostBoxFunction();
    ~ViewProviderFemPostBoxFunction() override;
};


// ***************************************************************************
class FemGuiExport ViewProviderFemPostCylinderFunction: public ViewProviderFemPostFunction, public ViewProviderCylinderExtension
{
    PROPERTY_HEADER_WITH_EXTENSIONS(FemGui::ViewProviderFemPostCylinderFunction);

public:
    ViewProviderFemPostCylinderFunction();
    ~ViewProviderFemPostCylinderFunction() override;
};




class FemGuiExport ViewProviderFemPostPlaneFunction: public ViewProviderFemPostFunction, public ViewProviderPlaneExtension
{
    PROPERTY_HEADER_WITH_EXTENSIONS(FemGui::ViewProviderFemPostPlaneFunction);

public:
    ViewProviderFemPostPlaneFunction();
    ~ViewProviderFemPostPlaneFunction() override;
};


// ***************************************************************************
class FemGuiExport ViewProviderFemPostSphereFunction: public ViewProviderFemPostFunction, public ViewProviderSphereExtension
{
    PROPERTY_HEADER_WITH_EXTENSIONS(FemGui::ViewProviderFemPostSphereFunction);

public:
    ViewProviderFemPostSphereFunction();
    ~ViewProviderFemPostSphereFunction() override;
};


}  // namespace FemGui


#endif  // FEM_VIEWPROVIDERFEMPOSTFUNCTION_H
