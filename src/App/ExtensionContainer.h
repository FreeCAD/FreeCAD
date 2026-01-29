// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2016 Stefan Tröger <stefantroeger@gmx.net>              *
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


#pragma once

#include <vector>
#include <string>
#include <map>

#include "PropertyContainer.h"


namespace App
{

class Extension;

/**
 * @brief A container that can hold extensions.
 * @ingroup ExtensionFramework
 *
 * This class provides a container for extensions.  It is used to hold
 * extensions of a document object.  For a more high-level discussion of
 * extensions see the topic @ref ExtensionFramework "Extension Framework".
 */
class AppExport ExtensionContainer: public App::PropertyContainer
{

    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    /// A type alias for iterating extensions.
    using ExtensionIterator = std::map<Base::Type, App::Extension*>::iterator;

    /// Construct an extension container.
    ExtensionContainer();
    /// Destruct an extension container.
    ~ExtensionContainer() override;

    /**
     * @brief Register an extension.
     *
     * @param[in] extension The type of the extension to register.
     * @param[in] ext The extension to register.
     */
    void registerExtension(Base::Type extension, App::Extension* ext);

    /**
     * @brief Whether this container has an extension of the given type.
     *
     * This version checks for derived types.  It returns the first extension
     * of the given type or the first extension that is derived from the type
     * if @p derived is true.
     *
     * @param[in] type The type of the extension to check for.
     * @param[in] derived (Optionally) Whether to check for derived types, true by default.
     *
     * @return True if the container has the (derived) extension, false otherwise.
     */
    bool hasExtension(Base::Type type, bool derived = true) const;

    /**
     * @brief Whether this container has an extension of the given type.
     *
     * This version checks for derived types.  It returns the first extension
     * of the given type.  Note that this function does not check for derived
     * types.
     *
     * @param[in] name The name of the extension to check for.
     * @return True if the container has the extension, false otherwise.
     */
    bool hasExtension(const std::string& name) const;

    /**
     * @brief Whether this container has extensions.
     *
     * @return True if the container has extensions, false otherwise.
     */
    bool hasExtensions() const;

    /**
     * @brief Get the extension of the given type.
     *
     * This version checks for derived types.  It returns the first extension
     * of the given type or the first extension that is derived from the type
     * if @p derived is true.  If @p no_except is true, it returns nullptr,
     *otherwise it throws an exception if no extension of the given type is
     * found.
     *
     * @param[in] type The type of the extension to get.
     * @param[in] derived (Optionally) Whether to check for derived types, true by default.
     * @param[in] no_except (Optionally) Whether to throw an exception if no extension is found,
     * false by default.
     *
     * @return The extension of the given type or `nullptr` if not found.
     * @throws Base::TypeError if no extension of the given type is found and
     * @p no_except is false.
     */
    App::Extension* getExtension(Base::Type type, bool derived = true, bool no_except = false) const;

    /**
     * @brief Get the extension with the given name.
     *
     * This version does not check for derived types.
     *
     * @param[in] name The name of the extension to get.
     *
     * @return The extension with the given name or `nullptr` if not found.
     */
    App::Extension* getExtension(const std::string& name) const;

    /**
     * @brief Get the extension of the given type.
     *
     * This version checks for derived types.  It returns the first extension
     * of the given type or the first extension that is derived from the type.
     * It doesn't throw an exception but returns `nullptr` if not found.
     *
     * @tparam ExtensionT The type of the extension to get.
     * @return The extension of the given type or `nullptr` if not found.
     */
    template<typename ExtensionT>
    ExtensionT* getExtension() const
    {
        return static_cast<ExtensionT*>(
            getExtension(ExtensionT::getExtensionClassTypeId(), true, true));
    }

    /**
     * @brief Get the extension with the given type.
     *
     *  This version checks for derived types if @p derived is true.  It returns the first
     * extension of the given type or the first extension that is derived from the type
     * if @p derived is true.  If not found, it returns `nullptr` if @p no_except is true,
     * otherwise it throws an exception.
     *
     * @tparam ExtensionT The type of the extension to get.
     *
     * @param[in] no_except (Optionally) Whether to throw an exception if no extension is found,
     * false by default.
     * @param[in] derived (Optionally) Whether to check for derived types, true by default.
     *
     * @return The extension of the given type or `nullptr` if not found if @p no_except is true.
     * @throws Base::TypeError if no extension of the given type is found and
     * @p no_except is false.
     */
    template<typename ExtensionT>
    ExtensionT* getExtensionByType(bool no_except = false, bool derived = true) const
    {
        return static_cast<ExtensionT*>(
            getExtension(ExtensionT::getExtensionClassTypeId(), derived, no_except));
    }

    /**
     * @brief Get all extensions with the given type as base class.
     *
     * @param[in] type The type of the extension to get.
     * @return A vector of extensions of the given type.
     */
    std::vector<Extension*> getExtensionsDerivedFrom(Base::Type type) const;

