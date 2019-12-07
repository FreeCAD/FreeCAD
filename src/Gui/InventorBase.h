/****************************************************************************
 *   Copyright (c) 2019 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
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

#ifndef GUI_INVENTOR_BASE_H
#define GUI_INVENTOR_BASE_H

#include <boost/intrusive_ptr.hpp>

class SoGroup;

// define this only if we actually decide to run coin in multiple thread
#ifdef FC_COIN_MULTI_THREAD
#   define FC_COIN_THREAD_LOCAL thread_local
#else
#   define FC_COIN_THREAD_LOCAL
#endif

namespace Gui {

/** Convenience smart pointer to wrap coin node. 
 *
 * It is basically boost::intrusive plus implicit pointer conversion to save the
 * trouble of typing get() all the time.
 */
template<class T>
class CoinPtr: public boost::intrusive_ptr<T> {
public:
    // Too bad, VC2013 does not support constructor inheritance
    //using boost::intrusive_ptr<T>::intrusive_ptr;
    typedef boost::intrusive_ptr<T> inherited;
    CoinPtr() {}
    CoinPtr(T *p, bool add_ref=true):inherited(p,add_ref){}
    template<class Y> CoinPtr(CoinPtr<Y> const &r):inherited(r){}

    operator T *() const {
        return this->get();
    }
};

/** Helper function to deal with bug in SoNode::removeAllChildren()
 *
 * @sa https://bitbucket.org/Coin3D/coin/pull-requests/119/fix-sochildlist-auditing/diff
 */
void GuiExport coinRemoveAllChildren(SoGroup *node);

}

#endif
