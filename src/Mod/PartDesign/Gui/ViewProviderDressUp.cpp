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

#include <string_view>

#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopExp.hxx>

#include <Gui/Application.h>
#include <Mod/Part/Gui/ReferenceHighlighter.h>
#include <Mod/PartDesign/App/FeatureDressUp.h>
#include <Base/ServiceProvider.h>
#include <Gui/Utilities.h>

#include "ViewProviderDressUp.h"
#include "StyleParameters.h"
#include "TaskDressUpParameters.h"
#include "TopExp_Explorer.hxx"
#include "TopoDS.hxx"


using namespace std::literals::string_view_literals;

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

    const std::vector<std::string> refs = pdDressUp->Base.getSubValues();

    if (!on) {
        vp->unsetHighlightedFaces();
        vp->unsetHighlightedEdges();
        return;
    }

    std::vector<std::string> faces;
    std::vector<std::string> edges;

    const TopoDS_Shape& shape = base->Shape.getValue();

    TopTools_IndexedMapOfShape allFaces;
    TopTools_IndexedMapOfShape allEdges;
    TopTools_IndexedMapOfShape allSolids;

    TopExp::MapShapes(shape, TopAbs_FACE, allFaces);
    TopExp::MapShapes(shape, TopAbs_EDGE, allEdges);
    TopExp::MapShapes(shape, TopAbs_SOLID, allSolids);

    std::set<int> solidIndices;

    const auto parseIndex = [](std::string_view value) -> int {
        int index = 0;

        const auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), index);

        if (ec != std::errc {} || ptr != value.data() + value.size()) {
            return 0;
        }

        return index;
    };

    const auto addSolidForShape =
        [&solidIndices, &allSolids](const TopoDS_Shape& selected, const TopAbs_ShapeEnum type) {
            for (int i = 1; i <= allSolids.Extent(); ++i) {
                for (TopExp_Explorer exp(allSolids(i), type); exp.More(); exp.Next()) {

                    if (selected.IsSame(exp.Current())) {
                        solidIndices.insert(i);
                        break;
                    }
                }
            }
        };

    for (const std::string& ref : refs) {
        const std::string_view refView {ref};

        if (refView.starts_with("Solid"sv)) {
            const int solidIndex = parseIndex(refView.substr(5));

            if (solidIndex >= 1 && solidIndex <= allSolids.Extent()) {
                solidIndices.insert(solidIndex);
            }

            continue;
        }

        if (refView.starts_with("Face"sv)) {
            const int faceIndex = parseIndex(refView.substr(4));

            if (faceIndex < 1 || faceIndex > allFaces.Extent()) {
                continue;
            }

            if (highlightFacesAsSolid) {
                addSolidForShape(allFaces(faceIndex), TopAbs_FACE);
            }
            else {
                faces.emplace_back(ref);
            }

            continue;
        }

        if (refView.starts_with("Edge"sv)) {
            const int edgeIndex = parseIndex(refView.substr(4));

            if (edgeIndex < 1 || edgeIndex > allEdges.Extent()) {
                continue;
            }

            if (highlightEdgesAsSolid) {
                addSolidForShape(allEdges(edgeIndex), TopAbs_EDGE);
            }
            else {
                edges.emplace_back(ref);
            }
        }
    }

    // A solid reference always means all of its faces.
    // When highlightAsSolid is enabled, Face/Edge references
    // have also populated solidIndices above.
    for (const int solidIndex : solidIndices) {
        const TopoDS_Shape& solid = allSolids(solidIndex);

        for (TopExp_Explorer exp(solid, TopAbs_FACE); exp.More(); exp.Next()) {
            const int faceIndex = allFaces.FindIndex(exp.Current());

            if (faceIndex > 0) {
                faces.emplace_back("Face" + std::to_string(faceIndex));
            }
        }
    }

    std::ranges::sort(faces);
    faces.erase(std::ranges::unique(faces).begin(), faces.end());

    if (!faces.empty()) {
        std::vector<App::Material> materials = vp->ShapeAppearance.getValues();

        PartGui::ReferenceHighlighter highlighter(shape, ShapeAppearance.getDiffuseColor());

        highlighter.getFaceMaterials(faces, materials);
        vp->setHighlightedFaces(materials);
    }

    if (!edges.empty()) {
        std::vector<Base::Color> colors = vp->LineColorArray.getValues();

        PartGui::ReferenceHighlighter highlighter(shape, LineColor.getValue());

        highlighter.getEdgeColors(edges, colors);
        vp->setHighlightedEdges(colors);
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
