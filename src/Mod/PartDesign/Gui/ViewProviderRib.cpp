// SPDX-License-Identifier: LGPL-2.1-or-later
#include "ViewProviderRib.h"
#include "TaskRibParameters.h"
#include "StyleParameters.h"
#include <Base/ServiceProvider.h>
#include <Mod/PartDesign/App/FeatureRib.h>
#include <Gui/Inventor/So3DAnnotation.h>
#include <Mod/Part/Gui/ViewProviderPreviewExtension.h>

namespace PartDesignGui
{
PROPERTY_SOURCE(PartDesignGui::ViewProviderRib, PartDesignGui::ViewProviderPad)

ViewProviderRib::ViewProviderRib()
    : extensionPreview(new PartGui::SoPreviewShape)
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

    return ViewProviderPad::getPreviewShape();
}

void ViewProviderRib::attachPreview()
{
    ViewProviderPad::attachPreview();

    auto annotation = new Gui::So3DAnnotation;
    annotation->addChild(extensionPreview);
    pcPreviewRoot->addChild(annotation);
    pcPreviewRoot->addChild(targetPreview);
    extensionPreview->lineWidth = 4.0F;
    targetPreview->transparency = .9F;
}

void ViewProviderRib::updatePreview()
{
    ViewProviderPad::updatePreview();

    auto manager = Base::provideService<Gui::StyleParameters::ParameterManager>();
    const auto extensionColor = manager->resolve(StyleParameters::PreviewRibExtensionColor);
    const auto targetColor = manager->resolve(StyleParameters::PreviewRibTargetColor);
    extensionPreview->color.setValue(extensionColor.r, extensionColor.g, extensionColor.b);
    targetPreview->color.setValue(targetColor.r, targetColor.g, targetColor.b);

    PartDesign::TopoShape extension, target;
    try {
        if (auto rib = getObject<PartDesign::Rib>(); rib && rib->Profile.getValue()) {
            std::tie(extension, target) = rib->getConstructionPreview();
        }
    }
    catch (const Base::Exception&) {
        // A failed reference must not leave an obsolete construction overlay.
    }
    catch (const Standard_Failure&) {
    }

    updatePreviewShape(extension, extensionPreview);
    updatePreviewShape(target, targetPreview);
}

void ViewProviderRib::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    addDefaultAction(menu, QObject::tr("Edit Rib/Web"));
    ViewProviderSketchBased::setupContextMenu(menu, receiver, member);
}

TaskDlgFeatureParameters* ViewProviderRib::getEditDialog()
{
    return new TaskDlgRibParameters(this);
}
}  // namespace PartDesignGui
