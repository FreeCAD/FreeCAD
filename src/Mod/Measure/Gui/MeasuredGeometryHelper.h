// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <string>
#include <vector>

namespace App
{
class DocumentObject;
}

namespace MeasureGui::MeasuredGeometryHelper
{

struct CanonicalGeometryReference
{
    App::DocumentObject* owner {nullptr};
    std::string stableSketchGeometry;
    std::string element;
};

CanonicalGeometryReference canonicalGeometryReference(
    App::DocumentObject* object,
    const std::string& subname
);

bool referencesSameGeometry(
    App::DocumentObject* firstObject,
    const std::string& firstSubname,
    App::DocumentObject* secondObject,
    const std::string& secondSubname,
    bool allowCurrentElementFallback = false
);

std::vector<std::string> getBoundarySubnames(App::DocumentObject* object, const std::string& subname);

std::vector<std::string> getCircularBoundarySubnames(
    App::DocumentObject* object,
    const std::string& subname
);

}  // namespace MeasureGui::MeasuredGeometryHelper
