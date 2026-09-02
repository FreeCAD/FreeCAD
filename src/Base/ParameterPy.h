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

#include <list>

#include "Parameter.h"
#include "Interpreter.h"


namespace Base
{
class ParameterGrpObserver;

using ParameterGrpObserverList = std::list<ParameterGrpObserver*>;

class BaseExport ParameterGrpPy: public Py::PythonExtension<ParameterGrpPy>  // NOLINT
{
public:
    static void init_type();  // announce properties and methods
    static PyTypeObject* type_object();

    explicit ParameterGrpPy(const Base::Reference<ParameterGrp>& rcParamGrp);
    ~ParameterGrpPy() override;

    Py::Object repr() override;

    // NOLINTBEGIN
    Py::Object getGroup(const Py::Tuple&);
    Py::Object getGroupName(const Py::Tuple&);
    Py::Object getGroups(const Py::Tuple&);
    Py::Object remGroup(const Py::Tuple&);
    Py::Object hasGroup(const Py::Tuple&);
    Py::Object renameGroup(const Py::Tuple&);
    Py::Object copyTo(const Py::Tuple&);

    Py::Object getManager(const Py::Tuple&);
    Py::Object getParent(const Py::Tuple&);

    Py::Object isEmpty(const Py::Tuple&);
    Py::Object clear(const Py::Tuple&);

    Py::Object attach(const Py::Tuple&);
    Py::Object attachManager(const Py::Tuple& args);
    Py::Object detach(const Py::Tuple&);
    Py::Object notify(const Py::Tuple&);
    Py::Object notifyAll(const Py::Tuple&);

    Py::Object setBool(const Py::Tuple&);
    Py::Object getBool(const Py::Tuple&);
    Py::Object getBools(const Py::Tuple&);
    Py::Object remBool(const Py::Tuple&);

    Py::Object setInt(const Py::Tuple&);
    Py::Object getInt(const Py::Tuple&);
    Py::Object getInts(const Py::Tuple&);
    Py::Object remInt(const Py::Tuple&);

    Py::Object setUnsigned(const Py::Tuple&);
    Py::Object getUnsigned(const Py::Tuple&);
    Py::Object getUnsigneds(const Py::Tuple&);
    Py::Object remUnsigned(const Py::Tuple&);

    Py::Object setFloat(const Py::Tuple&);
    Py::Object getFloat(const Py::Tuple&);
    Py::Object getFloats(const Py::Tuple&);
    Py::Object remFloat(const Py::Tuple&);

    Py::Object setString(const Py::Tuple&);
    Py::Object getString(const Py::Tuple&);
    Py::Object getStrings(const Py::Tuple&);
    Py::Object remString(const Py::Tuple&);

    Py::Object importFrom(const Py::Tuple&);
    Py::Object insert(const Py::Tuple&);
    Py::Object exportTo(const Py::Tuple&);

    Py::Object getContents(const Py::Tuple&);
    // NOLINTEND

private:
    void tryCall(
        ParameterGrpObserver* obs,
        ParameterGrp* Param,
        ParameterGrp::ParamType Type,
        const char* Name,
        const char* Value
    );

private:
    ParameterGrp::handle _cParamGrp;
    ParameterGrpObserverList _observers;
};

}  // namespace Base
