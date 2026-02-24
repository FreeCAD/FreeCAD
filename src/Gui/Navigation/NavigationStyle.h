// SPDX-License-Identifier: LGPL-2.1-or-later
/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <Inventor/C/basic.h>
#include <Inventor/SbBox2s.h>
#include <Inventor/SbPlane.h>
#include <Inventor/SbRotation.h>
#include <Inventor/SbSphere.h>
#include <Inventor/SbTime.h>
#include <Inventor/SbVec2f.h>
#include <Inventor/SbVec2s.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/events/SoEvents.h>

#include <QEvent>
#include <QAction>
#include <Base/BaseClass.h>
#include <Base/SmartPtrPy.h>
#include <Gui/Namespace.h>
#include <FCGlobal.h>
#include <memory>
#include <optional>

// forward declarations
class SoEvent;
class SoMouseWheelEvent;
class SoMotion3Event;
class SoQtViewer;
class SoCamera;
class SoSensor;
class SbSphereSheetProjector;

// NOLINTBEGIN(cppcoreguidelines-avoid*, readability-avoid-const-params-in-decls)
namespace Gui
{

class View3DInventorViewer;
class NavigationAnimator;
class AbstractMouseSelection;
class NavigationAnimation;

/**
 * @author Werner Mayer
 */
class GuiExport NavigationStyleEvent: public QEvent
{
public:
    explicit NavigationStyleEvent(const Base::Type& s);
    ~NavigationStyleEvent() override;
    const Base::Type& style() const;

private:
    Base::Type t;
};

/**
 * The navigation style base class
 * @author Werner Mayer
 */
class GuiExport NavigationStyle: public Base::BaseClass
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    enum ViewerMode
    {
        IDLE,
        INTERACT,
        ZOOMING,
        BOXZOOM,
        PANNING,
        DRAGGING,
        SPINNING,
        SEEK_WAIT_MODE,
        SEEK_MODE,
        SELECTION
    };

    enum SelectionMode
    {
        Lasso = 0,      /**< Select objects using a lasso. */
        Rectangle = 1,  /**< Select objects using a rectangle. */
        Rubberband = 2, /**< Select objects using a rubberband. */
        BoxZoom = 3,    /**< Perform a box zoom. */
        Clip = 4,       /**< Clip objects using a lasso. */
    };

    enum class ClarifySelectionMode
    {
        Default, /**< Long press with LMB to trigger clarify selection */
        Ctrl     /**< Long press with Ctrl+LMB to trigger clarify selection */
    };

    enum OrbitStyle
    {
        Turntable,
        Trackball,
        FreeTurntable,
        TrackballClassic,
        RoundedArcball
    };

    enum class RotationCenterMode
    {
        WindowCenter = 0,       /**< The center of the window */
        ScenePointAtCursor = 1, /**< Find the point in the scene at the cursor position. If there is
                                   no point then the focal plane is used */
        FocalPointAtCursor = 2, /**< Find the point on the focal plane at the cursor position. */
        BoundingBoxCenter = 4   /**< Find the center point of the bounding box of the scene. */
    };
    Q_DECLARE_FLAGS(RotationCenterModes, RotationCenterMode)

public:
    NavigationStyle();
    ~NavigationStyle() override;
    NavigationStyle(const NavigationStyle&) = delete;

    NavigationStyle& operator=(const NavigationStyle& ns);
    void setViewer(View3DInventorViewer*);

    void setAnimationEnabled(const SbBool enable);
    void setSpinningAnimationEnabled(const SbBool enable);
    SbBool isAnimationEnabled() const;
    SbBool isSpinningAnimationEnabled() const;
    SbBool isAnimating() const;
    SbBool isSpinning() const;
    void startAnimating(const std::shared_ptr<NavigationAnimation>& animation, bool wait = false) const;
    void stopAnimating() const;

    void setSensitivity(float);
    float getSensitivity() const;

    void setResetCursorPosition(SbBool);
    SbBool isResetCursorPosition() const;

