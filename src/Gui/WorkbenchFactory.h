/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <list>
#include <string>
#include <Base/Factory.h>

namespace Gui
{
class Workbench;

/**
 * The workbench factory provides methods for the dynamic creation of
 * special workbenches for each module. To create these workbenches once
 * they must be registered through a subclass of WorkbenchProducer.
 * @note To create workbenches you should use the API of WorkbenchManager.
 * @author Werner Mayer
 */
class GuiExport WorkbenchFactoryInst: public Base::Factory
{
public:
    /// The one and only instance.
    static WorkbenchFactoryInst& instance();
    /// Destructs the sole instance.
    static void destruct();

    /** Creates the workbench with \a name. If no such workbench is registered
     * 0 is returned.
     */
    Workbench* createWorkbench(const char* sName) const;
    /** Returns a list of all registered workbench classes. */
    std::list<std::string> workbenches() const;

private:
    static WorkbenchFactoryInst* _pcSingleton;

    WorkbenchFactoryInst() = default;
    ~WorkbenchFactoryInst() override = default;
};

inline GuiExport WorkbenchFactoryInst& WorkbenchFactory()
{
    return WorkbenchFactoryInst::instance();
}

// --------------------------------------------------------------------

/**
 * The WorkbenchProducer template class allows the registration and the creation of workbench
 * classes.
 * @author Werner Mayer
 */
template<class CLASS>
class WorkbenchProducer: public Base::AbstractProducer
{
public:
    WorkbenchProducer() = default;

    ~WorkbenchProducer() override = default;

    void* Produce() const override
    {
        return (new CLASS);
    }
};

}  // namespace Gui
