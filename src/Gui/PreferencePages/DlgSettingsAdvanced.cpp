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
import DlgSettingsAdvanced
DlgSettingsAdvanced.define()
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
#include "Gui/PreferencePages/DlgSettingsAdvanced.h"
using namespace Gui::Dialog;
/* TRANSLATOR Gui::Dialog::DlgSettingsAdvanced */

// Auto generated code (Tools/params_utils.py:598)
DlgSettingsAdvanced::DlgSettingsAdvanced(QWidget* parent)
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
    labelItemBackgroundPadding = new QLabel(this);
    layoutTreeview->addWidget(labelItemBackgroundPadding, 0, 0);
    ItemBackgroundPadding = new Gui::PrefSpinBox(this);
    layoutTreeview->addWidget(ItemBackgroundPadding, 0, 1);
    ItemBackgroundPadding->setValue(Gui::TreeParams::defaultItemBackgroundPadding());
    ItemBackgroundPadding->setEntryName("ItemBackgroundPadding");
    ItemBackgroundPadding->setParamGrpPath("TreeView");
    // Auto generated code (Tools/params_utils.py:1135)
    ItemBackgroundPadding->setMinimum(0);
    ItemBackgroundPadding->setMaximum(100);
    ItemBackgroundPadding->setSingleStep(1);
    ItemBackgroundPadding->setAlignment(Qt::AlignRight);
    ItemBackgroundPadding->setSuffix(QLatin1String(" px"));

    // Auto generated code (Tools/params_utils.py:433)
    labelFontSize = new QLabel(this);
    layoutTreeview->addWidget(labelFontSize, 1, 0);
    FontSize = new Gui::PrefSpinBox(this);
    layoutTreeview->addWidget(FontSize, 1, 1);
    FontSize->setValue(Gui::TreeParams::defaultFontSize());
    FontSize->setEntryName("FontSize");
    FontSize->setParamGrpPath("TreeView");
    // Auto generated code (Tools/params_utils.py:1135)
    FontSize->setMinimum(0);
    FontSize->setMaximum(100);
    FontSize->setSingleStep(1);
    FontSize->setAlignment(Qt::AlignRight);
    FontSize->setSuffix(QLatin1String(" pt"));


    // Auto generated code (Tools/params_utils.py:420)
    groupOverlay = new QGroupBox(this);
    layout->addWidget(groupOverlay);
    auto layoutHorizOverlay = new QHBoxLayout(groupOverlay);
    auto layoutOverlay = new QGridLayout();
    layoutHorizOverlay->addLayout(layoutOverlay);
    layoutHorizOverlay->addStretch();

    // Auto generated code (Tools/params_utils.py:433)
    labelDockOverlayWheelDelay = new QLabel(this);
    layoutOverlay->addWidget(labelDockOverlayWheelDelay, 0, 0);
    DockOverlayWheelDelay = new Gui::PrefSpinBox(this);
    layoutOverlay->addWidget(DockOverlayWheelDelay, 0, 1);
    DockOverlayWheelDelay->setValue(Gui::OverlayParams::defaultDockOverlayWheelDelay());
    DockOverlayWheelDelay->setEntryName("DockOverlayWheelDelay");
    DockOverlayWheelDelay->setParamGrpPath("View");
    // Auto generated code (Tools/params_utils.py:1135)
    DockOverlayWheelDelay->setMinimum(0);
    DockOverlayWheelDelay->setMaximum(99999);
    DockOverlayWheelDelay->setSingleStep(1);
    DockOverlayWheelDelay->setAlignment(Qt::AlignRight);
    DockOverlayWheelDelay->setSuffix(QLatin1String(" ms"));

    // Auto generated code (Tools/params_utils.py:433)
    labelDockOverlayAlphaRadius = new QLabel(this);
    layoutOverlay->addWidget(labelDockOverlayAlphaRadius, 1, 0);
    DockOverlayAlphaRadius = new Gui::PrefSpinBox(this);
    layoutOverlay->addWidget(DockOverlayAlphaRadius, 1, 1);
    DockOverlayAlphaRadius->setValue(Gui::OverlayParams::defaultDockOverlayAlphaRadius());
    DockOverlayAlphaRadius->setEntryName("DockOverlayAlphaRadius");
    DockOverlayAlphaRadius->setParamGrpPath("View");
    // Auto generated code (Tools/params_utils.py:1135)
    DockOverlayAlphaRadius->setMinimum(1);
    DockOverlayAlphaRadius->setMaximum(100);
    DockOverlayAlphaRadius->setSingleStep(1);
    DockOverlayAlphaRadius->setAlignment(Qt::AlignRight);
    DockOverlayAlphaRadius->setSuffix(QLatin1String(" px"));

    // Auto generated code (Tools/params_utils.py:433)
    DockOverlayCheckNaviCube = new Gui::PrefCheckBox(this);
    layoutOverlay->addWidget(DockOverlayCheckNaviCube, 2, 0);
    DockOverlayCheckNaviCube->setChecked(Gui::OverlayParams::defaultDockOverlayCheckNaviCube());
    DockOverlayCheckNaviCube->setEntryName("DockOverlayCheckNaviCube");
    DockOverlayCheckNaviCube->setParamGrpPath("View");

    // Auto generated code (Tools/params_utils.py:433)
    labelDockOverlayHintTriggerSize = new QLabel(this);
    layoutOverlay->addWidget(labelDockOverlayHintTriggerSize, 3, 0);
    DockOverlayHintTriggerSize = new Gui::PrefSpinBox(this);
    layoutOverlay->addWidget(DockOverlayHintTriggerSize, 3, 1);
    DockOverlayHintTriggerSize->setValue(Gui::OverlayParams::defaultDockOverlayHintTriggerSize());
    DockOverlayHintTriggerSize->setEntryName("DockOverlayHintTriggerSize");
    DockOverlayHintTriggerSize->setParamGrpPath("View");
    // Auto generated code (Tools/params_utils.py:1135)
    DockOverlayHintTriggerSize->setMinimum(1);
    DockOverlayHintTriggerSize->setMaximum(100);
    DockOverlayHintTriggerSize->setSingleStep(1);
    DockOverlayHintTriggerSize->setAlignment(Qt::AlignRight);
    DockOverlayHintTriggerSize->setSuffix(QLatin1String(" px"));

    // Auto generated code (Tools/params_utils.py:433)
    labelDockOverlayHintSize = new QLabel(this);
    layoutOverlay->addWidget(labelDockOverlayHintSize, 4, 0);
    DockOverlayHintSize = new Gui::PrefSpinBox(this);
    layoutOverlay->addWidget(DockOverlayHintSize, 4, 1);
    DockOverlayHintSize->setValue(Gui::OverlayParams::defaultDockOverlayHintSize());
    DockOverlayHintSize->setEntryName("DockOverlayHintSize");
    DockOverlayHintSize->setParamGrpPath("View");
    // Auto generated code (Tools/params_utils.py:1135)
    DockOverlayHintSize->setMinimum(1);
    DockOverlayHintSize->setMaximum(100);
    DockOverlayHintSize->setSingleStep(1);
    DockOverlayHintSize->setAlignment(Qt::AlignRight);
    DockOverlayHintSize->setSuffix(QLatin1String(" px"));

    // Auto generated code (Tools/params_utils.py:433)
    labelDockOverlayHintLeftOffset = new QLabel(this);
    layoutOverlay->addWidget(labelDockOverlayHintLeftOffset, 5, 0);
    DockOverlayHintLeftOffset = new Gui::PrefSpinBox(this);
    layoutOverlay->addWidget(DockOverlayHintLeftOffset, 5, 1);
    DockOverlayHintLeftOffset->setValue(Gui::OverlayParams::defaultDockOverlayHintLeftOffset());
    DockOverlayHintLeftOffset->setEntryName("DockOverlayHintLeftOffset");
    DockOverlayHintLeftOffset->setParamGrpPath("View");
    // Auto generated code (Tools/params_utils.py:1135)
    DockOverlayHintLeftOffset->setMinimum(0);
    DockOverlayHintLeftOffset->setMaximum(10000);
    DockOverlayHintLeftOffset->setSingleStep(10);
    DockOverlayHintLeftOffset->setAlignment(Qt::AlignRight);
    DockOverlayHintLeftOffset->setSuffix(QLatin1String(" px"));

    // Auto generated code (Tools/params_utils.py:433)
    labelDockOverlayHintLeftLength = new QLabel(this);
    layoutOverlay->addWidget(labelDockOverlayHintLeftLength, 6, 0);
    DockOverlayHintLeftLength = new Gui::PrefSpinBox(this);
    layoutOverlay->addWidget(DockOverlayHintLeftLength, 6, 1);
    DockOverlayHintLeftLength->setValue(Gui::OverlayParams::defaultDockOverlayHintLeftLength());
    DockOverlayHintLeftLength->setEntryName("DockOverlayHintLeftLength");
    DockOverlayHintLeftLength->setParamGrpPath("View");
    // Auto generated code (Tools/params_utils.py:1135)
    DockOverlayHintLeftLength->setMinimum(0);
    DockOverlayHintLeftLength->setMaximum(10000);
    DockOverlayHintLeftLength->setSingleStep(10);
    DockOverlayHintLeftLength->setAlignment(Qt::AlignRight);
    DockOverlayHintLeftLength->setSuffix(QLatin1String(" px"));

    // Auto generated code (Tools/params_utils.py:433)
    labelDockOverlayHintRightOffset = new QLabel(this);
    layoutOverlay->addWidget(labelDockOverlayHintRightOffset, 7, 0);
    DockOverlayHintRightOffset = new Gui::PrefSpinBox(this);
    layoutOverlay->addWidget(DockOverlayHintRightOffset, 7, 1);
    DockOverlayHintRightOffset->setValue(Gui::OverlayParams::defaultDockOverlayHintRightOffset());
    DockOverlayHintRightOffset->setEntryName("DockOverlayHintRightOffset");
    DockOverlayHintRightOffset->setParamGrpPath("View");
    // Auto generated code (Tools/params_utils.py:1135)
    DockOverlayHintRightOffset->setMinimum(0);
    DockOverlayHintRightOffset->setMaximum(10000);
    DockOverlayHintRightOffset->setSingleStep(10);
    DockOverlayHintRightOffset->setAlignment(Qt::AlignRight);
    DockOverlayHintRightOffset->setSuffix(QLatin1String(" px"));

    // Auto generated code (Tools/params_utils.py:433)
    labelDockOverlayHintRightLength = new QLabel(this);
    layoutOverlay->addWidget(labelDockOverlayHintRightLength, 8, 0);
    DockOverlayHintRightLength = new Gui::PrefSpinBox(this);
    layoutOverlay->addWidget(DockOverlayHintRightLength, 8, 1);
    DockOverlayHintRightLength->setValue(Gui::OverlayParams::defaultDockOverlayHintRightLength());
    DockOverlayHintRightLength->setEntryName("DockOverlayHintRightLength");
    DockOverlayHintRightLength->setParamGrpPath("View");
    // Auto generated code (Tools/params_utils.py:1135)
    DockOverlayHintRightLength->setMinimum(0);
    DockOverlayHintRightLength->setMaximum(10000);
    DockOverlayHintRightLength->setSingleStep(10);
    DockOverlayHintRightLength->setAlignment(Qt::AlignRight);
    DockOverlayHintRightLength->setSuffix(QLatin1String(" px"));

    // Auto generated code (Tools/params_utils.py:433)
    labelDockOverlayHintTopOffset = new QLabel(this);
    layoutOverlay->addWidget(labelDockOverlayHintTopOffset, 9, 0);
    DockOverlayHintTopOffset = new Gui::PrefSpinBox(this);
    layoutOverlay->addWidget(DockOverlayHintTopOffset, 9, 1);
    DockOverlayHintTopOffset->setValue(Gui::OverlayParams::defaultDockOverlayHintTopOffset());
    DockOverlayHintTopOffset->setEntryName("DockOverlayHintTopOffset");
    DockOverlayHintTopOffset->setParamGrpPath("View");
    // Auto generated code (Tools/params_utils.py:1135)
    DockOverlayHintTopOffset->setMinimum(0);
    DockOverlayHintTopOffset->setMaximum(10000);
    DockOverlayHintTopOffset->setSingleStep(10);
    DockOverlayHintTopOffset->setAlignment(Qt::AlignRight);
    DockOverlayHintTopOffset->setSuffix(QLatin1String(" px"));

    // Auto generated code (Tools/params_utils.py:433)
    labelDockOverlayHintTopLength = new QLabel(this);
    layoutOverlay->addWidget(labelDockOverlayHintTopLength, 10, 0);
    DockOverlayHintTopLength = new Gui::PrefSpinBox(this);
    layoutOverlay->addWidget(DockOverlayHintTopLength, 10, 1);
    DockOverlayHintTopLength->setValue(Gui::OverlayParams::defaultDockOverlayHintTopLength());
    DockOverlayHintTopLength->setEntryName("DockOverlayHintTopLength");
    DockOverlayHintTopLength->setParamGrpPath("View");
    // Auto generated code (Tools/params_utils.py:1135)
    DockOverlayHintTopLength->setMinimum(0);
    DockOverlayHintTopLength->setMaximum(10000);
    DockOverlayHintTopLength->setSingleStep(10);
    DockOverlayHintTopLength->setAlignment(Qt::AlignRight);
    DockOverlayHintTopLength->setSuffix(QLatin1String(" px"));

    // Auto generated code (Tools/params_utils.py:433)
    labelDockOverlayHintBottomOffset = new QLabel(this);
    layoutOverlay->addWidget(labelDockOverlayHintBottomOffset, 11, 0);
    DockOverlayHintBottomOffset = new Gui::PrefSpinBox(this);
    layoutOverlay->addWidget(DockOverlayHintBottomOffset, 11, 1);
    DockOverlayHintBottomOffset->setValue(Gui::OverlayParams::defaultDockOverlayHintBottomOffset());
    DockOverlayHintBottomOffset->setEntryName("DockOverlayHintBottomOffset");
    DockOverlayHintBottomOffset->setParamGrpPath("View");
    // Auto generated code (Tools/params_utils.py:1135)
    DockOverlayHintBottomOffset->setMinimum(0);
    DockOverlayHintBottomOffset->setMaximum(10000);
    DockOverlayHintBottomOffset->setSingleStep(10);
    DockOverlayHintBottomOffset->setAlignment(Qt::AlignRight);
    DockOverlayHintBottomOffset->setSuffix(QLatin1String(" px"));

    // Auto generated code (Tools/params_utils.py:433)
    labelDockOverlayHintBottomLength = new QLabel(this);
    layoutOverlay->addWidget(labelDockOverlayHintBottomLength, 12, 0);
    DockOverlayHintBottomLength = new Gui::PrefSpinBox(this);
    layoutOverlay->addWidget(DockOverlayHintBottomLength, 12, 1);
    DockOverlayHintBottomLength->setValue(Gui::OverlayParams::defaultDockOverlayHintBottomLength());
    DockOverlayHintBottomLength->setEntryName("DockOverlayHintBottomLength");
    DockOverlayHintBottomLength->setParamGrpPath("View");
    // Auto generated code (Tools/params_utils.py:1135)
    DockOverlayHintBottomLength->setMinimum(0);
    DockOverlayHintBottomLength->setMaximum(10000);
    DockOverlayHintBottomLength->setSingleStep(10);
    DockOverlayHintBottomLength->setAlignment(Qt::AlignRight);
    DockOverlayHintBottomLength->setSuffix(QLatin1String(" px"));

    // Auto generated code (Tools/params_utils.py:433)
    labelDockOverlayHintDelay = new QLabel(this);
    layoutOverlay->addWidget(labelDockOverlayHintDelay, 13, 0);
    DockOverlayHintDelay = new Gui::PrefSpinBox(this);
    layoutOverlay->addWidget(DockOverlayHintDelay, 13, 1);
    DockOverlayHintDelay->setValue(Gui::OverlayParams::defaultDockOverlayHintDelay());
    DockOverlayHintDelay->setEntryName("DockOverlayHintDelay");
    DockOverlayHintDelay->setParamGrpPath("View");
    // Auto generated code (Tools/params_utils.py:1135)
    DockOverlayHintDelay->setMinimum(0);
    DockOverlayHintDelay->setMaximum(1000);
    DockOverlayHintDelay->setSingleStep(100);
    DockOverlayHintDelay->setAlignment(Qt::AlignRight);
    DockOverlayHintDelay->setSuffix(QLatin1String(" ms"));

    // Auto generated code (Tools/params_utils.py:433)
    labelDockOverlaySplitterHandleTimeout = new QLabel(this);
    layoutOverlay->addWidget(labelDockOverlaySplitterHandleTimeout, 14, 0);
    DockOverlaySplitterHandleTimeout = new Gui::PrefSpinBox(this);
    layoutOverlay->addWidget(DockOverlaySplitterHandleTimeout, 14, 1);
    DockOverlaySplitterHandleTimeout->setValue(Gui::OverlayParams::defaultDockOverlaySplitterHandleTimeout());
    DockOverlaySplitterHandleTimeout->setEntryName("DockOverlaySplitterHandleTimeout");
    DockOverlaySplitterHandleTimeout->setParamGrpPath("View");
    // Auto generated code (Tools/params_utils.py:1135)
    DockOverlaySplitterHandleTimeout->setMinimum(0);
    DockOverlaySplitterHandleTimeout->setMaximum(99999);
    DockOverlaySplitterHandleTimeout->setSingleStep(100);
    DockOverlaySplitterHandleTimeout->setAlignment(Qt::AlignRight);
    DockOverlaySplitterHandleTimeout->setSuffix(QLatin1String(" ms"));

    // Auto generated code (Tools/params_utils.py:433)
    DockOverlayActivateOnHover = new Gui::PrefCheckBox(this);
    layoutOverlay->addWidget(DockOverlayActivateOnHover, 15, 0);
    DockOverlayActivateOnHover->setChecked(Gui::OverlayParams::defaultDockOverlayActivateOnHover());
    DockOverlayActivateOnHover->setEntryName("DockOverlayActivateOnHover");
    DockOverlayActivateOnHover->setParamGrpPath("View");

    // Auto generated code (Tools/params_utils.py:433)
    labelDockOverlayDelay = new QLabel(this);
    layoutOverlay->addWidget(labelDockOverlayDelay, 16, 0);
    DockOverlayDelay = new Gui::PrefSpinBox(this);
    layoutOverlay->addWidget(DockOverlayDelay, 16, 1);
    DockOverlayDelay->setValue(Gui::OverlayParams::defaultDockOverlayDelay());
    DockOverlayDelay->setEntryName("DockOverlayDelay");
    DockOverlayDelay->setParamGrpPath("View");
    // Auto generated code (Tools/params_utils.py:1135)
    DockOverlayDelay->setMinimum(0);
    DockOverlayDelay->setMaximum(5000);
    DockOverlayDelay->setSingleStep(100);
    DockOverlayDelay->setAlignment(Qt::AlignRight);
    DockOverlayDelay->setSuffix(QLatin1String(" ms"));

    // Auto generated code (Tools/params_utils.py:433)
    labelDockOverlayAnimationDuration = new QLabel(this);
    layoutOverlay->addWidget(labelDockOverlayAnimationDuration, 17, 0);
    DockOverlayAnimationDuration = new Gui::PrefSpinBox(this);
    layoutOverlay->addWidget(DockOverlayAnimationDuration, 17, 1);
    DockOverlayAnimationDuration->setValue(Gui::OverlayParams::defaultDockOverlayAnimationDuration());
    DockOverlayAnimationDuration->setEntryName("DockOverlayAnimationDuration");
    DockOverlayAnimationDuration->setParamGrpPath("View");
    // Auto generated code (Tools/params_utils.py:1135)
    DockOverlayAnimationDuration->setMinimum(0);
    DockOverlayAnimationDuration->setMaximum(5000);
    DockOverlayAnimationDuration->setSingleStep(100);
    DockOverlayAnimationDuration->setAlignment(Qt::AlignRight);
    DockOverlayAnimationDuration->setSuffix(QLatin1String(" ms"));

    // Auto generated code (Tools/params_utils.py:433)
    labelDockOverlayAnimationCurve = new QLabel(this);
    layoutOverlay->addWidget(labelDockOverlayAnimationCurve, 18, 0);
    DockOverlayAnimationCurve = new Gui::PrefComboBox(this);
    layoutOverlay->addWidget(DockOverlayAnimationCurve, 18, 1);
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
DlgSettingsAdvanced::~DlgSettingsAdvanced()
{
}

