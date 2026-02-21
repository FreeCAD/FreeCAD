// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2005 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include <map>
#include <vector>
#include <string>
#include <memory>
#include <Base/Persistence.h>

#include "DynamicProperty.h"

namespace Base {
class Writer;
}


namespace App
{
class Property;
class PropertyContainer;
class DocumentObject;
class Extension;

// clang-format off

/**
 * @brief Flags that define special behaviors for a property.
 *
 * These flags can be combined using bitwise OR to assign multiple attributes
 * to a property.
 */
enum PropertyType
{
    /// No special property type.
    Prop_None        = 0,
    /// The property is read-only in the editor.
    Prop_ReadOnly    = 1,
    /// The property content won't be saved to file, but still saves name, type and status.
    Prop_Transient   = 2,
    /// The property is hidden in the editor.
    Prop_Hidden      = 4,
    /// A modified property doesn't touch its parent container.
    Prop_Output      = 8,
    /// A modified property doesn't touch its container for recompute.
    Prop_NoRecompute = 16,
    /// The property won't be saved to file at all.
    Prop_NoPersist   = 32,
};
// clang-format on

struct AppExport PropertyData
{
  PropertyData();
  ~PropertyData();

  /// @brief Struct to hold the property specification.
  struct PropertySpec
  {
    /// The name of the property.
    const char * Name;
    /// The group name of the property.
    const char * Group;
    /// The documentation string of the property.
    const char * Docu;
    /// The offset of the property in the container.
    short Offset;
    /// The type of the property.
    short Type;

    /**
     * @brief Construct a PropertySpec.
     *
     * @param[in] name The name of the property.
     * @param[in] group The group name of the property.
     * @param[in] doc The documentation string of the property.
     * @param[in] offset The offset of the property in the container.
     * @param[in] type The type of the property.
     */
    inline PropertySpec(const char *name, const char *group, const char *doc, short offset, short type)
        :Name(name),Group(group),Docu(doc),Offset(offset),Type(type)
    {}
  };

  //purpose of this struct is to be constructible from all acceptable container types and to
  //be able to return the offset to a property from the accepted containers. This allows you to use
  //one function implementation for multiple container types without losing all type safety by
  //accepting void*
  /**
   * @brief Struct that represents the base for offset calculation.
   *
   * This struct is used to calculate the offset of a property in a container.
   * It can be constructed from either a PropertyContainer or an Extension.
   */
  struct OffsetBase
  {
      /**
       * @brief Construct an OffsetBase from a PropertyContainer.
       *
       * @param[in] container The PropertyContainer to construct from.
       */
      // Lint wants these marked explicit, but they are currently used implicitly in enough
      // places that I don't wnt to fix it. Instead we disable the Lint message.
      // NOLINTNEXTLINE(runtime/explicit)
      OffsetBase(const App::PropertyContainer* container) : m_container(container) {}
      /**
       * @brief Construct an OffsetBase from an Extension.
       *
       * @param[in] container The Extension to construct from.
       */
      // NOLINTNEXTLINE(runtime/explicit)
      OffsetBase(const App::Extension* container) : m_container(container) {}

      /**
       * @brief Get the offset to a property.
       *
       * @param[in] prop The property to get the offset to.
       * @return The offset to the property relative to the base offset.
       */
      short int getOffsetTo(const App::Property* prop) const {
            auto *pt = (const char*)prop;
            auto *base = (const char *)m_container;
            if(pt<base || pt>base+SHRT_MAX)
                return -1;
            return (short) (pt-base);
      }
      /**
       * @brief Get the base offset in bytes.
       *
       * @return The base offset in bytes.
       */
      char* getOffset() const {return (char*) m_container;}

  private:
      const void* m_container;
  };

  /// Whether the property data is merged with the parent.
  mutable bool parentMerged = false;

  /// The parent property data.
  const PropertyData*     parentPropertyData;

