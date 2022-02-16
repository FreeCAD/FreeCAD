/****************************************************************************
 *   Copyright (c) 2018 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
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
#ifndef _PreComp_
# include <QFont>
# include <QApplication>
# include <QEvent>
# include <QWidget>
#endif

/*[[[cog
import ViewParams
ViewParams.define()
]]]*/

// Auto generated code. See class document of ViewParams.
#include <unordered_map>
#include <App/Application.h>
#include <App/DynamicProperty.h>
#include "ViewParams.h"
using namespace Gui;

namespace {

// Auto generated code. See class document of ViewParams.
class ViewParamsP: public ParameterGrp::ObserverType {
public:
    // Auto generated code. See class document of ViewParams.
    ParameterGrp::handle handle;

    // Auto generated code. See class document of ViewParams.
    std::unordered_map<const char *,void(*)(ViewParamsP*),App::CStringHasher,App::CStringHasher> funcs;

    bool UseNewSelection; // Auto generated code. See class document of ViewParams.
    bool UseSelectionRoot; // Auto generated code. See class document of ViewParams.
    bool EnableSelection; // Auto generated code. See class document of ViewParams.
    long RenderCache; // Auto generated code. See class document of ViewParams.
    bool RandomColor; // Auto generated code. See class document of ViewParams.
    unsigned long BoundingBoxColor; // Auto generated code. See class document of ViewParams.
    unsigned long AnnotationTextColor; // Auto generated code. See class document of ViewParams.
    long MarkerSize; // Auto generated code. See class document of ViewParams.
    unsigned long DefaultLinkColor; // Auto generated code. See class document of ViewParams.
    unsigned long DefaultShapeLineColor; // Auto generated code. See class document of ViewParams.
    unsigned long DefaultShapeVertexColor; // Auto generated code. See class document of ViewParams.
    unsigned long DefaultShapeColor; // Auto generated code. See class document of ViewParams.
    long DefaultShapeLineWidth; // Auto generated code. See class document of ViewParams.
    long DefaultShapePointSize; // Auto generated code. See class document of ViewParams.
    bool CoinCycleCheck; // Auto generated code. See class document of ViewParams.
    bool EnablePropertyViewForInactiveDocument; // Auto generated code. See class document of ViewParams.
    bool ShowSelectionBoundingBox; // Auto generated code. See class document of ViewParams.
    unsigned long PropertyViewTimer; // Auto generated code. See class document of ViewParams.
    long DefaultFontSize; // Auto generated code. See class document of ViewParams.

