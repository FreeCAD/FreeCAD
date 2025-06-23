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
# include <QFile>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>     //TODO: should be in DrawTemplate.h??
#include <Base/Console.h>
#include <Base/FileInfo.h>
#include <Base/Quantity.h>

#include "DrawPage.h"
#include "DrawSVGTemplate.h"
#include "DrawSVGTemplatePy.h"
#include "DrawUtil.h"
#include "XMLQuery.h"

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
        //if we are restoring an existing file we just want the properties set as they were saved,
        //but if we are not restoring, we need to replace the embedded file and extract the new
        //EditableTexts.
        //We could try to find matching field names are preserve the values from
        //the old template, but there is no guarantee that the same fields will be present.
        replaceFileIncluded(Template.getValue());
        EditableTexts.setValues(getEditableTextsFromTemplate());
        QDomDocument templateDocument;
        if (getTemplateDocument(PageResult.getValue(), templateDocument)) {
            extractTemplateAttributes(templateDocument);
        }
    }

    TechDraw::DrawTemplate::onChanged(prop);
}

void DrawSVGTemplate::onSettingDocument()
{
    attachDocument(DocumentObject::getDocument());
    DrawTemplate::onSettingDocument();
}

//? should this check for creation of a template or a page?
void DrawSVGTemplate::slotCreatedObject(const App::DocumentObject& obj)
{
    // Base::Console().message("DSVGT::slotCreatedObject()\n");
    if (!obj.isDerivedFrom<TechDraw::DrawPage>()) {
        // we don't care
        return;
    }
    EditableTexts.touch();
}

void DrawSVGTemplate::slotDeletedObject(const App::DocumentObject& obj)
{
    // Base::Console().message("DSVGT::slotDeletedObject()\n");
    if (!obj.isDerivedFrom<TechDraw::DrawPage>()) {
        // we don't care
        return;
    }
    EditableTexts.touch();
}

//parse the Svg code, inserting current EditableTexts values, and return the result as a QString.
//While parsing, note the Orientation, Width and Height values in the Svg code.
QString DrawSVGTemplate::processTemplate()
{
    if (isRestoring()) {
        //until everything is fully restored, the embedded file is not available, so we
        //can't do anything
        return QString();
    }

    QDomDocument templateDocument;
    if (!getTemplateDocument(PageResult.getValue(), templateDocument)) {
        return QString();
    }

    XMLQuery query(templateDocument);
    std::map<std::string, std::string> substitutions = EditableTexts.getValues();
    // auto captureTextValues = m_initialTextValues;

    // XPath query to select all <tspan> nodes whose <text> parent
    // has "freecad:editable" attribute
    query.processItems(QStringLiteral(
        "declare default element namespace \"" SVG_NS_URI "\"; "
        "declare namespace freecad=\"" FREECAD_SVG_NS_URI "\"; "
        "//text[@" FREECAD_ATTR_EDITABLE "]/tspan"),
        // [this, &substitutions, &templateDocument, &captureTextValues](QDomElement& tspan) -> bool {
        [&substitutions, &templateDocument](QDomElement& tspan) -> bool {
        // Replace the editable text spans with new nodes holding actual values

        QString editableName = tspan.parentNode().toElement().attribute(QString::fromUtf8(FREECAD_ATTR_EDITABLE));
        std::map<std::string, std::string>::iterator item =
            substitutions.find(editableName.toStdString());
        if (item != substitutions.end()) {
            // we have an editable text
            QDomElement parent = tspan.parentNode().toElement();
            QString editableValue = QString::fromUtf8(item->second.c_str());

            // Keep all spaces in the text node
            tspan.setAttribute(QStringLiteral("xml:space"), QStringLiteral("preserve"));

            // Remove all child nodes and append text node with editable replacement as the only descendant
            while (!tspan.lastChild().isNull()) {
                tspan.removeChild(tspan.lastChild());
            }
            tspan.appendChild(templateDocument.createTextNode(editableValue));
        }
        return true;
    });

    extractTemplateAttributes(templateDocument);
    //all Qt holds on files should be released on exit #4085
    return templateDocument.toString();
}

