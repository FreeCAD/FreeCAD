/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2014 Luke Parry <l.parry@warwick.ac.uk>                 *
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

#ifndef _PreComp_
# include <sstream>
# include <QDomDocument>
# include <QFile>
# include <QJsonDocument>
# include <QJsonArray>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>     //TODO: should be in DrawTemplate.h??
#include <Base/Console.h>
#include <Base/FileInfo.h>
#include <Base/Quantity.h>
#include <Base/Tools.h>

#include "DrawPage.h"
#include "DrawSVGTemplate.h"
#include "DrawSVGTemplatePy.h"
#include "DrawUtil.h"
#include "XMLQuery.h"
#include "Preferences.h"

using namespace TechDraw;

PROPERTY_SOURCE(TechDraw::DrawSVGTemplate, TechDraw::DrawTemplate)

DrawSVGTemplate::DrawSVGTemplate()
{
    static const char *group = "Template";

    ADD_PROPERTY_TYPE(Filepath, (nullptr), group, App::Prop_Output, "Filename containing the SVG markup for the template");   //n/a for users

    // Width and Height properties shouldn't be set by the user
    Height.setStatus(App::Property::ReadOnly, true);
    Width.setStatus(App::Property::ReadOnly, true);
    Orientation.setStatus(App::Property::ReadOnly, true);

    std::string svgFilter("Svg files (*.svg *.SVG);;All files (*)");
    Filepath.setFilter(svgFilter);
}

DrawSVGTemplate::~DrawSVGTemplate()
{
}

PyObject *DrawSVGTemplate::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DrawSVGTemplatePy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}

void DrawSVGTemplate::onChanged(const App::Property* prop)
{
    if (prop == &Filepath && !isRestoring()) {
        //if we are restoring an existing file we just want the properties set as they were save,
        //but if we are not restoring, we need to replace the embedded file and extract the new
        //EditableTexts.
        //We could try to find matching field names are preserve the values from
        //the old template, but there is no guarantee that the same fields will be present.
        checkFilepath();
        processTemplate();
        fillEditableTexts();
    } else if (prop == &EditableTexts) {
        // partially handled by ViewProvider
        processTemplate();
    }

    TechDraw::DrawTemplate::onChanged(prop);
}

//parse the Svg code, inserting current EditableTexts values, and return the result as a QString.
//While parsing, note the Orientation, Width and Height values in the Svg code.
void DrawSVGTemplate::processTemplate()
{
//    Base::Console().Message("DSVGT::processTemplate() - isRestoring: %d\n", isRestoring());
    if (isRestoring()) {
        //until everything is fully restored, the embedded file is not available, so we
        //can't do anything
        return;
    }

    QFile templateFile(Base::Tools::fromStdString(Filepath.getValue()));
    if (!templateFile.open(QIODevice::ReadOnly)) {
        Base::Console().Error("DrawSVGTemplate::processTemplate can't read embedded template %s!\n", Filepath.getValue());
        return;
    }

    QDomDocument templateDocument;
    if (!templateDocument.setContent(&templateFile)) {
        Base::Console().Error("DrawSVGTemplate::processTemplate - failed to parse file: %s\n",
            Filepath.getValue());
        return;
    }

    XMLQuery query(templateDocument);
    std::map<std::string, std::string> substitutions = EditableTexts.getValues();

    // XPath query to select all <tspan> nodes whose <text> parent
    // has "freecad:editable" attribute
    query.processItems(QString::fromUtf8(
        "declare default element namespace \"" SVG_NS_URI "\"; "
        "declare namespace freecad=\"" FREECAD_SVG_NS_URI "\"; "
        "//text[@freecad:editable]/tspan"),
        [&substitutions, &templateDocument](QDomElement& tspan) -> bool {
        // Replace the editable text spans with new nodes holding actual values
        QString editableName = tspan.parentNode().toElement().attribute(QString::fromUtf8("freecad:editable"));
        std::map<std::string, std::string>::iterator item =
            substitutions.find(editableName.toStdString());
        if (item != substitutions.end()) {
            // Keep all spaces in the text node
            tspan.setAttribute(QString::fromUtf8("xml:space"), QString::fromUtf8("preserve"));

            // Remove all child nodes and append text node with editable replacement as the only descendant
            while (!tspan.lastChild().isNull()) {
                tspan.removeChild(tspan.lastChild());
            }
            tspan.appendChild(templateDocument.createTextNode(QString::fromUtf8(item->second.c_str())));
        }
        return true;
    });

    // Calculate the dimensions of the page and store for retrieval
    // Obtain the size of the SVG document by reading the document attributes
    QDomElement docElement = templateDocument.documentElement();
    Base::Quantity quantity;

    // Obtain the width
    QString str = docElement.attribute(QString::fromLatin1("width"));
    quantity = Base::Quantity::parse(str);
    quantity.setUnit(Base::Unit::Length);

    Width.setValue(quantity.getValue());

    str = docElement.attribute(QString::fromLatin1("height"));
    quantity = Base::Quantity::parse(str);
    quantity.setUnit(Base::Unit::Length);

    Height.setValue(quantity.getValue());

    bool isLandscape = getWidth() / getHeight() >= 1.;

    Orientation.setValue(isLandscape ? 1 : 0);

    //all Qt holds on files should be released on exit #4085
    m_SVGMarkup = templateDocument.toString();
}