    /**
     * @brief Get all extensions with the given type as base class.
     *
     * @tparam ExtensionT The type of the extension to get.
     * @return A vector of extensions of the given type.
     */
    template<typename ExtensionT>
    std::vector<ExtensionT*> getExtensionsDerivedFromType() const
    {
        std::vector<ExtensionT*> typevec;
        for (const auto& entry : _extensions) {
            if (entry.first.isDerivedFrom(ExtensionT::getExtensionClassTypeId())) {
                typevec.push_back(static_cast<ExtensionT*>(entry.second));
            }
        }
        return typevec;
    }

    /**
     * @brief Get the begin iterator for the extensions.
     *
     * @return The begin iterator for the extensions.
     */
    ExtensionIterator extensionBegin()
    {
        return _extensions.begin();
    }

    /**
     * @brief Get the end iterator for the extensions.
     *
     * @return The end iterator for the extensions.
     */
    ExtensionIterator extensionEnd()
    {
        return _extensions.end();
    }


    /** @name Access properties
     *
     * @{
     */

    Property* getPropertyByName(const char* name) const override;

    /**
     * @brief Find a property by its name and cast it to the specified type.
     *
     * This method finds a property by its name and casts it to the specified
     * type with a `freecad_cast`.
     *
     * @tparam T The property type to which the property is cast.
     * @param[in] name The name of the property to find.
     * @return The property if found cast to the specified type, or `nullptr` if not found.
     */
    template<typename T>
    T* getPropertyByName(const char* name) const {
        return freecad_cast<T*>(this->getPropertyByName(name));
    }

    const char* getPropertyName(const Property* prop) const override;

    void getPropertyMap(std::map<std::string, Property*>& Map) const override;

    void visitProperties(const std::function<void(Property*)>& visitor) const override;

    void getPropertyList(std::vector<Property*>& List) const override;

    short getPropertyType(const Property* prop) const override;

    short getPropertyType(const char* name) const override;

    const char* getPropertyGroup(const Property* prop) const override;

    const char* getPropertyGroup(const char* name) const override;

    const char* getPropertyDocumentation(const Property* prop) const override;

    const char* getPropertyDocumentation(const char* name) const override;
    ///@}

    void onChanged(const Property*) override;

    void Save(Base::Writer& writer) const override;

    void Restore(Base::XMLReader& reader) override;

    // those methods save/restore the dynamic extensions without handling properties, which is
    // something done by the default Save/Restore methods.
    /**
     * @brief Save the extensions to the given writer.
     *
     * This method saves the dynamic extensions to the given writer without
     * handling properties, which is something done by the default Save/Restore
     * methods.
     *
     * @param[in,out] writer The writer to save the extensions to.
     */
    void saveExtensions(Base::Writer& writer) const;

    /**
     * @brief Restore the extensions from the given reader.
     *
     * This method restores the dynamic extensions from the given reader
     * without handling properties, which is something done by the default
     * Save/Restore methods.
     *
     * @param[in,out] reader The reader to restore the extensions from.
     */
    void restoreExtensions(Base::XMLReader& reader);

    /**
     * @brief Handle a changed property name during restore.
     *
     * This method extends the rules for handling property name changed, so
     * that extensions are given an opportunity to handle it. If an extension
     * handles a change, neither the rest of the extensions, nor the container
     * itself get to handle it.
     *
     * Extensions get their Extension::extensionHandleChangedPropertyName()
     * called.
     *
     * If no extension handles the request, then
     * PropertyContainer::handleChangedPropertyName() is called.
     *
     * @param[in,out] reader The reader to restore the extensions from.
     * @param[in] TypeName The name of the type of the property to handle.
     * @param[in] PropName The name of the property to handle.
     */
    void handleChangedPropertyName(Base::XMLReader& reader,
                                   const char* TypeName,
                                   const char* PropName) override;

    /**
     * @brief Handle a changed property type during restore.
     *
     * This method extends the rules for handling property type changed, so
     * that extensions are given an opportunity to handle it.  If an extension
     * handles a change, neither the rest of the extensions, nor the container
     * itself get to handle it.
     *
     *  Extensions get their Extension::extensionHandleChangedPropertyType() called.
     *
     *  If no extension handles the request, then
     *  PropertyContainer::handleChangedPropertyType() is called.
     *
     * @param[in,out] reader The reader to restore the extensions from.
     * @param[in] TypeName The name of the type of the property to handle.
     * @param[in] prop The property that needs to be restored.  Its type differs from `TypeName`.
     */
    void handleChangedPropertyType(Base::XMLReader& reader,
                                   const char* TypeName,
                                   Property* prop) override;

private:
    // stored extensions
    std::map<Base::Type, App::Extension*> _extensions;
};

#define PROPERTY_HEADER_WITH_EXTENSIONS(_class_) PROPERTY_HEADER_WITH_OVERRIDE(_class_)

/// We make sure that the PropertyData of the container is not connected to the one of the extension
#define PROPERTY_SOURCE_WITH_EXTENSIONS(_class_, _parentclass_)                                    \
    PROPERTY_SOURCE(_class_, _parentclass_)

#define PROPERTY_SOURCE_ABSTRACT_WITH_EXTENSIONS(_class_, _parentclass_)                           \
    PROPERTY_SOURCE_ABSTRACT(_class_, _parentclass_)

}  // namespace App
