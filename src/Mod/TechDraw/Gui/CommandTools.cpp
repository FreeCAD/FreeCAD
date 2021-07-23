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

enum EdgeType{
        isInvalid,
        isHorizontal,
        isVertical,
        isDiagonal,
        isCircle,
        isEllipse,
        isBSplineCircle,
        isBSpline,
        isAngle,
        isAngle3Pt
    };

//internal test functions
int _isMyValidSingleEdge(Gui::Command* cmd);
std::vector<std::string> _getSubNames(Gui::Command* cmd);
void _printSelected(Gui::Command* cmd);
void _addCosEdge(Gui::Command* cmd);
void _addCosCircle(Gui::Command* cmd);
void _addCosCircleArc(Gui::Command* cmd);
void _addCosVertex(Gui::Command* cmd);
//internal helper functions
bool _circulation(Base::Vector3d A, Base::Vector3d B, Base::Vector3d C);
void _createThreadCircle(std::string Name, TechDraw::DrawViewPart* objFeat, float factor);
void _createThreadLines(std::vector<std::string> SubNames, TechDraw::DrawViewPart* objFeat, float factor);
void _setStyleAndWeight(TechDraw::CosmeticEdge* cosEdge, int style, float weight);


//===========================================================================
// TechDraw_ToolCircleCenterLines
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawToolCircleCenterLines)

CmdTechDrawToolCircleCenterLines::CmdTechDrawToolCircleCenterLines()
  : Command("TechDraw_ToolCircleCenterLines")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Draw circle center lines");
    sToolTipText    = QT_TR_NOOP("Draw circle center line cross at circles\n\
    - select many circles or arcs\n\
    - click this button");
    sWhatsThis      = "TechDraw_ToolCircleCenterLines";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_ToolCircleCenterLines";
}

void CmdTechDrawToolCircleCenterLines::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Base::Console().Message("ToolCircleCenterLines gestartet\n");
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
                std::string line1tag = objFeat->addCosmeticEdge(right, left);
                std::string line2tag = objFeat->addCosmeticEdge(top, bottom);
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

bool CmdTechDrawToolCircleCenterLines::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ToolThreadHoleSide
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawToolThreadHoleSide)

CmdTechDrawToolThreadHoleSide::CmdTechDrawToolThreadHoleSide()
  : Command("TechDraw_ToolThreadHoleSide")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Cosmetic thread hole side view");
    sToolTipText    = QT_TR_NOOP("Draw cosmetic thread hole side view\n\
    - select two parallel lines\n\
    - click this button");
    sWhatsThis      = "TechDraw_ToolThreadHoleSide";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_ToolThreadHoleSide";
}

void CmdTechDrawToolThreadHoleSide::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Base::Console().Message("ToolThreadHoleSide gestartet\n");
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

bool CmdTechDrawToolThreadHoleSide::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ToolThreadBoltSide
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawToolThreadBoltSide)

CmdTechDrawToolThreadBoltSide::CmdTechDrawToolThreadBoltSide()
  : Command("TechDraw_ToolThreadBoltSide")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Cosmetic thread bolt side view");
    sToolTipText    = QT_TR_NOOP("Draw cosmetic crew thread side view\n\
    - select two parallel lines\n\
    - click this button");
    sWhatsThis      = "TechDraw_ToolThreadBoltSide";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_ToolThreadBoltSide";
}

void CmdTechDrawToolThreadBoltSide::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Base::Console().Message("ToolThreadBoltSide gestartet\n");
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

bool CmdTechDrawToolThreadBoltSide::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ToolThreadHoleBottom
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawToolThreadHoleBottom)

CmdTechDrawToolThreadHoleBottom::CmdTechDrawToolThreadHoleBottom()
  : Command("TechDraw_ToolThreadHoleBottom")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Cosmetic thread hole bottom view");
    sToolTipText    = QT_TR_NOOP("Draw cosmetic hole thread ground view\n\
    - select many circles\n\
    - click this button");
    sWhatsThis      = "TechDraw_ToolThreadHoleBottom";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_ToolThreadHoleBottom";
}

void CmdTechDrawToolThreadHoleBottom::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Base::Console().Message("ToolThreadHoleBottom gestartet\n");
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

bool CmdTechDrawToolThreadHoleBottom::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ToolThreadBoltBottom
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawToolThreadBoltBottom)

