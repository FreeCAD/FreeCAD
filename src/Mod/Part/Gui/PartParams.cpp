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
#include <QTimer>
#include <App/Application.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include "ViewProvider.h"

namespace {
QTimer &getTimer() {
    static QTimer *timer;
    if (!timer) {
        timer = new QTimer();
        timer->setSingleShot(true);
        QObject::connect(timer, &QTimer::timeout, [](){
            // search for Part view providers and apply the new settings
            for (auto doc : App::GetApplication().getDocuments()) {
                auto gdoc = Gui::Application::Instance->getDocument(doc);
                for (auto vp : gdoc->getViewProvidersOfType(
                            PartGui::ViewProviderPart::getClassTypeId()))
                    static_cast<PartGui::ViewProviderPart*>(vp)->reload();
            }
        });
    }
    return *timer;
}
} // anonymous namespace

/*[[[cog
import PartGuiParams
PartGuiParams.define()
]]]*/

// Auto generated code (Tools/params_utils.py:166)
#include <unordered_map>
#include <App/Application.h>
#include <App/DynamicProperty.h>
#include "PartParams.h"
using namespace PartGui;

// Auto generated code (Tools/params_utils.py:175)
namespace {
class PartParamsP: public ParameterGrp::ObserverType {
public:
    ParameterGrp::handle handle;
    std::unordered_map<const char *,void(*)(PartParamsP*),App::CStringHasher,App::CStringHasher> funcs;

    bool NormalsFromUVNodes;
    bool TwoSideRendering;
    double MinimumDeviation;
    double MeshDeviation;
    double MeshAngularDeflection;
    double MinimumAngularDeflection;
    bool OverrideTessellation;
    bool MapFaceColor;
    bool MapLineColor;
    bool MapPointColor;
    bool MapTransparency;
    bool AutoGridScale;
    unsigned long PreviewAddColor;
    unsigned long PreviewSubColor;
    unsigned long PreviewDressColor;
    unsigned long PreviewIntersectColor;
    bool PreviewOnEdit;
    bool PreviewWithTransparency;
    bool EditOnTop;
    long EditRecomputeWait;
    bool AdjustCameraForNewFeature;
    unsigned long DefaultDatumColor;
    bool RespectSystemDPI;
    long SelectionPickThreshold;
    long SelectionPickThreshold2;
    bool SelectionPickRTree;