  /**
   * @brief Add a property to the property data.
   *
   * @param[in] offsetBase The base offset to offset the property.
   * @param[in] PropName The name of the property.
   * @param[in] Prop The property to add.
   * @param[in] PropertyGroup The group name of the property.
   * @param[in] Type The type of the property.
   * @param[in] PropertyDocu The documentation string of the property.
   */
  void addProperty(OffsetBase offsetBase,const char* PropName, Property *Prop, const char* PropertyGroup= nullptr, PropertyType Type=Prop_None, const char* PropertyDocu=nullptr );

  /**
   * @brief Find a property by its name.
   *
   * @param[in] offsetBase The base offset for the property.
   * @param[in] PropName The name of the property to find.
   * @return The property specification if found; `nullptr` otherwise.
   */
  const PropertySpec *findProperty(OffsetBase offsetBase,const char* PropName) const;

  /**
   * @brief Find a property by its pointer.
   *
   * @param[in] offsetBase The base offset for the property.
   * @param[in] prop The property to find.
   * @return The property specification if found; `nullptr` otherwise.
   */
  const PropertySpec *findProperty(OffsetBase offsetBase,const Property* prop) const;

  /**
   * @name PropertyData accessors
   *
   * @details These methods are used to access the property data based on:
   * - the offset base,
   * - either:
   *   - a property pointer, or
   *   - a property name.
   * @{
   */
  const char* getName         (OffsetBase offsetBase,const Property* prop) const;
  short       getType         (OffsetBase offsetBase,const Property* prop) const;
  short       getType         (OffsetBase offsetBase,const char* name)     const;
  const char* getGroup        (OffsetBase offsetBase,const char* name)     const;
  const char* getGroup        (OffsetBase offsetBase,const Property* prop) const;
  const char* getDocumentation(OffsetBase offsetBase,const char* name)     const;
  const char* getDocumentation(OffsetBase offsetBase,const Property* prop) const;
  /// @}

  /**
   * @brief Get a property by its name.
   *
   * @param[in] offsetBase The base offset for the property.
   * @param[in] name The name of the property to find.
   * @return The property if found; `nullptr` otherwise.
   */
  Property *getPropertyByName(OffsetBase offsetBase,const char* name) const;

  /**
   * @brief Get a map of properties.
   *
   * @param[in] offsetBase The base offset for the property.
   * @param[out] Map A map of property names to properties.
   */
  void getPropertyMap(OffsetBase offsetBase,std::map<std::string,Property*> &Map) const;

  /**
   * @brief Get a list of properties.
   *
   * @param[in] offsetBase The base offset for the property.
   * @param[out] List A vector of properties.
   */
  void getPropertyList(OffsetBase offsetBase,std::vector<Property*> &List) const;

  /**
   * @brief Get a list of properties with their names.
   *
   * @param[in] offsetBase The base offset for the property.
   * @param[out] List A vector of pairs, where each pair contains the name and
   * the property.
   */
  void getPropertyNamedList(OffsetBase offsetBase, std::vector<std::pair<const char*,Property*> > &List) const;

  /**
   * @brief Visit each property in the PropertyData struct.
   *
   * @param[in] offsetBase The base offset for the property.
   * @param[in] visitor The function to apply to each property.
   *
   * @see PropertyContainer::visitProperties()
   */
  void visitProperties(OffsetBase offsetBase, const std::function<void(Property*)>& visitor) const;

  /**
   * @brief Merge the PropertyData structs.
   *
   * Merge two PropertyData structs.  If `other` is `nullptr`, merge with the
   * parent PropertyData.
   *
   * @param[in] other (Optional) The other PropertyData to merge with;
   */
  void merge(PropertyData *other=nullptr) const;

