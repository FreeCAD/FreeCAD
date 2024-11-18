/****************************************************************************
 *   Copyright (c) 2022 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#include "PreCompiled.h"
#include "OverlayManager.h"

/*[[[cog
import OverlayParams
OverlayParams.define()
]]]*/

// Auto generated code (Tools/params_utils.py:196)
#include <unordered_map>
#include <App/Application.h>
#include <App/DynamicProperty.h>
#include "OverlayParams.h"
using namespace Gui;

// Auto generated code (Tools/params_utils.py:207)
namespace {
class OverlayParamsP: public ParameterGrp::ObserverType {
public:
    ParameterGrp::handle handle;
    std::unordered_map<const char *,void(*)(OverlayParamsP*),App::CStringHasher,App::CStringHasher> funcs;

    bool DockOverlayAutoView;
    long DockOverlayDelay;
    long DockOverlayRevealDelay;
    long DockOverlaySplitterHandleTimeout;
    bool DockOverlayActivateOnHover;
    bool DockOverlayAutoMouseThrough;
    bool DockOverlayWheelPassThrough;
    long DockOverlayWheelDelay;
    long DockOverlayAlphaRadius;
    bool DockOverlayCheckNaviCube;
    long DockOverlayHintTriggerSize;
    long DockOverlayHintSize;
    long DockOverlayHintLeftLength;
    long DockOverlayHintRightLength;
    long DockOverlayHintTopLength;
    long DockOverlayHintBottomLength;
    long DockOverlayHintLeftOffset;
    long DockOverlayHintRightOffset;
    long DockOverlayHintTopOffset;
    long DockOverlayHintBottomOffset;
    bool DockOverlayHintTabBar;
    bool DockOverlayHideTabBar;
    long DockOverlayHintDelay;
    long DockOverlayAnimationDuration;
    long DockOverlayAnimationCurve;
    bool DockOverlayHidePropertyViewScrollBar;
    long DockOverlayMinimumSize;

    // Auto generated code (Tools/params_utils.py:245)
    OverlayParamsP() {
        handle = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
        handle->Attach(this);

        DockOverlayAutoView = handle->GetBool("DockOverlayAutoView", true);
        funcs["DockOverlayAutoView"] = &OverlayParamsP::updateDockOverlayAutoView;
        DockOverlayDelay = handle->GetInt("DockOverlayDelay", 200);
        funcs["DockOverlayDelay"] = &OverlayParamsP::updateDockOverlayDelay;
        DockOverlayRevealDelay = handle->GetInt("DockOverlayRevealDelay", 2000);
        funcs["DockOverlayRevealDelay"] = &OverlayParamsP::updateDockOverlayRevealDelay;
        DockOverlaySplitterHandleTimeout = handle->GetInt("DockOverlaySplitterHandleTimeout", 0);
        funcs["DockOverlaySplitterHandleTimeout"] = &OverlayParamsP::updateDockOverlaySplitterHandleTimeout;
        DockOverlayActivateOnHover = handle->GetBool("DockOverlayActivateOnHover", true);
        funcs["DockOverlayActivateOnHover"] = &OverlayParamsP::updateDockOverlayActivateOnHover;
        DockOverlayAutoMouseThrough = handle->GetBool("DockOverlayAutoMouseThrough", true);
        funcs["DockOverlayAutoMouseThrough"] = &OverlayParamsP::updateDockOverlayAutoMouseThrough;
        DockOverlayWheelPassThrough = handle->GetBool("DockOverlayWheelPassThrough", true);
        funcs["DockOverlayWheelPassThrough"] = &OverlayParamsP::updateDockOverlayWheelPassThrough;
        DockOverlayWheelDelay = handle->GetInt("DockOverlayWheelDelay", 1000);
        funcs["DockOverlayWheelDelay"] = &OverlayParamsP::updateDockOverlayWheelDelay;
        DockOverlayAlphaRadius = handle->GetInt("DockOverlayAlphaRadius", 2);
        funcs["DockOverlayAlphaRadius"] = &OverlayParamsP::updateDockOverlayAlphaRadius;
        DockOverlayCheckNaviCube = handle->GetBool("DockOverlayCheckNaviCube", true);
        funcs["DockOverlayCheckNaviCube"] = &OverlayParamsP::updateDockOverlayCheckNaviCube;
        DockOverlayHintTriggerSize = handle->GetInt("DockOverlayHintTriggerSize", 16);
        funcs["DockOverlayHintTriggerSize"] = &OverlayParamsP::updateDockOverlayHintTriggerSize;
        DockOverlayHintSize = handle->GetInt("DockOverlayHintSize", 8);
        funcs["DockOverlayHintSize"] = &OverlayParamsP::updateDockOverlayHintSize;
        DockOverlayHintLeftLength = handle->GetInt("DockOverlayHintLeftLength", 100);
        funcs["DockOverlayHintLeftLength"] = &OverlayParamsP::updateDockOverlayHintLeftLength;
        DockOverlayHintRightLength = handle->GetInt("DockOverlayHintRightLength", 100);
        funcs["DockOverlayHintRightLength"] = &OverlayParamsP::updateDockOverlayHintRightLength;
        DockOverlayHintTopLength = handle->GetInt("DockOverlayHintTopLength", 100);
        funcs["DockOverlayHintTopLength"] = &OverlayParamsP::updateDockOverlayHintTopLength;
        DockOverlayHintBottomLength = handle->GetInt("DockOverlayHintBottomLength", 100);
        funcs["DockOverlayHintBottomLength"] = &OverlayParamsP::updateDockOverlayHintBottomLength;
        DockOverlayHintLeftOffset = handle->GetInt("DockOverlayHintLeftOffset", 0);
        funcs["DockOverlayHintLeftOffset"] = &OverlayParamsP::updateDockOverlayHintLeftOffset;
        DockOverlayHintRightOffset = handle->GetInt("DockOverlayHintRightOffset", 0);
        funcs["DockOverlayHintRightOffset"] = &OverlayParamsP::updateDockOverlayHintRightOffset;
        DockOverlayHintTopOffset = handle->GetInt("DockOverlayHintTopOffset", 0);
        funcs["DockOverlayHintTopOffset"] = &OverlayParamsP::updateDockOverlayHintTopOffset;
        DockOverlayHintBottomOffset = handle->GetInt("DockOverlayHintBottomOffset", 0);
        funcs["DockOverlayHintBottomOffset"] = &OverlayParamsP::updateDockOverlayHintBottomOffset;
        DockOverlayHintTabBar = handle->GetBool("DockOverlayHintTabBar", false);
        funcs["DockOverlayHintTabBar"] = &OverlayParamsP::updateDockOverlayHintTabBar;
        DockOverlayHideTabBar = handle->GetBool("DockOverlayHideTabBar", true);
        funcs["DockOverlayHideTabBar"] = &OverlayParamsP::updateDockOverlayHideTabBar;
        DockOverlayHintDelay = handle->GetInt("DockOverlayHintDelay", 200);
        funcs["DockOverlayHintDelay"] = &OverlayParamsP::updateDockOverlayHintDelay;
        DockOverlayAnimationDuration = handle->GetInt("DockOverlayAnimationDuration", 200);
        funcs["DockOverlayAnimationDuration"] = &OverlayParamsP::updateDockOverlayAnimationDuration;
        DockOverlayAnimationCurve = handle->GetInt("DockOverlayAnimationCurve", 7);
        funcs["DockOverlayAnimationCurve"] = &OverlayParamsP::updateDockOverlayAnimationCurve;
        DockOverlayHidePropertyViewScrollBar = handle->GetBool("DockOverlayHidePropertyViewScrollBar", false);
        funcs["DockOverlayHidePropertyViewScrollBar"] = &OverlayParamsP::updateDockOverlayHidePropertyViewScrollBar;
        DockOverlayMinimumSize = handle->GetInt("DockOverlayMinimumSize", 30);
        funcs["DockOverlayMinimumSize"] = &OverlayParamsP::updateDockOverlayMinimumSize;
    }

