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
# include <QDomNodeModel.h>
# include <QFile>
# include <QXmlQuery>
# include <QXmlResultItems>
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


using namespace TechDraw;

PROPERTY_SOURCE(TechDraw::DrawSVGTemplate, TechDraw::DrawTemplate)

DrawSVGTemplate::DrawSVGTemplate()
{
    static const char *group = "Template";

    ADD_PROPERTY_TYPE(PageResult, (nullptr),  group, App::Prop_Output,    "Embedded SVG code for template. For system use.");   //n/a for users
    ADD_PROPERTY_TYPE(Template,   (""), group, App::Prop_None, "Template file name.");

    // Width and Height properties shouldn't be set by the user
    Height.setStatus(App::Property::ReadOnly, true);
    Width.setStatus(App::Property::ReadOnly, true);
    Orientation.setStatus(App::Property::ReadOnly, true);

    std::string svgFilter("Svg files (*.svg *.SVG);;All files (*)");
    Template.setFilter(svgFilter);
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
    if (prop == &Template && !isRestoring()) {
        //if we are restoring an existing file we just want the properties set as they were save,
        //but if we are not restoring, we need to replace the embedded file and extract the new
        //EditableTexts.
        //We could try to find matching field names are preserve the values from
        //the old template, but there is no guarantee that the same fields will be present.
        replaceFileIncluded(Template.getValue());
        EditableTexts.setValues(getEditableTextsFromTemplate());
    } else if (prop == &EditableTexts) {
        //handled by ViewProvider
    }

    TechDraw::DrawTemplate::onChanged(prop);
}

//parse the Svg code, inserting current EditableTexts values, and return the result as a QString.
//While parsing, not the Orientation, Width and Height values in the Svg code.
QString DrawSVGTemplate::processTemplate()
{
//    Base::Console().Message("DSVGT::processTemplate() - isRestoring: %d\n", isRestoring());
    if (isRestoring()) {
        //until everything is fully restored, the embedded file is not available, so we
        //can't do anything
        return QString();
    }

    QFile templateFile(Base::Tools::fromStdString(PageResult.getValue()));
    if (!templateFile.open(QIODevice::ReadOnly)) {
        Base::Console().Error("DrawSVGTemplate::processTemplate can't read embedded template %s!\n", PageResult.getValue());
        return QString();
    }

	QDomDocument templateDocument;
	if (!templateDocument.setContent(&templateFile)) {
        Base::Console().Error("DrawSVGTemplate::processTemplate - failed to parse file: %s\n",
            PageResult.getValue());
		return QString();
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
		QString editableName = tspan.parentNode().toElement().attribute(QString::fromUtf8("freecad:editable"));
		std::map<std::string, std::string>::iterator item =
			substitutions.find(std::string(editableName.toUtf8().constData()));
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
	return templateDocument.toString();
}

double DrawSVGTemplate::getWidth() const
{
    return Width.getValue();
}

double DrawSVGTemplate::getHeight() const
{
    return Height.getValue();
}

void DrawSVGTemplate::replaceFileIncluded(std::string newTemplateFileName)
{
//    Base::Console().Message("DSVGT::replaceFileIncluded(%s)\n", newTemplateFileName.c_str());
    if (newTemplateFileName.empty()) {
        return;
    }

    Base::FileInfo tfi(newTemplateFileName);
    if (tfi.isReadable()) {
        PageResult.setValue(newTemplateFileName.c_str());
    } else {
        throw Base::RuntimeError("Could not read the new template file");
    }
}

std::map<std::string, std::string> DrawSVGTemplate::getEditableTextsFromTemplate()
{
//    Base::Console().Message("DSVGT::getEditableTextsFromTemplate()\n");
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
            Base::Console().Log("DrawSVGTemplate::getEditableTextsFromTemplate() not able to open %s!\n", Template.getValue());
            return editables;
        }
    }

    QFile templateFile(QString::fromUtf8(tfi.filePath().c_str()));
    if (!templateFile.open(QIODevice::ReadOnly)) {
        Base::Console().Log("DrawSVGTemplate::getEditableTextsFromTemplate() can't read template %s!\n", Template.getValue());
        return editables;
    }

    QDomDocument templateDocument;
    if (!templateDocument.setContent(&templateFile)) {
        Base::Console().Message("DrawSVGTemplate::getEditableTextsFromTemplate() - failed to parse file: %s\n",
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

    while (!queryResult.next().isNull()) {
        QDomElement tspan = model.toDomNode(queryResult.current().toNodeModelIndex()).toElement();

        QString editableName = tspan.parentNode().toElement().attribute(QString::fromUtf8("freecad:editable"));
        QString editableValue = tspan.firstChild().nodeValue();

        editables[std::string(editableName.toUtf8().constData())] =
            std::string(editableValue.toUtf8().constData());
    }

    return editables;
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
