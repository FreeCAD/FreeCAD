/******************************************************************************
 *   Copyright (c) 2012 Jan Rheinländer <jrheinlaender@users.sourceforge.net> *
 *                                                                            *
 *   This file is part of the FreeCAD CAx development system.                 *
 *                                                                            *
 *   This library is free software; you can redistribute it and/or            *
 *   modify it under the terms of the GNU Library General Public              *
 *   License as published by the Free Software Foundation; either             *
 *   version 2 of the License, or (at your option) any later version.         *
 *                                                                            *
 *   This library  is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Library General Public License for more details.                     *
 *                                                                            *
 *   You should have received a copy of the GNU Library General Public        *
 *   License along with this library; see the file COPYING.LIB. If not,       *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,            *
 *   Suite 330, Boston, MA  02111-1307, USA                                   *
 *                                                                            *
 ******************************************************************************/


#include "PreCompiled.h"

#include "ViewProviderLinearPattern.h"
#include "TaskLinearPatternParameters.h"

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderLinearPattern,PartDesignGui::ViewProviderTransformed)

TaskDlgFeatureParameters *ViewProviderLinearPattern::getEditDialog() {
    return new TaskDlgLinearPatternParameters (this);
}

void ViewProviderLinearPattern::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    addDefaultAction(menu, QObject::tr("Edit linear pattern"));
    PartDesignGui::ViewProvider::setupContextMenu(menu, receiver, member);
}

const std::string & ViewProviderLinearPattern::featureName() const
{
    static const std::string name = "LinearPattern";
    return name;
}
