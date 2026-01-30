/* SPDX - License - Identifier: LGPL - 2.1 - or -later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2026 Pierre-Louis Boyer                                  *
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

#ifndef GUI_LABELDISAMBIGUATOR_H
#define GUI_LABELDISAMBIGUATOR_H

#include "FCGlobal.h"
#include <string>
#include <vector>

namespace App
{
class Document;
class DocumentObject;
}  // namespace App

namespace Gui
{

/**
 * @brief Helper class to handle visual names for objects with duplicate labels.
 *
 * This class ensures that "Bolt", "Bolt <1>", "Bolt <2>" logic is consistent
 * across the Tree View and the Expression Editor.
 */
class GuiExport LabelDisambiguator
{
public:
    /**
     * @brief Gets the visual name for an object.
     *
     * If the object's label is unique, it returns the label (e.g. "Plate").
     * If the label is duplicated and the ViewProvider requires it,
     * returns label + suffix (e.g. "Bolt <1>").
     */
    static std::string getVisualName(const App::DocumentObject* obj);

    /**
     * @brief Resolves a visual name back to the document object.
     *
     * Parses "Bolt <2>", finds all objects labeled "Bolt", sorts them
     * identically to getVisualName, and picks the 2nd one.
     * Returns nullptr if not found.
     */
    static App::DocumentObject* getObjectFromVisualName(
        const App::Document* doc,
        const std::string& visualName
    );

    /**
     * @brief Converts an internal expression string (e.g. "Link002.Shape")
     * into a visual string (e.g. "Bolt <1>.Shape").
     */
    static std::string translateInternalToVisual(const App::Document* doc, const std::string& input);

    /**
     * @brief Converts a visual expression string (e.g. "Bolt <1>.Shape")
     * into an internal string (e.g. "Link002.Shape") for parsing.
     */
    static std::string translateVisualToInternal(const App::Document* doc, const std::string& input);
    /**
     * @brief Batch computes visual names for the entire document.
     *
     * This is significantly faster than calling getVisualName() for every object
     * because it groups objects by label and processes them in one pass.
     *
     * @return A map where the key is the Object and the value is the Visual Name.
     *         Objects that do not need disambiguation are NOT in the map (implied unique).
     */
    static std::map<App::DocumentObject*, std::string> computeVisualNames(const App::Document* doc);
};

}  // namespace Gui

#endif  // GUI_LABELDISAMBIGUATOR_H
