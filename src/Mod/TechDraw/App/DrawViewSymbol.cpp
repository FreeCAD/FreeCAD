/***************************************************************************
 *   Copyright (c) 2013 Yorik van Havre <yorik@uncreated.net>              *
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
#endif

#include <Base/Console.h>

#include "DrawViewSymbol.h"
#include "DrawViewSymbolPy.h" // generated from DrawViewSymbolPy.xml
#include "DrawPage.h"
#include "DrawUtil.h"
#include "XMLQuery.h"


using namespace TechDraw;

//===========================================================================
// DrawViewSymbol
//===========================================================================

PROPERTY_SOURCE(TechDraw::DrawViewSymbol, TechDraw::DrawView)


DrawViewSymbol::DrawViewSymbol()
{
    static const char* vgroup = "Drawing view";

    ADD_PROPERTY_TYPE(Symbol, (""), vgroup, App::Prop_None, "The SVG code defining this symbol");
    ADD_PROPERTY_TYPE(EditableTexts, (""), vgroup, App::Prop_None,
                      "Substitution values for the editable strings in this symbol");
    ScaleType.setValue("Custom");
    Scale.setStatus(App::Property::ReadOnly, false);
    Symbol.setStatus(App::Property::Hidden, true);
}

DrawViewSymbol::~DrawViewSymbol() {}

void DrawViewSymbol::onChanged(const App::Property* prop)
{
    if (prop == &Symbol) {
        if (!isRestoring() && !Symbol.isEmpty()) {
            std::vector<std::string> editables = getEditableFields();
            EditableTexts.setValues(editables);
        }
    }
    else if (prop == &EditableTexts) {
        //this will change Symbol, which will call onChanged(Symbol),
        //which will change EditableTexts, but the loop stops after
        //1 cycle
        updateFieldsInSymbol();
    }

    TechDraw::DrawView::onChanged(prop);
}

App::DocumentObjectExecReturn* DrawViewSymbol::execute()
{
    //nothing to do. DVS is just a container for properties.
    //the action takes place on the Gui side.
    return DrawView::execute();
}

QRectF DrawViewSymbol::getRect() const
{
    double w = 64.0;//must default to something
    double h = 64.0;
    return QRectF(0, 0, w, h);
}

//!Assume all svg files fit the page and/or the user will scale manually
bool DrawViewSymbol::checkFit(TechDraw::DrawPage* p) const
{
    (void)p;
    return true;
}

//get editable fields from symbol
std::vector<std::string> DrawViewSymbol::getEditableFields()
{
    QDomDocument symbolDocument;
    std::vector<std::string> editables;

    bool rc = loadQDomDocument(symbolDocument);
    if (rc) {
        XMLQuery query(symbolDocument);

        // XPath query to select all <tspan> nodes whose <text> parent
        // has "freecad:editable" attribute
        query.processItems(QString::fromUtf8("declare default element namespace \"" SVG_NS_URI "\"; "
                                             "declare namespace freecad=\"" FREECAD_SVG_NS_URI "\"; "
                                             "//text[@freecad:editable]/tspan"),
                           [&editables](QDomElement& tspan) -> bool {
            QString editableValue = tspan.firstChild().nodeValue();
            editables.emplace_back(editableValue.toStdString());
            return true;
        });
    }
    return editables;
}

//replace editable field in symbol with values from property
void DrawViewSymbol::updateFieldsInSymbol()
{
    const std::vector<std::string>& editText = EditableTexts.getValues();
    if (editText.empty()) {
        return;
    }

    QDomDocument symbolDocument;

    bool rc = loadQDomDocument(symbolDocument);
    if (rc) {
        XMLQuery query(symbolDocument);
        std::size_t count = 0;

        // XPath query to select all <tspan> nodes whose <text> parent
        // has "freecad:editable" attribute
        query.processItems(QString::fromUtf8("declare default element namespace \"" SVG_NS_URI "\"; "
                                             "declare namespace freecad=\"" FREECAD_SVG_NS_URI "\"; "
                                             "//text[@freecad:editable]/tspan"),
                           [&symbolDocument, &editText, &count](QDomElement& tspanElement) -> bool {

            if (count >= editText.size()) {
                return false;
            }
            // Keep all spaces in the text node
            tspanElement.setAttribute(QString::fromUtf8("xml:space"),
                                      QString::fromUtf8("preserve"));

            // Remove all child nodes (if any)
            while (!tspanElement.lastChild().isNull()) {
                tspanElement.removeChild(tspanElement.lastChild());
            }

            // Finally append text node with editable replacement as the only <tspan> descendant
            tspanElement.appendChild(
                symbolDocument.createTextNode(QString::fromStdString(editText[count])));
            ++count;
            return true;
        });

        Symbol.setValue(symbolDocument.toString(1).toStdString());
    }
}

//load QDomDocument
bool DrawViewSymbol::loadQDomDocument(QDomDocument& symbolDocument)
{
    const char* symbol = Symbol.getValue();
    QByteArray qba(symbol);
    if (qba.isEmpty()) {
        return false;
    }
    QString errorMsg;
    int errorLine;
    int errorCol;
    bool nsProcess = false;
    bool rc = symbolDocument.setContent(qba, nsProcess, &errorMsg, &errorLine, &errorCol);
    if (!rc) {
        //invalid SVG message
        Base::Console().Warning("DrawViewSymbol - %s - SVG for Symbol is not valid. See log.\n",
                                getNameInDocument());
        Base::Console().Log("DrawViewSymbol - %s - len: %d rc: %d error: %s line: %d col: %d\n",
                            getNameInDocument(), strlen(symbol), rc, qPrintable(errorMsg),
                            errorLine, errorCol);
    }
    return rc;
}

PyObject* DrawViewSymbol::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DrawViewSymbolPy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}

// Python Drawing feature ---------------------------------------------------------

namespace App
{
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TechDraw::DrawViewSymbolPython, TechDraw::DrawViewSymbol)
template<> const char* TechDraw::DrawViewSymbolPython::getViewProviderName() const
{
    return "TechDrawGui::ViewProviderSymbol";
}
/// @endcond

// explicit template instantiation
template class TechDrawExport FeaturePythonT<TechDraw::DrawViewSymbol>;
}// namespace App
