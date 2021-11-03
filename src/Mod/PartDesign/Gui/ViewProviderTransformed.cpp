/******************************************************************************
 *   Copyright (c) 2012 Jan Rheinländer <jrheinlaender@users.sourceforge.net> *
 *                                                                            *
 *   This file is part of the FreeCAD CAx development system.                 *
 *                                                                            *
 *   This library is free software; you can redistribute it and/or            *
 *   modify it under the terms of the GNU Library General Public              *
 *   License as published by the Free Software Foundation; either             *
 *   version 2 of the License, or (at your option) any later version.         *
 *                                                                            *
 *   This library  is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Library General Public License for more details.                     *
 *                                                                            *
 *   You should have received a copy of the GNU Library General Public        *
 *   License along with this library; see the file COPYING.LIB. If not,       *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,            *
 *   Suite 330, Boston, MA  02111-1307, USA                                   *
 *                                                                            *
 ******************************************************************************/


#include "PreCompiled.h"

#ifndef _PreComp_
# include <Bnd_Box.hxx>
# include <BRep_Tool.hxx>
# include <BRepBndLib.hxx>
# include <BRepMesh_IncrementalMesh.hxx>
# include <Standard_Version.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS.hxx>
# include <Poly_Triangulation.hxx>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoDrawStyle.h>
# include <Inventor/nodes/SoIndexedFaceSet.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoMultipleCopy.h>
# include <Inventor/nodes/SoNormal.h>
# include <Inventor/nodes/SoPickStyle.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoShapeHints.h>
# include <Inventor/nodes/SoTransparencyType.h>
# include <QAction>
# include <QMenu>
# include <QMessageBox>
#endif

#include "ViewProviderTransformed.h"
#include "TaskTransformedParameters.h"
#include <Base/Console.h>
#include <Gui/Control.h>
#include <Gui/Command.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/SoFCUnifiedSelection.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/Gui/PartParams.h>
#include <Mod/PartDesign/App/FeatureTransformed.h>
#include <Mod/PartDesign/App/FeatureAddSub.h>
#include <Mod/PartDesign/App/FeatureMultiTransform.h>

using namespace PartDesignGui;
using namespace PartDesign;

PROPERTY_SOURCE(PartDesignGui::ViewProviderTransformed,PartDesignGui::ViewProviderAddSub)

void ViewProviderTransformed::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    QAction* act;
    act = menu->addAction(QObject::tr("Edit %1").arg(QString::fromStdString(featureName)), receiver, member);
    act->setData(QVariant((int)ViewProvider::Default));
    PartDesignGui::ViewProvider::setupContextMenu(menu, receiver, member);
}

Gui::ViewProvider *ViewProviderTransformed::startEditing(int ModNum) {
    PartDesign::Transformed* pcTransformed = static_cast<PartDesign::Transformed*>(getObject());
    if(!pcTransformed->Originals.getSize()) {
        for(auto obj : pcTransformed->getInList()) {
            if(obj->isDerivedFrom(PartDesign::MultiTransform::getClassTypeId())) {
                auto vp = Gui::Application::Instance->getViewProvider(obj);
                if(vp)
                    return vp->startEditing(ModNum);
                return 0;
            }
        }
    }
    return ViewProvider::startEditing(ModNum);
}

void ViewProviderTransformed::recomputeFeature(bool recompute)
{
    PartDesign::Transformed* pcTransformed = static_cast<PartDesign::Transformed*>(getObject());
    if(recompute || (pcTransformed->isError() || pcTransformed->mustExecute()))
        pcTransformed->recomputeFeature(true);
    const PartDesign::Transformed::rejectedMap &rejected_trsf = pcTransformed->getRejectedTransformations();

    QString msg;

    unsigned rejected = rejected_trsf.size();
    auto error = pcTransformed->getDocument()->getErrorDescription(pcTransformed);
    if(rejected>0 || error) {
        if (rejected>0) {
            msg = QString::fromLatin1("<font color='orange'>%1</font>");
            if (rejected == 1)
                msg = msg.arg(QObject::tr("One transformed shape is causing error"));
            else {
                msg = msg.arg(QObject::tr("%1 transformed shapes are causing error"));
                msg = msg.arg(rejected);
            }
        }
        if(error) {
            if(msg.size())
                msg += QLatin1String("</br></br>");
            msg += QString::fromLatin1("<font color='red'>%1</font>").arg(QObject::tr(error));
        }
    }

    diagMessage = msg;
    signalDiagnosis(msg);

    // TODO set rejected shape with lower transparency or some other color in the preview
}

