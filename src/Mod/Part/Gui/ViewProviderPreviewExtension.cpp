// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Kacper Donat <kacper@kadet.net>                     *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoPickStyle.h>
#include <Inventor/nodes/SoPolygonOffset.h>
#include <Inventor/nodes/SoTransform.h>

#include "ViewProviderPreviewExtension.h"
#include "ViewProviderExt.h"

#include <App/Document.h>
#include <Gui/Utilities.h>
#include <Gui/Inventor/So3DAnnotation.h>
#include <Mod/Part/App/PreviewExtension.h>
#include <Mod/Part/App/Tools.h>

using namespace PartGui;

SO_NODE_SOURCE(SoPreviewShape);

const SbColor SoPreviewShape::defaultColor = SbColor(1.F, 0.F, 1.F);

SoPreviewShape::SoPreviewShape()
{
    SO_NODE_CONSTRUCTOR(SoPreviewShape);

    SO_NODE_ADD_FIELD(color, (defaultColor));
    SO_NODE_ADD_FIELD(transparency, (defaultTransparency));
    SO_NODE_ADD_FIELD(lineWidth, (defaultLineWidth));
    SO_NODE_ADD_FIELD(transform, (SbMatrix::identity()));

    pcTransform = new SoMatrixTransform;
    pcTransform->matrix.connectFrom(&transform);

    auto pickStyle = new SoPickStyle;
    pickStyle->style = SoPickStyle::UNPICKABLE;

    auto* solidLineStyle = new SoDrawStyle();
    solidLineStyle->lineWidth.connectFrom(&lineWidth);

    auto* hiddenLineStyle = new SoDrawStyle();
    hiddenLineStyle->lineWidth.connectFrom(&lineWidth);
    hiddenLineStyle->linePattern = 0xF0F0;

    auto* solidColorLightModel = new SoLightModel();
    solidColorLightModel->model = SoLightModel::BASE_COLOR;

    auto* normalBinding = new SoNormalBinding();
    normalBinding->value = SoNormalBinding::PER_VERTEX_INDEXED;

    // This should be OVERALL but then line pattern does not work correctly
    // Probably a bug in coin to be investigated.
    auto* materialBinding = new SoMaterialBinding();
    materialBinding->value = SoMaterialBinding::PER_FACE_INDEXED;

    auto* material = new SoMaterial;
    material->diffuseColor.connectFrom(&color);
    material->transparency.connectFrom(&transparency);

    auto* polygonOffset = new SoPolygonOffset;
    polygonOffset->factor = -0.00001F;
    polygonOffset->units = -1.0F;
    polygonOffset->on = true;
    polygonOffset->styles = SoPolygonOffset::FILLED;

    auto* lineMaterial = new SoMaterial;
    lineMaterial->diffuseColor.connectFrom(&color);
    lineMaterial->transparency = 0.0f;

    auto* lineSep = new SoSeparator;
    lineSep->addChild(normalBinding);
    lineSep->addChild(materialBinding);
    lineSep->addChild(solidColorLightModel);
    lineSep->addChild(lineMaterial);
    lineSep->addChild(lineset);

    auto* annotation = new Gui::So3DAnnotation;
    annotation->addChild(hiddenLineStyle);
    annotation->addChild(material);
    annotation->addChild(lineSep);
    annotation->addChild(polygonOffset);
    annotation->addChild(faceset);

    SoSeparator::addChild(pcTransform);
    SoSeparator::addChild(pickStyle);
    SoSeparator::addChild(solidLineStyle);
    SoSeparator::addChild(material);
    SoSeparator::addChild(coords);
    SoSeparator::addChild(norm);
    SoSeparator::addChild(lineSep);
    SoSeparator::addChild(polygonOffset);
    SoSeparator::addChild(faceset);
    SoSeparator::addChild(annotation);
}

void SoPreviewShape::initClass()
{
    SO_NODE_INIT_CLASS(SoPreviewShape, SoSeparator, "Separator");
}

EXTENSION_PROPERTY_SOURCE(PartGui::ViewProviderPreviewExtension, Gui::ViewProviderExtension)

