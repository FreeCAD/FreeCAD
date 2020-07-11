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
#include <QDomDocument>
#endif

#include <iomanip>
#include <iterator>

#include <boost/regex.hpp>

#include <QXmlQuery>
#include <QXmlResultItems>

#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Console.h>
#include <Base/Tools.h>

#include "QDomNodeModel.h"
#include "DrawUtil.h"
#include "DrawPage.h"
#include "DrawViewSymbol.h"

#include <Mod/TechDraw/App/DrawViewSymbolPy.h>  // generated from DrawViewSymbolPy.xml

using namespace TechDraw;
using namespace std;


//===========================================================================
// DrawViewSymbol
//===========================================================================

PROPERTY_SOURCE(TechDraw::DrawViewSymbol, TechDraw::DrawView)


DrawViewSymbol::DrawViewSymbol(void)
{
    static const char *vgroup = "Drawing view";

    ADD_PROPERTY_TYPE(Symbol,(""),vgroup,App::Prop_None,"The SVG code defining this symbol");
    ADD_PROPERTY_TYPE(EditableTexts,(""),vgroup,App::Prop_None,"Substitution values for the editable strings in this symbol");
    ScaleType.setValue("Custom");
    Symbol.setStatus(App::Property::Hidden,true);
}

DrawViewSymbol::~DrawViewSymbol()
{
}

void DrawViewSymbol::onChanged(const App::Property* prop)
{
//    Base::Console().Message("DVS::onChanged(%s) \n",prop->getName());
    if (prop == &Symbol) {
        if (!isRestoring() && Symbol.getValue()[0]) {
            //this pulls the initial values from svg into editabletexts
            // should only happen first time?? extra loop onChanged->execute->onChanged

            std::vector<string> editables;
            QDomDocument symbolDocument;

            const char* symbol = Symbol.getValue();
            QByteArray qba(symbol);
            QString errorMsg;
            int errorLine;
            int errorCol;
            bool nsProcess = false;
            bool rc = symbolDocument.setContent(qba, nsProcess, &errorMsg, &errorLine, &errorCol);
            if (rc) {
                QDomElement symbolDocElem = symbolDocument.documentElement();

                QXmlQuery query(QXmlQuery::XQuery10);
                QDomNodeModel model(query.namePool(), symbolDocument);
                query.setFocus(QXmlItem(model.fromDomNode(symbolDocElem)));

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
                    QDomElement tspanElement = model.toDomNode(queryResult.current().toNodeModelIndex()).toElement();
                    editables.push_back(Base::Tools::escapedUnicodeFromUtf8(tspanElement.text().toStdString().c_str()));
                }
            }
            else {
                Base::Console().Warning("DVS::onChanged - %s - SVG for Symbol is not valid. See log.\n");
                Base::Console().Log(
                    "Warning: DVS::onChanged(Symbol) for %s - len: %d rc: %d error: %s line: %d col: %d\n",
                                        getNameInDocument(), strlen(symbol), rc, 
                                        qPrintable(errorMsg), errorLine, errorCol);


            }

            EditableTexts.setValues(editables);
//            requestPaint();
        }
    }

    TechDraw::DrawView::onChanged(prop);
}

