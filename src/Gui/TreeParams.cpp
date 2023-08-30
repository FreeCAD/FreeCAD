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
#include "Tree.h"

/*[[[cog
import TreeParams
TreeParams.define()
]]]*/

// Auto generated code (Tools/params_utils.py:166)
#include <unordered_map>
#include <App/Application.h>
#include <App/DynamicProperty.h>
#include "TreeParams.h"
using namespace Gui;

// Auto generated code (Tools/params_utils.py:175)
namespace {
class TreeParamsP: public ParameterGrp::ObserverType {
public:
    ParameterGrp::handle handle;
    std::unordered_map<const char *,void(*)(TreeParamsP*),App::CStringHasher,App::CStringHasher> funcs;

    bool SyncSelection;
    bool CheckBoxesSelection;
    bool SyncView;
    bool PreSelection;
    bool SyncPlacement;
    bool RecordSelection;
    long DocumentMode;
    long StatusTimeout;
    long SelectionTimeout;
    long PreSelectionTimeout;
    long PreSelectionDelay;
    long PreSelectionMinDelay;
    bool RecomputeOnDrop;
    bool KeepRootOrder;
    bool TreeActiveAutoExpand;
    unsigned long TreeActiveColor;
    unsigned long TreeEditColor;
    unsigned long SelectingGroupColor;
    bool TreeActiveBold;
    bool TreeActiveItalic;
    bool TreeActiveUnderlined;
    bool TreeActiveOverlined;
    long Indentation;
    bool LabelExpression;
    long IconSize;
    long FontSize;
    long ItemSpacing;
    unsigned long ItemBackground;
    long ItemBackgroundPadding;
    bool HideColumn;
    bool HideScrollBar;
    bool HideHeaderView;
    bool ResizableColumn;
    long ColumnSize1;
    long ColumnSize2;
    bool TreeToolTipIcon;

    // Auto generated code (Tools/params_utils.py:203)
    TreeParamsP() {
        handle = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/TreeView");
        handle->Attach(this);

        SyncSelection = handle->GetBool("SyncSelection", true);
        funcs["SyncSelection"] = &TreeParamsP::updateSyncSelection;
        CheckBoxesSelection = handle->GetBool("CheckBoxesSelection", false);
        funcs["CheckBoxesSelection"] = &TreeParamsP::updateCheckBoxesSelection;
        SyncView = handle->GetBool("SyncView", true);
        funcs["SyncView"] = &TreeParamsP::updateSyncView;
        PreSelection = handle->GetBool("PreSelection", true);
        funcs["PreSelection"] = &TreeParamsP::updatePreSelection;
        SyncPlacement = handle->GetBool("SyncPlacement", false);
        funcs["SyncPlacement"] = &TreeParamsP::updateSyncPlacement;
        RecordSelection = handle->GetBool("RecordSelection", true);
        funcs["RecordSelection"] = &TreeParamsP::updateRecordSelection;
        DocumentMode = handle->GetInt("DocumentMode", 2);
        funcs["DocumentMode"] = &TreeParamsP::updateDocumentMode;
        StatusTimeout = handle->GetInt("StatusTimeout", 100);
        funcs["StatusTimeout"] = &TreeParamsP::updateStatusTimeout;
        SelectionTimeout = handle->GetInt("SelectionTimeout", 100);
        funcs["SelectionTimeout"] = &TreeParamsP::updateSelectionTimeout;
        PreSelectionTimeout = handle->GetInt("PreSelectionTimeout", 500);
        funcs["PreSelectionTimeout"] = &TreeParamsP::updatePreSelectionTimeout;
        PreSelectionDelay = handle->GetInt("PreSelectionDelay", 700);
        funcs["PreSelectionDelay"] = &TreeParamsP::updatePreSelectionDelay;
        PreSelectionMinDelay = handle->GetInt("PreSelectionMinDelay", 200);
        funcs["PreSelectionMinDelay"] = &TreeParamsP::updatePreSelectionMinDelay;
        RecomputeOnDrop = handle->GetBool("RecomputeOnDrop", true);
        funcs["RecomputeOnDrop"] = &TreeParamsP::updateRecomputeOnDrop;
        KeepRootOrder = handle->GetBool("KeepRootOrder", true);
        funcs["KeepRootOrder"] = &TreeParamsP::updateKeepRootOrder;
        TreeActiveAutoExpand = handle->GetBool("TreeActiveAutoExpand", true);
        funcs["TreeActiveAutoExpand"] = &TreeParamsP::updateTreeActiveAutoExpand;
        TreeActiveColor = handle->GetUnsigned("TreeActiveColor", 3873898495);
        funcs["TreeActiveColor"] = &TreeParamsP::updateTreeActiveColor;
        TreeEditColor = handle->GetUnsigned("TreeEditColor", 2459042047);
        funcs["TreeEditColor"] = &TreeParamsP::updateTreeEditColor;
        SelectingGroupColor = handle->GetUnsigned("SelectingGroupColor", 1082163711);
        funcs["SelectingGroupColor"] = &TreeParamsP::updateSelectingGroupColor;
        TreeActiveBold = handle->GetBool("TreeActiveBold", true);
        funcs["TreeActiveBold"] = &TreeParamsP::updateTreeActiveBold;
        TreeActiveItalic = handle->GetBool("TreeActiveItalic", false);
        funcs["TreeActiveItalic"] = &TreeParamsP::updateTreeActiveItalic;
        TreeActiveUnderlined = handle->GetBool("TreeActiveUnderlined", false);
        funcs["TreeActiveUnderlined"] = &TreeParamsP::updateTreeActiveUnderlined;
        TreeActiveOverlined = handle->GetBool("TreeActiveOverlined", false);
        funcs["TreeActiveOverlined"] = &TreeParamsP::updateTreeActiveOverlined;
        Indentation = handle->GetInt("Indentation", 0);
        funcs["Indentation"] = &TreeParamsP::updateIndentation;
        LabelExpression = handle->GetBool("LabelExpression", false);
        funcs["LabelExpression"] = &TreeParamsP::updateLabelExpression;
        IconSize = handle->GetInt("IconSize", 0);
        funcs["IconSize"] = &TreeParamsP::updateIconSize;
        FontSize = handle->GetInt("FontSize", 0);
        funcs["FontSize"] = &TreeParamsP::updateFontSize;
        ItemSpacing = handle->GetInt("ItemSpacing", 0);
        funcs["ItemSpacing"] = &TreeParamsP::updateItemSpacing;
        ItemBackground = handle->GetUnsigned("ItemBackground", 0x00000000);
        funcs["ItemBackground"] = &TreeParamsP::updateItemBackground;
        ItemBackgroundPadding = handle->GetInt("ItemBackgroundPadding", 10);
        funcs["ItemBackgroundPadding"] = &TreeParamsP::updateItemBackgroundPadding;
        HideColumn = handle->GetBool("HideColumn", true);
        funcs["HideColumn"] = &TreeParamsP::updateHideColumn;
        HideScrollBar = handle->GetBool("HideScrollBar", true);
        funcs["HideScrollBar"] = &TreeParamsP::updateHideScrollBar;
        HideHeaderView = handle->GetBool("HideHeaderView", true);
        funcs["HideHeaderView"] = &TreeParamsP::updateHideHeaderView;
        ResizableColumn = handle->GetBool("ResizableColumn", false);
        funcs["ResizableColumn"] = &TreeParamsP::updateResizableColumn;
        ColumnSize1 = handle->GetInt("ColumnSize1", 0);
        funcs["ColumnSize1"] = &TreeParamsP::updateColumnSize1;
        ColumnSize2 = handle->GetInt("ColumnSize2", 0);
        funcs["ColumnSize2"] = &TreeParamsP::updateColumnSize2;
        TreeToolTipIcon = handle->GetBool("TreeToolTipIcon", false);
        funcs["TreeToolTipIcon"] = &TreeParamsP::updateTreeToolTipIcon;
    }

