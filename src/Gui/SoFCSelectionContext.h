/****************************************************************************
 *   Copyright (c) 2018 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#ifndef GUI_SOFCSELECTIONCONTEXT_H
#define GUI_SOFCSELECTIONCONTEXT_H

#include <climits>
#include <map>
#include <memory>
#include <set>
#include <vector>
#include <Inventor/SbColor.h>

#include <App/Material.h>

class SoState;

namespace Gui {

class SoFCSelectionRoot;
struct SoFCSelectionContextBase;
using SoFCSelectionContextBasePtr = std::shared_ptr<SoFCSelectionContextBase>;

struct GuiExport SoFCSelectionContextBase {
    virtual ~SoFCSelectionContextBase() = default;
    using MergeFunc = int (int status,
                           SoFCSelectionContextBasePtr &output,
                           SoFCSelectionContextBasePtr input,
                           SoNode *node);
};

struct SoFCSelectionContext;
using SoFCSelectionContextPtr = std::shared_ptr<SoFCSelectionContext>;

struct GuiExport SoFCSelectionContext : SoFCSelectionContextBase
{
    int highlightIndex = -1;
    std::set<int> selectionIndex;
    SbColor selectionColor;
    SbColor highlightColor;
    std::shared_ptr<int> counter;

    ~SoFCSelectionContext() override;

    bool isSelected() const {
        return !selectionIndex.empty();
    }

    void selectAll() {
        selectionIndex.clear();
        selectionIndex.insert(-1);
    }

    bool isSelectAll() const{
        return !selectionIndex.empty() && *selectionIndex.begin()<0;
    }

    bool isHighlighted() const {
        return highlightIndex>=0;
    }

    bool isHighlightAll() const{
        return highlightIndex==INT_MAX && (selectionIndex.empty() || isSelectAll());
    }

    void highlightAll() {
        highlightIndex = INT_MAX;
    }

    void removeHighlight() {
        highlightIndex = -1;
    }

    bool removeIndex(int index);
    bool checkGlobal(SoFCSelectionContextPtr ctx);

    virtual SoFCSelectionContextBasePtr copy() {
        return std::make_shared<SoFCSelectionContext>(*this);
    }

    static MergeFunc merge;
};

struct SoFCSelectionContextEx;
using SoFCSelectionContextExPtr = std::shared_ptr<SoFCSelectionContextEx>;

struct GuiExport SoFCSelectionContextEx : SoFCSelectionContext
{
    std::map<int,App::Color> colors;
    float trans0 = 0.0;

    bool setColors(const std::map<std::string,App::Color> &colors, const std::string &element);
    uint32_t packColor(const App::Color &c, bool &hasTransparency);
    bool applyColor(int idx, std::vector<uint32_t> &packedColors, bool &hasTransparency);
    bool isSingleColor(uint32_t &color, bool &hasTransparency);

    SoFCSelectionContextBasePtr copy() override {
        return std::make_shared<SoFCSelectionContextEx>(*this);
    }

    static MergeFunc merge;
};

class SoHighlightElementAction;
class SoSelectionElementAction;

class GuiExport SoFCSelectionCounter {
public:
    SoFCSelectionCounter();
    virtual ~SoFCSelectionCounter();
    bool checkRenderCache(SoState *state);
    void checkAction(SoHighlightElementAction *hlaction);
    void checkAction(SoSelectionElementAction *selaction, SoFCSelectionContextPtr ctx);
protected:
    std::shared_ptr<int> counter;
    bool hasSelection{false};
    bool hasPreselection{false};
    static int cachingMode;
};

}
#endif //GUI_SOFCSELECTIONCONTEXT_H
