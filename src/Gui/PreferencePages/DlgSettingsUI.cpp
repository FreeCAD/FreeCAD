/****************************************************************************
 *   Copyright (c) 2020 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
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

#include <QPropertyAnimation>
#include <QTimer>

/*[[[cog
import DlgSettingsUI
DlgSettingsUI.define()
]]]*/

// Auto generated code (Tools/params_utils.py:567)
#ifndef _PreComp_
#   include <QApplication>
#   include <QLabel>
#   include <QGroupBox>
#   include <QGridLayout>
#   include <QVBoxLayout>
#   include <QHBoxLayout>
#endif
#include <Gui/TreeParams.h>
#include <Gui/OverlayParams.h>
// Auto generated code (Tools/params_utils.py:589)
#include "Gui/PreferencePages/DlgSettingsUI.h"
using namespace Gui::Dialog;
/* TRANSLATOR Gui::Dialog::DlgSettingsUI */

// Auto generated code (Tools/params_utils.py:598)
DlgSettingsUI::DlgSettingsUI(QWidget* parent)
    : PreferencePage( parent )
{

    auto layout = new QVBoxLayout(this);


    // Auto generated code (Tools/params_utils.py:420)
    groupTreeview = new QGroupBox(this);
    layout->addWidget(groupTreeview);
    auto layoutHorizTreeview = new QHBoxLayout(groupTreeview);
    auto layoutTreeview = new QGridLayout();
    layoutHorizTreeview->addLayout(layoutTreeview);
    layoutHorizTreeview->addStretch();

    // Auto generated code (Tools/params_utils.py:433)
    labelItemBackground = new QLabel(this);
    layoutTreeview->addWidget(labelItemBackground, 0, 0);
    ItemBackground = new Gui::PrefColorButton(this);
    layoutTreeview->addWidget(ItemBackground, 0, 1);
    ItemBackground->setPackedColor(Gui::TreeParams::defaultItemBackground());
    ItemBackground->setEntryName("ItemBackground");
    ItemBackground->setParamGrpPath("TreeView");
    ItemBackground->setAllowTransparency(true);

    // Auto generated code (Tools/params_utils.py:433)
    labelItemBackgroundPadding = new QLabel(this);
    layoutTreeview->addWidget(labelItemBackgroundPadding, 1, 0);
    ItemBackgroundPadding = new Gui::PrefSpinBox(this);
    layoutTreeview->addWidget(ItemBackgroundPadding, 1, 1);
    ItemBackgroundPadding->setValue(Gui::TreeParams::defaultItemBackgroundPadding());
    ItemBackgroundPadding->setEntryName("ItemBackgroundPadding");
    ItemBackgroundPadding->setParamGrpPath("TreeView");
    // Auto generated code (Tools/params_utils.py:1134)
    ItemBackgroundPadding->setMinimum(0);
    ItemBackgroundPadding->setMaximum(100);
    ItemBackgroundPadding->setSingleStep(1);

    // Auto generated code (Tools/params_utils.py:433)
    ResizableColumn = new Gui::PrefCheckBox(this);
    layoutTreeview->addWidget(ResizableColumn, 2, 0);
    ResizableColumn->setChecked(Gui::TreeParams::defaultResizableColumn());
    ResizableColumn->setEntryName("ResizableColumn");
    ResizableColumn->setParamGrpPath("TreeView");

    // Auto generated code (Tools/params_utils.py:433)
    CheckBoxesSelection = new Gui::PrefCheckBox(this);
    layoutTreeview->addWidget(CheckBoxesSelection, 3, 0);
    CheckBoxesSelection->setChecked(Gui::TreeParams::defaultCheckBoxesSelection());
    CheckBoxesSelection->setEntryName("CheckBoxesSelection");
    CheckBoxesSelection->setParamGrpPath("TreeView");

    // Auto generated code (Tools/params_utils.py:433)
    HideColumn = new Gui::PrefCheckBox(this);
    layoutTreeview->addWidget(HideColumn, 4, 0);
    HideColumn->setChecked(Gui::TreeParams::defaultHideColumn());
    HideColumn->setEntryName("HideColumn");
    HideColumn->setParamGrpPath("TreeView");

    // Auto generated code (Tools/params_utils.py:433)
    HideScrollBar = new Gui::PrefCheckBox(this);
    layoutTreeview->addWidget(HideScrollBar, 5, 0);
    HideScrollBar->setChecked(Gui::TreeParams::defaultHideScrollBar());
    HideScrollBar->setEntryName("HideScrollBar");
    HideScrollBar->setParamGrpPath("TreeView");

    // Auto generated code (Tools/params_utils.py:433)
    HideHeaderView = new Gui::PrefCheckBox(this);
    layoutTreeview->addWidget(HideHeaderView, 6, 0);
    HideHeaderView->setChecked(Gui::TreeParams::defaultHideHeaderView());
    HideHeaderView->setEntryName("HideHeaderView");
    HideHeaderView->setParamGrpPath("TreeView");

    // Auto generated code (Tools/params_utils.py:433)
    labelIconSize = new QLabel(this);
    layoutTreeview->addWidget(labelIconSize, 7, 0);
    IconSize = new Gui::PrefSpinBox(this);
    layoutTreeview->addWidget(IconSize, 7, 1);
    IconSize->setValue(Gui::TreeParams::defaultIconSize());
    IconSize->setEntryName("IconSize");
    IconSize->setParamGrpPath("TreeView");

    // Auto generated code (Tools/params_utils.py:433)
    labelFontSize = new QLabel(this);
    layoutTreeview->addWidget(labelFontSize, 8, 0);
    FontSize = new Gui::PrefSpinBox(this);
    layoutTreeview->addWidget(FontSize, 8, 1);
    FontSize->setValue(Gui::TreeParams::defaultFontSize());
    FontSize->setEntryName("FontSize");
    FontSize->setParamGrpPath("TreeView");

    // Auto generated code (Tools/params_utils.py:433)
    labelItemSpacing = new QLabel(this);
    layoutTreeview->addWidget(labelItemSpacing, 9, 0);
    ItemSpacing = new Gui::PrefSpinBox(this);
    layoutTreeview->addWidget(ItemSpacing, 9, 1);
    ItemSpacing->setValue(Gui::TreeParams::defaultItemSpacing());
    ItemSpacing->setEntryName("ItemSpacing");
    ItemSpacing->setParamGrpPath("TreeView");


    // Auto generated code (Tools/params_utils.py:420)
    groupOverlay = new QGroupBox(this);
    layout->addWidget(groupOverlay);
    auto layoutHorizOverlay = new QHBoxLayout(groupOverlay);
    auto layoutOverlay = new QGridLayout();
    layoutHorizOverlay->addLayout(layoutOverlay);
    layoutHorizOverlay->addStretch();

    // Auto generated code (Tools/params_utils.py:433)
    DockOverlayHideTabBar = new Gui::PrefCheckBox(this);
    layoutOverlay->addWidget(DockOverlayHideTabBar, 0, 0);
    DockOverlayHideTabBar->setChecked(Gui::OverlayParams::defaultDockOverlayHideTabBar());
    DockOverlayHideTabBar->setEntryName("DockOverlayHideTabBar");
    DockOverlayHideTabBar->setParamGrpPath("View");

    // Auto generated code (Tools/params_utils.py:433)
    DockOverlayHidePropertyViewScrollBar = new Gui::PrefCheckBox(this);
    layoutOverlay->addWidget(DockOverlayHidePropertyViewScrollBar, 1, 0);
    DockOverlayHidePropertyViewScrollBar->setChecked(Gui::OverlayParams::defaultDockOverlayHidePropertyViewScrollBar());
    DockOverlayHidePropertyViewScrollBar->setEntryName("DockOverlayHidePropertyViewScrollBar");
    DockOverlayHidePropertyViewScrollBar->setParamGrpPath("View");

    // Auto generated code (Tools/params_utils.py:433)
    DockOverlayAutoView = new Gui::PrefCheckBox(this);
    layoutOverlay->addWidget(DockOverlayAutoView, 2, 0);
    DockOverlayAutoView->setChecked(Gui::OverlayParams::defaultDockOverlayAutoView());
    DockOverlayAutoView->setEntryName("DockOverlayAutoView");
    DockOverlayAutoView->setParamGrpPath("View");

    // Auto generated code (Tools/params_utils.py:433)
    DockOverlayAutoMouseThrough = new Gui::PrefCheckBox(this);
    layoutOverlay->addWidget(DockOverlayAutoMouseThrough, 3, 0);
    DockOverlayAutoMouseThrough->setChecked(Gui::OverlayParams::defaultDockOverlayAutoMouseThrough());
    DockOverlayAutoMouseThrough->setEntryName("DockOverlayAutoMouseThrough");
    DockOverlayAutoMouseThrough->setParamGrpPath("View");

    // Auto generated code (Tools/params_utils.py:433)
    DockOverlayWheelPassThrough = new Gui::PrefCheckBox(this);
    layoutOverlay->addWidget(DockOverlayWheelPassThrough, 4, 0);
    DockOverlayWheelPassThrough->setChecked(Gui::OverlayParams::defaultDockOverlayWheelPassThrough());
    DockOverlayWheelPassThrough->setEntryName("DockOverlayWheelPassThrough");
    DockOverlayWheelPassThrough->setParamGrpPath("View");

    // Auto generated code (Tools/params_utils.py:433)
    labelDockOverlayWheelDelay = new QLabel(this);
    layoutOverlay->addWidget(labelDockOverlayWheelDelay, 5, 0);
    DockOverlayWheelDelay = new Gui::PrefSpinBox(this);
    layoutOverlay->addWidget(DockOverlayWheelDelay, 5, 1);
    DockOverlayWheelDelay->setValue(Gui::OverlayParams::defaultDockOverlayWheelDelay());
    DockOverlayWheelDelay->setEntryName("DockOverlayWheelDelay");
    DockOverlayWheelDelay->setParamGrpPath("View");
    // Auto generated code (Tools/params_utils.py:1134)
    DockOverlayWheelDelay->setMinimum(0);
    DockOverlayWheelDelay->setMaximum(99999);
    DockOverlayWheelDelay->setSingleStep(1);

    // Auto generated code (Tools/params_utils.py:433)
    labelDockOverlayAlphaRadius = new QLabel(this);
    layoutOverlay->addWidget(labelDockOverlayAlphaRadius, 6, 0);
    DockOverlayAlphaRadius = new Gui::PrefSpinBox(this);
    layoutOverlay->addWidget(DockOverlayAlphaRadius, 6, 1);
    DockOverlayAlphaRadius->setValue(Gui::OverlayParams::defaultDockOverlayAlphaRadius());
    DockOverlayAlphaRadius->setEntryName("DockOverlayAlphaRadius");
    DockOverlayAlphaRadius->setParamGrpPath("View");
    // Auto generated code (Tools/params_utils.py:1134)
    DockOverlayAlphaRadius->setMinimum(1);
    DockOverlayAlphaRadius->setMaximum(100);
    DockOverlayAlphaRadius->setSingleStep(1);

    // Auto generated code (Tools/params_utils.py:433)
    DockOverlayCheckNaviCube = new Gui::PrefCheckBox(this);
    layoutOverlay->addWidget(DockOverlayCheckNaviCube, 7, 0);
    DockOverlayCheckNaviCube->setChecked(Gui::OverlayParams::defaultDockOverlayCheckNaviCube());
    DockOverlayCheckNaviCube->setEntryName("DockOverlayCheckNaviCube");
    DockOverlayCheckNaviCube->setParamGrpPath("View");

    // Auto generated code (Tools/params_utils.py:433)
    labelDockOverlayHintTriggerSize = new QLabel(this);
    layoutOverlay->addWidget(labelDockOverlayHintTriggerSize, 8, 0);
    DockOverlayHintTriggerSize = new Gui::PrefSpinBox(this);
    layoutOverlay->addWidget(DockOverlayHintTriggerSize, 8, 1);
    DockOverlayHintTriggerSize->setValue(Gui::OverlayParams::defaultDockOverlayHintTriggerSize());
    DockOverlayHintTriggerSize->setEntryName("DockOverlayHintTriggerSize");
    DockOverlayHintTriggerSize->setParamGrpPath("View");
    // Auto generated code (Tools/params_utils.py:1134)
    DockOverlayHintTriggerSize->setMinimum(1);
    DockOverlayHintTriggerSize->setMaximum(100);
    DockOverlayHintTriggerSize->setSingleStep(1);

    // Auto generated code (Tools/params_utils.py:433)
    labelDockOverlayHintSize = new QLabel(this);
    layoutOverlay->addWidget(labelDockOverlayHintSize, 9, 0);
    DockOverlayHintSize = new Gui::PrefSpinBox(this);
    layoutOverlay->addWidget(DockOverlayHintSize, 9, 1);
    DockOverlayHintSize->setValue(Gui::OverlayParams::defaultDockOverlayHintSize());
    DockOverlayHintSize->setEntryName("DockOverlayHintSize");
    DockOverlayHintSize->setParamGrpPath("View");
    // Auto generated code (Tools/params_utils.py:1134)
    DockOverlayHintSize->setMinimum(1);
    DockOverlayHintSize->setMaximum(100);
    DockOverlayHintSize->setSingleStep(1);

    // Auto generated code (Tools/params_utils.py:433)
    labelDockOverlayHintLeftOffset = new QLabel(this);
    layoutOverlay->addWidget(labelDockOverlayHintLeftOffset, 10, 0);
    DockOverlayHintLeftOffset = new Gui::PrefSpinBox(this);
    layoutOverlay->addWidget(DockOverlayHintLeftOffset, 10, 1);
    DockOverlayHintLeftOffset->setValue(Gui::OverlayParams::defaultDockOverlayHintLeftOffset());
    DockOverlayHintLeftOffset->setEntryName("DockOverlayHintLeftOffset");
    DockOverlayHintLeftOffset->setParamGrpPath("View");
    // Auto generated code (Tools/params_utils.py:1134)
    DockOverlayHintLeftOffset->setMinimum(0);
    DockOverlayHintLeftOffset->setMaximum(10000);
    DockOverlayHintLeftOffset->setSingleStep(10);

    // Auto generated code (Tools/params_utils.py:433)
    labelDockOverlayHintLeftLength = new QLabel(this);
    layoutOverlay->addWidget(labelDockOverlayHintLeftLength, 11, 0);
    DockOverlayHintLeftLength = new Gui::PrefSpinBox(this);
    layoutOverlay->addWidget(DockOverlayHintLeftLength, 11, 1);
    DockOverlayHintLeftLength->setValue(Gui::OverlayParams::defaultDockOverlayHintLeftLength());
    DockOverlayHintLeftLength->setEntryName("DockOverlayHintLeftLength");
    DockOverlayHintLeftLength->setParamGrpPath("View");
    // Auto generated code (Tools/params_utils.py:1134)
    DockOverlayHintLeftLength->setMinimum(0);
    DockOverlayHintLeftLength->setMaximum(10000);
    DockOverlayHintLeftLength->setSingleStep(10);

    // Auto generated code (Tools/params_utils.py:433)
    labelDockOverlayHintRightOffset = new QLabel(this);
    layoutOverlay->addWidget(labelDockOverlayHintRightOffset, 12, 0);
    DockOverlayHintRightOffset = new Gui::PrefSpinBox(this);
    layoutOverlay->addWidget(DockOverlayHintRightOffset, 12, 1);
    DockOverlayHintRightOffset->setValue(Gui::OverlayParams::defaultDockOverlayHintRightOffset());
    DockOverlayHintRightOffset->setEntryName("DockOverlayHintRightOffset");
    DockOverlayHintRightOffset->setParamGrpPath("View");
    // Auto generated code (Tools/params_utils.py:1134)
    DockOverlayHintRightOffset->setMinimum(0);
    DockOverlayHintRightOffset->setMaximum(10000);
    DockOverlayHintRightOffset->setSingleStep(10);

    // Auto generated code (Tools/params_utils.py:433)
    labelDockOverlayHintRightLength = new QLabel(this);
    layoutOverlay->addWidget(labelDockOverlayHintRightLength, 13, 0);
    DockOverlayHintRightLength = new Gui::PrefSpinBox(this);
    layoutOverlay->addWidget(DockOverlayHintRightLength, 13, 1);
    DockOverlayHintRightLength->setValue(Gui::OverlayParams::defaultDockOverlayHintRightLength());
    DockOverlayHintRightLength->setEntryName("DockOverlayHintRightLength");
    DockOverlayHintRightLength->setParamGrpPath("View");
    // Auto generated code (Tools/params_utils.py:1134)
    DockOverlayHintRightLength->setMinimum(0);
    DockOverlayHintRightLength->setMaximum(10000);
    DockOverlayHintRightLength->setSingleStep(10);

    // Auto generated code (Tools/params_utils.py:433)
    labelDockOverlayHintTopOffset = new QLabel(this);
    layoutOverlay->addWidget(labelDockOverlayHintTopOffset, 14, 0);
    DockOverlayHintTopOffset = new Gui::PrefSpinBox(this);
    layoutOverlay->addWidget(DockOverlayHintTopOffset, 14, 1);
    DockOverlayHintTopOffset->setValue(Gui::OverlayParams::defaultDockOverlayHintTopOffset());
    DockOverlayHintTopOffset->setEntryName("DockOverlayHintTopOffset");
    DockOverlayHintTopOffset->setParamGrpPath("View");
    // Auto generated code (Tools/params_utils.py:1134)
    DockOverlayHintTopOffset->setMinimum(0);
    DockOverlayHintTopOffset->setMaximum(10000);
    DockOverlayHintTopOffset->setSingleStep(10);

    // Auto generated code (Tools/params_utils.py:433)
    labelDockOverlayHintTopLength = new QLabel(this);
    layoutOverlay->addWidget(labelDockOverlayHintTopLength, 15, 0);
    DockOverlayHintTopLength = new Gui::PrefSpinBox(this);
    layoutOverlay->addWidget(DockOverlayHintTopLength, 15, 1);
    DockOverlayHintTopLength->setValue(Gui::OverlayParams::defaultDockOverlayHintTopLength());
    DockOverlayHintTopLength->setEntryName("DockOverlayHintTopLength");
    DockOverlayHintTopLength->setParamGrpPath("View");
    // Auto generated code (Tools/params_utils.py:1134)
    DockOverlayHintTopLength->setMinimum(0);
    DockOverlayHintTopLength->setMaximum(10000);
    DockOverlayHintTopLength->setSingleStep(10);

    // Auto generated code (Tools/params_utils.py:433)
    labelDockOverlayHintBottomOffset = new QLabel(this);
    layoutOverlay->addWidget(labelDockOverlayHintBottomOffset, 16, 0);
    DockOverlayHintBottomOffset = new Gui::PrefSpinBox(this);
    layoutOverlay->addWidget(DockOverlayHintBottomOffset, 16, 1);
    DockOverlayHintBottomOffset->setValue(Gui::OverlayParams::defaultDockOverlayHintBottomOffset());
    DockOverlayHintBottomOffset->setEntryName("DockOverlayHintBottomOffset");
    DockOverlayHintBottomOffset->setParamGrpPath("View");
    // Auto generated code (Tools/params_utils.py:1134)
    DockOverlayHintBottomOffset->setMinimum(0);
    DockOverlayHintBottomOffset->setMaximum(10000);
    DockOverlayHintBottomOffset->setSingleStep(10);

    // Auto generated code (Tools/params_utils.py:433)
    labelDockOverlayHintBottomLength = new QLabel(this);
    layoutOverlay->addWidget(labelDockOverlayHintBottomLength, 17, 0);
    DockOverlayHintBottomLength = new Gui::PrefSpinBox(this);
    layoutOverlay->addWidget(DockOverlayHintBottomLength, 17, 1);
    DockOverlayHintBottomLength->setValue(Gui::OverlayParams::defaultDockOverlayHintBottomLength());
    DockOverlayHintBottomLength->setEntryName("DockOverlayHintBottomLength");
    DockOverlayHintBottomLength->setParamGrpPath("View");
    // Auto generated code (Tools/params_utils.py:1134)
    DockOverlayHintBottomLength->setMinimum(0);
    DockOverlayHintBottomLength->setMaximum(10000);
    DockOverlayHintBottomLength->setSingleStep(10);

    // Auto generated code (Tools/params_utils.py:433)
    DockOverlayHintTabBar = new Gui::PrefCheckBox(this);
    layoutOverlay->addWidget(DockOverlayHintTabBar, 18, 0);
    DockOverlayHintTabBar->setChecked(Gui::OverlayParams::defaultDockOverlayHintTabBar());
    DockOverlayHintTabBar->setEntryName("DockOverlayHintTabBar");
    DockOverlayHintTabBar->setParamGrpPath("View");

    // Auto generated code (Tools/params_utils.py:433)
    labelDockOverlayHintDelay = new QLabel(this);
    layoutOverlay->addWidget(labelDockOverlayHintDelay, 19, 0);
    DockOverlayHintDelay = new Gui::PrefSpinBox(this);
    layoutOverlay->addWidget(DockOverlayHintDelay, 19, 1);
    DockOverlayHintDelay->setValue(Gui::OverlayParams::defaultDockOverlayHintDelay());
    DockOverlayHintDelay->setEntryName("DockOverlayHintDelay");
    DockOverlayHintDelay->setParamGrpPath("View");
    // Auto generated code (Tools/params_utils.py:1134)
    DockOverlayHintDelay->setMinimum(0);
    DockOverlayHintDelay->setMaximum(1000);
    DockOverlayHintDelay->setSingleStep(100);

    // Auto generated code (Tools/params_utils.py:433)
    labelDockOverlaySplitterHandleTimeout = new QLabel(this);
    layoutOverlay->addWidget(labelDockOverlaySplitterHandleTimeout, 20, 0);
    DockOverlaySplitterHandleTimeout = new Gui::PrefSpinBox(this);
    layoutOverlay->addWidget(DockOverlaySplitterHandleTimeout, 20, 1);
    DockOverlaySplitterHandleTimeout->setValue(Gui::OverlayParams::defaultDockOverlaySplitterHandleTimeout());
    DockOverlaySplitterHandleTimeout->setEntryName("DockOverlaySplitterHandleTimeout");
    DockOverlaySplitterHandleTimeout->setParamGrpPath("View");
    // Auto generated code (Tools/params_utils.py:1134)
    DockOverlaySplitterHandleTimeout->setMinimum(0);
    DockOverlaySplitterHandleTimeout->setMaximum(99999);
    DockOverlaySplitterHandleTimeout->setSingleStep(100);

    // Auto generated code (Tools/params_utils.py:433)
    DockOverlayActivateOnHover = new Gui::PrefCheckBox(this);
    layoutOverlay->addWidget(DockOverlayActivateOnHover, 21, 0);
    DockOverlayActivateOnHover->setChecked(Gui::OverlayParams::defaultDockOverlayActivateOnHover());
    DockOverlayActivateOnHover->setEntryName("DockOverlayActivateOnHover");
    DockOverlayActivateOnHover->setParamGrpPath("View");

    // Auto generated code (Tools/params_utils.py:433)
    labelDockOverlayDelay = new QLabel(this);
    layoutOverlay->addWidget(labelDockOverlayDelay, 22, 0);
    DockOverlayDelay = new Gui::PrefSpinBox(this);
    layoutOverlay->addWidget(DockOverlayDelay, 22, 1);
    DockOverlayDelay->setValue(Gui::OverlayParams::defaultDockOverlayDelay());
    DockOverlayDelay->setEntryName("DockOverlayDelay");
    DockOverlayDelay->setParamGrpPath("View");
    // Auto generated code (Tools/params_utils.py:1134)
    DockOverlayDelay->setMinimum(0);
    DockOverlayDelay->setMaximum(5000);
    DockOverlayDelay->setSingleStep(100);

    // Auto generated code (Tools/params_utils.py:433)
    labelDockOverlayAnimationDuration = new QLabel(this);
    layoutOverlay->addWidget(labelDockOverlayAnimationDuration, 23, 0);
    DockOverlayAnimationDuration = new Gui::PrefSpinBox(this);
    layoutOverlay->addWidget(DockOverlayAnimationDuration, 23, 1);
    DockOverlayAnimationDuration->setValue(Gui::OverlayParams::defaultDockOverlayAnimationDuration());
    DockOverlayAnimationDuration->setEntryName("DockOverlayAnimationDuration");
    DockOverlayAnimationDuration->setParamGrpPath("View");
    // Auto generated code (Tools/params_utils.py:1134)
    DockOverlayAnimationDuration->setMinimum(0);
    DockOverlayAnimationDuration->setMaximum(5000);
    DockOverlayAnimationDuration->setSingleStep(100);

    // Auto generated code (Tools/params_utils.py:433)
    labelDockOverlayAnimationCurve = new QLabel(this);
    layoutOverlay->addWidget(labelDockOverlayAnimationCurve, 24, 0);
    DockOverlayAnimationCurve = new Gui::PrefComboBox(this);
    layoutOverlay->addWidget(DockOverlayAnimationCurve, 24, 1);
    DockOverlayAnimationCurve->setEntryName("DockOverlayAnimationCurve");
    DockOverlayAnimationCurve->setParamGrpPath("View");
    // Auto generated code (Gui/OverlayParams.py:94)
    for (const auto &item : OverlayParams::AnimationCurveTypes)
        DockOverlayAnimationCurve->addItem(item);
    DockOverlayAnimationCurve->setCurrentIndex(Gui::OverlayParams::defaultDockOverlayAnimationCurve());
    layout->addItem(new QSpacerItem(40, 20, QSizePolicy::Fixed, QSizePolicy::Expanding));
    retranslateUi();
    // Auto generated code (Tools/params_utils.py:607)
    init();
}