    // Auto generated code (Tools/params_utils.py:263)
    ~OverlayParamsP() {
    }

    // Auto generated code (Tools/params_utils.py:270)
    void OnChange(Base::Subject<const char*> &, const char* sReason) {
        if(!sReason)
            return;
        auto it = funcs.find(sReason);
        if(it == funcs.end())
            return;
        it->second(this);
    }


    // Auto generated code (Tools/params_utils.py:296)
    static void updateDockOverlayAutoView(OverlayParamsP *self) {
        auto v = self->handle->GetBool("DockOverlayAutoView", true);
        if (self->DockOverlayAutoView != v) {
            self->DockOverlayAutoView = v;
            OverlayParams::onDockOverlayAutoViewChanged();
        }
    }
    // Auto generated code (Tools/params_utils.py:288)
    static void updateDockOverlayDelay(OverlayParamsP *self) {
        self->DockOverlayDelay = self->handle->GetInt("DockOverlayDelay", 200);
    }
    // Auto generated code (Tools/params_utils.py:288)
    static void updateDockOverlayRevealDelay(OverlayParamsP *self) {
        self->DockOverlayRevealDelay = self->handle->GetInt("DockOverlayRevealDelay", 2000);
    }
    // Auto generated code (Tools/params_utils.py:288)
    static void updateDockOverlaySplitterHandleTimeout(OverlayParamsP *self) {
        self->DockOverlaySplitterHandleTimeout = self->handle->GetInt("DockOverlaySplitterHandleTimeout", 0);
    }
    // Auto generated code (Tools/params_utils.py:288)
    static void updateDockOverlayActivateOnHover(OverlayParamsP *self) {
        self->DockOverlayActivateOnHover = self->handle->GetBool("DockOverlayActivateOnHover", true);
    }
    // Auto generated code (Tools/params_utils.py:288)
    static void updateDockOverlayAutoMouseThrough(OverlayParamsP *self) {
        self->DockOverlayAutoMouseThrough = self->handle->GetBool("DockOverlayAutoMouseThrough", true);
    }
    // Auto generated code (Tools/params_utils.py:288)
    static void updateDockOverlayWheelPassThrough(OverlayParamsP *self) {
        self->DockOverlayWheelPassThrough = self->handle->GetBool("DockOverlayWheelPassThrough", true);
    }
    // Auto generated code (Tools/params_utils.py:288)
    static void updateDockOverlayWheelDelay(OverlayParamsP *self) {
        self->DockOverlayWheelDelay = self->handle->GetInt("DockOverlayWheelDelay", 1000);
    }
    // Auto generated code (Tools/params_utils.py:288)
    static void updateDockOverlayAlphaRadius(OverlayParamsP *self) {
        self->DockOverlayAlphaRadius = self->handle->GetInt("DockOverlayAlphaRadius", 2);
    }
    // Auto generated code (Tools/params_utils.py:296)
    static void updateDockOverlayCheckNaviCube(OverlayParamsP *self) {
        auto v = self->handle->GetBool("DockOverlayCheckNaviCube", true);
        if (self->DockOverlayCheckNaviCube != v) {
            self->DockOverlayCheckNaviCube = v;
            OverlayParams::onDockOverlayCheckNaviCubeChanged();
        }
    }
    // Auto generated code (Tools/params_utils.py:288)
    static void updateDockOverlayHintTriggerSize(OverlayParamsP *self) {
        self->DockOverlayHintTriggerSize = self->handle->GetInt("DockOverlayHintTriggerSize", 16);
    }
    // Auto generated code (Tools/params_utils.py:288)
    static void updateDockOverlayHintSize(OverlayParamsP *self) {
        self->DockOverlayHintSize = self->handle->GetInt("DockOverlayHintSize", 8);
    }
    // Auto generated code (Tools/params_utils.py:288)
    static void updateDockOverlayHintLeftLength(OverlayParamsP *self) {
        self->DockOverlayHintLeftLength = self->handle->GetInt("DockOverlayHintLeftLength", 100);
    }
    // Auto generated code (Tools/params_utils.py:288)
    static void updateDockOverlayHintRightLength(OverlayParamsP *self) {
        self->DockOverlayHintRightLength = self->handle->GetInt("DockOverlayHintRightLength", 100);
    }
    // Auto generated code (Tools/params_utils.py:288)
    static void updateDockOverlayHintTopLength(OverlayParamsP *self) {
        self->DockOverlayHintTopLength = self->handle->GetInt("DockOverlayHintTopLength", 100);
    }
    // Auto generated code (Tools/params_utils.py:288)
    static void updateDockOverlayHintBottomLength(OverlayParamsP *self) {
        self->DockOverlayHintBottomLength = self->handle->GetInt("DockOverlayHintBottomLength", 100);
    }
    // Auto generated code (Tools/params_utils.py:288)
    static void updateDockOverlayHintLeftOffset(OverlayParamsP *self) {
        self->DockOverlayHintLeftOffset = self->handle->GetInt("DockOverlayHintLeftOffset", 0);
    }
    // Auto generated code (Tools/params_utils.py:288)
    static void updateDockOverlayHintRightOffset(OverlayParamsP *self) {
        self->DockOverlayHintRightOffset = self->handle->GetInt("DockOverlayHintRightOffset", 0);
    }
    // Auto generated code (Tools/params_utils.py:288)
    static void updateDockOverlayHintTopOffset(OverlayParamsP *self) {
        self->DockOverlayHintTopOffset = self->handle->GetInt("DockOverlayHintTopOffset", 0);
    }
    // Auto generated code (Tools/params_utils.py:288)
    static void updateDockOverlayHintBottomOffset(OverlayParamsP *self) {
        self->DockOverlayHintBottomOffset = self->handle->GetInt("DockOverlayHintBottomOffset", 0);
    }
    // Auto generated code (Tools/params_utils.py:288)
    static void updateDockOverlayHintTabBar(OverlayParamsP *self) {
        self->DockOverlayHintTabBar = self->handle->GetBool("DockOverlayHintTabBar", false);
    }
    // Auto generated code (Tools/params_utils.py:296)
    static void updateDockOverlayHideTabBar(OverlayParamsP *self) {
        auto v = self->handle->GetBool("DockOverlayHideTabBar", true);
        if (self->DockOverlayHideTabBar != v) {
            self->DockOverlayHideTabBar = v;
            OverlayParams::onDockOverlayHideTabBarChanged();
        }
    }
    // Auto generated code (Tools/params_utils.py:288)
    static void updateDockOverlayHintDelay(OverlayParamsP *self) {
        self->DockOverlayHintDelay = self->handle->GetInt("DockOverlayHintDelay", 200);
    }
    // Auto generated code (Tools/params_utils.py:288)
    static void updateDockOverlayAnimationDuration(OverlayParamsP *self) {
        self->DockOverlayAnimationDuration = self->handle->GetInt("DockOverlayAnimationDuration", 200);
    }
    // Auto generated code (Tools/params_utils.py:288)
    static void updateDockOverlayAnimationCurve(OverlayParamsP *self) {
        self->DockOverlayAnimationCurve = self->handle->GetInt("DockOverlayAnimationCurve", 7);
    }
    // Auto generated code (Tools/params_utils.py:288)
    static void updateDockOverlayHidePropertyViewScrollBar(OverlayParamsP *self) {
        self->DockOverlayHidePropertyViewScrollBar = self->handle->GetBool("DockOverlayHidePropertyViewScrollBar", false);
    }
    // Auto generated code (Tools/params_utils.py:296)
    static void updateDockOverlayMinimumSize(OverlayParamsP *self) {
        auto v = self->handle->GetInt("DockOverlayMinimumSize", 30);
        if (self->DockOverlayMinimumSize != v) {
            self->DockOverlayMinimumSize = v;
            OverlayParams::onDockOverlayMinimumSizeChanged();
        }
    }
};

// Auto generated code (Tools/params_utils.py:310)
OverlayParamsP *instance() {
    static OverlayParamsP *inst = new OverlayParamsP;
    return inst;
}

} // Anonymous namespace

