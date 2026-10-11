// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "LinkArray.h"
#include "PathPatternExtension.h"

namespace Part
{

class PartExport LinkArrayPath: public Part::LinkArray, public Part::PathPatternExtension
{
    PROPERTY_HEADER_WITH_EXTENSIONS(Part::LinkArrayPath);
    using inherited = Part::LinkArray;

public:
    LinkArrayPath();
    void onDocumentRestored() override;

protected:
    std::vector<Base::Placement> getElementPlacements() override;

private:
    void setPathLinkScope();
};

}  // namespace Part
