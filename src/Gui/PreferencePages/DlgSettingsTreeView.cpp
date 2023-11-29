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

/*[[[cog
import DlgSettingsTreeView
DlgSettingsTreeView.define()
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
#include "Gui/PreferencePages/DlgSettingsTreeView.h"
using namespace Gui::Dialog;
/* TRANSLATOR Gui::Dialog::DlgSettingsTreeView */

// Auto generated code (Tools/params_utils.py:598)
DlgSettingsTreeView::DlgSettingsTreeView(QWidget* parent)
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

    layout->addItem(new QSpacerItem(40, 20, QSizePolicy::Fixed, QSizePolicy::Expanding));
    retranslateUi();
    // Auto generated code (Tools/params_utils.py:607)
}

// Auto generated code (Tools/params_utils.py:614)
DlgSettingsTreeView::~DlgSettingsTreeView()
{
}

// Auto generated code (Tools/params_utils.py:622)
void DlgSettingsTreeView::saveSettings()
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
}

// Auto generated code (Tools/params_utils.py:631)
void DlgSettingsTreeView::loadSettings()
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
}

// Auto generated code (Tools/params_utils.py:640)
void DlgSettingsTreeView::retranslateUi()
{
    setWindowTitle(QObject::tr("Tree View"));
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
}

// Auto generated code (Tools/params_utils.py:657)
void DlgSettingsTreeView::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QWidget::changeEvent(e);
}

// Auto generated code (Tools/params_utils.py:670)
#include "moc_DlgSettingsTreeView.cpp"
//[[[end]]]

// -----------------------------------------------------------------------------------
// user code start


// user code end
// -----------------------------------------------------------------------------------
