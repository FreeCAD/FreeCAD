// SPDX-License-Identifier: LGPL-2.1-or-later

#include "ViewProviderMbDGravity.h"

#include <App/Document.h>
#include <Base/Vector3D.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>

#include <Mod/MbDFEM/App/MbDAssembly.h>
#include <Mod/MbDFEM/App/MbDParameters.h>

using namespace MbDFEMGui;

PROPERTY_SOURCE(MbDFEMGui::ViewProviderMbDGravity, Gui::ViewProviderDocumentObject)

namespace
{

constexpr float minGravityMagnitude = 1.0e-9F;

MbDFEM::MbDAssembly* owningAssembly(MbDFEM::MbDGravity* gravity)
{
    auto* document = gravity ? gravity->getDocument() : nullptr;
    if (!document) {
        return nullptr;
    }

    for (auto* object : document->getObjectsOfType<MbDFEM::MbDAssembly>()) {
        if (object && object->getGravity() == gravity) {
            return object;
        }
    }

    return nullptr;
}

}  // namespace

ViewProviderMbDGravity::ViewProviderMbDGravity()
{
    sPixmap = "Document";

    ADD_PROPERTY_TYPE(ShowArrow,
                      (true),
                      "Display Options",
                      App::Prop_None,
                      "Show a selectable gravity direction arrow in the corner coordinate system");
}

ViewProviderMbDGravity::~ViewProviderMbDGravity()
{
    updateCornerGravityIndicator(true);
}

void ViewProviderMbDGravity::attach(App::DocumentObject* object)
{
    Gui::ViewProviderDocumentObject::attach(object);
    updateCornerGravityIndicator();
}

void ViewProviderMbDGravity::updateData(const App::Property* prop)
{
    Gui::ViewProviderDocumentObject::updateData(prop);

    auto* gravity = getObject<MbDFEM::MbDGravity>();
    if (gravity && prop == &gravity->gravity) {
        updateCornerGravityIndicator();
    }
}

void ViewProviderMbDGravity::onChanged(const App::Property* prop)
{
    Gui::ViewProviderDocumentObject::onChanged(prop);

    if (prop == &Visibility || prop == &ShowArrow) {
        updateCornerGravityIndicator();
    }
}

bool ViewProviderMbDGravity::arrowVisible() const
{
    auto* gravity = getObject<MbDFEM::MbDGravity>();
    return gravity && gravity->Visibility.getValue() && Visibility.getValue()
        && ShowArrow.getValue() && owningAssembly(gravity)
        && gravity->gravity.getValue().Length() > minGravityMagnitude;
}

void ViewProviderMbDGravity::updateCornerGravityIndicator(bool forceHidden)
{
    auto* gravity = getObject<MbDFEM::MbDGravity>();
    auto* document = gravity ? gravity->getDocument() : nullptr;
    auto* guiDocument = document ? Gui::Application::Instance->getDocument(document) : nullptr;
    auto* view = guiDocument ? dynamic_cast<Gui::View3DInventor*>(guiDocument->getActiveView())
                             : nullptr;
    auto* viewer = view ? view->getViewer() : nullptr;
    if (!viewer) {
        return;
    }

    if (!forceHidden && !owningAssembly(gravity)) {
        return;
    }

    const auto gravityVector = gravity ? gravity->gravity.getValue() : Base::Vector3d();
    viewer->setCornerGravityIndicator(
        !forceHidden && arrowVisible(),
        SbVec3f(static_cast<float>(gravityVector.x),
                static_cast<float>(gravityVector.y),
                static_cast<float>(gravityVector.z)),
        document ? document->getName() : nullptr,
        gravity ? gravity->getNameInDocument() : nullptr
    );
}
