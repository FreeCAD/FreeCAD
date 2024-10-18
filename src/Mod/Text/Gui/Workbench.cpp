/***************************************************************************
 *   Copyright (c) 2024 Martin Rodriguez Reboredo <yakoyoku@gmail.com>     *
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

#include "Utils.h"
#include "Workbench.h"
#include <Base/Console.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/WorkbenchManager.h>

using namespace TextGui;

#if 0// needed for Qt's lupdate utility
    qApp->translate("CommandGroup", "Text");
    qApp->translate("Workbench","P&rofiles");
    qApp->translate("Workbench","T&ext");
    qApp->translate("Workbench", "Text");
    qApp->translate("Workbench", "Text edit mode");
    qApp->translate("Workbench", "Text virtual space");
    qApp->translate("Workbench", "Text edit tools");
#endif

/// @namespace TextGui @class Workbench
TYPESYSTEM_SOURCE(TextGui::Workbench, Gui::StdWorkbench)

Workbench::Workbench()
{}

Workbench::~Workbench()
{}

Gui::MenuItem* Workbench::setupMenuBar() const
{
    Gui::MenuItem* root = StdWorkbench::setupMenuBar();
    Gui::MenuItem* item = root->findItem("&Windows");

    // == Text menu ==========================================

    Gui::MenuItem* text = new Gui::MenuItem;
    root->insertItem(item, text);
    text->setCommand("T&ext");
    addTextWorkbenchTextActions(*text);
    addTextWorkbenchTextEditModeActions(*text);

    return root;
}

Gui::ToolBarItem* Workbench::setupToolBars() const
{
    Gui::ToolBarItem* root = StdWorkbench::setupToolBars();

    Gui::ToolBarItem* text= new Gui::ToolBarItem(root);
    text->setCommand("Text");
    addTextWorkbenchTextActions(*text);

    Gui::ToolBarItem* textEditMode =
        new Gui::ToolBarItem(root, Gui::ToolBarItem::DefaultVisibility::Unavailable);
    textEditMode->setCommand("Text edit mode");
    addTextWorkbenchTextEditModeActions(*textEditMode);

    return root;
}

Gui::ToolBarItem* Workbench::setupCommandBars() const
{
    // Text tools
    Gui::ToolBarItem* root = new Gui::ToolBarItem;
    return root;
}


namespace
{
inline const QStringList editModeToolbarNames()
{
    return QStringList {QString::fromLatin1("Text edit mode"),
                        // QString::fromLatin1("Text virtual space"),
                        QString::fromLatin1("Text edit tools")};
}

inline const QStringList nonEditModeToolbarNames()
{
    return QStringList {QString::fromLatin1("Structure"), QString::fromLatin1("Text")};
}
}// namespace

void Workbench::activated()
{
    /* When the workbench is activated, it may happen that we are in edit mode or not.
     * If we are not in edit mode, the function enterEditMode (called by the ViewProvider) takes
     * care to save the state of toolbars outside of edit mode. We cannot do it here, as we are
     * coming from another WB.
     *
     * If we moved to another WB from edit mode, the new WB was activated before deactivating this.
     * Therefore we had no chance to tidy up the save state. We assume a loss of any CHANGE to
     * toolbar configuration since last entering edit mode in this case (for any change in
     * configuration to be stored, the edit mode must be left while the selected Workbench is the
     * sketcher WB).
     *
     * However, now that we are back (from another WB), we need to make the toolbars available.
     * These correspond to the last saved state.
     */
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    if (isShapeTextInEdit(doc)) {
        Gui::ToolBarManager::getInstance()->setState(editModeToolbarNames(),
                                                     Gui::ToolBarManager::State::ForceAvailable);
    }
}

void Workbench::enterEditMode()
{
    /* Ensure the state left by the non-edit mode toolbars is saved (in case of changing to edit
     * mode) without changing workbench
     */
    Gui::ToolBarManager::getInstance()->setState(nonEditModeToolbarNames(),
                                                 Gui::ToolBarManager::State::SaveState);

    Gui::ToolBarManager::getInstance()->setState(editModeToolbarNames(),
                                                 Gui::ToolBarManager::State::ForceAvailable);
    Gui::ToolBarManager::getInstance()->setState(nonEditModeToolbarNames(),
                                                 Gui::ToolBarManager::State::ForceHidden);
}

void Workbench::leaveEditMode()
{
    /* Ensure the state left by the edit mode toolbars is saved (in case of changing to edit mode)
     * without changing workbench.
     *
     * However, do not save state if the current workbench is not the Text WB, because otherwise
     * we would be saving the state of the currently activated workbench, and the toolbars would
     * disappear (as the toolbars of that other WB are only visible).
     */
    auto* workbench = Gui::WorkbenchManager::instance()->active();

    if (workbench->name() == "TextWorkbench") {
        Gui::ToolBarManager::getInstance()->setState(editModeToolbarNames(),
                                                     Gui::ToolBarManager::State::SaveState);
    }

    Gui::ToolBarManager::getInstance()->setState(editModeToolbarNames(),
                                                 Gui::ToolBarManager::State::RestoreDefault);
    Gui::ToolBarManager::getInstance()->setState(nonEditModeToolbarNames(),
                                                 Gui::ToolBarManager::State::RestoreDefault);
}

namespace TextGui {

template<typename T>
void TextAddWorkbenchTextActions(T& text);

template<>
inline void TextAddWorkbenchTextActions(Gui::MenuItem& text)
{
    text << "Text_NewText"
         << "Text_EditText"
         << "Text_MapText"
         << "Text_ReorientText";
}
template<>
inline void TextAddWorkbenchTextActions(Gui::ToolBarItem& text)
{
    text << "Text_NewText"
         << "Text_EditText"
         << "Text_MapText"
         << "Text_ReorientText";
}

template<typename T>
void TextAddWorkbenchTextEditModeActions(T& text);

template<>
inline void TextAddWorkbenchTextEditModeActions(Gui::MenuItem& text)
{
    text << "Text_LeaveText"
         << "Text_ViewText"
         << "Text_ViewSection"
         << "Text_StopOperation";
}
template<>
inline void TextAddWorkbenchTextEditModeActions(Gui::ToolBarItem& text)
{
    text << "Text_LeaveText"
         << "Text_ViewText"
         << "Text_ViewSection";
}

void addTextWorkbenchTextActions(Gui::MenuItem& text)
{
    TextAddWorkbenchTextActions(text);
}

void addTextWorkbenchTextEditModeActions(Gui::MenuItem& text)
{
    TextAddWorkbenchTextEditModeActions(text);
}

void addTextWorkbenchTextActions(Gui::ToolBarItem& text)
{
    TextAddWorkbenchTextActions(text);
}

void addTextWorkbenchTextEditModeActions(Gui::ToolBarItem& text)
{
    TextAddWorkbenchTextEditModeActions(text);
}

} /* namespace TextGui */
