/***************************************************************************
 *   (c) Jürgen Riegel (juergen.riegel@web.de) 2002                        *   
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
 *   Juergen Riegel 2002                                                   *
 ***************************************************************************/


#ifndef BASE_FACTORY_H
#define BASE_FACTORY_H

#include<typeinfo>
#include<string>
#include<map>
#include<list>
#include"../FCConfig.h"


namespace Base
{

/// Abstract base class of all producers
class BaseExport AbstractProducer
{
public:
    AbstractProducer() {}
    virtual ~AbstractProducer() {}
    /// overwritten by a concrete producer to produce the needed object
    virtual void* Produce (void) const = 0;
};


 
/** Base class of all factories
  * This class has the purpose to produce at runtime instances 
  * of classes not known at compile time. It holds a map of so called
  * producers which are able to produce an instance of a special class.
  * Producer can be registered at runtime through e.g. application modules
  */
class BaseExport Factory
{
public:
    /// Adds a new prducer instance
    void AddProducer (const char* sClassName, AbstractProducer *pcProducer);
    /// returns true if there is a producer for this class registered
    bool CanProduce(const char* sClassName) const;
    /// returns a list of all registered producer
    std::list<std::string> CanProduce() const;

protected:
    /// produce a class with the given name
    void* Produce (const char* sClassName) const;
    std::map<const std::string, AbstractProducer*> _mpcProducers;
    /// construction
    Factory (void){}
    /// destruction 
    virtual ~Factory ();
};

// --------------------------------------------------------------------

/** The ScriptFactorySingleton singleton
  */
class BaseExport ScriptFactorySingleton : public Factory
{
public:
    static ScriptFactorySingleton& Instance(void);
    static void Destruct (void);

    const char* ProduceScript (const char* sScriptName) const;

private:
    static ScriptFactorySingleton* _pcSingleton;

    ScriptFactorySingleton(){}
    ~ScriptFactorySingleton(){}
};

inline ScriptFactorySingleton& ScriptFactory(void)
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
    ScriptProducer (const char* name, const char* script) : mScript(script)
    {
        ScriptFactorySingleton::Instance().AddProducer(name, this);
    }

    virtual ~ScriptProducer (void){}

    /// Produce an instance
    virtual void* Produce (void) const
    { 
        return (void*)mScript;
    }

private:
    const char* mScript;
};

} //namespace Base


#endif

