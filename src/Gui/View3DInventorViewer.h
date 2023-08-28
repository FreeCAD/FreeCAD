/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
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


#ifndef GUI_VIEW3DINVENTORVIEWER_H
#define GUI_VIEW3DINVENTORVIEWER_H

#include <list>
#include <map>
#include <memory>
#include <set>
#include <vector>

#include <QCursor>
#include <QImage>

#include <Inventor/SbRotation.h>
#include <Inventor/nodes/SoEventCallback.h>
#include <Inventor/nodes/SoSwitch.h>

#include <Base/Placement.h>

#include "Namespace.h"
#include "Selection.h"
#include "View3DInventorSelection.h"
#include "Quarter/SoQTQuarterAdaptor.h"


class SoTranslation;
class SoTransform;
class SoText2;

class SoSeparator;
class SoShapeHints;
class SoMaterial;
class SoRotationXYZ;
class SbSphereSheetProjector;
class SoEventCallback;
class SbBox2s;
class SoVectorizeAction;
class QImage;
class SoGroup;
class SoPickStyle;
class NaviCube;
class SoClipPlane;

namespace Quarter = SIM::Coin3D::Quarter;

namespace Gui {

class ViewProvider;
class SoFCBackgroundGradient;
class NavigationStyle;
class SoFCUnifiedSelection;
class Document;
class GLGraphicsItem;
class SoShapeScale;
class ViewerEventFilter;

/** GUI view into a 3D scene provided by View3DInventor
 *
 */
class GuiExport View3DInventorViewer : public Quarter::SoQTQuarterAdaptor, public SelectionObserver
{
    using inherited = Quarter::SoQTQuarterAdaptor;
    Q_OBJECT

public:
    /// Pick modes for picking points in the scene
    enum SelectionMode {
        Lasso       = 0,  /**< Select objects using a lasso. */
        Rectangle   = 1,  /**< Select objects using a rectangle. */
        Rubberband  = 2,  /**< Select objects using a rubberband. */
        BoxZoom     = 3,  /**< Perform a box zoom. */
        Clip        = 4,  /**< Clip objects using a lasso. */
    };
    /** @name Modus handling of the viewer
      * Here you can switch several features on/off
      * and modes of the Viewer
      */
    //@{
    enum ViewerMod {
        ShowCoord=1,       /**< Enables the Coordinate system in the corner. */
        ShowFPS  =2,       /**< Enables the Frames Per Second counter. */
        SimpleBackground=4,/**< switch to a simple background. */
        DisallowRotation=8,/**< switch off the rotation. */
        DisallowPanning=16,/**< switch off the panning. */
        DisallowZooming=32,/**< switch off the zooming. */
    };
    //@}

    /** @name Anti-Aliasing modes of the rendered 3D scene
      * Specifies Anti-Aliasing (AA) method
      * - Smoothing enables OpenGL line and vertex smoothing (basically deprecated)
      * - MSAA is hardware multi sampling (with 2, 4 or 8 passes), a quite common and efficient AA technique
      */
    //@{
    enum AntiAliasing {
        None,
        Smoothing,
        MSAA2x,
        MSAA4x,
        MSAA8x
    };
    //@}

    /** @name Render mode
      */
    //@{
    enum RenderType {
        Native,
        Framebuffer,
        Image
    };
    //@}

    /** @name Background
      */
    //@{
    enum Background {
        NoGradient,
        LinearGradient,
        RadialGradient
    };
    //@}

    explicit View3DInventorViewer (QWidget *parent, const QtGLWidget* sharewidget = nullptr);
    View3DInventorViewer (const QtGLFormat& format, QWidget *parent, const QtGLWidget* sharewidget = nullptr);
    ~View3DInventorViewer() override;

    void init();

    /// Observer message from the Selection
    void onSelectionChanged(const SelectionChanges &Reason) override;

    SoDirectionalLight* getBacklight() const;
    void setBacklight(SbBool on);
    SbBool isBacklight() const;
    void setSceneGraph (SoNode *root) override;
    SbBool searchNode(SoNode*) const;

    void setAnimationEnabled(const SbBool enable);
    SbBool isAnimationEnabled() const;

    void setPopupMenuEnabled(const SbBool on);
    SbBool isPopupMenuEnabled() const;

    void startAnimating(const SbVec3f& axis, float velocity);
    void stopAnimating();
    SbBool isAnimating() const;

    void setFeedbackVisibility(const SbBool enable);
    SbBool isFeedbackVisible() const;

    void setFeedbackSize(const int size);
    int getFeedbackSize() const;

