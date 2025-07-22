/***************************************************************************
 *   Copyright (c) 2016 Victor Titov (DeepSOIC) <vv.titov@gmail.com>       *
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

/**
  * AttacherTexts.h, .cpp - files that contain user-friendly translatable names
  * of attachment modes, as well as help texts, and the like.
  */

#ifndef PARTATTACHERTEXTS_H
#define PARTATTACHERTEXTS_H

#include <vector>
#include <QString>
#include <QStringList>
#include <Mod/Part/App/Attacher.h>


namespace AttacherGui {

using TextSet = std::vector<QString>;

/**
 * @brief getUIStrings
 * @param attacherType
 * @param mmode
 * @return vector of two QStrings:
 * first is the name of attachment mode. e.g. "Tangent to surface";
 * second is tooltip-style explanation of the mode, like "Plane is tangent to a surface at vertex."
 */
TextSet PartGuiExport getUIStrings(Base::Type attacherType, Attacher::eMapMode mmode);


QString PartGuiExport getShapeTypeText(Attacher::eRefType type);

QStringList PartGuiExport getRefListForMode(Attacher::AttachEngine &attacher, Attacher::eMapMode mmode);


// Python interface
class PartGuiExport AttacherGuiPy{
public:
    static PyMethodDef    Methods[];
    static PyObject* sGetModeStrings(PyObject * /*self*/, PyObject *args);
    static PyObject* sGetRefTypeUserFriendlyName(PyObject * /*self*/, PyObject *args);
};

}
#endif