    void setZoomInverted(SbBool);
    SbBool isZoomInverted() const;
    void setZoomStep(float);
    void setZoomAtCursor(SbBool);
    SbBool isZoomAtCursor() const;
    void zoomIn();
    void zoomOut();
    void setRotationCenterMode(RotationCenterModes);
    RotationCenterModes getRotationCenterMode() const;
    void setRotationCenter(const SbVec3f& cnt);
    SbVec3f getFocalPoint() const;

    SoCamera* getCamera() const;
    std::shared_ptr<NavigationAnimation> setCameraOrientation(
        const SbRotation& orientation,
        SbBool moveToCenter = false
    ) const;
    std::shared_ptr<NavigationAnimation> translateCamera(const SbVec3f& translation) const;

#if (COIN_MAJOR_VERSION * 100 + COIN_MINOR_VERSION * 10 + COIN_MICRO_VERSION < 403)
    void findBoundingSphere();
#endif

    void reorientCamera(SoCamera* camera, const SbRotation& rotation);
    void reorientCamera(SoCamera* camera, const SbRotation& rotation, const SbVec3f& rotationCenter);

    void boxZoom(const SbBox2s& box);
    // Scale the camera inplace
    void scale(float factor);
    virtual void viewAll();

    void setViewingMode(const ViewerMode newmode);
    int getViewingMode() const;
    virtual SbBool processEvent(const SoEvent* const ev);
    virtual SbBool processMotionEvent(const SoMotion3Event* const ev);
    virtual SbBool processKeyboardEvent(const SoKeyboardEvent* const event);
    virtual SbBool processClickEvent(const SoMouseButtonEvent* const event);
    virtual SbBool processWheelEvent(const SoMouseWheelEvent* const event);

    void setPopupMenuEnabled(const SbBool on);
    SbBool isPopupMenuEnabled() const;

    void startSelection(AbstractMouseSelection*);
    void startSelection(SelectionMode = Lasso);
    void abortSelection();
    void stopSelection();
    SbBool isSelecting() const;
    const std::vector<SbVec2s>& getPolygon(SelectionRole* role = nullptr) const;

    bool isDraggerUnderCursor(const SbVec2s pos) const;

    virtual ClarifySelectionMode clarifySelectionMode() const
    {
        return ClarifySelectionMode::Default;
    }

    void setOrbitStyle(OrbitStyle style);
    OrbitStyle getOrbitStyle() const;

    SbBool isViewing() const;
    void setViewing(SbBool);

    SbVec3f getRotationCenter(SbBool&) const;

    std::optional<SbVec2s>& getRightClickPosition();

    PyObject* getPyObject() override;

protected:
    void initialize();
    void finalize();

    void interactiveCountInc();
    void interactiveCountDec();
    int getInteractiveCount() const;

    SbBool isSeekMode() const;
    void setSeekMode(SbBool enable);
    SbBool seekToPoint(const SbVec2s screenpos);
    void seekToPoint(const SbVec3f& scenepos);
    void lookAtPoint(const SbVec2s screenpos);
    void lookAtPoint(const SbVec3f& position);

    void panCamera(
        SoCamera* camera,
        float vpaspect,
        const SbPlane& panplane,
        const SbVec2f& previous,
        const SbVec2f& current
    );
    void setupPanningPlane(const SoCamera* camera);
    int getDelta() const;
    void zoom(SoCamera* camera, float diffvalue);
    virtual void zoomByCursor(const SbVec2f& thispos, const SbVec2f& prevpos);
    void doZoom(SoCamera* camera, int wheeldelta, const SbVec2f& pos);
    void doZoom(SoCamera* camera, float logzoomfactor, const SbVec2f& pos);
    void doScale(SoCamera* camera, float factor);
    void doRotate(SoCamera* camera, float angle, const SbVec2f& pos);
    void spin(const SbVec2f& pointerpos);
    SbBool doSpin();
    void spin_simplified(SbVec2f curpos, SbVec2f prevpos);
    void moveCursorPosition();
    void saveCursorPosition(const SoEvent* const ev);

    SbVec2f normalizePixelPos(SbVec2s pixpos);
    SbVec2f normalizePixelPos(SbVec2f pixpos);

