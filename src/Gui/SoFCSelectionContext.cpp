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

#include "PreCompiled.h"

#ifndef _PreComp_
# include <boost/algorithm/string/predicate.hpp>
# include <Inventor/elements/SoCacheElement.h>
# include <Inventor/misc/SoState.h>
#endif

#include "SoFCSelectionContext.h"
#include "SoFCUnifiedSelection.h"


using namespace Gui;

/////////////////////////////////////////////////////////////////////////////

SoFCSelectionContext::~SoFCSelectionContext() {
    if(counter)
        *counter -= 1;
}

bool SoFCSelectionContext::checkGlobal(SoFCSelectionContextPtr ctx) {
    bool sel = false;
    bool hl = false;
    SoFCSelectionRoot::checkSelection(sel,selectionColor,hl,highlightColor);
    if(sel)
        selectionIndex.insert(-1);
    else if(ctx && hl) {
        selectionColor = ctx->selectionColor;
        selectionIndex = ctx->selectionIndex;
    }else
        selectionIndex.clear();
    if(hl)
        highlightAll();
    else if(ctx && sel) {
        highlightIndex = ctx->highlightIndex;
        highlightColor = ctx->highlightColor;
    }else
        removeHighlight();
    return sel||hl;
}

bool SoFCSelectionContext::removeIndex(int index) {
    auto it = selectionIndex.find(index);
    if(it != selectionIndex.end()) {
        selectionIndex.erase(it);
        return true;
    }
    return false;
}

int SoFCSelectionContext::merge(int status, SoFCSelectionContextBasePtr &output,
        SoFCSelectionContextBasePtr input, SoNode *)
{
    auto ctx = std::dynamic_pointer_cast<SoFCSelectionContext>(input);
    if(!ctx)
        return status;

    if(ctx->selectionIndex.empty()) {
        output = ctx;
        return -1;
    }

    auto ret = std::dynamic_pointer_cast<SoFCSelectionContext>(output);
    if(!ret) {
        output = ctx;
        return 0;
    }

    if(ctx->isSelectAll())
        return status;

    if(ret->isSelectAll()) {
        if(!status) {
            output = ret->copy();
            ret = std::dynamic_pointer_cast<SoFCSelectionContext>(ret);
            assert(ret);
        }
        ret->selectionIndex = ctx->selectionIndex;
        return status;
    }

    std::vector<int> remove;
    for(auto idx : ret->selectionIndex) {
        if(!ctx->selectionIndex.count(idx))
            remove.push_back(idx);
    }

    for(auto idx : remove) {
        if(!status) {
            status = 1;
            output = ret->copy();
            ret = std::dynamic_pointer_cast<SoFCSelectionContext>(ret);
            assert(ret);
        }
        ret->selectionIndex.erase(idx);
        if(ret->selectionIndex.empty())
            return -1;
    }
    return status;
}

/////////////////////////////////////////////////////////////////////////////////////

bool SoFCSelectionContextEx::setColors(
        const std::map<std::string,App::Color> &colors, const std::string &element) {
    std::map<int,App::Color> tmp;
    auto it = colors.find("");
    if(it!=colors.end())
        tmp[-1] = it->second;
    for(auto it=colors.lower_bound(element);it!=colors.end();++it) {
        if(!boost::starts_with(it->first,element))
            break;
        if(it->first.size()==element.size())
            tmp[-1] = it->second;
        else {
            int idx = std::atoi(it->first.c_str()+4);
            if(idx>0) {
                idx -= 1;
                tmp[idx] = it->second;
            }
        }
    }
    if(tmp == this->colors)
        return false;
    this->colors.swap(tmp);
    return true;
}

uint32_t SoFCSelectionContextEx::packColor(const App::Color &c, bool &hasTransparency) {
    float trans = std::max(trans0,c.a);
    if(trans>0)
        hasTransparency = true;
    return SbColor(c.r,c.g,c.b).getPackedValue(trans);
}

bool SoFCSelectionContextEx::applyColor(int idx, std::vector<uint32_t> &packedColors, bool &hasTransparency) {
    if(colors.empty())
        return false;
    auto it = colors.find(idx);
    if(it==colors.end()) {
        if(colors.begin()->first >= 0)
            return false;
        it = colors.begin();
    }
    packedColors.push_back(packColor(it->second,hasTransparency));
    return true;
}

bool SoFCSelectionContextEx::isSingleColor(uint32_t &color, bool &hasTransparency) {
    if(!colors.empty() && colors.begin()->first<0) {
        color = packColor(colors.begin()->second,hasTransparency);
        return colors.size()==1;
    }
    return false;
}

int SoFCSelectionContextEx::merge(int status, SoFCSelectionContextBasePtr &output,
        SoFCSelectionContextBasePtr input, SoNode *node)
{
    auto ctx = std::dynamic_pointer_cast<SoFCSelectionContextEx>(input);
    SoFCSelectionRoot* selectionNode = dynamic_cast<SoFCSelectionRoot*>(node);

    if(!ctx) {
        if(selectionNode && selectionNode->hasColorOverride()) {
            if(!status)
                status = 2;
            else if(status == 1)
                status = 3;
        }
        return status;
    }

    int status_copy = status;
    if(status==2)
        status_copy = 0;
    else if(status==3)
        status_copy = 1;
    status_copy = SoFCSelectionContext::merge(status_copy,output,input,node);
    if(status_copy < 0)
        return status_copy;

    if(status>1) {
        // When status>1 it means there is color override before us, all
        // subsequent color override will be bypassed
        if(status_copy==1)
            status = 3;
        else
            status = 2;
        return status;
    }

    status = status_copy;
    auto ret = std::dynamic_pointer_cast<SoFCSelectionContextEx>(output);
    assert(ret);
    for(auto &v : ctx->colors) {
        if(ret->colors.count(v.first))
            continue;
        if(!status) {
            status = 1;
            output = ret->copy();
            ret = std::dynamic_pointer_cast<SoFCSelectionContextEx>(output);
            assert(ret);
        }
        ret->colors.insert(v);
    }

    if(selectionNode && selectionNode->hasColorOverride()) {
        if(!status)
            status = 2;
        else if(status == 1)
            status = 3;
    }
    return status;
}

///////////////////////////////////////////////////////////////////////

SoFCSelectionCounter::SoFCSelectionCounter()
  : counter(std::make_shared<int>(0))
{}

SoFCSelectionCounter::~SoFCSelectionCounter() = default;


bool SoFCSelectionCounter::checkRenderCache(SoState *state) {
    if(*counter ||
       (hasSelection && Selection().hasSelection()) ||
       (hasPreselection && Selection().hasPreselection()))
    {
        if(SoFCSelectionRoot::getCacheMode()!=SoSeparator::OFF)
            SoCacheElement::invalidate(state);
        return false;
    }
    if(!Selection().hasPreselection())
        hasPreselection = false;
    if(!Selection().hasSelection())
        hasSelection = false;
    return true;
}

void SoFCSelectionCounter::checkAction(SoHighlightElementAction *hlaction) {
    if(hlaction->isHighlighted())
        hasPreselection = true;
}

void SoFCSelectionCounter::checkAction(SoSelectionElementAction *selaction, SoFCSelectionContextPtr ctx) {
    switch(selaction->getType()) {
    case SoSelectionElementAction::None:
        return;
    case SoSelectionElementAction::All:
    case SoSelectionElementAction::Append:
        hasSelection = true;
        break;
    default:
        break;
    }
    if(selaction->isSecondary()) {
        if(ctx && !ctx->counter) {
            *counter += 1;
            ctx->counter = counter;
        }
    }
}