    // Auto generated code (Tools/params_utils.py:203)
    PartParamsP() {
        handle = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Part");
        handle->Attach(this);

        NormalsFromUVNodes = handle->GetBool("NormalsFromUVNodes", true);
        funcs["NormalsFromUVNodes"] = &PartParamsP::updateNormalsFromUVNodes;
        TwoSideRendering = handle->GetBool("TwoSideRendering", true);
        funcs["TwoSideRendering"] = &PartParamsP::updateTwoSideRendering;
        MinimumDeviation = handle->GetFloat("MinimumDeviation", 0.05);
        funcs["MinimumDeviation"] = &PartParamsP::updateMinimumDeviation;
        MeshDeviation = handle->GetFloat("MeshDeviation", 0.2);
        funcs["MeshDeviation"] = &PartParamsP::updateMeshDeviation;
        MeshAngularDeflection = handle->GetFloat("MeshAngularDeflection", 28.65);
        funcs["MeshAngularDeflection"] = &PartParamsP::updateMeshAngularDeflection;
        MinimumAngularDeflection = handle->GetFloat("MinimumAngularDeflection", 5.0);
        funcs["MinimumAngularDeflection"] = &PartParamsP::updateMinimumAngularDeflection;
        OverrideTessellation = handle->GetBool("OverrideTessellation", false);
        funcs["OverrideTessellation"] = &PartParamsP::updateOverrideTessellation;
        MapFaceColor = handle->GetBool("MapFaceColor", true);
        funcs["MapFaceColor"] = &PartParamsP::updateMapFaceColor;
        MapLineColor = handle->GetBool("MapLineColor", false);
        funcs["MapLineColor"] = &PartParamsP::updateMapLineColor;
        MapPointColor = handle->GetBool("MapPointColor", false);
        funcs["MapPointColor"] = &PartParamsP::updateMapPointColor;
        MapTransparency = handle->GetBool("MapTransparency", false);
        funcs["MapTransparency"] = &PartParamsP::updateMapTransparency;
        AutoGridScale = handle->GetBool("AutoGridScale", false);
        funcs["AutoGridScale"] = &PartParamsP::updateAutoGridScale;
        PreviewAddColor = handle->GetUnsigned("PreviewAddColor", 0x64FFFF30);
        funcs["PreviewAddColor"] = &PartParamsP::updatePreviewAddColor;
        PreviewSubColor = handle->GetUnsigned("PreviewSubColor", 0xFF646430);
        funcs["PreviewSubColor"] = &PartParamsP::updatePreviewSubColor;
        PreviewDressColor = handle->GetUnsigned("PreviewDressColor", 0xFF64FF30);
        funcs["PreviewDressColor"] = &PartParamsP::updatePreviewDressColor;
        PreviewIntersectColor = handle->GetUnsigned("PreviewIntersectColor", 0x6464FF30);
        funcs["PreviewIntersectColor"] = &PartParamsP::updatePreviewIntersectColor;
        PreviewOnEdit = handle->GetBool("PreviewOnEdit", true);
        funcs["PreviewOnEdit"] = &PartParamsP::updatePreviewOnEdit;
        PreviewWithTransparency = handle->GetBool("PreviewWithTransparency", true);
        funcs["PreviewWithTransparency"] = &PartParamsP::updatePreviewWithTransparency;
        EditOnTop = handle->GetBool("EditOnTop", false);
        funcs["EditOnTop"] = &PartParamsP::updateEditOnTop;
        EditRecomputeWait = handle->GetInt("EditRecomputeWait", 300);
        funcs["EditRecomputeWait"] = &PartParamsP::updateEditRecomputeWait;
        AdjustCameraForNewFeature = handle->GetBool("AdjustCameraForNewFeature", true);
        funcs["AdjustCameraForNewFeature"] = &PartParamsP::updateAdjustCameraForNewFeature;
        DefaultDatumColor = handle->GetUnsigned("DefaultDatumColor", 0xFFD70099);
        funcs["DefaultDatumColor"] = &PartParamsP::updateDefaultDatumColor;
        RespectSystemDPI = handle->GetBool("RespectSystemDPI", false);
        funcs["RespectSystemDPI"] = &PartParamsP::updateRespectSystemDPI;
        SelectionPickThreshold = handle->GetInt("SelectionPickThreshold", 1000);
        funcs["SelectionPickThreshold"] = &PartParamsP::updateSelectionPickThreshold;
        SelectionPickThreshold2 = handle->GetInt("SelectionPickThreshold2", 500);
        funcs["SelectionPickThreshold2"] = &PartParamsP::updateSelectionPickThreshold2;
        SelectionPickRTree = handle->GetBool("SelectionPickRTree", false);
        funcs["SelectionPickRTree"] = &PartParamsP::updateSelectionPickRTree;
    }

    // Auto generated code (Tools/params_utils.py:217)
    ~PartParamsP() {
    }

    // Auto generated code (Tools/params_utils.py:222)
    void OnChange(Base::Subject<const char*> &, const char* sReason) {
        if(!sReason)
            return;
        auto it = funcs.find(sReason);
        if(it == funcs.end())
            return;
        it->second(this);
        
    }


