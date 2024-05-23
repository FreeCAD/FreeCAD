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

#ifndef DrawGuiUtil_h_
#define DrawGuiUtil_h_

#include <string>
#include <QCoreApplication>
#include <QGraphicsItem>

#include <Base/Vector3D.h>
#include <Mod/TechDraw/TechDrawGlobal.h>


class QComboBox;
class QPointF;
class QRectF;

namespace App {
class DocumentObject;
}

namespace Part {
class Feature;
}

namespace TechDraw {
class DrawPage;
class DrawView;
class LineGenerator;
}
namespace Gui {
class Command;
}

namespace TechDrawGui
{

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
    static void loadLineStandardsChoices(QComboBox* combo);
    static void loadLineStyleChoices(QComboBox* combo, TechDraw::LineGenerator* generator = nullptr);
    static QIcon iconForLine(size_t lineNumber, TechDraw::LineGenerator* generator);

    static double roundToDigits(double original, int digits);

    static bool isSelectedInTree(QGraphicsItem *item);
    static void setSelectedTree(QGraphicsItem *item, bool selected);

    static Base::Vector3d fromSceneCoords(const Base::Vector3d& sceneCoord, bool invert = true);
    static Base::Vector3d toSceneCoords(const Base::Vector3d& pageCoord, bool invert = true);
    static Base::Vector3d toGuiPoint(TechDraw::DrawView* obj, const Base::Vector3d& toConvert);
};

} //end namespace TechDrawGui
#endif
