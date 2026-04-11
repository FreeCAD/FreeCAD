/***************************************************************************
 *   Copyright (c) 2016 WandererFan <wandererfan@gmail.com>                *
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

#pragma once

#include <string>
#include <QCoreApplication>
#include <QGraphicsItem>

#include <App/DocumentObject.h>
#include <Base/Vector3D.h>
#include <Gui/Selection/Selection.h>
#include <Mod/TechDraw/TechDrawGlobal.h>


class QComboBox;
class QPointF;
class QRectF;

namespace Part {
class Feature;
}

namespace TechDraw {
class DrawPage;
class DrawView;
class DrawViewPart;
class LineGenerator;
}
namespace Gui {
class Command;
}

namespace Base {
class Vector2d;
}

namespace TechDrawGui
{
class QGIEdge;
class QGIVertex;

/// Convenient utility functions for TechDraw Gui Module
class TechDrawGuiExport DrawGuiUtil {
    Q_DECLARE_TR_FUNCTIONS(TechDrawGui::DrawGuiUtil)
    public:
    static TechDraw::DrawPage* findPage(Gui::Command* cmd, bool findAny = false);

    static bool isDraftObject(App::DocumentObject* obj);
    static bool isArchObject(App::DocumentObject* obj);
    static bool isArchSection(App::DocumentObject* obj);

    static bool needPage(Gui::Command* cmd, bool findAny = false);
    static bool needView(Gui::Command* cmd, bool partOnly = true);
    static void dumpRectF(const char* text, const QRectF& r);
    static void dumpPointF(const char* text, const QPointF& p);
    static std::pair<Base::Vector3d, Base::Vector3d> get3DDirAndRot();
    static std::pair<Base::Vector3d, Base::Vector3d> getProjDirFromFace(App::DocumentObject* obj,
                                                                       std::string faceName);
    static void loadArrowBox(QComboBox* qcb);
    static void loadBalloonShapeBox(QComboBox* qballooncb);
    static void loadMattingStyleBox(QComboBox* qmattingcb);
    static void loadLineStandardsChoices(QComboBox* combo);
    static void loadLineStyleChoices(QComboBox* combo,
                                     TechDraw::LineGenerator* generator = nullptr);
    static void loadLineGroupChoices(QComboBox* combo);
    static QIcon iconForLine(size_t lineNumber, TechDraw::LineGenerator* generator);

    static double roundToDigits(double original, int digits);

    static bool isSelectedInTree(QGraphicsItem* item);
    static void setSelectedTree(QGraphicsItem* item, bool selected);
    static bool isStyleSheetDark(std::string curStyleSheet);
    static QIcon maskBlackPixels(QIcon itemIcon, QSize iconSize, QColor textColor);

    static Base::Vector3d fromSceneCoords(const Base::Vector3d& sceneCoord, bool invert = true);
    static Base::Vector3d toSceneCoords(const Base::Vector3d& pageCoord, bool invert = true);
    static Base::Vector3d toGuiPoint(TechDraw::DrawView* obj, const Base::Vector3d& toConvert);

    static bool findObjectInSelection(const std::vector<Gui::SelectionObject>& selection,
                                      const App::DocumentObject& targetObject);
    static std::vector<std::string>  getSubsForSelectedObject(const std::vector<Gui::SelectionObject>& selection,
                                                                App::DocumentObject* selectedObj);

    static void rotateToAlign(const QGIEdge* edge, const Base::Vector2d& direction);
    static void rotateToAlign(const QGIVertex* p1, const QGIVertex* p2, const Base::Vector2d& direction);
    static void rotateToAlign(TechDraw::DrawViewPart* view, const Base::Vector2d& oldDirection, const Base::Vector2d& newDirection);

    static void showNoPageMessage();

};

} //end namespace TechDrawGui