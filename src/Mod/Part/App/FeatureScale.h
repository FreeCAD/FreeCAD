/***************************************************************************
 *   Copyright (c) 2023 Wanderer Fan <wandererfan@gmail.com>               *
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

#ifndef PART_FEATURESCALE_H
#define PART_FEATURESCALE_H

#include <App/PropertyStandard.h>
#include <App/PropertyUnits.h>

#include "FaceMakerCheese.h"
#include "PartFeature.h"


namespace Part
{

class PartExport Scale : public Part::Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(Part::Scale);

public:
    Scale();

    App::PropertyLink Base;
    App::PropertyBool Uniform;
    App::PropertyFloat UniformScale;
    App::PropertyFloat XScale;
    App::PropertyFloat YScale;
    App::PropertyFloat ZScale;

    /**
     * @brief The ScaleParameters struct is supposed to be filled with final
     * scale parameters, after resolving links, applying mode logic,
     * reversing, etc., and be passed to scaleShape.
     */
    struct ScaleParameters {
        bool uniform;
        double uniformScale{1.0};
        double XScale{1.0};
        double YScale{1.0};
        double ZScale{1.0};
    };

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn *execute() override;
    short mustExecute() const override;
    /// returns the type name of the view provider
    const char* getViewProviderName() const override {
        return "PartGui::ViewProviderScale";
    }
    //@}
    Scale::ScaleParameters computeFinalParameters();

    /**
     * @brief scaleShape powers the extrusion feature.
     * @param source: the shape to be scaled
     * @param params: scale parameters
     * @return result of scaling
     */
    static TopoShape scaleShape(const TopoShape& source, const ScaleParameters& params);
    static TopoShape uniformScale(const TopoShape& source, const double& factor);
    static TopoShape nonuniformScale(const TopoShape& source, const Scale::ScaleParameters& params);
};

} //namespace Part


#endif // PART_FEATURESCALE_H