  /**
   * @brief Split the PropertyData structs.
   *
   * This method splits the PropertyData structs.  It is used to
   * restore the parent PropertyData after a merge.
   *
   * @param[in] other The other PropertyData to split with; this can be the parent PropertyData.
   */
  void split(PropertyData *other);

private:
  struct Impl;
  std::unique_ptr<Impl> impl;
};


/** @brief %Base class of all classes with properties.
 * @ingroup PropertyFramework
 *
 * @details This base class for all classes that hold properties has
 * essentially two important children: Document and DocumentObject.  Both
 * classes can hold properties and the shared functionality to make that happen
 * is in this class.
 *
 * For a more high-level overview see topic @ref PropertyFramework "Property Framework".
 */
class AppExport PropertyContainer: public Base::Persistence
{

  TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
  /**
   * @brief Construct a property container.
   */
  PropertyContainer();

  /**
   * @brief Destruct a property container.
   */
  ~PropertyContainer() override;

  unsigned int getMemSize () const override;

  /**
   * @brief Get the full name of the property container.
   *
   * The full name typically also contains the name of the document.
   *
   * @return The full name of the property container.
   */
  virtual std::string getFullName() const {return {};}

  /**
   * @brief Find a property by its name.
   *
   * @param[in] name The name of the property to find.
   * @return The property if found or `nullptr` when not found.
   */
  virtual Property *getPropertyByName(const char* name) const;

  /**
   * @brief Get the name of a property.
   *
   * @param[in] prop The property to get the name for.
   * @return The name of the property.
   */
  virtual const char* getPropertyName(const Property* prop) const;

  /**
   * Get all properties of the property container.
   *
   * This includes the properties of the parent container.
   *
   * @param[out] map A map of property names to properties.
   */
  virtual void getPropertyMap(std::map<std::string,Property*> &map) const;

  /**
   * Get all properties of the property container.
   *
   * This includes the properties of the parent container.
   *
   * @param[out] List A vector of properties.
   */
  virtual void getPropertyList(std::vector<Property*> &List) const;

  /**
   * @brief Visit each property in the container.
   *
   * This method allows you to apply a function to each property in the
   * container. The visiting order is undefined. This method is necessary
   * because PropertyContainer has no begin and end methods and it is not
   * practical to implement these. What gets visited is undefined if the
   * collection of Properties is changed during this call.
   *
   * @param[in] visitor The function to apply to each property.
   */
  virtual void visitProperties(const std::function<void(Property*)>& visitor) const;

  /**
   * @brief Get a list of properties with their names.
   *
   * The list may contain duplicates and aliases.
   *
   * @param[out] list A vector of pairs, where each pair contains the name and
   * the property.
   */
  virtual void getPropertyNamedList(std::vector<std::pair<const char*,Property*> > &list) const;

  /**
   * @brief Set a status bit for all properties.
   *
   * This method sets a status bit for all properties in the container at once.
   * The status bits are defined in enum Property::Status.
   *
   * @param[in] bit The status bit to set.
   * @param[in] value The value to set the status bit to.
   */
  void setPropertyStatus(unsigned char bit,bool value);

  /**
   * @brief Get the type of a property given a property.
   *
   * This method returns the type as a bitmask of the PropertyType enum.
   *
   * @param[in] prop The property to get the type for.
   * @return The type as a bitmask of the PropertyType enum.
   */
  virtual short getPropertyType(const Property* prop) const;

  /**
   * @brief Get the type of a property given a property name.
   *
   * This method returns the type as a bitmask of the PropertyType enum.
   *
   * @param[in] name The name of the property to get the type for.
   * @return The type as a bitmask of the PropertyType enum.
   */
  virtual short getPropertyType(const char *name) const;

  /**
   * @brief Get the group of a property given a property.
   *
   * @param[in] prop The property to get the group for.
   * @return The group name of the property.
   */
  virtual const char* getPropertyGroup(const Property* prop) const;

  /**
   * @brief Get the group of a property given a property name.
   *
   * @param[in] name The name of the property to get the group for.
   * @return The group name of the property.
   */
  virtual const char* getPropertyGroup(const char *name) const;

  /**
   * @brief Get the documentation of a property given a property.
   *
   * @param[in] prop The property to get the documentation for.
   * @return The documentation string of the property.
   */
  virtual const char* getPropertyDocumentation(const Property* prop) const;

