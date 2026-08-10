// SPDX-License-Identifier: LGPL-2.1-or-later

#include "MbDPart.h"

#include <algorithm>
#include <cstring>

#include <App/Document.h>
#include <App/GeoFeatureGroupExtension.h>

#include "MbDAssembly.h"
#include "MbDFolders.h"
#include "MbDGroupUtils.h"
#include "MbDMarker.h"
#include "MbDPartPy.h"

PROPERTY_SOURCE_WITH_EXTENSIONS(MbDFEM::MbDPart, Part::Feature)

namespace
{

App::DocumentObjectGroup* matchingFolder(const char* subname,
                                         const char*& rest,
                                         App::DocumentObjectGroup* folder)
{
    rest = nullptr;
    const char* dot = subname ? std::strchr(subname, '.') : nullptr;
    if (!dot || !folder) {
        return nullptr;
    }

    if (std::string(subname, dot) == folder->getNameInDocument()) {
        rest = dot + 1;
        return folder;
    }
    return nullptr;
}

App::DocumentObject* findDirectChildByInternalName(const char* element,
                                                   const std::vector<App::DocumentObject*>& children)
{
    if (!element || !*element) {
        return nullptr;
    }

    std::string name(element);
    if (!name.empty() && name.back() == '.') {
        name.pop_back();
    }

    for (auto* child : children) {
        if (!child) {
            continue;
        }
        if (name == child->getNameInDocument()) {
            return child;
        }
    }
    return nullptr;
}

bool parentAssemblyVisible(const App::DocumentObject* object)
{
    auto* group = object ? App::GeoFeatureGroupExtension::getGroupOfObject(object) : nullptr;
    auto* assembly = freecad_cast<MbDFEM::MbDAssembly*>(group);
    return !assembly || assembly->Visibility.getValue();
}

}  // namespace

MbDFEM::MbDPart::MbDPart()
{
    ADD_PROPERTY_TYPE(markers,
                      (nullptr),
                      "MbDFEM",
                      App::Prop_None,
                      "Markers belonging to this part");
    markers.setScope(App::LinkScope::Child);
    ADD_PROPERTY_TYPE(_markersFolder,
                      (nullptr),
                      "MbDFEM",
                      App::Prop_Hidden,
                      "Tree folder containing this part's markers");
    _markersFolder.setScope(App::LinkScope::Hidden);
    ADD_PROPERTY_TYPE(xs, (), "MbDFEM Results", App::Prop_None, "Solved X position values");
    ADD_PROPERTY_TYPE(ys, (), "MbDFEM Results", App::Prop_None, "Solved Y position values");
    ADD_PROPERTY_TYPE(zs, (), "MbDFEM Results", App::Prop_None, "Solved Z position values");
    ADD_PROPERTY_TYPE(bryxs, (), "MbDFEM Results", App::Prop_None, "Solved Bryant X angle values");
    ADD_PROPERTY_TYPE(bryys, (), "MbDFEM Results", App::Prop_None, "Solved Bryant Y angle values");
    ADD_PROPERTY_TYPE(bryzs, (), "MbDFEM Results", App::Prop_None, "Solved Bryant Z angle values");
    ADD_PROPERTY_TYPE(vxs, (), "MbDFEM Results", App::Prop_None, "Solved X velocity values");
    ADD_PROPERTY_TYPE(vys, (), "MbDFEM Results", App::Prop_None, "Solved Y velocity values");
    ADD_PROPERTY_TYPE(vzs, (), "MbDFEM Results", App::Prop_None, "Solved Z velocity values");
    ADD_PROPERTY_TYPE(omexs, (), "MbDFEM Results", App::Prop_None, "Solved X angular velocity values");
    ADD_PROPERTY_TYPE(omeys, (), "MbDFEM Results", App::Prop_None, "Solved Y angular velocity values");
    ADD_PROPERTY_TYPE(omezs, (), "MbDFEM Results", App::Prop_None, "Solved Z angular velocity values");
    ADD_PROPERTY_TYPE(axs, (), "MbDFEM Results", App::Prop_None, "Solved X acceleration values");
    ADD_PROPERTY_TYPE(ays, (), "MbDFEM Results", App::Prop_None, "Solved Y acceleration values");
    ADD_PROPERTY_TYPE(azs, (), "MbDFEM Results", App::Prop_None, "Solved Z acceleration values");
    ADD_PROPERTY_TYPE(alpxs, (), "MbDFEM Results", App::Prop_None, "Solved X angular acceleration values");
    ADD_PROPERTY_TYPE(alpys, (), "MbDFEM Results", App::Prop_None, "Solved Y angular acceleration values");
    ADD_PROPERTY_TYPE(alpzs, (), "MbDFEM Results", App::Prop_None, "Solved Z angular acceleration values");

    App::OriginGroupExtension::initExtension(this);
}

