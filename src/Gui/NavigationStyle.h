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


#ifndef GUI_NAVIGATIONSTYLE_H
#define GUI_NAVIGATIONSTYLE_H

#include <Inventor/C/basic.h>
#include <Inventor/SbBox2s.h>
#include <Inventor/SbVec2f.h>
#include <Inventor/SbVec2s.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/SbPlane.h>
#include <Inventor/SbRotation.h>
#include <Inventor/SbTime.h>
#include <Inventor/events/SoEvents.h>
#include <QCursor>
#include <QEvent>
#include <Base/BaseClass.h>
#include <Gui/Namespace.h>

// forward declarations
class SoEvent;
class SoMotion3Event;
class SoQtViewer;
class SoCamera;
class SoSensor;
class SbSphereSheetProjector;

namespace Gui {

class View3DInventorViewer;
class AbstractMouseSelection;

/**
 * @author Werner Mayer
 */
class GuiExport NavigationStyleEvent : public QEvent
{
public:
    NavigationStyleEvent(const Base::Type& s);
    ~NavigationStyleEvent();
    const Base::Type& style() const;
private:
    Base::Type t;
};

/**
 * The navigation style base class
 * @author Werner Mayer
 */
class GuiExport NavigationStyle : public Base::BaseClass
{
    TYPESYSTEM_HEADER();

public:
    enum ViewerMode {
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

    enum SelectionMode {
        Lasso       = 0,  /**< Select objects using a lasso. */
        Rectangle   = 1,  /**< Select objects using a rectangle. */
        Rubberband  = 2,  /**< Select objects using a rubberband. */
        BoxZoom     = 3,  /**< Perform a box zoom. */
        Clip        = 4,  /**< Clip objects using a lasso. */
    };

    enum OrbitStyle {
        Turntable,
        Trackball
    };

    enum RotationCenterMode {
        ScenePointAtCursor,     /**< Find the point in the scene at the cursor position. If there is no point then the focal plane is used */
        FocalPointAtCursor      /**< Find the point on the focal plane at the cursor position. */
    };

public:
    NavigationStyle();
    virtual ~NavigationStyle();

    NavigationStyle& operator = (const NavigationStyle& ns);
    void setViewer(View3DInventorViewer*);

    void setAnimationEnabled(const SbBool enable);
    SbBool isAnimationEnabled(void) const;

    void startAnimating(const SbVec3f& axis, float velocity);
    void stopAnimating(void);
    SbBool isAnimating(void) const;

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
    void setDragAtCursor(SbBool);
    SbBool isDragAtCursor() const;
    void setRotationCenterMode(RotationCenterMode);
    RotationCenterMode getRotationCenterMode() const;
    void setRotationCenter(const SbVec3f& cnt);
    SbVec3f getFocalPoint() const;

    void updateAnimation();
    void redraw();

    void setCameraOrientation(const SbRotation& rot, SbBool moveTocenter=false);
    void lookAtPoint(const SbVec3f&);
    void boxZoom(const SbBox2s& box);
    virtual void viewAll();

    void setViewingMode(const ViewerMode newmode);
    int getViewingMode() const;
    virtual SbBool processEvent(const SoEvent * const ev);
    virtual SbBool processMotionEvent(const SoMotion3Event * const ev);

    void setPopupMenuEnabled(const SbBool on);
    SbBool isPopupMenuEnabled(void) const;

    void startSelection(AbstractMouseSelection*);
    void startSelection(SelectionMode = Lasso);
    void stopSelection();
    SbBool isSelecting() const;
    const std::vector<SbVec2s>& getPolygon(SelectionRole* role=0) const;

    void setOrbitStyle(OrbitStyle style);
    OrbitStyle getOrbitStyle() const;

protected:
    void initialize();
    void finalize();

    void interactiveCountInc(void);
    void interactiveCountDec(void);
    int getInteractiveCount(void) const;

    SbBool isViewing(void) const;
    void setViewing(SbBool);
    SbBool isSeekMode(void) const;
    void setSeekMode(SbBool enable);
    SbBool seekToPoint(const SbVec2s screenpos);
    void seekToPoint(const SbVec3f& scenepos);
    SbBool lookAtPoint(const SbVec2s screenpos);
    SbVec3f getRotationCenter(SbBool*) const;

    void reorientCamera(SoCamera * camera, const SbRotation & rot);
    void panCamera(SoCamera * camera,
                   float vpaspect,
                   const SbPlane & panplane,
                   const SbVec2f & previous,
                   const SbVec2f & current);
    void pan(SoCamera* camera);
    void panToCenter(const SbPlane & pplane, const SbVec2f & currpos);
    void zoom(SoCamera * camera, float diffvalue);
    void zoomByCursor(const SbVec2f & thispos, const SbVec2f & prevpos);
    void doZoom(SoCamera * camera, SbBool forward, const SbVec2f& pos);
    void doZoom(SoCamera * camera, float logzoomfactor, const SbVec2f& pos);
    void doRotate(SoCamera * camera, float angle, const SbVec2f& pos);
    void spin(const SbVec2f & pointerpos);
    SbBool doSpin();
    void spin_simplified(SoCamera *cam, SbVec2f curpos, SbVec2f prevpos);
    void moveCursorPosition();
    void saveCursorPosition(const SoEvent * const ev);

    SbVec2f normalizePixelPos(SbVec2s pixpos);
    SbVec2f normalizePixelPos(SbVec2f pixpos);

    SbBool handleEventInForeground(const SoEvent* const e);
    virtual SbBool processSoEvent(const SoEvent * const ev);
    void syncWithEvent(const SoEvent * const ev);
    virtual void openPopupMenu(const SbVec2s& position);

