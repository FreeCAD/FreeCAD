// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2018 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include <algorithm>
#include <future>


namespace MeshCore
{
template<class Iter, class Pred>
static void parallel_sort(Iter begin, Iter end, Pred comp, int threads)
{
    if (threads < 2 || end - begin < 2) {
        std::sort(begin, end, comp);
    }
    else {
        Iter mid = begin + (end - begin) / 2;
        if (threads == 2) {
            auto future = std::async(parallel_sort<Iter, Pred>, begin, mid, comp, threads / 2);
            std::sort(mid, end, comp);
            future.wait();
        }
        else {
            auto a = std::async(
                std::launch::async,
                parallel_sort<Iter, Pred>,
                begin,
                mid,
                comp,
                threads / 2
            );
            auto b
                = std::async(std::launch::async, parallel_sort<Iter, Pred>, mid, end, comp, threads / 2);
            a.wait();
            b.wait();
        }
        std::inplace_merge(begin, mid, end, comp);
    }
}

}  // namespace MeshCore
