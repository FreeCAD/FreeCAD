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


# include <sstream>
# include <QCollator>
# include <QDateTime>


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
    if (const DrawPage* page = getParentPage()) {
        if (const auto it = std::ranges::find(pageNames, QString::fromUtf8(page->Label.getValue()));
            it != pageNames.end()) {
            pos = it - pageNames.begin() + 1;
        }
    }

    return std::pair<int, int>(pos, (int) pageNames.size());
}

//! get replacement values from document
std::string DrawTemplate::getAutofillValue(const std::string& id) const
{
    auto doc = getDocument();
    if (!doc || id.empty()) {
        return std::string();
    }

    // author
    if (id == Autofill::Author) {
        return doc->CreatedBy.getValue();
    }
    // date
    else if (id == Autofill::Date) {
        std::time_t now = std::time(0);
        std::tm cal = *std::localtime(&now);

        std::ostringstream oss;
        if (Preferences::enforceISODate()) {
            oss << std::put_time(&cal, "%F"); // %F format for ISO 8601 date format
            return oss.str();
        }

        oss.imbue(std::locale(""));       // Set output stream's locale to user native locale
        oss << std::put_time(&cal, "%x"); // %x format for localized date format
        return oss.str();
    }
    // organization ( also organisation/owner/company )
    else if (id == Autofill::Organization || id == Autofill::Organisation
             || id == Autofill::Owner || id == Autofill::Company) {
        return doc->Company.getValue();
    }
    // scale
    else if (id == Autofill::Scale) {
        DrawPage *page = getParentPage();
        if (page) {
            std::pair<int, int> scale = DrawUtil::nearestFraction(page->Scale.getValue());
            return (std::ostringstream() << scale.first << " : " << scale.second).str();
        }
    }
    // sheet
    else if (id == Autofill::Sheet) {
        std::pair<int, int> pageNumbers = getPageNumbers();
        return (std::ostringstream() << pageNumbers.first << " / " << pageNumbers.second).str();
    }
    // title
    else if (id == Autofill::Title) {
        return getDocument()->Label.getValue();
    }
    // page number
    else if (id == Autofill::PageNumber) {
        std::pair<int, int> pageNumbers = getPageNumbers();
        return std::to_string(pageNumbers.first);
    }
    // page total
    else if (id == Autofill::PageCount) {
        std::pair<int, int> pageNumbers = getPageNumbers();
        return std::to_string(pageNumbers.second);
    }

    return std::string();
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
