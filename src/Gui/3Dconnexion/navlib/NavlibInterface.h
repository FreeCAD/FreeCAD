/***************************************************************************
 *   Copyright (c) 2014-2023 3Dconnexion.                                  *
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

#ifdef USE_3DCONNEXION_NAVLIB

#include <QImage>
#include <array>

#include <SpaceMouse/CNavigation3D.hpp>

#include <Inventor/SbVec2f.h>
#include <Inventor/SbVec3f.h>

using CNav3D = TDx::SpaceMouse::Navigation3D::CNavigation3D;
using TDxCategory = TDx::SpaceMouse::CCategory;
using TDxCommand = TDx::SpaceMouse::CCommand;
using TDxCommandSet = TDx::SpaceMouse::CCommandSet;
using TDxImage = TDx::CImage;

constexpr uint32_t hitTestingResolution = 30;
constexpr std::string_view workbenchStr("Workbench");
constexpr std::string_view noneWorkbenchStr("NoneWorkbench");

class QGraphicsView;
class QAction;
class SoTransform;
class SoSwitch;
class SoResetTransform;
class SoImage;
class SoDepthBuffer;

namespace Gui
{
class MDIView;
class View3DInventor;
class View3DInventorViewer;
class Document;
class Command;
class ActionGroup;
}// namespace Gui

class NavlibInterface: public CNav3D
{

public:
    NavlibInterface();
    ~NavlibInterface();
    void enableNavigation();
    void disableNavigation();

private:
    long IsUserPivot(navlib::bool_t&) const override;
    long GetCameraMatrix(navlib::matrix_t&) const override;
    long GetPointerPosition(navlib::point_t&) const override;
    long GetViewExtents(navlib::box_t&) const override;
    long GetViewFOV(double&) const override;
    long GetViewFrustum(navlib::frustum_t&) const override;
    long GetIsViewPerspective(navlib::bool_t&) const override;
    long GetModelExtents(navlib::box_t&) const override;
    long GetSelectionExtents(navlib::box_t&) const override;
    long GetSelectionTransform(navlib::matrix_t&) const override;
    long GetIsSelectionEmpty(navlib::bool_t&) const override;
    long GetPivotPosition(navlib::point_t&) const override;
    long GetPivotVisible(navlib::bool_t&) const override;
    long GetHitLookAt(navlib::point_t&) const override;
    long GetFrontView(navlib::matrix_t&) const override;
    long GetCoordinateSystem(navlib::matrix_t&) const override;
    long GetIsViewRotatable(navlib::bool_t&) const override;
    long GetUnitsToMeters(double&) const override;

    long SetCameraMatrix(const navlib::matrix_t&) override;
    long SetViewExtents(const navlib::box_t&) override;
    long SetViewFOV(double) override;
    long SetViewFrustum(const navlib::frustum_t&) override;
    long SetSelectionTransform(const navlib::matrix_t&) override;
    long SetPivotPosition(const navlib::point_t&) override;
    long SetPivotVisible(bool) override;
    long SetHitAperture(double) override;
    long SetHitDirection(const navlib::vector_t&) override;
    long SetHitLookFrom(const navlib::point_t&) override;
    long SetHitSelectionOnly(bool) override;
    long SetActiveCommand(std::string) override;
    long SetTransaction(long) override;

    struct
    {
        const Gui::View3DInventor* pView3d = nullptr;
        QGraphicsView* pView2d = nullptr;
    } currentView;

    struct
    {
        SbVec3f origin;
        SbVec3f direction;
        float radius;
        bool selectionOnly;
    } ray;

    struct
    {
        SoTransform* pTransform = nullptr;
        SoSwitch* pVisibility = nullptr;
        SoResetTransform* pResetTransform = nullptr;
        SoImage* pImage = nullptr;
        SoDepthBuffer* pDepthTestAlways = nullptr;
        SoDepthBuffer* pDepthTestLess = nullptr;
        QImage pivotImage;
    } pivot;

    struct ParsedData
    {
        std::string groupName;
        std::string commandName;
        int32_t actionIndex;
    };

    ParsedData parseCommandId(const std::string& commandId) const;
    std::string getId(const Gui::Command& command, const int32_t parameter) const;
    TDxImage getImage(const QAction& qaction, const std::string& id) const;
    TDxCommand getCCommand(const Gui::Command& command,
                           const QAction& qAction,
                           const int32_t parameter) const;
	// This method removes markups from text (markup is a
	// string enclosed with '<' and '>' characters). Paragraph
	// ending markups "</p>" are being replaced with "\n\n".
    void removeMarkups(std::string& text) const;
    void initializePivot();
    void initializePattern() const;
    void connectActiveTab();
    template<class CameraType>
    CameraType getCamera() const;
    void onViewChanged(const Gui::MDIView*);
    bool is3DView() const;
    bool is2DView() const;
    void exportCommands(const std::string& workbench);
    void unpackCommands(Gui::Command& command,
                        TDxCategory& category,
                        std::vector<TDxImage>& images);

    std::error_code errorCode;
    std::pair<int, std::string> activeTab;
    mutable std::array<SbVec2f, hitTestingResolution> hitTestPattern;
    mutable bool patternInitialized;
    std::vector<std::string> exportedCommandSets;
    mutable bool wasPointerPick = false;
    double orthoNearDistance = 0.0;
};
#endif