CmdTechDrawToolThreadBoltBottom::CmdTechDrawToolThreadBoltBottom()
  : Command("TechDraw_ToolThreadBoltBottom")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Cosmetic thread bolt bottom view");
    sToolTipText    = QT_TR_NOOP("Draw cosmetic screw thread ground view\n\
    - select many circles\n\
    - click this button");
    sWhatsThis      = "TechDraw_ToolThreadBoltBottom";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_ToolThreadBoltBottom";
}

void CmdTechDrawToolThreadBoltBottom::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Base::Console().Message("ToolThreadBoltBottom gestartet\n");
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

bool CmdTechDrawToolThreadBoltBottom::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_MyCommand
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawMyCommand)

CmdTechDrawMyCommand::CmdTechDrawMyCommand()
  : Command("TechDraw_MyCommand")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert My Command");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_MyCommand";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_MyCommand";
}

void CmdTechDrawMyCommand::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Base::Console().Message("My Command gestartet\n");
    std::vector<std::string> SubNames;
    //SubNames = _getSubNames(this);
    _addCosEdge(this);
    _addCosCircle(this);
    _addCosVertex(this);
    _addCosCircleArc(this);
    /*
    for (std::string Name : SubNames){
        Base::Console().Message("%s\n",Name.c_str());
    }
    */
    //int edgeType = _isMyValidSingleEdge(this);
    //Base::Console().Message("Type: %d\n",edgeType);
    
    /*
    std::string Viewname; // name der View
    //-------------------------------------------
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx(); //Python: gui.Selection.getCompleteSelection()
    TechDraw::DrawViewPart * objFeat = 0;
    std::vector<std::string> SubNames;

    std::vector<Gui::SelectionObject>::iterator itSel = selection.begin();
    for (; itSel != selection.end(); itSel++)  {
        if ((*itSel).getObject()->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
            objFeat = static_cast<TechDraw::DrawViewPart*> ((*itSel).getObject()); //Python: gui.Selection.getCompleteSelection()[0]
            SubNames = (*itSel).getSubNames(); //Python: gui.Selection.getSelectionEx()[0].SubElementNames
        }
    }
    // gewählte Objekte als string: "Vertex5","Edge3",...
    for (std::string Name : SubNames){
        Base::Console().Message("%s\n",Name.c_str());
    }
    // Name der gewählten View
    Viewname = objFeat->getNameInDocument(); //Py: gui.Selection.getCompleteSelection()[0].Name
    Base::Console().Message("Name der View: %s\n",Viewname.c_str());
    // Liste der Koordinaten aller Vertexes der View
    std::vector<TechDraw::Vertex*> gVerts; // Vektor aus Pointern zu den vertexes
    gVerts = objFeat->getVertexGeometry(); // definiert in DrawViewPart.cpp
    for (auto& gv: gVerts) {
        Base::Console().Message("x-Pos: %f\n",gv->point().x);
        Base::Console().Message("Tag: %s\n",gv->getTagAsString().c_str()); // definiert in cosmetic.h
        gv->dump();
        int ii;
        ii = objFeat->getCVIndex(gv->getTagAsString());
        Base::Console().Message("Index: %d\n",ii);
        //Base::Console().Message("Vertex: %s\n",gv->toString().c_str()); //... gibt es nicht
        //Base::Console().Message("Style: %d\n",gv->style); //... gibt es nicht, nur bei ::CosmeticVertex ?
        }
    // ------------- ab hier Versuch -----------------
    //DrawViewPart* dvp = getDrawViewPartPtr();
    //TechDraw::CosmeticVertex* cov = dvp->getCosmeticVertexBySelection("Vertex5");
    */
}

bool CmdTechDrawMyCommand::isActive(void)
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
    int GeoId = TechDraw::DrawUtil::getIndexFromName(Name);
    TechDraw::BaseGeom* geom = objFeat->getGeomByIndex(GeoId);
    std::string GeoType = TechDraw::DrawUtil::getGeomTypeFromName(Name);
    if (GeoType == "Edge"){
        if (geom->geomType == TechDraw::CIRCLE){
            TechDraw::Circle* cgen = static_cast<TechDraw::Circle *>(geom);
            Base::Vector3d center = cgen->center;
            float radius = cgen->radius;
            TechDraw::BaseGeom* threadArc = new TechDraw::AOC(center, radius*factor, 255.0, 165.0);
            std::string arcTag = objFeat->addCosmeticEdge(threadArc);
            TechDraw::CosmeticEdge* arc = objFeat->getCosmeticEdge(arcTag);
            _setStyleAndWeight(arc,1,0.35);
        }
    }
}

