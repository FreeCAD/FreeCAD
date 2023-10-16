/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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


#ifndef BASE_OBSERVER_H
#define BASE_OBSERVER_H

#include <cassert>
#include <cstring>
#include <set>
#include "Console.h"
#include "Exception.h"


namespace Base
{

template <class MessageType> class Subject;


/** Observer class
 *  Implementation of the well known Observer Design Pattern.
 *  The observed object, which inherit FCSubject, will call all
 *  its observers in case of changes. A observer class has to
 *  Attach itself to the observed object.
 *  @see FCSubject
 */
template <class _MessageType>
class Observer
{
public:

  /**
   * A constructor.
   * No special function so far.
   */
  Observer() = default;

  /**
   * A destructor.
   * No special function so far.
   */
  virtual ~Observer() = default;

  /**
   * This method need to be reimplemented from the concrete Observer
   * and get called by the observed class
   * @param rCaller a reference to the calling object
   * @param rcReason
   * \todo undocumented parameter 2
   */
  virtual void OnChange(Subject<_MessageType>& rCaller, _MessageType rcReason) = 0;

  /**
   * This method need to be reimplemented from the concrete Observer
   * and get called by the observed class
   * @param rCaller a reference to the calling object
   */
  virtual void OnDestroy(Subject<_MessageType>& rCaller) {
    (void)rCaller;
  }

  /**
   * This method can be reimplemented from the concrete Observer
   * and returns the name of the observer. Needed to use the Get
   * Method of the Subject.
   */
  virtual const char *Name() {
      return nullptr;
  }
};

/** Subject class
 *  Implementation of the well known Observer Design Pattern.
 *  The observed object, which inherit FCSubject, will call all
 *  its observers in case of changes. A observer class has to
 *  Attach itself to the observed object.
 *  @see FCObserver
 */
template <class _MessageType>
class Subject
{
public:

  using ObserverType = Observer<_MessageType>;
  using MessageType  = _MessageType;
  using SubjectType  = Subject<_MessageType>;

  /**
   * A constructor.
   * No special function so far.
   */
  Subject() = default;

  /**
   * A destructor.
   * No special function so far.
   */
  virtual ~Subject()
  {
    if (_ObserverSet.size() > 0)
    {
      Base::Console().DeveloperWarning(std::string("~Subject()"),
                                       "Not detached all observers yet\n");
    }
  }

  /** Attach an Observer
   * Attach an Observer to the list of Observers which get
   * called when Notify is called.
   * @param ToObserv A pointer to a concrete Observer
   * @see Notify
   */
  void Attach(Observer<_MessageType> *ToObserv)
  {
#ifdef FC_DEBUG
    size_t count = _ObserverSet.size();
    _ObserverSet.insert(ToObserv);
    if ( _ObserverSet.size() == count ) {
        Base::Console().DeveloperWarning(std::string("Subject::Attach"),
                                         "Observer %p already attached\n",
                                         static_cast<void*>(ToObserv));
    }
#else
    _ObserverSet.insert(ToObserv);
#endif
  }

  /** Detach an Observer
   * Detach an Observer from the list of Observers which get
   * called when Notify is called.
   * @param ToObserv A pointer to a concrete Observer
   * @see Notify
   */
  void Detach(Observer<_MessageType> *ToObserv)
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

  /** Notify all Observers
   * Send a message to all Observers attached to this subject.
   * The Message depends on the implementation of a concrete
   * Oberserver and Subject.
   * @see Notify
   */
  void Notify(_MessageType rcReason)
  {
    for(typename std::set<Observer<_MessageType> * >::iterator Iter=_ObserverSet.begin();Iter!=_ObserverSet.end();++Iter)
    {
      try {
        (*Iter)->OnChange(*this,rcReason);   // send OnChange-signal
      } catch (Base::Exception &e) {
        Base::Console().Error("Unhandled Base::Exception caught when notifying observer.\n"
                              "The error message is: %s\n", e.what());
      } catch (std::exception &e) {
        Base::Console().Error("Unhandled std::exception caught when notifying observer\n"
                              "The error message is: %s\n", e.what());
      } catch (...) {
        Base::Console().Error("Unhandled unknown exception caught in when notifying observer.\n");
      }
    }
  }

  /** Get an Observer by name
   * Get a observer by name if the observer reimplements the Name() mthode.
   * @see Observer
   */
  Observer<_MessageType> * Get(const char *Name)
  {
    const char* OName = nullptr;
    for(typename std::set<Observer<_MessageType> * >::iterator Iter=_ObserverSet.begin();Iter!=_ObserverSet.end();++Iter)
    {
      OName = (*Iter)->Name();   // get the name
      if(OName && strcmp(OName,Name) == 0)
        return *Iter;
    }

    return nullptr;
  }

  /** Clears the list of all registered observers.
   * @note Using this function in your code may be an indication of design problems.
   */
  void ClearObserver()
  {
    _ObserverSet.clear();
  }


protected:
  /// Vector of attached observers
  std::set<Observer <_MessageType> *> _ObserverSet;
};

// Workaround for MSVC
#if defined (FreeCADBase_EXPORTS) && defined(_MSC_VER)
#  define Base_EXPORT
#else
#  define Base_EXPORT  BaseExport
#endif

extern template class Base_EXPORT Observer<const char*>;
extern template class Base_EXPORT Subject<const char*>;


} //namespace Base


#endif // BASE_OBSERVER_H