double DrawSVGTemplate::getWidth() const
{
    return Width.getValue();
}

double DrawSVGTemplate::getHeight() const
{
    return Height.getValue();
}

void DrawSVGTemplate::checkFilepath()
{
    // Base::Console().Message("DSVGT::checkFilepath(%s)\n", Filepath.getValue());
    if (Filepath.getValue()) {
        return;
    }

    Base::FileInfo tfi(Filepath.getValue());
    if (!tfi.isReadable()) {
        // If there is an old absolute template file set use a redirect
        const std::string newPath = App::Application::getResourceDir() + "Mod/Drawing/Templates/" + tfi.fileName();
        tfi.setFile(newPath);
        // Try the redirect
        if (tfi.isReadable()) {
            Filepath.setValue(newPath.c_str());
            return;
        }
        Filepath.setValue(nullptr);// We don't want to use an invalid filepath further down the road
        throw Base::RuntimeError("Could not read the new template file");
    }
}

void DrawSVGTemplate::fillEditableTexts() { 
    QByteArray rawPresetEntries = QByteArray::fromStdString( 
        Preferences::getPreferenceGroup("Template")->GetASCII("EditableTextPresets") 
    ); 
    QJsonArray presetEntries = QJsonDocument::fromJson(rawPresetEntries).array();

    EditableTexts.setStatus(App::Property::Busy, true);
    for(const QJsonValue presetEntry : presetEntries) { 
        std::string fieldName = presetEntry.toArray().at(0).toString().toStdString(); 
        std::string value = presetEntry.toArray().at(1).toString().toStdString();
        // Currently, everytime we set a single value, it will trigger processTemplate() :(
        EditableTexts.setValue(fieldName, value);
    }
}

//! get a translated label string from the context (ex TaskActiveView), the base name (ex ActiveView) and
//! the unique name within the document (ex ActiveView001), and use it to update the Label property.
void DrawSVGTemplate::translateLabel(std::string context, std::string baseName, std::string uniqueName)
{
    Label.setValue(DrawUtil::translateArbitrary(context, baseName, uniqueName));
}

// Python Template feature ---------------------------------------------------------
namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TechDraw::DrawSVGTemplatePython, TechDraw::DrawSVGTemplate)
template<> const char* TechDraw::DrawSVGTemplatePython::getViewProviderName() const {
    return "TechDrawGui::ViewProviderPython";
}
/// @endcond

// explicit template instantiation
template class TechDrawExport FeaturePythonT<TechDraw::DrawSVGTemplate>;
}
