/****************************************************************************
 *   Copyright (c) 2020 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
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
# include <unordered_set>
#endif

//#include <boost/functional/hash.hpp>

#include "MappedName.h"

using namespace Data;


void MappedName::compact()
{

    if (this->raw) {
        this->data = QByteArray(this->data.constData(), this->data.size());
        this->raw = false;
    }

#if 0
    static std::unordered_set<QByteArray, ByteArrayHasher> PostfixSet;
    if (this->postfix.size()) {
        auto res = PostfixSet.insert(this->postfix);
        if (!res.second)
            self->postfix = *res.first;
    }
#endif
}