// Auto generated code (Tools/params_utils.py:321)
ParameterGrp::handle OverlayParams::getHandle() {
    return instance()->handle;
}

// Auto generated code (Tools/params_utils.py:350)
const char *OverlayParams::docDockOverlayAutoView() {
    return "";
}

// Auto generated code (Tools/params_utils.py:358)
const bool & OverlayParams::getDockOverlayAutoView() {
    return instance()->DockOverlayAutoView;
}

// Auto generated code (Tools/params_utils.py:366)
const bool & OverlayParams::defaultDockOverlayAutoView() {
    const static bool def = true;
    return def;
}

// Auto generated code (Tools/params_utils.py:375)
void OverlayParams::setDockOverlayAutoView(const bool &v) {
    instance()->handle->SetBool("DockOverlayAutoView",v);
    instance()->DockOverlayAutoView = v;
}

// Auto generated code (Tools/params_utils.py:384)
void OverlayParams::removeDockOverlayAutoView() {
    instance()->handle->RemoveBool("DockOverlayAutoView");
}

// Auto generated code (Tools/params_utils.py:350)
const char *OverlayParams::docDockOverlayDelay() {
    return QT_TRANSLATE_NOOP("OverlayParams",
"Overlay dock (re),layout delay.");
}

// Auto generated code (Tools/params_utils.py:358)
const long & OverlayParams::getDockOverlayDelay() {
    return instance()->DockOverlayDelay;
}

// Auto generated code (Tools/params_utils.py:366)
const long & OverlayParams::defaultDockOverlayDelay() {
    const static long def = 200;
    return def;
}

