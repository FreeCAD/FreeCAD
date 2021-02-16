/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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


#ifndef APP_STATUSCOLLECTION_H
#define APP_STATUSCOLLECTION_H

#include <bitset>
#include <cstdlib>
#include <string>

namespace App
{

template<typename ENUM>
class StatusCollection : public std::bitset<sizeof(ENUM)*8>
{
public:
    static const size_t StatusBitLength = sizeof(ENUM)*8;

    constexpr StatusCollection() : std::bitset<StatusBitLength>() {}
    explicit StatusCollection(unsigned long long val): std::bitset<StatusBitLength>(val) {}
    StatusCollection(const char * str) : std::bitset<StatusBitLength>( strtoull(str,0,10)) {}
    StatusCollection(const StatusCollection<ENUM> &col) : std::bitset<StatusBitLength>(col) {}
    StatusCollection(ENUM status) { this->set(status);}

    StatusCollection<ENUM>& operator=(const StatusCollection<ENUM> & src) {
        std::bitset<StatusBitLength>::operator=(src);
        return *this;
    }
};

template<typename ENUM>
StatusCollection<ENUM> operator+(ENUM status1, ENUM status2) {
    StatusCollection<ENUM> collection;
    collection.set(status1);
    collection.set(status2);
    return collection;
}

template<typename ENUM>
StatusCollection<ENUM> operator+(const StatusCollection<ENUM>& c, ENUM status2) {
    StatusCollection<ENUM> collection(c);
    collection.set(status2);
    return collection;
}

}

#endif // APP_STATUSCOLLECTION_H