// Auto generated code (Tools/params_utils.py:622)
void DlgSettingsAdvanced::saveSettings()
{
    // Auto generated code (Tools/params_utils.py:461)
    ItemBackgroundPadding->onSave();
    FontSize->onSave();
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
    DockOverlayHintDelay->onSave();
    DockOverlaySplitterHandleTimeout->onSave();
    DockOverlayActivateOnHover->onSave();
    DockOverlayDelay->onSave();
    DockOverlayAnimationDuration->onSave();
    DockOverlayAnimationCurve->onSave();
}

// Auto generated code (Tools/params_utils.py:631)
void DlgSettingsAdvanced::loadSettings()
{
    // Auto generated code (Tools/params_utils.py:449)
    ItemBackgroundPadding->onRestore();
    FontSize->onRestore();
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
    DockOverlayHintDelay->onRestore();
    DockOverlaySplitterHandleTimeout->onRestore();
    DockOverlayActivateOnHover->onRestore();
    DockOverlayDelay->onRestore();
    DockOverlayAnimationDuration->onRestore();
    DockOverlayAnimationCurve->onRestore();
}

// Auto generated code (Tools/params_utils.py:640)
void DlgSettingsAdvanced::retranslateUi()
{
    setWindowTitle(QObject::tr("Advanced"));
    groupTreeview->setTitle(QObject::tr("Tree view"));
    ItemBackgroundPadding->setToolTip(QApplication::translate("TreeParams", Gui::TreeParams::docItemBackgroundPadding()));
    labelItemBackgroundPadding->setText(QObject::tr("Item background padding"));
    labelItemBackgroundPadding->setToolTip(ItemBackgroundPadding->toolTip());
    FontSize->setToolTip(QApplication::translate("TreeParams", Gui::TreeParams::docFontSize()));
    labelFontSize->setText(QObject::tr("Font size"));
    labelFontSize->setToolTip(FontSize->toolTip());
    groupOverlay->setTitle(QObject::tr("Overlay"));
    DockOverlayWheelDelay->setToolTip(QApplication::translate("OverlayParams", Gui::OverlayParams::docDockOverlayWheelDelay()));
    labelDockOverlayWheelDelay->setText(QObject::tr("Delay mouse wheel pass through"));
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
    DockOverlayHintDelay->setToolTip(QApplication::translate("OverlayParams", Gui::OverlayParams::docDockOverlayHintDelay()));
    labelDockOverlayHintDelay->setText(QObject::tr("Hint delay"));
    labelDockOverlayHintDelay->setToolTip(DockOverlayHintDelay->toolTip());
    DockOverlaySplitterHandleTimeout->setToolTip(QApplication::translate("OverlayParams", Gui::OverlayParams::docDockOverlaySplitterHandleTimeout()));
    labelDockOverlaySplitterHandleTimeout->setText(QObject::tr("Splitter auto hide delay"));
    labelDockOverlaySplitterHandleTimeout->setToolTip(DockOverlaySplitterHandleTimeout->toolTip());
    DockOverlayActivateOnHover->setToolTip(QApplication::translate("OverlayParams", Gui::OverlayParams::docDockOverlayActivateOnHover()));
    DockOverlayActivateOnHover->setText(QObject::tr("Activate on hover"));
    DockOverlayDelay->setToolTip(QApplication::translate("OverlayParams", Gui::OverlayParams::docDockOverlayDelay()));
    labelDockOverlayDelay->setText(QObject::tr("Layout delay"));
    labelDockOverlayDelay->setToolTip(DockOverlayDelay->toolTip());
    DockOverlayAnimationDuration->setToolTip(QApplication::translate("OverlayParams", Gui::OverlayParams::docDockOverlayAnimationDuration()));
    labelDockOverlayAnimationDuration->setText(QObject::tr("Animation duration"));
    labelDockOverlayAnimationDuration->setToolTip(DockOverlayAnimationDuration->toolTip());
    DockOverlayAnimationCurve->setToolTip(QApplication::translate("OverlayParams", Gui::OverlayParams::docDockOverlayAnimationCurve()));
    labelDockOverlayAnimationCurve->setText(QObject::tr("Animation curve type"));
    labelDockOverlayAnimationCurve->setToolTip(DockOverlayAnimationCurve->toolTip());
}