    // Auto generated code (Tools/params_utils.py:217)
    ~TreeParamsP() override = default;

    // Auto generated code (Tools/params_utils.py:222)
    void OnChange(Base::Subject<const char*> &, const char* sReason) override {
        if(!sReason)
            return;
        auto it = funcs.find(sReason);
        if(it == funcs.end())
            return;
        it->second(this);

    }


    // Auto generated code (Tools/params_utils.py:244)
    static void updateSyncSelection(TreeParamsP *self) {
        auto v = self->handle->GetBool("SyncSelection", true);
        if (self->SyncSelection != v) {
            self->SyncSelection = v;
            TreeParams::onSyncSelectionChanged();
        }
    }
    // Auto generated code (Tools/params_utils.py:244)
    static void updateCheckBoxesSelection(TreeParamsP *self) {
        auto v = self->handle->GetBool("CheckBoxesSelection", false);
        if (self->CheckBoxesSelection != v) {
            self->CheckBoxesSelection = v;
            TreeParams::onCheckBoxesSelectionChanged();
        }
    }
    // Auto generated code (Tools/params_utils.py:238)
    static void updateSyncView(TreeParamsP *self) {
        self->SyncView = self->handle->GetBool("SyncView", true);
    }
    // Auto generated code (Tools/params_utils.py:238)
    static void updatePreSelection(TreeParamsP *self) {
        self->PreSelection = self->handle->GetBool("PreSelection", true);
    }
    // Auto generated code (Tools/params_utils.py:238)
    static void updateSyncPlacement(TreeParamsP *self) {
        self->SyncPlacement = self->handle->GetBool("SyncPlacement", false);
    }
    // Auto generated code (Tools/params_utils.py:238)
    static void updateRecordSelection(TreeParamsP *self) {
        self->RecordSelection = self->handle->GetBool("RecordSelection", true);
    }
    // Auto generated code (Tools/params_utils.py:244)
    static void updateDocumentMode(TreeParamsP *self) {
        auto v = self->handle->GetInt("DocumentMode", 2);
        if (self->DocumentMode != v) {
            self->DocumentMode = v;
            TreeParams::onDocumentModeChanged();
        }
    }
    // Auto generated code (Tools/params_utils.py:238)
    static void updateStatusTimeout(TreeParamsP *self) {
        self->StatusTimeout = self->handle->GetInt("StatusTimeout", 100);
    }
    // Auto generated code (Tools/params_utils.py:238)
    static void updateSelectionTimeout(TreeParamsP *self) {
        self->SelectionTimeout = self->handle->GetInt("SelectionTimeout", 100);
    }
    // Auto generated code (Tools/params_utils.py:238)
    static void updatePreSelectionTimeout(TreeParamsP *self) {
        self->PreSelectionTimeout = self->handle->GetInt("PreSelectionTimeout", 500);
    }
    // Auto generated code (Tools/params_utils.py:238)
    static void updatePreSelectionDelay(TreeParamsP *self) {
        self->PreSelectionDelay = self->handle->GetInt("PreSelectionDelay", 700);
    }
    // Auto generated code (Tools/params_utils.py:238)
    static void updatePreSelectionMinDelay(TreeParamsP *self) {
        self->PreSelectionMinDelay = self->handle->GetInt("PreSelectionMinDelay", 200);
    }
    // Auto generated code (Tools/params_utils.py:238)
    static void updateRecomputeOnDrop(TreeParamsP *self) {
        self->RecomputeOnDrop = self->handle->GetBool("RecomputeOnDrop", true);
    }
    // Auto generated code (Tools/params_utils.py:238)
    static void updateKeepRootOrder(TreeParamsP *self) {
        self->KeepRootOrder = self->handle->GetBool("KeepRootOrder", true);
    }
    // Auto generated code (Tools/params_utils.py:238)
    static void updateTreeActiveAutoExpand(TreeParamsP *self) {
        self->TreeActiveAutoExpand = self->handle->GetBool("TreeActiveAutoExpand", true);
    }
    // Auto generated code (Tools/params_utils.py:244)
    static void updateTreeActiveColor(TreeParamsP *self) {
        auto v = self->handle->GetUnsigned("TreeActiveColor", 3873898495);
        if (self->TreeActiveColor != v) {
            self->TreeActiveColor = v;
            TreeParams::onTreeActiveColorChanged();
        }
    }
    // Auto generated code (Tools/params_utils.py:244)
    static void updateTreeEditColor(TreeParamsP *self) {
        auto v = self->handle->GetUnsigned("TreeEditColor", 2459042047);
        if (self->TreeEditColor != v) {
            self->TreeEditColor = v;
            TreeParams::onTreeEditColorChanged();
        }
    }
    // Auto generated code (Tools/params_utils.py:244)
    static void updateSelectingGroupColor(TreeParamsP *self) {
        auto v = self->handle->GetUnsigned("SelectingGroupColor", 1082163711);
        if (self->SelectingGroupColor != v) {
            self->SelectingGroupColor = v;
            TreeParams::onSelectingGroupColorChanged();
        }
    }
    // Auto generated code (Tools/params_utils.py:244)
    static void updateTreeActiveBold(TreeParamsP *self) {
        auto v = self->handle->GetBool("TreeActiveBold", true);
        if (self->TreeActiveBold != v) {
            self->TreeActiveBold = v;
            TreeParams::onTreeActiveBoldChanged();
        }
    }
    // Auto generated code (Tools/params_utils.py:244)
    static void updateTreeActiveItalic(TreeParamsP *self) {
        auto v = self->handle->GetBool("TreeActiveItalic", false);
        if (self->TreeActiveItalic != v) {
            self->TreeActiveItalic = v;
            TreeParams::onTreeActiveItalicChanged();
        }
    }
    // Auto generated code (Tools/params_utils.py:244)
    static void updateTreeActiveUnderlined(TreeParamsP *self) {
        auto v = self->handle->GetBool("TreeActiveUnderlined", false);
        if (self->TreeActiveUnderlined != v) {
            self->TreeActiveUnderlined = v;
            TreeParams::onTreeActiveUnderlinedChanged();
        }
    }
    // Auto generated code (Tools/params_utils.py:244)
    static void updateTreeActiveOverlined(TreeParamsP *self) {
        auto v = self->handle->GetBool("TreeActiveOverlined", false);
        if (self->TreeActiveOverlined != v) {
            self->TreeActiveOverlined = v;
            TreeParams::onTreeActiveOverlinedChanged();
        }
    }
    // Auto generated code (Tools/params_utils.py:244)
    static void updateIndentation(TreeParamsP *self) {
        auto v = self->handle->GetInt("Indentation", 0);
        if (self->Indentation != v) {
            self->Indentation = v;
            TreeParams::onIndentationChanged();
        }
    }
    // Auto generated code (Tools/params_utils.py:238)
    static void updateLabelExpression(TreeParamsP *self) {
        self->LabelExpression = self->handle->GetBool("LabelExpression", false);
    }
    // Auto generated code (Tools/params_utils.py:244)
    static void updateIconSize(TreeParamsP *self) {
        auto v = self->handle->GetInt("IconSize", 0);
        if (self->IconSize != v) {
            self->IconSize = v;
            TreeParams::onIconSizeChanged();
        }
    }
    // Auto generated code (Tools/params_utils.py:244)
    static void updateFontSize(TreeParamsP *self) {
        auto v = self->handle->GetInt("FontSize", 0);
        if (self->FontSize != v) {
            self->FontSize = v;
            TreeParams::onFontSizeChanged();
        }
    }
    // Auto generated code (Tools/params_utils.py:244)
    static void updateItemSpacing(TreeParamsP *self) {
        auto v = self->handle->GetInt("ItemSpacing", 0);
        if (self->ItemSpacing != v) {
            self->ItemSpacing = v;
            TreeParams::onItemSpacingChanged();
        }
    }
    // Auto generated code (Tools/params_utils.py:244)
    static void updateItemBackground(TreeParamsP *self) {
        auto v = self->handle->GetUnsigned("ItemBackground", 0x00000000);
        if (self->ItemBackground != v) {
            self->ItemBackground = v;
            TreeParams::onItemBackgroundChanged();
        }
    }
    // Auto generated code (Tools/params_utils.py:244)
    static void updateItemBackgroundPadding(TreeParamsP *self) {
        auto v = self->handle->GetInt("ItemBackgroundPadding", 10);
        if (self->ItemBackgroundPadding != v) {
            self->ItemBackgroundPadding = v;
            TreeParams::onItemBackgroundPaddingChanged();
        }
    }
    // Auto generated code (Tools/params_utils.py:244)
    static void updateHideColumn(TreeParamsP *self) {
        auto v = self->handle->GetBool("HideColumn", true);
        if (self->HideColumn != v) {
            self->HideColumn = v;
            TreeParams::onHideColumnChanged();
        }
    }
    // Auto generated code (Tools/params_utils.py:238)
    static void updateHideScrollBar(TreeParamsP *self) {
        self->HideScrollBar = self->handle->GetBool("HideScrollBar", true);
    }
    // Auto generated code (Tools/params_utils.py:238)
    static void updateHideHeaderView(TreeParamsP *self) {
        self->HideHeaderView = self->handle->GetBool("HideHeaderView", true);
    }
    // Auto generated code (Tools/params_utils.py:244)
    static void updateResizableColumn(TreeParamsP *self) {
        auto v = self->handle->GetBool("ResizableColumn", false);
        if (self->ResizableColumn != v) {
            self->ResizableColumn = v;
            TreeParams::onResizableColumnChanged();
        }
    }
    // Auto generated code (Tools/params_utils.py:238)
    static void updateColumnSize1(TreeParamsP *self) {
        self->ColumnSize1 = self->handle->GetInt("ColumnSize1", 0);
    }
    // Auto generated code (Tools/params_utils.py:238)
    static void updateColumnSize2(TreeParamsP *self) {
        self->ColumnSize2 = self->handle->GetInt("ColumnSize2", 0);
    }
    // Auto generated code (Tools/params_utils.py:238)
    static void updateTreeToolTipIcon(TreeParamsP *self) {
        self->TreeToolTipIcon = self->handle->GetBool("TreeToolTipIcon", false);
    }
};

// Auto generated code (Tools/params_utils.py:256)
TreeParamsP *instance() {
    static TreeParamsP *inst = new TreeParamsP;
    return inst;
}

} // Anonymous namespace

