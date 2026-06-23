// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2026 Andrew Shkolik <shkolik@gmail.com>                  *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
# include <QAction>
# include <QMenu>

# include <TopExp.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
#endif

#include <App/DocumentObject.h>
#include <Base/Color.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Control.h>
#include <Mod/Part/Gui/ViewProvider.h>

#include "ViewProviderGordonSurface.h"
#include "TaskGordonSurface.h"

#include <map>

PROPERTY_SOURCE(SurfaceGui::ViewProviderGordonSurface, PartGui::ViewProviderSpline)

namespace SurfaceGui
{

void ViewProviderGordonSurface::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    QAction* act;
    act = menu->addAction(QObject::tr("Edit gordon surface"), receiver, member);
    act->setData(QVariant((int)ViewProvider::Default));
    PartGui::ViewProviderSpline::setupContextMenu(menu, receiver, member);
}

bool ViewProviderGordonSurface::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default) {
        Surface::GordonSurface* obj = this->getObject<Surface::GordonSurface>();

        Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();

        // start the edit dialog
        if (dlg) {
            TaskGordonSurface* tDlg = qobject_cast<TaskGordonSurface*>(dlg);
            if (tDlg) {
                tDlg->setEditedObject(obj);
            }
            Gui::Control().showDialog(dlg);
        }
        else {
            Gui::Control().showDialog(new TaskGordonSurface(this, obj));
        }
        return true;
    }
    else {
        return ViewProviderSpline::setEdit(ModNum);
    }
}

void ViewProviderGordonSurface::unsetEdit(int ModNum)
{
    PartGui::ViewProviderSpline::unsetEdit(ModNum);
}

QIcon ViewProviderGordonSurface::getIcon() const
{
    return Gui::BitmapFactory().pixmap("Surface_GordonSurface");
}

void ViewProviderGordonSurface::highlightReferences(
    const References& profiles,
    const References& guides,
    bool on
)
{
    std::map<App::DocumentObject*, std::vector<std::string>> subs;

    auto append_unique = [](std::vector<std::string>& vec, const std::vector<std::string>& values) {
        for (const auto& val : values) {
            if (std::ranges::find(vec, val) == vec.end()) {
                vec.push_back(val);
            }
        }
    };

    for (const auto& [obj, subnames] : profiles) {
        if (!subs.contains(obj)) {
            subs[obj] = std::vector<std::string>();
        }
        append_unique(subs[obj], subnames);
    }

    for (const auto& [obj, subnames] : guides) {
        if (!subs.contains(obj)) {
            subs[obj] = std::vector<std::string>();
        }
        append_unique(subs[obj], subnames);
    }

    for (const auto& [obj, subnames] : subs) {
        auto const* base = dynamic_cast<Part::Feature*>(obj);
        if (base) {
            auto* svp = dynamic_cast<PartGui::ViewProviderPartExt*>(
                Gui::Application::Instance->getViewProvider(base)
            );
            if (!svp) {
                continue;
            }
            svp->unsetHighlightedEdges();
            if (on) {
                std::vector<Base::Color> colors;
                TopTools_IndexedMapOfShape eMap;
                TopExp::MapShapes(base->Shape.getValue(), TopAbs_EDGE, eMap);
                colors.resize(eMap.Extent(), svp->LineColor.getValue());

                for (const auto& jt : subnames) {
                    std::size_t idx = static_cast<std::size_t>(std::stoi(jt.substr(4)) - 1);
                    // check again that the index is in range because it's possible that
                    // the sub-names are invalid
                    if (idx < colors.size()) {
                        colors[idx] = Base::Color(1.0, 0.0, 1.0);  // magenta
                    }
                }
                svp->setHighlightedEdges(colors);
            }
        }
    }
}

}  // namespace SurfaceGui
