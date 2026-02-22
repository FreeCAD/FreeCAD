// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/****************************************************************************
 *   Copyright (c) 2012 Jan Rheinländer jrheinlaender@users.sourceforge.net *
 *   Copyright (c) 2025 AstoCAD                  <hello@astocad.com>        *
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

#include "ComboLinks.h"

#include <QComboBox>
#include <QVariant>
#include <QString>

#include <App/Document.h>
#include <App/DocumentObject.h>

namespace Gui
{

ComboLinks::ComboLinks(QComboBox* combo)
    : combo(combo)
{
    if (combo) {
        combo->clear();
    }
}

ComboLinks::~ComboLinks()
{
    clear();  // Deletes owned pointers in linksInList
}

void ComboLinks::setCombo(QComboBox* combobox)
{
    clear();  // Clear old state if any
    combo = combobox;
    if (combo) {
        combo->clear();
    }
}

int ComboLinks::addLink(const App::PropertyLinkSub& lnk, const QString& itemText, int userData)
{
    if (!combo) {
        return -1;
    }
    int newIndex = combo->count();
    int finalUserData = (userData == -1) ? newIndex : userData;

    // Store link internally (create a copy)
    auto* newLink = new App::PropertyLinkSub();
    newLink->Paste(lnk);
    linksInList.push_back(newLink);

    // Add to combo box
    combo->addItem(itemText, QVariant(finalUserData));

    // Track document context from the first valid object link
    if (!doc && newLink->getValue()) {
        doc = newLink->getValue()->getDocument();
    }
    return newIndex;
}

int ComboLinks::addLink(
    App::DocumentObject* linkObj,
    const std::string& linkSubname,
    const QString& itemText,
    int userData
)
{
    App::PropertyLinkSub lnk;
    std::vector<std::string> sub = {linkSubname};
    lnk.setValue(linkObj, sub);
    return addLink(lnk, itemText, userData);
}

int ComboLinks::addLinkBefore(
    const App::PropertyLinkSub& lnk,
    const QString& itemText,
    int targetUserData,
    int userData
)
{
    if (!combo) {
        return -1;
    }

    int insertPos = -1;
    for (int i = 0; i < combo->count(); ++i) {
        if (combo->itemData(i).toInt() == targetUserData) {
            insertPos = i;
            break;
        }
    }

    // Store link internally (create a copy)
    auto* newLink = new App::PropertyLinkSub();
    newLink->Paste(lnk);

    int finalUserData = (userData == -1) ? ((insertPos == -1) ? count() : insertPos) : userData;

    if (insertPos != -1) {
        linksInList.insert(linksInList.begin() + insertPos, newLink);
        combo->insertItem(insertPos, itemText, QVariant(finalUserData));
    }
    else {
        // Target not found, append to end
        insertPos = combo->count();
        linksInList.push_back(newLink);
        combo->addItem(itemText, QVariant(finalUserData));
    }

    // Update user data for subsequent items if default (-1) was used and inserting
    if (userData == -1 && insertPos != -1 && insertPos < count() - 1) {
        for (int i = insertPos + 1; i < count(); ++i) {
            if (combo->itemData(i).toInt() == i - 1) {  // Check if it was using default index
                combo->setItemData(i, QVariant(i));
            }
        }
    }


    // Track document context
    if (!doc && newLink->getValue()) {
        doc = newLink->getValue()->getDocument();
    }
    return insertPos;
}


void ComboLinks::clear()
{
    if (combo) {
        // Block signals while clearing to prevent issues if connected elsewhere
        QSignalBlocker blocker(combo);
        combo->clear();
    }
    for (App::PropertyLinkSub* linkPtr : linksInList) {
        delete linkPtr;  // Delete the objects pointed to
    }
    linksInList.clear();  // Clear the vector itself
    doc = nullptr;        // Reset document context
}

App::PropertyLinkSub& ComboLinks::getLink(int index) const
{
    if (index < 0 || static_cast<size_t>(index) >= linksInList.size()) {
        throw Base::IndexError("ComboLinks::getLink: Index out of range");
    }
    App::PropertyLinkSub* linkPtr = linksInList[static_cast<size_t>(index)];
    // Perform validity check only if we have a document context and a linked object
    if (doc && linkPtr->getValue() && !(doc->isIn(linkPtr->getValue()))) {
        throw Base::ValueError("Linked object is not in the document; it may have been deleted");
    }
    return *linkPtr;
}

App::PropertyLinkSub& ComboLinks::getCurrentLink() const
{
    assert(combo);
    return getLink(combo->currentIndex());
}

int ComboLinks::getUserData(int index) const
{
    if (!combo || index < 0 || index >= combo->count()) {
        return -1;  // Indicate invalid index or no combo
    }
    return combo->itemData(index).toInt();
}

int ComboLinks::getCurrentUserData() const
{
    if (!combo) {
        return -1;
    }
    return combo->currentData().toInt();
}


int ComboLinks::setCurrentLink(const App::PropertyLinkSub& lnk)
{
    if (!combo) {
        return -1;
    }
    for (size_t i = 0; i < linksInList.size(); ++i) {
        const App::PropertyLinkSub* it = linksInList[i];
        // Compare object pointer and sub-values vector
        if (lnk.getValue() == it->getValue() && lnk.getSubValues() == it->getSubValues()) {
            QSignalBlocker blocker(combo);
            combo->setCurrentIndex(static_cast<int>(i));
            return static_cast<int>(i);
        }
    }
    return -1;  // Not found
}

int ComboLinks::count() const
{
    return combo ? combo->count() : 0;
}

}  // namespace Gui
