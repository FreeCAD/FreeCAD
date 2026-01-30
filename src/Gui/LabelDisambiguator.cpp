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
#include "LabelDisambiguator.h"

#include <App/Document.h>
#include <App/Origin.h>
#include <App/DocumentObject.h>
#include <App/ExpressionParser.h>
#include <Gui/Application.h>
#include <Gui/ViewProviderDocumentObject.h>
#include <QRegularExpression>
#include <algorithm>
#include <map>

using namespace Gui;

static bool compareObjectsForNumbering(App::DocumentObject* a, App::DocumentObject* b)
{
    auto vpA = dynamic_cast<Gui::ViewProviderDocumentObject*>(
        Gui::Application::Instance->getViewProvider(a)
    );
    auto vpB = dynamic_cast<Gui::ViewProviderDocumentObject*>(
        Gui::Application::Instance->getViewProvider(b)
    );

    // If one lacks a VP (e.g. internal object or not loaded), fall back to name
    if (!vpA || !vpB) {
        return std::string(a->getNameInDocument()) < std::string(b->getNameInDocument());
    }

    // Primary sort: Tree Rank (Visual position)
    if (vpA->getTreeRank() != vpB->getTreeRank()) {
        return vpA->getTreeRank() < vpB->getTreeRank();
    }
    // Secondary sort: Internal Name (Deterministic fallback)
    return std::string(a->getNameInDocument()) < std::string(b->getNameInDocument());
}

// Helper to get the list of relevant objects for a specific label, sorted correctly
static std::vector<App::DocumentObject*> getSortedCandidates(
    const App::Document* doc,
    const std::string& label
)
{
    std::vector<App::DocumentObject*> objectsToNumber;
    std::vector<App::DocumentObject*> objectsToKeepAsIs;

    // 1. Find all objects with this label
    std::vector<App::DocumentObject*> allObjs = doc->getObjects();

    // Filter by label match
    for (auto* obj : allObjs) {
        if (obj->Label.getValue() == label) {
            auto vp = Gui::Application::Instance->getViewProvider(obj);
            // We use the dynamic cast check similar to the PR logic or the new virtual method
            auto vpDoc = dynamic_cast<Gui::ViewProviderDocumentObject*>(vp);

            if (vpDoc && vpDoc->showIndentationSuffixInLabel()) {
                objectsToNumber.push_back(obj);
            }
            else {
                objectsToKeepAsIs.push_back(obj);
            }
        }
    }

    // 2. Determine if disambiguation is actually needed
    if (objectsToNumber.empty()) {
        return {};
    }

    // Ambiguity exists if there is more than one numbering candidate,
    // OR if there is a numbering candidate that conflicts with a "KeepAsIs" object.
    if (objectsToNumber.size() <= 1 && objectsToKeepAsIs.empty()) {
        return {};
    }

    // 3. Sort to ensure deterministic numbering <1>, <2>...
    std::sort(objectsToNumber.begin(), objectsToNumber.end(), compareObjectsForNumbering);

    return objectsToNumber;
}

std::string LabelDisambiguator::getVisualName(const App::DocumentObject* obj)
{
    if (!obj) {
        return "";
    }

    // Special handling for Origin: "Label (ParentVisualName)"
    if (obj->isDerivedFrom<App::Origin>()) {
        if (App::DocumentObject* parent = obj->getFirstParent()) {
            std::string parentName = getVisualName(parent);
            return obj->Label.getValue() + std::string(" (") + parentName + ")";
        }
    }
    else if (obj->is<App::Line>() || obj->is<App::Plane>() || obj->is<App::Point>()) {
        // App::Datum only, not Part::Datums!
        auto* datum = static_cast<const App::DatumElement*>(obj);
        auto lcs = datum->getLCS();
        if (lcs->isOrigin()) {
            if (App::DocumentObject* parent = lcs->getFirstParent()) {
                std::string parentName = getVisualName(parent);
                return obj->Label.getValue() + std::string(" (") + parentName + ")";
            }
        }
        else {
            return obj->Label.getValue() + std::string(" (") + lcs->Label.getValue() + ")";
        }
    }

    std::string label = obj->Label.getValue();
    const App::Document* doc = obj->getDocument();

    if (!doc) {
        return label;
    }

    // Get the sorted list of objects that share this label and require numbering
    std::vector<App::DocumentObject*> candidates = getSortedCandidates(doc, label);

    // Find where 'obj' sits in this list
    auto it = std::find(candidates.begin(), candidates.end(), obj);

    if (it != candidates.end()) {
        // It is in the list, so it gets a suffix based on index + 1
        int index = std::distance(candidates.begin(), it) + 1;
        return label + " <" + std::to_string(index) + ">";
    }

    // Not in the candidate list (either unique, or doesn't support suffixing)
    return label;
}