// Auto generated code (Tools/params_utils.py:614)
DlgSettingsUI::~DlgSettingsUI()
{
}

// Auto generated code (Tools/params_utils.py:622)
void DlgSettingsUI::saveSettings()
{
    // Auto generated code (Tools/params_utils.py:461)
    ItemBackground->onSave();
    ItemBackgroundPadding->onSave();
    ResizableColumn->onSave();
    CheckBoxesSelection->onSave();
    HideColumn->onSave();
    HideScrollBar->onSave();
    HideHeaderView->onSave();
    IconSize->onSave();
    FontSize->onSave();
    ItemSpacing->onSave();
    DockOverlayHideTabBar->onSave();
    DockOverlayHidePropertyViewScrollBar->onSave();
    DockOverlayAutoView->onSave();
    DockOverlayAutoMouseThrough->onSave();
    DockOverlayWheelPassThrough->onSave();
    DockOverlayWheelDelay->onSave();
    DockOverlayAlphaRadius->onSave();
    DockOverlayCheckNaviCube->onSave();
    DockOverlayHintTriggerSize->onSave();
    DockOverlayHintSize->onSave();
    DockOverlayHintLeftOffset->onSave();
    DockOverlayHintLeftLength->onSave();
    DockOverlayHintRightOffset->onSave();
    DockOverlayHintRightLength->onSave();
    DockOverlayHintTopOffset->onSave();
    DockOverlayHintTopLength->onSave();
    DockOverlayHintBottomOffset->onSave();
    DockOverlayHintBottomLength->onSave();
    DockOverlayHintTabBar->onSave();
    DockOverlayHintDelay->onSave();
    DockOverlaySplitterHandleTimeout->onSave();
    DockOverlayActivateOnHover->onSave();
    DockOverlayDelay->onSave();
    DockOverlayAnimationDuration->onSave();
    DockOverlayAnimationCurve->onSave();
}