// Auto generated code (Tools/params_utils.py:375)
void OverlayParams::setDockOverlayDelay(const long &v) {
    instance()->handle->SetInt("DockOverlayDelay",v);
    instance()->DockOverlayDelay = v;
}

// Auto generated code (Tools/params_utils.py:384)
void OverlayParams::removeDockOverlayDelay() {
    instance()->handle->RemoveInt("DockOverlayDelay");
}

// Auto generated code (Tools/params_utils.py:350)
const char *OverlayParams::docDockOverlayRevealDelay() {
    return "";
}

// Auto generated code (Tools/params_utils.py:358)
const long & OverlayParams::getDockOverlayRevealDelay() {
    return instance()->DockOverlayRevealDelay;
}

// Auto generated code (Tools/params_utils.py:366)
const long & OverlayParams::defaultDockOverlayRevealDelay() {
    const static long def = 2000;
    return def;
}

// Auto generated code (Tools/params_utils.py:375)
void OverlayParams::setDockOverlayRevealDelay(const long &v) {
    instance()->handle->SetInt("DockOverlayRevealDelay",v);
    instance()->DockOverlayRevealDelay = v;
}

// Auto generated code (Tools/params_utils.py:384)
void OverlayParams::removeDockOverlayRevealDelay() {
    instance()->handle->RemoveInt("DockOverlayRevealDelay");
}

// Auto generated code (Tools/params_utils.py:350)
const char *OverlayParams::docDockOverlaySplitterHandleTimeout() {
    return QT_TRANSLATE_NOOP("OverlayParams",
"Overlay splitter handle auto hide delay. Set zero to disable auto hiding.");
}

// Auto generated code (Tools/params_utils.py:358)
const long & OverlayParams::getDockOverlaySplitterHandleTimeout() {
    return instance()->DockOverlaySplitterHandleTimeout;
}

// Auto generated code (Tools/params_utils.py:366)
const long & OverlayParams::defaultDockOverlaySplitterHandleTimeout() {
    const static long def = 0;
    return def;
}

// Auto generated code (Tools/params_utils.py:375)
void OverlayParams::setDockOverlaySplitterHandleTimeout(const long &v) {
    instance()->handle->SetInt("DockOverlaySplitterHandleTimeout",v);
    instance()->DockOverlaySplitterHandleTimeout = v;
}

// Auto generated code (Tools/params_utils.py:384)
void OverlayParams::removeDockOverlaySplitterHandleTimeout() {
    instance()->handle->RemoveInt("DockOverlaySplitterHandleTimeout");
}

// Auto generated code (Tools/params_utils.py:350)
const char *OverlayParams::docDockOverlayActivateOnHover() {
    return QT_TRANSLATE_NOOP("OverlayParams",
"Show auto hidden dock overlay on mouse over.\n"
"If disabled, then show on mouse click.");
}

// Auto generated code (Tools/params_utils.py:358)
const bool & OverlayParams::getDockOverlayActivateOnHover() {
    return instance()->DockOverlayActivateOnHover;
}

// Auto generated code (Tools/params_utils.py:366)
const bool & OverlayParams::defaultDockOverlayActivateOnHover() {
    const static bool def = true;
    return def;
}

// Auto generated code (Tools/params_utils.py:375)
void OverlayParams::setDockOverlayActivateOnHover(const bool &v) {
    instance()->handle->SetBool("DockOverlayActivateOnHover",v);
    instance()->DockOverlayActivateOnHover = v;
}

// Auto generated code (Tools/params_utils.py:384)
void OverlayParams::removeDockOverlayActivateOnHover() {
    instance()->handle->RemoveBool("DockOverlayActivateOnHover");
}

// Auto generated code (Tools/params_utils.py:350)
const char *OverlayParams::docDockOverlayAutoMouseThrough() {
    return QT_TRANSLATE_NOOP("OverlayParams",
"Auto mouse click through transparent part of dock overlay.");
}

// Auto generated code (Tools/params_utils.py:358)
const bool & OverlayParams::getDockOverlayAutoMouseThrough() {
    return instance()->DockOverlayAutoMouseThrough;
}

// Auto generated code (Tools/params_utils.py:366)
const bool & OverlayParams::defaultDockOverlayAutoMouseThrough() {
    const static bool def = true;
    return def;
}

// Auto generated code (Tools/params_utils.py:375)
void OverlayParams::setDockOverlayAutoMouseThrough(const bool &v) {
    instance()->handle->SetBool("DockOverlayAutoMouseThrough",v);
    instance()->DockOverlayAutoMouseThrough = v;
}

// Auto generated code (Tools/params_utils.py:384)
void OverlayParams::removeDockOverlayAutoMouseThrough() {
    instance()->handle->RemoveBool("DockOverlayAutoMouseThrough");
}

// Auto generated code (Tools/params_utils.py:350)
const char *OverlayParams::docDockOverlayWheelPassThrough() {
    return QT_TRANSLATE_NOOP("OverlayParams",
"Auto pass through mouse wheel event on transparent dock overlay.");
}

// Auto generated code (Tools/params_utils.py:358)
const bool & OverlayParams::getDockOverlayWheelPassThrough() {
    return instance()->DockOverlayWheelPassThrough;
}

// Auto generated code (Tools/params_utils.py:366)
const bool & OverlayParams::defaultDockOverlayWheelPassThrough() {
    const static bool def = true;
    return def;
}

// Auto generated code (Tools/params_utils.py:375)
void OverlayParams::setDockOverlayWheelPassThrough(const bool &v) {
    instance()->handle->SetBool("DockOverlayWheelPassThrough",v);
    instance()->DockOverlayWheelPassThrough = v;
}

// Auto generated code (Tools/params_utils.py:384)
void OverlayParams::removeDockOverlayWheelPassThrough() {
    instance()->handle->RemoveBool("DockOverlayWheelPassThrough");
}