App::DocumentObjectExecReturn *DrawViewSymbol::execute(void)
{
//    Base::Console().Message("DVS::execute() \n");
//    //dvs::execute is pretty fast. doesn't need to be blocked?
//    if (!keepUpdated()) {
//        return App::DocumentObject::StdReturn;
//    }

    std::string svg = Symbol.getValue();
    if (svg.empty()) {
        return App::DocumentObject::StdReturn;
    }

    const std::vector<std::string>& editText = EditableTexts.getValues();

    if (!editText.empty()) {
        QDomDocument symbolDocument;
        const char* symbol = Symbol.getValue();
        QByteArray qba(symbol);
        QString errorMsg;
        int errorLine;
        int errorCol;
        bool nsProcess = false;
        bool rc = symbolDocument.setContent(qba, nsProcess, &errorMsg, &errorLine, &errorCol);
        if (rc) {
            QDomElement symbolDocElem = symbolDocument.documentElement();

            QXmlQuery query(QXmlQuery::XQuery10);
            QDomNodeModel model(query.namePool(), symbolDocument);
            query.setFocus(QXmlItem(model.fromDomNode(symbolDocElem)));

            // XPath query to select all <tspan> nodes whose <text> parent
            // has "freecad:editable" attribute
            query.setQuery(QString::fromUtf8(
                "declare default element namespace \"" SVG_NS_URI "\"; "
                "declare namespace freecad=\"" FREECAD_SVG_NS_URI "\"; "
                "//text[@freecad:editable]/tspan"));

            QXmlResultItems queryResult;
            query.evaluateTo(&queryResult);

            unsigned int count = 0;
            while (!queryResult.next().isNull())
            {
                QDomElement tspanElement = model.toDomNode(queryResult.current().toNodeModelIndex()).toElement();

                // Keep all spaces in the text node
                tspanElement.setAttribute(QString::fromUtf8("xml:space"), QString::fromUtf8("preserve"));

                // Remove all child nodes (if any)
                while (!tspanElement.lastChild().isNull()) {
                    tspanElement.removeChild(tspanElement.lastChild());
                }

                // Finally append text node with editable replacement as the only <tspan> descendant
                tspanElement.appendChild(symbolDocument.createTextNode(
                                 QString::fromUtf8(Base::Tools::escapedUnicodeToUtf8(editText[count]).c_str())));
                ++count;
            }

            Symbol.setValue(symbolDocument.toString(1).toStdString());
        }
        else {
            Base::Console().Warning("DVS::execute - %s - SVG for Symbol is not valid. See log.\n");
            Base::Console().Log(
                    "Warning: DVS::execute() - %s - len: %d rc: %d error: %s line: %d col: %d\n",
                                        getNameInDocument(), strlen(symbol), rc, 
                                        qPrintable(errorMsg), errorLine, errorCol);
       }
    }

//    requestPaint();
    return DrawView::execute();
}

QRectF DrawViewSymbol::getRect() const
{
        double w = 64.0;         //must default to something
        double h = 64.0;
        return (QRectF(0,0,w,h));
//        std::string svg = Symbol.getValue();
//        string::const_iterator begin, end;
//        begin = svg.begin();
//        end = svg.end();
//        boost::match_results<std::string::const_iterator> what;

//        boost::regex e1 ("width=\"([0-9.]*?)[a-zA-Z]*?\"");
//        if (boost::regex_search(begin, end, what, e1)) {
//            //std::string wText = what[0].str();             //this is the whole match 'width="100"'
//            std::string wNum  = what[1].str();               //this is just the number 100
//            w = std::stod(wNum);
//        }
//        
//        boost::regex e2 ("height=\"([0-9.]*?)[a-zA-Z]*?\"");
//        if (boost::regex_search(begin, end, what, e2)) {
//            //std::string hText = what[0].str();
//            std::string hNum  = what[1].str();
//            h = std::stod(hNum);
//        }
//        return (QRectF(0,0,getScale() * w,getScale() * h));
//we now have a w x h, but we don't really know what it means - px,mm,in,...
        
}

//!Assume all svg files fit the page and/or the user will scale manually
//see getRect() above
bool DrawViewSymbol::checkFit(TechDraw::DrawPage* p) const
{
    (void)p;
    bool result = true;
    return result;
}

short DrawViewSymbol::mustExecute() const
{
    short result = 0;
    if (!isRestoring()) {
        result  =  (Scale.isTouched()  ||
                    EditableTexts.isTouched());
    }
    if ((bool) result) {
        return result;
    }
    return DrawView::mustExecute();
}


PyObject *DrawViewSymbol::getPyObject(void)
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DrawViewSymbolPy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}

// Python Drawing feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TechDraw::DrawViewSymbolPython, TechDraw::DrawViewSymbol)
template<> const char* TechDraw::DrawViewSymbolPython::getViewProviderName(void) const {
    return "TechDrawGui::ViewProviderSymbol";
}
/// @endcond

// explicit template instantiation
template class TechDrawExport FeaturePythonT<TechDraw::DrawViewSymbol>;
}
