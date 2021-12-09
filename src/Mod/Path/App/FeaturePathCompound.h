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


#ifndef PATH_FeatureCompound_H
#define PATH_FeatureCompound_H

#include <App/GeoFeature.h>
#include <App/PropertyFile.h>
#include <App/PropertyGeo.h>
#include <App/PropertyUnits.h>

#include "Path.h"
#include "FeaturePath.h"
#include "PropertyPath.h"

namespace Path
{

class PathExport FeatureCompound : public Path::Feature
{
    PROPERTY_HEADER(Path::Feature);

public:
    /// Constructor
    FeatureCompound(void);
    virtual ~FeatureCompound();

    App::PropertyLinkList     Group;
    App::PropertyBool         UsePlacements;

    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName(void) const {
        return "PathGui::ViewProviderPathCompound";
    }
    virtual App::DocumentObjectExecReturn *execute(void);
    
    /// Checks whether the object \a obj is part of this group.
    bool hasObject(const DocumentObject* obj) const;
    /// Adds an object to this group.
    void addObject(DocumentObject* obj);
    /// Removes an object from this group.
    void removeObject(DocumentObject* obj);
    virtual PyObject *getPyObject(void);

};

typedef App::FeaturePythonT<FeatureCompound> FeatureCompoundPython;

} //namespace Path


#endif // PATH_FeatureCompound_H
