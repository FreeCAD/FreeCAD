/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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


#include "PreCompiled.h"

#ifndef _PreComp_
# include <Standard.hxx>
# include <string>
#endif

#include <Base/FileInfo.h>
#include <Base/Console.h>
#include <Base/Stream.h>
#include <App/Application.h>
#include "RayProject.h"
#include "RayFeature.h"

using namespace Raytracing;
using namespace std;

PROPERTY_SOURCE(Raytracing::RayProject, App::DocumentObjectGroup)

//===========================================================================
// Feature
//===========================================================================

RayProject::RayProject(void)
{
    ADD_PROPERTY_TYPE(PageResult, (nullptr), 0, App::Prop_Output, "Resulting povray Project file");
    ADD_PROPERTY_TYPE(Template, (""), 0, App::Prop_None, "Template for the Povray project");
    ADD_PROPERTY_TYPE(Camera, (""), 0, App::Prop_None, "Camera settings");
}

void RayProject::onDocumentRestored()
{
    Base::FileInfo templateInfo(Template.getValue());
    if (!templateInfo.exists()) {
        Base::FileInfo fi(Template.getValue());
        if (fi.fileName().empty())
            fi.setFile(PageResult.getValue());
        std::string path = App::Application::getResourceDir() + "Mod/Raytracing/Templates/" + fi.fileName();
        // try to find the template in user dir/Templates first
        Base::FileInfo tempfi(App::Application::getUserAppDataDir() + "data/Mod/Raytracing/Templates/" + fi.fileName());
        if (tempfi.exists())
            path = tempfi.filePath();
        Template.setValue(path);
    }
}

App::DocumentObjectExecReturn *RayProject::execute(void)
{
    if (std::string(PageResult.getValue()) == "")
        PageResult.setValue(Template.getValue());

    Base::FileInfo fi(Template.getValue());
    if (!fi.isReadable()) {
        Base::Console().Log("RayProject::execute() not able to open %s!\n",Template.getValue());
        std::string error = std::string("Cannot open file ") + Template.getValue();
        return new App::DocumentObjectExecReturn(error);
    }
    // open Template file
    string line;
    ifstream file ( fi.filePath().c_str() );

    // make a temp file for FileIncluded Property
    string tempName = PageResult.getExchangeTempFile();
    ofstream ofile(tempName.c_str());

    // copy the input of the resource file
    while (getline (file,line)) {
        // check if the marker in the template is found
        if(line.find("//RaytracingContent") == string::npos)
            // if not -  write through
            ofile << line << endl;
        else {
            ofile << Camera.getValue();
        
            // get through the children and collect all the views
            const std::vector<App::DocumentObject*> &Grp = Group.getValues();
            for (std::vector<App::DocumentObject*>::const_iterator It= Grp.begin();It!=Grp.end();++It) {
                if ((*It)->getTypeId().isDerivedFrom(Raytracing::RaySegment::getClassTypeId())) {
                    Raytracing::RaySegment *View = static_cast<Raytracing::RaySegment *>(*It);
                    ofile << View->Result.getValue();
                    ofile << endl << endl << endl;
                }
            }
        }
    }

    file.close();
    ofile.close();

    PageResult.setValue(tempName.c_str());

    return App::DocumentObject::StdReturn;
}

short RayProject::mustExecute() const
{
    if (Camera.isTouched())
        return 1;
    if (Template.isTouched())
        return 1;
    return 0;
}
