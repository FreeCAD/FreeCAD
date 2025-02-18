/***************************************************************************
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
# include <QCollator>
# include <QDateTime>
#endif

#include <Base/Console.h>

#include <App/Application.h>
#include <App/Document.h>

#include "DrawTemplate.h"
#include "DrawTemplatePy.h"
#include "DrawPage.h"
#include "DrawUtil.h"
#include "Preferences.h"


using namespace TechDraw;

PROPERTY_SOURCE(TechDraw::DrawTemplate, App::DocumentObject)


const char* DrawTemplate::OrientationEnums[]= {"Portrait",
                                                  "Landscape",
                                                  nullptr};

DrawTemplate::DrawTemplate()
{
    const char *group = "Page Properties";

    Orientation.setEnums(OrientationEnums);
    ADD_PROPERTY(Orientation, (0l));

    // Physical Properties inherent to every template class
    ADD_PROPERTY_TYPE(Width,     (0),  group, App::PropertyType::Prop_None, "Width of page");
    ADD_PROPERTY_TYPE(Height,    (0),  group, App::PropertyType::Prop_None, "Height of page");

    ADD_PROPERTY_TYPE(EditableTexts, (), group, App::PropertyType::Prop_None,
                      "Editable strings in the template");
}

DrawTemplate::~DrawTemplate()
{
}

PyObject *DrawTemplate::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DrawTemplatePy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}

double DrawTemplate::getWidth() const
{
    return Width.getValue();
}

double DrawTemplate::getHeight() const
{
    return Height.getValue();
}

//find the (first) DrawPage which points to this template
DrawPage* DrawTemplate::getParentPage() const
{
    TechDraw::DrawPage* page(nullptr);
    std::vector<App::DocumentObject*> parents = getInList();
    for (auto& obj : parents) {
        if (obj->isDerivedFrom<DrawPage>()) {
            page = static_cast<TechDraw::DrawPage *>(obj);
            break;
        }
    }
    return page;
}

// Return the counts related to pages, namely collated page index and total page count
std::pair<int, int> DrawTemplate::getPageNumbers() const
{
    std::vector<DocumentObject *> pages = getDocument()->getObjectsOfType(TechDraw::DrawPage::getClassTypeId());
    std::vector<QString> pageNames;
    for (auto page : pages) {
        if (page->isAttachedToDocument() &&
            !page->testStatus(App::ObjectStatus::Remove)) {
            pageNames.push_back(QString::fromUtf8(page->Label.getValue()));
        }
    }
    QCollator collator;
    std::sort(pageNames.begin(), pageNames.end(), collator);

    int pos = 0;
    DrawPage *page = getParentPage();
    if (page) {
        auto it = std::find(pageNames.begin(), pageNames.end(), QString::fromUtf8(page->Label.getValue()));
        if (it != pageNames.end()) {
            pos = it - pageNames.begin() + 1;
        }
    }

    return std::pair<int, int>(pos, (int) pageNames.size());
}

//! get replacement values from document
QString DrawTemplate::getAutofillValue(const QString &id) const
{
    constexpr int ISODATELENGTH {10};
    auto doc = getDocument();
    if (!doc) {
        return QString();
    }
    // author
    if (id.compare(QString::fromUtf8(Autofill::Author)) == 0) {
        auto value = QString::fromUtf8(doc->CreatedBy.getValue());
        if (!value.isEmpty()) {
            return value;
        }
    }
    // date
    else if (id.compare(QString::fromUtf8(Autofill::Date)) == 0) {
        auto timeLocale = std::setlocale(LC_TIME, nullptr);
        QDateTime date = QDateTime::currentDateTime();
        if (Preferences::enforceISODate()) {
            auto rawDate = date.toString(Qt::ISODate);
            return rawDate.left(ISODATELENGTH);
        }
        auto qTimeLocale = QString::fromUtf8(timeLocale);
        return date.toString(QLocale(qTimeLocale).dateFormat(QLocale::ShortFormat));
    }
    // organization ( also organisation/owner/company )
    else if (id.compare(QString::fromUtf8(Autofill::Organization)) == 0 ||
             id.compare(QString::fromUtf8(Autofill::Organisation)) == 0 ||
             id.compare(QString::fromUtf8(Autofill::Owner)) == 0 ||
             id.compare(QString::fromUtf8(Autofill::Company)) == 0 ) {
        auto value = QString::fromUtf8(doc->Company.getValue());
        if (!value.isEmpty()) {
            return value;
        }
    }
    // scale
    else if (id.compare(QString::fromUtf8(Autofill::Scale)) == 0) {
        DrawPage *page = getParentPage();
        if (page) {
            std::pair<int, int> scale = DrawUtil::nearestFraction(page->Scale.getValue());
            return QString::asprintf("%d : %d", scale.first, scale.second);
        }
    }
    // sheet
    else if (id.compare(QString::fromUtf8(Autofill::Sheet)) == 0) {
        std::pair<int, int> pageNumbers = getPageNumbers();
        return QString::asprintf("%d / %d", pageNumbers.first, pageNumbers.second);
    }
    // title
    else if (id.compare(QString::fromUtf8(Autofill::Title)) == 0) {
        return QString::fromUtf8(getDocument()->Label.getValue());
    }
    // page number
    else if (id.compare(QString::fromUtf8(Autofill::PageNumber)) == 0) {
        std::pair<int, int> pageNumbers = getPageNumbers();
        return QString::number(pageNumbers.first);
    }
    // page total
    else if (id.compare(QString::fromUtf8(Autofill::PageCount)) == 0) {
        std::pair<int, int> pageNumbers = getPageNumbers();
        return QString::number(pageNumbers.second);
    }

    return QString();
}

// Python Template feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TechDraw::DrawTemplatePython, TechDraw::DrawTemplate)
template<> const char* TechDraw::DrawTemplatePython::getViewProviderName() const {
    return "TechDrawGui::ViewProviderPython";
}
/// @endcond

// explicit template instantiation
template class TechDrawExport FeaturePythonT<TechDraw::DrawTemplate>;
}   // namespace App
