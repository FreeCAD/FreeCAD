/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel (juergen.riegel@web.de)              *
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


#ifndef ROBOT_ViewProviderTrajectory_H
#define ROBOT_ViewProviderTrajectory_H

#include <Inventor/VRMLnodes/SoVRMLTransform.h>
#include <Gui/ViewProviderGeometryObject.h>
#include <Gui/SoFCSelection.h>

class SoDragger;
class SoJackDragger;
class SoCoordinate3;
class SoDrawStyle;  
class SoLineSet; 

namespace RobotGui
{

class RobotGuiExport ViewProviderTrajectory : public Gui::ViewProviderGeometryObject
{
    PROPERTY_HEADER(RobotGui::ViewProviderTrajectory);

public:
    /// constructor.
    ViewProviderTrajectory();

    /// destructor.
    ~ViewProviderTrajectory();

    void attach(App::DocumentObject *pcObject);
    void setDisplayMode(const char* ModeName);
    std::vector<std::string> getDisplayModes() const;
    void updateData(const App::Property*);
    void setupContextMenu(QMenu* menu, QObject* receiver, const char* member);

protected:
 
    Gui::SoFCSelection    * pcTrajectoryRoot;
    SoCoordinate3         * pcCoords;
    SoDrawStyle           * pcDrawStyle;
    SoLineSet             * pcLines;

 };

} //namespace RobotGui


#endif // ROBOT_VIEWPROVIDERROBOTOBJECT_H