// Auto generated code (Tools/params_utils.py:631)
void DlgSettingsUI::loadSettings()
{
    // Auto generated code (Tools/params_utils.py:449)
    ItemBackground->onRestore();
    ItemBackgroundPadding->onRestore();
    ResizableColumn->onRestore();
    CheckBoxesSelection->onRestore();
    HideColumn->onRestore();
    HideScrollBar->onRestore();
    HideHeaderView->onRestore();
    IconSize->onRestore();
    FontSize->onRestore();
    ItemSpacing->onRestore();
    DockOverlayHideTabBar->onRestore();
    DockOverlayHidePropertyViewScrollBar->onRestore();
    DockOverlayAutoView->onRestore();
    DockOverlayAutoMouseThrough->onRestore();
    DockOverlayWheelPassThrough->onRestore();
    DockOverlayWheelDelay->onRestore();
    DockOverlayAlphaRadius->onRestore();
    DockOverlayCheckNaviCube->onRestore();
    DockOverlayHintTriggerSize->onRestore();
    DockOverlayHintSize->onRestore();
    DockOverlayHintLeftOffset->onRestore();
    DockOverlayHintLeftLength->onRestore();
    DockOverlayHintRightOffset->onRestore();
    DockOverlayHintRightLength->onRestore();
    DockOverlayHintTopOffset->onRestore();
    DockOverlayHintTopLength->onRestore();
    DockOverlayHintBottomOffset->onRestore();
    DockOverlayHintBottomLength->onRestore();
    DockOverlayHintTabBar->onRestore();
    DockOverlayHintDelay->onRestore();
    DockOverlaySplitterHandleTimeout->onRestore();
    DockOverlayActivateOnHover->onRestore();
    DockOverlayDelay->onRestore();
    DockOverlayAnimationDuration->onRestore();
    DockOverlayAnimationCurve->onRestore();
}

