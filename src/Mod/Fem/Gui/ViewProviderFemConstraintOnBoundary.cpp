/***************************************************************************
 *   Copyright (c) 2022 FreeCAD Developers                                 *
 *   Author: Ajinkya Dahale <dahale.a.p@gmail.com>                         *
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

#include "PreCompiled.h"

#include "Mod/Fem/App/FemConstraint.h"
#include <Gui/Application.h>
#include <Mod/Part/Gui/ReferenceHighlighter.h>
#include <Mod/Part/Gui/ViewProvider.h>

#include "TaskFemConstraintOnBoundary.h"
#include "ViewProviderFemConstraintOnBoundary.h"


using namespace FemGui;

PROPERTY_SOURCE(FemGui::ViewProviderFemConstraintOnBoundary, FemGui::ViewProviderFemConstraint)

ViewProviderFemConstraintOnBoundary::ViewProviderFemConstraintOnBoundary() = default;

ViewProviderFemConstraintOnBoundary::~ViewProviderFemConstraintOnBoundary() = default;

void ViewProviderFemConstraintOnBoundary::highlightReferences(const bool on)
{
    Fem::Constraint* pcConstraint = static_cast<Fem::Constraint*>(this->getObject());
    const auto& subSets = pcConstraint->References.getSubListValues();

    for (auto& subSet : subSets) {
        Part::Feature* base = dynamic_cast<Part::Feature*>(subSet.first);
        if (!base) {
            continue;
        }
        PartGui::ViewProviderPart* vp = dynamic_cast<PartGui::ViewProviderPart*>(
            Gui::Application::Instance->getViewProvider(base));
        if (!vp) {
            continue;
        }

        // if somehow the subnames are empty, clear any existing colors
        if (on && !subSet.second.empty()) {
            // identify the type of subelements
            // TODO: Assumed here the subelements are of the same type.
            // It is a requirement but we should keep safeguards.
            if (subSet.second[0].find("Vertex") != std::string::npos) {
                // make sure original colors are remembered
                if (originalPointColors[base].empty()) {
                    originalPointColors[base] = vp->PointColorArray.getValues();
                }
                std::vector<App::Color> colors = originalPointColors[base];

                // go through the subelements with constraint and recolor them
                // TODO: Replace `ShapeColor` with anything more appropriate
                PartGui::ReferenceHighlighter highlighter(base->Shape.getValue(),
                                                          colors.empty() ? ShapeColor.getValue()
                                                                         : colors[0]);
                highlighter.getVertexColors(subSet.second, colors);
                vp->PointColorArray.setValues(colors);
            }
            else if (subSet.second[0].find("Edge") != std::string::npos) {
                // make sure original colors are remembered
                if (originalLineColors[base].empty()) {
                    originalLineColors[base] = vp->LineColorArray.getValues();
                }
                std::vector<App::Color> colors = originalLineColors[base];

                // go through the subelements with constraint and recolor them
                // TODO: Replace `ShapeColor` with anything more appropriate
                PartGui::ReferenceHighlighter highlighter(base->Shape.getValue(),
                                                          colors.empty() ? ShapeColor.getValue()
                                                                         : colors[0]);
                highlighter.getEdgeColors(subSet.second, colors);
                vp->LineColorArray.setValues(colors);
            }
            else if (subSet.second[0].find("Face") != std::string::npos) {
                // make sure original colors are remembered
                if (originalFaceColors[base].empty()) {
                    originalFaceColors[base] = vp->DiffuseColor.getValues();
                }
                std::vector<App::Color> colors = originalFaceColors[base];

                // go through the subelements with constraint and recolor them
                // TODO: Replace `FaceColor` with anything more appropriate
                PartGui::ReferenceHighlighter highlighter(base->Shape.getValue(),
                                                          colors.empty() ? FaceColor.getValue()
                                                                         : colors[0]);
                highlighter.getFaceColors(subSet.second, colors);
                vp->DiffuseColor.setValues(colors);
            }
        }
        else {
            if (!originalPointColors[base].empty()) {
                vp->PointColorArray.setValues(originalPointColors[base]);
                originalPointColors[base].clear();
            }
            else if (!originalLineColors[base].empty()) {
                vp->LineColorArray.setValues(originalLineColors[base]);
                originalLineColors[base].clear();
            }
            else if (!originalFaceColors[base].empty()) {
                vp->DiffuseColor.setValues(originalFaceColors[base]);
                originalFaceColors[base].clear();
            }
        }
    }

    if (subSets.empty()) {
        // there is nothing selected but previous selection may have highlighting
        // reset that highlighting here
        for (auto& ogPair : originalPointColors) {
            if (ogPair.second.empty()) {
                continue;
            }
            PartGui::ViewProviderPart* vp = dynamic_cast<PartGui::ViewProviderPart*>(
                Gui::Application::Instance->getViewProvider(ogPair.first));
            if (!vp) {
                continue;
            }

            vp->PointColorArray.setValues(ogPair.second);
            ogPair.second.clear();
        }

        for (auto& ogPair : originalLineColors) {
            if (ogPair.second.empty()) {
                continue;
            }
            PartGui::ViewProviderPart* vp = dynamic_cast<PartGui::ViewProviderPart*>(
                Gui::Application::Instance->getViewProvider(ogPair.first));
            if (!vp) {
                continue;
            }

            vp->LineColorArray.setValues(ogPair.second);
            ogPair.second.clear();
        }

        for (auto& ogPair : originalFaceColors) {
            if (ogPair.second.empty()) {
                continue;
            }
            PartGui::ViewProviderPart* vp = dynamic_cast<PartGui::ViewProviderPart*>(
                Gui::Application::Instance->getViewProvider(ogPair.first));
            if (!vp) {
                continue;
            }

            vp->DiffuseColor.setValues(ogPair.second);
            ogPair.second.clear();
        }
    }
}