    // Auto generated code. See class document of ViewParams.
    ViewParamsP() {
        handle = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
        handle->Attach(this);

        UseNewSelection = handle->GetBool("UseNewSelection", true);
        funcs["UseNewSelection"] = &ViewParamsP::updateUseNewSelection;
        UseSelectionRoot = handle->GetBool("UseSelectionRoot", true);
        funcs["UseSelectionRoot"] = &ViewParamsP::updateUseSelectionRoot;
        EnableSelection = handle->GetBool("EnableSelection", true);
        funcs["EnableSelection"] = &ViewParamsP::updateEnableSelection;
        RenderCache = handle->GetInt("RenderCache", 0);
        funcs["RenderCache"] = &ViewParamsP::updateRenderCache;
        RandomColor = handle->GetBool("RandomColor", false);
        funcs["RandomColor"] = &ViewParamsP::updateRandomColor;
        BoundingBoxColor = handle->GetUnsigned("BoundingBoxColor", 4294967295);
        funcs["BoundingBoxColor"] = &ViewParamsP::updateBoundingBoxColor;
        AnnotationTextColor = handle->GetUnsigned("AnnotationTextColor", 4294967295);
        funcs["AnnotationTextColor"] = &ViewParamsP::updateAnnotationTextColor;
        MarkerSize = handle->GetInt("MarkerSize", 9);
        funcs["MarkerSize"] = &ViewParamsP::updateMarkerSize;
        DefaultLinkColor = handle->GetUnsigned("DefaultLinkColor", 1728053247);
        funcs["DefaultLinkColor"] = &ViewParamsP::updateDefaultLinkColor;
        DefaultShapeLineColor = handle->GetUnsigned("DefaultShapeLineColor", 421075455);
        funcs["DefaultShapeLineColor"] = &ViewParamsP::updateDefaultShapeLineColor;
        DefaultShapeVertexColor = handle->GetUnsigned("DefaultShapeVertexColor", 421075455);
        funcs["DefaultShapeVertexColor"] = &ViewParamsP::updateDefaultShapeVertexColor;
        DefaultShapeColor = handle->GetUnsigned("DefaultShapeColor", 3435973887);
        funcs["DefaultShapeColor"] = &ViewParamsP::updateDefaultShapeColor;
        DefaultShapeLineWidth = handle->GetInt("DefaultShapeLineWidth", 2);
        funcs["DefaultShapeLineWidth"] = &ViewParamsP::updateDefaultShapeLineWidth;
        DefaultShapePointSize = handle->GetInt("DefaultShapePointSize", 2);
        funcs["DefaultShapePointSize"] = &ViewParamsP::updateDefaultShapePointSize;
        CoinCycleCheck = handle->GetBool("CoinCycleCheck", true);
        funcs["CoinCycleCheck"] = &ViewParamsP::updateCoinCycleCheck;
        EnablePropertyViewForInactiveDocument = handle->GetBool("EnablePropertyViewForInactiveDocument", true);
        funcs["EnablePropertyViewForInactiveDocument"] = &ViewParamsP::updateEnablePropertyViewForInactiveDocument;
        ShowSelectionBoundingBox = handle->GetBool("ShowSelectionBoundingBox", false);
        funcs["ShowSelectionBoundingBox"] = &ViewParamsP::updateShowSelectionBoundingBox;
        PropertyViewTimer = handle->GetUnsigned("PropertyViewTimer", 100);
        funcs["PropertyViewTimer"] = &ViewParamsP::updatePropertyViewTimer;
        DefaultFontSize = handle->GetInt("DefaultFontSize", 0);
        funcs["DefaultFontSize"] = &ViewParamsP::updateDefaultFontSize;
    }

    // Auto generated code. See class document of ViewParams.
    ~ViewParamsP() {
    }

    // Auto generated code. See class document of ViewParams.
    void OnChange(Base::Subject<const char*> &, const char* sReason) {
        if(!sReason)
            return;
        auto it = funcs.find(sReason);
        if(it == funcs.end())
            return;
        it->second(this);
    }