// Auto generated code (Tools/params_utils.py:350)
const char *OverlayParams::docDockOverlayWheelDelay() {
    return QT_TRANSLATE_NOOP("OverlayParams",
"Delay capturing mouse wheel event for passing through if it is\n"
"previously handled by other widget.");
}

// Auto generated code (Tools/params_utils.py:358)
const long & OverlayParams::getDockOverlayWheelDelay() {
    return instance()->DockOverlayWheelDelay;
}

// Auto generated code (Tools/params_utils.py:366)
const long & OverlayParams::defaultDockOverlayWheelDelay() {
    const static long def = 1000;
    return def;
}

// Auto generated code (Tools/params_utils.py:375)
void OverlayParams::setDockOverlayWheelDelay(const long &v) {
    instance()->handle->SetInt("DockOverlayWheelDelay",v);
    instance()->DockOverlayWheelDelay = v;
}

// Auto generated code (Tools/params_utils.py:384)
void OverlayParams::removeDockOverlayWheelDelay() {
    instance()->handle->RemoveInt("DockOverlayWheelDelay");
}

// Auto generated code (Tools/params_utils.py:350)
const char *OverlayParams::docDockOverlayAlphaRadius() {
    return QT_TRANSLATE_NOOP("OverlayParams",
"If auto mouse click through is enabled, then this radius\n"
"defines a region of alpha test under the mouse cursor.\n"
"Auto click through is only activated if all pixels within\n"
"the region are non-opaque.");
}

// Auto generated code (Tools/params_utils.py:358)
const long & OverlayParams::getDockOverlayAlphaRadius() {
    return instance()->DockOverlayAlphaRadius;
}

// Auto generated code (Tools/params_utils.py:366)
const long & OverlayParams::defaultDockOverlayAlphaRadius() {
    const static long def = 2;
    return def;
}

// Auto generated code (Tools/params_utils.py:375)
void OverlayParams::setDockOverlayAlphaRadius(const long &v) {
    instance()->handle->SetInt("DockOverlayAlphaRadius",v);
    instance()->DockOverlayAlphaRadius = v;
}

// Auto generated code (Tools/params_utils.py:384)
void OverlayParams::removeDockOverlayAlphaRadius() {
    instance()->handle->RemoveInt("DockOverlayAlphaRadius");
}

// Auto generated code (Tools/params_utils.py:350)
const char *OverlayParams::docDockOverlayCheckNaviCube() {
    return QT_TRANSLATE_NOOP("OverlayParams",
"Leave space for Navigation Cube in dock overlay");
}

// Auto generated code (Tools/params_utils.py:358)
const bool & OverlayParams::getDockOverlayCheckNaviCube() {
    return instance()->DockOverlayCheckNaviCube;
}

// Auto generated code (Tools/params_utils.py:366)
const bool & OverlayParams::defaultDockOverlayCheckNaviCube() {
    const static bool def = true;
    return def;
}

// Auto generated code (Tools/params_utils.py:375)
void OverlayParams::setDockOverlayCheckNaviCube(const bool &v) {
    instance()->handle->SetBool("DockOverlayCheckNaviCube",v);
    instance()->DockOverlayCheckNaviCube = v;
}

// Auto generated code (Tools/params_utils.py:384)
void OverlayParams::removeDockOverlayCheckNaviCube() {
    instance()->handle->RemoveBool("DockOverlayCheckNaviCube");
}

// Auto generated code (Tools/params_utils.py:350)
const char *OverlayParams::docDockOverlayHintTriggerSize() {
    return QT_TRANSLATE_NOOP("OverlayParams",
"Auto hide hint visual display triggering width");
}

// Auto generated code (Tools/params_utils.py:358)
const long & OverlayParams::getDockOverlayHintTriggerSize() {
    return instance()->DockOverlayHintTriggerSize;
}

// Auto generated code (Tools/params_utils.py:366)
const long & OverlayParams::defaultDockOverlayHintTriggerSize() {
    const static long def = 16;
    return def;
}

// Auto generated code (Tools/params_utils.py:375)
void OverlayParams::setDockOverlayHintTriggerSize(const long &v) {
    instance()->handle->SetInt("DockOverlayHintTriggerSize",v);
    instance()->DockOverlayHintTriggerSize = v;
}

// Auto generated code (Tools/params_utils.py:384)
void OverlayParams::removeDockOverlayHintTriggerSize() {
    instance()->handle->RemoveInt("DockOverlayHintTriggerSize");
}

// Auto generated code (Tools/params_utils.py:350)
const char *OverlayParams::docDockOverlayHintSize() {
    return QT_TRANSLATE_NOOP("OverlayParams",
"Auto hide hint visual display width");
}

// Auto generated code (Tools/params_utils.py:358)
const long & OverlayParams::getDockOverlayHintSize() {
    return instance()->DockOverlayHintSize;
}

// Auto generated code (Tools/params_utils.py:366)
const long & OverlayParams::defaultDockOverlayHintSize() {
    const static long def = 8;
    return def;
}

// Auto generated code (Tools/params_utils.py:375)
void OverlayParams::setDockOverlayHintSize(const long &v) {
    instance()->handle->SetInt("DockOverlayHintSize",v);
    instance()->DockOverlayHintSize = v;
}

// Auto generated code (Tools/params_utils.py:384)
void OverlayParams::removeDockOverlayHintSize() {
    instance()->handle->RemoveInt("DockOverlayHintSize");
}

// Auto generated code (Tools/params_utils.py:350)
const char *OverlayParams::docDockOverlayHintLeftLength() {
    return QT_TRANSLATE_NOOP("OverlayParams",
"Auto hide hint visual display length for left panel. Set to zero to fill the space.");
}

// Auto generated code (Tools/params_utils.py:358)
const long & OverlayParams::getDockOverlayHintLeftLength() {
    return instance()->DockOverlayHintLeftLength;
}

// Auto generated code (Tools/params_utils.py:366)
const long & OverlayParams::defaultDockOverlayHintLeftLength() {
    const static long def = 100;
    return def;
}