void _createThreadLines(std::vector<std::string> SubNames, TechDraw::DrawViewPart* objFeat, float factor){
    // create symbolizing lines of a thread from the side seen
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
            std::string line0Tag = objFeat->addCosmeticEdge(start0-delta, end0-delta);
            std::string line1Tag = objFeat->addCosmeticEdge(start1+delta, end1+delta);
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

//===========================================================================
// internal test routines
//===========================================================================

std::vector<std::string> _getSubNames(Gui::Command* cmd) {
    // get the Subnames of all selected elements
    auto selection = cmd->getSelection().getSelectionEx();
    const std::vector<std::string> SubNames = selection[0].getSubNames();
    return SubNames;
}

void _addCosVertex(Gui::Command* cmd) {
    // create a cosmetic vertex at point pnt
    auto selection = cmd->getSelection().getSelectionEx();
    auto objFeat = dynamic_cast<TechDraw::DrawViewPart *>(selection[0].getObject());
    Base::Vector3d pnt(-10.0, -10.0, 0.0);
    pnt = DrawUtil::invertY(objFeat->projectPoint(pnt));
    std::string id = objFeat->addCosmeticVertex(pnt);
    objFeat->add1CVToGV(id); // needed ?
    objFeat->refreshCVGeoms();

    //objFeat->refreshCEGeoms();
    objFeat->requestPaint();
}

void _addCosCircleArc(Gui::Command* cmd) {
    // create a cosmetic arc of circle with center pc, radius and angle1, angle2
    auto selection = cmd->getSelection().getSelectionEx();
    auto objFeat = dynamic_cast<TechDraw::DrawViewPart *>(selection[0].getObject());
    Base::Vector3d pc(10.0, -10.0, 0.0);
    float radius = 10.0;
    float angle1 = 0.0;
    float angle2 = 90.0;
    pc = DrawUtil::invertY(objFeat->projectPoint(pc));
    TechDraw::BaseGeom* baseGeo = new TechDraw::AOC(pc, radius, angle1, angle2);
    std::string cTag = objFeat->addCosmeticEdge(baseGeo);
    objFeat->refreshCEGeoms();
    objFeat->requestPaint();
}

void _addCosCircle(Gui::Command* cmd) {
    // create a cosmetic circle with center pc and radius
    auto selection = cmd->getSelection().getSelectionEx();
    auto objFeat = dynamic_cast<TechDraw::DrawViewPart *>(selection[0].getObject());
    Base::Vector3d pc(-20.0, -20.0, 0.0);
    float radius = 10.0;
    pc = DrawUtil::invertY(objFeat->projectPoint(pc));
    TechDraw::BaseGeom* baseGeo = new TechDraw::Circle(pc, radius);
    std::string cTag = objFeat->addCosmeticEdge(baseGeo);
    objFeat->refreshCEGeoms();
    objFeat->requestPaint();
}

void _addCosEdge(Gui::Command* cmd) {
    // create a cosmetic edge from p0 to p1
    auto selection = cmd->getSelection().getSelectionEx();
    auto objFeat = dynamic_cast<TechDraw::DrawViewPart *>(selection[0].getObject());
    Base::Vector3d p0(-20.0, -20.0, 0.0);
    Base::Vector3d p1(20.0, 20.0, 0.0);
    p0 = DrawUtil::invertY(objFeat->projectPoint(p0));
    p1 = DrawUtil::invertY(objFeat->projectPoint(p1));
    std::string etag = objFeat->addCosmeticEdge(p0, p1); // ..für Gerade
    auto ce = objFeat->getCosmeticEdge(etag);  
    ce->m_format.m_style = 1;
    ce->m_format.m_weight = 0.5;
    ce->m_format.m_color = App::Color(1.0f,0.0f,0.0f);
    objFeat->refreshCEGeoms();
    objFeat->requestPaint();
}

void _printSelected(Gui::Command* cmd) {
    // print info of selected Line, Circle, Arc and Vertex to the console
    auto selection = cmd->getSelection().getSelectionEx();
    auto objFeat = dynamic_cast<TechDraw::DrawViewPart *>(selection[0].getObject());
    const std::vector<std::string> SubNames = selection[0].getSubNames();
    for (std::string Name : SubNames)   {
        int GeoId = TechDraw::DrawUtil::getIndexFromName(Name);
        TechDraw::BaseGeom* geom = objFeat->getGeomByIndex(GeoId);
        std::string GeoType = TechDraw::DrawUtil::getGeomTypeFromName(Name);
        if (GeoType == "Edge"){
            if (geom->geomType == TechDraw::GENERIC){
                Base::Console().Message("%s ist eine Gerade\n",Name.c_str());
                TechDraw::Generic* gen = static_cast<TechDraw::Generic *>(geom);
                Base::Console().Message("und hat %d Punkte ",gen->points.size());
                Base::Vector3d P0 = gen->points.at(0);
                Base::Vector3d P1 = gen->points.at(1);
                Base::Console().Message("bei %f %f und %f %f\n",P0.x,P0.y,P1.x,P1.y);
            } else if (geom->geomType == TechDraw::CIRCLE) {
                Base::Console().Message("%s ist ein Kreis\n",Name.c_str());
                TechDraw::Circle* cgen = static_cast<TechDraw::Circle *>(geom);
                Base::Vector3d Mitte = cgen->center;
                float Radius = cgen->radius;
                Base::Console().Message("Mitte bei %f %f mit dem Radius: %f\n",Mitte.x,Mitte.y,Radius);
            } else if (geom->geomType == TechDraw::ARCOFCIRCLE) {
                Base::Console().Message("%s ist ein Bogen\n",Name.c_str());
                TechDraw::Circle* agen = static_cast<TechDraw::Circle *>(geom);
                Base::Vector3d Mitte = agen->center;
                float Radius = agen->radius;
                Base::Console().Message("Mitte bei %f %f mit dem Radius: %f\n",Mitte.x,Mitte.y,Radius);
            }      
        } else if (GeoType == "Vertex") {
            Base::Console().Message("%s ist ein Punkt",Name.c_str());
            TechDraw::Vertex* vert = objFeat->getProjVertexByIndex(GeoId);
            Base::Console().Message("bei: %f %f\n",vert->point().x,vert->point().y);
        }
    }
}

//! verify that Selection contains a valid Geometry for a single Edge Dimension
// kopiert von CommandCreateDims.cpp

int _isMyValidSingleEdge(Gui::Command* cmd) {
    auto edgeType( isInvalid );
    auto selection = cmd->getSelection().getSelectionEx();

    auto objFeat = dynamic_cast<TechDraw::DrawViewPart *>(selection[0].getObject());
    if( objFeat == nullptr ) {
        return isInvalid;
    }

    const std::vector<std::string> SubNames = selection[0].getSubNames();
    if (SubNames.size() == 1) {                                                 //only 1 subshape selected
        if (TechDraw::DrawUtil::getGeomTypeFromName(SubNames[0]) == "Edge") {   //the Name starts with "Edge"
            int GeoId( TechDraw::DrawUtil::getIndexFromName(SubNames[0]) );
            TechDraw::BaseGeom* geom = objFeat->getGeomByIndex(GeoId);
            if (!geom) {
                Base::Console().Error("Logic Error: no geometry for GeoId: %d\n",GeoId);
                return isInvalid;
            }

            if(geom->geomType == TechDraw::GENERIC) {
                TechDraw::Generic* gen1 = static_cast<TechDraw::Generic *>(geom);
                if(gen1->points.size() > 2) {                                   //the edge is a polyline
                    return isInvalid;
                }
                Base::Vector3d line = gen1->points.at(1) - gen1->points.at(0);
                if(fabs(line.y) < FLT_EPSILON ) {
                    edgeType = isHorizontal;
                } else if(fabs(line.x) < FLT_EPSILON) {
                    edgeType = isVertical;
                } else {
                    edgeType = isDiagonal;
                }
            } else if (geom->geomType == TechDraw::CIRCLE ||
                       geom->geomType == TechDraw::ARCOFCIRCLE ) {
                edgeType = isCircle;
            } else if (geom->geomType == TechDraw::ELLIPSE ||
                       geom->geomType == TechDraw::ARCOFELLIPSE) {
                edgeType = isEllipse;
            } else if (geom->geomType == TechDraw::BSPLINE) {
                TechDraw::BSpline* spline = static_cast<TechDraw::BSpline*>(geom);
                if (spline->isCircle()) {
                    edgeType = isBSplineCircle;
                } else {
                    edgeType = isBSpline;
                }
            } else {
                edgeType = isInvalid;
            }
        }
    }
    return edgeType;
}

//------------------------------------------------------------------------------
void CreateTechDrawCommandsTools(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdTechDrawMyCommand());
    rcCmdMgr.addCommand(new CmdTechDrawToolCircleCenterLines());
    rcCmdMgr.addCommand(new CmdTechDrawToolThreadHoleSide());
    rcCmdMgr.addCommand(new CmdTechDrawToolThreadBoltSide());
    rcCmdMgr.addCommand(new CmdTechDrawToolThreadHoleBottom());
    rcCmdMgr.addCommand(new CmdTechDrawToolThreadBoltBottom());
}