    // Auto generated code. See class document of ViewParams.
    static void updateUseNewSelection(ViewParamsP *self) {
        self->UseNewSelection = self->handle->GetBool("UseNewSelection", true);
    }
    // Auto generated code. See class document of ViewParams.
    static void updateUseSelectionRoot(ViewParamsP *self) {
        self->UseSelectionRoot = self->handle->GetBool("UseSelectionRoot", true);
    }
    // Auto generated code. See class document of ViewParams.
    static void updateEnableSelection(ViewParamsP *self) {
        self->EnableSelection = self->handle->GetBool("EnableSelection", true);
    }
    // Auto generated code. See class document of ViewParams.
    static void updateRenderCache(ViewParamsP *self) {
        self->RenderCache = self->handle->GetInt("RenderCache", 0);
    }
    // Auto generated code. See class document of ViewParams.
    static void updateRandomColor(ViewParamsP *self) {
        self->RandomColor = self->handle->GetBool("RandomColor", false);
    }
    // Auto generated code. See class document of ViewParams.
    static void updateBoundingBoxColor(ViewParamsP *self) {
        self->BoundingBoxColor = self->handle->GetUnsigned("BoundingBoxColor", 4294967295);
    }
    // Auto generated code. See class document of ViewParams.
    static void updateAnnotationTextColor(ViewParamsP *self) {
        self->AnnotationTextColor = self->handle->GetUnsigned("AnnotationTextColor", 4294967295);
    }
    // Auto generated code. See class document of ViewParams.
    static void updateMarkerSize(ViewParamsP *self) {
        self->MarkerSize = self->handle->GetInt("MarkerSize", 9);
    }
    // Auto generated code. See class document of ViewParams.
    static void updateDefaultLinkColor(ViewParamsP *self) {
        self->DefaultLinkColor = self->handle->GetUnsigned("DefaultLinkColor", 1728053247);
    }
    // Auto generated code. See class document of ViewParams.
    static void updateDefaultShapeLineColor(ViewParamsP *self) {
        self->DefaultShapeLineColor = self->handle->GetUnsigned("DefaultShapeLineColor", 421075455);
    }
    // Auto generated code. See class document of ViewParams.
    static void updateDefaultShapeVertexColor(ViewParamsP *self) {
        self->DefaultShapeVertexColor = self->handle->GetUnsigned("DefaultShapeVertexColor", 421075455);
    }
    // Auto generated code. See class document of ViewParams.
    static void updateDefaultShapeColor(ViewParamsP *self) {
        self->DefaultShapeColor = self->handle->GetUnsigned("DefaultShapeColor", 3435973887);
    }
    // Auto generated code. See class document of ViewParams.
    static void updateDefaultShapeLineWidth(ViewParamsP *self) {
        self->DefaultShapeLineWidth = self->handle->GetInt("DefaultShapeLineWidth", 2);
    }
    // Auto generated code. See class document of ViewParams.
    static void updateDefaultShapePointSize(ViewParamsP *self) {
        self->DefaultShapePointSize = self->handle->GetInt("DefaultShapePointSize", 2);
    }
    // Auto generated code. See class document of ViewParams.
    static void updateCoinCycleCheck(ViewParamsP *self) {
        self->CoinCycleCheck = self->handle->GetBool("CoinCycleCheck", true);
    }
    // Auto generated code. See class document of ViewParams.
    static void updateEnablePropertyViewForInactiveDocument(ViewParamsP *self) {
        self->EnablePropertyViewForInactiveDocument = self->handle->GetBool("EnablePropertyViewForInactiveDocument", true);
    }
    // Auto generated code. See class document of ViewParams.
    static void updateShowSelectionBoundingBox(ViewParamsP *self) {
        self->ShowSelectionBoundingBox = self->handle->GetBool("ShowSelectionBoundingBox", false);
    }
    // Auto generated code. See class document of ViewParams.
    static void updatePropertyViewTimer(ViewParamsP *self) {
        self->PropertyViewTimer = self->handle->GetUnsigned("PropertyViewTimer", 100);
    }
    // Auto generated code. See class document of ViewParams.
    static void updateDefaultFontSize(ViewParamsP *self) {
        auto v = self->handle->GetInt("DefaultFontSize", 0);
        if (self->DefaultFontSize != v) {
            self->DefaultFontSize = v;
            ViewParams::onDefaultFontSizeChanged();
        }
    }
};

// Auto generated code. See class document of ViewParams.
ViewParamsP *instance() {
    static ViewParamsP *inst = new ViewParamsP;
    return inst;
}

} // Anonymous namespace

// Auto generated code. See class document of ViewParams.
ParameterGrp::handle ViewParams::getHandle() {
    return instance()->handle;
}

// Auto generated code. See class document of ViewParams.
const char *ViewParams::docUseNewSelection() {
    return "";
}

// Auto generated code. See class document of ViewParams.
const bool & ViewParams::getUseNewSelection() {
    return instance()->UseNewSelection;
}

// Auto generated code. See class document of ViewParams.
const bool & ViewParams::defaultUseNewSelection() {
    const static bool def = true;
    return def;
}

// Auto generated code. See class document of ViewParams.
void ViewParams::setUseNewSelection(const bool &v) {
    instance()->handle->SetBool("UseNewSelection",v);
    instance()->UseNewSelection = v;
}

// Auto generated code. See class document of ViewParams.
void ViewParams::removeUseNewSelection() {
    instance()->handle->RemoveBool("UseNewSelection");
}

// Auto generated code. See class document of ViewParams.
const char *ViewParams::docUseSelectionRoot() {
    return "";
}