// Auto generated code (Tools/params_utils.py:375)
void OverlayParams::setDockOverlayHintLeftLength(const long &v) {
    instance()->handle->SetInt("DockOverlayHintLeftLength",v);
    instance()->DockOverlayHintLeftLength = v;
}

// Auto generated code (Tools/params_utils.py:384)
void OverlayParams::removeDockOverlayHintLeftLength() {
    instance()->handle->RemoveInt("DockOverlayHintLeftLength");
}

// Auto generated code (Tools/params_utils.py:350)
const char *OverlayParams::docDockOverlayHintRightLength() {
    return QT_TRANSLATE_NOOP("OverlayParams",
"Auto hide hint visual display length for right panel. Set to zero to fill the space.");
}

// Auto generated code (Tools/params_utils.py:358)
const long & OverlayParams::getDockOverlayHintRightLength() {
    return instance()->DockOverlayHintRightLength;
}

// Auto generated code (Tools/params_utils.py:366)
const long & OverlayParams::defaultDockOverlayHintRightLength() {
    const static long def = 100;
    return def;
}

// Auto generated code (Tools/params_utils.py:375)
void OverlayParams::setDockOverlayHintRightLength(const long &v) {
    instance()->handle->SetInt("DockOverlayHintRightLength",v);
    instance()->DockOverlayHintRightLength = v;
}

// Auto generated code (Tools/params_utils.py:384)
void OverlayParams::removeDockOverlayHintRightLength() {
    instance()->handle->RemoveInt("DockOverlayHintRightLength");
}

// Auto generated code (Tools/params_utils.py:350)
const char *OverlayParams::docDockOverlayHintTopLength() {
    return QT_TRANSLATE_NOOP("OverlayParams",
"Auto hide hint visual display length for top panel. Set to zero to fill the space.");
}

// Auto generated code (Tools/params_utils.py:358)
const long & OverlayParams::getDockOverlayHintTopLength() {
    return instance()->DockOverlayHintTopLength;
}

// Auto generated code (Tools/params_utils.py:366)
const long & OverlayParams::defaultDockOverlayHintTopLength() {
    const static long def = 100;
    return def;
}

// Auto generated code (Tools/params_utils.py:375)
void OverlayParams::setDockOverlayHintTopLength(const long &v) {
    instance()->handle->SetInt("DockOverlayHintTopLength",v);
    instance()->DockOverlayHintTopLength = v;
}

// Auto generated code (Tools/params_utils.py:384)
void OverlayParams::removeDockOverlayHintTopLength() {
    instance()->handle->RemoveInt("DockOverlayHintTopLength");
}

// Auto generated code (Tools/params_utils.py:350)
const char *OverlayParams::docDockOverlayHintBottomLength() {
    return QT_TRANSLATE_NOOP("OverlayParams",
"Auto hide hint visual display length for bottom panel. Set to zero to fill the space.");
}

// Auto generated code (Tools/params_utils.py:358)
const long & OverlayParams::getDockOverlayHintBottomLength() {
    return instance()->DockOverlayHintBottomLength;
}

// Auto generated code (Tools/params_utils.py:366)
const long & OverlayParams::defaultDockOverlayHintBottomLength() {
    const static long def = 100;
    return def;
}

// Auto generated code (Tools/params_utils.py:375)
void OverlayParams::setDockOverlayHintBottomLength(const long &v) {
    instance()->handle->SetInt("DockOverlayHintBottomLength",v);
    instance()->DockOverlayHintBottomLength = v;
}

// Auto generated code (Tools/params_utils.py:384)
void OverlayParams::removeDockOverlayHintBottomLength() {
    instance()->handle->RemoveInt("DockOverlayHintBottomLength");
}

// Auto generated code (Tools/params_utils.py:350)
const char *OverlayParams::docDockOverlayHintLeftOffset() {
    return QT_TRANSLATE_NOOP("OverlayParams",
"Auto hide hint visual display offset for left panel");
}

// Auto generated code (Tools/params_utils.py:358)
const long & OverlayParams::getDockOverlayHintLeftOffset() {
    return instance()->DockOverlayHintLeftOffset;
}

// Auto generated code (Tools/params_utils.py:366)
const long & OverlayParams::defaultDockOverlayHintLeftOffset() {
    const static long def = 0;
    return def;
}

// Auto generated code (Tools/params_utils.py:375)
void OverlayParams::setDockOverlayHintLeftOffset(const long &v) {
    instance()->handle->SetInt("DockOverlayHintLeftOffset",v);
    instance()->DockOverlayHintLeftOffset = v;
}

// Auto generated code (Tools/params_utils.py:384)
void OverlayParams::removeDockOverlayHintLeftOffset() {
    instance()->handle->RemoveInt("DockOverlayHintLeftOffset");
}

// Auto generated code (Tools/params_utils.py:350)
const char *OverlayParams::docDockOverlayHintRightOffset() {
    return QT_TRANSLATE_NOOP("OverlayParams",
"Auto hide hint visual display offset for right panel");
}

// Auto generated code (Tools/params_utils.py:358)
const long & OverlayParams::getDockOverlayHintRightOffset() {
    return instance()->DockOverlayHintRightOffset;
}

// Auto generated code (Tools/params_utils.py:366)
const long & OverlayParams::defaultDockOverlayHintRightOffset() {
    const static long def = 0;
    return def;
}

// Auto generated code (Tools/params_utils.py:375)
void OverlayParams::setDockOverlayHintRightOffset(const long &v) {
    instance()->handle->SetInt("DockOverlayHintRightOffset",v);
    instance()->DockOverlayHintRightOffset = v;
}

// Auto generated code (Tools/params_utils.py:384)
void OverlayParams::removeDockOverlayHintRightOffset() {
    instance()->handle->RemoveInt("DockOverlayHintRightOffset");
}

