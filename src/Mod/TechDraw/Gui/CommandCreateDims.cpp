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
# include <QApplication>
# include <QMessageBox>
# include <iostream>
# include <string>
# include <sstream>
# include <cstdlib>
# include <exception>
#endif  //#ifndef _PreComp_

#include <QGraphicsView>

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Base/Exception.h>
#include <Base/Console.h>
#include <Base/Type.h>
#include <Gui/Action.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Gui/SelectionObject.h>
#include <Gui/MainWindow.h>
#include <Gui/FileDialog.h>
#include <Gui/ViewProvider.h>

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
# include <Mod/TechDraw/App/Preferences.h>

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

void execHExtent(Gui::Command* cmd);
void execVExtent(Gui::Command* cmd);


//NOTE: this is not shown in toolbar and doesn't always work right in the menu.
//      should be removed.
//===========================================================================
// TechDraw_NewDimension
//===========================================================================

// this is deprecated. use individual add dimension commands.

DEF_STD_CMD_A(CmdTechDrawDimension)

CmdTechDrawDimension::CmdTechDrawDimension()
  : Command("TechDraw_Dimension")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert Dimension");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_Dimension";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_Dimension";
}

void CmdTechDrawDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
//    bool result = _checkSelection(this,2);
//    if (!result)
//        return;
//    result = _checkDrawViewPart(this);
//    if (!result)
//        return;

////do we still need to pick DVPs out of selection? or should we complain about junk?
//    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();
//    TechDraw::DrawViewPart* objFeat = 0;
//    std::vector<std::string> SubNames;
//    std::vector<Gui::SelectionObject>::iterator itSel = selection.begin();
//    for (; itSel != selection.end(); itSel++)  {
//        if ((*itSel).getObject()->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
//            objFeat = static_cast<TechDraw::DrawViewPart*> ((*itSel).getObject());
//            SubNames = (*itSel).getSubNames();
//        }
//    }
//    TechDraw::DrawPage* page = objFeat->findParentPage();
//    std::string PageName = page->getNameInDocument();

//    TechDraw::DrawViewDimension *dim = 0;
//    std::string FeatName = getUniqueObjectName("Dimension");
//    std::string dimType;

//    std::vector<App::DocumentObject *> objs;
//    std::vector<std::string> subs;

//    int edgeType = _isValidSingleEdge(this);

//    if (edgeType) {
//        if (edgeType < isCircle) {
//            dimType = "Distance";
//            objs.push_back(objFeat);
//            subs.push_back(SubNames[0]);
//        } else if (edgeType == isCircle) {
//            dimType = "Radius";
//        } else {
//            dimType = "Radius";
//        }
//    } else if (_isValidVertexes(this)) {
//        dimType = "Distance";
//        objs.push_back(objFeat);
//        objs.push_back(objFeat);
//        subs.push_back(SubNames[0]);
//        subs.push_back(SubNames[1]);
//    } else if (_isValidEdgeToEdge(this)) {
//        int edgeCase = _isValidEdgeToEdge(this);
//        objs.push_back(objFeat);
//        objs.push_back(objFeat);
//        subs.push_back(SubNames[0]);
//        subs.push_back(SubNames[1]);
//        switch (edgeCase) {
//             case isHorizontal:
//                dimType = "DistanceX";
//                break;
//             case isVertical:
//                dimType = "DistanceY";
//                break;
//            case isDiagonal:
//                dimType = "Distance";
//                break;
//            case isAngle:
//                dimType = "Angle";
//            default:
//                break;
//        }
//    } else if (_isValidVertexToEdge(this)) {
//        dimType = "Distance";
//        objs.push_back(objFeat);
//        objs.push_back(objFeat);
//        subs.push_back(SubNames[0]);
//        subs.push_back(SubNames[1]);
//    } else {
//        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Incorrect Selection"),
//                                                   QObject::tr("Can not make a Dimension from this selection"));
//        return;
//    }