// Auto generated code. See class document of ViewParams.
const bool & ViewParams::getUseSelectionRoot() {
    return instance()->UseSelectionRoot;
}

// Auto generated code. See class document of ViewParams.
const bool & ViewParams::defaultUseSelectionRoot() {
    const static bool def = true;
    return def;
}

// Auto generated code. See class document of ViewParams.
void ViewParams::setUseSelectionRoot(const bool &v) {
    instance()->handle->SetBool("UseSelectionRoot",v);
    instance()->UseSelectionRoot = v;
}

// Auto generated code. See class document of ViewParams.
void ViewParams::removeUseSelectionRoot() {
    instance()->handle->RemoveBool("UseSelectionRoot");
}

// Auto generated code. See class document of ViewParams.
const char *ViewParams::docEnableSelection() {
    return "";
}

// Auto generated code. See class document of ViewParams.
const bool & ViewParams::getEnableSelection() {
    return instance()->EnableSelection;
}

// Auto generated code. See class document of ViewParams.
const bool & ViewParams::defaultEnableSelection() {
    const static bool def = true;
    return def;
}

// Auto generated code. See class document of ViewParams.
void ViewParams::setEnableSelection(const bool &v) {
    instance()->handle->SetBool("EnableSelection",v);
    instance()->EnableSelection = v;
}

// Auto generated code. See class document of ViewParams.
void ViewParams::removeEnableSelection() {
    instance()->handle->RemoveBool("EnableSelection");
}

// Auto generated code. See class document of ViewParams.
const char *ViewParams::docRenderCache() {
    return "";
}

// Auto generated code. See class document of ViewParams.
const long & ViewParams::getRenderCache() {
    return instance()->RenderCache;
}

// Auto generated code. See class document of ViewParams.
const long & ViewParams::defaultRenderCache() {
    const static long def = 0;
    return def;
}

// Auto generated code. See class document of ViewParams.
void ViewParams::setRenderCache(const long &v) {
    instance()->handle->SetInt("RenderCache",v);
    instance()->RenderCache = v;
}

// Auto generated code. See class document of ViewParams.
void ViewParams::removeRenderCache() {
    instance()->handle->RemoveInt("RenderCache");
}

// Auto generated code. See class document of ViewParams.
const char *ViewParams::docRandomColor() {
    return "";
}

// Auto generated code. See class document of ViewParams.
const bool & ViewParams::getRandomColor() {
    return instance()->RandomColor;
}

// Auto generated code. See class document of ViewParams.
const bool & ViewParams::defaultRandomColor() {
    const static bool def = false;
    return def;
}

// Auto generated code. See class document of ViewParams.
void ViewParams::setRandomColor(const bool &v) {
    instance()->handle->SetBool("RandomColor",v);
    instance()->RandomColor = v;
}

// Auto generated code. See class document of ViewParams.
void ViewParams::removeRandomColor() {
    instance()->handle->RemoveBool("RandomColor");
}

// Auto generated code. See class document of ViewParams.
const char *ViewParams::docBoundingBoxColor() {
    return "";
}

// Auto generated code. See class document of ViewParams.
const unsigned long & ViewParams::getBoundingBoxColor() {
    return instance()->BoundingBoxColor;
}

// Auto generated code. See class document of ViewParams.
const unsigned long & ViewParams::defaultBoundingBoxColor() {
    const static unsigned long def = 4294967295;
    return def;
}

// Auto generated code. See class document of ViewParams.
void ViewParams::setBoundingBoxColor(const unsigned long &v) {
    instance()->handle->SetUnsigned("BoundingBoxColor",v);
    instance()->BoundingBoxColor = v;
}

// Auto generated code. See class document of ViewParams.
void ViewParams::removeBoundingBoxColor() {
    instance()->handle->RemoveUnsigned("BoundingBoxColor");
}

// Auto generated code. See class document of ViewParams.
const char *ViewParams::docAnnotationTextColor() {
    return "";
}

// Auto generated code. See class document of ViewParams.
const unsigned long & ViewParams::getAnnotationTextColor() {
    return instance()->AnnotationTextColor;
}

