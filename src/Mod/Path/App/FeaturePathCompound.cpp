/***************************************************************************
 *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
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

#include "PreCompiled.h"

#include "FeaturePathCompound.h"
#include "Command.h"
#include "FeaturePathCompoundPy.h"


using namespace Path;
using namespace App;

PROPERTY_SOURCE(Path::FeatureCompound, Path::Feature)


FeatureCompound::FeatureCompound()
{
    ADD_PROPERTY_TYPE( Group,         (nullptr),   "Base",Prop_None,"Ordered list of paths to combine");
    ADD_PROPERTY_TYPE( UsePlacements, (false), "Base",Prop_None,"Specifies if the placements of children must be computed");
}

FeatureCompound::~FeatureCompound()
{
}

App::DocumentObjectExecReturn *FeatureCompound::execute()
{
    const std::vector<DocumentObject*> &Paths = Group.getValues();
    Path::Toolpath result;

    for (std::vector<DocumentObject*>::const_iterator it= Paths.begin();it!=Paths.end();++it) {
        if ((*it)->getTypeId().isDerivedFrom(Path::Feature::getClassTypeId())){
            const std::vector<Command*> &cmds = static_cast<Path::Feature*>(*it)->Path.getValue().getCommands();
            const Base::Placement pl = static_cast<Path::Feature*>(*it)->Placement.getValue();
            for (std::vector<Command*>::const_iterator it2= cmds.begin();it2!=cmds.end();++it2) {
                if (UsePlacements.getValue()) {
                    result.addCommand((*it2)->transform(pl));
                } else {
                    result.addCommand(**it2);
                }
            }
        } else {
            return new App::DocumentObjectExecReturn("Not all objects in group are paths!");
        }
    }

    result.setCenter(Path.getValue().getCenter());
    Path.setValue(result);

    return App::DocumentObject::StdReturn;
}

bool FeatureCompound::hasObject(const DocumentObject* obj) const
{
    const std::vector<DocumentObject*>& grp = Group.getValues();
    for (std::vector<DocumentObject*>::const_iterator it = grp.begin(); it != grp.end(); ++it) {
        if (*it == obj)
            return true;
    }

    return false;
}

void FeatureCompound::addObject(DocumentObject* obj)
{
    if (!hasObject(obj)) {
        std::vector<DocumentObject*> grp = Group.getValues();
        grp.push_back(obj);
        Group.setValues(grp);
    }
}

void FeatureCompound::removeObject(DocumentObject* obj)
{
    std::vector<DocumentObject*> grp = Group.getValues();
    for (std::vector<DocumentObject*>::iterator it = grp.begin(); it != grp.end(); ++it) {
        if (*it == obj) {
            grp.erase(it);
            Group.setValues(grp);
            break;
        }
    }
}

PyObject *FeatureCompound::getPyObject()
{
    if (PythonObject.is(Py::_None())){
        // ref counter is set to 1
        PythonObject = Py::Object(new FeaturePathCompoundPy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}

// Python Path Compound feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(Path::FeatureCompoundPython, Path::FeatureCompound)
template<> const char* Path::FeatureCompoundPython::getViewProviderName() const {
    return "PathGui::ViewProviderPathCompoundPython";
}
/// @endcond

// explicit template instantiation
template class PathExport FeaturePythonT<Path::FeatureCompound>;
}
