/***************************************************************************
 *   Copyright (c) 2012 Jan Rheinländer <jrheinlaender@users.sourceforge.net>        *
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

#include <Mod/PartDesign/App/FeatureDressUp.h>
#include <Gui/Application.h>

#include "ViewProviderDressUp.h"

#include "TaskDressUpParameters.h"

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderDressUp,PartDesignGui::ViewProvider)


void ViewProviderDressUp::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    QAction* act;
    act = menu->addAction(QObject::tr("Edit %1").arg(QString::fromStdString(featureName())), receiver, member);
    act->setData(QVariant((int)ViewProvider::Default));
    PartDesignGui::ViewProvider::setupContextMenu(menu, receiver, member);
}


const std::string & ViewProviderDressUp::featureName() const {
    static const std::string name = "Undefined";
    return name;
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
            QMessageBox::warning ( 0, QObject::tr("Feature error"),
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
    if (base == NULL) return;
    PartGui::ViewProviderPart* vp = dynamic_cast<PartGui::ViewProviderPart*>(
                Gui::Application::Instance->getViewProvider(base));
    if (vp == NULL) return;

    std::vector<std::string> faces = pcDressUp->Base.getSubValuesStartsWith("Face");
    std::vector<std::string> edges = pcDressUp->Base.getSubValuesStartsWith("Edge");

    if (on) {
        if (!faces.empty() && originalFaceColors.empty()) {
            TopTools_IndexedMapOfShape fMap;
            TopExp::MapShapes(base->Shape.getValue(), TopAbs_FACE, fMap);

            originalFaceColors = vp->DiffuseColor.getValues();
            std::vector<App::Color> colors = originalFaceColors;
            colors.resize(fMap.Extent(), ShapeColor.getValue());

            for (std::vector<std::string>::const_iterator f = faces.begin(); f != faces.end(); ++f) {
                // Note: std::stoi may throw in case of bad or very long face name, but screw the try {} catch
                int idx = std::stoi(f->substr(4)) - 1;
                assert ( idx>=0 );
                if ( idx < (ssize_t) colors.size() )
                    colors[idx] = App::Color(1.0,0.0,1.0); // magenta
            }
            vp->DiffuseColor.setValues(colors);
        }
        if (!edges.empty() && originalLineColors.empty()) {
            TopTools_IndexedMapOfShape eMap;
            TopExp::MapShapes(base->Shape.getValue(), TopAbs_EDGE, eMap);
            originalLineColors = vp->LineColorArray.getValues();
            std::vector<App::Color> colors = originalLineColors;
            colors.resize(eMap.Extent(), LineColor.getValue());

            for (std::vector<std::string>::const_iterator e = edges.begin(); e != edges.end(); ++e) {
                int idx = std::stoi(e->substr(4)) - 1;
                assert ( idx>=0 );
                if ( idx < (ssize_t) colors.size() )
                    colors[idx] = App::Color(1.0,0.0,1.0); // magenta
            }
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

