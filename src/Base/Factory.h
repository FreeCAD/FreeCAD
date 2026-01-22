// SPDX-License-Identifier: LGPL-2.1-or-later

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


#pragma once

#include <list>
#include <map>
#include <string>
#ifndef FC_GLOBAL_H
# include <FCGlobal.h>
#endif


namespace Base
{

/// Abstract base class of all producers
class BaseExport AbstractProducer
{
public:
    AbstractProducer() = default;
    virtual ~AbstractProducer() = default;
    /// overwritten by a concrete producer to produce the needed object
    virtual void* Produce() const = 0;

    FC_DISABLE_COPY_MOVE(AbstractProducer)
};


/** Base class of all factories
 * This class has the purpose to produce instances of classes at runtime
 * that are unknown at compile time. It holds a map of so called
 * producers which are able to produce an instance of a special class.
 * Producer can be registered at runtime through e.g. application modules
 */
class BaseExport Factory
{
public:
    /// Adds a new prducer instance
    void AddProducer(const char* sClassName, AbstractProducer* pcProducer);
    /// returns true if there is a producer for this class registered
    bool CanProduce(const char* sClassName) const;
    /// returns a list of all registered producer
    std::list<std::string> CanProduce() const;
    /// destruction
    virtual ~Factory();

    FC_DISABLE_COPY_MOVE(Factory)

protected:
    /// produce a class with the given name
    void* Produce(const char* sClassName) const;
    /// construction
    Factory() = default;

    std::map<const std::string, AbstractProducer*> _mpcProducers;
};

// --------------------------------------------------------------------

/** The ScriptFactorySingleton singleton
 */
class BaseExport ScriptFactorySingleton: public Factory  // NOLINT
{
public:
    static ScriptFactorySingleton& Instance();
    static void Destruct();

    const char* ProduceScript(const char* sScriptName) const;

    FC_DISABLE_COPY_MOVE(ScriptFactorySingleton)

private:
    static ScriptFactorySingleton* _pcSingleton;  // NOLINT

protected:
    ScriptFactorySingleton() = default;
    ~ScriptFactorySingleton() override = default;
};

inline ScriptFactorySingleton& ScriptFactory()
{
    return ScriptFactorySingleton::Instance();
}

// --------------------------------------------------------------------

/** Script Factory
 * This class produce Scripts.
 * @see Factory
 */
class BaseExport ScriptProducer: public AbstractProducer
{
public:
    /// Constructor
    ScriptProducer(const char* name, const char* script)
        : mScript(script)
    {
        ScriptFactorySingleton::Instance().AddProducer(name, this);
    }

    ~ScriptProducer() override = default;

    /// Produce an instance
    void* Produce() const override
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
        return const_cast<char*>(mScript);
    }

    FC_DISABLE_COPY_MOVE(ScriptProducer)

private:
    const char* mScript;
};

}  // namespace Base
