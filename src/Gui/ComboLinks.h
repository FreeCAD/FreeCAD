// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 AstoCAD                  <hello@astocad.com>        *
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

#include <vector>
#include <string>

#include <App/PropertyLinks.h>
#include <Base/Exception.h>

// Forward declarations
class QComboBox;
class QString;
namespace App
{
class Document;
class DocumentObject;
}  // namespace App

namespace Gui
{

/**
 * @brief The ComboLinks class is a helper class that binds to a QComboBox and
 * provides an interface to add App::PropertyLinkSub items, retrieve links,
 * and select items by link value.
 */
class GuiExport ComboLinks
{
public:
    /**
     * @brief Constructor. Binds to an existing QComboBox.
     * @param combo The combo box to manage. It will be cleared upon binding.
     * Do not add/remove items directly to/from the combo after binding.
     */
    explicit ComboLinks(QComboBox* combo);

    /**
     * @brief Default constructor. Use setCombo() later.
     */
    ComboLinks() = default;

    /**
     * @brief Destructor. Clears internal resources.
     */
    ~ComboLinks();

    FC_DISABLE_COPY_MOVE(ComboLinks)

    /**
     * @brief Binds the helper to a QComboBox. Clears the combo box.
     * @param combo The combo box to manage.
     */
    void setCombo(QComboBox* combo);

    /**
     * @brief Adds an item to the combo box associated with a PropertyLinkSub.
     * @param lnk The link data. Can point to nullptr (e.g., for "Select reference...").
     * @param itemText The text to display in the combo box.
     * @param userData Optional integer data associated with the item (default: item index).
     * @return The index of the added item.
     */
    int addLink(const App::PropertyLinkSub& lnk, const QString& itemText, int userData = -1);

    /**
     * @brief Adds an item to the combo box associated with specific object and subname.
     * @param linkObj The document object (can be nullptr).
     * @param linkSubname The sub-element name (e.g., "Edge1", "Face2", "X_Axis").
     * @param itemText The text to display in the combo box.
     * @param userData Optional integer data associated with the item (default: item index).
     * @return The index of the added item.
     */
    int addLink(
        App::DocumentObject* linkObj,
        const std::string& linkSubname,
        const QString& itemText,
        int userData = -1
    );


    /**
     * @brief Adds an item *before* an item identified by its user data.
     * Useful for inserting custom items before a standard item like "Select reference...".
     * If targetUserData is not found, appends to the end.
     *
     * @param lnk The link data.
     * @param itemText The text to display.
     * @param targetUserData The user data of the item to insert before.
     * @param userData Optional integer data for the new item (default: item index).
     * @return The index of the inserted item.
     */
    int addLinkBefore(
        const App::PropertyLinkSub& lnk,
        const QString& itemText,
        int targetUserData,
        int userData = -1
    );


    /**
     * @brief Clears all items from the combo box and the internal list.
     */
    void clear();

    /**
     * @brief Gets the PropertyLinkSub associated with the item at the given index.
     * @param index The index of the item.
     * @return A reference to the stored PropertyLinkSub.
     * @throws Base::IndexError if index is out of range.
     * @throws Base::ValueError if the linked object exists but is not in the tracked document.
     */
    App::PropertyLinkSub& getLink(int index) const;

    /**
     * @brief Gets the PropertyLinkSub associated with the currently selected item.
     * @return A reference to the stored PropertyLinkSub.
     * @throws Base::IndexError if no item is selected or combo is invalid.
     * @throws Base::ValueError if linked object validity check fails.
     */
    App::PropertyLinkSub& getCurrentLink() const;

    /**
     * @brief Gets the user data associated with the item at the given index.
     * @param index The index of the item.
     * @return The user data. Returns -1 if index is invalid.
     */
    int getUserData(int index) const;

    /**
     * @brief Gets the user data associated with the currently selected item.
     * @return The user data. Returns -1 if no item is selected or combo is invalid.
     */
    int getCurrentUserData() const;

    /**
     * @brief Selects the item whose PropertyLinkSub matches the provided link.
     * Blocks combo box signals during the operation.
     * @param lnk The link to match.
     * @return The index of the selected item, or -1 if no match is found.
     */
    int setCurrentLink(const App::PropertyLinkSub& lnk);

    /**
     * @brief Gets the number of items in the combo box.
     * @return Item count.
     */
    int count() const;


private:
    QComboBox* combo = nullptr;
    App::Document* doc = nullptr;                    // Document context for validation
    std::vector<App::PropertyLinkSub*> linksInList;  // Owned pointers
};

}  // namespace Gui