void MbDFEM::MbDPart::addMarker(MbDMarker* marker)
{
    if (!marker) {
        return;
    }

    addChildToListFolderAndGeoGroup(this, markers, ensureMarkersFolder(), marker);
}

void MbDFEM::MbDPart::removeMarker(MbDMarker* marker)
{
    removeChildFromListFolderAndGeoGroup(this, markers, getMarkersFolder(), marker);
}

int MbDFEM::MbDPart::setElementVisible(const char* element, bool visible)
{
    auto* child = findDirectChildByInternalName(element, markers.getValues());
    if (!child) {
        return Part::Feature::setElementVisible(element, visible);
    }

    child->Visibility.setValue(visible);
    return visible ? 1 : 0;
}

int MbDFEM::MbDPart::isElementVisible(const char* element) const
{
    if (!Visibility.getValue() || !parentAssemblyVisible(this)) {
        return 0;
    }

    auto* child = findDirectChildByInternalName(element, markers.getValues());
    if (!child) {
        return Part::Feature::isElementVisible(element);
    }

    return child->Visibility.getValue() ? 1 : 0;
}

App::DocumentObject* MbDFEM::MbDPart::getSubObject(const char* subname,
                                                   PyObject** pyObj,
                                                   Base::Matrix4D* mat,
                                                   bool transform,
                                                   int depth) const
{
    const char* rest = nullptr;
    auto* folder = matchingFolder(subname, rest, getMarkersFolder());
    if (folder) {
        if (!rest || *rest == '\0') {
            return folder;
        }
        return Part::Feature::getSubObject(rest, pyObj, mat, transform, depth);
    }

    return Part::Feature::getSubObject(subname, pyObj, mat, transform, depth);
}

void MbDFEM::MbDPart::unsetupObject()
{
    auto* document = getDocument();
    if (document) {
        const auto markerValues = markers.getValues();
        for (auto* marker : markerValues) {
            if (marker && marker->isAttachedToDocument() && !marker->isRemoving()) {
                document->removeObject(marker->getNameInDocument());
            }
        }

        if (auto* folder = getMarkersFolder()) {
            if (folder->isAttachedToDocument() && !folder->isRemoving()) {
                document->removeObject(folder->getNameInDocument());
            }
        }
    }

    Part::Feature::unsetupObject();
}

App::DocumentObjectGroup* MbDFEM::MbDPart::getMarkersFolder() const
{
    return dynamic_cast<App::DocumentObjectGroup*>(_markersFolder.getValue());
}

App::DocumentObjectGroup* MbDFEM::MbDPart::ensureMarkersFolder()
{
    if (auto* folder = getMarkersFolder()) {
        return folder;
    }
    if (!getDocument()) {
        return nullptr;
    }

    const std::string name = std::string(getNameInDocument()) + "_Markers";
    auto* folder = static_cast<App::DocumentObjectGroup*>(
        getDocument()->addObject("MbDFEM::MbDMarkersFolder", name.c_str()));
    folder->Label.setValue("Markers");
    _markersFolder.setValue(folder);
    return folder;
}

PyObject* MbDFEM::MbDPart::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        PythonObject = Py::Object(new MbDPartPy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}
