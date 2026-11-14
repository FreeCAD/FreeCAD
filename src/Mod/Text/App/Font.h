/***************************************************************************
 *   Copyright (c) 2024 Martin Rodriguez Reboredo <yakoyoku@gmail.com>     *
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

#ifndef TEXT_Font_H
#define TEXT_Font_H

#include <App/DocumentObject.h>
#include <App/FeaturePython.h>
#include <App/PropertyFile.h>
#include <CXX/Objects.hxx>
#include <Mod/Text/TextGlobal.h>


namespace Text
{


class TextExport Font : public App::DocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(Text::Font);

public:
    Font();
    ~Font() override;

    App::PropertyString       Name;
    App::PropertyEnumeration  Source;
    App::PropertyFile         File;
    App::PropertyFileIncluded Included;

    PyObject* getPyObject() override;

    App::DocumentObjectExecReturn* execute() override;
    short mustExecute() const override;

    const char* getViewProviderName() const override
    {
        return "TextGui::ViewProviderFont";
    }

protected:
    void onChanged(const App::Property* prop) override;

private:
    static const char* SourceEnums[];
};

using FontPython = App::FeaturePythonT<Font>;

}  // namespace Text

#endif // TEXT_Font_H