    void clearLog(void);
    void addToLog(const SbVec2s pos, const SbTime time);


protected:
    struct { // tracking mouse movement in a log
        short size;
        short historysize;
        SbVec2s * position;
        SbTime * time;
    } log;

    View3DInventorViewer* viewer;
    ViewerMode currentmode;
    SbVec2f lastmouseposition;
    SbVec2s globalPos;
    SbVec2s localPos;
    SbPlane panningplane;
    SbTime prevRedrawTime;
    SbTime centerTime;
    SbBool lockrecenter;
    SbBool menuenabled;
    SbBool ctrldown, shiftdown, altdown;
    SbBool button1down, button2down, button3down;
    SbBool invertZoom;
    SbBool zoomAtCursor;
    float zoomStep;

    /** @name Mouse model */
    //@{
    AbstractMouseSelection* mouseSelection;
    std::vector<SbVec2s> pcPolygon;
    SelectionRole selectedRole;
    //@}

    /** @name Spinning data */
    //@{
    SbBool spinanimatingallowed;
    int spinsamplecounter;
    SbRotation spinincrement;
    SbRotation spinRotation;
    SbSphereSheetProjector * spinprojector;
    //@}

private:
    NavigationStyle(const NavigationStyle&);
    struct NavigationStyleP* pimpl;
    friend struct NavigationStyleP;
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
class GuiExport UserNavigationStyle : public NavigationStyle {
    TYPESYSTEM_HEADER();

public:
    UserNavigationStyle(){}
    ~UserNavigationStyle(){}
    virtual const char* mouseButtons(ViewerMode) = 0;
    virtual std::string userFriendlyName() const;
    static std::map<Base::Type, std::string> getUserFriendlyNames();
};

class GuiExport InventorNavigationStyle : public UserNavigationStyle {
    typedef UserNavigationStyle inherited;

    TYPESYSTEM_HEADER();

public:
    InventorNavigationStyle();
    ~InventorNavigationStyle();
    const char* mouseButtons(ViewerMode);
    virtual std::string userFriendlyName() const;

protected:
    SbBool processSoEvent(const SoEvent * const ev);

private:
    SoMouseButtonEvent mouseDownConsumedEvent;
};

class GuiExport CADNavigationStyle : public UserNavigationStyle {
    typedef UserNavigationStyle inherited;

    TYPESYSTEM_HEADER();

public:
    CADNavigationStyle();
    ~CADNavigationStyle();
    const char* mouseButtons(ViewerMode);

protected:
    SbBool processSoEvent(const SoEvent * const ev);

private:
    SbBool lockButton1;
    SoMouseButtonEvent mouseDownConsumedEvent;
};

class GuiExport RevitNavigationStyle : public UserNavigationStyle {
    typedef UserNavigationStyle inherited;

    TYPESYSTEM_HEADER();

public:
    RevitNavigationStyle();
    ~RevitNavigationStyle();
    const char* mouseButtons(ViewerMode);

protected:
    SbBool processSoEvent(const SoEvent * const ev);

private:
    SbBool lockButton1;
    SoMouseButtonEvent mouseDownConsumedEvent;
};

class GuiExport BlenderNavigationStyle : public UserNavigationStyle {
    typedef UserNavigationStyle inherited;

    TYPESYSTEM_HEADER();

public:
    BlenderNavigationStyle();
    ~BlenderNavigationStyle();
    const char* mouseButtons(ViewerMode);

protected:
    SbBool processSoEvent(const SoEvent * const ev);

private:
    SbBool lockButton1;
    SoMouseButtonEvent mouseDownConsumedEvent;
};

class GuiExport MayaGestureNavigationStyle : public UserNavigationStyle {
    typedef UserNavigationStyle inherited;

    TYPESYSTEM_HEADER();

public:
    MayaGestureNavigationStyle();
    ~MayaGestureNavigationStyle();
    const char* mouseButtons(ViewerMode);

protected:
    SbBool processSoEvent(const SoEvent * const ev);

    SbVec2s mousedownPos;//the position where some mouse button was pressed (local pixel coordinates).
    short mouseMoveThreshold;//setting. Minimum move required to consider it a move (in pixels).
    bool mouseMoveThresholdBroken;//a flag that the move threshold was surpassed since last mousedown.
    int mousedownConsumedCount;//a flag for remembering that a mousedown of button1/button2 was consumed.
    SoMouseButtonEvent mousedownConsumedEvent[5];//the event that was consumed and is to be refired. 2 should be enough, but just for a case of the maximum 5 buttons...
    bool testMoveThreshold(const SbVec2s currentPos) const;

    bool thisClickIsComplex;//a flag that becomes set when a complex clicking pattern is detected (i.e., two or more mouse buttons were down at the same time).
    bool inGesture; //a flag that is used to filter out mouse events during gestures.
};

class GuiExport TouchpadNavigationStyle : public UserNavigationStyle {
    typedef UserNavigationStyle inherited;

    TYPESYSTEM_HEADER();

public:
    TouchpadNavigationStyle();
    ~TouchpadNavigationStyle();
    const char* mouseButtons(ViewerMode);

protected:
    SbBool processSoEvent(const SoEvent * const ev);

private:
    SoMouseButtonEvent mouseDownConsumedEvent;
};

class GuiExport OpenCascadeNavigationStyle : public UserNavigationStyle {
    typedef UserNavigationStyle inherited;

    TYPESYSTEM_HEADER();

public:
    OpenCascadeNavigationStyle();
    ~OpenCascadeNavigationStyle();
    const char* mouseButtons(ViewerMode);

protected:
    SbBool processSoEvent(const SoEvent * const ev);

private:
    SoMouseButtonEvent mouseDownConsumedEvent;
};

} // namespace Gui

#endif // GUI_NAVIGATIONSTYLE_H 
