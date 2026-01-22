// SPDX-License-Identifier: LGPL-2.1-or-later

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


#pragma once

#include "GeoFeature.h"
#include "PropertyFile.h"


namespace App
{

class AppExport VRMLObject: public GeoFeature
{
    PROPERTY_HEADER_WITH_OVERRIDE(App::VRMLObject);

public:
    /// Constructor
    VRMLObject();

    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override
    {
        return "Gui::ViewProviderVRMLObject";
    }
    DocumentObjectExecReturn* execute() override
    {
        return DocumentObject::StdReturn;
    }
    short mustExecute() const override;
    PyObject* getPyObject() override;
    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;
    void SaveDocFile(Base::Writer& writer) const override;
    void RestoreDocFile(Base::Reader& reader) override;

    // NOLINTBEGIN
    PropertyFileIncluded VrmlFile;
    PropertyStringList Urls;
    PropertyStringList Resources;
    // NOLINTEND

protected:
    void onChanged(const App::Property*) override;

private:
    std::string getRelativePath(const std::string& prefix, const std::string& resource) const;
    static std::string fixRelativePath(const std::string& name, const std::string& resource);
    static void makeDirectories(const std::string& path, const std::string& subdir);
    bool restoreTextureFinished(Base::Reader& reader);
    void reloadFile();

private:
    mutable std::string vrmlPath;
    mutable int indexRestore {0};
    mutable int indexSave {0};
    mutable bool restoreData {false};
};

}  // namespace App