    // Auto generated code (Tools/params_utils.py:238)
    static void updateNormalsFromUVNodes(PartParamsP *self) {
        self->NormalsFromUVNodes = self->handle->GetBool("NormalsFromUVNodes", true);
    }
    // Auto generated code (Tools/params_utils.py:238)
    static void updateTwoSideRendering(PartParamsP *self) {
        self->TwoSideRendering = self->handle->GetBool("TwoSideRendering", true);
    }
    // Auto generated code (Tools/params_utils.py:244)
    static void updateMinimumDeviation(PartParamsP *self) {
        auto v = self->handle->GetFloat("MinimumDeviation", 0.05);
        if (self->MinimumDeviation != v) {
            self->MinimumDeviation = v;
            PartParams::onMinimumDeviationChanged();
        }
    }
    // Auto generated code (Tools/params_utils.py:244)
    static void updateMeshDeviation(PartParamsP *self) {
        auto v = self->handle->GetFloat("MeshDeviation", 0.2);
        if (self->MeshDeviation != v) {
            self->MeshDeviation = v;
            PartParams::onMeshDeviationChanged();
        }
    }
    // Auto generated code (Tools/params_utils.py:244)
    static void updateMeshAngularDeflection(PartParamsP *self) {
        auto v = self->handle->GetFloat("MeshAngularDeflection", 28.65);
        if (self->MeshAngularDeflection != v) {
            self->MeshAngularDeflection = v;
            PartParams::onMeshAngularDeflectionChanged();
        }
    }
    // Auto generated code (Tools/params_utils.py:244)
    static void updateMinimumAngularDeflection(PartParamsP *self) {
        auto v = self->handle->GetFloat("MinimumAngularDeflection", 5.0);
        if (self->MinimumAngularDeflection != v) {
            self->MinimumAngularDeflection = v;
            PartParams::onMinimumAngularDeflectionChanged();
        }
    }
    // Auto generated code (Tools/params_utils.py:244)
    static void updateOverrideTessellation(PartParamsP *self) {
        auto v = self->handle->GetBool("OverrideTessellation", false);
        if (self->OverrideTessellation != v) {
            self->OverrideTessellation = v;
            PartParams::onOverrideTessellationChanged();
        }
    }
    // Auto generated code (Tools/params_utils.py:238)
    static void updateMapFaceColor(PartParamsP *self) {
        self->MapFaceColor = self->handle->GetBool("MapFaceColor", true);
    }
    // Auto generated code (Tools/params_utils.py:238)
    static void updateMapLineColor(PartParamsP *self) {
        self->MapLineColor = self->handle->GetBool("MapLineColor", false);
    }
    // Auto generated code (Tools/params_utils.py:238)
    static void updateMapPointColor(PartParamsP *self) {
        self->MapPointColor = self->handle->GetBool("MapPointColor", false);
    }
    // Auto generated code (Tools/params_utils.py:238)
    static void updateMapTransparency(PartParamsP *self) {
        self->MapTransparency = self->handle->GetBool("MapTransparency", false);
    }
    // Auto generated code (Tools/params_utils.py:238)
    static void updateAutoGridScale(PartParamsP *self) {
        self->AutoGridScale = self->handle->GetBool("AutoGridScale", false);
    }
    // Auto generated code (Tools/params_utils.py:238)
    static void updatePreviewAddColor(PartParamsP *self) {
        self->PreviewAddColor = self->handle->GetUnsigned("PreviewAddColor", 0x64FFFF30);
    }
    // Auto generated code (Tools/params_utils.py:238)
    static void updatePreviewSubColor(PartParamsP *self) {
        self->PreviewSubColor = self->handle->GetUnsigned("PreviewSubColor", 0xFF646430);
    }
    // Auto generated code (Tools/params_utils.py:238)
    static void updatePreviewDressColor(PartParamsP *self) {
        self->PreviewDressColor = self->handle->GetUnsigned("PreviewDressColor", 0xFF64FF30);
    }
    // Auto generated code (Tools/params_utils.py:238)
    static void updatePreviewIntersectColor(PartParamsP *self) {
        self->PreviewIntersectColor = self->handle->GetUnsigned("PreviewIntersectColor", 0x6464FF30);
    }
    // Auto generated code (Tools/params_utils.py:238)
    static void updatePreviewOnEdit(PartParamsP *self) {
        self->PreviewOnEdit = self->handle->GetBool("PreviewOnEdit", true);
    }
    // Auto generated code (Tools/params_utils.py:238)
    static void updatePreviewWithTransparency(PartParamsP *self) {
        self->PreviewWithTransparency = self->handle->GetBool("PreviewWithTransparency", true);
    }
    // Auto generated code (Tools/params_utils.py:238)
    static void updateEditOnTop(PartParamsP *self) {
        self->EditOnTop = self->handle->GetBool("EditOnTop", false);
    }
    // Auto generated code (Tools/params_utils.py:238)
    static void updateEditRecomputeWait(PartParamsP *self) {
        self->EditRecomputeWait = self->handle->GetInt("EditRecomputeWait", 300);
    }
    // Auto generated code (Tools/params_utils.py:238)
    static void updateAdjustCameraForNewFeature(PartParamsP *self) {
        self->AdjustCameraForNewFeature = self->handle->GetBool("AdjustCameraForNewFeature", true);
    }
    // Auto generated code (Tools/params_utils.py:238)
    static void updateDefaultDatumColor(PartParamsP *self) {
        self->DefaultDatumColor = self->handle->GetUnsigned("DefaultDatumColor", 0xFFD70099);
    }
    // Auto generated code (Tools/params_utils.py:244)
    static void updateRespectSystemDPI(PartParamsP *self) {
        auto v = self->handle->GetBool("RespectSystemDPI", false);
        if (self->RespectSystemDPI != v) {
            self->RespectSystemDPI = v;
            PartParams::onRespectSystemDPIChanged();
        }
    }
    // Auto generated code (Tools/params_utils.py:238)
    static void updateSelectionPickThreshold(PartParamsP *self) {
        self->SelectionPickThreshold = self->handle->GetInt("SelectionPickThreshold", 1000);
    }
    // Auto generated code (Tools/params_utils.py:238)
    static void updateSelectionPickThreshold2(PartParamsP *self) {
        self->SelectionPickThreshold2 = self->handle->GetInt("SelectionPickThreshold2", 500);
    }
    // Auto generated code (Tools/params_utils.py:238)
    static void updateSelectionPickRTree(PartParamsP *self) {
        self->SelectionPickRTree = self->handle->GetBool("SelectionPickRTree", false);
    }
};

// Auto generated code (Tools/params_utils.py:256)
PartParamsP *instance() {
    static PartParamsP *inst = new PartParamsP;
    return inst;
}

} // Anonymous namespace

