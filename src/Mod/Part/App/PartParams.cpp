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

/*[[[cog
import PartParams
PartParams.define()
]]]*/

// Auto generated code (Tools/params_utils.py:196)
#include <unordered_map>
#include <App/Application.h>
#include <App/DynamicProperty.h>
#include "PartParams.h"
using namespace Part;

// Auto generated code (Tools/params_utils.py:207)
namespace {
class PartParamsP: public ParameterGrp::ObserverType {
public:
    ParameterGrp::handle handle;
    std::unordered_map<const char *,void(*)(PartParamsP*),App::CStringHasher,App::CStringHasher> funcs;

    bool ShapePropertyCopy;
    bool DisableShapeCache;
    long CommandOverride;
    long EnableWrapFeature;
    bool CopySubShape;
    bool UseBrepToolsOuterWire;
    bool UseBaseObjectName;
    bool AutoGroupSolids;
    bool SingleSolid;
    bool UsePipeForExtrusionDraft;
    bool LinearizeExtrusionDraft;
    bool AutoCorrectLink;
    bool RefineModel;
    bool AuxGroupUniqueLabel;
    bool SplitEllipsoid;
    long ParallelRunThreshold;
    bool AutoValidateShape;
    bool FixShape;
    unsigned long LoftMaxDegree;
    double MinimumDeviation;
    double MeshDeviation;
    double MeshAngularDeflection;
    double MinimumAngularDeflection;

    // Auto generated code (Tools/params_utils.py:245)
    PartParamsP() {
        handle = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Part");
        handle->Attach(this);

        ShapePropertyCopy = handle->GetBool("ShapePropertyCopy", false);
        funcs["ShapePropertyCopy"] = &PartParamsP::updateShapePropertyCopy;
        DisableShapeCache = handle->GetBool("DisableShapeCache", false);
        funcs["DisableShapeCache"] = &PartParamsP::updateDisableShapeCache;
        CommandOverride = handle->GetInt("CommandOverride", 2);
        funcs["CommandOverride"] = &PartParamsP::updateCommandOverride;
        EnableWrapFeature = handle->GetInt("EnableWrapFeature", 2);
        funcs["EnableWrapFeature"] = &PartParamsP::updateEnableWrapFeature;
        CopySubShape = handle->GetBool("CopySubShape", false);
        funcs["CopySubShape"] = &PartParamsP::updateCopySubShape;
        UseBrepToolsOuterWire = handle->GetBool("UseBrepToolsOuterWire", true);
        funcs["UseBrepToolsOuterWire"] = &PartParamsP::updateUseBrepToolsOuterWire;
        UseBaseObjectName = handle->GetBool("UseBaseObjectName", false);
        funcs["UseBaseObjectName"] = &PartParamsP::updateUseBaseObjectName;
        AutoGroupSolids = handle->GetBool("AutoGroupSolids", false);
        funcs["AutoGroupSolids"] = &PartParamsP::updateAutoGroupSolids;
        SingleSolid = handle->GetBool("SingleSolid", false);
        funcs["SingleSolid"] = &PartParamsP::updateSingleSolid;
        UsePipeForExtrusionDraft = handle->GetBool("UsePipeForExtrusionDraft", false);
        funcs["UsePipeForExtrusionDraft"] = &PartParamsP::updateUsePipeForExtrusionDraft;
        LinearizeExtrusionDraft = handle->GetBool("LinearizeExtrusionDraft", true);
        funcs["LinearizeExtrusionDraft"] = &PartParamsP::updateLinearizeExtrusionDraft;
        AutoCorrectLink = handle->GetBool("AutoCorrectLink", false);
        funcs["AutoCorrectLink"] = &PartParamsP::updateAutoCorrectLink;
        RefineModel = handle->GetBool("RefineModel", false);
        funcs["RefineModel"] = &PartParamsP::updateRefineModel;
        AuxGroupUniqueLabel = handle->GetBool("AuxGroupUniqueLabel", false);
        funcs["AuxGroupUniqueLabel"] = &PartParamsP::updateAuxGroupUniqueLabel;
        SplitEllipsoid = handle->GetBool("SplitEllipsoid", true);
        funcs["SplitEllipsoid"] = &PartParamsP::updateSplitEllipsoid;
        ParallelRunThreshold = handle->GetInt("ParallelRunThreshold", 100);
        funcs["ParallelRunThreshold"] = &PartParamsP::updateParallelRunThreshold;
        AutoValidateShape = handle->GetBool("AutoValidateShape", false);
        funcs["AutoValidateShape"] = &PartParamsP::updateAutoValidateShape;
        FixShape = handle->GetBool("FixShape", false);
        funcs["FixShape"] = &PartParamsP::updateFixShape;
        LoftMaxDegree = handle->GetUnsigned("LoftMaxDegree", 5);
        funcs["LoftMaxDegree"] = &PartParamsP::updateLoftMaxDegree;
        MinimumDeviation = handle->GetFloat("MinimumDeviation", 0.05);
        funcs["MinimumDeviation"] = &PartParamsP::updateMinimumDeviation;
        MeshDeviation = handle->GetFloat("MeshDeviation", 0.2);
        funcs["MeshDeviation"] = &PartParamsP::updateMeshDeviation;
        MeshAngularDeflection = handle->GetFloat("MeshAngularDeflection", 28.65);
        funcs["MeshAngularDeflection"] = &PartParamsP::updateMeshAngularDeflection;
        MinimumAngularDeflection = handle->GetFloat("MinimumAngularDeflection", 5.0);
        funcs["MinimumAngularDeflection"] = &PartParamsP::updateMinimumAngularDeflection;
    }

