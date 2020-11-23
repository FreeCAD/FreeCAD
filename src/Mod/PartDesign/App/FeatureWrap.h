/****************************************************************************
 *   Copyright (c) 2020 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
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

#ifndef PARTDESIGN_FeatureWrap_H
#define PARTDESIGN_FeatureWrap_H

#include "FeatureAddSub.h"

namespace PartDesign
{

class PartDesignExport FeatureWrap : public PartDesign::FeatureAddSub
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::FeatureWrap);

public:
    FeatureWrap();

    App::PropertyLink WrapFeature;
    App::PropertyEnumeration Type;
    App::PropertyBool Frozen;

    virtual const char* getViewProviderName(void) const override {
        return "PartDesignGui::ViewProviderWrap";
    }

    virtual void onChanged(const App::Property* prop) override;
    virtual App::DocumentObjectExecReturn *execute(void) override;
    virtual void onDocumentRestored() override;
    virtual bool isElementGenerated(const TopoShape &shape, const char *name) const override;

    bool isSolidFeature() const;
    void setWrappedLinkScope();
};

typedef App::FeaturePythonT<FeatureWrap> FeatureWrapPython;

} //namespace PartDesign


#endif // PARTDESIGN_FeatureWrap_H
