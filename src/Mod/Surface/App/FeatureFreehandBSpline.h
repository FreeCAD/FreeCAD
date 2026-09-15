// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include <Mod/Part/App/FeaturePartSpline.h>
#include <Mod/Surface/SurfaceGlobal.h>
#include <Geom_BSplineCurve.hxx>

namespace Surface
{
class SurfaceExport FreehandBSpline: public Part::Spline
{
    PROPERTY_HEADER_WITH_OVERRIDE(Surface::FreehandBSpline);

public:
    FreehandBSpline();
    App::PropertyVectorList Points;
    App::PropertyBool Periodic;
    App::PropertyFloatConstraint Parameterization;
    App::PropertyBoolList LinearSegments;
    App::PropertyLinkSubList Support;
    App::PropertyIntegerList SupportPointIndices;
    App::PropertyVectorList SupportParameters;
    void setPointSupport(int index, App::DocumentObject* object, const std::string& subname);
    App::PropertyLinkSubList TangentSupport;
    App::PropertyIntegerList TangentPointIndices;
    std::vector<std::string> tangentChoices(int index) const;
    void setPointTangent(int index, const std::string& subname);
    void remapSupports(const std::vector<int>& retained);
    void updateSupportedPoints(bool project);
    void sanitizeReferences();


    Base::Vector3d tangentDirection(
        int index,
        const std::vector<Base::Vector3d>& points,
        App::DocumentObject* object,
        const std::string& name
    ) const;
    short mustExecute() const override;
    App::DocumentObjectExecReturn* execute() override;
    Handle(Geom_BSplineCurve) interpolate() const;
    Handle(Geom_BSplineCurve) interpolate(const std::vector<Base::Vector3d>& points) const;
    const char* getViewProviderName() const override
    {
        return "SurfaceGui::ViewProviderFreehandBSpline";
    }

protected:
    void onBeforeChange(const App::Property*) override;
    void onChanged(const App::Property*) override;

private:
    std::vector<size_t> removedSupportSlots;
    std::vector<size_t> removedTangentSlots;
};
}  // namespace Surface