// Auto generated code (Tools/params_utils.py:657)
void DlgSettingsAdvanced::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QWidget::changeEvent(e);
}

// Auto generated code (Tools/params_utils.py:670)
#include "moc_DlgSettingsAdvanced.cpp"
//[[[end]]]

// -----------------------------------------------------------------------------------
// user code start

void DlgSettingsAdvanced::init()
{
    timer = new QTimer(this);
    timer->setSingleShot(true);

    animator1 = new QPropertyAnimation(this, "offset1", this);
    QObject::connect(animator1, &QPropertyAnimation::stateChanged, [this]() {
        if (animator1->state() != QAbstractAnimation::Running)
            timer->start(1000);
    });

    QObject::connect(DockOverlayAnimationCurve, QOverload<int>::of(&QComboBox::currentIndexChanged),
                     this, &DlgSettingsAdvanced::onCurveChange);

    QObject::connect(timer, &QTimer::timeout, [this]() {
        if (animator1->state() != QAbstractAnimation::Running) {
            this->setOffset1(1);
            this->a1 = this->b1 = 0;
        }
    });
}

qreal DlgSettingsAdvanced::offset1() const
{
    return this->t1;
}

void DlgSettingsAdvanced::setOffset1(qreal t)
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

void DlgSettingsAdvanced::onCurveChange(int index)
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
