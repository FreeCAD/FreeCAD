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

#include <map>
#include <vector>
#include <set>
#include <memory>
#include <climits>
#include <App/Material.h>

namespace Gui {

class SoFCSelectionRoot;
struct SoFCSelectionContextBase;
typedef std::shared_ptr<SoFCSelectionContextBase> SoFCSelectionContextBasePtr;

struct GuiExport SoFCSelectionContextBase {
    virtual ~SoFCSelectionContextBase() {}
    typedef int MergeFunc(int status, SoFCSelectionContextBasePtr &output,
            SoFCSelectionContextBasePtr input, SoFCSelectionRoot *node);
};

struct SoFCSelectionContext;
typedef std::shared_ptr<SoFCSelectionContext> SoFCSelectionContextPtr;

struct GuiExport SoFCSelectionContext : SoFCSelectionContextBase
{
    int highlightIndex = -1;
    std::set<int> selectionIndex;
    SbColor selectionColor;
    SbColor highlightColor;
    std::shared_ptr<int> counter;

    virtual ~SoFCSelectionContext();

    bool isSelected() const {
        return !selectionIndex.empty();
    }

    void selectAll() {
        selectionIndex.clear();
        selectionIndex.insert(-1);
    }

    bool isSelectAll() const{
        return selectionIndex.size() && *selectionIndex.begin()<0;
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
typedef std::shared_ptr<SoFCSelectionContextEx> SoFCSelectionContextExPtr;

struct GuiExport SoFCSelectionContextEx : SoFCSelectionContext
{
    std::map<int,App::Color> colors;
    float trans0 = 0.0;

    bool setColors(const std::map<std::string,App::Color> &colors, const std::string &element);
    uint32_t packColor(const App::Color &c, bool &hasTransparency);
    bool applyColor(int idx, std::vector<uint32_t> &packedColors, bool &hasTransparency);
    bool isSingleColor(uint32_t &color, bool &hasTransparency);

    virtual SoFCSelectionContextBasePtr copy() {
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
    bool hasSelection;
    bool hasPreselection;
    static int cachingMode;
};

}
#endif //GUI_SOFCSELECTIONCONTEXT_H