// Auto generated code (Tools/params_utils.py:265)
ParameterGrp::handle PartParams::getHandle() {
    return instance()->handle;
}

// Auto generated code (Tools/params_utils.py:288)
const char *PartParams::docNormalsFromUVNodes() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const bool & PartParams::getNormalsFromUVNodes() {
    return instance()->NormalsFromUVNodes;
}

// Auto generated code (Tools/params_utils.py:300)
const bool & PartParams::defaultNormalsFromUVNodes() {
    const static bool def = true;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void PartParams::setNormalsFromUVNodes(const bool &v) {
    instance()->handle->SetBool("NormalsFromUVNodes",v);
    instance()->NormalsFromUVNodes = v;
}

// Auto generated code (Tools/params_utils.py:314)
void PartParams::removeNormalsFromUVNodes() {
    instance()->handle->RemoveBool("NormalsFromUVNodes");
}

// Auto generated code (Tools/params_utils.py:288)
const char *PartParams::docTwoSideRendering() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const bool & PartParams::getTwoSideRendering() {
    return instance()->TwoSideRendering;
}

// Auto generated code (Tools/params_utils.py:300)
const bool & PartParams::defaultTwoSideRendering() {
    const static bool def = true;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void PartParams::setTwoSideRendering(const bool &v) {
    instance()->handle->SetBool("TwoSideRendering",v);
    instance()->TwoSideRendering = v;
}

// Auto generated code (Tools/params_utils.py:314)
void PartParams::removeTwoSideRendering() {
    instance()->handle->RemoveBool("TwoSideRendering");
}

// Auto generated code (Tools/params_utils.py:288)
const char *PartParams::docMinimumDeviation() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const double & PartParams::getMinimumDeviation() {
    return instance()->MinimumDeviation;
}

// Auto generated code (Tools/params_utils.py:300)
const double & PartParams::defaultMinimumDeviation() {
    const static double def = 0.05;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void PartParams::setMinimumDeviation(const double &v) {
    instance()->handle->SetFloat("MinimumDeviation",v);
    instance()->MinimumDeviation = v;
}

// Auto generated code (Tools/params_utils.py:314)
void PartParams::removeMinimumDeviation() {
    instance()->handle->RemoveFloat("MinimumDeviation");
}

// Auto generated code (Tools/params_utils.py:288)
const char *PartParams::docMeshDeviation() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const double & PartParams::getMeshDeviation() {
    return instance()->MeshDeviation;
}

// Auto generated code (Tools/params_utils.py:300)
const double & PartParams::defaultMeshDeviation() {
    const static double def = 0.2;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void PartParams::setMeshDeviation(const double &v) {
    instance()->handle->SetFloat("MeshDeviation",v);
    instance()->MeshDeviation = v;
}

// Auto generated code (Tools/params_utils.py:314)
void PartParams::removeMeshDeviation() {
    instance()->handle->RemoveFloat("MeshDeviation");
}

// Auto generated code (Tools/params_utils.py:288)
const char *PartParams::docMeshAngularDeflection() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const double & PartParams::getMeshAngularDeflection() {
    return instance()->MeshAngularDeflection;
}

// Auto generated code (Tools/params_utils.py:300)
const double & PartParams::defaultMeshAngularDeflection() {
    const static double def = 28.65;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void PartParams::setMeshAngularDeflection(const double &v) {
    instance()->handle->SetFloat("MeshAngularDeflection",v);
    instance()->MeshAngularDeflection = v;
}

// Auto generated code (Tools/params_utils.py:314)
void PartParams::removeMeshAngularDeflection() {
    instance()->handle->RemoveFloat("MeshAngularDeflection");
}

// Auto generated code (Tools/params_utils.py:288)
const char *PartParams::docMinimumAngularDeflection() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const double & PartParams::getMinimumAngularDeflection() {
    return instance()->MinimumAngularDeflection;
}

// Auto generated code (Tools/params_utils.py:300)
const double & PartParams::defaultMinimumAngularDeflection() {
    const static double def = 5.0;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void PartParams::setMinimumAngularDeflection(const double &v) {
    instance()->handle->SetFloat("MinimumAngularDeflection",v);
    instance()->MinimumAngularDeflection = v;
}

// Auto generated code (Tools/params_utils.py:314)
void PartParams::removeMinimumAngularDeflection() {
    instance()->handle->RemoveFloat("MinimumAngularDeflection");
}

// Auto generated code (Tools/params_utils.py:288)
const char *PartParams::docOverrideTessellation() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const bool & PartParams::getOverrideTessellation() {
    return instance()->OverrideTessellation;
}

// Auto generated code (Tools/params_utils.py:300)
const bool & PartParams::defaultOverrideTessellation() {
    const static bool def = false;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void PartParams::setOverrideTessellation(const bool &v) {
    instance()->handle->SetBool("OverrideTessellation",v);
    instance()->OverrideTessellation = v;
}

// Auto generated code (Tools/params_utils.py:314)
void PartParams::removeOverrideTessellation() {
    instance()->handle->RemoveBool("OverrideTessellation");
}

// Auto generated code (Tools/params_utils.py:288)
const char *PartParams::docMapFaceColor() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const bool & PartParams::getMapFaceColor() {
    return instance()->MapFaceColor;
}

// Auto generated code (Tools/params_utils.py:300)
const bool & PartParams::defaultMapFaceColor() {
    const static bool def = true;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void PartParams::setMapFaceColor(const bool &v) {
    instance()->handle->SetBool("MapFaceColor",v);
    instance()->MapFaceColor = v;
}

// Auto generated code (Tools/params_utils.py:314)
void PartParams::removeMapFaceColor() {
    instance()->handle->RemoveBool("MapFaceColor");
}

// Auto generated code (Tools/params_utils.py:288)
const char *PartParams::docMapLineColor() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const bool & PartParams::getMapLineColor() {
    return instance()->MapLineColor;
}

// Auto generated code (Tools/params_utils.py:300)
const bool & PartParams::defaultMapLineColor() {
    const static bool def = false;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void PartParams::setMapLineColor(const bool &v) {
    instance()->handle->SetBool("MapLineColor",v);
    instance()->MapLineColor = v;
}

// Auto generated code (Tools/params_utils.py:314)
void PartParams::removeMapLineColor() {
    instance()->handle->RemoveBool("MapLineColor");
}

// Auto generated code (Tools/params_utils.py:288)
const char *PartParams::docMapPointColor() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const bool & PartParams::getMapPointColor() {
    return instance()->MapPointColor;
}

// Auto generated code (Tools/params_utils.py:300)
const bool & PartParams::defaultMapPointColor() {
    const static bool def = false;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void PartParams::setMapPointColor(const bool &v) {
    instance()->handle->SetBool("MapPointColor",v);
    instance()->MapPointColor = v;
}

// Auto generated code (Tools/params_utils.py:314)
void PartParams::removeMapPointColor() {
    instance()->handle->RemoveBool("MapPointColor");
}

// Auto generated code (Tools/params_utils.py:288)
const char *PartParams::docMapTransparency() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const bool & PartParams::getMapTransparency() {
    return instance()->MapTransparency;
}

// Auto generated code (Tools/params_utils.py:300)
const bool & PartParams::defaultMapTransparency() {
    const static bool def = false;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void PartParams::setMapTransparency(const bool &v) {
    instance()->handle->SetBool("MapTransparency",v);
    instance()->MapTransparency = v;
}

// Auto generated code (Tools/params_utils.py:314)
void PartParams::removeMapTransparency() {
    instance()->handle->RemoveBool("MapTransparency");
}

// Auto generated code (Tools/params_utils.py:288)
const char *PartParams::docAutoGridScale() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const bool & PartParams::getAutoGridScale() {
    return instance()->AutoGridScale;
}

// Auto generated code (Tools/params_utils.py:300)
const bool & PartParams::defaultAutoGridScale() {
    const static bool def = false;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void PartParams::setAutoGridScale(const bool &v) {
    instance()->handle->SetBool("AutoGridScale",v);
    instance()->AutoGridScale = v;
}

// Auto generated code (Tools/params_utils.py:314)
void PartParams::removeAutoGridScale() {
    instance()->handle->RemoveBool("AutoGridScale");
}

// Auto generated code (Tools/params_utils.py:288)
const char *PartParams::docPreviewAddColor() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const unsigned long & PartParams::getPreviewAddColor() {
    return instance()->PreviewAddColor;
}

// Auto generated code (Tools/params_utils.py:300)
const unsigned long & PartParams::defaultPreviewAddColor() {
    const static unsigned long def = 0x64FFFF30;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void PartParams::setPreviewAddColor(const unsigned long &v) {
    instance()->handle->SetUnsigned("PreviewAddColor",v);
    instance()->PreviewAddColor = v;
}

// Auto generated code (Tools/params_utils.py:314)
void PartParams::removePreviewAddColor() {
    instance()->handle->RemoveUnsigned("PreviewAddColor");
}

// Auto generated code (Tools/params_utils.py:288)
const char *PartParams::docPreviewSubColor() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const unsigned long & PartParams::getPreviewSubColor() {
    return instance()->PreviewSubColor;
}

// Auto generated code (Tools/params_utils.py:300)
const unsigned long & PartParams::defaultPreviewSubColor() {
    const static unsigned long def = 0xFF646430;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void PartParams::setPreviewSubColor(const unsigned long &v) {
    instance()->handle->SetUnsigned("PreviewSubColor",v);
    instance()->PreviewSubColor = v;
}

// Auto generated code (Tools/params_utils.py:314)
void PartParams::removePreviewSubColor() {
    instance()->handle->RemoveUnsigned("PreviewSubColor");
}

// Auto generated code (Tools/params_utils.py:288)
const char *PartParams::docPreviewDressColor() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const unsigned long & PartParams::getPreviewDressColor() {
    return instance()->PreviewDressColor;
}

// Auto generated code (Tools/params_utils.py:300)
const unsigned long & PartParams::defaultPreviewDressColor() {
    const static unsigned long def = 0xFF64FF30;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void PartParams::setPreviewDressColor(const unsigned long &v) {
    instance()->handle->SetUnsigned("PreviewDressColor",v);
    instance()->PreviewDressColor = v;
}

// Auto generated code (Tools/params_utils.py:314)
void PartParams::removePreviewDressColor() {
    instance()->handle->RemoveUnsigned("PreviewDressColor");
}

// Auto generated code (Tools/params_utils.py:288)
const char *PartParams::docPreviewIntersectColor() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const unsigned long & PartParams::getPreviewIntersectColor() {
    return instance()->PreviewIntersectColor;
}

// Auto generated code (Tools/params_utils.py:300)
const unsigned long & PartParams::defaultPreviewIntersectColor() {
    const static unsigned long def = 0x6464FF30;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void PartParams::setPreviewIntersectColor(const unsigned long &v) {
    instance()->handle->SetUnsigned("PreviewIntersectColor",v);
    instance()->PreviewIntersectColor = v;
}

// Auto generated code (Tools/params_utils.py:314)
void PartParams::removePreviewIntersectColor() {
    instance()->handle->RemoveUnsigned("PreviewIntersectColor");
}

// Auto generated code (Tools/params_utils.py:288)
const char *PartParams::docPreviewOnEdit() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const bool & PartParams::getPreviewOnEdit() {
    return instance()->PreviewOnEdit;
}

// Auto generated code (Tools/params_utils.py:300)
const bool & PartParams::defaultPreviewOnEdit() {
    const static bool def = true;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void PartParams::setPreviewOnEdit(const bool &v) {
    instance()->handle->SetBool("PreviewOnEdit",v);
    instance()->PreviewOnEdit = v;
}

// Auto generated code (Tools/params_utils.py:314)
void PartParams::removePreviewOnEdit() {
    instance()->handle->RemoveBool("PreviewOnEdit");
}

// Auto generated code (Tools/params_utils.py:288)
const char *PartParams::docPreviewWithTransparency() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const bool & PartParams::getPreviewWithTransparency() {
    return instance()->PreviewWithTransparency;
}

// Auto generated code (Tools/params_utils.py:300)
const bool & PartParams::defaultPreviewWithTransparency() {
    const static bool def = true;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void PartParams::setPreviewWithTransparency(const bool &v) {
    instance()->handle->SetBool("PreviewWithTransparency",v);
    instance()->PreviewWithTransparency = v;
}

// Auto generated code (Tools/params_utils.py:314)
void PartParams::removePreviewWithTransparency() {
    instance()->handle->RemoveBool("PreviewWithTransparency");
}

// Auto generated code (Tools/params_utils.py:288)
const char *PartParams::docEditOnTop() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const bool & PartParams::getEditOnTop() {
    return instance()->EditOnTop;
}

// Auto generated code (Tools/params_utils.py:300)
const bool & PartParams::defaultEditOnTop() {
    const static bool def = false;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void PartParams::setEditOnTop(const bool &v) {
    instance()->handle->SetBool("EditOnTop",v);
    instance()->EditOnTop = v;
}

// Auto generated code (Tools/params_utils.py:314)
void PartParams::removeEditOnTop() {
    instance()->handle->RemoveBool("EditOnTop");
}

// Auto generated code (Tools/params_utils.py:288)
const char *PartParams::docEditRecomputeWait() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const long & PartParams::getEditRecomputeWait() {
    return instance()->EditRecomputeWait;
}

// Auto generated code (Tools/params_utils.py:300)
const long & PartParams::defaultEditRecomputeWait() {
    const static long def = 300;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void PartParams::setEditRecomputeWait(const long &v) {
    instance()->handle->SetInt("EditRecomputeWait",v);
    instance()->EditRecomputeWait = v;
}

// Auto generated code (Tools/params_utils.py:314)
void PartParams::removeEditRecomputeWait() {
    instance()->handle->RemoveInt("EditRecomputeWait");
}

// Auto generated code (Tools/params_utils.py:288)
const char *PartParams::docAdjustCameraForNewFeature() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const bool & PartParams::getAdjustCameraForNewFeature() {
    return instance()->AdjustCameraForNewFeature;
}

// Auto generated code (Tools/params_utils.py:300)
const bool & PartParams::defaultAdjustCameraForNewFeature() {
    const static bool def = true;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void PartParams::setAdjustCameraForNewFeature(const bool &v) {
    instance()->handle->SetBool("AdjustCameraForNewFeature",v);
    instance()->AdjustCameraForNewFeature = v;
}

// Auto generated code (Tools/params_utils.py:314)
void PartParams::removeAdjustCameraForNewFeature() {
    instance()->handle->RemoveBool("AdjustCameraForNewFeature");
}

// Auto generated code (Tools/params_utils.py:288)
const char *PartParams::docDefaultDatumColor() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const unsigned long & PartParams::getDefaultDatumColor() {
    return instance()->DefaultDatumColor;
}

// Auto generated code (Tools/params_utils.py:300)
const unsigned long & PartParams::defaultDefaultDatumColor() {
    const static unsigned long def = 0xFFD70099;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void PartParams::setDefaultDatumColor(const unsigned long &v) {
    instance()->handle->SetUnsigned("DefaultDatumColor",v);
    instance()->DefaultDatumColor = v;
}

// Auto generated code (Tools/params_utils.py:314)
void PartParams::removeDefaultDatumColor() {
    instance()->handle->RemoveUnsigned("DefaultDatumColor");
}

// Auto generated code (Tools/params_utils.py:288)
const char *PartParams::docRespectSystemDPI() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const bool & PartParams::getRespectSystemDPI() {
    return instance()->RespectSystemDPI;
}

// Auto generated code (Tools/params_utils.py:300)
const bool & PartParams::defaultRespectSystemDPI() {
    const static bool def = false;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void PartParams::setRespectSystemDPI(const bool &v) {
    instance()->handle->SetBool("RespectSystemDPI",v);
    instance()->RespectSystemDPI = v;
}

// Auto generated code (Tools/params_utils.py:314)
void PartParams::removeRespectSystemDPI() {
    instance()->handle->RemoveBool("RespectSystemDPI");
}

// Auto generated code (Tools/params_utils.py:288)
const char *PartParams::docSelectionPickThreshold() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const long & PartParams::getSelectionPickThreshold() {
    return instance()->SelectionPickThreshold;
}

// Auto generated code (Tools/params_utils.py:300)
const long & PartParams::defaultSelectionPickThreshold() {
    const static long def = 1000;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void PartParams::setSelectionPickThreshold(const long &v) {
    instance()->handle->SetInt("SelectionPickThreshold",v);
    instance()->SelectionPickThreshold = v;
}

// Auto generated code (Tools/params_utils.py:314)
void PartParams::removeSelectionPickThreshold() {
    instance()->handle->RemoveInt("SelectionPickThreshold");
}

// Auto generated code (Tools/params_utils.py:288)
const char *PartParams::docSelectionPickThreshold2() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const long & PartParams::getSelectionPickThreshold2() {
    return instance()->SelectionPickThreshold2;
}

// Auto generated code (Tools/params_utils.py:300)
const long & PartParams::defaultSelectionPickThreshold2() {
    const static long def = 500;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void PartParams::setSelectionPickThreshold2(const long &v) {
    instance()->handle->SetInt("SelectionPickThreshold2",v);
    instance()->SelectionPickThreshold2 = v;
}

// Auto generated code (Tools/params_utils.py:314)
void PartParams::removeSelectionPickThreshold2() {
    instance()->handle->RemoveInt("SelectionPickThreshold2");
}

// Auto generated code (Tools/params_utils.py:288)
const char *PartParams::docSelectionPickRTree() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const bool & PartParams::getSelectionPickRTree() {
    return instance()->SelectionPickRTree;
}

// Auto generated code (Tools/params_utils.py:300)
const bool & PartParams::defaultSelectionPickRTree() {
    const static bool def = false;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void PartParams::setSelectionPickRTree(const bool &v) {
    instance()->handle->SetBool("SelectionPickRTree",v);
    instance()->SelectionPickRTree = v;
}

// Auto generated code (Tools/params_utils.py:314)
void PartParams::removeSelectionPickRTree() {
    instance()->handle->RemoveBool("SelectionPickRTree");
}
//[[[end]]]

void PartParams::onMeshDeviationChanged() {
    getTimer().start(100);
}

void PartParams::onMeshAngularDeflectionChanged() {
    getTimer().start(100);
}

void PartParams::onMinimumDeviationChanged() {
    getTimer().start(100);
}

void PartParams::onMinimumAngularDeflectionChanged() {
    getTimer().start(100);
}

void PartParams::onOverrideTessellationChanged() {
    getTimer().start(100);
}

void PartParams::onRespectSystemDPIChanged() {
    getTimer().start(100);
}
