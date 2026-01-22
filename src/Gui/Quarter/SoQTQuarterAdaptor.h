/*
 * Extends the QuarterWidget with all functions the SoQtViewer has
 * Copyright (c) 2014 Stefan Tröger <stefantroeger@gmx.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#pragma once

#include <Inventor/SoType.h>
#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/lists/SoCallbackList.h>
#include <Inventor/sensors/SoTimerSensor.h>

#include <Base/Color.h>

#include "QuarterWidget.h"

class QOpenGLContext;
class QOpenGLWidget;
class QSurfaceFormat;

class SbViewportRegion;
class SoCamera;
class SoOrthographicCamera;
class SoPerspectiveCamera;

namespace SIM {
namespace Coin3D {
namespace Quarter {

class SoQTQuarterAdaptor;
using SoQTQuarterAdaptorCB = void (void* data, SoQTQuarterAdaptor* viewer);

class QUARTER_DLL_API SoQTQuarterAdaptor :  public QuarterWidget {

    Q_OBJECT

public:
    explicit SoQTQuarterAdaptor(QWidget* parent = nullptr,
                                const QOpenGLWidget* sharewidget = nullptr,
                                Qt::WindowFlags flags = Qt::WindowFlags());
    explicit SoQTQuarterAdaptor(const QSurfaceFormat& format,
                                QWidget* parent = nullptr,
                                const QOpenGLWidget* shareWidget = nullptr,
                                Qt::WindowFlags flags = Qt::WindowFlags());
    explicit SoQTQuarterAdaptor(QOpenGLContext* context,
                                QWidget* parent = nullptr,
                                const QOpenGLWidget* sharewidget = nullptr,
                                Qt::WindowFlags flags = Qt::WindowFlags());
    ~SoQTQuarterAdaptor() override;

    //the functions available in soqtviewer but missing in quarter
    QWidget* getWidget();
    QWidget* getGLWidget();
    QWidget* getWidget() const;
    QWidget* getGLWidget() const;

    virtual void setCameraType(SoType type);
    SoCamera * getCamera() const;

    const SbViewportRegion & getViewportRegion() const;

    virtual void setViewing(bool enable);
    bool isViewing() const;

    void interactiveCountInc();
    void interactiveCountDec();
    int  getInteractiveCount() const;

    void addStartCallback(SoQTQuarterAdaptorCB* func, void* data = nullptr);
    void addFinishCallback(SoQTQuarterAdaptorCB* func, void* data = nullptr);
    void removeStartCallback(SoQTQuarterAdaptorCB* func, void* data = nullptr);
    void removeFinishCallback(SoQTQuarterAdaptorCB* func, void* data = nullptr);

    virtual void setSeekMode(bool enable);
    bool isSeekMode() const;
    bool seekToPoint(const SbVec2s& screenpos);
    void seekToPoint(const SbVec3f& scenepos);
    void setSeekTime(float seconds);
    float getSeekTime() const;
    void setSeekDistance(float distance);
    float getSeekDistance() const;
    void setSeekValueAsPercentage(bool on);
    bool isSeekValuePercentage() const;

    virtual float getPickRadius() const {return this->pickRadius;}
    virtual void setPickRadius(float pickRadius);

    virtual void saveHomePosition();
    virtual void resetToHomePosition();
    virtual bool hasHomePosition() const
    {
        return m_storedcamera != nullptr;
    }

    void setSceneGraph(SoNode* root) override
    {
        QuarterWidget::setSceneGraph(root);
    }
    
    bool processSoEvent(const SoEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

    //this functions still need to be ported
    virtual void afterRealizeHook() {} //enables spacenav and joystick in soqt, dunno if this is needed

private:
    void init();
    static void convertPerspective2Ortho(const SoPerspectiveCamera* in,  SoOrthographicCamera* out);
    static void convertOrtho2Perspective(const SoOrthographicCamera* in, SoPerspectiveCamera* out);
    void getCameraCoordinateSystem(SoCamera * camera, SoNode * root, SbMatrix & matrix, SbMatrix & inverse);
    static void seeksensorCB(void * data, SoSensor * sensor);
    void moveCameraScreen(const SbVec2f & screenpos);
    void resetFrameCounter();
    SbVec2f addFrametime(double ft);

    bool m_viewingflag = false;
    int  m_interactionnesting = 0;
    SoCallbackList m_interactionStartCallback;
    SoCallbackList m_interactionEndCallback;

    double frametime = 0.0;
    double drawtime = 0.0;
    double starttime = 0.0;
    int framecount = 0.0;

    // Seek functionality
    SoTimerSensor* m_seeksensor = nullptr;
    float m_seekperiod = 0.0F;
    bool m_inseekmode = false;
    SbVec3f m_camerastartposition, m_cameraendposition;
    SbRotation m_camerastartorient, m_cameraendorient;
    float m_seekdistance = 0.0F;
    bool m_seekdistanceabs = false;
    SoSearchAction searchaction;
    SoGetMatrixAction matrixaction;
    float pickRadius = 0.0F;
    // Home position storage.
    SoNode * m_storedcamera = nullptr;

protected:
    static void draw2DString(const char * str, SbVec2s glsize, SbVec2f position, Base::Color color);
    static void printString(const char * str);
    SbVec2f framesPerSecond;  // NOLINT
};

} //Quarter
} //Coin3D
} //