// Auto generated code. See class document of ViewParams.
const unsigned long & ViewParams::defaultAnnotationTextColor() {
    const static unsigned long def = 4294967295;
    return def;
}

// Auto generated code. See class document of ViewParams.
void ViewParams::setAnnotationTextColor(const unsigned long &v) {
    instance()->handle->SetUnsigned("AnnotationTextColor",v);
    instance()->AnnotationTextColor = v;
}

// Auto generated code. See class document of ViewParams.
void ViewParams::removeAnnotationTextColor() {
    instance()->handle->RemoveUnsigned("AnnotationTextColor");
}

// Auto generated code. See class document of ViewParams.
const char *ViewParams::docMarkerSize() {
    return "";
}

// Auto generated code. See class document of ViewParams.
const long & ViewParams::getMarkerSize() {
    return instance()->MarkerSize;
}

// Auto generated code. See class document of ViewParams.
const long & ViewParams::defaultMarkerSize() {
    const static long def = 9;
    return def;
}

// Auto generated code. See class document of ViewParams.
void ViewParams::setMarkerSize(const long &v) {
    instance()->handle->SetInt("MarkerSize",v);
    instance()->MarkerSize = v;
}

// Auto generated code. See class document of ViewParams.
void ViewParams::removeMarkerSize() {
    instance()->handle->RemoveInt("MarkerSize");
}

// Auto generated code. See class document of ViewParams.
const char *ViewParams::docDefaultLinkColor() {
    return "";
}

// Auto generated code. See class document of ViewParams.
const unsigned long & ViewParams::getDefaultLinkColor() {
    return instance()->DefaultLinkColor;
}

// Auto generated code. See class document of ViewParams.
const unsigned long & ViewParams::defaultDefaultLinkColor() {
    const static unsigned long def = 1728053247;
    return def;
}

// Auto generated code. See class document of ViewParams.
void ViewParams::setDefaultLinkColor(const unsigned long &v) {
    instance()->handle->SetUnsigned("DefaultLinkColor",v);
    instance()->DefaultLinkColor = v;
}

// Auto generated code. See class document of ViewParams.
void ViewParams::removeDefaultLinkColor() {
    instance()->handle->RemoveUnsigned("DefaultLinkColor");
}

// Auto generated code. See class document of ViewParams.
const char *ViewParams::docDefaultShapeLineColor() {
    return "";
}

// Auto generated code. See class document of ViewParams.
const unsigned long & ViewParams::getDefaultShapeLineColor() {
    return instance()->DefaultShapeLineColor;
}

// Auto generated code. See class document of ViewParams.
const unsigned long & ViewParams::defaultDefaultShapeLineColor() {
    const static unsigned long def = 421075455;
    return def;
}

// Auto generated code. See class document of ViewParams.
void ViewParams::setDefaultShapeLineColor(const unsigned long &v) {
    instance()->handle->SetUnsigned("DefaultShapeLineColor",v);
    instance()->DefaultShapeLineColor = v;
}

// Auto generated code. See class document of ViewParams.
void ViewParams::removeDefaultShapeLineColor() {
    instance()->handle->RemoveUnsigned("DefaultShapeLineColor");
}

// Auto generated code. See class document of ViewParams.
const char *ViewParams::docDefaultShapeVertexColor() {
    return "";
}

// Auto generated code. See class document of ViewParams.
const unsigned long & ViewParams::getDefaultShapeVertexColor() {
    return instance()->DefaultShapeVertexColor;
}

// Auto generated code. See class document of ViewParams.
const unsigned long & ViewParams::defaultDefaultShapeVertexColor() {
    const static unsigned long def = 421075455;
    return def;
}

// Auto generated code. See class document of ViewParams.
void ViewParams::setDefaultShapeVertexColor(const unsigned long &v) {
    instance()->handle->SetUnsigned("DefaultShapeVertexColor",v);
    instance()->DefaultShapeVertexColor = v;
}

// Auto generated code. See class document of ViewParams.
void ViewParams::removeDefaultShapeVertexColor() {
    instance()->handle->RemoveUnsigned("DefaultShapeVertexColor");
}