    /// Get the preferred samples from the user settings
    static int getNumSamples();
    void setRenderType(const RenderType type);
    RenderType getRenderType() const;
    void renderToFramebuffer(QtGLFramebufferObject*);
    QImage grabFramebuffer();
    void imageFromFramebuffer(int width, int height, int samples,
                              const QColor& bgcolor, QImage& img);

    void setViewing(SbBool enable) override;
    virtual void setCursorEnabled(SbBool enable);

    void addGraphicsItem(GLGraphicsItem*);
    void removeGraphicsItem(GLGraphicsItem*);
    std::list<GLGraphicsItem*> getGraphicsItems() const;
    std::list<GLGraphicsItem*> getGraphicsItemsOfType(const Base::Type&) const;
    void clearGraphicsItems();

    /** @name Handling of view providers */
    //@{
    /// Checks if the view provider is a top-level object of the scene
    SbBool hasViewProvider(ViewProvider*) const;
    /// Checks if the view provider is part of the scene.
    /// In contrast to hasViewProvider() this method also checks if the view
    /// provider is a child of another view provider
    SbBool containsViewProvider(const ViewProvider*) const;
    /// adds an ViewProvider to the view, e.g. from a feature
    void addViewProvider(ViewProvider*);
    /// remove a ViewProvider
    void removeViewProvider(ViewProvider*);
    /// get view provider by path
    ViewProvider* getViewProviderByPath(SoPath*) const;
    ViewProvider* getViewProviderByPathFromTail(SoPath*) const;
    /// get all view providers of given type
    std::vector<ViewProvider*> getViewProvidersOfType(const Base::Type& typeId) const;
    /// set the ViewProvider in special edit mode
    void setEditingViewProvider(Gui::ViewProvider* p, int ModNum);
    /// return whether a view provider is edited
    SbBool isEditingViewProvider() const;
    /// reset from edit mode
    void resetEditingViewProvider();
    void setupEditingRoot(SoNode *node=nullptr, const Base::Matrix4D *mat=nullptr);
    void resetEditingRoot(bool updateLinks=true);
    void setEditingTransform(const Base::Matrix4D &mat);
    /** Helper method to get picked entities while editing.
     * It's in the responsibility of the caller to delete the returned instance.
     */
    SoPickedPoint* getPointOnRay(const SbVec2s& pos, const ViewProvider* vp) const;
    /** Helper method to get picked entities while editing.
     * It's in the responsibility of the caller to delete the returned instance.
     */
    SoPickedPoint* getPointOnRay(const SbVec3f& pos, const SbVec3f& dir, const ViewProvider* vp) const;
    /// display override mode
    void setOverrideMode(const std::string &mode);
    void updateOverrideMode(const std::string &mode);
    std::string getOverrideMode() const {return overrideMode;}
    //@}

    /** @name Making pictures */
    //@{
    /**
     * Creates an image with width \a w and height \a h of the current scene graph
     * using a multi-sampling of \a s and exports the rendered scenegraph to an image.
     */
    void savePicture(int w, int h, int s, const QColor&, QImage&) const;
    void saveGraphic(int pagesize, const QColor&, SoVectorizeAction* va) const;
    //@}
    /**
     * Writes the current scenegraph to an Inventor file, either in ascii or binary.
     */
    bool dumpToFile(SoNode* node, const char* filename, bool binary) const;

    /** @name Selection methods */
    //@{
    void startSelection(SelectionMode = Lasso);
    void abortSelection();
    void stopSelection();
    bool isSelecting() const;
    std::vector<SbVec2f> getGLPolygon(SelectionRole* role=nullptr) const;
    std::vector<SbVec2f> getGLPolygon(const std::vector<SbVec2s>&) const;
    const std::vector<SbVec2s>& getPolygon(SelectionRole* role=nullptr) const;
    void setSelectionEnabled(const SbBool enable);
    SbBool isSelectionEnabled() const;
    //@}

    /// Returns the screen coordinates of the origin of the path's tail object
    /*! Return value is in floating-point pixels, origin at bottom-left. */
    SbVec2f screenCoordsOfPath(SoPath *path) const;

    /** @name Edit methods */
    //@{
    void setEditing(SbBool edit);
    SbBool isEditing() const { return this->editing; }
    void setEditingCursor (const QCursor& cursor);
    void setComponentCursor(const QCursor& cursor);
    void setRedirectToSceneGraph(SbBool redirect) { this->redirected = redirect; }
    SbBool isRedirectedToSceneGraph() const { return this->redirected; }
    void setRedirectToSceneGraphEnabled(SbBool enable) { this->allowredir = enable; }
    SbBool isRedirectToSceneGraphEnabled() const { return this->allowredir; }
    //@}

