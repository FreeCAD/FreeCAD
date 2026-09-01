// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2021 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
 *   Copyright (c) 2026 Turan Furkan Topak                                 *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include <FCConfig.h>

#include <memory>
#include <optional>
#include <string>

#include <QString>

#include <Inventor/SbVec3f.h>
#include <Inventor/SoPath.h>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/nodes/SoInfo.h>
#include <Inventor/nodes/SoPickStyle.h>
#include <Inventor/nodes/SoSeparator.h>

#include <Base/Placement.h>
#include <Base/Vector3D.h>
#include <Gui/SoDatumLabel.h>
#include <Mod/Sketcher/App/Constraint.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include "EditModeConstraintCoinManager.h"
#include "DimensionDatumPlacement.h"
#include "ViewProviderSketch.h"
#include "ViewProviderSketchCoinAttorney.h"

namespace SketcherGui
{

void EditModeConstraintCoinManager::setDimensionOptions(const std::vector<DimensionOption>& options)
{
    dimensionOptionList = options;

    if (dimensionOptionActive >= static_cast<int>(dimensionOptionList.size())) {
        dimensionOptionActive = -1;
    }

    rebuildDimensionOptionNodes();
}

bool EditModeConstraintCoinManager::setActiveDimensionOption(int index)
{
    if (index < 0 || index >= static_cast<int>(dimensionOptionList.size())) {
        index = -1;
    }

    if (dimensionOptionActive == index) {
        return false;
    }

    dimensionOptionActive = index;
    rebuildDimensionOptionNodes();
    return true;
}

std::optional<EditModeConstraintCoinManager::PreparedPreviewDatum> EditModeConstraintCoinManager::preparePreviewDatum(
    const DimensionOption& option
) const
{
    auto* sketch = viewProvider.getSketchObject();
    if (!sketch) {
        return std::nullopt;
    }

    auto constraint = buildDimensionConstraint(*sketch, option);
    if (!constraint) {
        return std::nullopt;
    }
    const bool radial = constraint->Type == Sketcher::Radius
        || constraint->Type == Sketcher::Diameter;
    const bool placementPrepared = option.customLabelPosition
        ? prepareDimensionDatumPlacement(*sketch, *constraint, *option.customLabelPosition)
        : radial || prepareDimensionDatumPlacement(*sketch, *constraint);
    if (!placementPrepared) {
        return std::nullopt;
    }
    constraint->isActive = true;

    auto datum = createDimensionDatumLabel(*constraint, DatumLabelKind::Preview);
    if (!configurePreviewDatumLabel(*constraint, *datum)) {
        return std::nullopt;
    }

    return PreparedPreviewDatum {std::move(constraint), std::move(datum)};
}

std::optional<DimensionOption> EditModeConstraintCoinManager::resolveDimensionOption(int index) const
{
    const auto& options = dimensionOptionList;
    if (index < 0 || index >= static_cast<int>(options.size())) {
        return std::nullopt;
    }

    DimensionOption option = options[index];

    const auto preview = preparePreviewDatum(option);
    if (!preview) {
        return std::nullopt;
    }

    option.preparedDatumPlacement = DimensionOption::DatumPlacement {
        preview->constraint->LabelDistance,
        preview->constraint->LabelPosition,
    };
    return option;
}

Gui::CoinPtr<Gui::SoDatumLabel> EditModeConstraintCoinManager::createDimensionDatumLabel(
    const Sketcher::Constraint& constraint,
    DatumLabelKind kind
) const
{
    Gui::CoinPtr<Gui::SoDatumLabel> datum {new Gui::SoDatumLabel};
    const bool preview = kind == DatumLabelKind::Preview;

    Base::Vector3d sketchNormal(0.0, 0.0, 1.0);
    Base::Placement placement = ViewProviderSketchCoinAttorney::getEditingPlacement(viewProvider);
    Base::Rotation rotation(placement.getRotation());
    rotation.multVec(sketchNormal, sketchNormal);

    datum->norm.setValue(SbVec3f(sketchNormal.x, sketchNormal.y, sketchNormal.z));
    datum->string = "";
    datum->textColor = kind == DatumLabelKind::DeactivatedConstraint
        ? drawingParameters.DeactivatedConstrDimColor
        : preview ? drawingParameters.CursorTextColor
                  : (constraint.isDriving ? drawingParameters.ConstrDimColor
                                          : drawingParameters.NonDrivingConstrDimColor);

    if (!drawingParameters.labelFontName.isEmpty()) {
        datum->name.setValue(drawingParameters.labelFontName.toStdString().c_str());
    }
    datum->size.setValue(drawingParameters.labelFontSize);
    constexpr float previewLineWidth = 1.0F;
    datum->lineWidth = (preview ? previewLineWidth : drawingParameters.DimensionalConstraintLineWidth)
        * drawingParameters.pixelScalingFactor;
    datum->linePattern = drawingParameters.DimensionalConstraintLinePattern;
    datum->useAntialiasing = false;
    datum->strikethrough = false;
    return datum;
}

int EditModeConstraintCoinManager::pickDimensionOption(const SoPickedPoint* point) const
{
    if (!point || !dimensionOptionRoot) {
        return -1;
    }

    SoPath* path = point->getPath();
    if (!path) {
        return -1;
    }

    for (int pathIndex = 0; pathIndex + 1 < path->getLength(); ++pathIndex) {
        if (path->getNode(pathIndex) != dimensionOptionRoot) {
            continue;
        }

        auto* optionSeparator = dynamic_cast<SoSeparator*>(path->getNode(pathIndex + 1));
        if (!optionSeparator) {
            return -1;
        }

        for (int childIndex = 0; childIndex < optionSeparator->getNumChildren(); ++childIndex) {
            auto* optionIdNode = dynamic_cast<SoInfo*>(optionSeparator->getChild(childIndex));
            if (!optionIdNode) {
                continue;
            }

            bool ok = false;
            const int optionIndex
                = QString::fromLatin1(optionIdNode->string.getValue().getString()).toInt(&ok);
            if (ok && optionIndex >= 0 && optionIndex < static_cast<int>(dimensionOptionList.size())) {
                return optionIndex;
            }
            return -1;
        }
    }

    return -1;
}

void EditModeConstraintCoinManager::ensureDimensionOptionRoot()
{
    if (dimensionOptionRoot) {
        return;
    }

    if (!editModeScenegraphNodes.EditRoot) {
        return;
    }

    dimensionOptionRoot = new SoSeparator;
    dimensionOptionRoot->setName("DimensionOptionRoot");

    int insertIndex = -1;
    for (int i = 0; i < editModeScenegraphNodes.EditRoot->getNumChildren(); ++i) {
        if (editModeScenegraphNodes.EditRoot->getChild(i) == editModeScenegraphNodes.constrGroup) {
            insertIndex = i + 1;
            break;
        }
    }

    if (insertIndex >= 0) {
        editModeScenegraphNodes.EditRoot->insertChild(dimensionOptionRoot, insertIndex);
    }
    else {
        editModeScenegraphNodes.EditRoot->addChild(dimensionOptionRoot);
    }
}

void EditModeConstraintCoinManager::rebuildDimensionOptionNodes()
{
    ensureDimensionOptionRoot();
    if (!dimensionOptionRoot) {
        return;
    }

    Gui::coinRemoveAllChildren(dimensionOptionRoot);

    const auto& options = dimensionOptionList;
    for (int i = 0; i < static_cast<int>(options.size()); ++i) {
        auto preview = preparePreviewDatum(options[i]);
        if (!preview) {
            continue;
        }

        auto* sep = new SoSeparator;
        sep->renderCaching = SoSeparator::OFF;

        auto* pickStyle = new SoPickStyle;
        pickStyle->style = SoPickStyle::SHAPE;
        sep->addChild(pickStyle);

        sep->addChild(editModeScenegraphNodes.ConstraintDrawStyle);

        if (i == dimensionOptionActive) {
            preview->datum->textColor = drawingParameters.PreselectColor;
        }
        sep->addChild(preview->datum.get());

        auto* optionIdNode = new SoInfo;
        optionIdNode->string = SbString(std::to_string(i).c_str());
        sep->addChild(optionIdNode);

        dimensionOptionRoot->addChild(sep);
    }
}

}  // namespace SketcherGui
