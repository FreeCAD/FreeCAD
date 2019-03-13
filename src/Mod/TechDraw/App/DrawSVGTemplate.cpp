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
    std::string templValue = Template.getValue();
    if (templValue.empty())
        return App::DocumentObject::StdReturn;

    Base::FileInfo fi(templValue);
    if (!fi.isReadable()) {
        // non-empty template value, but can't read file
        // if there is a old absolute template file set use a redirect
        fi.setFile(App::Application::getResourceDir() + "Mod/Drawing/Templates/" + fi.fileName());
        // try the redirect
        if (!fi.isReadable()) {
            Base::Console().Log("DrawPage::execute() not able to open %s!\n",Template.getValue());
            std::string error = std::string("Cannot open file ") + Template.getValue();
            return new App::DocumentObjectExecReturn(error);
        }
    }

    if (std::string(PageResult.getValue()).empty())                    //first time through?
        PageResult.setValue(fi.filePath().c_str());

    // open Template file
    string line;
    ifstream inTemplate (fi.filePath().c_str());

    ostringstream copyTemplate;
    string tempendl = "--endOfLine--";

    //inTemplate to copyTemplate
    //remove DrawingContent comment line
    //change line endings
    //capture TitleBlock dimensions
    while (getline(inTemplate,line))
    {
        // copy every line except the DrawingContent comment?
        if(line.find("<!-- DrawingContent -->") == string::npos) {
            // if not -  write through
            copyTemplate << line << tempendl;
        }

        //double t0, t1,t2,t3;
        float t0, t1,t2,t3;
        if(line.find("<!-- Title block") != std::string::npos) {
            (void) sscanf(line.c_str(), "%*s %*s %*s %f %f %f %f", &t0, &t1, &t2, &t3);    //eg "    <!-- Working space 10 10 410 287 -->"
                                                                                           //coverity 151677
            blockDimensions = QRectF(t0, t1, t2 - t0, t3 - t1);
        }

    }
    inTemplate.close();

    string outfragment(copyTemplate.str());
    std::string newfragment = outfragment;

    // update EditableText SVG clauses with Property values
    std::map<std::string, std::string> subs = EditableTexts.getValues();

    if (subs.size() > 0) {
        boost::regex e1 ("<text.*?freecad:editable=\"(.*?)\".*?<tspan.*?>(.*?)</tspan>");
        string::const_iterator begin, end;
        begin = outfragment.begin();
        end = outfragment.end();
        boost::match_results<std::string::const_iterator> what;

        // Find editable texts
        while (boost::regex_search(begin, end, what, e1)) {            //search in outfragment
            // if we have a replacement value for the text we've found
            if (subs.count(what[1].str())) {
                 // change it to specified value
                 boost::regex e2 ("(<text.*?freecad:editable=\"" + what[1].str() + "\".*?<tspan.*?)>(.*?)(</tspan>)");
                 newfragment = boost::regex_replace(newfragment, e2, "$1>" + subs[what[1].str()] + "$3");   //replace in newfragment
            }
            begin = what[0].second;
        }
    }


    // restoring linebreaks and saving the file
    boost::regex e3 ("--endOfLine--");
    string fmt = "\\n";
    outfragment = boost::regex_replace(newfragment, e3, fmt);

    const QString qsOut = QString::fromStdString(outfragment);
    QDomDocument doc(QString::fromLatin1("mydocument"));

    //if (!doc.setContent(&resultFile)) {
    if (!doc.setContent(qsOut)) {
        //setError(); //???? how/when does this get reset?
        std::string errMsg = std::string("Invalid SVG syntax in ") + getNameInDocument() + std::string(" - Check EditableTexts");
        return new App::DocumentObjectExecReturn(errMsg);
    } else {
        // make a temp file for FileIncluded Property
        string tempName = PageResult.getExchangeTempFile();
        ofstream outfinal(tempName.c_str());
        outfinal << outfragment;
        outfinal.close();
        PageResult.setValue(tempName.c_str());
    }

    // Calculate the dimensions of the page and store for retrieval
    // Parse the document XML
    QDomElement docElem = doc.documentElement();

    // Obtain the size of the SVG document by reading the document attributes
    Base::Quantity quantity;

    // Obtain the width
    QString str = docElem.attribute(QString::fromLatin1("width"));
    quantity = Base::Quantity::parse(str);
    quantity.setUnit(Base::Unit::Length);

    Width.setValue(quantity.getValue());

    str = docElem.attribute(QString::fromLatin1("height"));
    quantity = Base::Quantity::parse(str);
    quantity.setUnit(Base::Unit::Length);

    Height.setValue(quantity.getValue());

    bool isLandscape = getWidth() / getHeight() >= 1.;

    Orientation.setValue(isLandscape ? 1 : 0);

    return TechDraw::DrawTemplate::execute();
}

void DrawSVGTemplate::getBlockDimensions(double &x, double &y, double &width, double &height) const
{
    x      = blockDimensions.left();
    y      = blockDimensions.bottom();
    width  = blockDimensions.width();
    height = blockDimensions.height();
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
    std::map<std::string, std::string> eds;

    std::string temp = Template.getValue();
    if (!temp.empty()) {
        Base::FileInfo tfi(temp);
        if (!tfi.isReadable()) {
            // if there is a old absolute template file set use a redirect
            tfi.setFile(App::Application::getResourceDir() + "Mod/Drawing/Templates/" + tfi.fileName());
            // try the redirect
            if (!tfi.isReadable()) {
                return eds;
            }
        }
        string tline, tfrag;
        ifstream tfile (tfi.filePath().c_str());
        while (getline (tfile,tline)) {
            tfrag += tline;
            tfrag += "--endOfLine--";
        }
        tfile.close();
        //this catches all the tags: <text ... </tspan></text>
        //keep tagRegex in sync with Gui/QGISVGTemplate.cpp
        boost::regex tagRegex ("<text([^>]*freecad:editable=[^>]*)>[^<]*<tspan[^>]*>([^<]*)</tspan>");
        boost::regex nameRegex("freecad:editable=\"(.*?)\"");
        boost::regex valueRegex("<tspan.*?>(.*?)</tspan>");

        string::const_iterator tbegin, tend;
        tbegin = tfrag.begin();
        tend = tfrag.end();
        boost::match_results<std::string::const_iterator> tagMatch;
        boost::match_results<std::string::const_iterator> nameMatch;
        boost::match_results<std::string::const_iterator> valueMatch;
        while (boost::regex_search(tbegin, tend, tagMatch, tagRegex)) {
            if ( boost::regex_search(tagMatch[0].first, tagMatch[0].second, nameMatch, nameRegex) &&
                 boost::regex_search(tagMatch[0].first, tagMatch[0].second, valueMatch, valueRegex)) {
                //found valid name/value pair
                string name = nameMatch[1];
                string value = valueMatch[1];
                if (eds.count(name) > 0) {
                    //TODO: Throw or [better] change key
                    qDebug() << "Got duplicate value for key "<<name.c_str();
                } else {
                    eds[name] = value;
                }
            }
            tbegin = tagMatch[0].second;
        }
    }
    return eds;
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