  /**
   * @brief Get the documentation of a property given a property name.
   *
   * @param[in] name The name of the property to get the documentation for.
   * @return The documentation string of the property.
   */
  virtual const char* getPropertyDocumentation(const char *name) const;

  /**
   * @brief Check if a property is read-only given a property.
   *
   * @param[in] prop The property to check.
   * @return `true` if the property is read-only; `false` otherwise.
   */
  bool isReadOnly(const Property* prop) const;

  /**
   * @brief Check if a property is read-only given a property name.
   *
   * @param[in] name The name of the property to check.
   * @return `true` if the property is read-only; `false` otherwise.
   */
  bool isReadOnly(const char *name) const;

  /**
   * @brief Check if a property is hidden given a property.
   *
   * @param[in] prop The property to check.
   * @return `true` if the property is hidden; `false` otherwise.
   */
  bool isHidden(const Property* prop) const;

  /**
   * @brief Check if a property is hidden given a property name.
   *
   * @param[in] name The name of the property to check.
   * @return `true` if the property is hidden; `false` otherwise.
   */
  bool isHidden(const char *name) const;

  /**
   * Add a property at runtime.
   *
   * Dynamic properties are properties that are not defined at compile-time
   * but can be added and removed during the lifetime of the object.
   *
   * @param[in] type The type name of the property (e.g., "App::PropertyFloat").
   * @param[in] name Optional name of the property. If null, a default name may be generated.
   * @param[in] group Optional group name to which the property belongs
   * (used for UI or logical grouping).
   * @param[in] doc Optional documentation string describing the purpose of the property.
   * @param[in] attr Bitmask of property attributes, composed of values from the
   * PropertyType enum.
   * @param[in] ro If true, the property is marked as read-only.
   * @param[in] hidden If true, the property is hidden from the user interface.
   * @return A pointer to the newly added Property.
   * @throws Exception A runtime exception is thrown if the property cannot be added, such
   * as when the name is invalid.
   */
  virtual App::Property* addDynamicProperty(
        const char* type, const char* name=nullptr,
        const char* group=nullptr, const char* doc=nullptr,
        short attr=0, bool ro=false, bool hidden=false);

  /**
   * @brief Get the data of a dynamic property.
   *
   * This function retrieves the data associated with a dynamic property.
   *
   * @param[in] prop The property to get the data for.
   * @returns The data of the dynamic property.
   */
  DynamicProperty::PropData getDynamicPropertyData(const Property* prop) const {
      return dynamicProps.getDynamicPropertyData(prop);
  }

  /**
   * Change the group and documentation of a dynamic property.
   *
   * @param[in] prop The property to change.
   * @param[in] group The new group name for organizational purposes, (e.g., in UI panels).
   * @param[in] doc The new documentation string for this property.
   * @return `true` if the update was successful; `false` otherwise.
   */
  bool changeDynamicProperty(const Property *prop, const char *group, const char *doc) {
      return dynamicProps.changeDynamicProperty(prop,group,doc);
  }

  /**
   * @brief Rename the dynamic property.
   *
   * @param[in] prop The property to rename.
   * @param[in] name The new name for the property.
   *
   * @return `true` if the property was renamed; `false` otherwise.
   * @throw Base::NameError If the new name is invalid or already exists.
   */
  virtual bool renameDynamicProperty(Property *prop, const char *name) {
      return dynamicProps.renameDynamicProperty(prop,name);
  }

  /**
   * @brief Remove a dynamic property.
   *
   * @param[in] name The name of the property to remove.
   * @return `true` if the property was removed; `false` otherwise.
   */
  virtual bool removeDynamicProperty(const char* name) {
      return dynamicProps.removeDynamicProperty(name);
  }

  /**
   * @brief Get the names of all dynamic properties.
   *
   * @returns A vector of strings containing the names of all dynamic properties.
   */
  virtual std::vector<std::string> getDynamicPropertyNames() const {
      return dynamicProps.getDynamicPropertyNames();
  }