    /** @name Pick actions */
    //@{
    // calls a PickAction on the scene graph
    bool pickPoint(const SbVec2s& pos,SbVec3f &point,SbVec3f &norm) const;
    SoPickedPoint* pickPoint(const SbVec2s& pos) const;
    const SoPickedPoint* getPickedPoint(SoEventCallback * n) const;
    SbBool pubSeekToPoint(const SbVec2s& pos);
    void pubSeekToPoint(const SbVec3f& pos);
    //@}

    /**
     * Set up a callback function \a cb which will be invoked for the given eventtype.
     * \a userdata will be given as the first argument to the callback function.
     */
    void addEventCallback(SoType eventtype, SoEventCallbackCB * cb, void* userdata = nullptr);
    /**
     * Unregister the given callback function \a cb.
     */
    void removeEventCallback(SoType eventtype, SoEventCallbackCB * cb, void* userdata = nullptr);

    /** @name Clipping plane, near and far plane */
    //@{
    /** Returns the view direction from the user's eye point in direction to the
     * viewport which is actually the negative normal of the near plane.
     * The vector is normalized to length of 1.
     */
    SbVec3f getViewDirection() const;
    void    setViewDirection(SbVec3f);
    /** Returns the up direction */
    SbVec3f getUpDirection() const;

    /** Returns the orientation of the camera. */
    SbRotation getCameraOrientation() const;

    /** Returns the 3d point on the focal plane to the given 2d point. */
    SbVec3f getPointOnFocalPlane(const SbVec2s&) const;

    /** Returns the 2d coordinates on the viewport to the given 3d point. */
    SbVec2s getPointOnViewport(const SbVec3f&) const;

    /** Converts Inventor coordinates into Qt coordinates.
     * The conversion takes the device pixel ratio into account.
     */
    QPoint toQPoint(const SbVec2s&) const;

    /** Converts Qt coordinates into Inventor coordinates.
     * The conversion takes the device pixel ratio into account.
     */
    SbVec2s fromQPoint(const QPoint&) const;

    /** Returns the near plane represented by its normal and base point. */
    void getNearPlane(SbVec3f& rcPt, SbVec3f& rcNormal) const;

    /** Returns the far plane represented by its normal and base point. */
    void getFarPlane(SbVec3f& rcPt, SbVec3f& rcNormal) const;

    /** Adds or remove a manipulator to/from the scenegraph. */
    void toggleClippingPlane(int toggle=-1, bool beforeEditing=false,
            bool noManip=false, const Base::Placement &pla = Base::Placement());

    /** Checks whether a clipping plane is set or not. */
    bool hasClippingPlane() const;

    /** Project the given normalized 2d point onto the near plane */
    SbVec3f projectOnNearPlane(const SbVec2f&) const;

    /** Project the given normalized 2d point onto the far plane */
    SbVec3f projectOnFarPlane(const SbVec2f&) const;

    /** Project the given 2d point to a line */
    void projectPointToLine(const SbVec2s&, SbVec3f& pt1, SbVec3f& pt2) const;

    /** Get the normalized position of the 2d point. */
    SbVec2f getNormalizedPosition(const SbVec2s&) const;
    //@}

    /** @name Dimension controls
     * the "turn*" functions are wired up to parameter groups through view3dinventor.
     * don't call them directly. instead set the parameter groups.
     * @see TaskDimension
     */
    //@{
    void turnAllDimensionsOn();
    void turnAllDimensionsOff();
    void turn3dDimensionsOn();
    void turn3dDimensionsOff();
    void turnDeltaDimensionsOn();
    void turnDeltaDimensionsOff();
    void eraseAllDimensions();
    void addDimension3d(SoNode *node);
    void addDimensionDelta(SoNode *node);
    //@}

    /**
     * Set the camera's orientation. If isAnimationEnabled() returns
     * \a true the reorientation is animated, otherwise its directly
     * set.
     */
    void setCameraOrientation(const SbRotation& rot, SbBool moveTocenter=false);
    void setCameraType(SoType t) override;
    void moveCameraTo(const SbRotation& rot, const SbVec3f& pos, int steps, int ms);
    /**
     * Zooms the viewport to the size of the bounding box.
     */
    void boxZoom(const SbBox2s&);
    /**
     * Reposition the current camera so we can see the complete scene.
     */
    void viewAll() override;
    void viewAll(float factor);

    /// Breaks out a VR window for a Rift
    void viewVR();

    /**
     * Returns the bounding box of the scene graph.
     */
    SbBox3f getBoundingBox() const;

    /**
     * Reposition the current camera so we can see all selected objects
     * of the scene. Therefore we search for all SOFCSelection nodes, if
     * none of them is selected nothing happens.
     */
    void viewSelection();

