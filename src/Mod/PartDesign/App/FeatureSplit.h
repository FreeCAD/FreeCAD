/****************************************************************************
 *   Copyright (c) 2019 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
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

#ifndef PARTDESIGN_FeatureSplit_H
#define PARTDESIGN_FeatureSplit_H

#include <App/PropertyUnits.h>
#include <App/GeoFeatureGroupExtension.h>
#include "Feature.h"


namespace PartDesign
{

class Solid;

/**
 * Split previous features into multiple solids using optional tool features
 */
class PartDesignExport Split : public PartDesign::Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::Split);

public:
    Split();

    App::PropertyEnumeration Mode;
    App::PropertyBool Fragment;
    App::PropertyLength Tolerance;
    App::PropertyLinkList Tools;
    App::PropertyLinkList Solids;

    virtual App::DocumentObjectExecReturn *execute(void) override;
    virtual void afterRecompute() override;

    virtual const char* getViewProviderName(void) const override {
        return "PartDesignGui::ViewProviderSplit";
    }
    virtual void onChanged(const App::Property* prop) override;

    virtual DocumentObject *getSubObject(const char *subname, PyObject **pyObj=0, 
            Base::Matrix4D *mat=0, bool transform=true, int depth=0) const override;

    bool isToolAllowed(App::DocumentObject *, bool inside=false) const;

    void updateActiveSolid(Solid *);

    virtual bool isElementGenerated(const TopoShape &shape, const char *name) const override;

    virtual int isElementVisible(const char *element) const override;
    virtual int setElementVisible(const char *element, bool visible) override;

private:
    std::size_t newSolidCount = 0;
};

} //namespace PartDesign


#endif // PARTDESIGN_FeatureSplit_H
