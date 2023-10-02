/***************************************************************************
 *   Copyright (c) 2012 Jan Rheinländer                                    *
 *                                   <jrheinlaender@users.sourceforge.net> *
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
# include <QMenu>
# include <QAction>
# include <QMessageBox>
# include <TopTools_IndexedMapOfShape.hxx>
# include <TopExp.hxx>
#endif

#include <Gui/Application.h>
#include <Mod/Part/Gui/ReferenceHighlighter.h>
#include <Mod/PartDesign/App/FeatureDressUp.h>

#include "ViewProviderDressUp.h"
#include "TaskDressUpParameters.h"

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderDressUp,PartDesignGui::ViewProvider)


void ViewProviderDressUp::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    QString text = QString::fromStdString(getObject()->Label.getStrValue());
    addDefaultAction(menu, QObject::tr("Edit %1").arg(text));
    PartDesignGui::ViewProvider::setupContextMenu(menu, receiver, member);
}

const std::string & ViewProviderDressUp::featureName() const {
    static const std::string name = "Undefined";
    return name;
}

std::string ViewProviderDressUp::featureIcon() const
{
    return std::string("PartDesign_") + featureName();
}


bool ViewProviderDressUp::setEdit(int ModNum) {
    if (ModNum == ViewProvider::Default ) {
        // Here we should prevent edit of a Feature with missing base
        // Otherwise it could call unhandled exception.
        PartDesign::DressUp* dressUp = static_cast<PartDesign::DressUp*>(getObject());
        assert (dressUp);
        if (dressUp->getBaseObject (/*silent =*/ true)) {
            return ViewProvider::setEdit(ModNum);
        } else {
            QMessageBox::warning ( nullptr, QObject::tr("Feature error"),
                    QObject::tr("%1 misses a base feature.\n"
                           "This feature is broken and can't be edited.")
                        .arg( QString::fromLatin1(dressUp->getNameInDocument()) )
                );
            return false;
        }

    } else {
        return ViewProvider::setEdit(ModNum);
    }
}


void ViewProviderDressUp::highlightReferences(const bool on)
{
    PartDesign::DressUp* pcDressUp = static_cast<PartDesign::DressUp*>(getObject());
    Part::Feature* base = pcDressUp->getBaseObject (/*silent =*/ true);
    if (!base)
        return;
    PartGui::ViewProviderPart* vp = dynamic_cast<PartGui::ViewProviderPart*>(
                Gui::Application::Instance->getViewProvider(base));
    if (!vp)
        return;

    std::vector<std::string> faces = pcDressUp->Base.getSubValuesStartsWith("Face");
    std::vector<std::string> edges = pcDressUp->Base.getSubValuesStartsWith("Edge");

    if (on) {
        if (!faces.empty() && originalFaceColors.empty()) {
            originalFaceColors = vp->DiffuseColor.getValues();
            std::vector<App::Color> colors = originalFaceColors;

            PartGui::ReferenceHighlighter highlighter(base->Shape.getValue(), ShapeColor.getValue());
            highlighter.getFaceColors(faces, colors);
            vp->DiffuseColor.setValues(colors);
        }
        if (!edges.empty() && originalLineColors.empty()) {
            originalLineColors = vp->LineColorArray.getValues();
            std::vector<App::Color> colors = originalLineColors;

            PartGui::ReferenceHighlighter highlighter(base->Shape.getValue(), LineColor.getValue());
            highlighter.getEdgeColors(edges, colors);
            vp->LineColorArray.setValues(colors);
        }
    } else {
        if (!faces.empty() && !originalFaceColors.empty()) {
            vp->DiffuseColor.setValues(originalFaceColors);
            originalFaceColors.clear();
        }
        if (!edges.empty() && !originalLineColors.empty()) {
            vp->LineColorArray.setValues(originalLineColors);
            originalLineColors.clear();
        }
    }
}