// Auto generated code (Tools/params_utils.py:640)
void DlgSettingsUI::retranslateUi()
{
    setWindowTitle(QObject::tr("UI"));
    groupTreeview->setTitle(QObject::tr("Tree view"));
    ItemBackground->setToolTip(QApplication::translate("TreeParams", Gui::TreeParams::docItemBackground()));
    labelItemBackground->setText(QObject::tr("Item background color"));
    labelItemBackground->setToolTip(ItemBackground->toolTip());
    ItemBackgroundPadding->setToolTip(QApplication::translate("TreeParams", Gui::TreeParams::docItemBackgroundPadding()));
    labelItemBackgroundPadding->setText(QObject::tr("Item background padding"));
    labelItemBackgroundPadding->setToolTip(ItemBackgroundPadding->toolTip());
    ResizableColumn->setToolTip(QApplication::translate("TreeParams", Gui::TreeParams::docResizableColumn()));
    ResizableColumn->setText(QObject::tr("Resizable columns"));
    CheckBoxesSelection->setToolTip(QApplication::translate("TreeParams", Gui::TreeParams::docCheckBoxesSelection()));
    CheckBoxesSelection->setText(QObject::tr("Show item checkbox"));
    HideColumn->setToolTip(QApplication::translate("TreeParams", Gui::TreeParams::docHideColumn()));
    HideColumn->setText(QObject::tr("Hide extra column"));
    HideScrollBar->setToolTip(QApplication::translate("TreeParams", Gui::TreeParams::docHideScrollBar()));
    HideScrollBar->setText(QObject::tr("Hide scroll bar"));
    HideHeaderView->setToolTip(QApplication::translate("TreeParams", Gui::TreeParams::docHideHeaderView()));
    HideHeaderView->setText(QObject::tr("Hide header"));
    IconSize->setToolTip(QApplication::translate("TreeParams", Gui::TreeParams::docIconSize()));
    labelIconSize->setText(QObject::tr("IconSize"));
    labelIconSize->setToolTip(IconSize->toolTip());
    FontSize->setToolTip(QApplication::translate("TreeParams", Gui::TreeParams::docFontSize()));
    labelFontSize->setText(QObject::tr("FontSize"));
    labelFontSize->setToolTip(FontSize->toolTip());
    ItemSpacing->setToolTip(QApplication::translate("TreeParams", Gui::TreeParams::docItemSpacing()));
    labelItemSpacing->setText(QObject::tr("ItemSpacing"));
    labelItemSpacing->setToolTip(ItemSpacing->toolTip());
    groupOverlay->setTitle(QObject::tr("Overlay"));
    DockOverlayHideTabBar->setToolTip(QApplication::translate("OverlayParams", Gui::OverlayParams::docDockOverlayHideTabBar()));
    DockOverlayHideTabBar->setText(QObject::tr("Hide tab bar"));
    DockOverlayHidePropertyViewScrollBar->setToolTip(QApplication::translate("OverlayParams", Gui::OverlayParams::docDockOverlayHidePropertyViewScrollBar()));
    DockOverlayHidePropertyViewScrollBar->setText(QObject::tr("Hide property view scroll bar"));
    DockOverlayAutoView->setToolTip(QApplication::translate("OverlayParams", Gui::OverlayParams::docDockOverlayAutoView()));
    DockOverlayAutoView->setText(QObject::tr("Auto hide in non 3D view"));
    DockOverlayAutoMouseThrough->setToolTip(QApplication::translate("OverlayParams", Gui::OverlayParams::docDockOverlayAutoMouseThrough()));
    DockOverlayAutoMouseThrough->setText(QObject::tr("Auto mouse pass through"));
    DockOverlayWheelPassThrough->setToolTip(QApplication::translate("OverlayParams", Gui::OverlayParams::docDockOverlayWheelPassThrough()));
    DockOverlayWheelPassThrough->setText(QObject::tr("Auto mouse wheel pass through"));
    DockOverlayWheelDelay->setToolTip(QApplication::translate("OverlayParams", Gui::OverlayParams::docDockOverlayWheelDelay()));
    labelDockOverlayWheelDelay->setText(QObject::tr("Delay mouse wheel pass through (ms)"));
    labelDockOverlayWheelDelay->setToolTip(DockOverlayWheelDelay->toolTip());
    DockOverlayAlphaRadius->setToolTip(QApplication::translate("OverlayParams", Gui::OverlayParams::docDockOverlayAlphaRadius()));
    labelDockOverlayAlphaRadius->setText(QObject::tr("Alpha test radius"));
    labelDockOverlayAlphaRadius->setToolTip(DockOverlayAlphaRadius->toolTip());
    DockOverlayCheckNaviCube->setToolTip(QApplication::translate("OverlayParams", Gui::OverlayParams::docDockOverlayCheckNaviCube()));
    DockOverlayCheckNaviCube->setText(QObject::tr("Check Navigation Cube"));
    DockOverlayHintTriggerSize->setToolTip(QApplication::translate("OverlayParams", Gui::OverlayParams::docDockOverlayHintTriggerSize()));
    labelDockOverlayHintTriggerSize->setText(QObject::tr("Hint trigger size"));
    labelDockOverlayHintTriggerSize->setToolTip(DockOverlayHintTriggerSize->toolTip());
    DockOverlayHintSize->setToolTip(QApplication::translate("OverlayParams", Gui::OverlayParams::docDockOverlayHintSize()));
    labelDockOverlayHintSize->setText(QObject::tr("Hint width"));
    labelDockOverlayHintSize->setToolTip(DockOverlayHintSize->toolTip());
    DockOverlayHintLeftOffset->setToolTip(QApplication::translate("OverlayParams", Gui::OverlayParams::docDockOverlayHintLeftOffset()));
    labelDockOverlayHintLeftOffset->setText(QObject::tr("Left panel hint offset"));
    labelDockOverlayHintLeftOffset->setToolTip(DockOverlayHintLeftOffset->toolTip());
    DockOverlayHintLeftLength->setToolTip(QApplication::translate("OverlayParams", Gui::OverlayParams::docDockOverlayHintLeftLength()));
    labelDockOverlayHintLeftLength->setText(QObject::tr("Left panel hint length"));
    labelDockOverlayHintLeftLength->setToolTip(DockOverlayHintLeftLength->toolTip());
    DockOverlayHintRightOffset->setToolTip(QApplication::translate("OverlayParams", Gui::OverlayParams::docDockOverlayHintRightOffset()));
    labelDockOverlayHintRightOffset->setText(QObject::tr("Right panel hint offset"));
    labelDockOverlayHintRightOffset->setToolTip(DockOverlayHintRightOffset->toolTip());
    DockOverlayHintRightLength->setToolTip(QApplication::translate("OverlayParams", Gui::OverlayParams::docDockOverlayHintRightLength()));
    labelDockOverlayHintRightLength->setText(QObject::tr("Right panel hint length"));
    labelDockOverlayHintRightLength->setToolTip(DockOverlayHintRightLength->toolTip());
    DockOverlayHintTopOffset->setToolTip(QApplication::translate("OverlayParams", Gui::OverlayParams::docDockOverlayHintTopOffset()));
    labelDockOverlayHintTopOffset->setText(QObject::tr("Top panel hint offset"));
    labelDockOverlayHintTopOffset->setToolTip(DockOverlayHintTopOffset->toolTip());
    DockOverlayHintTopLength->setToolTip(QApplication::translate("OverlayParams", Gui::OverlayParams::docDockOverlayHintTopLength()));
    labelDockOverlayHintTopLength->setText(QObject::tr("Top panel hint length"));
    labelDockOverlayHintTopLength->setToolTip(DockOverlayHintTopLength->toolTip());
    DockOverlayHintBottomOffset->setToolTip(QApplication::translate("OverlayParams", Gui::OverlayParams::docDockOverlayHintBottomOffset()));
    labelDockOverlayHintBottomOffset->setText(QObject::tr("Bottom panel hint offset"));
    labelDockOverlayHintBottomOffset->setToolTip(DockOverlayHintBottomOffset->toolTip());
    DockOverlayHintBottomLength->setToolTip(QApplication::translate("OverlayParams", Gui::OverlayParams::docDockOverlayHintBottomLength()));
    labelDockOverlayHintBottomLength->setText(QObject::tr("Bottom panel hint length"));
    labelDockOverlayHintBottomLength->setToolTip(DockOverlayHintBottomLength->toolTip());
    DockOverlayHintTabBar->setToolTip(QApplication::translate("OverlayParams", Gui::OverlayParams::docDockOverlayHintTabBar()));
    DockOverlayHintTabBar->setText(QObject::tr("Hint show tab bar"));
    DockOverlayHintDelay->setToolTip(QApplication::translate("OverlayParams", Gui::OverlayParams::docDockOverlayHintDelay()));
    labelDockOverlayHintDelay->setText(QObject::tr("Hint delay (ms)"));
    labelDockOverlayHintDelay->setToolTip(DockOverlayHintDelay->toolTip());
    DockOverlaySplitterHandleTimeout->setToolTip(QApplication::translate("OverlayParams", Gui::OverlayParams::docDockOverlaySplitterHandleTimeout()));
    labelDockOverlaySplitterHandleTimeout->setText(QObject::tr("Splitter auto hide delay (ms)"));
    labelDockOverlaySplitterHandleTimeout->setToolTip(DockOverlaySplitterHandleTimeout->toolTip());
    DockOverlayActivateOnHover->setToolTip(QApplication::translate("OverlayParams", Gui::OverlayParams::docDockOverlayActivateOnHover()));
    DockOverlayActivateOnHover->setText(QObject::tr("Activate on hover"));
    DockOverlayDelay->setToolTip(QApplication::translate("OverlayParams", Gui::OverlayParams::docDockOverlayDelay()));
    labelDockOverlayDelay->setText(QObject::tr("Layout delay (ms)"));
    labelDockOverlayDelay->setToolTip(DockOverlayDelay->toolTip());
    DockOverlayAnimationDuration->setToolTip(QApplication::translate("OverlayParams", Gui::OverlayParams::docDockOverlayAnimationDuration()));
    labelDockOverlayAnimationDuration->setText(QObject::tr("Animation duration (ms)"));
    labelDockOverlayAnimationDuration->setToolTip(DockOverlayAnimationDuration->toolTip());
    DockOverlayAnimationCurve->setToolTip(QApplication::translate("OverlayParams", Gui::OverlayParams::docDockOverlayAnimationCurve()));
    labelDockOverlayAnimationCurve->setText(QObject::tr("Animation curve type"));
    labelDockOverlayAnimationCurve->setToolTip(DockOverlayAnimationCurve->toolTip());
}

