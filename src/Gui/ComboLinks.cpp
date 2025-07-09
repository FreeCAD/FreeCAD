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

#include "PreCompiled.h"

#include "ComboLinks.h"

#ifndef _PreComp_
#include <QComboBox>
#include <QVariant>
#include <QString>
#endif

#include <App/Document.h>
#include <App/DocumentObject.h>

namespace Gui {

    ComboLinks::ComboLinks(QComboBox& combo)
        : _combo(&combo)
    {
        if (_combo) {
            _combo->clear();
        }
    }

    ComboLinks::~ComboLinks()
    {
        clear(); // Deletes owned pointers in linksInList
        _combo = nullptr; // Don't delete, not owned
    }

    void ComboLinks::setCombo(QComboBox& combo)
    {
        clear(); // Clear old state if any
        _combo = &combo;
        if (_combo) {
            _combo->clear();
        }
    }

    int ComboLinks::addLink(const App::PropertyLinkSub& lnk, const QString& itemText, int userData)
    {
        if (!_combo) {
            return -1;
        }
        int newIndex = _combo->count();
        int finalUserData = (userData == -1) ? newIndex : userData;

        // Store link internally (create a copy)
        auto* newLink = new App::PropertyLinkSub();
        newLink->Paste(lnk);
        linksInList.push_back(newLink);

        // Add to combo box
        _combo->addItem(itemText, QVariant(finalUserData));

        // Track document context from the first valid object link
        if (!doc && newLink->getValue()) {
            doc = newLink->getValue()->getDocument();
        }
        return newIndex;
    }

    int ComboLinks::addLink(App::DocumentObject* linkObj, const std::string& linkSubname, const QString& itemText, int userData)
    {
        App::PropertyLinkSub lnk;
        std::vector<std::string> sub = { linkSubname };
        // Handle empty subname correctly
        if (linkSubname.empty()) {
            sub.clear();
        }
        lnk.setValue(linkObj, sub);
        return addLink(lnk, itemText, userData);
    }

    int ComboLinks::addLinkBefore(const App::PropertyLinkSub& lnk, const QString& itemText, int targetUserData, int userData)
    {
        if (!_combo) {
            return -1;
        }

        int insertPos = -1;
        for (int i = 0; i < _combo->count(); ++i) {
            if (_combo->itemData(i).toInt() == targetUserData) {
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
            _combo->insertItem(insertPos, itemText, QVariant(finalUserData));
        }
        else {
            // Target not found, append to end
            insertPos = _combo->count();
            linksInList.push_back(newLink);
            _combo->addItem(itemText, QVariant(finalUserData));
        }

        // Update user data for subsequent items if default (-1) was used and inserting
        if (userData == -1 && insertPos != -1 && insertPos < count() - 1) {
            for (int i = insertPos + 1; i < count(); ++i) {
                if (_combo->itemData(i).toInt() == i - 1) { // Check if it was using default index
                    _combo->setItemData(i, QVariant(i));
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
        if (_combo) {
            // Block signals while clearing to prevent issues if connected elsewhere
            bool wasBlocked = _combo->signalsBlocked();
            _combo->blockSignals(true);
            _combo->clear();
            _combo->blockSignals(wasBlocked);
        }
        for (App::PropertyLinkSub* linkPtr : linksInList) {
            delete linkPtr; // Delete the objects pointed to
        }
        linksInList.clear(); // Clear the vector itself
        doc = nullptr; // Reset document context
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
        assert(_combo);
        return getLink(_combo->currentIndex());

    }

    int ComboLinks::getUserData(int index) const
    {
        if (!_combo || index < 0 || index >= _combo->count()) {
            return -1; // Indicate invalid index or no combo
        }
        return _combo->itemData(index).toInt();
    }

    int ComboLinks::getCurrentUserData() const
    {
        if (!_combo) {
            return -1;
        }
        return _combo->currentData().toInt();
    }


    int ComboLinks::setCurrentLink(const App::PropertyLinkSub& lnk)
    {
        if (!_combo) {
            return -1;
        }
        for (size_t i = 0; i < linksInList.size(); ++i) {
            const App::PropertyLinkSub& it = *(linksInList[i]);
            // Compare object pointer and sub-values vector
            if (lnk.getValue() == it.getValue() && lnk.getSubValues() == it.getSubValues()) {
                bool wasBlocked = _combo->signalsBlocked();
                _combo->blockSignals(true);
                _combo->setCurrentIndex(static_cast<int>(i));
                _combo->blockSignals(wasBlocked);
                return static_cast<int>(i);
            }
        }
        return -1; // Not found
    }

    int ComboLinks::count() const
    {
        return _combo ? _combo->count() : 0;
    }

} // namespace Gui
