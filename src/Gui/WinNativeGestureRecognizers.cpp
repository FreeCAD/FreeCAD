/***************************************************************************
 *   Copyright (c) Victor Titov (DeepSOIC)                                 *
 *                                           (vv.titov@gmail.com) 2015     *
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
# include <QEvent>
# include <QGraphicsItem>
# include <QWidget>
# ifdef FC_OS_WIN32
# include <Windows.h>
# endif
# include <cassert>
#endif

#include "WinNativeGestureRecognizers.h"
#ifdef GESTURE_MESS
//this implementation is a bit incompatible with Qt5, since
//nativegesture members were transformed into properties, and
//the whole event was made public


#include <qgesture.h>

#include <Base/Exception.h>

QT_BEGIN_NAMESPACE

#if !defined(QT_NO_NATIVE_GESTURES)

//#include <private/qevent_p.h>
//this include is not available on conda Qt, see https://forum.freecadweb.org/viewtopic.php?f=4&t=21405&p=167395#p167395
//copy-pasted from this header:
class QNativeGestureEvent : public QEvent
{
public:
    enum Type {
        None,
        GestureBegin,
        GestureEnd,
        Pan,
        Zoom,
        Rotate,
        Swipe
    };

    QNativeGestureEvent()
        : QEvent(QEvent::NativeGesture), gestureType(None), percentage(0)
#ifdef Q_WS_WIN
        , sequenceId(0), argument(0)
#endif
    {
    }

    Type gestureType;
    float percentage;
    QPoint position;
    float angle;
#ifdef Q_WS_WIN
    ulong sequenceId;
    quint64 argument;
#endif
};


QGesture* WinNativeGestureRecognizerPinch::create(QObject* target)
{
    if (!target)
      return new QPinchGestureN; // a special case
    if (!target->isWidgetType())
      return 0;
    if (qobject_cast<QGraphicsObject *>(target))
      return 0;

    //QWidget* q = static_cast<QWidget *>(target);
    /*QWidgetPrivate *d = q->d_func();
    d->nativeGesturePanEnabled = true;
    d->winSetupGestures();*/ //fails to compile =(, but we can rely on this being done by grabGesture(Pan...

    return new QPinchGestureN;
}

QGestureRecognizer::Result WinNativeGestureRecognizerPinch::recognize(QGesture *gesture, QObject *watched, QEvent *event)
{
    QPinchGestureN* q = static_cast<QPinchGestureN*> (gesture);
    //QPinchGesturePrivate* d = q->d_func();//this fails to compile =( But we can get away without it.

    QGestureRecognizer::Result result = QGestureRecognizer::Ignore;
    if (event->type() == QEvent::NativeGesture) {
        bool bZoom = false;
        bool bRotate = false;
        QNativeGestureEvent *ev = static_cast<QNativeGestureEvent*>(event);
        switch(ev->gestureType) {
        case QNativeGestureEvent::GestureBegin:
            break;
        case QNativeGestureEvent::Zoom:
        case QNativeGestureEvent::Rotate:
            bZoom = ev->gestureType == QNativeGestureEvent::Zoom;
            bRotate = ev->gestureType == QNativeGestureEvent::Rotate;
            result = QGestureRecognizer::TriggerGesture;
            event->accept();
            break;
        case QNativeGestureEvent::GestureEnd:
            if (q->state() == Qt::NoGesture)
                return QGestureRecognizer::Ignore; // some other gesture has ended
            result = QGestureRecognizer::FinishGesture;
            break;
        default:
            return QGestureRecognizer::Ignore;
        }
        double ang = 0.0;
        if (bRotate)
            ang=GID_ROTATE_ANGLE_FROM_ARGUMENT(LOWORD(ev->argument));
        if (q->state() == Qt::NoGesture) {
            //start of a new gesture, prefill stuff
            //d->isNewSequence = true;
            q->setTotalChangeFlags(0); q->setChangeFlags(0);

            q->setLastCenterPoint(QPointF());
            q->setCenterPoint(
                  QPointF(
                    qreal(ev->position.x()),
                    qreal(ev->position.y())
                  )
            );
            q->setStartCenterPoint(q->centerPoint());
            q->setTotalRotationAngle(0.0); q->setLastRotationAngle(0.0);  q->setRotationAngle(0.0);
            q->setTotalScaleFactor(1.0); q->setLastScaleFactor(1.0); q->setScaleFactor(1.0);
            if(bZoom) {
                q->lastFingerDistance = ev->argument;
                q->fingerDistance = ev->argument;
            } else if (bRotate) {
                q->myLastRotationAngle = 0;
                q->myRotationAngle = 0;
            }
        } else {//in the middle of gesture

            //store new last values
            q->setLastCenterPoint(q->centerPoint());
            q->setLastRotationAngle(q->rotationAngle());
            q->setLastScaleFactor(q->scaleFactor());
            q->lastFingerDistance = q->fingerDistance;
            q->myLastRotationAngle = q->myRotationAngle;

            //update the current values
            if (bZoom)
                q->fingerDistance = ev->argument;
            if (bRotate)
                q->myRotationAngle = ang;
            if(ev->gestureType == QNativeGestureEvent::GestureEnd){
                q->fingerDistance = q->lastFingerDistance;//the end-of-gesture event holds no finger separation data, hence we are using the last value.
                q->myRotationAngle = q->myLastRotationAngle;
            }
            if (bZoom)
                q->setScaleFactor(
                        (qreal)(q->fingerDistance) / (qreal)(q->lastFingerDistance)
                );
            if (bRotate)
                q->setRotationAngle(qreal(unbranchAngle(q->myRotationAngle - q->myLastRotationAngle)));
            q->setCenterPoint(
                  QPointF(
                    qreal(ev->position.x()),
                    qreal(ev->position.y())
                  )
            );

            //compute the changes
            QPinchGesture::ChangeFlags cf = 0;
            if ( q->scaleFactor() != 1.0 )
                cf |= QPinchGesture::ScaleFactorChanged;
            if (q->lastCenterPoint() != q->centerPoint())
                cf |= QPinchGesture::CenterPointChanged;
            if (q->rotationAngle() != 0.0)
                cf |= QPinchGesture::RotationAngleChanged;
            q->setChangeFlags(cf);

            //increment totals
            q->setTotalChangeFlags (q->totalChangeFlags() | q->changeFlags());
            q->setTotalScaleFactor (q->totalScaleFactor() * q->scaleFactor());
            q->setTotalRotationAngle (q->totalRotationAngle() + q->rotationAngle());
        }
    }
    return result;
}