void ViewProviderTransformed::updateAddSubShapeIndicator()
{
    auto view = getAddSubView();
    if (view) {
        view->DiffuseColor.setValue();
        view->PointColorArray.setValue();
        view->LineColorArray.setValue();
    }
    ViewProviderAddSub::updateAddSubShapeIndicator();
    checkAddSubColor();
}

void ViewProviderTransformed::checkAddSubColor()
{
    auto view = getAddSubView();
    if (!view)
        return;
    auto feat = Base::freecad_dynamic_cast<Transformed>(getObject());
    if (!feat)
        return;
    std::vector<std::pair<Part::TopoShape, FeatureAddSub::Type> > shapes;
    feat->getAddSubShape(shapes);
    if (shapes.empty())
        return;
    int addCount=0, subCount=0, intersectCount=0;
    for (auto &v : shapes) {
        if (v.second == FeatureAddSub::Additive)
            ++addCount;
        else if (v.second == FeatureAddSub::Subtractive)
            ++subCount;
        else if (v.second == FeatureAddSub::Intersecting)
            ++intersectCount;
    }
    App::Color addcolor((uint32_t)PartGui::PartParams::PreviewAddColor());
    App::Color subcolor((uint32_t)PartGui::PartParams::PreviewSubColor());
    App::Color intersectcolor((uint32_t)PartGui::PartParams::PreviewIntersectColor());
    if (!addCount) {
        if (!intersectCount)
            addcolor = subcolor;
        else if (!subCount)
            addcolor = intersectcolor;
    }
    // clamp transparency between 0.1 ~ 0.8
    float t = std::max(0.1f, std::min(0.8f, 1.0f - addcolor.a));
    if (!PartGui::PartParams::PreviewWithTransparency()) {
        t = 0.0f;
        previewGroup->priority = 0;
    } else
        previewGroup->priority = -2;

    view->LineColor.setValue(addcolor);
    auto material = view->PointMaterial.getValue();
    material.diffuseColor = addcolor;
    material.transparency = 1.0f;
    view->PointMaterial.setValue(material);
    view->ShapeColor.setValue(addcolor);
    view->Transparency.setValue(t*100);

    if ((!addCount && !subCount)
            || (!addCount && !intersectCount)
            || (!subCount && !intersectCount))
        return;

    auto addsubShape = feat->AddSubShape.getShape();
    std::array<std::vector<App::Color>, 2> colors;
    std::array<TopAbs_ShapeEnum, 2> types = {TopAbs_EDGE, TopAbs_FACE};
    App::PropertyColorList *props[] = {&view->LineColorArray,
                                       &view->DiffuseColor};
    int j=0;
    FeatureAddSub::Type majorType;
    App::Color color;
    if (addCount > subCount && addCount > intersectCount) {
        majorType = FeatureAddSub::Additive;
        color = addcolor;
    } else if (subCount > addCount && subCount > intersectCount) {
        majorType = FeatureAddSub::Subtractive;
        color = subcolor;
    } else {
        majorType = FeatureAddSub::Intersecting;
        color = intersectcolor;
    }

    addcolor.a = t;
    subcolor.a = t;
    intersectcolor.a = t;

    for (auto type : types)
        colors[j++].resize(addsubShape.countSubShapes(type), color);

    for (auto &v : shapes) {
        if (majorType == v.second)
            continue;
        auto &shape = v.first;
        j = 0;
        for (auto type : types) {
            for (auto &s : shape.getSubShapes(type)) {
                int idx = addsubShape.findShape(s);
                if (idx >= 1 && idx <= (int)colors[j].size()) {
                    colors[j][idx-1] = v.second == FeatureAddSub::Additive ? addcolor
                        : (v.second == FeatureAddSub::Subtractive ? subcolor : intersectcolor);
                }
            }
            auto prop = props[j];
            prop->setValues(colors[j]);
            j++;
        }
    }
}
