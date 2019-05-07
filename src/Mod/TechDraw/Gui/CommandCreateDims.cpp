/***************************************************************************
 *   Copyright (c) 2014 Luke Parry <l.parry@warwick.ac.uk>                 *
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
# include <Mod/TechDraw/App/DrawPage.h>
# include <Mod/TechDraw/App/DrawUtil.h>
# include <Mod/TechDraw/App/Geometry.h>

#include <Mod/TechDraw/Gui/QGVPage.h>


#include "DrawGuiUtil.h"
#include "MDIViewPage.h"
#include "ViewProviderPage.h"
#include "TaskLinkDim.h"

using namespace TechDrawGui;
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


//===========================================================================
// utility routines
//===========================================================================


//internal functions
bool _checkSelection(Gui::Command* cmd, unsigned maxObjs = 2);
bool _checkDrawViewPart(Gui::Command* cmd);
bool _checkPartFeature(Gui::Command* cmd);
int _isValidSingleEdge(Gui::Command* cmd);
bool _isValidVertexes(Gui::Command* cmd, int count = 2);
int _isValidEdgeToEdge(Gui::Command* cmd);
bool _isValidVertexToEdge(Gui::Command* cmd);
char* _edgeTypeToText(int e);
//bool _checkActive(Gui::Command* cmd, Base::Type classType, bool needSubs);


//NOTE: this is not shown in toolbar and doesn't always work right in the menu.
//      should be removed.
//===========================================================================
// TechDraw_NewDimension
//===========================================================================

// this is deprecated. use individual add dimension commands.

DEF_STD_CMD_A(CmdTechDrawNewDimension);

CmdTechDrawNewDimension::CmdTechDrawNewDimension()
  : Command("TechDraw_NewDimension")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert a dimension into a drawing");
    sToolTipText    = QT_TR_NOOP("Insert a new dimension");
    sWhatsThis      = "TechDraw_NewDimension";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_Dimension";
}

void CmdTechDrawNewDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    bool result = _checkSelection(this,2);
    if (!result)
        return;
    result = _checkDrawViewPart(this);
    if (!result)
        return;

//do we still need to pick DVPs out of selection? or should we complain about junk?
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();
    TechDraw::DrawViewPart* objFeat = 0;
    std::vector<std::string> SubNames;
    std::vector<Gui::SelectionObject>::iterator itSel = selection.begin();
    for (; itSel != selection.end(); itSel++)  {
        if ((*itSel).getObject()->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
            objFeat = static_cast<TechDraw::DrawViewPart*> ((*itSel).getObject());
            SubNames = (*itSel).getSubNames();
        }
    }
    TechDraw::DrawPage* page = objFeat->findParentPage();
    std::string PageName = page->getNameInDocument();

    TechDraw::DrawViewDimension *dim = 0;
    std::string FeatName = getUniqueObjectName("Dimension");
    std::string dimType;

    std::vector<App::DocumentObject *> objs;
    std::vector<std::string> subs;

    int edgeType = _isValidSingleEdge(this);

    if (edgeType) {
        if (edgeType < isCircle) {
            dimType = "Distance";
            objs.push_back(objFeat);
            subs.push_back(SubNames[0]);
        } else if (edgeType == isCircle) {
            dimType = "Radius";
        } else {
            dimType = "Radius";
        }
    } else if (_isValidVertexes(this)) {
        dimType = "Distance";
        objs.push_back(objFeat);
        objs.push_back(objFeat);
        subs.push_back(SubNames[0]);
        subs.push_back(SubNames[1]);
    } else if (_isValidEdgeToEdge(this)) {
        int edgeCase = _isValidEdgeToEdge(this);
        objs.push_back(objFeat);
        objs.push_back(objFeat);
        subs.push_back(SubNames[0]);
        subs.push_back(SubNames[1]);
        switch (edgeCase) {
             case isHorizontal:
                dimType = "DistanceX";
                break;
             case isVertical:
                dimType = "DistanceY";
                break;
            case isDiagonal:
                dimType = "Distance";
                break;
            case isAngle:
                dimType = "Angle";
            default:
                break;
        }
    } else if (_isValidVertexToEdge(this)) {
        dimType = "Distance";
        objs.push_back(objFeat);
        objs.push_back(objFeat);
        subs.push_back(SubNames[0]);
        subs.push_back(SubNames[1]);
    } else {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Incorrect Selection"),
                                                   QObject::tr("Can not make a Dimension from this selection"));
        return;
    }

    openCommand("Create Dimension");
    doCommand(Doc,"App.activeDocument().addObject('TechDraw::DrawViewDimension','%s')",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Type = '%s'",FeatName.c_str()
                                                       ,dimType.c_str());

    dim = dynamic_cast<TechDraw::DrawViewDimension *>(getDocument()->getObject(FeatName.c_str()));
    if (!dim) {
        throw Base::TypeError("CmdTechDrawNewDimension - dim not found\n");
    }
    dim->References2D.setValues(objs, subs);

    doCommand(Doc,"App.activeDocument().%s.addView(App.activeDocument().%s)",PageName.c_str(),FeatName.c_str());
    commitCommand();
    dim->recomputeFeature();

    //Horrible hack to force Tree update
    double x = objFeat->X.getValue();
    objFeat->X.setValue(x);
}

bool CmdTechDrawNewDimension::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_NewRadiusDimension
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawNewRadiusDimension);

CmdTechDrawNewRadiusDimension::CmdTechDrawNewRadiusDimension()
  : Command("TechDraw_NewRadiusDimension")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert a new radius dimension");
    sToolTipText    = QT_TR_NOOP("Insert a new radius dimension");
    sWhatsThis      = "TechDraw_Dimension_Radius";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_Dimension_Radius";
}

void CmdTechDrawNewRadiusDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    bool result = _checkSelection(this,1);
    if (!result)
        return;
    result = _checkDrawViewPart(this);
    if (!result)
        return;

    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();
    TechDraw::DrawViewPart * objFeat = 0;
    std::vector<std::string> SubNames;

    std::vector<Gui::SelectionObject>::iterator itSel = selection.begin();
    for (; itSel != selection.end(); itSel++)  {
        if ((*itSel).getObject()->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
            objFeat = static_cast<TechDraw::DrawViewPart*> ((*itSel).getObject());
            SubNames = (*itSel).getSubNames();
        }
    }
    TechDraw::DrawPage* page = objFeat->findParentPage();
    std::string PageName = page->getNameInDocument();

    TechDraw::DrawViewDimension *dim = 0;
    std::string FeatName = getUniqueObjectName("Dimension");

    std::vector<App::DocumentObject *> objs;
    std::vector<std::string> subs;

    int edgeType = _isValidSingleEdge(this);
    if (edgeType == isCircle) {
        objs.push_back(objFeat);
        subs.push_back(SubNames[0]);
    } else if (edgeType == isEllipse) {
        QMessageBox::StandardButton result =
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Ellipse Curve Warning"),
                             QObject::tr("Selected edge is an Ellipse.  Radius will be approximate. Continue?"),
                             QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel);
        if (result == QMessageBox::Ok) {
            objs.push_back(objFeat);
            subs.push_back(SubNames[0]);
        } else {
            return;
        }
    } else if (edgeType == isBSplineCircle) {
        QMessageBox::StandardButton result =
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("BSpline Curve Warning"),
                             QObject::tr("Selected edge is a BSpline.  Radius will be approximate. Continue?"),
                             QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel);
        if (result == QMessageBox::Ok) {
            objs.push_back(objFeat);
            subs.push_back(SubNames[0]);
        } else {
            return;
        }
    } else if (edgeType == isBSpline) {
        QMessageBox::StandardButton result =
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("BSpline Curve Warning"),
                             QObject::tr("Selected edge is a BSpline.  Radius will be approximate. Continue?"),
                             QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel);
        if (result == QMessageBox::Ok) {
            objs.push_back(objFeat);
            subs.push_back(SubNames[0]);
        } else {
            return;
        }
    } else {
        std::stringstream edgeMsg;
        edgeMsg << "Selection for Radius does not contain a circular edge (edge type: " << _edgeTypeToText(edgeType) << ")";
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Incorrect Selection"),
                                                   QObject::tr(edgeMsg.str().c_str()));
        return;
    }

    openCommand("Create Dimension");
    doCommand(Doc,"App.activeDocument().addObject('TechDraw::DrawViewDimension','%s')",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Type = '%s'",FeatName.c_str()
                                                       ,"Radius");

    dim = dynamic_cast<TechDraw::DrawViewDimension *>(getDocument()->getObject(FeatName.c_str()));
    if (!dim) {
        throw Base::TypeError("CmdTechDrawNewRadiusDimension - dim not found\n");
    }
    dim->References2D.setValues(objs, subs);

    doCommand(Doc,"App.activeDocument().%s.addView(App.activeDocument().%s)",PageName.c_str(),FeatName.c_str());

    commitCommand();

    dim->recomputeFeature();

    //Horrible hack to force Tree update
    double x = objFeat->X.getValue();
    objFeat->X.setValue(x);
}

bool CmdTechDrawNewRadiusDimension::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_NewDiameterDimension
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawNewDiameterDimension);

CmdTechDrawNewDiameterDimension::CmdTechDrawNewDiameterDimension()
  : Command("TechDraw_NewDiameterDimension")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert a new diameter dimension");
    sToolTipText    = QT_TR_NOOP("Insert a new diameter dimension feature");
    sWhatsThis      = "TechDraw_Dimension_Diameter";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_Dimension_Diameter";
}

void CmdTechDrawNewDiameterDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    bool result = _checkSelection(this,1);
    if (!result)
        return;
    result = _checkDrawViewPart(this);
    if (!result)
        return;

    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();
    TechDraw::DrawViewPart * objFeat = 0;
    std::vector<std::string> SubNames;

    std::vector<Gui::SelectionObject>::iterator itSel = selection.begin();
    for (; itSel != selection.end(); itSel++)  {
        if ((*itSel).getObject()->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
            objFeat = static_cast<TechDraw::DrawViewPart*> ((*itSel).getObject());
            SubNames = (*itSel).getSubNames();
        }
    }
    TechDraw::DrawPage* page = objFeat->findParentPage();
    std::string PageName = page->getNameInDocument();

    TechDraw::DrawViewDimension *dim = 0;
    std::string FeatName = getUniqueObjectName("Dimension");

    std::vector<App::DocumentObject *> objs;
    std::vector<std::string> subs;

    int edgeType = _isValidSingleEdge(this);
    if (edgeType == isCircle) {
        objs.push_back(objFeat);
        subs.push_back(SubNames[0]);
    } else if (edgeType == isEllipse) {
        QMessageBox::StandardButton result =
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Ellipse Curve Warning"),
                             QObject::tr("Selected edge is an Ellipse.  Diameter will be approximate. Continue?"),
                             QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel);
        if (result == QMessageBox::Ok) {
            objs.push_back(objFeat);
            subs.push_back(SubNames[0]);
        } else {
            return;
        }
    } else if (edgeType == isBSplineCircle) {
        QMessageBox::StandardButton result =
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("BSpline Curve Warning"),
                             QObject::tr("Selected edge is a BSpline.  Diameter will be approximate. Continue?"),
                             QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel);
        if (result == QMessageBox::Ok) {
            objs.push_back(objFeat);
            subs.push_back(SubNames[0]);
        } else {
            return;
        }
    } else if (edgeType == isBSpline) {
        QMessageBox::StandardButton result =
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("BSpline Curve Warning"),
                             QObject::tr("Selected edge is a BSpline.  Diameter will be approximate. Continue?"),
                             QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel);
        if (result == QMessageBox::Ok) {
            objs.push_back(objFeat);
            subs.push_back(SubNames[0]);
        } else {
            return;
        }
    } else {
        std::stringstream edgeMsg;
        edgeMsg << "Selection for Diameter does not contain a circular edge (edge type: " << _edgeTypeToText(edgeType) << ")";
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Incorrect Selection"),
                                                   QObject::tr(edgeMsg.str().c_str()));
        return;
    }

    openCommand("Create Dimension");
    doCommand(Doc,"App.activeDocument().addObject('TechDraw::DrawViewDimension','%s')",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Type = '%s'",FeatName.c_str()
                                                       ,"Diameter");

    dim = dynamic_cast<TechDraw::DrawViewDimension *>(getDocument()->getObject(FeatName.c_str()));
    if (!dim) {
        throw Base::TypeError("CmdTechDrawNewDiameterDimension - dim not found\n");
    }
    dim->References2D.setValues(objs, subs);

    doCommand(Doc,"App.activeDocument().%s.addView(App.activeDocument().%s)",PageName.c_str(),FeatName.c_str());

    commitCommand();
    dim->recomputeFeature();

    //Horrible hack to force Tree update
    double x = objFeat->X.getValue();
    objFeat->X.setValue(x);
}

bool CmdTechDrawNewDiameterDimension::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_NewLengthDimension
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawNewLengthDimension);

CmdTechDrawNewLengthDimension::CmdTechDrawNewLengthDimension()
  : Command("TechDraw_NewLengthDimension")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert a new length dimension");
    sToolTipText    = QT_TR_NOOP("Insert a new length dimension");
    sWhatsThis      = "TechDraw_Dimension_Length";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_Dimension_Length";
}

void CmdTechDrawNewLengthDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    bool result = _checkSelection(this,2);
    if (!result)
        return;
    result = _checkDrawViewPart(this);
    if (!result)
        return;

    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();
    TechDraw::DrawViewPart * objFeat = 0;
    std::vector<std::string> SubNames;

    std::vector<Gui::SelectionObject>::iterator itSel = selection.begin();
    for (; itSel != selection.end(); itSel++)  {
        if ((*itSel).getObject()->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
            objFeat = static_cast<TechDraw::DrawViewPart*> ((*itSel).getObject());
            SubNames = (*itSel).getSubNames();
        }
    }
    TechDraw::DrawPage* page = objFeat->findParentPage();
    std::string PageName = page->getNameInDocument();

    TechDraw::DrawViewDimension *dim = 0;
    std::string FeatName = getUniqueObjectName("Dimension");
    std::string dimType;

    std::vector<App::DocumentObject *> objs;
    std::vector<std::string> subs;

    int edgeType = _isValidSingleEdge(this);
    if ((edgeType == isHorizontal) ||
        (edgeType == isVertical) ||
        (edgeType == isDiagonal)) {
        objs.push_back(objFeat);
        subs.push_back(SubNames[0]);
    } else if (_isValidVertexes(this)) {
        objs.push_back(objFeat);
        objs.push_back(objFeat);
        subs.push_back(SubNames[0]);
        subs.push_back(SubNames[1]);
    } else if ((_isValidEdgeToEdge(this) == isHorizontal) ||
               (_isValidEdgeToEdge(this) == isVertical) ||
               (_isValidEdgeToEdge(this) == isDiagonal) ||
               (_isValidEdgeToEdge(this) == isAngle)) {
        edgeType = _isValidEdgeToEdge(this);
        objs.push_back(objFeat);
        objs.push_back(objFeat);
        subs.push_back(SubNames[0]);
        subs.push_back(SubNames[1]);
    } else if (_isValidVertexToEdge(this)) {
        objs.push_back(objFeat);
        objs.push_back(objFeat);
        subs.push_back(SubNames[0]);
        subs.push_back(SubNames[1]);
    } else {
        std::stringstream edgeMsg;
        edgeMsg << "Need 2 Vertexes, 1 straight Edge, 1 Vertex/1 straight edge or 2 straight Edges for length Dimension (edge type: " << _edgeTypeToText(edgeType) << ")";
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Incorrect Selection"),
                                                   QObject::tr(edgeMsg.str().c_str()));
        return;
    }

    openCommand("Create Dimension");
    doCommand(Doc,"App.activeDocument().addObject('TechDraw::DrawViewDimension','%s')", FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Type = '%s'", FeatName.c_str()
                                                       , "Distance");
    dim = dynamic_cast<TechDraw::DrawViewDimension *>(getDocument()->getObject(FeatName.c_str()));
    if (!dim) {
        throw Base::TypeError("CmdTechDrawNewLengthDimension - dim not found\n");
    }
    dim->References2D.setValues(objs, subs);

    doCommand(Doc,"App.activeDocument().%s.addView(App.activeDocument().%s)",PageName.c_str(),FeatName.c_str());

    commitCommand();
    dim->recomputeFeature();
    TechDraw::pointPair pp = dim->getLinearPoints();
    Base::Vector3d mid = (pp.first + pp.second)/2.0;
    dim->X.setValue(mid.x);
    dim->Y.setValue(-mid.y);

    //Horrible hack to force Tree update (claimChildren)
    double x = objFeat->X.getValue();
    objFeat->X.setValue(x);
}

bool CmdTechDrawNewLengthDimension::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_NewDistanceXDimension
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawNewDistanceXDimension);

CmdTechDrawNewDistanceXDimension::CmdTechDrawNewDistanceXDimension()
  : Command("TechDraw_NewDistanceXDimension")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert a new horizontal dimension");
    sToolTipText    = QT_TR_NOOP("Insert a new horizontal distance dimension");
    sWhatsThis      = "TechDraw_Dimension_Horizontal";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_Dimension_Horizontal";
}

void CmdTechDrawNewDistanceXDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    bool result = _checkSelection(this,2);
    if (!result)
        return;
    result = _checkDrawViewPart(this);
    if (!result)
        return;

    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();
    TechDraw::DrawViewPart * objFeat = 0;
    std::vector<std::string> SubNames;

    std::vector<Gui::SelectionObject>::iterator itSel = selection.begin();
    for (; itSel != selection.end(); itSel++)  {
        if ((*itSel).getObject()->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
            objFeat = static_cast<TechDraw::DrawViewPart*> ((*itSel).getObject());
            SubNames = (*itSel).getSubNames();
        }
    }
    TechDraw::DrawPage* page = objFeat->findParentPage();
    std::string PageName = page->getNameInDocument();

    TechDraw::DrawViewDimension *dim = 0;
    std::string FeatName = getUniqueObjectName("Dimension");
    std::string dimType;

    std::vector<App::DocumentObject *> objs;
    std::vector<std::string> subs;

    int edgeType = _isValidSingleEdge(this);
    if ((edgeType == isHorizontal) ||
        (edgeType == isDiagonal)) {
        objs.push_back(objFeat);
        subs.push_back(SubNames[0]);
    } else if (_isValidVertexes(this)) {
        objs.push_back(objFeat);
        objs.push_back(objFeat);
        subs.push_back(SubNames[0]);
        subs.push_back(SubNames[1]);
    } else if ((_isValidEdgeToEdge(this) == isHorizontal) ||
               (_isValidEdgeToEdge(this) == isVertical)   ||
               (_isValidEdgeToEdge(this) == isDiagonal)   ||
               (_isValidEdgeToEdge(this) == isAngle)) {
        edgeType = _isValidEdgeToEdge(this);
        objs.push_back(objFeat);
        objs.push_back(objFeat);
        subs.push_back(SubNames[0]);
        subs.push_back(SubNames[1]);
    } else if (_isValidVertexToEdge(this)) {
        objs.push_back(objFeat);
        objs.push_back(objFeat);
        subs.push_back(SubNames[0]);
        subs.push_back(SubNames[1]);
    } else {
        std::stringstream edgeMsg;
        edgeMsg << "Need 2 Vertexes, 1 straight Edge, 1 Vertex/1 straight edge or 2 straight Edges for Horizontal Dimension (edge type: " << _edgeTypeToText(edgeType) << ")";
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Incorrect Selection"),
                                                   QObject::tr(edgeMsg.str().c_str()));
        return;
    }

    openCommand("Create Dimension");
    doCommand(Doc,"App.activeDocument().addObject('TechDraw::DrawViewDimension','%s')",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Type = '%s'",FeatName.c_str()
                                                       ,"DistanceX");

    dim = dynamic_cast<TechDraw::DrawViewDimension *>(getDocument()->getObject(FeatName.c_str()));
    if (!dim) {
        throw Base::TypeError("CmdTechDrawNewDistanceXDimension - dim not found\n");
    }
    dim->References2D.setValues(objs, subs);

    doCommand(Doc,"App.activeDocument().%s.addView(App.activeDocument().%s)",PageName.c_str(),FeatName.c_str());

    commitCommand();
    dim->recomputeFeature();
    TechDraw::pointPair pp = dim->getLinearPoints();
    Base::Vector3d mid = (pp.first + pp.second)/2.0;
    dim->X.setValue(mid.x);
    dim->Y.setValue(-mid.y);

    //Horrible hack to force Tree update
    double x = objFeat->X.getValue();
    objFeat->X.setValue(x);
}

bool CmdTechDrawNewDistanceXDimension::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_NewDistanceYDimension
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawNewDistanceYDimension);

CmdTechDrawNewDistanceYDimension::CmdTechDrawNewDistanceYDimension()
  : Command("TechDraw_NewDistanceYDimension")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert a new vertical dimension");
    sToolTipText    = QT_TR_NOOP("Insert a new vertical distance dimension");
    sWhatsThis      = "TechDraw_Dimension_Vertical";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_Dimension_Vertical";
}

void CmdTechDrawNewDistanceYDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    bool result = _checkSelection(this,2);
    if (!result)
        return;
    result = _checkDrawViewPart(this);
    if (!result)
        return;

    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();
    TechDraw::DrawViewPart * objFeat = 0;
    std::vector<std::string> SubNames;

    std::vector<Gui::SelectionObject>::iterator itSel = selection.begin();
    for (; itSel != selection.end(); itSel++)  {
        if ((*itSel).getObject()->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
            objFeat = static_cast<TechDraw::DrawViewPart*> ((*itSel).getObject());
            SubNames = (*itSel).getSubNames();
        }
    }
    TechDraw::DrawPage* page = objFeat->findParentPage();
    std::string PageName = page->getNameInDocument();

    TechDraw::DrawViewDimension *dim = 0;
    std::string FeatName = getUniqueObjectName("Dimension");
    std::string dimType;

    std::vector<App::DocumentObject *> objs;
    std::vector<std::string> subs;

    int edgeType = _isValidSingleEdge(this);
    if ((edgeType == isVertical) ||
        (edgeType == isDiagonal)) {
        objs.push_back(objFeat);
        subs.push_back(SubNames[0]);
    } else if (_isValidVertexes(this)) {
        objs.push_back(objFeat);
        objs.push_back(objFeat);
        subs.push_back(SubNames[0]);
        subs.push_back(SubNames[1]);
    } else if ((_isValidEdgeToEdge(this) == isVertical)   ||
               (_isValidEdgeToEdge(this) == isHorizontal) ||
               (_isValidEdgeToEdge(this) == isDiagonal) ||
               (_isValidEdgeToEdge(this) == isAngle))  {
        edgeType = _isValidEdgeToEdge(this);
        objs.push_back(objFeat);
        objs.push_back(objFeat);
        subs.push_back(SubNames[0]);
        subs.push_back(SubNames[1]);
    } else if (_isValidVertexToEdge(this)) {
        objs.push_back(objFeat);
        objs.push_back(objFeat);
        subs.push_back(SubNames[0]);
        subs.push_back(SubNames[1]);
    } else {
        std::stringstream edgeMsg;
        edgeMsg << "Need 2 Vertexes, 1 straight Edge, 1 Vertex/1 straight edge or 2 straight Edges for Horizontal Dimension (edge type: " << _edgeTypeToText(edgeType) << ")";
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Incorrect Selection"),
                                                   QObject::tr(edgeMsg.str().c_str()));
        return;
    }

    openCommand("Create Dimension");
    doCommand(Doc,"App.activeDocument().addObject('TechDraw::DrawViewDimension','%s')",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Type = '%s'",FeatName.c_str()
                                                       ,"DistanceY");
    dim = dynamic_cast<TechDraw::DrawViewDimension *>(getDocument()->getObject(FeatName.c_str()));
    if (!dim) {
        throw Base::TypeError("CmdTechDrawNewDistanceYDimension - dim not found\n");
    }
    dim->References2D.setValues(objs, subs);

    doCommand(Doc,"App.activeDocument().%s.addView(App.activeDocument().%s)",PageName.c_str(),FeatName.c_str());

    commitCommand();
    dim->recomputeFeature();
    TechDraw::pointPair pp = dim->getLinearPoints();
    Base::Vector3d mid = (pp.first + pp.second)/2.0;
    dim->X.setValue(mid.x);
    dim->Y.setValue(-mid.y);

    //Horrible hack to force Tree update
    double x = objFeat->X.getValue();
    objFeat->X.setValue(x);
}

bool CmdTechDrawNewDistanceYDimension::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_NewAngleDimension
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawNewAngleDimension);

CmdTechDrawNewAngleDimension::CmdTechDrawNewAngleDimension()
  : Command("TechDraw_NewAngleDimension")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert a new angle dimension");
    sToolTipText    = QT_TR_NOOP("Insert a new angle dimension");
    sWhatsThis      = "TechDraw_Dimension_Angle";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_Dimension_Angle";
}

void CmdTechDrawNewAngleDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    bool result = _checkSelection(this,2);
    if (!result)
        return;
    result = _checkDrawViewPart(this);
    if (!result)
        return;

    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();
    TechDraw::DrawViewPart * objFeat = 0;
    std::vector<std::string> SubNames;

    std::vector<Gui::SelectionObject>::iterator itSel = selection.begin();
    for (; itSel != selection.end(); itSel++)  {
        if ((*itSel).getObject()->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
            objFeat = static_cast<TechDraw::DrawViewPart*> ((*itSel).getObject());
            SubNames = (*itSel).getSubNames();
        }
    }
    TechDraw::DrawPage* page = objFeat->findParentPage();
    std::string PageName = page->getNameInDocument();

    TechDraw::DrawViewDimension *dim = 0;
    std::string FeatName = getUniqueObjectName("Dimension");

    std::vector<App::DocumentObject *> objs;
    std::vector<std::string> subs;

    int edgeType = _isValidEdgeToEdge(this);
    if (edgeType == isAngle) {
        objs.push_back(objFeat);
        objs.push_back(objFeat);
        subs.push_back(SubNames[0]);
        subs.push_back(SubNames[1]);
    } else {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Incorrect Selection"),
                                                   QObject::tr("Need two straight edges to make an Angle Dimension"));
        return;
    }

    openCommand("Create Dimension");
    doCommand(Doc,"App.activeDocument().addObject('TechDraw::DrawViewDimension','%s')",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Type = '%s'",FeatName.c_str()
                                                       ,"Angle");

    dim = dynamic_cast<TechDraw::DrawViewDimension *>(getDocument()->getObject(FeatName.c_str()));
    if (!dim) {
        throw Base::TypeError("CmdTechDrawNewAngleDimension - dim not found\n");
    }
    dim->References2D.setValues(objs, subs);

    doCommand(Doc,"App.activeDocument().%s.addView(App.activeDocument().%s)",PageName.c_str(),FeatName.c_str());

    commitCommand();
    dim->recomputeFeature();

    //Horrible hack to force Tree update
    double x = objFeat->X.getValue();
    objFeat->X.setValue(x);
}

bool CmdTechDrawNewAngleDimension::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_NewAngle3PtDimension
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawNewAngle3PtDimension);

CmdTechDrawNewAngle3PtDimension::CmdTechDrawNewAngle3PtDimension()
  : Command("TechDraw_NewAngle3PtDimension")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert a new 3 point Angle dimension");
    sToolTipText    = QT_TR_NOOP("Insert a new 3 point Angle dimension");
    sWhatsThis      = "TechDraw_Dimension_Angle3Pt";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_Dimension_Angle3Pt";
}

void CmdTechDrawNewAngle3PtDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    bool result = _checkSelection(this,3);
    if (!result)
        return;
    result = _checkDrawViewPart(this);
    if (!result)
        return;

    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();
    TechDraw::DrawViewPart * objFeat = 0;
    std::vector<std::string> SubNames;

    std::vector<Gui::SelectionObject>::iterator itSel = selection.begin();
    for (; itSel != selection.end(); itSel++)  {
        if ((*itSel).getObject()->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
            objFeat = static_cast<TechDraw::DrawViewPart*> ((*itSel).getObject());
            SubNames = (*itSel).getSubNames();
        }
    }
    TechDraw::DrawPage* page = objFeat->findParentPage();
    std::string PageName = page->getNameInDocument();

    TechDraw::DrawViewDimension *dim = 0;
    std::string FeatName = getUniqueObjectName("Dimension");

    std::vector<App::DocumentObject *> objs;
    std::vector<std::string> subs;

    if (_isValidVertexes(this, 3))  {
        objs.push_back(objFeat);
        objs.push_back(objFeat);
        objs.push_back(objFeat);
        subs.push_back(SubNames[0]);
        subs.push_back(SubNames[1]);
        subs.push_back(SubNames[2]);
    } else {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Incorrect Selection"),
                                                   QObject::tr("Need three points to make a 3 point Angle Dimension"));
        return;
    }

    openCommand("Create Dimension");
    doCommand(Doc,"App.activeDocument().addObject('TechDraw::DrawViewDimension','%s')",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Type = '%s'",FeatName.c_str()
                                                       ,"Angle3Pt");

    dim = dynamic_cast<TechDraw::DrawViewDimension *>(getDocument()->getObject(FeatName.c_str()));
    if (!dim) {
        throw Base::TypeError("CmdTechDrawNewAngle3PtDimension - dim not found\n");
    }
    dim->References2D.setValues(objs, subs);

    doCommand(Doc,"App.activeDocument().%s.addView(App.activeDocument().%s)",PageName.c_str(),FeatName.c_str());

    commitCommand();
    dim->recomputeFeature();

    //Horrible hack to force Tree update
    double x = objFeat->X.getValue();
    objFeat->X.setValue(x);
}

bool CmdTechDrawNewAngle3PtDimension::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}


//! link 3D geometry to Dimension(s) on a Page
//TODO: should we present all potential Dimensions from all Pages?
//===========================================================================
// TechDraw_LinkDimension
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawLinkDimension);

CmdTechDrawLinkDimension::CmdTechDrawLinkDimension()
  : Command("TechDraw_LinkDimension")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Link a dimension to 3D geometry");
    sToolTipText    = QT_TR_NOOP("Link a dimension to 3D geometry");
    sWhatsThis      = "TechDraw_Dimension_Link";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_Dimension_Link";
}

void CmdTechDrawLinkDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(this);
    if (!page) {
        return;
    }
    std::string PageName = page->getNameInDocument();

    bool result = _checkSelection(this,2);
    if (!result)
        return;

    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    App::DocumentObject* obj3D = 0;
    std::vector<App::DocumentObject*> parts;
    std::vector<std::string> subs;

    std::vector<Gui::SelectionObject>::iterator itSel = selection.begin();
    for (; itSel != selection.end(); itSel++)  {
        if ((*itSel).getObject()->isDerivedFrom(Part::Feature::getClassTypeId())) {
            obj3D = ((*itSel).getObject());
            std::vector<std::string> subList = (*itSel).getSubNames();
            for (auto& s:subList) {
                parts.push_back(obj3D);
                subs.push_back(s);
            }
        }
    }

    if (!obj3D) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Incorrect Selection"),
                                                   QObject::tr("There is no 3D object in your selection"));
        return;
    }

    if (subs.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Incorrect Selection"),
                                                   QObject::tr("There are no 3D Edges or Vertices in your selection"));
        return;
    }


    // dialog to select the Dimension to link
    Gui::Control().showDialog(new TaskDlgLinkDim(parts,subs,page));

    page->getDocument()->recompute();                                  //still need to recompute in Gui. why?
}

bool CmdTechDrawLinkDimension::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    bool taskInProgress = false;
    if (havePage) {
        taskInProgress = Gui::Control().activeDialog();
    }
    return (havePage && haveView && !taskInProgress);
}

void CreateTechDrawCommandsDims(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdTechDrawNewDimension());
    rcCmdMgr.addCommand(new CmdTechDrawNewRadiusDimension());
    rcCmdMgr.addCommand(new CmdTechDrawNewDiameterDimension());
    rcCmdMgr.addCommand(new CmdTechDrawNewLengthDimension());
    rcCmdMgr.addCommand(new CmdTechDrawNewDistanceXDimension());
    rcCmdMgr.addCommand(new CmdTechDrawNewDistanceYDimension());
    rcCmdMgr.addCommand(new CmdTechDrawNewAngleDimension());
    rcCmdMgr.addCommand(new CmdTechDrawNewAngle3PtDimension());
    rcCmdMgr.addCommand(new CmdTechDrawLinkDimension());
}

//===========================================================================
// Selection Validation Helpers
//===========================================================================

//! common checks of Selection for Dimension commands
//non-empty selection, no more than maxObjs selected and at least 1 DrawingPage exists
bool _checkSelection(Gui::Command* cmd, unsigned maxObjs) {
    std::vector<Gui::SelectionObject> selection = cmd->getSelection().getSelectionEx();
    if (selection.size() == 0) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Incorrect selection"),
                             QObject::tr("Select an object first"));
        return false;
    }

    const std::vector<std::string> SubNames = selection[0].getSubNames();
    if (SubNames.size() > maxObjs){
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Incorrect selection"),
            QObject::tr("Too many objects selected"));
        return false;
    }

    std::vector<App::DocumentObject*> pages = cmd->getDocument()->getObjectsOfType(TechDraw::DrawPage::getClassTypeId());
    if (pages.empty()){
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Incorrect selection"),
            QObject::tr("Create a page first."));
        return false;
    }
    return true;
}

bool _checkDrawViewPart(Gui::Command* cmd) {
    std::vector<Gui::SelectionObject> selection = cmd->getSelection().getSelectionEx();
    auto objFeat( dynamic_cast<TechDraw::DrawViewPart *>(selection[0].getObject()) );
    if( !objFeat ) {
        QMessageBox::warning( Gui::getMainWindow(),
                              QObject::tr("Incorrect selection"),
                              QObject::tr("No View of a Part in selection.") );
        return false;
    }
    return true;
}

bool _checkPartFeature(Gui::Command* cmd) {
    bool result = false;
    std::vector<Gui::SelectionObject> selection = cmd->getSelection().getSelectionEx();
    std::vector<Gui::SelectionObject>::iterator itSel = selection.begin();
    for (; itSel != selection.end(); itSel++) {
        if (itSel->isDerivedFrom(Part::Feature::getClassTypeId())) {
            result = true;
        }
    }
    if(!result) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Incorrect selection"),
                             QObject::tr("No Feature with Shape in selection."));
    }
    return result;
}

//! verify that Selection contains a valid Geometry for a single Edge Dimension
int _isValidSingleEdge(Gui::Command* cmd) {
    auto edgeType( isInvalid );
    auto selection( cmd->getSelection().getSelectionEx() );

    auto objFeat( dynamic_cast<TechDraw::DrawViewPart *>(selection[0].getObject()) );
    if( objFeat == nullptr ) {
        return isInvalid;
    }

    const std::vector<std::string> SubNames = selection[0].getSubNames();
    if (SubNames.size() == 1) {                                                 //only 1 subshape selected
        if (TechDraw::DrawUtil::getGeomTypeFromName(SubNames[0]) == "Edge") {                                //the Name starts with "Edge"
            int GeoId( TechDraw::DrawUtil::getIndexFromName(SubNames[0]) );
            TechDrawGeometry::BaseGeom* geom = objFeat->getProjEdgeByIndex(GeoId);
            if (!geom) {
                Base::Console().Error("Logic Error: no geometry for GeoId: %d\n",GeoId);
                return isInvalid;
            }

            if(geom->geomType == TechDrawGeometry::GENERIC) {
                TechDrawGeometry::Generic* gen1 = static_cast<TechDrawGeometry::Generic *>(geom);
                if(gen1->points.size() > 2) {                                   //the edge is a polyline
                    return isInvalid;
                }
                Base::Vector2d line = gen1->points.at(1) - gen1->points.at(0);
                if(fabs(line.y) < FLT_EPSILON ) {
                    edgeType = isHorizontal;
                } else if(fabs(line.x) < FLT_EPSILON) {
                    edgeType = isVertical;
                } else {
                    edgeType = isDiagonal;
                }
            } else if (geom->geomType == TechDrawGeometry::CIRCLE ||
                       geom->geomType == TechDrawGeometry::ARCOFCIRCLE ) {
                edgeType = isCircle;
            } else if (geom->geomType == TechDrawGeometry::ELLIPSE ||
                       geom->geomType == TechDrawGeometry::ARCOFELLIPSE) {
                edgeType = isEllipse;
            } else if (geom->geomType == TechDrawGeometry::BSPLINE) {
                TechDrawGeometry::BSpline* spline = static_cast<TechDrawGeometry::BSpline*>(geom);
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

//! verify that Selection contains valid geometries for a Vertex based Dimensions
bool _isValidVertexes(Gui::Command* cmd, int count) {
    std::vector<Gui::SelectionObject> selection = cmd->getSelection().getSelectionEx();
    const std::vector<std::string> SubNames = selection[0].getSubNames();
    bool isValid = true;
    if(SubNames.size() == (unsigned) count) {
        for (auto& s: SubNames) {
            if (TechDraw::DrawUtil::getGeomTypeFromName(s) != "Vertex") {
                isValid = false;
                break;
            }
        }
    } else {
        isValid = false;
    }
    return isValid;
}

//! verify that the Selection contains valid geometries for an Edge to Edge Dimension
int _isValidEdgeToEdge(Gui::Command* cmd) {
//TODO: can the edges be in 2 different features??
    std::vector<Gui::SelectionObject> selection = cmd->getSelection().getSelectionEx();

    auto objFeat0( dynamic_cast<TechDraw::DrawViewPart *>(selection[0].getObject()) );
    // getObject() can return null pointer, or dynamic_cast can fail
    if ( !objFeat0 ) {
        Base::Console().Error("Logic error in _isValidEdgeToEdge()\n");
        return isInvalid;
    }

    int edgeType = isInvalid;
    const std::vector<std::string> SubNames = selection[0].getSubNames();
    if(SubNames.size() == 2) {                                         //there are 2
        if (TechDraw::DrawUtil::getGeomTypeFromName(SubNames[0]) == "Edge" &&                      //they both start with "Edge"
            TechDraw::DrawUtil::getGeomTypeFromName(SubNames[1]) == "Edge") {
            int GeoId0( TechDraw::DrawUtil::getIndexFromName(SubNames[0]) );
            int GeoId1( TechDraw::DrawUtil::getIndexFromName(SubNames[1]) );
            TechDrawGeometry::BaseGeom* geom0 = objFeat0->getProjEdgeByIndex(GeoId0);
            TechDrawGeometry::BaseGeom* geom1 = objFeat0->getProjEdgeByIndex(GeoId1);
            if ((!geom0) || (!geom1)) {
                Base::Console().Error("Logic Error: no geometry for GeoId: %d or GeoId: %d\n",GeoId0,GeoId1);
                return isInvalid;
            }

            if(geom0->geomType == TechDrawGeometry::GENERIC &&
               geom1->geomType == TechDrawGeometry::GENERIC) {
                TechDrawGeometry::Generic *gen0 = static_cast<TechDrawGeometry::Generic *>(geom0);
                TechDrawGeometry::Generic *gen1 = static_cast<TechDrawGeometry::Generic *>(geom1);
                if(gen0->points.size() > 2 ||
                   gen1->points.size() > 2) {                          //the edge is a polyline
                    return isInvalid;
                }
                Base::Vector2d line0 = gen0->points.at(1) - gen0->points.at(0);
                Base::Vector2d line1 = gen1->points.at(1) - gen1->points.at(0);
                double xprod = fabs(line0.x * line1.y - line0.y * line1.x);
                if(xprod > FLT_EPSILON) {                              //edges are not parallel
                    return isAngle;
                }
                if(fabs(line0.x) < FLT_EPSILON && fabs(line1.x) < FLT_EPSILON) {   //both horizontal
                    edgeType = isHorizontal;
                } else if(fabs(line0.y) < FLT_EPSILON && fabs(line1.y) < FLT_EPSILON) {  //both vertical
                    edgeType = isVertical;
                } else {
                    edgeType = isDiagonal;
                }
            } else {
                return isInvalid;
            }
        }
    }
    return edgeType;
}

//! verify that the Selection contains valid geometries for a Vertex to Edge Dimension
bool _isValidVertexToEdge(Gui::Command* cmd) {
    bool result = false;
    std::vector<Gui::SelectionObject> selection = cmd->getSelection().getSelectionEx();
    TechDraw::DrawViewPart* objFeat0 = static_cast<TechDraw::DrawViewPart *>(selection[0].getObject());
    //TechDraw::DrawViewPart* objFeat1 = static_castt<TechDraw::DrawViewPart *>(selection[1].getObject());
    const std::vector<std::string> SubNames = selection[0].getSubNames();
    if(SubNames.size() == 2) {                                         //there are 2
        int eId,vId;
        TechDrawGeometry::BaseGeom* e;
        TechDrawGeometry::Vertex* v;
        if (TechDraw::DrawUtil::getGeomTypeFromName(SubNames[0]) == "Edge" &&
            TechDraw::DrawUtil::getGeomTypeFromName(SubNames[1]) == "Vertex") {
            eId = TechDraw::DrawUtil::getIndexFromName(SubNames[0]);
            vId = TechDraw::DrawUtil::getIndexFromName(SubNames[1]);
        } else if (TechDraw::DrawUtil::getGeomTypeFromName(SubNames[1]) == "Edge" &&
            TechDraw::DrawUtil::getGeomTypeFromName(SubNames[0]) == "Vertex") {
            eId = TechDraw::DrawUtil::getIndexFromName(SubNames[1]);
            vId = TechDraw::DrawUtil::getIndexFromName(SubNames[0]);
        } else {
            return false;
        }
        e = objFeat0->getProjEdgeByIndex(eId);
        v = objFeat0->getProjVertexByIndex(vId);
        if ((!e) || (!v)) {
            Base::Console().Error("Logic Error: no geometry for GeoId: %d or GeoId: %d\n",eId,vId);
            return false;
        }
        if (e->geomType != TechDrawGeometry::GENERIC)  {      //only vertex-line for now.
            return false;
        }
        result = true;
    }
    return result;
}

char* _edgeTypeToText(int e)
{
    char* result;
    switch(e) {
        case isInvalid:
            result = "invalid";
            break;
        case isHorizontal:
            result = "horizontal";
            break;
        case isVertical:
            result = "vertical";
            break;
        case isDiagonal:
            result = "diagonal";
            break;
        case isCircle:
            result = "circle";
            break;
        case isEllipse:
            result = "ellipse";
            break;
        case isBSpline:
            result = "bspline";
            break;
        case isBSplineCircle:
            result = "circular bspline";
            break;
        case isAngle:
            result = "angle";
            break;
        case isAngle3Pt:
            result = "angle3";
            break;
        default:
            result = "unknown";
    }
    return result;
}

//bool _checkActive(Gui::Command* cmd, Base::Type classType, bool needSubs)
//{
//    //need a page, a selected classType and [a subelement]
//    bool active = false;
//    if (cmd->hasActiveDocument()) {
//        auto drawPageType( TechDraw::DrawPage::getClassTypeId() );
//        auto selPages = cmd->getDocument()->getObjectsOfType(drawPageType);
//        if (!selPages.empty()) {
//            std::vector<Gui::SelectionObject> selection = cmd->getSelection().getSelectionEx();
//            for (auto& s:selection) {
//                if (s.getObject()->isDerivedFrom(classType)) {
//                    if (needSubs && !(s.getSubNames().empty())) {
//                        active = true;
//                        break;
//                    }
//                }
//            }
//         }
//    }
//    return active;
//}
