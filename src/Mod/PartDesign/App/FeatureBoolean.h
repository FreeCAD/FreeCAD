/******************************************************************************
 *   Copyright (c) 2013 Jan Rheinländer <jrheinlaender@users.sourceforge.net> *
 *                                                                            *
 *   This file is part of the FreeCAD CAx development system.                 *
 *                                                                            *
 *   This library is free software; you can redistribute it and/or            *
 *   modify it under the terms of the GNU Library General Public              *
 *   License as published by the Free Software Foundation; either             *
 *   version 2 of the License, or (at your option) any later version.         *
 *                                                                            *
 *   This library  is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Library General Public License for more details.                     *
 *                                                                            *
 *   You should have received a copy of the GNU Library General Public        *
 *   License along with this library; see the file COPYING.LIB. If not,       *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,            *
 *   Suite 330, Boston, MA  02111-1307, USA                                   *
 *                                                                            *
 ******************************************************************************/


#ifndef PARTDESIGN_FeatureBoolean_H
#define PARTDESIGN_FeatureBoolean_H

#include <App/GeoFeatureGroupExtension.h>
#include <App/PropertyStandard.h>
#include "Feature.h"


namespace PartDesign
{

/**
 * Abstract superclass of all features that are created by transformation of another feature
 * Transformations are translation, rotation and mirroring
 */
class PartDesignExport Boolean : public PartDesign::Feature, public App::GeoFeatureGroupExtension
{
    PROPERTY_HEADER_WITH_EXTENSIONS(PartDesign::Boolean);

public:
    Boolean();

    /// The type of the boolean operation
    App::PropertyEnumeration    Type;

    App::PropertyBool Refine;

   /** @name methods override feature */
    //@{
    /// Recalculate the feature
    App::DocumentObjectExecReturn *execute() override;
    short mustExecute() const override;
    /// returns the type name of the view provider
    const char* getViewProviderName() const override {
        return "PartDesignGui::ViewProviderBoolean";
    }
    void onChanged(const App::Property* prop) override;
    //@}

protected:
    void handleChangedPropertyName(Base::XMLReader &reader, const char * TypeName, const char *PropName) override;
    TopoDS_Shape refineShapeIfActive(const TopoDS_Shape&) const;


private:
    static const char* TypeEnums[];

};

} //namespace PartDesign


#endif // PARTDESIGN_FeatureBoolean_H
