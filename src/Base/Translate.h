// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2018 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
 *   write to the Free Software Foundation, Inc., 51 Franklin Street,      *
 *   Fifth Floor, Boston, MA  02110-1301, USA                              *
 *                                                                         *
 ***************************************************************************/


#pragma once

#include <CXX/Extensions.hxx>
#include <QTranslator>
#include <memory>
#include <list>
#ifndef FC_GLOBAL_H
# include <FCGlobal.h>
#endif

namespace Py
{
class Object;
class Tuple;
}  // namespace Py

namespace Base
{

class BaseExport Translate: public Py::ExtensionModule<Translate>  // NOLINT
{
public:
    Translate();
    ~Translate() override;

private:
    Py::Object translate(const Py::Tuple& args);
    Py::Object translateNoop(const Py::Tuple& args);
    Py::Object translateNoop3(const Py::Tuple& args);
    Py::Object trNoop(const Py::Tuple& args);
    Py::Object installTranslator(const Py::Tuple& args);
    Py::Object removeTranslators(const Py::Tuple& args);

private:
    std::list<std::shared_ptr<QTranslator>> translators;
};

}  // namespace Base
