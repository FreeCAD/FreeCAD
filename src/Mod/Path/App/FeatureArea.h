/****************************************************************************
 *   Copyright (c) 2017 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#ifndef PATH_FeatureArea_H
#define PATH_FeatureArea_H

#include <App/DocumentObject.h>
#include <App/GeoFeature.h>
#include <App/PropertyUnits.h>
#include <App/FeaturePython.h>
#include "Mod/Part/App/PartFeature.h"

#include "Area.h"

namespace Path
{

class PathExport FeatureArea : public Part::Feature
{
    PROPERTY_HEADER(Path::FeatureArea);

public:
    /// Constructor
    FeatureArea(void);
    virtual ~FeatureArea();

    Area &getArea();
    const std::vector<TopoDS_Shape> &getShapes();

    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName(void) const {
        return "PathGui::ViewProviderArea";
    }
    virtual App::DocumentObjectExecReturn *execute(void);
    virtual short mustExecute(void) const;
    virtual PyObject *getPyObject(void);

    App::PropertyLinkList   Sources;
    Part::PropertyPartShape WorkPlane;

    PARAM_PROP_DECLARE(AREA_PARAMS_ALL)

    void setWorkPlane(const TopoDS_Shape &shape) {
        WorkPlane.setValue(shape);
        myArea.setPlane(shape);
    }

private:
    Area myArea;
    std::vector<TopoDS_Shape> myShapes;
    bool myInited;
};

typedef App::FeaturePythonT<FeatureArea> FeatureAreaPython;

class PathExport FeatureAreaView : public Part::Feature
{
    PROPERTY_HEADER(Path::FeatureAreaView);

public:
    /// Constructor
    FeatureAreaView(void);

    std::list<TopoDS_Shape> getShapes();

    virtual const char* getViewProviderName(void) const {
        return "PathGui::ViewProviderAreaView";
    }
    virtual App::DocumentObjectExecReturn *execute(void);

    App::PropertyLink       Source;
    App::PropertyInteger    SectionIndex;
    App::PropertyInteger    SectionCount;
};

typedef App::FeaturePythonT<FeatureAreaView> FeatureAreaViewPython;

} //namespace Path


#endif // PATH_FeaturePath_H
