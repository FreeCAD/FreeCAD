/***************************************************************************
 *   Copyright (C) 2023 <bgbsww@gmail.com>                                 *
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
#include <QApplication>
#include <QMessageBox>
#endif

#include <App/DocumentObserver.h>
#include <Gui/Application.h>
#include <Gui/CommandT.h>
#include <Gui/MainWindow.h>
#include <Gui/BitmapFactory.h>
#include <Mod/PartDesign/App/Feature.h>
#include <Mod/PartDesign/App/Body.h>

#include "TaskAddSubParameters.h"

using namespace PartDesignGui;

TaskAddSubParameters::TaskAddSubParameters(PartDesignGui::ViewProvider *vp, QWidget *parent,
                                                     const std::string& pixmapname, const QString& parname)
    : TaskFeatureParameters(vp, parent, pixmapname, parname)
{
}


void TaskAddSubParameters::onOutsideChanged(bool on)
{
    PartDesign::FeatureAddSub* feature = static_cast<PartDesign::FeatureAddSub*>(vp->getObject());
    feature->Outside.setValue(on);
    recomputeFeature();
}

bool TaskAddSubParameters::enableOutside(PartDesignGui::ViewProvider* vp)  {
    PartDesign::FeatureAddSub* addsub = static_cast<PartDesign::FeatureAddSub*>(vp->getObject());
    return addsub->isSubtractive();
}

// Compiler wants this implemented, whether we need it or not.
void TaskAddSubParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (msg.Type == Gui::SelectionChanges::AddSelection) {
    }
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgAddSubParameters::TaskDlgAddSubParameters(PartDesignGui::ViewProvider *vp)
    : TaskDlgFeatureParameters(vp)
{
    // assert(vp);
    // Content.push_back ( new TaskAddSubParameters(vp2 ) );
}