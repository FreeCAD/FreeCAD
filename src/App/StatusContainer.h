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

#ifndef APP_STATUSCONTAINER_H
#define APP_STATUSCONTAINER_H

#include "StatusCollection.h"

namespace App
{

template<typename ENUM>
class StatusContainer
{
public:
    void setStatus(const StatusCollection<ENUM> & other) {
        for (size_t bit=0;bit< StatusCollection<ENUM>::StatusBitLength;bit++) {
            if (other.test(bit)) {
                setStatus(static_cast<ENUM>(bit));
            }
        }
    }

    const StatusCollection<ENUM>& getStatus() const { return this->statusBits;}

    bool testStatus(const ENUM& pos) const {return statusBits.test(pos);}

    void setStatus(const ENUM& pos, bool on=true) {

        if (testStatus(pos)!=on) {
            statusBits.set(pos, on);
            onStatusChanged(pos, on);
        }
    }

    bool testStatusAny() const { return statusBits.any();}

protected:
    virtual void onStatusChanged(const ENUM& pos, bool newValue) =0;

private:
    StatusCollection<ENUM> statusBits;
};

};
#endif // APP_STATUSCONTAINER_H
