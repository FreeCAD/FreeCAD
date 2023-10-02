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

#ifndef SIM_COIN3D_SOQTQUARTERADAPTOR_H
#define SIM_COIN3D_SOQTQUARTERADAPTOR_H

#include <Inventor/SoType.h>
#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/lists/SoCallbackList.h>
#include <Inventor/sensors/SoTimerSensor.h>

#include "QuarterWidget.h"


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
    explicit SoQTQuarterAdaptor(QWidget* parent = nullptr, const QtGLWidget* sharewidget = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    explicit SoQTQuarterAdaptor(const QtGLFormat& format, QWidget* parent = nullptr, const QtGLWidget* shareWidget = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    explicit SoQTQuarterAdaptor(QtGLContext* context, QWidget* parent = nullptr, const QtGLWidget* sharewidget = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    ~SoQTQuarterAdaptor() override;

    //the functions available in soqtviewer but missing in quarter
    QWidget* getWidget();
    QWidget* getGLWidget();
    QWidget* getWidget() const;
    QWidget* getGLWidget() const;

    virtual void setCameraType(SoType type);
    SoCamera * getCamera() const;

    const SbViewportRegion & getViewportRegion() const;

    virtual void setViewing(SbBool enable);
    SbBool isViewing() const;

    void interactiveCountInc();
    void interactiveCountDec();
    int  getInteractiveCount() const;

    void addStartCallback(SoQTQuarterAdaptorCB* func, void* data = nullptr);
    void addFinishCallback(SoQTQuarterAdaptorCB* func, void* data = nullptr);
    void removeStartCallback(SoQTQuarterAdaptorCB* func, void* data = nullptr);
    void removeFinishCallback(SoQTQuarterAdaptorCB* func, void* data = nullptr);

    virtual void setSeekMode(SbBool enable);
    SbBool isSeekMode() const;
    SbBool seekToPoint(const SbVec2s screenpos);
    void seekToPoint(const SbVec3f& scenepos);
    void setSeekTime(const float seconds);
    float getSeekTime() const;
    void setSeekDistance(const float distance);
    float getSeekDistance() const;
    void setSeekValueAsPercentage(const SbBool on);
    SbBool isSeekValuePercentage() const;

    virtual float getPickRadius() const {return this->pickRadius;}
    virtual void setPickRadius(float pickRadius);

    virtual void saveHomePosition();
    virtual void resetToHomePosition();
    virtual bool hasHomePosition() const {return m_storedcamera;}

    void setSceneGraph(SoNode* root) override {
        QuarterWidget::setSceneGraph(root);
    }
    
    bool processSoEvent(const SoEvent* event) override;
    void paintEvent(QPaintEvent*) override;

    //this functions still need to be ported
    virtual void afterRealizeHook() {} //enables spacenav and joystick in soqt, dunno if this is needed

private:
    void init();
    void convertPerspective2Ortho(const SoPerspectiveCamera* in,  SoOrthographicCamera* out);
    void convertOrtho2Perspective(const SoOrthographicCamera* in, SoPerspectiveCamera* out);
    void getCameraCoordinateSystem(SoCamera * camera, SoNode * root, SbMatrix & matrix, SbMatrix & inverse);
    static void seeksensorCB(void * data, SoSensor * s);
    void moveCameraScreen(const SbVec2f & screenpos);
    void resetFrameCounter();
    SbVec2f addFrametime(double ft);

    bool m_viewingflag;
    int  m_interactionnesting;
    SoCallbackList m_interactionStartCallback;
    SoCallbackList m_interactionEndCallback;

    double frametime;
    double drawtime;
    double starttime;
    int framecount;

    // Seek functionality
    SoTimerSensor* m_seeksensor;
    float m_seekperiod;
    SbBool m_inseekmode;
    SbVec3f m_camerastartposition, m_cameraendposition;
    SbRotation m_camerastartorient, m_cameraendorient;
    float m_seekdistance;
    SbBool m_seekdistanceabs;
    SoSearchAction searchaction;
    SoGetMatrixAction matrixaction;
    float pickRadius;
    // Home position storage.
    SoNode * m_storedcamera;
    
protected:
    void draw2DString(const char * str, SbVec2s glsize, SbVec2f position);
    void printString(const char * s);
    SbVec2f framesPerSecond;
};

} //Quarter
} //Coin3D
} //

#endif // SIM_COIN3D_SOQTQUARTERADAPTOR_H
