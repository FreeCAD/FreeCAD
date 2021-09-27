/***************************************************************************
 *   Copyright (c) 2021 edi                                                *
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
#ifndef _PreComp_
# include <QMessageBox>
# include <iostream>
# include <string>
# include <sstream>
# include <cstdlib>
# include <exception>
#endif  //#ifndef _PreComp_

#include <QGraphicsView>

# include <App/DocumentObject.h>
# include <Base/Exception.h>
#include <Base/Console.h>
#include <Base/Type.h>
# include <Gui/Action.h>
# include <Gui/Application.h>
# include <Gui/BitmapFactory.h>
# include <Gui/Command.h>
# include <Gui/Control.h>
# include <Gui/Document.h>
# include <Gui/Selection.h>
# include <Gui/MainWindow.h>
# include <Gui/FileDialog.h>
# include <Gui/ViewProvider.h>

# include <Mod/Part/App/PartFeature.h>

# include <Mod/TechDraw/App/DrawViewPart.h>
# include <Mod/TechDraw/App/DrawProjGroupItem.h>
# include <Mod/TechDraw/App/DrawProjGroup.h>
# include <Mod/TechDraw/App/DrawViewDimension.h>
# include <Mod/TechDraw/App/DrawDimHelper.h>
# include <Mod/TechDraw/App/LandmarkDimension.h>
# include <Mod/TechDraw/App/DrawPage.h>
# include <Mod/TechDraw/App/DrawUtil.h>
# include <Mod/TechDraw/App/Geometry.h>

#include <Mod/TechDraw/Gui/QGVPage.h>


#include "DrawGuiUtil.h"
#include "MDIViewPage.h"
#include "ViewProviderPage.h"
#include "TaskLinkDim.h"

using namespace TechDrawGui;
using namespace TechDraw;
using namespace std;

//internal helper functions
bool _circulation(Base::Vector3d A, Base::Vector3d B, Base::Vector3d C);
void _createThreadCircle(std::string Name, TechDraw::DrawViewPart* objFeat, float factor);
void _createThreadLines(std::vector<std::string> SubNames, TechDraw::DrawViewPart* objFeat, float factor);
void _setStyleAndWeight(TechDraw::CosmeticEdge* cosEdge, int style, float weight);

//===========================================================================
// TechDraw_ExtensionCircleCenterLines
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawExtensionCircleCenterLines)

CmdTechDrawExtensionCircleCenterLines::CmdTechDrawExtensionCircleCenterLines()
  : Command("TechDraw_ExtensionCircleCenterLines")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Draw circle centerlines");
    sToolTipText    = QT_TR_NOOP("Draw circle centerline cross at circles\n\
    - select many circles or arcs\n\
    - click this button");
    sWhatsThis      = "TechDraw_ExtensionCircleCenterLines";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_ExtensionCircleCenterLines";
}

void CmdTechDrawExtensionCircleCenterLines::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    auto selection = getSelection().getSelectionEx();
    if( selection.empty() ) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("TechDraw Circle Centerlines"),
                             QObject::tr("Selection is empty"));
        return;
    }
    auto objFeat = dynamic_cast<TechDraw::DrawViewPart *>(selection[0].getObject());
    if( objFeat == nullptr ) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("TechDraw Circle Centerlines"),
                             QObject::tr("No object selected"));
        return;
    }
    double scale = objFeat->getScale();
    const std::vector<std::string> SubNames = selection[0].getSubNames();
    for (std::string Name : SubNames) {
        int GeoId = TechDraw::DrawUtil::getIndexFromName(Name);
        TechDraw::BaseGeom* geom = objFeat->getGeomByIndex(GeoId);
        std::string GeoType = TechDraw::DrawUtil::getGeomTypeFromName(Name);
        if (GeoType == "Edge"){
            if (geom->geomType == TechDraw::CIRCLE ||
                geom->geomType == TechDraw::ARCOFCIRCLE){
                TechDraw::Circle* cgen = static_cast<TechDraw::Circle *>(geom);
                Base::Vector3d center = cgen->center;
                center.y = -center.y;
                float radius = cgen->radius;
                Base::Vector3d right(center.x+radius+2.0,center.y,0.0);
                Base::Vector3d top(center.x,center.y+radius+2.0,0.0);
                Base::Vector3d left(center.x-radius-2.0,center.y,0.0);
                Base::Vector3d bottom(center.x,center.y-radius-2.0,0.0);
                std::string line1tag = objFeat->addCosmeticEdge(right/scale, left/scale);
                std::string line2tag = objFeat->addCosmeticEdge(top/scale, bottom/scale);
                TechDraw::CosmeticEdge* horiz = objFeat->getCosmeticEdge(line1tag);
                _setStyleAndWeight(horiz,4,0.35);
                TechDraw::CosmeticEdge* vert = objFeat->getCosmeticEdge(line2tag);
                _setStyleAndWeight(vert,4,0.35);
            }
        }
    }
    getSelection().clearSelection();
    objFeat->refreshCEGeoms();
    objFeat->requestPaint();
}

bool CmdTechDrawExtensionCircleCenterLines::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionThreadHoleSide
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawExtensionThreadHoleSide)

CmdTechDrawExtensionThreadHoleSide::CmdTechDrawExtensionThreadHoleSide()
  : Command("TechDraw_ExtensionThreadHoleSide")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Cosmetic thread hole side view");
    sToolTipText    = QT_TR_NOOP("Draw cosmetic thread hole side view\n\
    - select two parallel lines\n\
    - click this button");
    sWhatsThis      = "TechDraw_ExtensionThreadHoleSide";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_ExtensionThreadHoleSide";
}

void CmdTechDrawExtensionThreadHoleSide::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    auto selection = getSelection().getSelectionEx();
    if( selection.empty() ) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("TechDraw Thread Hole Side"),
                             QObject::tr("Selection is empty"));
        return;
    }
    TechDraw::DrawViewPart* objFeat = dynamic_cast<TechDraw::DrawViewPart *>(selection[0].getObject());
    if( objFeat == nullptr ) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("TechDraw Thread Hole Side"),
                             QObject::tr("No object selected"));
        return;
    }
    const std::vector<std::string> SubNames = selection[0].getSubNames();
    if (SubNames.size() >= 2) {
        _createThreadLines(SubNames, objFeat, 1.176);
    }
    getSelection().clearSelection();
    objFeat->refreshCEGeoms();
    objFeat->requestPaint();
}

bool CmdTechDrawExtensionThreadHoleSide::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionThreadBoltSide
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawExtensionThreadBoltSide)

CmdTechDrawExtensionThreadBoltSide::CmdTechDrawExtensionThreadBoltSide()
  : Command("TechDraw_ExtensionThreadBoltSide")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Cosmetic thread bolt side view");
    sToolTipText    = QT_TR_NOOP("Draw cosmetic screw thread side view\n\
    - select two parallel lines\n\
    - click this button");
    sWhatsThis      = "TechDraw_ExtensionThreadBoltSide";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_ExtensionThreadBoltSide";
}

void CmdTechDrawExtensionThreadBoltSide::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    auto selection = getSelection().getSelectionEx();
    if( selection.empty() ) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("TechDraw Thread Bolt Side"),
                             QObject::tr("Selection is empty"));
        return;
    }
    TechDraw::DrawViewPart* objFeat = dynamic_cast<TechDraw::DrawViewPart *>(selection[0].getObject());
    if( objFeat == nullptr ) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("TechDraw Thread Bolt Side"),
                             QObject::tr("No object selected"));
        return;
    }
    const std::vector<std::string> SubNames = selection[0].getSubNames();
    if (SubNames.size() >= 2) {
        _createThreadLines(SubNames, objFeat, 0.85);
    }
    getSelection().clearSelection();
    objFeat->refreshCEGeoms();
    objFeat->requestPaint();
}

bool CmdTechDrawExtensionThreadBoltSide::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionThreadHoleBottom
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawExtensionThreadHoleBottom)

CmdTechDrawExtensionThreadHoleBottom::CmdTechDrawExtensionThreadHoleBottom()
  : Command("TechDraw_ExtensionThreadHoleBottom")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Cosmetic thread hole bottom view");
    sToolTipText    = QT_TR_NOOP("Draw cosmetic hole thread ground view\n\
    - select many circles\n\
    - click this button");
    sWhatsThis      = "TechDraw_ExtensionThreadHoleBottom";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_ExtensionThreadHoleBottom";
}

void CmdTechDrawExtensionThreadHoleBottom::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    auto selection = getSelection().getSelectionEx();
    if( selection.empty() ) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("TechDraw Thread Hole Bottom"),
                             QObject::tr("Selection is empty"));
        return;
    }
    auto objFeat = dynamic_cast<TechDraw::DrawViewPart *>(selection[0].getObject());
    if( objFeat == nullptr ) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("TechDraw Thread Hole Bottom"),
                             QObject::tr("No object selected"));
        return;
    }
    const std::vector<std::string> SubNames = selection[0].getSubNames();
    for (std::string Name : SubNames) {
        _createThreadCircle(Name, objFeat, 1.177);
    }
    getSelection().clearSelection();
    objFeat->refreshCEGeoms();
    objFeat->requestPaint();
}

bool CmdTechDrawExtensionThreadHoleBottom::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionThreadBoltBottom
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawExtensionThreadBoltBottom)

CmdTechDrawExtensionThreadBoltBottom::CmdTechDrawExtensionThreadBoltBottom()
  : Command("TechDraw_ExtensionThreadBoltBottom")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Cosmetic thread bolt bottom view");
    sToolTipText    = QT_TR_NOOP("Draw cosmetic screw thread ground view\n\
    - select many circles\n\
    - click this button");
    sWhatsThis      = "TechDraw_ExtensionThreadBoltBottom";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_ExtensionThreadBoltBottom";
}

void CmdTechDrawExtensionThreadBoltBottom::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    auto selection = getSelection().getSelectionEx();
    if( selection.empty() ) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("TechDraw Tread Bolt Bottom"),
                             QObject::tr("Selection is empty"));
        return;
    }
    auto objFeat = dynamic_cast<TechDraw::DrawViewPart *>(selection[0].getObject());
    if( objFeat == nullptr ) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("TechDraw Tread Bolt Bottom"),
                             QObject::tr("No object selected"));
        return;
    }
    const std::vector<std::string> SubNames = selection[0].getSubNames();
    for (std::string Name : SubNames) {
        _createThreadCircle(Name, objFeat, 0.85);
    }
    getSelection().clearSelection();
    objFeat->refreshCEGeoms();
    objFeat->requestPaint();
}

bool CmdTechDrawExtensionThreadBoltBottom::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// internal helper routines
//===========================================================================

bool _circulation(Base::Vector3d A, Base::Vector3d B, Base::Vector3d C){
    // test the circulation of the triangle A-B-C
    if (A.x*B.y+A.y*C.x+B.x*C.y-C.x*B.y-C.y*A.x-B.x*A.y > 0.0)
        return true;
    else
        return false;
}

void _createThreadCircle(std::string Name, TechDraw::DrawViewPart* objFeat, float factor){
    // create the 3/4 arc symbolizing a thread from top seen
    double scale = objFeat->getScale();
    int GeoId = TechDraw::DrawUtil::getIndexFromName(Name);
    TechDraw::BaseGeom* geom = objFeat->getGeomByIndex(GeoId);
    std::string GeoType = TechDraw::DrawUtil::getGeomTypeFromName(Name);
    if (GeoType == "Edge"){
        if (geom->geomType == TechDraw::CIRCLE){
            TechDraw::Circle* cgen = static_cast<TechDraw::Circle *>(geom);
            Base::Vector3d center = cgen->center;
            float radius = cgen->radius;
            TechDraw::BaseGeom* threadArc = new TechDraw::AOC(center/scale, radius*factor/scale, 255.0, 165.0);
            std::string arcTag = objFeat->addCosmeticEdge(threadArc);
            TechDraw::CosmeticEdge* arc = objFeat->getCosmeticEdge(arcTag);
            _setStyleAndWeight(arc,1,0.35);
        }
    }
}

void _createThreadLines(std::vector<std::string> SubNames, TechDraw::DrawViewPart* objFeat, float factor){
    // create symbolizing lines of a thread from the side seen
    double scale = objFeat->getScale();
    std::string GeoType0 = TechDraw::DrawUtil::getGeomTypeFromName(SubNames[0]);
    std::string GeoType1 = TechDraw::DrawUtil::getGeomTypeFromName(SubNames[1]);
    if ((GeoType0 == "Edge") && (GeoType1 == "Edge")) {
        int GeoId0 = TechDraw::DrawUtil::getIndexFromName(SubNames[0]);
        int GeoId1 = TechDraw::DrawUtil::getIndexFromName(SubNames[1]);
        TechDraw::BaseGeom* geom0 = objFeat->getGeomByIndex(GeoId0);
        TechDraw::BaseGeom* geom1 = objFeat->getGeomByIndex(GeoId1);
        if ((geom0->geomType == TechDraw::GENERIC) && (geom1->geomType == TechDraw::GENERIC)) {
            TechDraw::Generic* line0 = static_cast<TechDraw::Generic *>(geom0);
            TechDraw::Generic* line1 = static_cast<TechDraw::Generic *>(geom1);
            Base::Vector3d start0 = line0->points.at(0);
            Base::Vector3d end0 = line0->points.at(1);
            Base::Vector3d start1 = line1->points.at(0);
            Base::Vector3d end1 = line1->points.at(1);
            if (_circulation(start0,end0,start1) != _circulation(end0,end1,start1)) {
                Base::Vector3d help1 = start1;
                Base::Vector3d help2 = end1;
                start1 = help2;
                end1 = help1;
            }
            start0.y = -start0.y;
            end0.y = -end0.y;
            start1.y = -start1.y;
            end1.y = -end1.y;
            float kernelDiam = (start1-start0).Length();
            float kernelFactor = (kernelDiam*factor-kernelDiam)/2;
            Base::Vector3d delta = (start1-start0).Normalize()*kernelFactor;
            std::string line0Tag = objFeat->addCosmeticEdge((start0-delta)/scale, (end0-delta)/scale);
            std::string line1Tag = objFeat->addCosmeticEdge((start1+delta)/scale, (end1+delta)/scale);
            TechDraw::CosmeticEdge* cosTag0 = objFeat->getCosmeticEdge(line0Tag);
            TechDraw::CosmeticEdge* cosTag1 = objFeat->getCosmeticEdge(line1Tag);
            _setStyleAndWeight(cosTag0,1,0.35);
            _setStyleAndWeight(cosTag1,1,0.35);
        } else {
            QMessageBox::warning(Gui::getMainWindow(),
                                 QObject::tr("TechDraw Thread Hole Side"),
                                 QObject::tr("Please select two straight lines"));
            return;
        }
    }
}

void _setStyleAndWeight(TechDraw::CosmeticEdge* cosEdge, int style, float weight) {
    // set style and weight of a cosmetic edge
    cosEdge->m_format.m_style = style;
    cosEdge->m_format.m_weight = weight;
}

//------------------------------------------------------------------------------
void CreateTechDrawCommandsExtensions(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdTechDrawExtensionCircleCenterLines());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionThreadHoleSide());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionThreadBoltSide());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionThreadHoleBottom());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionThreadBoltBottom());
}