//    openCommand(QT_TRANSLATE_NOOP("Command", "Create Dimension"));
//    doCommand(Doc,"App.activeDocument().addObject('TechDraw::DrawViewDimension','%s')",FeatName.c_str());
//    doCommand(Doc,"App.activeDocument().%s.Type = '%s'",FeatName.c_str()
//                                                       ,dimType.c_str());

//    dim = dynamic_cast<TechDraw::DrawViewDimension *>(getDocument()->getObject(FeatName.c_str()));
//    if (!dim) {
//        throw Base::TypeError("CmdTechDrawNewDimension - dim not found\n");
//    }
//    dim->References2D.setValues(objs, subs);

//    doCommand(Doc,"App.activeDocument().%s.addView(App.activeDocument().%s)",PageName.c_str(),FeatName.c_str());
//    commitCommand();
//    dim->recomputeFeature();

//    //Horrible hack to force Tree update
//    double x = objFeat->X.getValue();
//    objFeat->X.setValue(x);
}

bool CmdTechDrawDimension::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_RadiusDimension
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawRadiusDimension)

CmdTechDrawRadiusDimension::CmdTechDrawRadiusDimension()
  : Command("TechDraw_RadiusDimension")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert Radius Dimension");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_RadiusDimension";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_RadiusDimension";
}

void CmdTechDrawRadiusDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    bool result = _checkSelection(this,1);
    if (!result)
        return;
    result = _checkDrawViewPart(this);
    if (!result)
        return;

    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();
    TechDraw::DrawViewPart * objFeat = nullptr;
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

    TechDraw::DrawViewDimension *dim = nullptr;
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
        QMessageBox::critical(Gui::getMainWindow(), QObject::tr("BSpline Curve Error"),
                             QObject::tr("Selected edge is a BSpline and a radius can not be calculated."));
        return;
    } else {
        QMessageBox::warning(
            Gui::getMainWindow(), QObject::tr("Incorrect Selection"),
            QObject::tr("Selection for Radius does not contain a circular edge "
                        "(edge type: %1)")
                .arg(QString::fromStdString(_edgeTypeToText(edgeType))));
        return;
    }

    openCommand(QT_TRANSLATE_NOOP("Command", "Create Dimension"));
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

bool CmdTechDrawRadiusDimension::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_DiameterDimension
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawDiameterDimension)

CmdTechDrawDiameterDimension::CmdTechDrawDiameterDimension()
  : Command("TechDraw_DiameterDimension")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert Diameter Dimension");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_DiameterDimension";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_DiameterDimension";
}

void CmdTechDrawDiameterDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    bool result = _checkSelection(this,1);
    if (!result)
        return;
    result = _checkDrawViewPart(this);
    if (!result)
        return;

    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();
    TechDraw::DrawViewPart * objFeat = nullptr;
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

    TechDraw::DrawViewDimension *dim = nullptr;
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
        QMessageBox::critical(Gui::getMainWindow(), QObject::tr("BSpline Curve Error"),
                             QObject::tr("Selected edge is a BSpline and a diameter can not be calculated."));
        return;
    } else {
      QMessageBox::warning(
          Gui::getMainWindow(), QObject::tr("Incorrect Selection"),
          QObject::tr("Selection for Diameter does not contain a circular edge "
                      "(edge type: %1)")
              .arg(QString::fromStdString(_edgeTypeToText(edgeType))));
      return;
    }

    openCommand(QT_TRANSLATE_NOOP("Command", "Create Dimension"));
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

bool CmdTechDrawDiameterDimension::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_LengthDimension
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawLengthDimension)

CmdTechDrawLengthDimension::CmdTechDrawLengthDimension()
  : Command("TechDraw_LengthDimension")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert Length Dimension");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_LengthDimension";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_LengthDimension";
}

void CmdTechDrawLengthDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    bool result = _checkSelection(this,2);
    if (!result)
        return;
    result = _checkDrawViewPart(this);
    if (!result)
        return;

    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();
    TechDraw::DrawViewPart * objFeat = nullptr;
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

    TechDraw::DrawViewDimension *dim = nullptr;
    std::string FeatName = getUniqueObjectName("Dimension");
    std::string dimType;

    std::vector<App::DocumentObject *> objs;
    std::vector<std::string> subs;

    if (_isValidSingleEdge(this) ) {
        objs.push_back(objFeat);
        subs.push_back(SubNames[0]);
    } else if (  _isValidVertexes(this) || 
                (_isValidEdgeToEdge(this) == isVertical)   ||
                (_isValidEdgeToEdge(this) == isHorizontal) ||
                (_isValidEdgeToEdge(this) == isDiagonal) ||
                (_isValidVertexToEdge(this)) ) {
        objs.push_back(objFeat);
        objs.push_back(objFeat);
        subs.push_back(SubNames[0]);
        subs.push_back(SubNames[1]);
    } else {
      QMessageBox::warning(Gui::getMainWindow(),
                           QObject::tr("Incorrect Selection"),
                           QObject::tr("Need 2 Vertexes, 2 Edges or 1 Vertex "
                                       "and 1 Edge for Distance Dimension"));
      return;
    }

    openCommand(QT_TRANSLATE_NOOP("Command", "Create Dimension"));
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
    double fontSize = Preferences::dimFontSizeMM();
    dim->Y.setValue(-mid.y + 0.5 * fontSize);

    //Horrible hack to force Tree update (claimChildren)
    double x = objFeat->X.getValue();
    objFeat->X.setValue(x);
}

bool CmdTechDrawLengthDimension::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_HorizontalDimension
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawHorizontalDimension)

CmdTechDrawHorizontalDimension::CmdTechDrawHorizontalDimension()
  : Command("TechDraw_HorizontalDimension")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert Horizontal Dimension");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_HorizontalDimension";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_HorizontalDimension";
    sAccel          = "SHIFT+H";
}

void CmdTechDrawHorizontalDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    bool result = _checkSelection(this,2);
    if (!result)
        return;
    result = _checkDrawViewPart(this);
    if (!result)
        return;

    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();
    TechDraw::DrawViewPart * objFeat = nullptr;
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

    TechDraw::DrawViewDimension *dim = nullptr;
    std::string FeatName = getUniqueObjectName("Dimension");
    std::string dimType;

    std::vector<App::DocumentObject *> objs;
    std::vector<std::string> subs;

    if (_isValidSingleEdge(this) ) {
        objs.push_back(objFeat);
        subs.push_back(SubNames[0]);
    } else if (  _isValidVertexes(this) || 
                (_isValidEdgeToEdge(this) == isVertical)   ||
                (_isValidEdgeToEdge(this) == isHorizontal) ||
                (_isValidEdgeToEdge(this) == isDiagonal) ||
                (_isValidVertexToEdge(this)) ) {
        objs.push_back(objFeat);
        objs.push_back(objFeat);
        subs.push_back(SubNames[0]);
        subs.push_back(SubNames[1]);
    } else {
      QMessageBox::warning(Gui::getMainWindow(),
                           QObject::tr("Incorrect Selection"),
                           QObject::tr("Need 2 Vertexes, 2 Edges or 1 Vertex "
                                       "and 1 Edge for Horizontal Dimension"));
      return;
    }

    openCommand(QT_TRANSLATE_NOOP("Command", "Create Dimension"));
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
    double fontSize = Preferences::dimFontSizeMM();
    dim->Y.setValue(-mid.y + 0.5 * fontSize);

    //Horrible hack to force Tree update
    double x = objFeat->X.getValue();
    objFeat->X.setValue(x);
}

bool CmdTechDrawHorizontalDimension::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_VerticalDimension
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawVerticalDimension)

CmdTechDrawVerticalDimension::CmdTechDrawVerticalDimension()
  : Command("TechDraw_VerticalDimension")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert Vertical Dimension");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_VerticalDimension";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_VerticalDimension";
    sAccel          = "SHIFT+V";
}

void CmdTechDrawVerticalDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    bool result = _checkSelection(this,2);
    if (!result)
        return;
    result = _checkDrawViewPart(this);
    if (!result)
        return;

    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();
    TechDraw::DrawViewPart * objFeat = nullptr;
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

    TechDraw::DrawViewDimension *dim = nullptr;
    std::string FeatName = getUniqueObjectName("Dimension");
    std::string dimType;

    std::vector<App::DocumentObject *> objs;
    std::vector<std::string> subs;

    if (_isValidSingleEdge(this) ) {
        objs.push_back(objFeat);
        subs.push_back(SubNames[0]);
    } else if (  _isValidVertexes(this) || 
                (_isValidEdgeToEdge(this) == isVertical)   ||
                (_isValidEdgeToEdge(this) == isHorizontal) ||
                (_isValidEdgeToEdge(this) == isDiagonal) ||
                (_isValidVertexToEdge(this)) ) {
        objs.push_back(objFeat);
        objs.push_back(objFeat);
        subs.push_back(SubNames[0]);
        subs.push_back(SubNames[1]);
    } else {
      QMessageBox::warning(Gui::getMainWindow(),
                           QObject::tr("Incorrect Selection"),
                           QObject::tr("Need 2 Vertexes, 2 Edges or 1 Vertex "
                                       "and 1 Edge for Vertical Dimension"));
      return;
    }

    openCommand(QT_TRANSLATE_NOOP("Command", "Create Dimension"));
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
    double fontSize = Preferences::dimFontSizeMM();
    dim->Y.setValue(-mid.y + 0.5 * fontSize);

    //Horrible hack to force Tree update
    double x = objFeat->X.getValue();
    objFeat->X.setValue(x);
}

bool CmdTechDrawVerticalDimension::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_AngleDimension
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawAngleDimension)

CmdTechDrawAngleDimension::CmdTechDrawAngleDimension()
  : Command("TechDraw_AngleDimension")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert Angle Dimension");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_AngleDimension";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_AngleDimension";
}

void CmdTechDrawAngleDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    bool result = _checkSelection(this,2);
    if (!result)
        return;
    result = _checkDrawViewPart(this);
    if (!result)
        return;

    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();
    TechDraw::DrawViewPart * objFeat = nullptr;
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

    TechDraw::DrawViewDimension *dim = nullptr;
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

    openCommand(QT_TRANSLATE_NOOP("Command", "Create Dimension"));
    doCommand(Doc,"App.activeDocument().addObject('TechDraw::DrawViewDimension','%s')",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Type = '%s'",FeatName.c_str()
                                                       ,"Angle");

    dim = dynamic_cast<TechDraw::DrawViewDimension *>(getDocument()->getObject(FeatName.c_str()));
    if (!dim) {
        throw Base::TypeError("CmdTechDrawAngleDimension - dim not found\n");
    }
    dim->References2D.setValues(objs, subs);

    doCommand(Doc,"App.activeDocument().%s.addView(App.activeDocument().%s)",PageName.c_str(),FeatName.c_str());

    commitCommand();
    dim->recomputeFeature();

    //Horrible hack to force Tree update
    double x = objFeat->X.getValue();
    objFeat->X.setValue(x);
}

bool CmdTechDrawAngleDimension::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_3PtAngleDimension
//===========================================================================

DEF_STD_CMD_A(CmdTechDraw3PtAngleDimension)

CmdTechDraw3PtAngleDimension::CmdTechDraw3PtAngleDimension()
  : Command("TechDraw_3PtAngleDimension")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert 3-Point Angle Dimension");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_3PtAngleDimension";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_3PtAngleDimension";
}

void CmdTechDraw3PtAngleDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    bool result = _checkSelection(this,3);
    if (!result)
        return;
    result = _checkDrawViewPart(this);
    if (!result)
        return;

    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();
    TechDraw::DrawViewPart * objFeat = nullptr;
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

    TechDraw::DrawViewDimension *dim = nullptr;
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

    openCommand(QT_TRANSLATE_NOOP("Command", "Create Dimension"));
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

bool CmdTechDraw3PtAngleDimension::isActive(void)
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

DEF_STD_CMD_A(CmdTechDrawLinkDimension)

