// SPDX-License-Identifier: LGPL-2.1-or-later

#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/Selection/Selection.h>

#include "TaskProjectOnSurface.h"
#include "ViewProviderProjectOnSurface.h"

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderProjectOnSurface, PartGui::ViewProviderProjectOnSurface)

ViewProviderProjectOnSurface::ViewProviderProjectOnSurface()
{
    sPixmap = "Part_ProjectionOnSurface";
}

bool ViewProviderProjectOnSurface::setEdit(int mode)
{
    if (mode == ViewProvider::Default) {
        if (Gui::Control().activeDialog()) {
            return false;
        }
        Gui::Selection().clearSelection();
        Gui::Control().showDialog(new TaskDlgProjectOnSurface(this));
        return true;
    }
    return PartGui::ViewProviderProjectOnSurface::setEdit(mode);
}

void ViewProviderProjectOnSurface::unsetEdit(int mode)
{
    if (mode == ViewProvider::Default) {
        Gui::Control().closeDialog(getDocument()->getDocument());
    }
    else {
        PartGui::ViewProviderProjectOnSurface::unsetEdit(mode);
    }
}