// Auto generated code. See class document of ViewParams.
const char *ViewParams::docDefaultShapeColor() {
    return "";
}

// Auto generated code. See class document of ViewParams.
const unsigned long & ViewParams::getDefaultShapeColor() {
    return instance()->DefaultShapeColor;
}

// Auto generated code. See class document of ViewParams.
const unsigned long & ViewParams::defaultDefaultShapeColor() {
    const static unsigned long def = 3435973887;
    return def;
}

// Auto generated code. See class document of ViewParams.
void ViewParams::setDefaultShapeColor(const unsigned long &v) {
    instance()->handle->SetUnsigned("DefaultShapeColor",v);
    instance()->DefaultShapeColor = v;
}

// Auto generated code. See class document of ViewParams.
void ViewParams::removeDefaultShapeColor() {
    instance()->handle->RemoveUnsigned("DefaultShapeColor");
}

// Auto generated code. See class document of ViewParams.
const char *ViewParams::docDefaultShapeLineWidth() {
    return "";
}

// Auto generated code. See class document of ViewParams.
const long & ViewParams::getDefaultShapeLineWidth() {
    return instance()->DefaultShapeLineWidth;
}

// Auto generated code. See class document of ViewParams.
const long & ViewParams::defaultDefaultShapeLineWidth() {
    const static long def = 2;
    return def;
}

// Auto generated code. See class document of ViewParams.
void ViewParams::setDefaultShapeLineWidth(const long &v) {
    instance()->handle->SetInt("DefaultShapeLineWidth",v);
    instance()->DefaultShapeLineWidth = v;
}

// Auto generated code. See class document of ViewParams.
void ViewParams::removeDefaultShapeLineWidth() {
    instance()->handle->RemoveInt("DefaultShapeLineWidth");
}

// Auto generated code. See class document of ViewParams.
const char *ViewParams::docDefaultShapePointSize() {
    return "";
}

// Auto generated code. See class document of ViewParams.
const long & ViewParams::getDefaultShapePointSize() {
    return instance()->DefaultShapePointSize;
}

// Auto generated code. See class document of ViewParams.
const long & ViewParams::defaultDefaultShapePointSize() {
    const static long def = 2;
    return def;
}

// Auto generated code. See class document of ViewParams.
void ViewParams::setDefaultShapePointSize(const long &v) {
    instance()->handle->SetInt("DefaultShapePointSize",v);
    instance()->DefaultShapePointSize = v;
}

// Auto generated code. See class document of ViewParams.
void ViewParams::removeDefaultShapePointSize() {
    instance()->handle->RemoveInt("DefaultShapePointSize");
}

// Auto generated code. See class document of ViewParams.
const char *ViewParams::docCoinCycleCheck() {
    return "";
}

// Auto generated code. See class document of ViewParams.
const bool & ViewParams::getCoinCycleCheck() {
    return instance()->CoinCycleCheck;
}

// Auto generated code. See class document of ViewParams.
const bool & ViewParams::defaultCoinCycleCheck() {
    const static bool def = true;
    return def;
}

// Auto generated code. See class document of ViewParams.
void ViewParams::setCoinCycleCheck(const bool &v) {
    instance()->handle->SetBool("CoinCycleCheck",v);
    instance()->CoinCycleCheck = v;
}

// Auto generated code. See class document of ViewParams.
void ViewParams::removeCoinCycleCheck() {
    instance()->handle->RemoveBool("CoinCycleCheck");
}

// Auto generated code. See class document of ViewParams.
const char *ViewParams::docEnablePropertyViewForInactiveDocument() {
    return "";
}

// Auto generated code. See class document of ViewParams.
const bool & ViewParams::getEnablePropertyViewForInactiveDocument() {
    return instance()->EnablePropertyViewForInactiveDocument;
}

// Auto generated code. See class document of ViewParams.
const bool & ViewParams::defaultEnablePropertyViewForInactiveDocument() {
    const static bool def = true;
    return def;
}