CmdTechDrawLinkDimension::CmdTechDrawLinkDimension()
  : Command("TechDraw_LinkDimension")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Link Dimension to 3D Geometry");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_LinkDimension";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_LinkDimension";
}

void CmdTechDrawLinkDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(this);
    if (!page) {
        return;
    }

    bool result = _checkSelection(this,2);
    if (!result)
        return;

    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx(nullptr,
            App::DocumentObject::getClassTypeId(), Gui::ResolveMode::NoResolve);

    App::DocumentObject* obj3D = nullptr;
    std::vector<App::DocumentObject*> parts;
    std::vector<std::string> subs;

    std::vector<Gui::SelectionObject>::iterator itSel = selection.begin();
    for (; itSel != selection.end(); itSel++)  {
        obj3D = ((*itSel).getObject());
        std::vector<std::string> subList = (*itSel).getSubNames();
        for (auto& s:subList) {
            parts.push_back(obj3D);
            subs.push_back(s);
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

//===========================================================================
// TechDraw_ExtentGroup
//===========================================================================

DEF_STD_CMD_ACL(CmdTechDrawExtentGroup)

CmdTechDrawExtentGroup::CmdTechDrawExtentGroup()
  : Command("TechDraw_ExtentGroup")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert Extent Dimension");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_ExtentGroup";
    sStatusTip      = sToolTipText;
//    eType           = ForEdit;
}

void CmdTechDrawExtentGroup::activated(int iMsg)
{
//    Base::Console().Message("CMD::ExtentGrp - activated(%d)\n", iMsg);
    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    if (dlg != nullptr) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task In Progress"),
            QObject::tr("Close active task dialog and try again."));
        return;
    }

    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    pcAction->setIcon(pcAction->actions().at(iMsg)->icon());
    switch(iMsg) {
        case 0:
            execHExtent(this);
            break;
        case 1:
            execVExtent(this);
            break;
        default:
            Base::Console().Message("CMD::ExtGrp - invalid iMsg: %d\n",iMsg);
    };
}

Gui::Action * CmdTechDrawExtentGroup::createAction(void)
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* p1 = pcAction->addAction(QString());
    p1->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_HorizontalExtentDimension"));
    p1->setObjectName(QString::fromLatin1("TechDraw_HorizontalExtentDimension"));
    p1->setWhatsThis(QString::fromLatin1("TechDraw_HorizontalExtentDimension"));
    QAction* p2 = pcAction->addAction(QString());
    p2->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_VerticalExtentDimension"));
    p2->setObjectName(QString::fromLatin1("TechDraw_VerticalExtentDimension"));
    p2->setWhatsThis(QString::fromLatin1("TechDraw_VerticalExtentDimension"));

    _pcAction = pcAction;
    languageChange();

    pcAction->setIcon(p1->icon());
    int defaultId = 0;
    pcAction->setProperty("defaultAction", QVariant(defaultId));

    return pcAction;
}

void CmdTechDrawExtentGroup::languageChange()
{
    Command::languageChange();

    if (!_pcAction)
        return;
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    QAction* arc1 = a[0];
    arc1->setText(QApplication::translate("CmdTechDrawExtentGroup","Horizontal Extent"));
    arc1->setToolTip(QApplication::translate("TechDraw_HorizontalExtent","Insert Horizontal Extent Dimension"));
    arc1->setStatusTip(arc1->toolTip());
    QAction* arc2 = a[1];
    arc2->setText(QApplication::translate("CmdTechDrawExtentGroup","Vertical Extent"));
    arc2->setToolTip(QApplication::translate("TechDraw_VerticalExtentDimension","Insert Vertical Extent Dimension"));
    arc2->setStatusTip(arc2->toolTip());
}

bool CmdTechDrawExtentGroup::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, false);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_HorizontalExtentDimension
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawHorizontalExtentDimension)

CmdTechDrawHorizontalExtentDimension::CmdTechDrawHorizontalExtentDimension()
  : Command("TechDraw_HorizontalExtentDimension")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert Horizontal Extent Dimension");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_HorizontalExtentDimension";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_HorizontalExtentDimension";
}

void CmdTechDrawHorizontalExtentDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    if (dlg != nullptr) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task In Progress"),
            QObject::tr("Close active task dialog and try again."));
        return;
    }

    execHExtent(this);
}

bool CmdTechDrawHorizontalExtentDimension::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, false);
    return (havePage && haveView);
}

void execHExtent(Gui::Command* cmd)
{
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(cmd);
    if (!page) {
        return;
    }

    std::vector<Gui::SelectionObject> selection = cmd->getSelection().getSelectionEx();
    TechDraw::DrawViewPart* baseFeat = nullptr;
    if (!selection.empty()) {
        baseFeat =  dynamic_cast<TechDraw::DrawViewPart *>(selection[0].getObject());
        if( baseFeat == nullptr ) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Selection Error"),
                                 QObject::tr("No base View in Selection."));
            return;
        }
    } else {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Selection Error"),
                                 QObject::tr("Please select a View [and Edges]."));
            return;
    }

    std::vector<std::string> SubNames;

    std::vector<Gui::SelectionObject>::iterator itSel = selection.begin();
    for (; itSel != selection.end(); itSel++)  {
        if ((*itSel).getObject()->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
 //           baseFeat = static_cast<TechDraw::DrawViewPart*> ((*itSel).getObject());
            SubNames = (*itSel).getSubNames();
            if (SubNames.empty() || SubNames[0].empty()) {
                SubNames.clear();
            }
        }
    }

    std::vector<std::string> edgeNames;
    for (auto& s: SubNames) {
        std::string geomType = DrawUtil::getGeomTypeFromName(s);
        if (geomType == "Edge") {
            edgeNames.push_back(s);
        }
    }

    DrawDimHelper::makeExtentDim(baseFeat,
                                 edgeNames,
                                 0);
}

//===========================================================================
// TechDraw_VerticalExtentDimension
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawVerticalExtentDimension)

CmdTechDrawVerticalExtentDimension::CmdTechDrawVerticalExtentDimension()
  : Command("TechDraw_VerticalExtentDimension")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert Vertical Extent Dimension");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_VerticalExtentDimension";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_VerticalExtentDimension";
}

void CmdTechDrawVerticalExtentDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    if (dlg != nullptr) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task In Progress"),
            QObject::tr("Close active task dialog and try again."));
        return;
    }

    execVExtent(this);
}

bool CmdTechDrawVerticalExtentDimension::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, false);
    return (havePage && haveView);
}

void execVExtent(Gui::Command* cmd)
{
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(cmd);
    if (!page) {
        return;
    }

    std::vector<Gui::SelectionObject> selection = cmd->getSelection().getSelectionEx();
    TechDraw::DrawViewPart* baseFeat = nullptr;
    if (!selection.empty()) {
        baseFeat =  dynamic_cast<TechDraw::DrawViewPart *>(selection[0].getObject());
        if( baseFeat == nullptr ) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Selection Error"),
                                 QObject::tr("No base View in Selection."));
            return;
        }
    } else {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Selection Error"),
                                 QObject::tr("Please select a View [and Edges]."));
            return;
    }

    std::vector<std::string> SubNames;

    std::vector<Gui::SelectionObject>::iterator itSel = selection.begin();
    for (; itSel != selection.end(); itSel++)  {
        if ((*itSel).getObject()->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
            baseFeat = static_cast<TechDraw::DrawViewPart*> ((*itSel).getObject());
            SubNames = (*itSel).getSubNames();
        }
    }
    std::vector<std::string> edgeNames;
    for (auto& s: SubNames) {
        std::string geomType = DrawUtil::getGeomTypeFromName(s);
        if (geomType == "Edge") {
            edgeNames.push_back(s);
        }
    }

    DrawDimHelper::makeExtentDim(baseFeat,
                                 edgeNames,
                                 1);
}

//===========================================================================
// TechDraw_LandmarkDimension
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawLandmarkDimension)