    void setGradientBackground(Background);
    Background getGradientBackground() const;
    void setGradientBackgroundColor(const SbColor& fromColor,
                                    const SbColor& toColor);
    void setGradientBackgroundColor(const SbColor& fromColor,
                                    const SbColor& toColor,
                                    const SbColor& midColor);
    void setNavigationType(Base::Type);

    void setAxisCross(bool b);
    bool hasAxisCross();

    void showRotationCenter(bool show);

    void setEnabledFPSCounter(bool b);
    void setEnabledNaviCube(bool b);
    bool isEnabledNaviCube() const;
    void setNaviCubeCorner(int);
    NaviCube* getNaviCube() const;
    void setEnabledVBO(bool b);
    bool isEnabledVBO() const;
    void setRenderCache(int);

    void getDimensions(float& fHeight, float& fWidth) const;
    float getMaxDimension() const;
    SbVec3f getCenterPointOnFocalPlane() const;

    NavigationStyle* navigationStyle() const;

    void setDocument(Gui::Document *pcDocument);
    Gui::Document* getDocument();

    virtual PyObject *getPyObject();

protected:
    GLenum getInternalTextureFormat() const;
    void renderScene();
    void renderFramebuffer();
    void renderGLImage();
    void animatedViewAll(int steps, int ms);
    void actualRedraw() override;
    void setSeekMode(SbBool enable) override;
    void afterRealizeHook() override;
    bool processSoEvent(const SoEvent * ev) override;
    void dropEvent (QDropEvent * e) override;
    void dragEnterEvent (QDragEnterEvent * e) override;
    void dragMoveEvent(QDragMoveEvent *e) override;
    void dragLeaveEvent(QDragLeaveEvent *e) override;
    SbBool processSoEventBase(const SoEvent * const ev);
    void printDimension();
    void selectAll();

private:
    static void setViewportCB(void * userdata, SoAction * action);
    static void clearBufferCB(void * userdata, SoAction * action);
    static void setGLWidgetCB(void * userdata, SoAction * action);
    static void handleEventCB(void * userdata, SoEventCallback * n);
    static void interactionStartCB(void * data, Quarter::SoQTQuarterAdaptor * viewer);
    static void interactionFinishCB(void * data, Quarter::SoQTQuarterAdaptor * viewer);
    static void interactionLoggerCB(void * ud, SoAction* action);

private:
    static void selectCB(void * closure, SoPath * p);
    static void deselectCB(void * closure, SoPath * p);
    static SoPath * pickFilterCB(void * data, const SoPickedPoint * pick);
    void initialize();
    void drawAxisCross();
    static void drawArrow();
    void drawSingleBackground(const QColor&);
    void setCursorRepresentation(int mode);
    void aboutToDestroyGLContext() override;
    void createStandardCursors(double);

private:
    NaviCube* naviCube;
    std::set<ViewProvider*> _ViewProviderSet;
    std::map<SoSeparator*,ViewProvider*> _ViewProviderMap;
    std::list<GLGraphicsItem*> graphicsItems;
    ViewProvider* editViewProvider;
    SoFCBackgroundGradient *pcBackGround;
    SoSeparator * backgroundroot;
    SoSeparator * foregroundroot;
    SoDirectionalLight* backlight;

    // Scene graph root
    SoSeparator * pcViewProviderRoot;
    // Child group in the scene graph that contains view providers related to the physical object
    SoGroup* objectGroup;

    std::unique_ptr<View3DInventorSelection> inventorSelection;

    SoSeparator * pcEditingRoot;
    SoTransform * pcEditingTransform;
    bool restoreEditingRoot;
    SoEventCallback* pEventCallback;
    NavigationStyle* navigation;
    SoFCUnifiedSelection* selectionRoot;

    SoClipPlane *pcClipPlane;

    RenderType renderType;
    QtGLFramebufferObject* framebuffer;
    QImage glImage;
    SbBool shading;
    SoSwitch *dimensionRoot;

    // small axis cross in the corner
    SbBool axiscrossEnabled;
    int axiscrossSize;
    // big one in the middle
    SoShapeScale* axisCross;
    SoGroup* axisGroup;

    SoGroup* rotationCenterGroup;

    //stuff needed to draw the fps counter
    bool fpsEnabled;
    bool vboEnabled;
    SbBool naviCubeEnabled;

    SbBool editing;
    QCursor editCursor, zoomCursor, panCursor, spinCursor;
    SbBool redirected;
    SbBool allowredir;

    std::string overrideMode;
    Gui::Document* guiDocument = nullptr;

    ViewerEventFilter* viewerEventFilter;

    PyObject *_viewerPy;

    // friends
    friend class NavigationStyle;
    friend class GLPainter;
    friend class ViewerEventFilter;
};

} // namespace Gui

#endif  // GUI_VIEW3DINVENTORVIEWER_H