App::DocumentObject* LabelDisambiguator::getObjectFromVisualName(
    const App::Document* doc,
    const std::string& visualName
)
{
    if (!doc) {
        return nullptr;
    }

    // 1. Handle Origin objects (Pattern: "Label (Parent)")
    // If the string ends with ')', it might be an Origin reference.
    if (visualName.back() == ')') {
        // Iterate all Origin objects in the document.
        // This handles nested parentheses and localized labels perfectly.
        std::vector<App::DocumentObject*> objs = doc->getObjects();
        for (auto* obj : objs) {
            if (obj->isDerivedFrom<App::Origin>()) {
                // If this object generates the same visual string, we found it.
                if (getVisualName(obj) == visualName) {
                    return obj;
                }
            }
        }
    }

    // Check if the string matches "Label <N>" pattern
    // We look for " <" followed by digits followed by ">" at the end of string
    static QRegularExpression re(QStringLiteral("^(.*) <(\\d+)>$"));
    QRegularExpressionMatch match = re.match(QString::fromUtf8(visualName.c_str()));

    if (match.hasMatch()) {
        std::string baseLabel = match.captured(1).toUtf8().constData();
        int index = match.captured(2).toInt();

        if (index > 0) {
            // Re-run the sorting logic to find which object is <N>
            std::vector<App::DocumentObject*> candidates = getSortedCandidates(doc, baseLabel);

            // Indices are 1-based in visual name, 0-based in vector
            if (static_cast<size_t>(index) <= candidates.size()) {
                return candidates[index - 1];
            }
        }
    }

    // Fallback: If it doesn't match the pattern, or index is out of bounds,
    // we assume it is a standard unique Label or Name.
    // However, the requirement is to map Visual Name -> Object.
    // If the user typed "Bolt" but only "Bolt <1>" exists, this function returns nullptr
    // because "Bolt" is technically ambiguous or incorrect in this context.

    // But we should check if there is an exact label match that is NOT numbered
    auto allObjs = doc->getObjects();
    for (auto* obj : allObjs) {
        if (obj->Label.getValue() == visualName) {
            // Check if this specific object WOULD have been renamed
            // If getVisualName(obj) == visualName, then it's a match.
            // If getVisualName(obj) == visualName + " <1>", then this is not the object.
            if (getVisualName(obj) == visualName) {
                return obj;
            }
        }
    }

    return nullptr;
}

std::string LabelDisambiguator::translateInternalToVisual(
    const App::Document* doc,
    const std::string& input
)
{
    if (!doc || input.empty()) {
        return input;
    }

    auto tokens = App::ExpressionParser::tokenize(input);
    if (tokens.empty()) {
        return input;
    }

    std::string result;
    size_t lastPos = 0;

    for (const auto& token : tokens) {
        int id = std::get<0>(token);
        int start = std::get<1>(token);
        std::string text = std::get<2>(token);

        if (start > (int)lastPos) {
            result += input.substr(lastPos, start - lastPos);
        }

        if (id == App::ExpressionParser::IDENTIFIER) {
            App::DocumentObject* obj = doc->getObject(text.c_str());
            if (obj) {
                text = getVisualName(obj);
            }
        }

        result += text;
        lastPos = start + std::get<2>(token).length();
    }

    if (lastPos < input.length()) {
        result += input.substr(lastPos);
    }

    return result;
}

// Helper: Checks if a char is a letter, number, underscore, or high-bit (UTF-8 part).
static bool isIdentifierChar(char c)
{
    // 1. Alphanumeric (ASCII) + Underscore
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_') {
        return true;
    }
    // 2. High-bit set means multi-byte UTF-8 character (e.g. é, ß).
    if (static_cast<signed char>(c) < 0) {
        return true;
    }
    return false;
}

