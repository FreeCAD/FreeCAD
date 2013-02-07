/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinländer <jrheinlaender[at]users.sourceforge.net>     *
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


#ifndef GUI_VIEWPROVIDERFEMCONSTRAINT_H
#define GUI_VIEWPROVIDERFEMCONSTRAINT_H

#include <TopoDS_Shape.hxx>

#include "Gui/ViewProviderGeometryObject.h"
#include <QObject>

class SoFontStyle;
class SoText2;
class SoBaseColor;
class SoTranslation;
class SoCoordinate3;
class SoIndexedLineSet;
class SoIndexedFaceSet;
class SoEventCallback;
class SoMarkerSet;

namespace Gui  {
class View3DInventorViewer;
}

namespace FemGui
{

class FemGuiExport ViewProviderFemConstraint : public Gui::ViewProviderGeometryObject
{
    PROPERTY_HEADER(FemGui::ViewProviderFemConstraint);

public:
    /// Constructor
    ViewProviderFemConstraint(void);
    virtual ~ViewProviderFemConstraint();

    // Display properties
    App::PropertyColor          TextColor;
    App::PropertyColor          FaceColor;
    App::PropertyInteger        FontSize;
    App::PropertyFloat          DistFactor;
    App::PropertyBool           Mirror;

    void attach(App::DocumentObject *);
    void updateData(const App::Property*);
    std::vector<std::string> getDisplayModes(void) const;
    void setDisplayMode(const char* ModeName);

    std::vector<App::DocumentObject*> claimChildren(void)const;
    void setupContextMenu(QMenu*, QObject*, const char*);

protected:
    void onChanged(const App::Property* prop);
    virtual bool setEdit(int ModNum);
    virtual void unsetEdit(int ModNum);

private:
    SoFontStyle      * pFont;
    SoText2          * pLabel;
    SoBaseColor      * pColor;
    SoBaseColor      * pTextColor;
    SoTranslation    * pTranslation;
    SoCoordinate3    * pCoords;
    SoIndexedFaceSet * pFaces;

    /// Direction pointing outside of the solid
    SbVec3f          * normalDirection;
    /// Direction of the force
    SbVec3f          * arrowDirection;

    void findCylinderData(SbVec3f& z, SbVec3f& y, SbVec3f& x, SbVec3f& p, double& radius, double& height);
};

} //namespace FemGui


#endif // GUI_VIEWPROVIDERFEMCONSTRAINT_H