CmdTechDrawLandmarkDimension::CmdTechDrawLandmarkDimension()
  : Command("TechDraw_LandmarkDimension")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert Landmark Dimension - EXPERIMENTAL");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_LandmarkDimension";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_LandmarkDimension";
}

void CmdTechDrawLandmarkDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    bool result = _checkSelection(this,3);
    if (!result)
        return;

    const std::vector<App::DocumentObject*> objects = getSelection().
                                        getObjectsOfType(Part::Feature::getClassTypeId());  //??
    if (objects.size() != 2) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select 2 point objects and 1 View. (1)"));
        return;
    }

    const std::vector<App::DocumentObject*> views = getSelection().
                                        getObjectsOfType(TechDraw::DrawViewPart::getClassTypeId());
    if (views.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select 2 point objects and 1 View. (2)"));
        return;
    }

    TechDraw::DrawViewPart* dvp = static_cast<TechDraw::DrawViewPart*>(views.front());

    std::vector<App::DocumentObject*> refs2d;

    std::vector<std::string> subs;
    subs.push_back("Vertex1");
    subs.push_back("Vertex1");
    TechDraw::DrawPage* page = dvp->findParentPage();
    std::string parentName = dvp->getNameInDocument();
    std::string PageName = page->getNameInDocument();

    TechDraw::LandmarkDimension *dim = nullptr;
    std::string FeatName = getUniqueObjectName("LandmarkDim");

    openCommand(QT_TRANSLATE_NOOP("Command", "Create Dimension"));
    doCommand(Doc,"App.activeDocument().addObject('TechDraw::LandmarkDimension','%s')",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.addView(App.activeDocument().%s)",PageName.c_str(),FeatName.c_str());
    if (objects.size() == 2) {
        //what about distanceX and distanceY??
        doCommand(Doc,"App.activeDocument().%s.Type = '%s'",FeatName.c_str(), "Distance");
        refs2d.push_back(dvp);
        refs2d.push_back(dvp);
    }
//    } else if (objects.size() == 3) {             //not implemented yet
//        doCommand(Doc,"App.activeDocument().%s.Type = '%s'",FeatName.c_str(), "Angle3Pt");
//        refs2d.push_back(dvp);
//        refs2d.push_back(dvp);
//        refs2d.push_back(dvp);
//        //subs.push_back("Vertex1");
//        //subs.push_back("Vertex1");
//        //subs.push_back("Vertex1");
//    }

    dim = dynamic_cast<TechDraw::LandmarkDimension *>(getDocument()->getObject(FeatName.c_str()));
    if (!dim) {
        throw Base::TypeError("CmdTechDrawLandmarkDimension - dim not found\n");
    }
    dim->References2D.setValues(refs2d, subs);
    dim->References3D.setValues(objects, subs);
    
    commitCommand();
    dim->recomputeFeature();

    //Horrible hack to force Tree update
    double x = dvp->X.getValue();
    dvp->X.setValue(x);
}

bool CmdTechDrawLandmarkDimension::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}