  /**
   * @brief Get a dynamic property.
   *
   * @param[in] name The name of the property.
   * @returns The property if found or `nullptr` when not found.
   */
  virtual App::Property *getDynamicPropertyByName(const char* name) const {
      return dynamicProps.getDynamicPropertyByName(name);
  }

  /**
   * @brief Called when the status of a property is changed.
   *
   * This method is called when the status of a property changes.  It can be
   * overridden by subclasses to implement custom behavior when a property
   * status changes.
   *
   * @param[in] prop The property whose status has changed.
   * @param[in] oldStatus The old status of the property as a bitmask of enum
   * Property::Status.
   */
  virtual void onPropertyStatusChanged(const Property &prop, unsigned long oldStatus);

  void Save (Base::Writer &writer) const override;
  void Restore(Base::XMLReader &reader) override;

  /**
   * @brief Prepare the properties for saving.
   *
   * All non-transient properties are prepared to be saved.
   *
   * @see Property::beforeSave()
   */
  virtual void beforeSave() const;

  /**
   * @brief Called when a property is edited by the user.
   *
   * Subclasses can override this method to implement custom behavior for
   * editing specific properties.
   *
   * @param[in] propName The name of the property to be edited.
   */
  virtual void editProperty([[maybe_unused]] const char* propName) {}

  /**
   * @brief Get the prefix for property names.
   *
   * @return The prefix for property names.
   */
  const char *getPropertyPrefix() const {
      return _propertyPrefix.c_str();
  }

  /**
   * @brief Set the prefix for property names.
   *
   * @param[in] prefix The new prefix for property names.
   */
  void setPropertyPrefix(const char *prefix) {
      _propertyPrefix = prefix;
  }

  friend class Property;
  friend class DynamicProperty;


protected:
  /**
   * @brief Called when a property is about to change.
   *
   * This method can be overridden by subclasses to implement custom behavior
   * when a property is about to change.  This method is called in
   * Property::touch() right before onChanged() is called, typically in the
   * DocumentObject::execute() phase.
   *
   * @note This can be considered a low-level callback and is not widely
   * overridden.  For preparing for a property change, use onBeforeChange().
   *
   * @param[in] prop The property that is about to change.
   * @see Property::onBeforeChange()
   */
  virtual void onEarlyChange([[maybe_unused]] const Property* prop){}

  /**
   * @brief Called when a property has changed.
   *
   * This method can be overridden by subclasses to implement custom behavior
   * when a property has changed.
   *
   * @param[in] prop The property that has changed.
   */
  virtual void onChanged([[maybe_unused]] const Property* prop){}

  /**
   * @brief Called before a property is changed.
   *
   * This method can be overridden by subclasses to implement custom behavior
   * before a property has changed.  It allows property containers to prepare
   * for a property change.
   *
   * @param[in] prop The property that is about to change.
   */
  virtual void onBeforeChange([[maybe_unused]] const Property* prop){}

  /**
   * @brief Get a pointer to the class-wide static property data.
   *
   * This method gives access to the class-wide static PropertyData shared by
   * all instances of the class.  The macros `PROPERTY_HEADER` and
   * `PROPERTY_SOURCE` variants ensure that each subclass of %PropertyContainer
   * has its own static PropertyData instance, and that this method returns the
   * instance for that subclass.
   *
   * @returns A pointer to the static `PropertyData`.
   */
  static const PropertyData*  getPropertyDataPtr();

  /**
   * @brief Get a reference to the static property data for the dynamic type of this instance.
   *
   * This virtual method allows retrieval of the class-level static
   * PropertyData associated with the actual (dynamic) type of the object, even
   * when accessed via a base class pointer. The `PROPERTY_HEADER` and
   * `PROPERTY_SOURCE` macros ensure that each subclass defines its own static
   * PropertyData instance, and that this method correctly dispatches to return
   * it.
   *
   * @return A reference to the static `PropertyData` corresponding to the dynamic type.
   */
  virtual const PropertyData& getPropertyData() const;

