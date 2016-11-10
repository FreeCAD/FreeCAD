/***************************************************************************
 *   Copyright (c) Yorik van Havre (yorik@uncreated.net) 2014              *
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


#ifndef PATH_FeaturePathShape_H
#define PATH_FeaturePathShape_H

#include <App/DocumentObject.h>
#include <App/GeoFeature.h>
#include <App/PropertyFile.h>
#include <App/PropertyGeo.h>
#include <App/FeaturePython.h>
#include "Mod/Part/App/PropertyTopoShape.h"

#include "Path.h"
#include "PropertyPath.h"
#include "FeaturePath.h"

namespace Path
{

class PathExport FeatureShape : public Path::Feature
{
    PROPERTY_HEADER(Path::FeatureShape);

public:
    /// Constructor
    FeatureShape(void);
    virtual ~FeatureShape();
    
    Part::PropertyPartShape Shape;

    //@{
    /// recalculate the feature
    virtual App::DocumentObjectExecReturn *execute(void);
    virtual short mustExecute(void) const;
    //@}

    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName(void) const {
        return "PathGui::ViewProviderPathShape";
    }
    
protected:
    /// get called by the container when a property has changed
    //virtual void onChanged (const App::Property* prop);

};

typedef App::FeaturePythonT<FeatureShape> FeatureShapePython;

} //namespace Path


#endif // PATH_FeaturePathShape_H