//------------------------------------------------------------------------------
void CreateTechDrawCommandsDims(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdTechDrawDimension());
    rcCmdMgr.addCommand(new CmdTechDrawRadiusDimension());
    rcCmdMgr.addCommand(new CmdTechDrawDiameterDimension());
    rcCmdMgr.addCommand(new CmdTechDrawLengthDimension());
    rcCmdMgr.addCommand(new CmdTechDrawHorizontalDimension());
    rcCmdMgr.addCommand(new CmdTechDrawVerticalDimension());
    rcCmdMgr.addCommand(new CmdTechDrawAngleDimension());
    rcCmdMgr.addCommand(new CmdTechDraw3PtAngleDimension());
    rcCmdMgr.addCommand(new CmdTechDrawExtentGroup());
    rcCmdMgr.addCommand(new CmdTechDrawVerticalExtentDimension());
    rcCmdMgr.addCommand(new CmdTechDrawHorizontalExtentDimension());
    rcCmdMgr.addCommand(new CmdTechDrawLinkDimension());
    rcCmdMgr.addCommand(new CmdTechDrawLandmarkDimension());
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
            TechDraw::BaseGeomPtr geom = objFeat->getGeomByIndex(GeoId);
            if (!geom) {
                Base::Console().Error("Logic Error: no geometry for GeoId: %d\n",GeoId);
                return isInvalid;
            }

            if(geom->geomType == TechDraw::GENERIC) {
                TechDraw::GenericPtr gen1 = std::static_pointer_cast<TechDraw::Generic>(geom);
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
                TechDraw::BSplinePtr  spline = static_pointer_cast<TechDraw::BSpline> (geom);
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
    std::vector<Gui::SelectionObject> selection = cmd->getSelection().getSelectionEx();

    auto objFeat0( dynamic_cast<TechDraw::DrawViewPart *>(selection[0].getObject()) );
    // getObject() can return null pointer, or dynamic_cast can fail
    if ( !objFeat0 ) {
        Base::Console().Error("Logic error in _isValidEdgeToEdge()\n");
        return isInvalid;
    }

    int edgeType = isInvalid;
    const std::vector<std::string> SubNames = selection[0].getSubNames();
    if(SubNames.size() == 2) {                                                   //there are 2
        if (TechDraw::DrawUtil::getGeomTypeFromName(SubNames[0]) == "Edge" &&    //they both start with "Edge"
            TechDraw::DrawUtil::getGeomTypeFromName(SubNames[1]) == "Edge") {
            int GeoId0( TechDraw::DrawUtil::getIndexFromName(SubNames[0]) );
            int GeoId1( TechDraw::DrawUtil::getIndexFromName(SubNames[1]) );
            TechDraw::BaseGeomPtr geom0 = objFeat0->getGeomByIndex(GeoId0);
            TechDraw::BaseGeomPtr geom1 = objFeat0->getGeomByIndex(GeoId1);

            if ((!geom0) || (!geom1)) {                                         // missing gometry
                Base::Console().Error("Logic Error: no geometry for GeoId: %d or GeoId: %d\n",GeoId0,GeoId1);
                return isInvalid;
            }

            if(geom0->geomType == TechDraw::GENERIC &&
               geom1->geomType == TechDraw::GENERIC) {
                TechDraw::GenericPtr gen0 = std::static_pointer_cast<TechDraw::Generic> (geom0);
                TechDraw::GenericPtr gen1 = std::static_pointer_cast<TechDraw::Generic> (geom1);
                if(gen0->points.size() > 2 ||
                   gen1->points.size() > 2) {                          //the edge is a polyline
                    return isInvalid;                                  //not supported yet
                }
                Base::Vector3d line0 = gen0->points.at(1) - gen0->points.at(0);
                Base::Vector3d line1 = gen1->points.at(1) - gen1->points.at(0);
                double xprod = fabs(line0.x * line1.y - line0.y * line1.x);
                if (xprod > FLT_EPSILON) {                              //edges are not parallel
                    return isAngle;                                 //angle or distance
                } else {
                    return isDiagonal;                              //distance || line
                }
            } else {
                return isDiagonal;                                  //two edges, not both straight lines
            }
        }  //edges
    } // 2 sub objects
    return edgeType;
}

//! verify that the Selection contains valid geometries for a Vertex to Edge Dimension
bool _isValidVertexToEdge(Gui::Command* cmd) {
    bool result = false;
    std::vector<Gui::SelectionObject> selection = cmd->getSelection().getSelectionEx();
    TechDraw::DrawViewPart* objFeat0 = static_cast<TechDraw::DrawViewPart *>(selection[0].getObject());
    const std::vector<std::string> SubNames = selection[0].getSubNames();
    if(SubNames.size() == 2) {                                         //there are 2
        int eId,vId;
        TechDraw::BaseGeomPtr e;
        TechDraw::VertexPtr v;
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
        e = objFeat0->getGeomByIndex(eId);
        v = objFeat0->getProjVertexByIndex(vId);
        if ((!e) || (!v)) {
            Base::Console().Error("Logic Error: no geometry for GeoId: %d or GeoId: %d\n",eId,vId);
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