std::string LabelDisambiguator::translateVisualToInternal(
    const App::Document* doc,
    const std::string& input
)
{
    if (!doc || input.empty()) {
        return input;
    }

    std::vector<std::pair<std::string, std::string>> replacements;
    std::vector<App::DocumentObject*> objs = doc->getObjects();

    for (App::DocumentObject* obj : objs) {
        std::string visual = getVisualName(obj);
        std::string internal = obj->getNameInDocument();
        // Only replace if they differ
        if (visual != internal) {
            replacements.push_back({visual, internal});
        }
    }

    // Sort by length descending.
    // This is the key logic that handles "Box <1>.Length" before "Box <1>"
    std::sort(
        replacements.begin(),
        replacements.end(),
        [](const std::pair<std::string, std::string>& a, const std::pair<std::string, std::string>& b
        ) { return a.first.length() > b.first.length(); }
    );

    std::string result = input;
    for (const auto& pair : replacements) {
        const std::string& visual = pair.first;
        const std::string& internal = pair.second;

        if (visual.empty()) {
            continue;
        }

        size_t pos = 0;
        while ((pos = result.find(visual, pos)) != std::string::npos) {

            // If the match is inside << >>, it is a Label Reference.
            // We must preserve it as-is and NOT replace it with the Internal Name.
            if (pos >= 2 && result.compare(pos - 2, 2, "<<") == 0) {
                if (pos + visual.length() + 2 <= result.length()
                    && result.compare(pos + visual.length(), 2, ">>") == 0) {

                    // Skip this occurrence.
                    // Advance pos past this block to avoid re-finding it.
                    pos += visual.length();
                    continue;
                }
            }



            bool boundaryError = false;

            // 1. Check Preceding Character
            // Prevent replacing "Box" inside "GearBox"
            if (pos > 0) {
                char prevChar = result[pos - 1];
                if (isIdentifierChar(prevChar)) {
                    boundaryError = true;
                }
            }

            // 2. Check Succeeding Character
            // Prevent replacing "Box" inside "BoxWidth"
            if (!boundaryError && (pos + visual.length() < result.length())) {
                char nextChar = result[pos + visual.length()];
                if (isIdentifierChar(nextChar)) {
                    boundaryError = true;
                }
            }

            if (!boundaryError) {
                result.replace(pos, visual.length(), internal);
                pos += internal.length();
            }
            else {
                pos += 1;
            }
        }
    }

    return result;
}

std::map<App::DocumentObject*, std::string> LabelDisambiguator::computeVisualNames(
    const App::Document* doc
)
{
    std::map<App::DocumentObject*, std::string> results;
    if (!doc) {
        return results;
    }

    // 1. Group all objects by their raw Label
    std::map<std::string, std::vector<App::DocumentObject*>> groups;
    std::vector<App::DocumentObject*> allObjs = doc->getObjects();

    for (auto* obj : allObjs) {
        groups[obj->Label.getValue()].push_back(obj);
    }

    // 2. Process groups
    for (auto& entry : groups) {
        const std::string& label = entry.first;
        std::vector<App::DocumentObject*>& candidates = entry.second;

        // Optimization: Unique labels need no processing
        if (candidates.size() < 2) {
            continue;
        }

        std::vector<App::DocumentObject*> objectsToNumber;
        bool hasKeepAsIs = false;

        // Filter based on ViewProvider capability
        for (auto* obj : candidates) {
            auto vp = Gui::Application::Instance->getViewProvider(obj);
            auto vpDoc = dynamic_cast<Gui::ViewProviderDocumentObject*>(vp);

            if (vpDoc && vpDoc->showIndentationSuffixInLabel()) {
                objectsToNumber.push_back(obj);
            }
            else {
                hasKeepAsIs = true;
            }
        }

        // If no conflict exists (e.g. 1 numbering candidate and 0 others), skip
        if (objectsToNumber.empty()) {
            continue;
        }
        if (objectsToNumber.size() == 1 && !hasKeepAsIs) {
            continue;
        }

        // Sort the objects that need numbering
        std::sort(objectsToNumber.begin(), objectsToNumber.end(), compareObjectsForNumbering);

        // Assign suffixes
        int index = 1;
        for (auto* obj : objectsToNumber) {
            results[obj] = label + " <" + std::to_string(index++) + ">";
        }
    }

    return results;
}