// Auto generated code (Tools/params_utils.py:350)
const char *OverlayParams::docDockOverlayHintTopOffset() {
    return QT_TRANSLATE_NOOP("OverlayParams",
"Auto hide hint visual display offset for top panel");
}

// Auto generated code (Tools/params_utils.py:358)
const long & OverlayParams::getDockOverlayHintTopOffset() {
    return instance()->DockOverlayHintTopOffset;
}

// Auto generated code (Tools/params_utils.py:366)
const long & OverlayParams::defaultDockOverlayHintTopOffset() {
    const static long def = 0;
    return def;
}

// Auto generated code (Tools/params_utils.py:375)
void OverlayParams::setDockOverlayHintTopOffset(const long &v) {
    instance()->handle->SetInt("DockOverlayHintTopOffset",v);
    instance()->DockOverlayHintTopOffset = v;
}

// Auto generated code (Tools/params_utils.py:384)
void OverlayParams::removeDockOverlayHintTopOffset() {
    instance()->handle->RemoveInt("DockOverlayHintTopOffset");
}

// Auto generated code (Tools/params_utils.py:350)
const char *OverlayParams::docDockOverlayHintBottomOffset() {
    return QT_TRANSLATE_NOOP("OverlayParams",
"Auto hide hint visual display offset for bottom panel");
}

// Auto generated code (Tools/params_utils.py:358)
const long & OverlayParams::getDockOverlayHintBottomOffset() {
    return instance()->DockOverlayHintBottomOffset;
}

// Auto generated code (Tools/params_utils.py:366)
const long & OverlayParams::defaultDockOverlayHintBottomOffset() {
    const static long def = 0;
    return def;
}

// Auto generated code (Tools/params_utils.py:375)
void OverlayParams::setDockOverlayHintBottomOffset(const long &v) {
    instance()->handle->SetInt("DockOverlayHintBottomOffset",v);
    instance()->DockOverlayHintBottomOffset = v;
}

// Auto generated code (Tools/params_utils.py:384)
void OverlayParams::removeDockOverlayHintBottomOffset() {
    instance()->handle->RemoveInt("DockOverlayHintBottomOffset");
}

// Auto generated code (Tools/params_utils.py:350)
const char *OverlayParams::docDockOverlayHintTabBar() {
    return QT_TRANSLATE_NOOP("OverlayParams",
"Show tab bar on mouse over when auto hide");
}

// Auto generated code (Tools/params_utils.py:358)
const bool & OverlayParams::getDockOverlayHintTabBar() {
    return instance()->DockOverlayHintTabBar;
}

// Auto generated code (Tools/params_utils.py:366)
const bool & OverlayParams::defaultDockOverlayHintTabBar() {
    const static bool def = false;
    return def;
}

// Auto generated code (Tools/params_utils.py:375)
void OverlayParams::setDockOverlayHintTabBar(const bool &v) {
    instance()->handle->SetBool("DockOverlayHintTabBar",v);
    instance()->DockOverlayHintTabBar = v;
}

// Auto generated code (Tools/params_utils.py:384)
void OverlayParams::removeDockOverlayHintTabBar() {
    instance()->handle->RemoveBool("DockOverlayHintTabBar");
}

// Auto generated code (Tools/params_utils.py:350)
const char *OverlayParams::docDockOverlayHideTabBar() {
    return QT_TRANSLATE_NOOP("OverlayParams",
"Hide tab bar in dock overlay");
}

// Auto generated code (Tools/params_utils.py:358)
const bool & OverlayParams::getDockOverlayHideTabBar() {
    return instance()->DockOverlayHideTabBar;
}

// Auto generated code (Tools/params_utils.py:366)
const bool & OverlayParams::defaultDockOverlayHideTabBar() {
    const static bool def = true;
    return def;
}

// Auto generated code (Tools/params_utils.py:375)
void OverlayParams::setDockOverlayHideTabBar(const bool &v) {
    instance()->handle->SetBool("DockOverlayHideTabBar",v);
    instance()->DockOverlayHideTabBar = v;
}

// Auto generated code (Tools/params_utils.py:384)
void OverlayParams::removeDockOverlayHideTabBar() {
    instance()->handle->RemoveBool("DockOverlayHideTabBar");
}

// Auto generated code (Tools/params_utils.py:350)
const char *OverlayParams::docDockOverlayHintDelay() {
    return QT_TRANSLATE_NOOP("OverlayParams",
"Delay before show hint visual");
}

// Auto generated code (Tools/params_utils.py:358)
const long & OverlayParams::getDockOverlayHintDelay() {
    return instance()->DockOverlayHintDelay;
}

// Auto generated code (Tools/params_utils.py:366)
const long & OverlayParams::defaultDockOverlayHintDelay() {
    const static long def = 200;
    return def;
}

// Auto generated code (Tools/params_utils.py:375)
void OverlayParams::setDockOverlayHintDelay(const long &v) {
    instance()->handle->SetInt("DockOverlayHintDelay",v);
    instance()->DockOverlayHintDelay = v;
}

// Auto generated code (Tools/params_utils.py:384)
void OverlayParams::removeDockOverlayHintDelay() {
    instance()->handle->RemoveInt("DockOverlayHintDelay");
}

// Auto generated code (Tools/params_utils.py:350)
const char *OverlayParams::docDockOverlayAnimationDuration() {
    return QT_TRANSLATE_NOOP("OverlayParams",
"Auto hide animation duration, 0 to disable");
}

// Auto generated code (Tools/params_utils.py:358)
const long & OverlayParams::getDockOverlayAnimationDuration() {
    return instance()->DockOverlayAnimationDuration;
}

// Auto generated code (Tools/params_utils.py:366)
const long & OverlayParams::defaultDockOverlayAnimationDuration() {
    const static long def = 200;
    return def;
}

// Auto generated code (Tools/params_utils.py:375)
void OverlayParams::setDockOverlayAnimationDuration(const long &v) {
    instance()->handle->SetInt("DockOverlayAnimationDuration",v);
    instance()->DockOverlayAnimationDuration = v;
}