void WinNativeGestureRecognizerPinch::reset(QGesture* gesture)
{
  QGestureRecognizer::reset(gesture);//resets the state of the gesture, which is not write-accessible otherwise
  QPinchGestureN *q = static_cast<QPinchGestureN*>(gesture);
  q->lastFingerDistance = 0;
  q->setTotalChangeFlags(0); q->setChangeFlags(0);

  q->setLastCenterPoint(QPointF());
  q->setCenterPoint(
        QPointF(
          0.0,
          0.0
        )
  );
  q->setStartCenterPoint(q->centerPoint());
  q->setTotalRotationAngle(0.0); q->setLastRotationAngle(0.0);  q->setRotationAngle(0.0);
  q->setTotalScaleFactor(1.0); q->setLastScaleFactor(1.0); q->setScaleFactor(1.0);
  q->lastFingerDistance = 0;
  q->fingerDistance = 0;
}

//function prototype for dymanic linking
typedef BOOL ( __stdcall * ptrSetGestureConfig) (
     HWND ,              // window for which configuration is specified
     DWORD ,             // reserved, must be 0
     UINT ,              // count of GESTURECONFIG structures
     PGESTURECONFIG ,    // array of GESTURECONFIG structures, dwIDs will be processed in the
                         // order specified and repeated occurrences will overwrite previous ones
     UINT );             // sizeof(GESTURECONFIG)

void WinNativeGestureRecognizerPinch::TuneWindowsGestures(QWidget* target)
{
    //modify windows-specific gesture options
#if WINVER >= _WIN32_WINNT_WIN7
    //dynamic linking - required to be able to run on windows pre-7
    HINSTANCE hinstLib = LoadLibraryA("user32.dll");
    if (hinstLib == 0)
        throw Base::RuntimeError("LoadLibrary(user32.dll) failed. Could not tune Windows gestures.");

    ptrSetGestureConfig dllSetGestureConfig = reinterpret_cast<ptrSetGestureConfig> (GetProcAddress(hinstLib,"SetGestureConfig"));
    if (dllSetGestureConfig == 0)
        throw Base::RuntimeError("DLL entry point for SetGestureConfig not found in user32.dll. Could not tune Windows gestures.");

    HWND w = target->winId();

    //fill in the options
    const UINT nCfg = 2;
    GESTURECONFIG cfgs[nCfg];
    ZeroMemory(&cfgs, sizeof(cfgs));
    cfgs[0].dwID = GID_PAN;
    cfgs[0].dwWant = GC_PAN;
    cfgs[0].dwBlock = GC_PAN_WITH_GUTTER;//disables stickiness to pure vertical/pure horizontal pans
    cfgs[1].dwID = GID_ROTATE;
    cfgs[1].dwWant = GC_ROTATE;

    //set the options
    bool ret = dllSetGestureConfig(w, 0, nCfg, cfgs, sizeof(GESTURECONFIG));
    assert(ret);
    if(!ret){
        DWORD err = GetLastError();
        QString errMsg = QString::fromLatin1("Error in SetGestureConfig. GetLastError = %1").arg(err);
        throw Base::RuntimeError(errMsg.toLatin1());
    }
#endif
}

/*!
 * \brief WinNativeGestureRecognizerPinch::unbranchAngle utility function to bring an angle into -pi..pi region.
 * \param ang
 * \return
 */
double WinNativeGestureRecognizerPinch::unbranchAngle(double ang)
{
    const double Pi = 3.14159265358979323846;
    return ang - 2.0*Pi*floor((ang+Pi)/(2.0*Pi));
}

#endif //!defined(QT_NO_NATIVE_GESTURES)

#endif // GESTURE_MESS
