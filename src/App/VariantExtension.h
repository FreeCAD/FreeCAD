/****************************************************************************
 *   Copyright (c) 2025 Pieter Hijma <info@pieterhijma.net>                 *
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#ifndef APP_VARIANT_EXTENSION_H
#define APP_VARIANT_EXTENSION_H

#include "GroupExtension.h"

namespace App
{

class AppExport VariantExtension: public GroupExtension
{
    EXTENSION_PROPERTY_HEADER_WITH_OVERRIDE(App::VariantExtension);

public:
    VariantExtension();
    ~VariantExtension() override = default;

    PropertyXLink Support;

    DocumentObjectExecReturn* extensionExecute() override;
    void extensionOnChanged(const App::Property* prop) override;
    static Property* createPropertyContext(DocumentObject* objContext,
                                           const DocumentObject* obj,
                                           const char* name);

private:
    void executeObjects(const std::vector<DocumentObject*>& objs);
    void pushContexts(const std::vector<DocumentObject*>& objs);
    void popContexts(const std::vector<DocumentObject*>& objs);
    std::vector<DocumentObject*> getSortedDependencies(DocumentObject* support);
    DocumentObject* createNewContext(DocumentObject* obj, Document* doc);
    void arrangeContext(DocumentObject* obj,
                        Document* doc, DocumentObject* support);
    void arrangeContexts();
    void removeDynamicProperties(DocumentObject* obj) const;
    void adoptExpressionEngine(DocumentObject* objContext,
                               const DocumentObject* obj, const DocumentObject* support) const;
    std::set<std::string> getNamesPropsWithExposedExprs(const DocumentObject* obj,
                                                        const DocumentObject* support) const;
    void adoptExposedProps(const DocumentObject* support);
    void adoptProperty(const App::Property* prop);
    std::vector<DocumentObject*> getObjectsRequiringVarSets();

private:
    std::set<const Property*> adoptedExposedProps;
    std::unordered_map<const DocumentObject*, DocumentObject*> objToContext;
    std::unordered_map<const DocumentObject*, DocumentObject*> contextToObj;
};

}  // namespace App
#endif