ViewProviderPreviewExtension::ViewProviderPreviewExtension()
{
    const Base::Color magenta(1.0F, 0.0F, 1.0F);

    EXTENSION_ADD_PROPERTY_TYPE(
        PreviewColor,
        (magenta),
        "Preview",
        static_cast<App::PropertyType>(App::Prop_Transient | App::Prop_Hidden),
        "Color used for 3D Preview"
    );

    initExtensionType(ViewProviderPreviewExtension::getExtensionClassTypeId());
}

void ViewProviderPreviewExtension::extensionAttach(App::DocumentObject* documentObject)
{
    ViewProviderExtension::extensionAttach(documentObject);

    pcPreviewRoot = new SoSeparator;
    pcPreviewShape = new SoPreviewShape;

    attachPreview();

    auto document = documentObject->getDocument();
    if (!document->testStatus(App::Document::Restoring)) {
        updatePreview();
    }
}

void ViewProviderPreviewExtension::extensionBeforeDelete()
{
    ViewProviderExtension::extensionBeforeDelete();

    showPreview(false);
}

void ViewProviderPreviewExtension::showPreview(bool enable)
{
    auto feature = getExtendedViewProvider()->getObject<Part::Feature>();
    if (!feature) {
        return;
    }

    auto previewExtension = feature->getExtensionByType<Part::PreviewExtension>(true);
    if (!previewExtension) {
        return;
    }

    _isPreviewEnabled = enable;

    auto annotationRoot = getExtendedViewProvider()->getAnnotation();
    if (enable) {
        previewExtension->updatePreview();

        if (annotationRoot->findChild(pcPreviewRoot) < 0) {
            annotationRoot->addChild(pcPreviewRoot);
        }
    }
    else {
        annotationRoot->removeChild(pcPreviewRoot);
    }
}

void ViewProviderPreviewExtension::extensionOnChanged(const App::Property* prop)
{
    if (prop == &PreviewColor) {
        pcPreviewShape->color.setValue(Base::convertTo<SbColor>(PreviewColor.getValue()));
    }

    ViewProviderExtension::extensionOnChanged(prop);
}

void ViewProviderPreviewExtension::attachPreview()
{
    pcPreviewRoot->addChild(pcPreviewShape);
}

void ViewProviderPreviewExtension::updatePreview()
{
    updatePreviewShape(getPreviewShape(), pcPreviewShape);
}

void ViewProviderPreviewExtension::updatePreviewShape(Part::TopoShape shape, SoPreviewShape* preview)
{
    if (shape.isNull() || preview == nullptr) {
        return;
    }

    auto vp = freecad_cast<ViewProviderPartExt*>(getExtendedViewProvider());

    if (!vp) {
        return;
    }

    const auto updatePreviewShape = [vp](SoPreviewShape* preview, Part::TopoShape shape) {
        ViewProviderPartExt::setupCoinGeometry(
            shape.getShape(),
            preview,
            vp->Deviation.getValue(),
            vp->AngularDeflection.getValue()
        );
    };

    try {
        updatePreviewShape(preview, shape);
        preview->transform.setValue(Base::convertTo<SbMatrix>(shape.getTransform()));
    }
    catch (Standard_Failure& e) {
        Base::Console().userTranslatedNotification(
            tr("Failure while rendering preview: %1. That usually indicates an error with model.")
                .arg(QString::fromUtf8(e.GetMessageString()))
                .toUtf8()
        );

        updatePreviewShape(preview, {});
    }

    // For some reason line patterns are not rendered correctly if material binding is set to
    // anything other than PER_FACE. PER_FACE material binding seems to require materialIndex per
    // each distinct edge. Until that is fixed, this code forces each edge to use the first
    // material.
    unsigned lineCoordsCount = preview->lineset->coordIndex.getNum();
    unsigned lineCount = 1;

    for (unsigned i = 0; i < lineCoordsCount; ++i) {
        if (preview->lineset->coordIndex[i] < 0) {
            lineCount++;
        }
    }

    preview->lineset->materialIndex.setNum(lineCount);
    for (unsigned i = 0; i < lineCount; ++i) {
        preview->lineset->materialIndex.set1Value(i, 0);
    }
}

namespace Gui
{
EXTENSION_PROPERTY_SOURCE_TEMPLATE(
    PartGui::ViewProviderPreviewExtensionPython,
    PartGui::ViewProviderPreviewExtension
)

// explicit template instantiation
template class PartGuiExport ViewProviderExtensionPythonT<PartGui::ViewProviderPreviewExtension>;
}  // namespace Gui