    // Auto generated code (Tools/params_utils.py:263)
    ~PartParamsP() {
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


    // Auto generated code (Tools/params_utils.py:288)
    static void updateShapePropertyCopy(PartParamsP *self) {
        self->ShapePropertyCopy = self->handle->GetBool("ShapePropertyCopy", false);
    }
    // Auto generated code (Tools/params_utils.py:288)
    static void updateDisableShapeCache(PartParamsP *self) {
        self->DisableShapeCache = self->handle->GetBool("DisableShapeCache", false);
    }
    // Auto generated code (Tools/params_utils.py:288)
    static void updateCommandOverride(PartParamsP *self) {
        self->CommandOverride = self->handle->GetInt("CommandOverride", 2);
    }
    // Auto generated code (Tools/params_utils.py:288)
    static void updateEnableWrapFeature(PartParamsP *self) {
        self->EnableWrapFeature = self->handle->GetInt("EnableWrapFeature", 2);
    }
    // Auto generated code (Tools/params_utils.py:288)
    static void updateCopySubShape(PartParamsP *self) {
        self->CopySubShape = self->handle->GetBool("CopySubShape", false);
    }
    // Auto generated code (Tools/params_utils.py:288)
    static void updateUseBrepToolsOuterWire(PartParamsP *self) {
        self->UseBrepToolsOuterWire = self->handle->GetBool("UseBrepToolsOuterWire", true);
    }
    // Auto generated code (Tools/params_utils.py:288)
    static void updateUseBaseObjectName(PartParamsP *self) {
        self->UseBaseObjectName = self->handle->GetBool("UseBaseObjectName", false);
    }
    // Auto generated code (Tools/params_utils.py:288)
    static void updateAutoGroupSolids(PartParamsP *self) {
        self->AutoGroupSolids = self->handle->GetBool("AutoGroupSolids", false);
    }
    // Auto generated code (Tools/params_utils.py:288)
    static void updateSingleSolid(PartParamsP *self) {
        self->SingleSolid = self->handle->GetBool("SingleSolid", false);
    }
    // Auto generated code (Tools/params_utils.py:288)
    static void updateUsePipeForExtrusionDraft(PartParamsP *self) {
        self->UsePipeForExtrusionDraft = self->handle->GetBool("UsePipeForExtrusionDraft", false);
    }
    // Auto generated code (Tools/params_utils.py:288)
    static void updateLinearizeExtrusionDraft(PartParamsP *self) {
        self->LinearizeExtrusionDraft = self->handle->GetBool("LinearizeExtrusionDraft", true);
    }
    // Auto generated code (Tools/params_utils.py:288)
    static void updateAutoCorrectLink(PartParamsP *self) {
        self->AutoCorrectLink = self->handle->GetBool("AutoCorrectLink", false);
    }
    // Auto generated code (Tools/params_utils.py:288)
    static void updateRefineModel(PartParamsP *self) {
        self->RefineModel = self->handle->GetBool("RefineModel", false);
    }
    // Auto generated code (Tools/params_utils.py:288)
    static void updateAuxGroupUniqueLabel(PartParamsP *self) {
        self->AuxGroupUniqueLabel = self->handle->GetBool("AuxGroupUniqueLabel", false);
    }
    // Auto generated code (Tools/params_utils.py:288)
    static void updateSplitEllipsoid(PartParamsP *self) {
        self->SplitEllipsoid = self->handle->GetBool("SplitEllipsoid", true);
    }
    // Auto generated code (Tools/params_utils.py:288)
    static void updateParallelRunThreshold(PartParamsP *self) {
        self->ParallelRunThreshold = self->handle->GetInt("ParallelRunThreshold", 100);
    }
    // Auto generated code (Tools/params_utils.py:288)
    static void updateAutoValidateShape(PartParamsP *self) {
        self->AutoValidateShape = self->handle->GetBool("AutoValidateShape", false);
    }
    // Auto generated code (Tools/params_utils.py:288)
    static void updateFixShape(PartParamsP *self) {
        self->FixShape = self->handle->GetBool("FixShape", false);
    }
    // Auto generated code (Tools/params_utils.py:288)
    static void updateLoftMaxDegree(PartParamsP *self) {
        self->LoftMaxDegree = self->handle->GetUnsigned("LoftMaxDegree", 5);
    }
    // Auto generated code (Tools/params_utils.py:288)
    static void updateMinimumDeviation(PartParamsP *self) {
        self->MinimumDeviation = self->handle->GetFloat("MinimumDeviation", 0.05);
    }
    // Auto generated code (Tools/params_utils.py:288)
    static void updateMeshDeviation(PartParamsP *self) {
        self->MeshDeviation = self->handle->GetFloat("MeshDeviation", 0.2);
    }
    // Auto generated code (Tools/params_utils.py:288)
    static void updateMeshAngularDeflection(PartParamsP *self) {
        self->MeshAngularDeflection = self->handle->GetFloat("MeshAngularDeflection", 28.65);
    }
    // Auto generated code (Tools/params_utils.py:288)
    static void updateMinimumAngularDeflection(PartParamsP *self) {
        self->MinimumAngularDeflection = self->handle->GetFloat("MinimumAngularDeflection", 5.0);
    }
};

// Auto generated code (Tools/params_utils.py:310)
PartParamsP *instance() {
    static PartParamsP *inst = new PartParamsP;
    return inst;
}

} // Anonymous namespace

// Auto generated code (Tools/params_utils.py:321)
ParameterGrp::handle PartParams::getHandle() {
    return instance()->handle;
}

// Auto generated code (Tools/params_utils.py:350)
const char *PartParams::docShapePropertyCopy() {
    return "";
}

// Auto generated code (Tools/params_utils.py:358)
const bool & PartParams::getShapePropertyCopy() {
    return instance()->ShapePropertyCopy;
}

// Auto generated code (Tools/params_utils.py:366)
const bool & PartParams::defaultShapePropertyCopy() {
    const static bool def = false;
    return def;
}

// Auto generated code (Tools/params_utils.py:375)
void PartParams::setShapePropertyCopy(const bool &v) {
    instance()->handle->SetBool("ShapePropertyCopy",v);
    instance()->ShapePropertyCopy = v;
}

// Auto generated code (Tools/params_utils.py:384)
void PartParams::removeShapePropertyCopy() {
    instance()->handle->RemoveBool("ShapePropertyCopy");
}

// Auto generated code (Tools/params_utils.py:350)
const char *PartParams::docDisableShapeCache() {
    return "";
}

// Auto generated code (Tools/params_utils.py:358)
const bool & PartParams::getDisableShapeCache() {
    return instance()->DisableShapeCache;
}

// Auto generated code (Tools/params_utils.py:366)
const bool & PartParams::defaultDisableShapeCache() {
    const static bool def = false;
    return def;
}

// Auto generated code (Tools/params_utils.py:375)
void PartParams::setDisableShapeCache(const bool &v) {
    instance()->handle->SetBool("DisableShapeCache",v);
    instance()->DisableShapeCache = v;
}

// Auto generated code (Tools/params_utils.py:384)
void PartParams::removeDisableShapeCache() {
    instance()->handle->RemoveBool("DisableShapeCache");
}

// Auto generated code (Tools/params_utils.py:350)
const char *PartParams::docCommandOverride() {
    return "";
}

// Auto generated code (Tools/params_utils.py:358)
const long & PartParams::getCommandOverride() {
    return instance()->CommandOverride;
}

// Auto generated code (Tools/params_utils.py:366)
const long & PartParams::defaultCommandOverride() {
    const static long def = 2;
    return def;
}

// Auto generated code (Tools/params_utils.py:375)
void PartParams::setCommandOverride(const long &v) {
    instance()->handle->SetInt("CommandOverride",v);
    instance()->CommandOverride = v;
}

// Auto generated code (Tools/params_utils.py:384)
void PartParams::removeCommandOverride() {
    instance()->handle->RemoveInt("CommandOverride");
}

// Auto generated code (Tools/params_utils.py:350)
const char *PartParams::docEnableWrapFeature() {
    return "";
}

// Auto generated code (Tools/params_utils.py:358)
const long & PartParams::getEnableWrapFeature() {
    return instance()->EnableWrapFeature;
}

// Auto generated code (Tools/params_utils.py:366)
const long & PartParams::defaultEnableWrapFeature() {
    const static long def = 2;
    return def;
}

// Auto generated code (Tools/params_utils.py:375)
void PartParams::setEnableWrapFeature(const long &v) {
    instance()->handle->SetInt("EnableWrapFeature",v);
    instance()->EnableWrapFeature = v;
}

// Auto generated code (Tools/params_utils.py:384)
void PartParams::removeEnableWrapFeature() {
    instance()->handle->RemoveInt("EnableWrapFeature");
}

// Auto generated code (Tools/params_utils.py:350)
const char *PartParams::docCopySubShape() {
    return "";
}

// Auto generated code (Tools/params_utils.py:358)
const bool & PartParams::getCopySubShape() {
    return instance()->CopySubShape;
}

// Auto generated code (Tools/params_utils.py:366)
const bool & PartParams::defaultCopySubShape() {
    const static bool def = false;
    return def;
}

// Auto generated code (Tools/params_utils.py:375)
void PartParams::setCopySubShape(const bool &v) {
    instance()->handle->SetBool("CopySubShape",v);
    instance()->CopySubShape = v;
}

// Auto generated code (Tools/params_utils.py:384)
void PartParams::removeCopySubShape() {
    instance()->handle->RemoveBool("CopySubShape");
}

// Auto generated code (Tools/params_utils.py:350)
const char *PartParams::docUseBrepToolsOuterWire() {
    return "";
}

// Auto generated code (Tools/params_utils.py:358)
const bool & PartParams::getUseBrepToolsOuterWire() {
    return instance()->UseBrepToolsOuterWire;
}

// Auto generated code (Tools/params_utils.py:366)
const bool & PartParams::defaultUseBrepToolsOuterWire() {
    const static bool def = true;
    return def;
}

// Auto generated code (Tools/params_utils.py:375)
void PartParams::setUseBrepToolsOuterWire(const bool &v) {
    instance()->handle->SetBool("UseBrepToolsOuterWire",v);
    instance()->UseBrepToolsOuterWire = v;
}

// Auto generated code (Tools/params_utils.py:384)
void PartParams::removeUseBrepToolsOuterWire() {
    instance()->handle->RemoveBool("UseBrepToolsOuterWire");
}

// Auto generated code (Tools/params_utils.py:350)
const char *PartParams::docUseBaseObjectName() {
    return "";
}

// Auto generated code (Tools/params_utils.py:358)
const bool & PartParams::getUseBaseObjectName() {
    return instance()->UseBaseObjectName;
}

// Auto generated code (Tools/params_utils.py:366)
const bool & PartParams::defaultUseBaseObjectName() {
    const static bool def = false;
    return def;
}

// Auto generated code (Tools/params_utils.py:375)
void PartParams::setUseBaseObjectName(const bool &v) {
    instance()->handle->SetBool("UseBaseObjectName",v);
    instance()->UseBaseObjectName = v;
}

// Auto generated code (Tools/params_utils.py:384)
void PartParams::removeUseBaseObjectName() {
    instance()->handle->RemoveBool("UseBaseObjectName");
}

// Auto generated code (Tools/params_utils.py:350)
const char *PartParams::docAutoGroupSolids() {
    return "";
}

// Auto generated code (Tools/params_utils.py:358)
const bool & PartParams::getAutoGroupSolids() {
    return instance()->AutoGroupSolids;
}

// Auto generated code (Tools/params_utils.py:366)
const bool & PartParams::defaultAutoGroupSolids() {
    const static bool def = false;
    return def;
}

// Auto generated code (Tools/params_utils.py:375)
void PartParams::setAutoGroupSolids(const bool &v) {
    instance()->handle->SetBool("AutoGroupSolids",v);
    instance()->AutoGroupSolids = v;
}

// Auto generated code (Tools/params_utils.py:384)
void PartParams::removeAutoGroupSolids() {
    instance()->handle->RemoveBool("AutoGroupSolids");
}

// Auto generated code (Tools/params_utils.py:350)
const char *PartParams::docSingleSolid() {
    return "";
}

// Auto generated code (Tools/params_utils.py:358)
const bool & PartParams::getSingleSolid() {
    return instance()->SingleSolid;
}

// Auto generated code (Tools/params_utils.py:366)
const bool & PartParams::defaultSingleSolid() {
    const static bool def = false;
    return def;
}

// Auto generated code (Tools/params_utils.py:375)
void PartParams::setSingleSolid(const bool &v) {
    instance()->handle->SetBool("SingleSolid",v);
    instance()->SingleSolid = v;
}

// Auto generated code (Tools/params_utils.py:384)
void PartParams::removeSingleSolid() {
    instance()->handle->RemoveBool("SingleSolid");
}

// Auto generated code (Tools/params_utils.py:350)
const char *PartParams::docUsePipeForExtrusionDraft() {
    return "";
}

// Auto generated code (Tools/params_utils.py:358)
const bool & PartParams::getUsePipeForExtrusionDraft() {
    return instance()->UsePipeForExtrusionDraft;
}

// Auto generated code (Tools/params_utils.py:366)
const bool & PartParams::defaultUsePipeForExtrusionDraft() {
    const static bool def = false;
    return def;
}

// Auto generated code (Tools/params_utils.py:375)
void PartParams::setUsePipeForExtrusionDraft(const bool &v) {
    instance()->handle->SetBool("UsePipeForExtrusionDraft",v);
    instance()->UsePipeForExtrusionDraft = v;
}

// Auto generated code (Tools/params_utils.py:384)
void PartParams::removeUsePipeForExtrusionDraft() {
    instance()->handle->RemoveBool("UsePipeForExtrusionDraft");
}

// Auto generated code (Tools/params_utils.py:350)
const char *PartParams::docLinearizeExtrusionDraft() {
    return "";
}

// Auto generated code (Tools/params_utils.py:358)
const bool & PartParams::getLinearizeExtrusionDraft() {
    return instance()->LinearizeExtrusionDraft;
}

// Auto generated code (Tools/params_utils.py:366)
const bool & PartParams::defaultLinearizeExtrusionDraft() {
    const static bool def = true;
    return def;
}

// Auto generated code (Tools/params_utils.py:375)
void PartParams::setLinearizeExtrusionDraft(const bool &v) {
    instance()->handle->SetBool("LinearizeExtrusionDraft",v);
    instance()->LinearizeExtrusionDraft = v;
}

// Auto generated code (Tools/params_utils.py:384)
void PartParams::removeLinearizeExtrusionDraft() {
    instance()->handle->RemoveBool("LinearizeExtrusionDraft");
}

// Auto generated code (Tools/params_utils.py:350)
const char *PartParams::docAutoCorrectLink() {
    return "";
}

// Auto generated code (Tools/params_utils.py:358)
const bool & PartParams::getAutoCorrectLink() {
    return instance()->AutoCorrectLink;
}

// Auto generated code (Tools/params_utils.py:366)
const bool & PartParams::defaultAutoCorrectLink() {
    const static bool def = false;
    return def;
}

// Auto generated code (Tools/params_utils.py:375)
void PartParams::setAutoCorrectLink(const bool &v) {
    instance()->handle->SetBool("AutoCorrectLink",v);
    instance()->AutoCorrectLink = v;
}

// Auto generated code (Tools/params_utils.py:384)
void PartParams::removeAutoCorrectLink() {
    instance()->handle->RemoveBool("AutoCorrectLink");
}

// Auto generated code (Tools/params_utils.py:350)
const char *PartParams::docRefineModel() {
    return "";
}

// Auto generated code (Tools/params_utils.py:358)
const bool & PartParams::getRefineModel() {
    return instance()->RefineModel;
}

// Auto generated code (Tools/params_utils.py:366)
const bool & PartParams::defaultRefineModel() {
    const static bool def = false;
    return def;
}

// Auto generated code (Tools/params_utils.py:375)
void PartParams::setRefineModel(const bool &v) {
    instance()->handle->SetBool("RefineModel",v);
    instance()->RefineModel = v;
}

// Auto generated code (Tools/params_utils.py:384)
void PartParams::removeRefineModel() {
    instance()->handle->RemoveBool("RefineModel");
}

// Auto generated code (Tools/params_utils.py:350)
const char *PartParams::docAuxGroupUniqueLabel() {
    return "";
}

// Auto generated code (Tools/params_utils.py:358)
const bool & PartParams::getAuxGroupUniqueLabel() {
    return instance()->AuxGroupUniqueLabel;
}

// Auto generated code (Tools/params_utils.py:366)
const bool & PartParams::defaultAuxGroupUniqueLabel() {
    const static bool def = false;
    return def;
}

// Auto generated code (Tools/params_utils.py:375)
void PartParams::setAuxGroupUniqueLabel(const bool &v) {
    instance()->handle->SetBool("AuxGroupUniqueLabel",v);
    instance()->AuxGroupUniqueLabel = v;
}

// Auto generated code (Tools/params_utils.py:384)
void PartParams::removeAuxGroupUniqueLabel() {
    instance()->handle->RemoveBool("AuxGroupUniqueLabel");
}

// Auto generated code (Tools/params_utils.py:350)
const char *PartParams::docSplitEllipsoid() {
    return "";
}

// Auto generated code (Tools/params_utils.py:358)
const bool & PartParams::getSplitEllipsoid() {
    return instance()->SplitEllipsoid;
}

// Auto generated code (Tools/params_utils.py:366)
const bool & PartParams::defaultSplitEllipsoid() {
    const static bool def = true;
    return def;
}

// Auto generated code (Tools/params_utils.py:375)
void PartParams::setSplitEllipsoid(const bool &v) {
    instance()->handle->SetBool("SplitEllipsoid",v);
    instance()->SplitEllipsoid = v;
}

// Auto generated code (Tools/params_utils.py:384)
void PartParams::removeSplitEllipsoid() {
    instance()->handle->RemoveBool("SplitEllipsoid");
}

// Auto generated code (Tools/params_utils.py:350)
const char *PartParams::docParallelRunThreshold() {
    return "";
}

// Auto generated code (Tools/params_utils.py:358)
const long & PartParams::getParallelRunThreshold() {
    return instance()->ParallelRunThreshold;
}

// Auto generated code (Tools/params_utils.py:366)
const long & PartParams::defaultParallelRunThreshold() {
    const static long def = 100;
    return def;
}

// Auto generated code (Tools/params_utils.py:375)
void PartParams::setParallelRunThreshold(const long &v) {
    instance()->handle->SetInt("ParallelRunThreshold",v);
    instance()->ParallelRunThreshold = v;
}

// Auto generated code (Tools/params_utils.py:384)
void PartParams::removeParallelRunThreshold() {
    instance()->handle->RemoveInt("ParallelRunThreshold");
}

// Auto generated code (Tools/params_utils.py:350)
const char *PartParams::docAutoValidateShape() {
    return "";
}

// Auto generated code (Tools/params_utils.py:358)
const bool & PartParams::getAutoValidateShape() {
    return instance()->AutoValidateShape;
}

// Auto generated code (Tools/params_utils.py:366)
const bool & PartParams::defaultAutoValidateShape() {
    const static bool def = false;
    return def;
}

// Auto generated code (Tools/params_utils.py:375)
void PartParams::setAutoValidateShape(const bool &v) {
    instance()->handle->SetBool("AutoValidateShape",v);
    instance()->AutoValidateShape = v;
}

// Auto generated code (Tools/params_utils.py:384)
void PartParams::removeAutoValidateShape() {
    instance()->handle->RemoveBool("AutoValidateShape");
}

// Auto generated code (Tools/params_utils.py:350)
const char *PartParams::docFixShape() {
    return "";
}

// Auto generated code (Tools/params_utils.py:358)
const bool & PartParams::getFixShape() {
    return instance()->FixShape;
}

// Auto generated code (Tools/params_utils.py:366)
const bool & PartParams::defaultFixShape() {
    const static bool def = false;
    return def;
}

// Auto generated code (Tools/params_utils.py:375)
void PartParams::setFixShape(const bool &v) {
    instance()->handle->SetBool("FixShape",v);
    instance()->FixShape = v;
}

// Auto generated code (Tools/params_utils.py:384)
void PartParams::removeFixShape() {
    instance()->handle->RemoveBool("FixShape");
}

// Auto generated code (Tools/params_utils.py:350)
const char *PartParams::docLoftMaxDegree() {
    return "";
}

// Auto generated code (Tools/params_utils.py:358)
const unsigned long & PartParams::getLoftMaxDegree() {
    return instance()->LoftMaxDegree;
}

// Auto generated code (Tools/params_utils.py:366)
const unsigned long & PartParams::defaultLoftMaxDegree() {
    const static unsigned long def = 5;
    return def;
}

// Auto generated code (Tools/params_utils.py:375)
void PartParams::setLoftMaxDegree(const unsigned long &v) {
    instance()->handle->SetUnsigned("LoftMaxDegree",v);
    instance()->LoftMaxDegree = v;
}

// Auto generated code (Tools/params_utils.py:384)
void PartParams::removeLoftMaxDegree() {
    instance()->handle->RemoveUnsigned("LoftMaxDegree");
}

// Auto generated code (Tools/params_utils.py:350)
const char *PartParams::docMinimumDeviation() {
    return "";
}

// Auto generated code (Tools/params_utils.py:358)
const double & PartParams::getMinimumDeviation() {
    return instance()->MinimumDeviation;
}

// Auto generated code (Tools/params_utils.py:366)
const double & PartParams::defaultMinimumDeviation() {
    const static double def = 0.05;
    return def;
}

// Auto generated code (Tools/params_utils.py:375)
void PartParams::setMinimumDeviation(const double &v) {
    instance()->handle->SetFloat("MinimumDeviation",v);
    instance()->MinimumDeviation = v;
}

// Auto generated code (Tools/params_utils.py:384)
void PartParams::removeMinimumDeviation() {
    instance()->handle->RemoveFloat("MinimumDeviation");
}

// Auto generated code (Tools/params_utils.py:350)
const char *PartParams::docMeshDeviation() {
    return "";
}

// Auto generated code (Tools/params_utils.py:358)
const double & PartParams::getMeshDeviation() {
    return instance()->MeshDeviation;
}

// Auto generated code (Tools/params_utils.py:366)
const double & PartParams::defaultMeshDeviation() {
    const static double def = 0.2;
    return def;
}

// Auto generated code (Tools/params_utils.py:375)
void PartParams::setMeshDeviation(const double &v) {
    instance()->handle->SetFloat("MeshDeviation",v);
    instance()->MeshDeviation = v;
}

// Auto generated code (Tools/params_utils.py:384)
void PartParams::removeMeshDeviation() {
    instance()->handle->RemoveFloat("MeshDeviation");
}

// Auto generated code (Tools/params_utils.py:350)
const char *PartParams::docMeshAngularDeflection() {
    return "";
}

// Auto generated code (Tools/params_utils.py:358)
const double & PartParams::getMeshAngularDeflection() {
    return instance()->MeshAngularDeflection;
}

// Auto generated code (Tools/params_utils.py:366)
const double & PartParams::defaultMeshAngularDeflection() {
    const static double def = 28.65;
    return def;
}

// Auto generated code (Tools/params_utils.py:375)
void PartParams::setMeshAngularDeflection(const double &v) {
    instance()->handle->SetFloat("MeshAngularDeflection",v);
    instance()->MeshAngularDeflection = v;
}

// Auto generated code (Tools/params_utils.py:384)
void PartParams::removeMeshAngularDeflection() {
    instance()->handle->RemoveFloat("MeshAngularDeflection");
}

// Auto generated code (Tools/params_utils.py:350)
const char *PartParams::docMinimumAngularDeflection() {
    return "";
}

// Auto generated code (Tools/params_utils.py:358)
const double & PartParams::getMinimumAngularDeflection() {
    return instance()->MinimumAngularDeflection;
}

// Auto generated code (Tools/params_utils.py:366)
const double & PartParams::defaultMinimumAngularDeflection() {
    const static double def = 5.0;
    return def;
}

// Auto generated code (Tools/params_utils.py:375)
void PartParams::setMinimumAngularDeflection(const double &v) {
    instance()->handle->SetFloat("MinimumAngularDeflection",v);
    instance()->MinimumAngularDeflection = v;
}

// Auto generated code (Tools/params_utils.py:384)
void PartParams::removeMinimumAngularDeflection() {
    instance()->handle->RemoveFloat("MinimumAngularDeflection");
}
//[[[end]]]
