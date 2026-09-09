// SPDX-License-Identifier: LGPL-2.1-or-later

/******************************************************************************
 *   Copyright (c) 2012 Jan Rheinländer <jrheinlaender@users.sourceforge.net> *
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


#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <Precision.hxx>


#include "FeatureMultiTransform.h"
#include "FeatureAddSub.h"
#include "FeatureScaled.h"
#include "FeaturePointPattern.h"

#include <cmath>


using namespace PartDesign;

namespace PartDesign
{


PROPERTY_SOURCE(PartDesign::MultiTransform, PartDesign::Transformed)

MultiTransform::MultiTransform()
{
    ADD_PROPERTY(Transformations, (nullptr));
    Transformations.setSize(0);
}

void MultiTransform::positionBySupport()
{
    PartDesign::Transformed::positionBySupport();
    std::vector<App::DocumentObject*> transFeatures = Transformations.getValues();
    for (auto f : transFeatures) {
        auto transFeature = freecad_cast<PartDesign::Transformed*>(f);
        if (!transFeature) {
            throw Base::TypeError("Transformation features must be subclasses of Transformed");
        }

        const auto& placement = this->Placement.getValue();
        if (transFeature->Placement.getValue() != placement) {
            transFeature->Placement.setValue(placement);
        }

        // These objects only store transformation parameters. Their execution is handled by this
        // MultiTransform, so synchronizing their placement must not schedule another document pass.
        transFeature->purgeTouched();
    }
}

short MultiTransform::mustExecute() const
{
    if (Transformations.isTouched()) {
        return 1;
    }
    return Transformed::mustExecute();
}

bool MultiTransform::isTransformationSuppressed(int index) const
{
    return Transformed::isTransformationSuppressed(index)
        || (index >= 0 && static_cast<std::size_t>(index) < generatedSuppression.size()
            && generatedSuppression[index]);
}

const std::list<gp_Trsf> MultiTransform::getTransformations(
    const std::vector<App::DocumentObject*> originals
)
{
    generatedSuppression.clear();
    originalTransformation = true;
    const auto transFeatures = Transformations.getValues();
    if (transFeatures.empty()) {
        return {};
    }

    gp_Pnt cog;
    if (!originals.empty()) {
        if (auto* feature = freecad_cast<PartDesign::FeatureAddSub*>(originals.front())) {
            GProp_GProps props;
            BRepGProp::VolumeProperties(feature->AddSubShape.getShape().getShape(), props);
            cog = props.CentreOfMass();
        }
    }

    struct Instance
    {
        gp_Trsf transform;
        gp_Pnt center;
        bool suppressed;
    };
    std::vector<Instance> instances;
    bool firstFeature = true;
    for (auto* feature : transFeatures) {
        auto* transformed = freecad_cast<PartDesign::Transformed*>(feature);
        if (!transformed) {
            throw Base::TypeError("Transformation features must be subclasses of Transformed");
        }

        // A standalone point pattern puts its first point in Placement. Helpers must
        // carry that translation in the transformations instead, since they are not executed.
        auto* points = freecad_cast<PartDesign::PointPattern*>(transformed);
        const auto transformations = points ? points->calculateTransformations(false)
                                            : transformed->getTransformations(originals);
        transformed->purgeTouched();
        if (firstFeature) {
            int index = 0;
            for (const auto& transform : transformations) {
                instances.push_back(
                    {transform,
                     cog.Transformed(transform),
                     transformed->isTransformationSuppressed(index++)}
                );
            }
            firstFeature = false;
            continue;
        }

        std::vector<Instance> previous;
        previous.swap(instances);
        if (previous.empty()) {
            continue;  // An empty product must not restart at the next helper.
        }
        if (transformed->is<PartDesign::Scaled>()) {
            if (transformations.empty() || previous.size() % transformations.size() != 0) {
                throw Base::ValueError(
                    "Number of occurrences must be a divisor of previous number of occurrences"
                );
            }
            const std::size_t sliceLength = previous.size() / transformations.size();
            std::size_t oldIndex = 0;
            int index = 0;
            for (const auto& transform : transformations) {
                const bool suppressed = transformed->isTransformationSuppressed(index++);
                for (std::size_t slice = 0; slice < sliceLength; ++slice) {
                    const auto& old = previous[oldIndex++];
                    gp_Trsf combined;
                    gp_Pnt center = old.center;
                    const double factor = transform.ScaleFactor();
                    if (factor > Precision::Confusion()) {
                        combined.SetScale(center, factor);
                        combined.Multiply(old.transform);
                    }
                    else {
                        combined = transform * old.transform;
                        center.Transform(transform);
                    }
                    instances.push_back({combined, center, old.suppressed || suppressed});
                }
            }
        }
        else {
            int index = 0;
            for (const auto& transform : transformations) {
                const bool suppressed = transformed->isTransformationSuppressed(index++);
                for (const auto& old : previous) {
                    instances.push_back(
                        {transform * old.transform,
                         old.center.Transformed(transform),
                         old.suppressed || suppressed}
                    );
                }
            }
        }
    }

    if (instances.empty()) {
        // Keep an explicitly suppressed identity so execute() removes the original
        // material rather than treating an empty list as an unfinished feature.
        generatedSuppression.push_back(true);
        return {gp_Trsf()};
    }
    const auto& first = instances.front().transform;
    for (int row = 1; row <= 3; ++row) {
        for (int column = 1; column <= 4; ++column) {
            if (std::abs(first.Value(row, column) - (row == column ? 1.0 : 0.0))
                > Precision::Confusion()) {
                originalTransformation = false;
            }
        }
    }

    std::list<gp_Trsf> result;
    for (const auto& instance : instances) {
        result.push_back(instance.transform);
        generatedSuppression.push_back(instance.suppressed);
    }
    return result;
}

}  // namespace PartDesign
