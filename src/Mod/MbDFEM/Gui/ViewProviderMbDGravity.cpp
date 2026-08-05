// SPDX-License-Identifier: LGPL-2.1-or-later

#include "ViewProviderMbDGravity.h"

#include <App/Application.h>
#include <App/Document.h>
#include <Base/Vector3D.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>

#include <QTimer>

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

Gui::View3DInventorViewer* getActiveGravityViewer(MbDFEM::MbDGravity* gravity)
{
    auto* document = gravity ? gravity->getDocument() : nullptr;
    auto* guiDocument = document ? Gui::Application::Instance->getDocument(document) : nullptr;
    auto* view = guiDocument ? dynamic_cast<Gui::View3DInventor*>(guiDocument->getActiveView())
                             : nullptr;
    return view ? view->getViewer() : nullptr;
}

void updateGravityIndicator(MbDFEM::MbDGravity* gravity,
                            bool visible,
                            bool forceHidden,
                            const SbVec3f& gravityVector)
{
    auto* viewer = getActiveGravityViewer(gravity);
    if (!viewer) {
        return;
    }

    auto* document = gravity ? gravity->getDocument() : nullptr;
    viewer->setCornerGravityIndicator(
        !forceHidden && visible,
        gravityVector,
        document ? document->getName() : nullptr,
        gravity ? gravity->getNameInDocument() : nullptr
    );
}

bool gravityIndicatorVisible(MbDFEM::MbDGravity* gravity, ViewProviderMbDGravity* viewProvider)
{
    return gravity && gravity->Visibility.getValue() && viewProvider
        && viewProvider->Visibility.getValue() && viewProvider->ShowArrow.getValue()
        && owningAssembly(gravity) && gravity->gravity.getValue().Length() > minGravityMagnitude;
}

void updateGravityIndicatorByName(const std::string& documentName, const std::string& objectName)
{
    auto* document = App::GetApplication().getDocument(documentName.c_str());
    auto* gravity = document
        ? freecad_cast<MbDFEM::MbDGravity*>(document->getObject(objectName.c_str()))
        : nullptr;
    auto* guiDocument = document ? Gui::Application::Instance->getDocument(document) : nullptr;
    auto* viewProvider = gravity && guiDocument
        ? dynamic_cast<ViewProviderMbDGravity*>(guiDocument->getViewProvider(gravity))
        : nullptr;
    const auto gravityVector = gravity ? gravity->gravity.getValue() : Base::Vector3d();

    updateGravityIndicator(
        gravity,
        gravityIndicatorVisible(gravity, viewProvider),
        false,
        SbVec3f(static_cast<float>(gravityVector.x),
                static_cast<float>(gravityVector.y),
                static_cast<float>(gravityVector.z))
    );
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

    // MbDAssembly::ensureGravity() assigns the hidden owner link after addObject()
    // returns, so this view provider can attach before owningAssembly() succeeds.
    const std::string documentName = object && object->getDocument()
        ? object->getDocument()->getName()
        : "";
    const std::string objectName = object ? object->getNameInDocument() : "";
    QTimer::singleShot(0, [documentName, objectName]() {
        updateGravityIndicatorByName(documentName, objectName);
    });
    QTimer::singleShot(100, [documentName, objectName]() {
        updateGravityIndicatorByName(documentName, objectName);
    });
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
    return gravityIndicatorVisible(gravity, const_cast<ViewProviderMbDGravity*>(this));
}

void ViewProviderMbDGravity::updateCornerGravityIndicator(bool forceHidden)
{
    auto* gravity = getObject<MbDFEM::MbDGravity>();
    if (!forceHidden && !owningAssembly(gravity)) {
        return;
    }

    const auto gravityVector = gravity ? gravity->gravity.getValue() : Base::Vector3d();
    updateGravityIndicator(
        gravity,
        arrowVisible(),
        forceHidden,
        SbVec3f(static_cast<float>(gravityVector.x),
                static_cast<float>(gravityVector.y),
                static_cast<float>(gravityVector.z))
    );
}
