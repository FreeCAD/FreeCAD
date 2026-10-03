// SPDX-License-Identifier: LGPL-2.1-or-later

#include <Gui/BitmapFactory.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/Selection/Selection.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TreeItemMode.h>
#include <App/Document.h>
#include <Mod/Surface/App/FeatureFreehandBSpline.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoSeparator.h>
#include "FreehandBSplineEditor.h"
#include "ViewProviderFreehandBSpline.h"

using namespace SurfaceGui;
PROPERTY_SOURCE(SurfaceGui::ViewProviderFreehandBSpline, PartGui::ViewProviderSpline)

namespace
{
class TaskFreehandBSpline: public Gui::TaskView::TaskDialog
{
public:
    TaskFreehandBSpline(ViewProviderFreehandBSpline* provider, FreehandBSplineEditor* widget)
        : vp(provider)
        , panel(widget)
    {
        addTaskBox(Gui::BitmapFactory().pixmap("Surface_FreehandBSpline"), panel);
        setDocumentName(vp->getObject()->getDocument()->getName());
        setAutoCloseOnTransactionChange(false);
    }
    bool accept() override
    {
        if (!panel->validate()) {
            return false;
        }
        auto doc = vp->getDocument();
        doc->resetEdit();
        doc->getDocument()->recompute();
        doc->commitCommand();
        return true;
    }
    bool reject() override
    {
        auto doc = vp->getDocument();
        doc->abortCommand();
        doc->resetEdit();
        doc->getDocument()->recompute();
        return true;
    }

private:
    ViewProviderFreehandBSpline* vp;
    FreehandBSplineEditor* panel;
};
}  // namespace

QIcon ViewProviderFreehandBSpline::getIcon() const
{
    return Gui::BitmapFactory().pixmap("Surface_FreehandBSpline");
}

bool ViewProviderFreehandBSpline::doubleClicked()
{
    startDefaultEditMode();
    return true;
}

bool ViewProviderFreehandBSpline::isSelectable() const
{
    return !editor && ViewProviderSpline::isSelectable();
}

void ViewProviderFreehandBSpline::onChanged(const App::Property* property)
{
    ViewProviderSpline::onChanged(property);
    if (editor && property == &Selectable) {
        setSelectable(false);
    }
}

bool ViewProviderFreehandBSpline::setEdit(int mode)
{
    if (mode != Default) {
        return ViewProviderSpline::setEdit(mode);
    }
    if (Gui::Control().activeDialog()) {
        return false;
    }
    if (!getDocument()->hasPendingCommand()) {
        getDocument()->openCommand(QT_TRANSLATE_NOOP("Command", "Edit freehand B-spline"));
    }
    // Clear the old highlight before the editor starts rebuilding the shape during drags.
    Gui::Selection().rmvPreselect();
    Gui::Selection().rmvSelection(
        getObject()->getDocument()->getName(),
        getObject()->getNameInDocument()
    );
    static_cast<Surface::FreehandBSpline*>(getObject())->sanitizeReferences();
    // Construct the panel before hiding the normal display.
    editor = new FreehandBSplineEditor(this);
    // Wrap the normal display so shape updates cannot make it visible during editing.
    auto root = getRoot();
    auto normal = getModeSwitch();
    editDisplaySwitch = new SoSwitch;
    editDisplaySwitch->setName("FreehandBSplineHiddenDisplay");
    editDisplaySwitch->whichChild = SO_SWITCH_NONE;
    editDisplaySwitch->addChild(normal);
    root->replaceChild(normal, editDisplaySwitch);
    // Change only the scene's picking state, preserving the saved Selectable preference.
    setSelectable(false);
    Gui::Control().showDialog(new TaskFreehandBSpline(this, editor));
    signalChangeHighlight(true, Gui::HighlightMode::Bold);
    return true;
}

void ViewProviderFreehandBSpline::unsetEdit(int mode)
{
    if (mode != Default) {
        ViewProviderSpline::unsetEdit(mode);
        return;
    }
    if (editor) {
        editor->setViewer(nullptr);
    }
    editor = nullptr;
    if (editDisplaySwitch) {
        getRoot()->replaceChild(editDisplaySwitch, getModeSwitch());
        editDisplaySwitch = nullptr;
    }
    setSelectable(Selectable.getValue());
    signalChangeHighlight(false, Gui::HighlightMode::Bold);
    Gui::Control().closeDialog();
}

void ViewProviderFreehandBSpline::setEditViewer(Gui::View3DInventorViewer* viewer, int mode)
{
    ViewProviderSpline::setEditViewer(viewer, mode);
    if (editor) {
        editor->setViewer(viewer);
    }
}

void ViewProviderFreehandBSpline::unsetEditViewer(Gui::View3DInventorViewer* viewer)
{
    if (editor) {
        editor->setViewer(nullptr);
    }
    ViewProviderSpline::unsetEditViewer(viewer);
}

bool ViewProviderFreehandBSpline::selectAll()
{
    if (!editor) {
        return false;
    }
    editor->selectAllPoints();
    return true;
}

bool ViewProviderFreehandBSpline::onDelete(const std::vector<std::string>& subNames)
{
    if (editor) {
        editor->deletePoints();
        return false;
    }
    return ViewProviderSpline::onDelete(subNames);
}
