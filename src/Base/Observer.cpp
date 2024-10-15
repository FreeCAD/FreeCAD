/***************************************************************************
 *   Copyright (c) 2023 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License (LGPL)   *
 *   as published by the Free Software Foundation; either version 2 of     *
 *   the License, or (at your option) any later version.                   *
 *   for detail see the LICENCE text file.                                 *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful,            *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with FreeCAD; if not, write to the Free Software        *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
 *   USA                                                                   *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"

#include "Console.h"
#include "Observer.h"


namespace Base
{

template<class MsgType>
Subject<MsgType>::~Subject()
{
    if (_ObserverSet.size() > 0) {
        Base::Console().DeveloperWarning(std::string("~Subject()"),
                                            "Not detached all observers yet\n");
    }
}

template<class MsgType>
void Subject<MsgType>::Attach(Observer<MsgType>* ToObserv)
{
#ifdef FC_DEBUG
    size_t count = _ObserverSet.size();
    _ObserverSet.insert(ToObserv);
    if (_ObserverSet.size() == count) {
        Base::Console().DeveloperWarning(std::string("Subject::Attach"),
                                            "Observer %p already attached\n",
                                            static_cast<void*>(ToObserv));
    }
#else
    _ObserverSet.insert(ToObserv);
#endif
}

template<class MsgType>
void Subject<MsgType>::Notify(MsgType rcReason)
{
    for (typename std::set<Observer<MsgType>*>::iterator Iter = _ObserverSet.begin();
            Iter != _ObserverSet.end();
            ++Iter) {
        try {
            (*Iter)->OnChange(*this, rcReason);  // send OnChange-signal
        }
        catch (Base::Exception& e) {
            Base::Console().Error("Unhandled Base::Exception caught when notifying observer.\n"
                                    "The error message is: %s\n",
                                    e.what());
        }
        catch (std::exception& e) {
            Base::Console().Error("Unhandled std::exception caught when notifying observer\n"
                                    "The error message is: %s\n",
                                    e.what());
        }
        catch (...) {
            Base::Console().Error(
                "Unhandled unknown exception caught in when notifying observer.\n");
        }
    }
}

template<class MsgType>
void Subject<MsgType>::Detach(Observer<MsgType>* ToObserv)
{
#ifdef FC_DEBUG
    size_t count = _ObserverSet.size();
    _ObserverSet.erase(ToObserv);
    if (_ObserverSet.size() == count) {
        Base::Console().DeveloperWarning(std::string("Subject::Detach"),
                                            "Observer %p already detached\n",
                                            static_cast<void*>(ToObserv));
    }
#else
    _ObserverSet.erase(ToObserv);
#endif
}


#if !defined(__MINGW32__)
template class BaseExport Observer<const char*>;
template class BaseExport Subject<const char*>;
#endif

}  // namespace Base
