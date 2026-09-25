// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2012 Jan Rheinländer                                    *
 *                                   <jrheinlaender@users.sourceforge.net> *
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


#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopExp.hxx>


#include <Gui/Application.h>
#include <Mod/Part/Gui/ReferenceHighlighter.h>
#include <Mod/PartDesign/App/FeatureDressUp.h>

#include "ViewProviderDressUp.h"

#include "StyleParameters.h"
#include "TaskDressUpParameters.h"
#include "TopExp_Explorer.hxx"

#include <Base/ServiceProvider.h>
#include <Gui/Utilities.h>

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderDressUp, PartDesignGui::ViewProvider)


void ViewProviderDressUp::attach(App::DocumentObject* pcObject)
{
    ViewProvider::attach(pcObject);

    setErrorState(false);
}

void ViewProviderDressUp::updatePreviewColor()
{
    auto* styleParameterManager = Base::provideService<Gui::StyleParameters::ParameterManager>();
    PreviewColor.setValue(styleParameterManager->resolve(StyleParameters::PreviewDressUpColor));
}

void ViewProviderDressUp::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    QString text = QString::fromStdString(getObject()->Label.getStrValue());
    addDefaultAction(menu, QObject::tr("Edit %1").arg(text));
    PartDesignGui::ViewProvider::setupContextMenu(menu, receiver, member);
}

const std::string& ViewProviderDressUp::featureName() const
{
    static const std::string name = "Undefined";
    return name;
}

std::string ViewProviderDressUp::featureIcon() const
{
    return std::string("PartDesign_") + featureName();
}


bool ViewProviderDressUp::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default) {
        // Here we should prevent edit of a Feature with missing base
        // Otherwise it could call unhandled exception.
        PartDesign::DressUp* dressUp = getObject<PartDesign::DressUp>();
        assert(dressUp);
        if (dressUp->getBaseObject(/*silent =*/true)) {
            return ViewProvider::setEdit(ModNum);
        }
        else {
            QMessageBox::warning(
                nullptr,
                QObject::tr("Feature error"),
                QObject::tr(
                    "%1 misses a base feature.\n"
                    "This feature is broken and cannot be edited."
                )
                    .arg(QString::fromLatin1(dressUp->getNameInDocument()))
            );
            return false;
        }
    }
    else {
        return ViewProvider::setEdit(ModNum);
    }
}

void ViewProviderDressUp::highlightReferences(const bool on)
{
    const auto* pdDressUp = getObject<PartDesign::DressUp>();
    const Part::Feature* base = pdDressUp->getBaseObject(/*silent =*/true);
    if (!base) {
        return;
    }

    auto* vp = dynamic_cast<ViewProviderPart*>(Gui::Application::Instance->getViewProvider(base));
    if (!vp) {
        return;
    }

    std::vector<std::string> faces = pdDressUp->Base.getSubValuesStartsWith("Face");
    std::vector<std::string> edges = pdDressUp->Base.getSubValuesStartsWith("Edge");

    if (on) {
        if (!faces.empty()) {
            std::vector<App::Material> materials = vp->ShapeAppearance.getValues();

            if (faceAsSolids) {
                const TopoDS_Shape& shape = base->Shape.getValue();

                // Map each face to the solid containing it.
                std::map<int, int> faceToSolid;
                int solidIndex = 0;

                for (TopExp_Explorer solidExp(shape, TopAbs_SOLID); solidExp.More();
                     solidExp.Next(), ++solidIndex) {

                    int faceIndex = 0;

                    for (TopExp_Explorer shapeFaceExp(shape, TopAbs_FACE); shapeFaceExp.More();
                         shapeFaceExp.Next(), ++faceIndex) {

                        for (TopExp_Explorer solidFaceExp(solidExp.Current(), TopAbs_FACE);
                             solidFaceExp.More();
                             solidFaceExp.Next()) {

                            if (shapeFaceExp.Current().IsSame(solidFaceExp.Current())) {
                                faceToSolid[faceIndex] = solidIndex;
                                break;
                            }
                        }
                    }
                }

                // Find which solids contain the referenced faces.
                std::set<int> selectedSolids;

                for (const auto& faceName : faces) {
                    const int faceIndex = std::stoi(faceName.substr(4)) - 1;

                    const auto it = faceToSolid.find(faceIndex);
                    if (it != faceToSolid.end()) {
                        selectedSolids.insert(it->second);
                    }
                }

                // Add all faces belonging to selected solids.
                int faceIndex = 0;

                for (TopExp_Explorer shapeFaceExp(shape, TopAbs_FACE); shapeFaceExp.More();
                     shapeFaceExp.Next(), ++faceIndex) {

                    const auto it = faceToSolid.find(faceIndex);
                    if (it != faceToSolid.end() && selectedSolids.contains(it->second)) {
                        faces.emplace_back("Face" + std::to_string(faceIndex + 1));
                    }
                }

                std::sort(faces.begin(), faces.end());
                faces.erase(std::unique(faces.begin(), faces.end()), faces.end());
            }

            PartGui::ReferenceHighlighter highlighter(
                base->Shape.getValue(),
                ShapeAppearance.getDiffuseColor()
            );
            highlighter.getFaceMaterials(faces, materials);

            vp->setHighlightedFaces(materials);
        }

        if (!edges.empty()) {
            std::vector<Base::Color> colors = vp->LineColorArray.getValues();

            PartGui::ReferenceHighlighter highlighter(base->Shape.getValue(), LineColor.getValue());
            highlighter.getEdgeColors(edges, colors);

            vp->setHighlightedEdges(colors);
        }
    }
    else {
        vp->unsetHighlightedFaces();
        vp->unsetHighlightedEdges();
    }
}

void ViewProviderDressUp::setErrorState(bool error)
{
    auto* styleParameterManager = Base::provideService<Gui::StyleParameters::ParameterManager>();

    const float opacity = static_cast<float>(
        styleParameterManager
            ->resolve(error ? StyleParameters::PreviewErrorOpacity : StyleParameters::PreviewShapeOpacity)
            .value
    );

    pcPreviewShape->transparency = 1.0F - opacity;
    pcPreviewShape->color = error
        ? styleParameterManager->resolve(StyleParameters::PreviewErrorColor).asValue<SbColor>()
        : PreviewColor.getValue().asValue<SbColor>();
}