// find the width, height and orientation of the template and update the properties
void DrawSVGTemplate::extractTemplateAttributes(QDomDocument& templateDocument)
{
    QDomElement docElement = templateDocument.documentElement();
    Base::Quantity quantity;

    // Obtain the width
    QString str = docElement.attribute(QStringLiteral("width"));
    quantity = Base::Quantity::parse(str.toStdString());
    quantity.setUnit(Base::Unit::Length);

    Width.setValue(quantity.getValue());

    str = docElement.attribute(QStringLiteral("height"));
    quantity = Base::Quantity::parse(str.toStdString());
    quantity.setUnit(Base::Unit::Length);

    Height.setValue(quantity.getValue());

    bool isLandscape = getWidth() / getHeight() >= 1.;

    Orientation.setValue(isLandscape ? 1 : 0);
}

// load the included template file as a QDomDocument
bool DrawSVGTemplate::getTemplateDocument(std::string sourceFile, QDomDocument& templateDocument) const
{
    if (sourceFile.empty()) {
        return false;
    }
    QFile templateFile(QString::fromStdString(sourceFile));
    if (!templateFile.open(QIODevice::ReadOnly)) {
        Base::Console().error("DrawSVGTemplate::processTemplate cannot read embedded template %s!\n", PageResult.getValue());
        return false;
    }

    if (!templateDocument.setContent(&templateFile)) {
        Base::Console().error("DrawSVGTemplate::processTemplate - failed to parse file: %s\n",
            PageResult.getValue());
        return false;
    }
    // no errors templateDocument is loaded
    return true;
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


//! find the special fields in the template (freecad:editable or freecad:autofill)
std::map<std::string, std::string> DrawSVGTemplate::getEditableTextsFromTemplate()
{
    std::map<std::string, std::string> editables;

    QDomDocument templateDocument;
    if (!getTemplateDocument(PageResult.getValue(), templateDocument)) {
        return editables;
    }

    XMLQuery query(templateDocument);

    // XPath query to select all <tspan> nodes whose <text> parent
    // has "freecad:editable" attribute
    query.processItems(QStringLiteral(
        "declare default element namespace \"" SVG_NS_URI "\"; "
        "declare namespace freecad=\"" FREECAD_SVG_NS_URI "\"; "
        "//text[@" FREECAD_ATTR_EDITABLE "]/tspan"),
        [this, &editables](QDomElement& tspan) -> bool {
            QDomElement parent = tspan.parentNode().toElement();
            QString editableName = parent.attribute(QString::fromUtf8(FREECAD_ATTR_EDITABLE));
            QString editableValue;
            if (parent.hasAttribute(QString::fromUtf8(FREECAD_ATTR_AUTOFILL))) {
                QString autofillName = parent.attribute(QString::fromUtf8(FREECAD_ATTR_AUTOFILL));
                QString autofillValue = getAutofillValue(autofillName);
                if (!autofillValue.isEmpty()) {
                    editableValue = autofillValue;
                }
            }

            // If the autofill value is not specified or unsupported, use the default text value
            if (editableValue.isEmpty()) {
                editableValue = tspan.firstChild().nodeValue();
            }

            editables[std::string(editableName.toUtf8().constData())] =
                std::string(editableValue.toUtf8().constData());
            return true;
        });

    return editables;
}

QString  DrawSVGTemplate::getAutofillByEditableName(QString nameToMatch)
{
    QString result;
    QString nameCapture{nameToMatch};

    QDomDocument templateDocument;
    if (!getTemplateDocument(PageResult.getValue(), templateDocument)) {
        return {};
    }

    XMLQuery query(templateDocument);

    // XPath query to select all <tspan> nodes whose <text> parent
    // has "freecad:editable" attribute
    query.processItems(QStringLiteral(
        "declare default element namespace \"" SVG_NS_URI "\"; "
        "declare namespace freecad=\"" FREECAD_SVG_NS_URI "\"; "
        "//text[@" FREECAD_ATTR_EDITABLE "]/tspan"),
        [this, &nameCapture, &result](QDomElement& tspan) -> bool {
            QDomElement parent = tspan.parentNode().toElement();
            QString editableName = parent.attribute(QString::fromUtf8(FREECAD_ATTR_EDITABLE));
            if (editableName == nameCapture  &&
                parent.hasAttribute(QString::fromUtf8(FREECAD_ATTR_AUTOFILL))) {
                QString autofillName = parent.attribute(QString::fromUtf8(FREECAD_ATTR_AUTOFILL));
                QString autofillValue = getAutofillValue(autofillName);
                if (!autofillValue.isEmpty()) {
                    result = autofillValue;
                }
            }
            return true;
        });
    return result;
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
