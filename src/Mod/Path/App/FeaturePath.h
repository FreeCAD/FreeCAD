/***************************************************************************
 *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
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

#ifndef PATH_FeaturePath_H
#define PATH_FeaturePath_H

#include <App/DocumentObject.h>
#include <App/GeoFeature.h>
#include <App/FeaturePython.h>

#include "PropertyPath.h"


namespace Path
{

class PathExport Feature : public App::GeoFeature
{
    PROPERTY_HEADER_WITH_OVERRIDE(Path::Feature);

public:
    /// Constructor
    Feature();
    ~Feature() override;

    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override {
        return "PathGui::ViewProviderPath";
    }
    App::DocumentObjectExecReturn *execute() override {
        return App::DocumentObject::StdReturn;
    }
    short mustExecute() const override;
    PyObject *getPyObject() override;

    PropertyPath           Path;


protected:
    /// get called by the container when a property has changed
    void onChanged (const App::Property* prop) override;

};

using FeaturePython = App::FeaturePythonT<Feature>;

} //namespace Path


#endif // PATH_FeaturePath_H
