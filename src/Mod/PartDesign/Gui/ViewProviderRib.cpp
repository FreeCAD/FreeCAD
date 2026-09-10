// SPDX-License-Identifier: LGPL-2.1-or-later
#include "ViewProviderRib.h"
#include "TaskRibParameters.h"
#include "RibStyleParameters.h"
#include <App/Document.h>
#include <Base/ServiceProvider.h>
#include <Mod/PartDesign/App/FeatureRib.h>
#include <Gui/Inventor/So3DAnnotation.h>
#include <Mod/Part/Gui/ViewProviderPreviewExtension.h>

namespace PartDesignGui
{
PROPERTY_SOURCE(PartDesignGui::ViewProviderRib, PartDesignGui::ViewProvider)

ViewProviderRib::ViewProviderRib()
    : profilePreview(new PartGui::SoPreviewShape)
    , extensionPreview(new PartGui::SoPreviewShape)
    , targetPreview(new PartGui::SoPreviewShape)
{
    sPixmap = "PartDesign_Rib.svg";
}

ViewProviderRib::~ViewProviderRib() = default;

Part::TopoShape ViewProviderRib::getPreviewShape()
{
    if (const auto rib = getObject<PartDesign::Rib>(); rib && !rib->isValid()) {
        return {};  // Never present the last successful wall as a failed new result.
    }

    return ViewProvider::getPreviewShape();
}

void ViewProviderRib::attachPreview()
{
    ViewProvider::attachPreview();

    auto annotation = new Gui::So3DAnnotation;
    annotation->addChild(profilePreview);
    annotation->addChild(extensionPreview);
    profilePreview->lineWidth = 4.0F;
    profilePreview->transparency = 1.0F;
    pcPreviewRoot->addChild(annotation);
    pcPreviewRoot->addChild(targetPreview);
    extensionPreview->lineWidth = 4.0F;
    targetPreview->transparency = .9F;
}

void ViewProviderRib::updatePreview()
{
    ViewProvider::updatePreview();

    auto manager = Base::provideService<Gui::StyleParameters::ParameterManager>();
    const auto extensionColor = manager->resolve(StyleParameters::PreviewRibExtensionColor);
    const auto targetColor = manager->resolve(StyleParameters::PreviewRibTargetColor);
    extensionPreview->color.setValue(extensionColor.r, extensionColor.g, extensionColor.b);
    targetPreview->color.setValue(targetColor.r, targetColor.g, targetColor.b);

    const auto profileColor = manager->resolve(StyleParameters::PreviewThinProfileColor);
    profilePreview->color.setValue(profileColor.r, profileColor.g, profileColor.b);
    PartDesign::TopoShape profile, extension, target;
    try {
        if (auto rib = getObject<PartDesign::Rib>(); rib && rib->Profile.getValue()) {
            if (App::GetApplication()
                    .GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/PartDesign/Preview")
                    ->GetBool("ShowProfilePreview", true)) {
                profile = rib->getInputProfile();
                profile.move(rib->getLocation().Inverted());
            }
            std::tie(extension, target) = rib->getConstructionPreview();
        }
    }
    catch (const Base::Exception&) {
        // A failed reference must not leave an obsolete construction overlay.
    }
    catch (const Standard_Failure&) {
    }

    updatePreviewShape(profile, profilePreview);
    updatePreviewShape(extension, extensionPreview);
    updatePreviewShape(target, targetPreview);
}

std::vector<App::DocumentObject*> ViewProviderRib::claimChildren() const
{
    auto profile = getObject<PartDesign::Rib>()->Profile.getValue();
    if (profile && !profile->isDerivedFrom<PartDesign::Feature>()) {
        return {profile};
    }
    return {};
}

void ViewProviderRib::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    addDefaultAction(menu, QObject::tr("Edit Rib"));
    ViewProvider::setupContextMenu(menu, receiver, member);
}

TaskDlgFeatureParameters* ViewProviderRib::getEditDialog()
{
    return new TaskDlgRibParameters(this);
}
}  // namespace PartDesignGui
