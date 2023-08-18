/***************************************************************************
 *   Copyright (c) 2023 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <Inventor/fields/SoSFColor.h>
# include <Inventor/nodes/SoDirectionalLight.h>
# include <Inventor/nodes/SoOrthographicCamera.h>
# include <Inventor/nodes/SoPerspectiveCamera.h>
#endif

#include <Base/Builder3D.h>
#include <App/Color.h>

#include "NaviCube.h"
#include "NavigationStyle.h"
#include "SoFCSelectionAction.h"
#include "View3DSettings.h"
#include "View3DInventorViewer.h"

using namespace Gui;

View3DSettings::View3DSettings(ParameterGrp::handle hGrp,
                               View3DInventorViewer* view)
    : hGrp(hGrp)
    , _viewers{view}
{
    hGrp->Attach(this);
}

View3DSettings::View3DSettings(ParameterGrp::handle hGrp,
                               const std::vector<View3DInventorViewer *>& view)
    : hGrp(hGrp)
    , _viewers(view)
{
    hGrp->Attach(this);
}

View3DSettings::~View3DSettings()
{
    hGrp->Detach(this);
}

int View3DSettings::stopAnimatingIfDeactivated() const
{
    long defaultTimeout = 3000;
    return hGrp->GetInt("stopAnimatingIfDeactivated", defaultTimeout);
}

void View3DSettings::applySettings()
{
    // apply the user settings
    OnChange(*hGrp,"EyeDistance");
    OnChange(*hGrp,"CornerCoordSystem");
    OnChange(*hGrp,"CornerCoordSystemSize");
    OnChange(*hGrp,"ShowAxisCross");
    OnChange(*hGrp,"UseAutoRotation");
    OnChange(*hGrp,"Gradient");
    OnChange(*hGrp,"RadialGradient");
    OnChange(*hGrp,"BackgroundColor");
    OnChange(*hGrp,"BackgroundColor2");
    OnChange(*hGrp,"BackgroundColor3");
    OnChange(*hGrp,"BackgroundColor4");
    OnChange(*hGrp,"UseBackgroundColorMid");
    OnChange(*hGrp,"ShowFPS");
    OnChange(*hGrp,"ShowNaviCube");
    OnChange(*hGrp,"UseVBO");
    OnChange(*hGrp,"RenderCache");
    OnChange(*hGrp,"Orthographic");
    OnChange(*hGrp,"HeadlightColor");
    OnChange(*hGrp,"HeadlightDirection");
    OnChange(*hGrp,"HeadlightIntensity");
    OnChange(*hGrp,"EnableBacklight");
    OnChange(*hGrp,"BacklightColor");
    OnChange(*hGrp,"BacklightDirection");
    OnChange(*hGrp,"BacklightIntensity");
    OnChange(*hGrp,"NavigationStyle");
    OnChange(*hGrp,"OrbitStyle");
    OnChange(*hGrp,"Sensitivity");
    OnChange(*hGrp,"ResetCursorPosition");
    OnChange(*hGrp,"DimensionsVisible");
    OnChange(*hGrp,"Dimensions3dVisible");
    OnChange(*hGrp,"DimensionsDeltaVisible");
    OnChange(*hGrp,"PickRadius");
    OnChange(*hGrp,"TransparentObjectRenderType");
}

void View3DSettings::OnChange(ParameterGrp::SubjectType &rCaller,ParameterGrp::MessageType Reason)
{
    const ParameterGrp& rGrp = static_cast<ParameterGrp&>(rCaller);
    if (strcmp(Reason,"HeadlightColor") == 0) {
        unsigned long headlight = rGrp.GetUnsigned("HeadlightColor",ULONG_MAX); // default color (white)
        float transparency;
        SbColor headlightColor;
        headlightColor.setPackedValue((uint32_t)headlight, transparency);
        for (auto _viewer : _viewers) {
            _viewer->getHeadlight()->color.setValue(headlightColor);
        }
    }
    else if (strcmp(Reason,"HeadlightDirection") == 0) {
        try {
            std::string pos = rGrp.GetASCII("HeadlightDirection");
            Base::Vector3f dir = Base::to_vector(pos);
            for (auto _viewer : _viewers) {
                _viewer->getHeadlight()->direction.setValue(dir.x, dir.y, dir.z);
            }
        }
        catch (const std::exception&) {
            // ignore exception
        }
    }
    else if (strcmp(Reason,"HeadlightIntensity") == 0) {
        long value = rGrp.GetInt("HeadlightIntensity", 100);
        for (auto _viewer : _viewers) {
            _viewer->getHeadlight()->intensity.setValue((float)value/100.0f);
        }
    }
    else if (strcmp(Reason,"EnableBacklight") == 0) {
        for (auto _viewer : _viewers) {
            _viewer->setBacklight(rGrp.GetBool("EnableBacklight", false));
        }
    }
    else if (strcmp(Reason,"BacklightColor") == 0) {
        unsigned long backlight = rGrp.GetUnsigned("BacklightColor",ULONG_MAX); // default color (white)
        float transparency;
        SbColor backlightColor;
        backlightColor.setPackedValue((uint32_t)backlight, transparency);
        for (auto _viewer : _viewers) {
            _viewer->getBacklight()->color.setValue(backlightColor);
        }
    }
    else if (strcmp(Reason,"BacklightDirection") == 0) {
        try {
            std::string pos = rGrp.GetASCII("BacklightDirection");
            Base::Vector3f dir = Base::to_vector(pos);
            for (auto _viewer : _viewers) {
                _viewer->getBacklight()->direction.setValue(dir.x, dir.y, dir.z);
            }
        }
        catch (const std::exception&) {
            // ignore exception
        }
    }
    else if (strcmp(Reason,"BacklightIntensity") == 0) {
        long value = rGrp.GetInt("BacklightIntensity", 100);
        for (auto _viewer : _viewers) {
            _viewer->getBacklight()->intensity.setValue((float)value/100.0f);
        }
    }
    else if (strcmp(Reason,"EnablePreselection") == 0) {
        const ParameterGrp& rclGrp = ((ParameterGrp&)rCaller);
        SoFCEnableHighlightAction cAct(rclGrp.GetBool("EnablePreselection", true));
        for (auto _viewer : _viewers) {
            cAct.apply(_viewer->getSceneGraph());
        }
    }
    else if (strcmp(Reason,"EnableSelection") == 0) {
        const ParameterGrp& rclGrp = ((ParameterGrp&)rCaller);
        SoFCEnableSelectionAction cAct(rclGrp.GetBool("EnableSelection", true));
        for (auto _viewer : _viewers) {
            cAct.apply(_viewer->getSceneGraph());
        }
    }
    else if (strcmp(Reason,"HighlightColor") == 0) {
        float transparency;
        SbColor highlightColor(0.8f, 0.1f, 0.1f);
        auto highlight = (unsigned long)(highlightColor.getPackedValue());
        highlight = rGrp.GetUnsigned("HighlightColor", highlight);
        highlightColor.setPackedValue((uint32_t)highlight, transparency);
        SoSFColor col; col.setValue(highlightColor);
        SoFCHighlightColorAction cAct(col);
        for (auto _viewer : _viewers) {
            cAct.apply(_viewer->getSceneGraph());
        }
    }
    else if (strcmp(Reason,"SelectionColor") == 0) {
        float transparency;
        SbColor selectionColor(0.1f, 0.8f, 0.1f);
        auto selection = (unsigned long)(selectionColor.getPackedValue());
        selection = rGrp.GetUnsigned("SelectionColor", selection);
        selectionColor.setPackedValue((uint32_t)selection, transparency);
        SoSFColor col; col.setValue(selectionColor);
        SoFCSelectionColorAction cAct(col);
        for (auto _viewer : _viewers) {
            cAct.apply(_viewer->getSceneGraph());
        }
    }
    else if (strcmp(Reason,"NavigationStyle") == 0) {
        if (!ignoreNavigationStyle) {
            // check whether the simple or the full mouse model is used
            std::string model = rGrp.GetASCII("NavigationStyle",CADNavigationStyle::getClassTypeId().getName());
            Base::Type type = Base::Type::fromName(model.c_str());
            for (auto _viewer : _viewers) {
                _viewer->setNavigationType(type);
            }
        }
    }
    else if (strcmp(Reason,"OrbitStyle") == 0) {
        int style = rGrp.GetInt("OrbitStyle",1);
        for (auto _viewer : _viewers) {
            _viewer->navigationStyle()->setOrbitStyle(NavigationStyle::OrbitStyle(style));
        }
    }
    else if (strcmp(Reason,"Sensitivity") == 0) {
        float val = rGrp.GetFloat("Sensitivity",2.0f);
        for (auto _viewer : _viewers) {
            _viewer->navigationStyle()->setSensitivity(val);
        }
    }
    else if (strcmp(Reason,"ResetCursorPosition") == 0) {
        bool on = rGrp.GetBool("ResetCursorPosition",false);
        for (auto _viewer : _viewers) {
            _viewer->navigationStyle()->setResetCursorPosition(on);
        }
    }
    else if (strcmp(Reason,"InvertZoom") == 0) {
        bool on = rGrp.GetBool("InvertZoom", true);
        for (auto _viewer : _viewers) {
            _viewer->navigationStyle()->setZoomInverted(on);
        }
    }
    else if (strcmp(Reason,"ZoomAtCursor") == 0) {
        bool on = rGrp.GetBool("ZoomAtCursor", true);
        for (auto _viewer : _viewers) {
            _viewer->navigationStyle()->setZoomAtCursor(on);
        }
    }
    else if (strcmp(Reason,"ZoomStep") == 0) {
        float val = rGrp.GetFloat("ZoomStep", 0.0f);
        for (auto _viewer : _viewers) {
            _viewer->navigationStyle()->setZoomStep(val);
        }
    }
    else if (strcmp(Reason,"RotationMode") == 0) {
        long mode = rGrp.GetInt("RotationMode", 1);
        for (auto _viewer : _viewers) {
            if (mode == 0) {
                _viewer->navigationStyle()->setRotationCenterMode(NavigationStyle::RotationCenterMode::WindowCenter);
            }
            else if (mode == 1) {
                _viewer->navigationStyle()->setRotationCenterMode(NavigationStyle::RotationCenterMode::ScenePointAtCursor |
                                                                  NavigationStyle::RotationCenterMode::FocalPointAtCursor);
            }
            else if (mode == 2) {
                _viewer->navigationStyle()->setRotationCenterMode(NavigationStyle::RotationCenterMode::ScenePointAtCursor |
                                                                  NavigationStyle::RotationCenterMode::BoundingBoxCenter);
            }
        }
    }
    else if (strcmp(Reason,"EyeDistance") == 0) {
        for (auto _viewer : _viewers) {
            _viewer->getSoRenderManager()->setStereoOffset(rGrp.GetFloat("EyeDistance", 5.0));
        }
    }
    else if (strcmp(Reason,"CornerCoordSystem") == 0) {
        for (auto _viewer : _viewers) {
            _viewer->setFeedbackVisibility(rGrp.GetBool("CornerCoordSystem", true));
        }
    }
    else if (strcmp(Reason,"CornerCoordSystemSize") == 0) {
        for (auto _viewer : _viewers) {
            _viewer->setFeedbackSize(rGrp.GetInt("CornerCoordSystemSize", 10));
        }
    }
    else if (strcmp(Reason,"ShowAxisCross") == 0) {
        for (auto _viewer : _viewers) {
            _viewer->setAxisCross(rGrp.GetBool("ShowAxisCross", false));
        }
    }
    else if (strcmp(Reason,"UseAutoRotation") == 0) {
        for (auto _viewer : _viewers) {
            _viewer->setAnimationEnabled(rGrp.GetBool("UseAutoRotation", false));
        }
    }
    else if (strcmp(Reason,"Gradient") == 0 || strcmp(Reason,"RadialGradient") == 0) {
        View3DInventorViewer::Background background = View3DInventorViewer::Background::NoGradient;
        if (rGrp.GetBool("Gradient", true)) {
            background = View3DInventorViewer::Background::LinearGradient;
        }
        else if (rGrp.GetBool("RadialGradient", false)) {
            background = View3DInventorViewer::Background::RadialGradient;
        }
        for (auto _viewer : _viewers) {
            _viewer->setGradientBackground(background);
        }
    }
    else if (strcmp(Reason,"ShowFPS") == 0) {
        for (auto _viewer : _viewers) {
            _viewer->setEnabledFPSCounter(rGrp.GetBool("ShowFPS", false));
        }
    }
    else if (strcmp(Reason,"ShowNaviCube") == 0) {
        for (auto _viewer : _viewers) {
            _viewer->setEnabledNaviCube(rGrp.GetBool("ShowNaviCube", true));
        }
    }
    else if (strcmp(Reason,"UseVBO") == 0) {
        if (!ignoreVBO) {
            for (auto _viewer : _viewers) {
                _viewer->setEnabledVBO(rGrp.GetBool("UseVBO", false));
            }
        }
    }
    else if (strcmp(Reason,"RenderCache") == 0) {
        if (!ignoreRenderCache) {
            for (auto _viewer : _viewers) {
                _viewer->setRenderCache(rGrp.GetInt("RenderCache", 0));
            }
        }
    }
    else if (strcmp(Reason,"Orthographic") == 0) {
        // check whether a perspective or orthogrphic camera should be set
        if (rGrp.GetBool("Orthographic", true)) {
            for (auto _viewer : _viewers) {
                _viewer->setCameraType(SoOrthographicCamera::getClassTypeId());
            }
        }
        else {
            for (auto _viewer : _viewers) {
                _viewer->setCameraType(SoPerspectiveCamera::getClassTypeId());
            }
        }
    }
    else if (strcmp(Reason, "DimensionsVisible") == 0) {
        if (!ignoreDimensions) {
            if (rGrp.GetBool("DimensionsVisible", true)) {
                for (auto _viewer : _viewers) {
                    _viewer->turnAllDimensionsOn();
                }
            }
            else {
                for (auto _viewer : _viewers) {
                    _viewer->turnAllDimensionsOff();
                }
            }
        }
    }
    else if (strcmp(Reason, "Dimensions3dVisible") == 0) {
        if (!ignoreDimensions) {
            if (rGrp.GetBool("Dimensions3dVisible", true)) {
                for (auto _viewer : _viewers) {
                    _viewer->turn3dDimensionsOn();
                }
            }
            else {
                for (auto _viewer : _viewers) {
                    _viewer->turn3dDimensionsOff();
                }
            }
        }
    }
    else if (strcmp(Reason, "DimensionsDeltaVisible") == 0) {
        if (!ignoreDimensions) {
            if (rGrp.GetBool("DimensionsDeltaVisible", true)) {
                for (auto _viewer : _viewers) {
                    _viewer->turnDeltaDimensionsOn();
                }
            }
            else {
                for (auto _viewer : _viewers) {
                    _viewer->turnDeltaDimensionsOff();
                }
            }
        }
    }
    else if (strcmp(Reason, "PickRadius") == 0) {
        for (auto _viewer : _viewers) {
            _viewer->setPickRadius(rGrp.GetFloat("PickRadius", 5.0f));
        }
    }
    else if (strcmp(Reason, "TransparentObjectRenderType") == 0) {
        if (!ignoreTransparent) {
            long renderType = rGrp.GetInt("TransparentObjectRenderType", 0);
            if (renderType == 0) {
                for (auto _viewer : _viewers) {
                    _viewer->getSoRenderManager()->getGLRenderAction()
                           ->setTransparentDelayedObjectRenderType(SoGLRenderAction::ONE_PASS);
                }
            }
            else if (renderType == 1) {
                for (auto _viewer : _viewers) {
                    _viewer->getSoRenderManager()->getGLRenderAction()
                           ->setTransparentDelayedObjectRenderType(SoGLRenderAction::
                                                                   NONSOLID_SEPARATE_BACKFACE_PASS);
                }
            }
        }
    }
    else {
        unsigned long col1 = rGrp.GetUnsigned("BackgroundColor",3940932863UL);
        unsigned long col2 = rGrp.GetUnsigned("BackgroundColor2",859006463UL); // default color (dark blue)
        unsigned long col3 = rGrp.GetUnsigned("BackgroundColor3",2880160255UL); // default color (blue/grey)
        unsigned long col4 = rGrp.GetUnsigned("BackgroundColor4",1869583359UL); // default color (blue/grey)
        float r1,g1,b1,r2,g2,b2,r3,g3,b3,r4,g4,b4;
        r1 = ((col1 >> 24) & 0xff) / 255.0; g1 = ((col1 >> 16) & 0xff) / 255.0; b1 = ((col1 >> 8) & 0xff) / 255.0;
        r2 = ((col2 >> 24) & 0xff) / 255.0; g2 = ((col2 >> 16) & 0xff) / 255.0; b2 = ((col2 >> 8) & 0xff) / 255.0;
        r3 = ((col3 >> 24) & 0xff) / 255.0; g3 = ((col3 >> 16) & 0xff) / 255.0; b3 = ((col3 >> 8) & 0xff) / 255.0;
        r4 = ((col4 >> 24) & 0xff) / 255.0; g4 = ((col4 >> 16) & 0xff) / 255.0; b4 = ((col4 >> 8) & 0xff) / 255.0;
        for (auto _viewer : _viewers) {
            _viewer->setBackgroundColor(QColor::fromRgbF(r1, g1, b1));
            if (!rGrp.GetBool("UseBackgroundColorMid",false)) {
                _viewer->setGradientBackgroundColor(SbColor(r2, g2, b2),
                                                    SbColor(r3, g3, b3));
            }
            else {
                _viewer->setGradientBackgroundColor(SbColor(r2, g2, b2),
                                                    SbColor(r3, g3, b3),
                                                    SbColor(r4, g4, b4));
            }
        }
    }
}

// ----------------------------------------------------------------------------

NaviCubeSettings::NaviCubeSettings(ParameterGrp::handle hGrp,
                                   View3DInventorViewer* view)
    : hGrp(hGrp)
    , _viewer(view)
{
    connectParameterChanged = hGrp->Manager()->signalParamChanged.connect(
        [this](ParameterGrp*, ParameterGrp::ParamType, const char *Name, const char *) {
            parameterChanged(Name);
    });
}

NaviCubeSettings::~NaviCubeSettings()
{
    connectParameterChanged.disconnect();
}

void NaviCubeSettings::applySettings()
{
    parameterChanged("BaseColor");
    parameterChanged("EmphaseColor");
    parameterChanged("HiliteColor");
    parameterChanged("CornerNaviCube");
    parameterChanged("OffsetX"); // Updates OffsetY too
    parameterChanged("CubeSize");
    parameterChanged("ChamferSize");
    parameterChanged("NaviRotateToNearest");
    parameterChanged("NaviStepByTurn");
    parameterChanged("BorderWidth");
    parameterChanged("FontZoom");
    parameterChanged("FontString");
    parameterChanged("FontWeight");
    parameterChanged("FontStretch");
    parameterChanged("ShowCS");
    parameterChanged("TextFront"); // Updates all labels
}

void NaviCubeSettings::parameterChanged(const char* Name)
{
    if (Name == nullptr)
        return;
    NaviCube* nc = _viewer->getNaviCube();
    if (strcmp(Name, "CornerNaviCube") == 0) {
        nc->setCorner(static_cast<NaviCube::Corner>(hGrp->GetInt("CornerNaviCube", 1)));
    }
    else if (strcmp(Name, "OffsetX") == 0 || strcmp(Name, "OffsetY") == 0) {
        nc->setOffset(hGrp->GetInt("OffsetX", 0), hGrp->GetInt("OffsetY", 0));
    }
    else if (strcmp(Name, "ChamferSize") == 0) {
        nc->setChamfer(hGrp->GetFloat("ChamferSize", 0.12f));
    }
    else if (strcmp(Name, "CubeSize") == 0) {
        nc->setSize(hGrp->GetInt("CubeSize", 132));
    }
    else if (strcmp(Name, "NaviRotateToNearest") == 0) {
        nc->setNaviRotateToNearest(hGrp->GetBool("NaviRotateToNearest", true));
    }
    else if (strcmp(Name, "NaviStepByTurn") == 0) {
        nc->setNaviStepByTurn(hGrp->GetInt("NaviStepByTurn", 8));
    }
    else if (strcmp(Name, "FontZoom") == 0) {
        nc->setFontZoom(hGrp->GetFloat("FontZoom", 0.3));
    }
    else if (strcmp(Name, "FontString") == 0) {
        nc->setFont(hGrp->GetASCII("FontString"));
    }
    else if (strcmp(Name, "FontWeight") == 0) {
        nc->setFontWeight(hGrp->GetInt("FontWeight", 0));
    }
    else if (strcmp(Name, "FontStretch") == 0) {
        nc->setFontStretch(hGrp->GetInt("FontStretch", 0));
    }
    else if (strcmp(Name, "BaseColor") == 0) {
        unsigned long col = hGrp->GetUnsigned("BaseColor", 3806916544);
        nc->setBaseColor(App::Color::fromPackedRGBA<QColor>(col));
        // update default contrast colors
        parameterChanged("EmphaseColor");
    }
    else if (strcmp(Name, "EmphaseColor") == 0) {
        App::Color bc((uint32_t)hGrp->GetUnsigned("BaseColor", 3806916544));
        unsigned long d = bc.r + bc.g + bc.b >= 1.5f ? 255 : 4294967295;
        unsigned long col = hGrp->GetUnsigned("EmphaseColor", d);
        nc->setEmphaseColor(App::Color::fromPackedRGBA<QColor>(col));
    }
    else if (strcmp(Name, "HiliteColor") == 0) {
        unsigned long col = hGrp->GetUnsigned("HiliteColor", 2867003391);
        nc->setHiliteColor(App::Color::fromPackedRGBA<QColor>(col));
    }
    else if (strcmp(Name, "BorderWidth") == 0) {
        nc->setBorderWidth(hGrp->GetFloat("BorderWidth", 1.1));
    }
    else if (strcmp(Name, "ShowCS") == 0) {
        nc->setShowCS(hGrp->GetBool("ShowCS", true));
    }
    else if (strcmp(Name, "TextTop") == 0 || strcmp(Name, "TextBottom") == 0
             || strcmp(Name, "TextFront") == 0 || strcmp(Name, "TextRear") == 0
             || strcmp(Name, "TextLeft") == 0 || strcmp(Name, "TextRight") == 0) {
        std::vector<std::string> labels;
        QByteArray frontByteArray = tr("FRONT").toUtf8();
        labels.push_back(hGrp->GetASCII("TextFront", frontByteArray.constData()));
        QByteArray topByteArray = tr("TOP").toUtf8();
        labels.push_back(hGrp->GetASCII("TextTop", topByteArray.constData()));
        QByteArray rightByteArray = tr("RIGHT").toUtf8();
        labels.push_back(hGrp->GetASCII("TextRight", rightByteArray.constData()));
        QByteArray rearByteArray = tr("REAR").toUtf8();
        labels.push_back(hGrp->GetASCII("TextRear", rearByteArray.constData()));
        QByteArray bottomByteArray = tr("BOTTOM").toUtf8();
        labels.push_back(hGrp->GetASCII("TextBottom", bottomByteArray.constData()));
        QByteArray leftByteArray = tr("LEFT").toUtf8();
        labels.push_back(hGrp->GetASCII("TextLeft", leftByteArray.constData()));
        nc->setNaviCubeLabels(labels);
    }
    _viewer->getSoRenderManager()->scheduleRedraw();
}
