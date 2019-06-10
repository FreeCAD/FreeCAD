/***************************************************************************
 *   Copyright (c) 2014 Luke Parry <l.parry@warwick.ac.uk>                 *
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2002     *
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
  #include <sstream>
  #include <QDomDocument>
  #include <QFile>
#endif

#include <QXmlQuery>
#include <QXmlResultItems>
#include "QDomNodeModel.h"

#include <Base/Exception.h>
#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/FileInfo.h>
#include <Base/PyObjectBase.h>
#include <Base/Quantity.h>

#include <App/Application.h>

#include <boost/regex.hpp>
#include <boost/graph/graph_concepts.hpp>
#include <iostream>
#include <iterator>

#include <QDebug>

#include "DrawPage.h"
#include "DrawSVGTemplate.h"

#include <Mod/TechDraw/App/DrawSVGTemplatePy.h>

using namespace TechDraw;
using namespace std;

PROPERTY_SOURCE(TechDraw::DrawSVGTemplate, TechDraw::DrawTemplate)

DrawSVGTemplate::DrawSVGTemplate()
{
    static const char *group = "Template";

    //TODO: Do we need PageResult anymore?  -wf Yes!
    // PageResult points to a temporary file in tmp/FreeCAD-AB-CD-EF-.../myTemplate.svg
    // which is really copy of original Template with EditableFields replaced
    // When restoring saved document, Template is redundant/incorrect/not present - PageResult is the correct info.  -wf-
    ADD_PROPERTY_TYPE(PageResult, (0),  group, App::Prop_Output,    "Current SVG code for template");
    ADD_PROPERTY_TYPE(Template,   (""), group, App::Prop_Transient, "Template for the page");             //sb TemplateFileName???

    // Width and Height properties shouldn't be set by the user
    Height.setStatus(App::Property::ReadOnly,true);
    Width.setStatus(App::Property::ReadOnly,true);
    Orientation.setStatus(App::Property::ReadOnly,true);
}

DrawSVGTemplate::~DrawSVGTemplate()
{
}

PyObject *DrawSVGTemplate::getPyObject(void)
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DrawSVGTemplatePy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}

unsigned int DrawSVGTemplate::getMemSize(void) const
{
    return 0;
}

short DrawSVGTemplate::mustExecute() const
{
    return TechDraw::DrawTemplate::mustExecute();
}

void DrawSVGTemplate::onChanged(const App::Property* prop)
{
    bool updatePage = false;

    if (prop == &PageResult) {
        if (isRestoring()) {

            //original template has been stored in fcstd file
            Template.setValue(PageResult.getValue());
        }
    } else if (prop == &Template) {            //fileName has changed
        if (!isRestoring()) {
            EditableTexts.setValues(getEditableTextsFromTemplate());
            updatePage = true;
        }
    } else if (prop == &EditableTexts) {
        if (!isRestoring()) {
            updatePage = true;
        }
    }

    if (updatePage) {
        execute();
    }

    TechDraw::DrawTemplate::onChanged(prop);
}

App::DocumentObjectExecReturn * DrawSVGTemplate::execute(void)
{
    std::string templateFilename = Template.getValue();
    if (templateFilename.empty())
        return App::DocumentObject::StdReturn;

    Base::FileInfo fi(templateFilename);
    if (!fi.isReadable()) {
        // non-empty template value, but can't read file
        // if there is a old absolute template file set use a redirect
        fi.setFile(App::Application::getResourceDir() + "Mod/Drawing/Templates/" + fi.fileName());
        // try the redirect
        if (!fi.isReadable()) {
            Base::Console().Log("DrawPage::execute() not able to open %s!\n", Template.getValue());
            std::string error = std::string("Cannot open file ") + Template.getValue();
            return new App::DocumentObjectExecReturn(error);
        }
    }

    if (std::string(PageResult.getValue()).empty())                    //first time through?
        PageResult.setValue(fi.filePath().c_str());

    QFile templateFile(QString::fromUtf8(fi.filePath().c_str()));
    if (!templateFile.open(QIODevice::ReadOnly)) {
        Base::Console().Log("DrawPage::execute() can't read template %s!\n", Template.getValue());
        std::string error = std::string("Cannot read file ") + Template.getValue();
        return new App::DocumentObjectExecReturn(error);
    }

    QDomDocument templateDocument;
    if (!templateDocument.setContent(&templateFile)) {
        Base::Console().Message("DrawPage::execute() - failed to parse file: %s\n",
                                Template.getValue());
        std::string error = std::string("Cannot parse file ") + Template.getValue();
        return new App::DocumentObjectExecReturn(error);
    }

    QXmlQuery query(QXmlQuery::XQuery10);
    QDomNodeModel model(query.namePool(), templateDocument);
    query.setFocus(QXmlItem(model.fromDomNode(templateDocument.documentElement())));

    // XPath query to select all <tspan> nodes whose <text> parent
    // has "freecad:editable" attribute
    query.setQuery(QString::fromUtf8(
        "declare default element namespace \"" SVG_NS_URI "\"; "        
        "declare namespace freecad=\"" FREECAD_SVG_NS_URI "\"; "
        "//text[@freecad:editable]/tspan"));

    QXmlResultItems queryResult;
    query.evaluateTo(&queryResult);

    std::map<std::string, std::string> substitutions = EditableTexts.getValues();
    while (!queryResult.next().isNull())
    {
        QDomElement tspan = model.toDomNode(queryResult.current().toNodeModelIndex()).toElement();

        // Replace the editable text spans with new nodes holding actual values
        std::map<std::string, std::string>::iterator item =
            substitutions.find(tspan.parentNode().toElement()
                               .attribute(QString::fromUtf8("freecad:editable")).toStdString());
        if (item != substitutions.end()) {
            // Keep all spaces in the text node
            tspan.setAttribute(QString::fromUtf8("xml:space"), QString::fromUtf8("preserve"));

            // Remove all child nodes and append text node with editable replacement as the only descendant
            while (!tspan.lastChild().isNull()) {
                tspan.removeChild(tspan.lastChild());
            }
            tspan.appendChild(templateDocument.createTextNode(QString::fromUtf8(item->second.c_str())));
        }
    }

    string pageResultFilename = PageResult.getExchangeTempFile();
    QFile pageResult(QString::fromUtf8(pageResultFilename.c_str()));
    if (pageResult.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&pageResult);
        stream << templateDocument.toString();
        pageResult.close();
        PageResult.setValue(pageResultFilename.c_str());
    }
    else {
        Base::Console().Message("DrawPage::execute() - failed to open file for writing: %s\n",
                                pageResultFilename.c_str());
    }

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

    return TechDraw::DrawTemplate::execute();
}

double DrawSVGTemplate::getWidth() const
{
    return Width.getValue();
}

double DrawSVGTemplate::getHeight() const
{
    return Height.getValue();
}

std::map<std::string, std::string> DrawSVGTemplate::getEditableTextsFromTemplate()
{
    std::map<std::string, std::string> editables;

    std::string templateFilename = Template.getValue();
    if (templateFilename.empty()) {
        return editables;
    }

    Base::FileInfo tfi(templateFilename);
    if (!tfi.isReadable()) {
        // if there is a old absolute template file set use a redirect
        tfi.setFile(App::Application::getResourceDir() + "Mod/Drawing/Templates/" + tfi.fileName());
        // try the redirect
        if (!tfi.isReadable()) {
            Base::Console().Log("DrawPage::getEditableTextsFromTemplate() not able to open %s!\n", Template.getValue());
            return editables;
        }
    }

    QFile templateFile(QString::fromUtf8(tfi.filePath().c_str()));
    if (!templateFile.open(QIODevice::ReadOnly)) {
        Base::Console().Log("DrawPage::getEditableTextsFromTemplate() can't read template %s!\n", Template.getValue());
        return editables;
    }

    QDomDocument templateDocument;
    if (!templateDocument.setContent(&templateFile)) {
        Base::Console().Message("DrawPage::getEditableTextsFromTemplate() - failed to parse file: %s\n",
                                Template.getValue());
        return editables;
    }

    QXmlQuery query(QXmlQuery::XQuery10);
    QDomNodeModel model(query.namePool(), templateDocument, true);
    query.setFocus(QXmlItem(model.fromDomNode(templateDocument.documentElement())));

    // XPath query to select all <tspan> nodes whose <text> parent
    // has "freecad:editable" attribute
    query.setQuery(QString::fromUtf8(
        "declare default element namespace \"" SVG_NS_URI "\"; "        
        "declare namespace freecad=\"" FREECAD_SVG_NS_URI "\"; "
        "//text[@freecad:editable]/tspan"));

    QXmlResultItems queryResult;
    query.evaluateTo(&queryResult);

    while (!queryResult.next().isNull())
    {
        QDomElement tspan = model.toDomNode(queryResult.current().toNodeModelIndex()).toElement();

        // Takeover the names stored as attributes and the values stored as text items
        editables[tspan.parentNode().toElement().attribute(QString::fromUtf8("freecad:editable"))
                  .toStdString()] = tspan.firstChild().nodeValue().toStdString();
    }

    return editables;
}


// Python Template feature ---------------------------------------------------------
namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TechDraw::DrawSVGTemplatePython, TechDraw::DrawSVGTemplate)
template<> const char* TechDraw::DrawSVGTemplatePython::getViewProviderName(void) const {
    return "TechDrawGui::ViewProviderPython";
}
/// @endcond

// explicit template instantiation
template class TechDrawExport FeaturePythonT<TechDraw::DrawSVGTemplate>;
}
