/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef APP_VRMLOROBJECT_H
#define APP_VRMLOROBJECT_H

#include "GeoFeature.h"
#include "PropertyFile.h"


namespace App
{

class AppExport VRMLObject : public GeoFeature
{
    PROPERTY_HEADER(App::VRMLObject);

public:
    /// Constructor
    VRMLObject(void);
    virtual ~VRMLObject();

    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName(void) const {
        return "Gui::ViewProviderVRMLObject";
    }
    virtual DocumentObjectExecReturn *execute(void) {
        return DocumentObject::StdReturn;
    }
    virtual short mustExecute(void) const;
    virtual PyObject *getPyObject(void);
    virtual void Save (Base::Writer &writer) const;
    virtual void Restore(Base::XMLReader &reader);
    virtual void SaveDocFile (Base::Writer &writer) const;
    virtual void RestoreDocFile(Base::Reader &reader);

    PropertyFileIncluded VrmlFile;
    PropertyStringList Urls;
    PropertyStringList Resources;

protected:
    void onChanged(const App::Property*);
    std::string getRelativePath(const std::string&, const std::string&) const;
    std::string fixRelativePath(const std::string&, const std::string&) const;
    void makeDirectories(const std::string&, const std::string&);

private:
    mutable std::string vrmlPath;
    mutable int index;
};

} //namespace App


#endif // APP_INVENTOROBJECT_H