  /**
   * @brief Handle a changed property name during restore.
   *
   * This method is called during restore to possibly fix reading of older
   * versions of this property container.  This method is typically called if
   * the property on file has changed its name in more recent versions.
   *
   * The default implementation does nothing.
   *
   * @param[in,out] reader The XML stream to read from.
   * @param[in] typeName The name of the property type in the file.
   * @param[in] propName The name of the property in the file that does no
   * longer exist in the container.
   */
  virtual void handleChangedPropertyName(Base::XMLReader& reader, const char* typeName, const char* propName);

  /**
   * @brief Handle a changed property type during restore.
   *
   * This method is called during restore to possibly fix reading of older
   * versions of this property container.  This method is typically called if
   * the property on file has changed its type in more recent versions.
   *
   * The default implementation does nothing.
   *
   * @param[in,out] reader The XML stream to read from.
   * @param[in] typeName The name of the property type in the file.
   * @param[in, out] prop The property that needs to be restored.  Its type differs from `typeName`.
   */
  virtual void handleChangedPropertyType(Base::XMLReader &reader, const char * typeName, Property * prop);

public:

  /// The copy constructor is deleted to prevent copying.
  PropertyContainer(const PropertyContainer&) = delete;

  /// The assignment operator is deleted to prevent assignment.
  PropertyContainer& operator = (const PropertyContainer&) = delete;

protected:

  /// The container for dynamic properties.
  DynamicProperty dynamicProps;

private:
  std::string _propertyPrefix;
  static PropertyData propertyData;
};

// clang-format off
/// Property define
#define _ADD_PROPERTY(_name,_prop_, _defaultval_) \
  do { \
    this->_prop_.setValue _defaultval_;\
    this->_prop_.setContainer(this); \
    propertyData.addProperty(static_cast<App::PropertyContainer*>(this), _name, &this->_prop_); \
  } while (0)

#define ADD_PROPERTY(_prop_, _defaultval_) \
    _ADD_PROPERTY(#_prop_, _prop_, _defaultval_)

#define _ADD_PROPERTY_TYPE(_name,_prop_, _defaultval_, _group_,_type_,_Docu_) \
  do { \
    this->_prop_.setValue _defaultval_;\
    this->_prop_.setContainer(this); \
    propertyData.addProperty(static_cast<App::PropertyContainer*>(this), _name, &this->_prop_, (_group_),(_type_),(_Docu_)); \
  } while (0)

#define ADD_PROPERTY_TYPE(_prop_, _defaultval_, _group_,_type_,_Docu_) \
    _ADD_PROPERTY_TYPE(#_prop_,_prop_,_defaultval_,_group_,_type_,_Docu_)


#define PROPERTY_HEADER(_class_) \
  TYPESYSTEM_HEADER(); \
protected: \
  static const App::PropertyData * getPropertyDataPtr(void); \
  virtual const App::PropertyData &getPropertyData(void) const; \
private: \
  static App::PropertyData propertyData

/// Like PROPERTY_HEADER, but with overridden methods declared as such
#define PROPERTY_HEADER_WITH_OVERRIDE(_class_) \
  TYPESYSTEM_HEADER_WITH_OVERRIDE(); \
public: \
  static consteval const char* getClassName() {\
    static_assert(sizeof(_class_) > 0, "Class is not complete"); \
    \
    constexpr const char* sClass = #_class_; \
    constexpr std::string_view vClass {sClass}; \
    static_assert(!vClass.empty(), "Class name must not be empty"); \
    static_assert(std::is_base_of<App::PropertyContainer, _class_>::value, \
                  "Class must be derived from App::PropertyContainer"); \
    \
    constexpr bool isSubClassOfDocObj = std::is_base_of<App::DocumentObject, _class_>::value && \
                                        !std::is_same<App::DocumentObject, _class_>::value; \
    if constexpr (isSubClassOfDocObj) { \
      constexpr auto pos = vClass.find("::"); \
      static_assert(pos != std::string_view::npos, \
                    "Class name must be fully qualified for document object derived classes"); \
      constexpr auto vNamespace = vClass.substr(0, pos); \
      static_assert(!vNamespace.empty(), "Namespace must not be empty"); \
      \
      constexpr std::string_view filePath = __FILE__; \
      constexpr std::string_view modPath = "/src/Mod/"; \
      constexpr auto posAfterSrcMod = filePath.find(modPath); \
      constexpr bool hasSrcModInPath = posAfterSrcMod != std::string_view::npos; \
      if constexpr (hasSrcModInPath) { \
        constexpr auto pathAfterSrcMod = filePath.substr(posAfterSrcMod + modPath.size()); \
        constexpr bool isPathOk = pathAfterSrcMod.starts_with(vNamespace); \
        static_assert( \
          /* some compilers evaluate static_asserts inside ifs before checking it's a valid codepath */ \
          !isSubClassOfDocObj || !hasSrcModInPath || \
          /* allow `Path` until it's been properly renamed to CAM */ \
          vNamespace == "Path" || isPathOk, \
          "Classes in `src/Mod` needs to be in a directory with the same name as" \
          " the namespace in order to load correctly"); \
      } \
    } \
    return sClass; \
  } \
protected: \
  static const App::PropertyData * getPropertyDataPtr(void); \
  const App::PropertyData &getPropertyData(void) const override; \
private: \
  static App::PropertyData propertyData
///
#define PROPERTY_SOURCE(_class_, _parentclass_) \
TYPESYSTEM_SOURCE_P(_class_)\
const App::PropertyData * _class_::getPropertyDataPtr(void){return &propertyData;} \
const App::PropertyData & _class_::getPropertyData(void) const{return propertyData;} \
App::PropertyData _class_::propertyData; \
void _class_::init(void){\
  initSubclass(_class_::classTypeId, #_class_ , #_parentclass_, &(_class_::create) ); \
  _class_::propertyData.parentPropertyData = _parentclass_::getPropertyDataPtr(); \
}

#define PROPERTY_SOURCE_ABSTRACT(_class_, _parentclass_) \
TYPESYSTEM_SOURCE_ABSTRACT_P(_class_)\
const App::PropertyData * _class_::getPropertyDataPtr(void){return &propertyData;} \
const App::PropertyData & _class_::getPropertyData(void) const{return propertyData;} \
App::PropertyData _class_::propertyData; \
void _class_::init(void){\
  initSubclass(_class_::classTypeId, #_class_ , #_parentclass_, &(_class_::create) ); \
  _class_::propertyData.parentPropertyData = _parentclass_::getPropertyDataPtr(); \
}

#define TYPESYSTEM_SOURCE_TEMPLATE(_class_) \
template<> Base::Type _class_::classTypeId = Base::Type::BadType; \
template<> Base::Type _class_::getClassTypeId(void) { return _class_::classTypeId; } \
template<> Base::Type _class_::getTypeId(void) const { return _class_::classTypeId; } \
template<> void * _class_::create(void){\
   return new _class_ ();\
}

#define PROPERTY_SOURCE_TEMPLATE(_class_, _parentclass_) \
TYPESYSTEM_SOURCE_TEMPLATE(_class_)\
template<> App::PropertyData _class_::propertyData = App::PropertyData(); \
template<> const App::PropertyData * _class_::getPropertyDataPtr(void){return &propertyData;} \
template<> const App::PropertyData & _class_::getPropertyData(void) const{return propertyData;} \
template<> void _class_::init(void){\
  initSubclass(_class_::classTypeId, #_class_ , #_parentclass_, &(_class_::create) ); \
  _class_::propertyData.parentPropertyData = _parentclass_::getPropertyDataPtr(); \
}
// clang-format on

} // namespace App