// Auto generated code (Tools/params_utils.py:265)
ParameterGrp::handle TreeParams::getHandle() {
    return instance()->handle;
}

// Auto generated code (Tools/params_utils.py:288)
const char *TreeParams::docSyncSelection() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const bool & TreeParams::getSyncSelection() {
    return instance()->SyncSelection;
}

// Auto generated code (Tools/params_utils.py:300)
const bool & TreeParams::defaultSyncSelection() {
    const static bool def = true;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void TreeParams::setSyncSelection(const bool &v) {
    instance()->handle->SetBool("SyncSelection",v);
    instance()->SyncSelection = v;
}

// Auto generated code (Tools/params_utils.py:314)
void TreeParams::removeSyncSelection() {
    instance()->handle->RemoveBool("SyncSelection");
}

// Auto generated code (Tools/params_utils.py:288)
const char *TreeParams::docCheckBoxesSelection() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const bool & TreeParams::getCheckBoxesSelection() {
    return instance()->CheckBoxesSelection;
}

// Auto generated code (Tools/params_utils.py:300)
const bool & TreeParams::defaultCheckBoxesSelection() {
    const static bool def = false;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void TreeParams::setCheckBoxesSelection(const bool &v) {
    instance()->handle->SetBool("CheckBoxesSelection",v);
    instance()->CheckBoxesSelection = v;
}

// Auto generated code (Tools/params_utils.py:314)
void TreeParams::removeCheckBoxesSelection() {
    instance()->handle->RemoveBool("CheckBoxesSelection");
}

// Auto generated code (Tools/params_utils.py:288)
const char *TreeParams::docSyncView() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const bool & TreeParams::getSyncView() {
    return instance()->SyncView;
}

// Auto generated code (Tools/params_utils.py:300)
const bool & TreeParams::defaultSyncView() {
    const static bool def = true;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void TreeParams::setSyncView(const bool &v) {
    instance()->handle->SetBool("SyncView",v);
    instance()->SyncView = v;
}

// Auto generated code (Tools/params_utils.py:314)
void TreeParams::removeSyncView() {
    instance()->handle->RemoveBool("SyncView");
}

// Auto generated code (Tools/params_utils.py:288)
const char *TreeParams::docPreSelection() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const bool & TreeParams::getPreSelection() {
    return instance()->PreSelection;
}

// Auto generated code (Tools/params_utils.py:300)
const bool & TreeParams::defaultPreSelection() {
    const static bool def = true;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void TreeParams::setPreSelection(const bool &v) {
    instance()->handle->SetBool("PreSelection",v);
    instance()->PreSelection = v;
}

// Auto generated code (Tools/params_utils.py:314)
void TreeParams::removePreSelection() {
    instance()->handle->RemoveBool("PreSelection");
}

// Auto generated code (Tools/params_utils.py:288)
const char *TreeParams::docSyncPlacement() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const bool & TreeParams::getSyncPlacement() {
    return instance()->SyncPlacement;
}

// Auto generated code (Tools/params_utils.py:300)
const bool & TreeParams::defaultSyncPlacement() {
    const static bool def = false;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void TreeParams::setSyncPlacement(const bool &v) {
    instance()->handle->SetBool("SyncPlacement",v);
    instance()->SyncPlacement = v;
}

// Auto generated code (Tools/params_utils.py:314)
void TreeParams::removeSyncPlacement() {
    instance()->handle->RemoveBool("SyncPlacement");
}

// Auto generated code (Tools/params_utils.py:288)
const char *TreeParams::docRecordSelection() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const bool & TreeParams::getRecordSelection() {
    return instance()->RecordSelection;
}

// Auto generated code (Tools/params_utils.py:300)
const bool & TreeParams::defaultRecordSelection() {
    const static bool def = true;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void TreeParams::setRecordSelection(const bool &v) {
    instance()->handle->SetBool("RecordSelection",v);
    instance()->RecordSelection = v;
}

// Auto generated code (Tools/params_utils.py:314)
void TreeParams::removeRecordSelection() {
    instance()->handle->RemoveBool("RecordSelection");
}

// Auto generated code (Tools/params_utils.py:288)
const char *TreeParams::docDocumentMode() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const long & TreeParams::getDocumentMode() {
    return instance()->DocumentMode;
}

// Auto generated code (Tools/params_utils.py:300)
const long & TreeParams::defaultDocumentMode() {
    const static long def = 2;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void TreeParams::setDocumentMode(const long &v) {
    instance()->handle->SetInt("DocumentMode",v);
    instance()->DocumentMode = v;
}

// Auto generated code (Tools/params_utils.py:314)
void TreeParams::removeDocumentMode() {
    instance()->handle->RemoveInt("DocumentMode");
}

// Auto generated code (Tools/params_utils.py:288)
const char *TreeParams::docStatusTimeout() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const long & TreeParams::getStatusTimeout() {
    return instance()->StatusTimeout;
}

// Auto generated code (Tools/params_utils.py:300)
const long & TreeParams::defaultStatusTimeout() {
    const static long def = 100;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void TreeParams::setStatusTimeout(const long &v) {
    instance()->handle->SetInt("StatusTimeout",v);
    instance()->StatusTimeout = v;
}

// Auto generated code (Tools/params_utils.py:314)
void TreeParams::removeStatusTimeout() {
    instance()->handle->RemoveInt("StatusTimeout");
}

// Auto generated code (Tools/params_utils.py:288)
const char *TreeParams::docSelectionTimeout() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const long & TreeParams::getSelectionTimeout() {
    return instance()->SelectionTimeout;
}

// Auto generated code (Tools/params_utils.py:300)
const long & TreeParams::defaultSelectionTimeout() {
    const static long def = 100;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void TreeParams::setSelectionTimeout(const long &v) {
    instance()->handle->SetInt("SelectionTimeout",v);
    instance()->SelectionTimeout = v;
}

// Auto generated code (Tools/params_utils.py:314)
void TreeParams::removeSelectionTimeout() {
    instance()->handle->RemoveInt("SelectionTimeout");
}

// Auto generated code (Tools/params_utils.py:288)
const char *TreeParams::docPreSelectionTimeout() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const long & TreeParams::getPreSelectionTimeout() {
    return instance()->PreSelectionTimeout;
}

// Auto generated code (Tools/params_utils.py:300)
const long & TreeParams::defaultPreSelectionTimeout() {
    const static long def = 500;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void TreeParams::setPreSelectionTimeout(const long &v) {
    instance()->handle->SetInt("PreSelectionTimeout",v);
    instance()->PreSelectionTimeout = v;
}

// Auto generated code (Tools/params_utils.py:314)
void TreeParams::removePreSelectionTimeout() {
    instance()->handle->RemoveInt("PreSelectionTimeout");
}

// Auto generated code (Tools/params_utils.py:288)
const char *TreeParams::docPreSelectionDelay() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const long & TreeParams::getPreSelectionDelay() {
    return instance()->PreSelectionDelay;
}

// Auto generated code (Tools/params_utils.py:300)
const long & TreeParams::defaultPreSelectionDelay() {
    const static long def = 700;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void TreeParams::setPreSelectionDelay(const long &v) {
    instance()->handle->SetInt("PreSelectionDelay",v);
    instance()->PreSelectionDelay = v;
}

// Auto generated code (Tools/params_utils.py:314)
void TreeParams::removePreSelectionDelay() {
    instance()->handle->RemoveInt("PreSelectionDelay");
}

// Auto generated code (Tools/params_utils.py:288)
const char *TreeParams::docPreSelectionMinDelay() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const long & TreeParams::getPreSelectionMinDelay() {
    return instance()->PreSelectionMinDelay;
}

// Auto generated code (Tools/params_utils.py:300)
const long & TreeParams::defaultPreSelectionMinDelay() {
    const static long def = 200;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void TreeParams::setPreSelectionMinDelay(const long &v) {
    instance()->handle->SetInt("PreSelectionMinDelay",v);
    instance()->PreSelectionMinDelay = v;
}

// Auto generated code (Tools/params_utils.py:314)
void TreeParams::removePreSelectionMinDelay() {
    instance()->handle->RemoveInt("PreSelectionMinDelay");
}

// Auto generated code (Tools/params_utils.py:288)
const char *TreeParams::docRecomputeOnDrop() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const bool & TreeParams::getRecomputeOnDrop() {
    return instance()->RecomputeOnDrop;
}

// Auto generated code (Tools/params_utils.py:300)
const bool & TreeParams::defaultRecomputeOnDrop() {
    const static bool def = true;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void TreeParams::setRecomputeOnDrop(const bool &v) {
    instance()->handle->SetBool("RecomputeOnDrop",v);
    instance()->RecomputeOnDrop = v;
}

// Auto generated code (Tools/params_utils.py:314)
void TreeParams::removeRecomputeOnDrop() {
    instance()->handle->RemoveBool("RecomputeOnDrop");
}

// Auto generated code (Tools/params_utils.py:288)
const char *TreeParams::docKeepRootOrder() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const bool & TreeParams::getKeepRootOrder() {
    return instance()->KeepRootOrder;
}

// Auto generated code (Tools/params_utils.py:300)
const bool & TreeParams::defaultKeepRootOrder() {
    const static bool def = true;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void TreeParams::setKeepRootOrder(const bool &v) {
    instance()->handle->SetBool("KeepRootOrder",v);
    instance()->KeepRootOrder = v;
}

// Auto generated code (Tools/params_utils.py:314)
void TreeParams::removeKeepRootOrder() {
    instance()->handle->RemoveBool("KeepRootOrder");
}

// Auto generated code (Tools/params_utils.py:288)
const char *TreeParams::docTreeActiveAutoExpand() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const bool & TreeParams::getTreeActiveAutoExpand() {
    return instance()->TreeActiveAutoExpand;
}

// Auto generated code (Tools/params_utils.py:300)
const bool & TreeParams::defaultTreeActiveAutoExpand() {
    const static bool def = true;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void TreeParams::setTreeActiveAutoExpand(const bool &v) {
    instance()->handle->SetBool("TreeActiveAutoExpand",v);
    instance()->TreeActiveAutoExpand = v;
}

// Auto generated code (Tools/params_utils.py:314)
void TreeParams::removeTreeActiveAutoExpand() {
    instance()->handle->RemoveBool("TreeActiveAutoExpand");
}

// Auto generated code (Tools/params_utils.py:288)
const char *TreeParams::docTreeActiveColor() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const unsigned long & TreeParams::getTreeActiveColor() {
    return instance()->TreeActiveColor;
}

// Auto generated code (Tools/params_utils.py:300)
const unsigned long & TreeParams::defaultTreeActiveColor() {
    const static unsigned long def = 3873898495;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void TreeParams::setTreeActiveColor(const unsigned long &v) {
    instance()->handle->SetUnsigned("TreeActiveColor",v);
    instance()->TreeActiveColor = v;
}

// Auto generated code (Tools/params_utils.py:314)
void TreeParams::removeTreeActiveColor() {
    instance()->handle->RemoveUnsigned("TreeActiveColor");
}

// Auto generated code (Tools/params_utils.py:288)
const char *TreeParams::docTreeEditColor() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const unsigned long & TreeParams::getTreeEditColor() {
    return instance()->TreeEditColor;
}

// Auto generated code (Tools/params_utils.py:300)
const unsigned long & TreeParams::defaultTreeEditColor() {
    const static unsigned long def = 2459042047;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void TreeParams::setTreeEditColor(const unsigned long &v) {
    instance()->handle->SetUnsigned("TreeEditColor",v);
    instance()->TreeEditColor = v;
}

// Auto generated code (Tools/params_utils.py:314)
void TreeParams::removeTreeEditColor() {
    instance()->handle->RemoveUnsigned("TreeEditColor");
}

// Auto generated code (Tools/params_utils.py:288)
const char *TreeParams::docSelectingGroupColor() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const unsigned long & TreeParams::getSelectingGroupColor() {
    return instance()->SelectingGroupColor;
}

// Auto generated code (Tools/params_utils.py:300)
const unsigned long & TreeParams::defaultSelectingGroupColor() {
    const static unsigned long def = 1082163711;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void TreeParams::setSelectingGroupColor(const unsigned long &v) {
    instance()->handle->SetUnsigned("SelectingGroupColor",v);
    instance()->SelectingGroupColor = v;
}

// Auto generated code (Tools/params_utils.py:314)
void TreeParams::removeSelectingGroupColor() {
    instance()->handle->RemoveUnsigned("SelectingGroupColor");
}

// Auto generated code (Tools/params_utils.py:288)
const char *TreeParams::docTreeActiveBold() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const bool & TreeParams::getTreeActiveBold() {
    return instance()->TreeActiveBold;
}

// Auto generated code (Tools/params_utils.py:300)
const bool & TreeParams::defaultTreeActiveBold() {
    const static bool def = true;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void TreeParams::setTreeActiveBold(const bool &v) {
    instance()->handle->SetBool("TreeActiveBold",v);
    instance()->TreeActiveBold = v;
}

// Auto generated code (Tools/params_utils.py:314)
void TreeParams::removeTreeActiveBold() {
    instance()->handle->RemoveBool("TreeActiveBold");
}

// Auto generated code (Tools/params_utils.py:288)
const char *TreeParams::docTreeActiveItalic() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const bool & TreeParams::getTreeActiveItalic() {
    return instance()->TreeActiveItalic;
}

// Auto generated code (Tools/params_utils.py:300)
const bool & TreeParams::defaultTreeActiveItalic() {
    const static bool def = false;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void TreeParams::setTreeActiveItalic(const bool &v) {
    instance()->handle->SetBool("TreeActiveItalic",v);
    instance()->TreeActiveItalic = v;
}

// Auto generated code (Tools/params_utils.py:314)
void TreeParams::removeTreeActiveItalic() {
    instance()->handle->RemoveBool("TreeActiveItalic");
}

// Auto generated code (Tools/params_utils.py:288)
const char *TreeParams::docTreeActiveUnderlined() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const bool & TreeParams::getTreeActiveUnderlined() {
    return instance()->TreeActiveUnderlined;
}

// Auto generated code (Tools/params_utils.py:300)
const bool & TreeParams::defaultTreeActiveUnderlined() {
    const static bool def = false;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void TreeParams::setTreeActiveUnderlined(const bool &v) {
    instance()->handle->SetBool("TreeActiveUnderlined",v);
    instance()->TreeActiveUnderlined = v;
}

// Auto generated code (Tools/params_utils.py:314)
void TreeParams::removeTreeActiveUnderlined() {
    instance()->handle->RemoveBool("TreeActiveUnderlined");
}

// Auto generated code (Tools/params_utils.py:288)
const char *TreeParams::docTreeActiveOverlined() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const bool & TreeParams::getTreeActiveOverlined() {
    return instance()->TreeActiveOverlined;
}

// Auto generated code (Tools/params_utils.py:300)
const bool & TreeParams::defaultTreeActiveOverlined() {
    const static bool def = false;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void TreeParams::setTreeActiveOverlined(const bool &v) {
    instance()->handle->SetBool("TreeActiveOverlined",v);
    instance()->TreeActiveOverlined = v;
}

// Auto generated code (Tools/params_utils.py:314)
void TreeParams::removeTreeActiveOverlined() {
    instance()->handle->RemoveBool("TreeActiveOverlined");
}

// Auto generated code (Tools/params_utils.py:288)
const char *TreeParams::docIndentation() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const long & TreeParams::getIndentation() {
    return instance()->Indentation;
}

// Auto generated code (Tools/params_utils.py:300)
const long & TreeParams::defaultIndentation() {
    const static long def = 0;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void TreeParams::setIndentation(const long &v) {
    instance()->handle->SetInt("Indentation",v);
    instance()->Indentation = v;
}

// Auto generated code (Tools/params_utils.py:314)
void TreeParams::removeIndentation() {
    instance()->handle->RemoveInt("Indentation");
}

// Auto generated code (Tools/params_utils.py:288)
const char *TreeParams::docLabelExpression() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const bool & TreeParams::getLabelExpression() {
    return instance()->LabelExpression;
}

// Auto generated code (Tools/params_utils.py:300)
const bool & TreeParams::defaultLabelExpression() {
    const static bool def = false;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void TreeParams::setLabelExpression(const bool &v) {
    instance()->handle->SetBool("LabelExpression",v);
    instance()->LabelExpression = v;
}

// Auto generated code (Tools/params_utils.py:314)
void TreeParams::removeLabelExpression() {
    instance()->handle->RemoveBool("LabelExpression");
}

// Auto generated code (Tools/params_utils.py:288)
const char *TreeParams::docIconSize() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const long & TreeParams::getIconSize() {
    return instance()->IconSize;
}

// Auto generated code (Tools/params_utils.py:300)
const long & TreeParams::defaultIconSize() {
    const static long def = 0;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void TreeParams::setIconSize(const long &v) {
    instance()->handle->SetInt("IconSize",v);
    instance()->IconSize = v;
}

// Auto generated code (Tools/params_utils.py:314)
void TreeParams::removeIconSize() {
    instance()->handle->RemoveInt("IconSize");
}

// Auto generated code (Tools/params_utils.py:288)
const char *TreeParams::docFontSize() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const long & TreeParams::getFontSize() {
    return instance()->FontSize;
}

// Auto generated code (Tools/params_utils.py:300)
const long & TreeParams::defaultFontSize() {
    const static long def = 0;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void TreeParams::setFontSize(const long &v) {
    instance()->handle->SetInt("FontSize",v);
    instance()->FontSize = v;
}

// Auto generated code (Tools/params_utils.py:314)
void TreeParams::removeFontSize() {
    instance()->handle->RemoveInt("FontSize");
}

// Auto generated code (Tools/params_utils.py:288)
const char *TreeParams::docItemSpacing() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const long & TreeParams::getItemSpacing() {
    return instance()->ItemSpacing;
}

// Auto generated code (Tools/params_utils.py:300)
const long & TreeParams::defaultItemSpacing() {
    const static long def = 0;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void TreeParams::setItemSpacing(const long &v) {
    instance()->handle->SetInt("ItemSpacing",v);
    instance()->ItemSpacing = v;
}

// Auto generated code (Tools/params_utils.py:314)
void TreeParams::removeItemSpacing() {
    instance()->handle->RemoveInt("ItemSpacing");
}

// Auto generated code (Tools/params_utils.py:288)
const char *TreeParams::docItemBackground() {
    return QT_TRANSLATE_NOOP("TreeParams",
"Tree view item background. Only effective in overlay.");
}

// Auto generated code (Tools/params_utils.py:294)
const unsigned long & TreeParams::getItemBackground() {
    return instance()->ItemBackground;
}

// Auto generated code (Tools/params_utils.py:300)
const unsigned long & TreeParams::defaultItemBackground() {
    const static unsigned long def = 0x00000000;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void TreeParams::setItemBackground(const unsigned long &v) {
    instance()->handle->SetUnsigned("ItemBackground",v);
    instance()->ItemBackground = v;
}

// Auto generated code (Tools/params_utils.py:314)
void TreeParams::removeItemBackground() {
    instance()->handle->RemoveUnsigned("ItemBackground");
}

// Auto generated code (Tools/params_utils.py:288)
const char *TreeParams::docItemBackgroundPadding() {
    return QT_TRANSLATE_NOOP("TreeParams",
"Tree view item background padding.");
}

// Auto generated code (Tools/params_utils.py:294)
const long & TreeParams::getItemBackgroundPadding() {
    return instance()->ItemBackgroundPadding;
}

// Auto generated code (Tools/params_utils.py:300)
const long & TreeParams::defaultItemBackgroundPadding() {
    const static long def = 10;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void TreeParams::setItemBackgroundPadding(const long &v) {
    instance()->handle->SetInt("ItemBackgroundPadding",v);
    instance()->ItemBackgroundPadding = v;
}

// Auto generated code (Tools/params_utils.py:314)
void TreeParams::removeItemBackgroundPadding() {
    instance()->handle->RemoveInt("ItemBackgroundPadding");
}

// Auto generated code (Tools/params_utils.py:288)
const char *TreeParams::docHideColumn() {
    return QT_TRANSLATE_NOOP("TreeParams",
"Hide extra tree view column for item description.");
}

// Auto generated code (Tools/params_utils.py:294)
const bool & TreeParams::getHideColumn() {
    return instance()->HideColumn;
}

// Auto generated code (Tools/params_utils.py:300)
const bool & TreeParams::defaultHideColumn() {
    const static bool def = true;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void TreeParams::setHideColumn(const bool &v) {
    instance()->handle->SetBool("HideColumn",v);
    instance()->HideColumn = v;
}

// Auto generated code (Tools/params_utils.py:314)
void TreeParams::removeHideColumn() {
    instance()->handle->RemoveBool("HideColumn");
}

// Auto generated code (Tools/params_utils.py:288)
const char *TreeParams::docHideScrollBar() {
    return QT_TRANSLATE_NOOP("TreeParams",
"Hide tree view scroll bar in dock overlay.");
}

// Auto generated code (Tools/params_utils.py:294)
const bool & TreeParams::getHideScrollBar() {
    return instance()->HideScrollBar;
}

// Auto generated code (Tools/params_utils.py:300)
const bool & TreeParams::defaultHideScrollBar() {
    const static bool def = true;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void TreeParams::setHideScrollBar(const bool &v) {
    instance()->handle->SetBool("HideScrollBar",v);
    instance()->HideScrollBar = v;
}

// Auto generated code (Tools/params_utils.py:314)
void TreeParams::removeHideScrollBar() {
    instance()->handle->RemoveBool("HideScrollBar");
}

// Auto generated code (Tools/params_utils.py:288)
const char *TreeParams::docHideHeaderView() {
    return QT_TRANSLATE_NOOP("TreeParams",
"Hide tree view header view in dock overlay.");
}

// Auto generated code (Tools/params_utils.py:294)
const bool & TreeParams::getHideHeaderView() {
    return instance()->HideHeaderView;
}

// Auto generated code (Tools/params_utils.py:300)
const bool & TreeParams::defaultHideHeaderView() {
    const static bool def = true;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void TreeParams::setHideHeaderView(const bool &v) {
    instance()->handle->SetBool("HideHeaderView",v);
    instance()->HideHeaderView = v;
}

// Auto generated code (Tools/params_utils.py:314)
void TreeParams::removeHideHeaderView() {
    instance()->handle->RemoveBool("HideHeaderView");
}

// Auto generated code (Tools/params_utils.py:288)
const char *TreeParams::docResizableColumn() {
    return QT_TRANSLATE_NOOP("TreeParams",
"Allow tree view columns to be manually resized.");
}

// Auto generated code (Tools/params_utils.py:294)
const bool & TreeParams::getResizableColumn() {
    return instance()->ResizableColumn;
}

// Auto generated code (Tools/params_utils.py:300)
const bool & TreeParams::defaultResizableColumn() {
    const static bool def = false;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void TreeParams::setResizableColumn(const bool &v) {
    instance()->handle->SetBool("ResizableColumn",v);
    instance()->ResizableColumn = v;
}

// Auto generated code (Tools/params_utils.py:314)
void TreeParams::removeResizableColumn() {
    instance()->handle->RemoveBool("ResizableColumn");
}

// Auto generated code (Tools/params_utils.py:288)
const char *TreeParams::docColumnSize1() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const long & TreeParams::getColumnSize1() {
    return instance()->ColumnSize1;
}

// Auto generated code (Tools/params_utils.py:300)
const long & TreeParams::defaultColumnSize1() {
    const static long def = 0;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void TreeParams::setColumnSize1(const long &v) {
    instance()->handle->SetInt("ColumnSize1",v);
    instance()->ColumnSize1 = v;
}

// Auto generated code (Tools/params_utils.py:314)
void TreeParams::removeColumnSize1() {
    instance()->handle->RemoveInt("ColumnSize1");
}

// Auto generated code (Tools/params_utils.py:288)
const char *TreeParams::docColumnSize2() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const long & TreeParams::getColumnSize2() {
    return instance()->ColumnSize2;
}

// Auto generated code (Tools/params_utils.py:300)
const long & TreeParams::defaultColumnSize2() {
    const static long def = 0;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void TreeParams::setColumnSize2(const long &v) {
    instance()->handle->SetInt("ColumnSize2",v);
    instance()->ColumnSize2 = v;
}

// Auto generated code (Tools/params_utils.py:314)
void TreeParams::removeColumnSize2() {
    instance()->handle->RemoveInt("ColumnSize2");
}

// Auto generated code (Tools/params_utils.py:288)
const char *TreeParams::docTreeToolTipIcon() {
    return "";
}

// Auto generated code (Tools/params_utils.py:294)
const bool & TreeParams::getTreeToolTipIcon() {
    return instance()->TreeToolTipIcon;
}

// Auto generated code (Tools/params_utils.py:300)
const bool & TreeParams::defaultTreeToolTipIcon() {
    const static bool def = false;
    return def;
}

// Auto generated code (Tools/params_utils.py:307)
void TreeParams::setTreeToolTipIcon(const bool &v) {
    instance()->handle->SetBool("TreeToolTipIcon",v);
    instance()->TreeToolTipIcon = v;
}

// Auto generated code (Tools/params_utils.py:314)
void TreeParams::removeTreeToolTipIcon() {
    instance()->handle->RemoveBool("TreeToolTipIcon");
}
//[[[end]]]

void TreeParams::onSyncSelectionChanged() {
    if(!TreeParams::getSyncSelection() || !Gui::Selection().hasSelection())
        return;
    TreeWidget::scrollItemToTop();
}

void TreeParams::onCheckBoxesSelectionChanged()
{
    TreeWidget::synchronizeSelectionCheckBoxes();
}

void TreeParams::onDocumentModeChanged() {
    App::GetApplication().setActiveDocument(App::GetApplication().getActiveDocument());
}

void TreeParams::onResizableColumnChanged() {
    TreeWidget::setupResizableColumn();
}

void TreeParams::onIconSizeChanged() {
    // auto tree = TreeWidget::instance();
    // Commented out temporarily while merging PR #7888
    //if (tree)
        //tree->setIconHeight(TreeParams::getIconSize());
}

void TreeParams::onFontSizeChanged() {
    int fontSize = TreeParams::getFontSize();
    if (fontSize <= 0)
        return;
    for(auto tree : TreeWidget::Instances) {
        QFont font = tree->font();
        font.setPointSize(std::max(8,fontSize));
        tree->setFont(font);
    }
}

void TreeParams::onItemSpacingChanged()
{
    refreshTreeViews();
}

void TreeParams::refreshTreeViews()
{
    for(auto tree : TreeWidget::Instances) {
        tree->scheduleDelayedItemsLayout();
    }
}

void TreeParams::onTreeActiveColorChanged()
{
    refreshTreeViews();
}

void TreeParams::onTreeEditColorChanged()
{
    refreshTreeViews();
}

void TreeParams::onSelectingGroupColorChanged()
{
    refreshTreeViews();
}

void TreeParams::onTreeActiveBoldChanged()
{
    refreshTreeViews();
}

void TreeParams::onTreeActiveItalicChanged()
{
    refreshTreeViews();
}

void TreeParams::onTreeActiveUnderlinedChanged()
{
    refreshTreeViews();
}

void TreeParams::onTreeActiveOverlinedChanged()
{
    refreshTreeViews();
}

void TreeParams::onIndentationChanged()
{
    refreshTreeViews();
}

void TreeParams::onItemBackgroundPaddingChanged()
{
    if (getItemBackground())
        refreshTreeViews();
}

void TreeParams::onHideColumnChanged()
{
    for(auto tree : TreeWidget::Instances)
        tree->setColumnHidden(1, TreeParams::getHideColumn());
}
