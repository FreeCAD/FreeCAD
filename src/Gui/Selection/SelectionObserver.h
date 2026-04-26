// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2011 Juergen Riegel <juergen.riegel@web.de>
// SPDX-FileCopyrightText: 2011 Werner Mayer <wmayer[at]users.sourceforge.net>
// SPDX-FileCopyrightText: 2026 FreeCAD Project Association
// SPDX-FileNotice: Part of the FreeCAD project.
/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#pragma once

#include <string>

#include <Base/Observer.h>
#include <FCGlobal.h>

#include "SelectionChanges.h"
#include "SelectionTypes.h"

namespace Gui
{

class ViewProviderDocumentObject;

/**
 * The SelectionObserver class simplifies the step to write classes that listen
 * to what happens to the selection.
 *
 * @author Werner Mayer
 */
class GuiExport SelectionObserver
{

public:
    /** Constructor
     *
     * @param attach: whether to   attach this observer on construction
     * @param resolve: sub-object resolving mode.
     */
    explicit SelectionObserver(bool attach = true, ResolveMode resolve = ResolveMode::OldStyleElement);
    /** Constructor
     *
     * @param vp: filtering view object.
     * @param attach: whether to attach this observer on construction
     * @param resolve: sub-object resolving mode.
     *
     * Constructs an selection observer that receives only selection event of
     * objects within the same document as the input view object.
     */
    explicit SelectionObserver(
        const Gui::ViewProviderDocumentObject* vp,
        bool attach = true,
        ResolveMode resolve = ResolveMode::OldStyleElement
    );

    virtual ~SelectionObserver();
    bool blockSelection(bool block);
    bool isSelectionBlocked() const;
    bool isSelectionAttached() const;

    /** Attaches to the selection. */
    void attachSelection();
    /** Detaches from the selection. */
    void detachSelection();
    /** clears the document scope filter, allowing cross-document selection events. */
    void clearDocumentScope()
    {
        documentScopeName.clear();
    }

private:
    virtual void onSelectionChanged(const SelectionChanges& msg) = 0;
    void _onSelectionChanged(const SelectionChanges& msg);

private:
    using Connection = fastsignals::connection;
    Connection connectSelection;
    std::string filterDocName;
    std::string filterObjName;
    ResolveMode resolve;
    std::string documentScopeName;
    bool blockedSelection;
};

}  // namespace Gui