// Auto generated code (Tools/params_utils.py:384)
void OverlayParams::removeDockOverlayAnimationDuration() {
    instance()->handle->RemoveInt("DockOverlayAnimationDuration");
}

// Auto generated code (Tools/params_utils.py:350)
const char *OverlayParams::docDockOverlayAnimationCurve() {
    return QT_TRANSLATE_NOOP("OverlayParams",
"Auto hide animation curve type");
}

// Auto generated code (Tools/params_utils.py:358)
const long & OverlayParams::getDockOverlayAnimationCurve() {
    return instance()->DockOverlayAnimationCurve;
}

// Auto generated code (Tools/params_utils.py:366)
const long & OverlayParams::defaultDockOverlayAnimationCurve() {
    const static long def = 7;
    return def;
}

// Auto generated code (Tools/params_utils.py:375)
void OverlayParams::setDockOverlayAnimationCurve(const long &v) {
    instance()->handle->SetInt("DockOverlayAnimationCurve",v);
    instance()->DockOverlayAnimationCurve = v;
}

// Auto generated code (Tools/params_utils.py:384)
void OverlayParams::removeDockOverlayAnimationCurve() {
    instance()->handle->RemoveInt("DockOverlayAnimationCurve");
}

// Auto generated code (Tools/params_utils.py:350)
const char *OverlayParams::docDockOverlayHidePropertyViewScrollBar() {
    return QT_TRANSLATE_NOOP("OverlayParams",
"Hide property view scroll bar in dock overlay");
}

// Auto generated code (Tools/params_utils.py:358)
const bool & OverlayParams::getDockOverlayHidePropertyViewScrollBar() {
    return instance()->DockOverlayHidePropertyViewScrollBar;
}

// Auto generated code (Tools/params_utils.py:366)
const bool & OverlayParams::defaultDockOverlayHidePropertyViewScrollBar() {
    const static bool def = false;
    return def;
}

// Auto generated code (Tools/params_utils.py:375)
void OverlayParams::setDockOverlayHidePropertyViewScrollBar(const bool &v) {
    instance()->handle->SetBool("DockOverlayHidePropertyViewScrollBar",v);
    instance()->DockOverlayHidePropertyViewScrollBar = v;
}

// Auto generated code (Tools/params_utils.py:384)
void OverlayParams::removeDockOverlayHidePropertyViewScrollBar() {
    instance()->handle->RemoveBool("DockOverlayHidePropertyViewScrollBar");
}

// Auto generated code (Tools/params_utils.py:350)
const char *OverlayParams::docDockOverlayMinimumSize() {
    return QT_TRANSLATE_NOOP("OverlayParams",
"Minimum overlay dock widget width/height");
}

// Auto generated code (Tools/params_utils.py:358)
const long & OverlayParams::getDockOverlayMinimumSize() {
    return instance()->DockOverlayMinimumSize;
}

// Auto generated code (Tools/params_utils.py:366)
const long & OverlayParams::defaultDockOverlayMinimumSize() {
    const static long def = 30;
    return def;
}

// Auto generated code (Tools/params_utils.py:375)
void OverlayParams::setDockOverlayMinimumSize(const long &v) {
    instance()->handle->SetInt("DockOverlayMinimumSize",v);
    instance()->DockOverlayMinimumSize = v;
}

// Auto generated code (Tools/params_utils.py:384)
void OverlayParams::removeDockOverlayMinimumSize() {
    instance()->handle->RemoveInt("DockOverlayMinimumSize");
}

// Auto generated code (Gui/OverlayParams.py:171)
const std::vector<QString> OverlayParams::AnimationCurveTypes = {
    QStringLiteral("Linear"),
    QStringLiteral("InQuad"),
    QStringLiteral("OutQuad"),
    QStringLiteral("InOutQuad"),
    QStringLiteral("OutInQuad"),
    QStringLiteral("InCubic"),
    QStringLiteral("OutCubic"),
    QStringLiteral("InOutCubic"),
    QStringLiteral("OutInCubic"),
    QStringLiteral("InQuart"),
    QStringLiteral("OutQuart"),
    QStringLiteral("InOutQuart"),
    QStringLiteral("OutInQuart"),
    QStringLiteral("InQuint"),
    QStringLiteral("OutQuint"),
    QStringLiteral("InOutQuint"),
    QStringLiteral("OutInQuint"),
    QStringLiteral("InSine"),
    QStringLiteral("OutSine"),
    QStringLiteral("InOutSine"),
    QStringLiteral("OutInSine"),
    QStringLiteral("InExpo"),
    QStringLiteral("OutExpo"),
    QStringLiteral("InOutExpo"),
    QStringLiteral("OutInExpo"),
    QStringLiteral("InCirc"),
    QStringLiteral("OutCirc"),
    QStringLiteral("InOutCirc"),
    QStringLiteral("OutInCirc"),
    QStringLiteral("InElastic"),
    QStringLiteral("OutElastic"),
    QStringLiteral("InOutElastic"),
    QStringLiteral("OutInElastic"),
    QStringLiteral("InBack"),
    QStringLiteral("OutBack"),
    QStringLiteral("InOutBack"),
    QStringLiteral("OutInBack"),
    QStringLiteral("InBounce"),
    QStringLiteral("OutBounce"),
    QStringLiteral("InOutBounce"),
    QStringLiteral("OutInBounce"),
};
//[[[end]]]

void OverlayParams::onDockOverlayAutoViewChanged() {
    OverlayManager::instance()->refresh();
}

void OverlayParams::onDockOverlayCheckNaviCubeChanged() {
    OverlayManager::instance()->refresh();
}

void OverlayParams::onDockOverlayHideTabBarChanged() {
    OverlayManager::instance()->refresh(nullptr, true);
}

void OverlayParams::onDockOverlayMinimumSizeChanged() {
    OverlayManager::instance()->refresh();
}