// Auto generated code (Tools/params_utils.py:657)
void DlgSettingsUI::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QWidget::changeEvent(e);
}

// Auto generated code (Tools/params_utils.py:670)
#include "moc_DlgSettingsUI.cpp"
//[[[end]]]

// -----------------------------------------------------------------------------------
// user code start

void DlgSettingsUI::init()
{
    timer = new QTimer(this);
    timer->setSingleShot(true);

    animator1 = new QPropertyAnimation(this, "offset1", this);
    QObject::connect(animator1, &QPropertyAnimation::stateChanged, [this]() {
        if (animator1->state() != QAbstractAnimation::Running)
            timer->start(1000);
    });

    QObject::connect(DockOverlayAnimationCurve, QOverload<int>::of(&QComboBox::currentIndexChanged),
                     this, &DlgSettingsUI::onCurveChange);

    QObject::connect(timer, &QTimer::timeout, [=]() {
        if (animator1->state() != QAbstractAnimation::Running) {
            this->setOffset1(1);
            this->a1 = this->b1 = 0;
        }
    });
}

qreal DlgSettingsUI::offset1() const
{
    return this->t1;
}

void DlgSettingsUI::setOffset1(qreal t)
{
    if (t == this->t1)
        return;
    this->t1 = t;
    QLabel *label = this->labelDockOverlayAnimationCurve;
    if (this->a1 == this->b1) {
        this->a1 = label->x();
        QPoint pos(width(), 0);
        this->b1 = width() - label->fontMetrics().boundingRect(label->text()).width() - 5;
    }
    label->move(this->a1 * (1-t) + this->b1 * t, label->y());
}

void DlgSettingsUI::onCurveChange(int index)
{
    if (sender() != DockOverlayAnimationCurve)
        return;
    double value = DockOverlayAnimationDuration->value()*2;
    auto animator = animator1;
    animator->setStartValue(0.0);
    animator->setEndValue(1.0);
    animator->setEasingCurve((QEasingCurve::Type)index);
    animator->setDuration(value);
    animator->start();
}

// user code end
// -----------------------------------------------------------------------------------
