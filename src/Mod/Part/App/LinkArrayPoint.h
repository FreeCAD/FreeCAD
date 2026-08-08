// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "LinkArray.h"
#include "PointPatternExtension.h"

namespace Part
{

class PartExport LinkArrayPoint: public Part::LinkArray, public Part::PointPatternExtension
{
    PROPERTY_HEADER_WITH_EXTENSIONS(Part::LinkArrayPoint);
    using inherited = Part::LinkArray;

public:
    LinkArrayPoint();
    void onDocumentRestored() override;

protected:
    std::vector<Base::Placement> getElementPlacements() override;

private:
    void setPointObjectLinkScope();
};

}  // namespace Part