    SbBool handleEventInForeground(const SoEvent* const e);
    virtual SbBool processSoEvent(const SoEvent* const ev);
    void syncWithEvent(const SoEvent* const ev);
    virtual void openPopupMenu(const SbVec2s& position);

private:
    void spinInternal(const SbVec2f& pointerpos, const SbVec2f& lastpos);
    void spinSimplifiedInternal(const SbVec2f curpos, const SbVec2f prevpos);

protected:
    void clearLog();
    void addToLog(const SbVec2s pos, const SbTime time);

    void syncModifierKeys(const SoEvent* const ev);

protected:
    struct
    {  // tracking mouse movement in a log
        short size;
        short historysize;
        SbVec2s* position;
        SbTime* time;
    } log;

    View3DInventorViewer* viewer {nullptr};
    NavigationAnimator* animator;
    SbBool animationEnabled;
    ViewerMode currentmode;
    SoMouseButtonEvent mouseDownConsumedEvent;
    SbVec2f lastmouseposition;
    SbVec2s globalPos;
    SbVec2s localPos;
    SbPlane panningplane;
    SbTime centerTime;
    SbBool lockrecenter;
    SbBool menuenabled;
    SbBool ctrldown, shiftdown, altdown;
    SbBool button1down, button2down, button3down;
    SbBool invertZoom;
    SbBool zoomAtCursor;
    float zoomStep;
    SbBool hasDragged;
    SbBool hasPanned;
    SbBool hasZoomed;

    /** @name Mouse model */
    //@{
    AbstractMouseSelection* mouseSelection {nullptr};
    std::vector<SbVec2s> pcPolygon;
    SelectionRole selectedRole;
    //@}

    /** @name Spinning data */
    //@{
    SbBool spinningAnimationEnabled;
    int spinsamplecounter;
    SbRotation spinincrement;
    SbSphereSheetProjector* spinprojector;
    //@}

    Py::SmartPtr pythonObject;

    // store the position where right-click occurred just before
    // the menu popped up
    std::optional<SbVec2s> rightClickPosition;

private:
    friend class NavigationAnimator;

    SbVec3f rotationCenter;
    SbBool rotationCenterFound;
    SbBool rotationCenterIsScenePointAtCursor;
    NavigationStyle::RotationCenterModes rotationCenterMode;
    float sensitivity;
    SbBool resetcursorpos;

#if (COIN_MAJOR_VERSION * 100 + COIN_MINOR_VERSION * 10 + COIN_MICRO_VERSION < 403)
    SbSphere boundingSphere;
#endif
};

/** Sub-classes of this class appear in the preference dialog where users can
 * choose their favorite navigation style.
 * All other classes that inherit directly from NavigationStyle do not appear
 * in the above dialog.
 * This mechanism is useful to implement special navigation styles which are
 * only needed for certain purposes. Thus, it should not be possible to be
 * choosable by the user
 * @author Werner Mayer
 */
class GuiExport UserNavigationStyle: public NavigationStyle
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    UserNavigationStyle() = default;
    ~UserNavigationStyle() override = default;
    virtual const char* mouseButtons(ViewerMode) = 0;
    virtual std::string userFriendlyName() const;
    static std::map<Base::Type, std::string> getUserFriendlyNames();
};

class GuiExport InventorNavigationStyle: public UserNavigationStyle
{
    using inherited = UserNavigationStyle;

    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    InventorNavigationStyle();
    ~InventorNavigationStyle() override;
    const char* mouseButtons(ViewerMode) override;
    std::string userFriendlyName() const override;
    ClarifySelectionMode clarifySelectionMode() const override
    {
        return ClarifySelectionMode::Ctrl;
    }

protected:
    SbBool processSoEvent(const SoEvent* const ev) override;
};

class GuiExport CADNavigationStyle: public UserNavigationStyle
{
    using inherited = UserNavigationStyle;

    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    CADNavigationStyle();
    ~CADNavigationStyle() override;
    const char* mouseButtons(ViewerMode) override;

protected:
    SbBool processSoEvent(const SoEvent* const ev) override;

private:
    SbBool lockButton1 {false};
};

class GuiExport RevitNavigationStyle: public UserNavigationStyle
{
    using inherited = UserNavigationStyle;

    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    RevitNavigationStyle();
    ~RevitNavigationStyle() override;
    const char* mouseButtons(ViewerMode) override;

protected:
    SbBool processSoEvent(const SoEvent* const ev) override;

private:
    SbBool lockButton1 {false};
};

