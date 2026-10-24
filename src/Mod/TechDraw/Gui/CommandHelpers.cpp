// SPDX-License-Identifier: LGPL-2.0-or-later

/***************************************************************************
 *   Copyright (c) 2024 WandererFan <wandererfan@gmail.com>                *
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

//! CommandHelpers is a collection of methods for common actions in commands

#include <QMessageBox>
#include <numbers>

#include <App/DocumentObject.h>
#include <App/Link.h>
#include <App/Application.h>
#include <App/Document.h>

#include <Gui/Command.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection/Selection.h>
#include <Gui/Selection/SelectionObject.h>
#include <Gui/Document.h>

#include <Mod/Part/App/Geometry2d.h>

#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawView.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/DrawViewSpreadsheet.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/Preferences.h>
#include <Mod/TechDraw/Gui/ViewProviderPage.h>
#include <Mod/TechDraw/Gui/QGSPage.h>
#include <Mod/TechDraw/App/CosmeticVertex.h>
#include <Mod/TechDraw/App/Cosmetic.h>
#include <Mod/TechDraw/App/CenterLine.h>
#include <Mod/TechDraw/App/LineFormat.h>
#include <Mod/TechDraw/App/Geometry.h>

#include <Mod/TechDraw/Gui/PreferencesGui.h>
#include <Mod/TechDraw/Gui/DrawGuiUtil.h>


#include "CommandHelpers.h"
#include "ViewProviderPage.h"

using namespace TechDraw;
using namespace TechDrawGui;
using DU = DrawUtil;

//! find the first DrawView in the current selection for use as a base view (owner)
TechDraw::DrawView* CommandHelpers::firstViewInSelection(Gui::Command* cmd)
{
    std::vector<Gui::SelectionObject> selection = cmd->getSelection().getSelectionEx();
    TechDraw::DrawView* baseView{nullptr};
    if (!selection.empty()) {
        for (auto& selobj : selection) {
            if (selobj.getObject()->isDerivedFrom<DrawView>()) {
                auto docobj = selobj.getObject();
                baseView =  static_cast<TechDraw::DrawView *>(docobj);
                break;
            }
        }
    }
    return baseView;
}

std::vector<std::string> CommandHelpers::getSelectedSubElements(Gui::Command* cmd,
                                                TechDraw::DrawViewPart* &dvp,
                                                std::string subType)
{
    std::vector<std::string> selectedSubs;
    std::vector<std::string> subNames;
    dvp = nullptr;
    std::vector<Gui::SelectionObject> selection = cmd->getSelection().getSelectionEx();
    std::vector<Gui::SelectionObject>::iterator itSel = selection.begin();
    for (; itSel != selection.end(); itSel++)  {
        if ((*itSel).getObject()->isDerivedFrom<TechDraw::DrawViewPart>()) {
            dvp = static_cast<TechDraw::DrawViewPart*> ((*itSel).getObject());
            subNames = (*itSel).getSubNames();
            break;
        }
    }
    if (!dvp) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                             QObject::tr("No part view in selection"));
        return selectedSubs;
    }

    for (auto& s: subNames) {
        if (TechDraw::DrawUtil::getGeomTypeFromName(s) == subType) {
            selectedSubs.push_back(s);
        }
    }

    if (selectedSubs.empty()) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Wrong selection"),
                             QObject::tr("No %1 in selection")
                                 .arg(QString::fromStdString(subType)));
        return selectedSubs;
    }

    return selectedSubs;
}


std::pair<App::DocumentObject*, std::string> CommandHelpers::faceFromSelection()
{
    auto selection = Gui::Selection().getSelectionEx(
        nullptr, App::DocumentObject::getClassTypeId(), Gui::ResolveMode::NoResolve);

    if (selection.empty()) {
        return { nullptr, "" };
    }

    for (auto& sel : selection) {
        for (auto& sub : sel.getSubNames()) {
            if (TechDraw::DrawUtil::getGeomTypeFromName(sub) == "Face") {
                return { sel.getObject(), sub };
            }
        }
    }

    return { nullptr, "" };
}

namespace TechDrawGui
{

LineFormat& _getActiveLineAttributes()
{
    return LineFormat::getCurrentLineFormat();
}

std::string _createBalloon(Gui::Command* cmd, TechDraw::DrawViewPart* objFeat)
// create a new balloon, return its name as string
{
    std::string featName;
    TechDraw::DrawPage* page = objFeat->findParentPage();
    Gui::Document* guiDoc = Gui::Application::Instance->getDocument(page->getDocument());
    auto pageVP = freecad_cast<ViewProviderPage*>(guiDoc->getViewProvider(page));
    if (pageVP) {
        QGSPage* scenePage = pageVP->getQGSPage();
        featName = scenePage->getDrawPage()->getDocument()->getUniqueObjectName("Balloon");
        std::string pageName = scenePage->getDrawPage()->getNameInDocument();
        cmd->doCommand(cmd->Doc,
                       "App.activeDocument().addObject('TechDraw::DrawViewBalloon', '%s')",
                       featName.c_str());
        cmd->doCommand(cmd->Doc, "App.activeDocument().%s.SourceView = (App.activeDocument().%s)",
                       featName.c_str(), objFeat->getNameInDocument());

        cmd->doCommand(cmd->Doc, "App.activeDocument().%s.addView(App.activeDocument().%s)",
                       pageName.c_str(), featName.c_str());
    }
    return featName;
}

bool _checkSel(Gui::Command* cmd, std::vector<Gui::SelectionObject>& selection,
               TechDraw::DrawViewPart*& objFeat, const std::string& message)
{
    // check selection of getSelectionEx() and selection[0].getObject()
    selection = cmd->getSelection().getSelectionEx();
    if (selection.empty()) {
        // message is translated in caller
        QMessageBox::warning(Gui::getMainWindow(), QString::fromUtf8(message.c_str()),
                             QObject::tr("Selection is empty"));
        return false;
    }

    objFeat = dynamic_cast<TechDraw::DrawViewPart*>(selection[0].getObject());
    if (!objFeat) {
        QMessageBox::warning(Gui::getMainWindow(), QString::fromUtf8(message.c_str()),
                             QObject::tr("No object selected"));
        return false;
    }

    return true;
}

//! return the vertices in the selection as [Base::Vector3d]
std::vector<Base::Vector3d> _getVertexPoints(const std::vector<std::string>& SubNames,
                                             TechDraw::DrawViewPart* objFeat)
{
    std::vector<Base::Vector3d> vertexPoints;
    for (const std::string& Name : SubNames) {
        std::string GeoType = TechDraw::DrawUtil::getGeomTypeFromName(Name);
        if (GeoType == "Vertex") {
            int GeoId = TechDraw::DrawUtil::getIndexFromName(Name);
            TechDraw::VertexPtr vert = objFeat->getProjVertexByIndex(GeoId);
            vertexPoints.push_back(vert->point());
        }
    }
    return vertexPoints;
}

//! get angle between x-axis and the vector from center to point.
//! result is [0, 360]
double _getAngle(Base::Vector3d center, Base::Vector3d point)
{
    constexpr double DegreesHalfCircle{180.0};
    Base::Vector3d vecCP = point - center;
    double angle = DU::angleWithX(vecCP) * DegreesHalfCircle / std::numbers::pi;
    return angle;
}

Base::Vector3d _circleCenter(Base::Vector3d p1, Base::Vector3d p2, Base::Vector3d p3)
{
    Base::Vector2d v1(p1.x, p1.y);
    Base::Vector2d v2(p2.x, p2.y);
    Base::Vector2d v3(p3.x, p3.y);
    Base::Vector2d center = Part::Geom2dCircle::getCircleCenter(v1, v2, v3);
    return Base::Vector3d(center.x, center.y, 0.0);
}

std::string _createThreadCircle(const std::string Name, TechDraw::DrawViewPart* objFeat, double factor)
{
    constexpr double ArcStartDegree{15.0};
    constexpr double ArcEndDegree{285.0};
    // create the 3/4 arc symbolizing a thread from top seen
    double scale = objFeat->getScale();
    int GeoId = TechDraw::DrawUtil::getIndexFromName(Name);
    TechDraw::BaseGeomPtr geom = objFeat->getGeomByIndex(GeoId);
    std::string GeoType = TechDraw::DrawUtil::getGeomTypeFromName(Name);

    if (GeoType == "Edge" && geom->getGeomType() == GeomType::CIRCLE) {
        TechDraw::CirclePtr cgen = std::static_pointer_cast<TechDraw::Circle>(geom);
        // center is rotated and scaled
        Base::Vector3d center = CosmeticVertex::makeCanonicalPointInverted(objFeat, cgen->center);
        // radius is scaled
        float radius = cgen->radius * factor / scale;
        TechDraw::BaseGeomPtr threadArc =
            std::make_shared<TechDraw::AOC>(center, radius, ArcStartDegree, ArcEndDegree);
        std::string arcTag = objFeat->addCosmeticEdge(threadArc);
        TechDraw::CosmeticEdge* arc = objFeat->getCosmeticEdge(arcTag);
        int solidStyle = 1; // Qt::SolidLine
        float thinWeight = (float)TechDraw::DrawUtil::getDefaultLineWeight("Thin");
        Base::Color threadColor = _getActiveLineAttributes().getColor(); 
        _setLineAttributes(arc, solidStyle, thinWeight, threadColor);
        return arcTag;
    } else {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("TechDraw create thread circle"),
                             QObject::tr("Can not make thread circle for %1")
                                 .arg(QString::fromStdString(GeometryUtils::getGeomTypeName(geom->getGeomType()))));
    }
    return "";
}

void _createThreadLines(const std::vector<std::string>& SubNames, TechDraw::DrawViewPart* objFeat,
                        double factor, bool endLine)
{
    // create symbolizing lines of a thread from the side seen
    std::string GeoType0 = TechDraw::DrawUtil::getGeomTypeFromName(SubNames[0]);
    std::string GeoType1 = TechDraw::DrawUtil::getGeomTypeFromName(SubNames[1]);
    if ((GeoType0 == "Edge") && (GeoType1 == "Edge")) {
        int GeoId0 = TechDraw::DrawUtil::getIndexFromName(SubNames[0]);
        int GeoId1 = TechDraw::DrawUtil::getIndexFromName(SubNames[1]);
        TechDraw::BaseGeomPtr geom0 = objFeat->getGeomByIndex(GeoId0);
        TechDraw::BaseGeomPtr geom1 = objFeat->getGeomByIndex(GeoId1);
        if (geom0->getGeomType() != GeomType::GENERIC || geom1->getGeomType() != GeomType::GENERIC) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("TechDraw thread hole side"),
                                 QObject::tr("Select 2 straight lines"));
            return;
        }

        TechDraw::GenericPtr line0 = std::static_pointer_cast<TechDraw::Generic>(geom0);
        TechDraw::GenericPtr line1 = std::static_pointer_cast<TechDraw::Generic>(geom1);
        // start and end points are scaled,rotated and inverted (CSRIx).
        // convert start and end to unscaled, unrotated.
        Base::Vector3d start0 = CosmeticVertex::makeCanonicalPointInverted(objFeat, line0->getStartPoint());
        Base::Vector3d start1 = CosmeticVertex::makeCanonicalPointInverted(objFeat, line1->getStartPoint());
        Base::Vector3d end0 = CosmeticVertex::makeCanonicalPointInverted(objFeat, line0->getEndPoint());
        Base::Vector3d end1 = CosmeticVertex::makeCanonicalPointInverted(objFeat, line1->getEndPoint());
        if (DrawUtil::circulation(start0, end0, start1)
            != DrawUtil::circulation(end0, end1, start1)) {
            Base::Vector3d help1 = start1;
            Base::Vector3d help2 = end1;
            start1 = help2;
            end1 = help1;
        }
        float kernelDiam = (start1 - start0).Length();
        float kernelFactor = (kernelDiam * factor - kernelDiam) / 2;
        Base::Vector3d delta = (start1 - start0).Normalize() * kernelFactor;
        std::string line0Tag =
            objFeat->addCosmeticEdge(start0 - delta, end0 - delta);
        std::string line1Tag =
            objFeat->addCosmeticEdge(start1 + delta, end1 + delta);
        TechDraw::CosmeticEdge* cosTag0 = objFeat->getCosmeticEdge(line0Tag);
        TechDraw::CosmeticEdge* cosTag1 = objFeat->getCosmeticEdge(line1Tag);
        int solidStyle = Qt::SolidLine;
        float thinWeight = (float)TechDraw::DrawUtil::getDefaultLineWeight("Thin");
        Base::Color threadColor = _getActiveLineAttributes().getColor();
        _setLineAttributes(cosTag0, solidStyle, thinWeight, threadColor);
        _setLineAttributes(cosTag1, solidStyle, thinWeight, threadColor);
        if (endLine) {
            float graphicWeight = (float)TechDraw::DrawUtil::getDefaultLineWeight("Graphic");
            std::string line3Tag =
                objFeat->addCosmeticEdge(end0 - delta, end1 + delta);
            TechDraw::CosmeticEdge* cosTag3 = objFeat->getCosmeticEdge(line3Tag);
            _setLineAttributes(cosTag3, solidStyle, graphicWeight, threadColor);
        }
    }
}

void _setLineAttributes(TechDraw::CosmeticEdge* cosEdge)
{
    // set line attributes of a cosmetic edge
    cosEdge->m_format.setStyle(_getActiveLineAttributes().getStyle());
    cosEdge->m_format.setWidth(_getActiveLineAttributes().getWidth());
    cosEdge->m_format.setColor(_getActiveLineAttributes().getColor());
    cosEdge->m_format.setVisible(_getActiveLineAttributes().getVisible());
    cosEdge->m_format.setLineNumber(_getActiveLineAttributes().getLineNumber());
}

void _setLineAttributes(TechDraw::CenterLine* cosEdge)
{
    // set line attributes of a cosmetic edge
    cosEdge->m_format.setStyle(_getActiveLineAttributes().getStyle());
    cosEdge->m_format.setWidth(_getActiveLineAttributes().getWidth());
    cosEdge->m_format.setColor(_getActiveLineAttributes().getColor());
    cosEdge->m_format.setVisible(_getActiveLineAttributes().getVisible());
    cosEdge->m_format.setLineNumber(_getActiveLineAttributes().getLineNumber());
}

void _setLineAttributes(TechDraw::CosmeticEdge* cosEdge, int style, float weight, Base::Color color)
{
    // set line attributes of a cosmetic edge
    cosEdge->m_format.setStyle(style);
    cosEdge->m_format.setWidth(weight);
    cosEdge->m_format.setColor(color);
    cosEdge->m_format.setVisible(_getActiveLineAttributes().getVisible());
    cosEdge->m_format.setLineNumber(style);
}

void _setLineAttributes(TechDraw::CenterLine* cosEdge, int style, float weight, Base::Color color)
{
    // set line attributes of a centerline
    cosEdge->m_format.setStyle(style);
    cosEdge->m_format.setWidth(weight);
    cosEdge->m_format.setColor(color);
    cosEdge->m_format.setVisible(_getActiveLineAttributes().getVisible());
    cosEdge->m_format.setLineNumber(style);}
}// namespace TechDrawGui