// Auto generated code. See class document of ViewParams.
void ViewParams::setEnablePropertyViewForInactiveDocument(const bool &v) {
    instance()->handle->SetBool("EnablePropertyViewForInactiveDocument",v);
    instance()->EnablePropertyViewForInactiveDocument = v;
}

// Auto generated code. See class document of ViewParams.
void ViewParams::removeEnablePropertyViewForInactiveDocument() {
    instance()->handle->RemoveBool("EnablePropertyViewForInactiveDocument");
}

// Auto generated code. See class document of ViewParams.
const char *ViewParams::docShowSelectionBoundingBox() {
    return QT_TRANSLATE_NOOP("ViewParams",
"Show bounding box when object is selectedin 3D view");
}

// Auto generated code. See class document of ViewParams.
const bool & ViewParams::getShowSelectionBoundingBox() {
    return instance()->ShowSelectionBoundingBox;
}

// Auto generated code. See class document of ViewParams.
const bool & ViewParams::defaultShowSelectionBoundingBox() {
    const static bool def = false;
    return def;
}

// Auto generated code. See class document of ViewParams.
void ViewParams::setShowSelectionBoundingBox(const bool &v) {
    instance()->handle->SetBool("ShowSelectionBoundingBox",v);
    instance()->ShowSelectionBoundingBox = v;
}

// Auto generated code. See class document of ViewParams.
void ViewParams::removeShowSelectionBoundingBox() {
    instance()->handle->RemoveBool("ShowSelectionBoundingBox");
}

// Auto generated code. See class document of ViewParams.
const char *ViewParams::docPropertyViewTimer() {
    return "";
}

// Auto generated code. See class document of ViewParams.
const unsigned long & ViewParams::getPropertyViewTimer() {
    return instance()->PropertyViewTimer;
}

// Auto generated code. See class document of ViewParams.
const unsigned long & ViewParams::defaultPropertyViewTimer() {
    const static unsigned long def = 100;
    return def;
}

// Auto generated code. See class document of ViewParams.
void ViewParams::setPropertyViewTimer(const unsigned long &v) {
    instance()->handle->SetUnsigned("PropertyViewTimer",v);
    instance()->PropertyViewTimer = v;
}

// Auto generated code. See class document of ViewParams.
void ViewParams::removePropertyViewTimer() {
    instance()->handle->RemoveUnsigned("PropertyViewTimer");
}

// Auto generated code. See class document of ViewParams.
const char *ViewParams::docDefaultFontSize() {
    return "";
}

// Auto generated code. See class document of ViewParams.
const long & ViewParams::getDefaultFontSize() {
    return instance()->DefaultFontSize;
}

// Auto generated code. See class document of ViewParams.
const long & ViewParams::defaultDefaultFontSize() {
    const static long def = 0;
    return def;
}

// Auto generated code. See class document of ViewParams.
void ViewParams::setDefaultFontSize(const long &v) {
    instance()->handle->SetInt("DefaultFontSize",v);
    instance()->DefaultFontSize = v;
}

// Auto generated code. See class document of ViewParams.
void ViewParams::removeDefaultFontSize() {
    instance()->handle->RemoveInt("DefaultFontSize");
}
//[[[end]]]

//-- Start of non auto generated code --//

void ViewParams::init() {
    onDefaultFontSizeChanged();
}

int ViewParams::appDefaultFontSize() {
    static int defaultSize;
    if (!defaultSize) {
        QFont font;
        defaultSize = font.pointSize();
    }
    return defaultSize;
}

void ViewParams::onDefaultFontSizeChanged() {
    int defaultSize = appDefaultFontSize();
    int fontSize = getDefaultFontSize();
    if (fontSize <= 0)
        fontSize = defaultSize;
    else if (fontSize < 8)
        fontSize = 8;
    QFont font = QApplication::font();
    if (font.pointSize() != fontSize) {
        font.setPointSize(fontSize);
        QApplication::setFont(font);
        QEvent e(QEvent::ApplicationFontChange);
        for (auto w : QApplication::allWidgets())
            QApplication::sendEvent(w, &e);
    }
}
