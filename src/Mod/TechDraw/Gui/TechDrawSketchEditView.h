// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2026 AstoCAD     <hello@astocad.com>                     *
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

#pragma once

#include <string>

#include <fastsignals/connection.h>
#include <QPointer>

#include <Gui/View3DInventor.h>
#include <Mod/TechDraw/TechDrawGlobal.h>

class QCloseEvent;
class QMdiSubWindow;

namespace App
{
class DocumentObject;
}

namespace Gui
{
class ViewProvider;
}

namespace TechDraw
{
class DrawView;
}

namespace TechDrawGui
{

class MDIViewPage;
class PageBackgroundViewProvider;

class TechDrawGuiExport TechDrawSketchEditView: public Gui::View3DInventor
{
public:
    TechDrawSketchEditView(Gui::Document* document,
                           MDIViewPage* pageView,
                           App::DocumentObject* sketch,
                           TechDraw::DrawView* owner);
    ~TechDrawSketchEditView() override;

    bool startSketchEdit();
    const char* getName() const override;

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    bool installEditingView();
    bool installOwnerExternalGeometry();
    void alignCameraToPage();
    void installPageBackground();
    void removePageBackground();
    void restoreWorkbench();
    void restorePage();

    QPointer<MDIViewPage> m_pageView;
    App::DocumentObject* m_sketch;
    TechDraw::DrawView* m_owner;
    Gui::ViewProvider* m_sketchViewProvider {nullptr};
    PageBackgroundViewProvider* m_pageBackgroundProvider {nullptr};
    QPointer<QMdiSubWindow> m_pageSubWindow;
    fastsignals::connection m_resetEditConnection;
    std::string m_previousWorkbench;
    bool m_restoring {false};
};

}  // namespace TechDrawGui
