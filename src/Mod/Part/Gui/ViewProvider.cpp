/***************************************************************************
 *   Copyright (c) 2004 Juergen Riegel <juergen.riegel@web.de>             *
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
#include <QObject>
#endif

#include <App/Document.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Gui/CommandT.h>

#include "ViewProvider.h"

#include <Base/Tools.h>


using namespace PartGui;

PROPERTY_SOURCE(PartGui::ViewProviderPart, PartGui::ViewProviderPartExt)  // NOLINT


ViewProviderPart::ViewProviderPart() = default;

ViewProviderPart::~ViewProviderPart() = default;

bool ViewProviderPart::doubleClicked()
{
    try {
        QString text = QObject::tr("Edit %1").arg(QString::fromUtf8(getObject()->Label.getValue()));
        Gui::Command::openCommand(text.toUtf8());
        Gui::cmdSetEdit(pcObject);
        return true;
    }
    catch (const Base::Exception& e) {
        Base::Console().error("%s\n", e.what());
        return false;
    }
}

void ViewProviderPart::applyColor(const Part::ShapeHistory& hist,
                                  const std::vector<Base::Color>& colBase,
                                  std::vector<Base::Color>& colBool)
{
    // apply color from modified faces
    for (const auto& jt : hist.shapeMap) {
        for (auto kt : jt.second) {
            colBool.at(kt) = colBase.at(jt.first);
        }
    }
}

void ViewProviderPart::applyMaterial(const Part::ShapeHistory& hist,
                                     const std::vector<App::Material>& colBase,
                                     std::vector<App::Material>& colBool)
{
    // apply color from modified faces
    for (const auto& jt : hist.shapeMap) {
        for (auto kt : jt.second) {
            colBool.at(kt) = colBase.at(jt.first);
        }
    }
}

void ViewProviderPart::applyTransparency(float transparency, std::vector<Base::Color>& colors)
{
    if (transparency != 0.0) {
        // transparency has been set object-wide
        for (auto& j : colors) {
            // transparency hasn't been set for this face
            if (j.a == 0.0) {
                j.setTransparency(Base::fromPercent(transparency));  // transparency comes in percent
            }
        }
    }
}

void ViewProviderPart::applyTransparency(float transparency, std::vector<App::Material>& colors)
{
    if (transparency != 0.0) {
        // transparency has been set object-wide
        for (auto& j : colors) {
            // transparency hasn't been set for this face
            if (j.transparency == 0.0) {
                j.transparency = Base::fromPercent(transparency);  // transparency comes in percent
            }
        }
    }
}

// ----------------------------------------------------------------------------

void ViewProviderShapeBuilder::buildNodes(const App::Property* prop, std::vector<SoNode*>& nodes) const
{
    Q_UNUSED(prop)
    Q_UNUSED(nodes)
}

void ViewProviderShapeBuilder::createShape(const App::Property* prop, SoSeparator* node) const
{
    Q_UNUSED(prop)
    Q_UNUSED(node)
}
