/****************************************************************************
 *   Copyright (c) 2024 Ondsel <development@ondsel.com>                     *
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

#ifndef APP_VARSET_H
#define APP_VARSET_H

#include "DocumentObject.h"
#include "PropertyVarSet.h"


namespace App
{

/**
 * @brief A DocumentObject class with the purpose to store variables.
 *
 * The VarSet is a DocumentObject in core that behaves in a special manner when
 * its parent is an App::Part.  It will enable a boolean property Expose that
 * will create a PropertyVarSet (an external link) that points to this VarSet.
 */
class AppExport VarSet : public App::DocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(App::VarSet);
	
public:

    /**
     * @brief Get the name of the property in the parent pointing to the
     * original VarSet.
     *
     * Given a name for the PropertyVarSet of the parent that points to a
     * VarSet, get the name of the property in the parent that points to the
     * original VarSet.
     *
     * @param name The name of the property that points to a VarSet
     * @return The name of the property that points to the original VarSet
     */
    static std::string getNameOriginalVarSetProperty(const char* name);
    

    /**
     * @brief Create a VarSet that is not exposed.
     */
    VarSet();
    ~VarSet() override = default;

    void onBeforeChange(const Property* prop) override;
    void onChanged(const Property* prop) override;
    void onDocumentRestored() override;
    
    const char* getViewProviderName() const override;

    /**
     * @brief Acquire the VarSet of the parent.
     *
     * Returns the VarSet the parent is pointing to.  This can be this VarSet
     * but it can also be an equivalent VarSet that replaces this VarSet.  If
     * the VarSet is not part of an App::Part or if it is not exposed, it
     * returns nullptr.
     *
     * @return The VarSet the parent is pointing to if exposed or nullptr
     * otherwise.
     */
    VarSet* getParentVarSet();

    /**
     * @brief Enable the PropertyBool Exposed.
     *
     * Exposed should only be enabled if the parent is an App::Part.
     */
    void enableExposed();

    /**
     * @brief Disable the PropertyBool Exposed.
     *
     * Exposed should only be enabled if the parent is an App::Part.
     */
    void disableExposed();

    /**
     * @brief Whether this VarSet is exposed.
     *
     * @return true if the VarSet is exposed.
     */
    bool isExposed() const;

    /**
     * @brief Whether this VarSet is equivalent to varSet.
     *
     * VarSets are equivalent if they have the same set of properties with
     * similar types.
     *
     * @param varSet The VarSet to check equivalence with.
     * @return true if varSet is equivalent to this VarSet.
     */
    bool isEquivalent(VarSet* other) const;

private:
    PropertyBool Exposed;
    PropertyVarSet ReplacedBy;
    PropertyString NamePropertyParent;
    
    void exposedChanged();

    const char* getNameVarSetPropertyParent();
    void createVarSetPropertiesParent(DocumentObject* varSet);
    void createVarSetPropertyParent(DocumentObject* parent, const char* name,
                                    std::string& doc, DocumentObject* value,
                                    bool hidden = false, bool copyOnChange = false);
    void addVarSetPropertiesParent();
    void removeVarSetPropertiesParent();
    void renameVarSetPropertiesParent(const char *oldName);

    const char* previousNameParentProperty;

    /** @brief Get the parent of a VarSet that is exposed
     *
     * Get the parent of a VarSet that is exposed.  If there is no such parent or
     * the parent is not a Part, disable the boolean flag Exposed.
     *
     * @return the parent document object of the exposed VarSet or nullptr otherwise
     */
    DocumentObject* getParentExposed();
    
    }; 
}
#endif
