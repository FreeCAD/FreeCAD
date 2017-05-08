/***************************************************************************
 *   Copyright (c) 2017 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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


#ifndef BASE_EXCEPTIONFACTORY_H
#define BASE_EXCEPTIONFACTORY_H



#include "Factory.h"

namespace Base
{

struct ExceptionInfo {
    std::string exceptionname;
    std::string function;
    std::string message;
    std::string file;
    unsigned int line;
};

/// Abstract base class of all exception producers
class BaseExport AbstractExceptionProducer : public AbstractProducer
{
public:
    AbstractExceptionProducer () {}
    ~AbstractExceptionProducer() {}
    // just implement it
    void* Produce () const {
        return nullptr;
    }
    virtual void raiseException(const ExceptionInfo& info) const = 0;
};

// --------------------------------------------------------------------

/** The ExceptionFactory */
class BaseExport ExceptionFactory : public Factory
{
public:
    static ExceptionFactory& Instance(void);
    static void Destruct (void);
    
    void raiseException(const ExceptionInfo& info) const;
    
private:
    static ExceptionFactory* _pcSingleton;
    
    ExceptionFactory(){}
    ~ExceptionFactory(){}
};

/* Producers */

template <class CLASS>
class BaseExport ExceptionProducer : public AbstractExceptionProducer
{
public:
    ExceptionProducer ()
    {
        ExceptionFactory::Instance().AddProducer(typeid(CLASS).name(), this);
    }
    
    virtual ~ExceptionProducer (){}
    
    void raiseException(const ExceptionInfo& info) const
    {
        CLASS c;
        
        c.setMessage(info.message);

        c.setDebugInformation(info.file, info.line, info.function);

        throw c;
    }
};

} //namespace Base


#endif