class GuiExport BlenderNavigationStyle: public UserNavigationStyle
{
    using inherited = UserNavigationStyle;

    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    BlenderNavigationStyle();
    ~BlenderNavigationStyle() override;
    const char* mouseButtons(ViewerMode) override;

protected:
    SbBool processSoEvent(const SoEvent* const ev) override;

private:
    SbBool lockButton1 {false};
};

class GuiExport SolidWorksNavigationStyle: public UserNavigationStyle
{
    using inherited = UserNavigationStyle;

    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    SolidWorksNavigationStyle();
    ~SolidWorksNavigationStyle() override;
    const char* mouseButtons(ViewerMode) override;

protected:
    SbBool processSoEvent(const SoEvent* const ev) override;

private:
    SbBool lockButton1 {false};
};

class GuiExport MayaGestureNavigationStyle: public UserNavigationStyle
{
    using inherited = UserNavigationStyle;

    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    MayaGestureNavigationStyle();
    ~MayaGestureNavigationStyle() override;
    const char* mouseButtons(ViewerMode) override;

protected:
    void zoomByCursor(const SbVec2f& thispos, const SbVec2f& prevpos) override;

    SbBool processSoEvent(const SoEvent* const ev) override;

    SbVec2s mousedownPos;  // the position where some mouse button was pressed (local pixel coordinates).
    short mouseMoveThreshold;  // setting. Minimum move required to consider it a move (in pixels).
    bool mouseMoveThresholdBroken;  // a flag that the move threshold was surpassed since last mousedown.
    int mousedownConsumedCount;  // a flag for remembering that a mousedown of button1/button2 was
                                 // consumed.
    SoMouseButtonEvent mousedownConsumedEvents[5];  // the event that was consumed and is to be
                                                    // refired. 2 should be enough, but just for a
                                                    // case of the maximum 5 buttons...
    bool testMoveThreshold(const SbVec2s currentPos) const;

    bool thisClickIsComplex;  // a flag that becomes set when a complex clicking pattern is detected
                              // (i.e., two or more mouse buttons were down at the same time).
    bool inGesture;           // a flag that is used to filter out mouse events during gestures.
};

class GuiExport TouchpadNavigationStyle: public UserNavigationStyle
{
    using inherited = UserNavigationStyle;

    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    TouchpadNavigationStyle();
    ~TouchpadNavigationStyle() override;
    const char* mouseButtons(ViewerMode) override;

protected:
    SbBool processSoEvent(const SoEvent* const ev) override;

private:
    SbBool blockPan {false};  // Used to block the first pan in a mouse movement to prevent big jumps
};

class GuiExport OpenCascadeNavigationStyle: public UserNavigationStyle
{
    using inherited = UserNavigationStyle;

    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    OpenCascadeNavigationStyle();
    ~OpenCascadeNavigationStyle() override;
    const char* mouseButtons(ViewerMode) override;

protected:
    SbBool processSoEvent(const SoEvent* const ev) override;
};

class GuiExport OpenSCADNavigationStyle: public UserNavigationStyle
{
    using inherited = UserNavigationStyle;

    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    OpenSCADNavigationStyle();
    ~OpenSCADNavigationStyle() override;
    const char* mouseButtons(ViewerMode) override;
    ClarifySelectionMode clarifySelectionMode() const override
    {
        return ClarifySelectionMode::Ctrl;
    }

protected:
    SbBool processSoEvent(const SoEvent* const ev) override;
};

class GuiExport TinkerCADNavigationStyle: public UserNavigationStyle
{
    using inherited = UserNavigationStyle;

    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    TinkerCADNavigationStyle();
    ~TinkerCADNavigationStyle() override;
    const char* mouseButtons(ViewerMode) override;

protected:
    SbBool processSoEvent(const SoEvent* const ev) override;
};

}  // namespace Gui
// NOLINTEND(cppcoreguidelines-avoid*, readability-avoid-const-params-in-decls)

Q_DECLARE_OPERATORS_FOR_FLAGS(Gui::NavigationStyle::RotationCenterModes)
