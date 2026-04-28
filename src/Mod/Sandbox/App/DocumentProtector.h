// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer@users.sourceforge.net>        *
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

#include <string>
#include <App/DocumentObserver.h>

namespace App {
    class Document;
    class DocumentObject;
}

namespace Sandbox {

class SandboxAppExport DocumentProtector : public App::DocumentObserver
{
public:
    DocumentProtector(App::Document*);
    ~DocumentProtector();

    static void init();

    App::DocumentObject *addObject(const std::string& type, const std::string& name="");
    void removeObject(const std::string& name);
    void recompute();

private:
    /** Checks if the given document is about to be closed */
    void slotDeletedDocument(const App::Document& Doc);
    void validate();
};

class AbstractCallable
{
public:
    AbstractCallable()
    {
    }
    virtual ~AbstractCallable()
    {
    }

    virtual void operator()() const = 0;
};

template <class T, void (T::*method)()>
class Callable : public AbstractCallable
{
public:
    Callable(App::DocumentObject* o) : obj(o)
    {
    }
    virtual ~Callable()
    {
    }

    virtual void operator()() const
    {
        T* v = static_cast<T*>(obj);
        (v->*method)();
    }

private:
    App::DocumentObject* obj;
};

template <class T, class Arg, void (T::*method)(Arg)>
class CallableWithArgs : public AbstractCallable
{
public:
    CallableWithArgs(App::DocumentObject* o, Arg a) : obj(o), arg(a)
    {
    }
    virtual ~CallableWithArgs()
    {
    }

    virtual void operator()() const
    {
        T* v = static_cast<T*>(obj);
        (v->*method)(arg);
    }

private:
    App::DocumentObject* obj;
    Arg arg;
};

class SandboxAppExport DocumentObjectProtector
{
public:
    DocumentObjectProtector(App::DocumentObject*);
    ~DocumentObjectProtector();

    App::DocumentObject* getObject() const;
    bool setProperty(const std::string& name, const App::Property& p);
    void execute(const AbstractCallable&);
    void purgeTouched();

private:
    void validate();

private:
    App::DocumentObject* obj;
};

}