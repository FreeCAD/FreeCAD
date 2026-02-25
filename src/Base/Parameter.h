// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License (LGPL)   *
 *   as published by the Free Software Foundation; either version 2 of     *
 *   the License, or (at your option) any later version.                   *
 *   for detail see the LICENCE text file.                                 *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful,            *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with FreeCAD; if not, write to the Free Software        *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
 *   USA                                                                   *
 *                                                                         *
 ***************************************************************************/

/**
 * \file Parameter.h
 * \brief The classes defined here are used to interface with the XML-based
 * FreeCAD config files: user.cfg and system.cfg files. It can parse, get,
 * and store the parameters/configurations for the user's preferences.
 * 3rd party Xerces-C++ XML parser is used to parse and write the XML.
 */

#pragma once

// Python stuff
using PyObject = struct _object;

#include <FCConfig.h>

#ifdef FC_OS_MACOSX
# undef toupper
# undef tolower
# undef isupper
# undef islower
# undef isspace
# undef isalpha
# undef isalnum
#endif

#include <map>
#include <vector>
#include <fastsignals/signal.h>
#include <xercesc/util/XercesDefs.hpp>

#include "Handle.h"
#include "Observer.h"
#include "Color.h"

#ifdef _MSC_VER
# pragma warning(disable : 4251)
# pragma warning(disable : 4503)
# pragma warning(disable : 4786)  // specifier longer then 255 chars
# pragma warning(disable : 4290)  // not implemented throw specification
# pragma warning(disable : 4275)
#endif

namespace XERCES_CPP_NAMESPACE
{
class DOMNode;
class DOMElement;
class DOMDocument;
class XMLFormatTarget;
class InputSource;
}  // namespace XERCES_CPP_NAMESPACE

class ParameterManager;

/** The parameter container class
 *  This is the base class of all classes handle parameter.
 *  The class contains a map of key-value pairs in a grouping
 *  structure, not unlike the windows registry.
 *  It allows the user to set and retrieve values of the
 *  type float, long and string. Also it handles importing
 *  and exporting groups of parameters and enables streaming
 *  to a persistent medium via XML.
 *  \par
 *  Its main task is making user parameter persistent, saving
 *  last used values in dialog boxes, setting and retrieving all
 *  kind of preferences and so on.
 *  @see ParameterManager
 */
class BaseExport ParameterGrp: public Base::Handled, public Base::Subject<const char*>
{
public:
    ParameterGrp(const ParameterGrp&) = delete;
    ParameterGrp(ParameterGrp&&) = delete;
    ParameterGrp& operator=(const ParameterGrp&) = delete;
    ParameterGrp& operator=(ParameterGrp&&) = delete;

    /** @name Copying & Inserting */
    //@{

    /**
     *  Overwrites another group with this one.
     *
     *  @param[out] Group The group to overwrite.
     */
    void copyTo(const Base::Reference<ParameterGrp>& Group);

    /**
     *  Inserts items from this group into another.
     *
     *  @param[out] Group The group to insert into.
     *
     *  @note
     *  Inserts new and replaces existing items.
     */
    void insertTo(const Base::Reference<ParameterGrp>& Group);

    /**
     *  Exports this group to a given file.
     *
     *  @param[out] FileName The path to the file.
     */
    void exportTo(const char* FileName);

    /**
     *  Overwrites this group with the given file.
     *
     *  @param[in] FileName The path to the file.
     */
    void importFrom(const char* FileName);

    /**
     *  Inserts items from the given file.
     *
     *  @param[in] FileName The path to the file.
     *
     *  @note
     *  Inserts new and replaces existing items.
     */
    void insert(const char* FileName);

    /**
     *  Removes items from this group that are present in the given file.
     *
     *  @param[in] FileName The path to the file.
     *
     *  @note
     *  Only removes items that have the same value.
     */
    void revert(const char* FileName);

    /**
     *  Removes items from this group that are present in the other.
     *
     *  @param[in] Group The group to compare with.
     *
     *  @note
     *  Only removes items that have the same value.
     */
    void revert(const Base::Reference<ParameterGrp>& Group);

    //@}

    /** @name methods for group handling */
    //@{

    /**
     *  Returns or creates a sub-group with the given name.
     *
     *  @param[in] Name Name of the sub-group.
     *  @returns A handle to the sub-group.
     */
    Base::Reference<ParameterGrp> GetGroup(const char* Name);

    /**
     *  Returns all sub-groups.
     *
     *  @returns A vector of handles to the sub-groups.
     */
    std::vector<Base::Reference<ParameterGrp>> GetGroups();

    /**
     *  Tests if this group is empty.
     */
    bool IsEmpty() const;

    /**
     *  Tests if a sub-group exists.
     *
     *  @param[in] Name Name of the sub-group.
     */
    bool HasGroup(const char* Name) const;

    /// type of the handle
    using handle = Base::Reference<ParameterGrp>;

    /**
     *  Removes a sub-group.
     *
     *  @param[in] Name Name of the sub-group.
     */
    void RemoveGrp(const char* Name);

    /**
     *  Renames a sub-group.
     *
     *  @param[in] OldName The current name of the sub-group.
     *  @param[in] NewName The new name the sub-group will have.
     *  @returns Whether or not the renaming succeeded.
     *
     *  @note Does nothing if a sub-group with the new name already exists.
     */
    bool RenameGrp(const char* OldName, const char* NewName);

    /**
     *  Empties this group.
     *
     *  @param[in] notify Whether to notify on deleted parameters using the Observer interface.
     */
    void Clear(bool notify = false);

    //@}

    /** @name methods for generic attribute handling */
    //@{

    enum class ParamType
    {
        FCInvalid = 0,
        FCText = 1,
        FCBool = 2,
        FCInt = 3,
        FCUInt = 4,
        FCFloat = 5,
        FCGroup = 6,
    };
    static const char* TypeName(ParamType type);
    static ParamType TypeValue(const char*);

    /**
     *  Sets the value of an attribute.
     *
     *  @param[in] Type The type of the attribute.
     *  @param[in] Name The name of the attribute.
     *  @param[in] Value The value to be set.
     */
    void SetAttribute(ParamType Type, const char* Name, const char* Value);

    /**
     *  Removes an attribute.
     *
     *  @param[in] Type The type of the attribute.
     *  @param[in] Name The name of the attribute.
     */
    void RemoveAttribute(ParamType Type, const char* Name);

    /**
     *  Returns the value of the attribute.
     *
     *  If the attribute can't be found, \n
     *  the fallback value is returned.
     *
     *  @param[in] Type The type of the attribute.
     *  @param[in] Name The name of the attribute.
     *  @param[out] Value The value of attribute or the fallback value.
     *  @param[in] Default The fallback value.
     */
    const char* GetAttribute(ParamType Type, const char* Name, std::string& Value, const char* Default) const;

    /**
     *  Returns all attributes with the given type.
     *
     *  Optionally filters them to only include attributes \n
     *  with names that contain a certain string.
     *
     *  @param[in] Type The type of attributes to be returned
     *  @param[in] sFilter String that has to be present in the names of the attributes.
     *  @returns Vector of attribute name & value pairs.
     */
    std::vector<std::pair<std::string, std::string>> GetAttributeMap(
        ParamType Type,
        const char* sFilter = nullptr
    ) const;


    /**
     *  Returns all parameters.
     *
     *  Optionally filters them to only include attributes \n
     *  with names that contain a certain string.
     *
     *  @param[in] sFilter String that has to be present in the names of the attributes.
     *  @returns Vector of attribute type & name pairs.
     */
    std::vector<std::pair<ParamType, std::string>> GetParameterNames(
        const char* sFilter = nullptr
    ) const;

    //@}

    /** @name methods for bool handling */
    //@{
    /// read bool values or give default
    bool GetBool(const char* Name, bool bPreset = false) const;
    /// set a bool value
    void SetBool(const char* Name, bool bValue);
    /// get a vector of all bool values in this group
    std::vector<bool> GetBools(const char* sFilter = nullptr) const;
    /// get a map with all bool values and the keys of this group
    std::vector<std::pair<std::string, bool>> GetBoolMap(const char* sFilter = nullptr) const;
    /// remove a bool value from this group
    void RemoveBool(const char* Name);
    //@}

    /** @name methods for Int handling */
    //@{
    /// read bool values or give default
    long GetInt(const char* Name, long lPreset = 0) const;
    /// set a int value
    void SetInt(const char* Name, long lValue);
    /// get a vector of all int values in this group
    std::vector<long> GetInts(const char* sFilter = nullptr) const;
    /// get a map with all int values and the keys of this group
    std::vector<std::pair<std::string, long>> GetIntMap(const char* sFilter = nullptr) const;
    /// remove a int value from this group
    void RemoveInt(const char* Name);
    //@}

    /** @name methods for Unsigned Int handling */
    //@{
    /// read uint values or give default
    unsigned long GetUnsigned(const char* Name, unsigned long lPreset = 0) const;
    /// set a uint value
    void SetUnsigned(const char* Name, unsigned long lValue);
    /// get a vector of all uint values in this group
    std::vector<unsigned long> GetUnsigneds(const char* sFilter = nullptr) const;
    /// get a map with all uint values and the keys of this group
    std::vector<std::pair<std::string, unsigned long>> GetUnsignedMap(
        const char* sFilter = nullptr
    ) const;
    /// remove a uint value from this group
    void RemoveUnsigned(const char* Name);
    //@}

    /** @name methods for Colors handling, colors are persisted as packed uints */
    //@{
    /// read color value or give default
    Base::Color GetColor(const char* Name, Base::Color lPreset = Base::Color(1.0, 1.0, 1.0)) const;
    /// set a color value
    void SetColor(const char* Name, Base::Color lValue);
    /// get a vector of all color values in this group
    std::vector<Base::Color> GetColors(const char* sFilter = nullptr) const;
    /// get a map with all color values and the keys of this group
    std::vector<std::pair<std::string, Base::Color>> GetColorMap(const char* sFilter = nullptr) const;
    /// remove a color value from this group
    void RemoveColor(const char* Name);
    //@}


    /** @name methods for Float handling */
    //@{
    /// set a float value
    double GetFloat(const char* Name, double dPreset = 0.0) const;
    /// read float values or give default
    void SetFloat(const char* Name, double dValue);
    /// get a vector of all float values in this group
    std::vector<double> GetFloats(const char* sFilter = nullptr) const;
    /// get a map with all float values and the keys of this group
    std::vector<std::pair<std::string, double>> GetFloatMap(const char* sFilter = nullptr) const;
    /// remove a float value from this group
    void RemoveFloat(const char* Name);
    //@}


    /** @name methods for String handling */
    //@{
    /// set a string value
    void SetASCII(const char* Name, const char* sValue);
    /// set a string value
    void SetASCII(const char* Name, const std::string& sValue)
    {
        SetASCII(Name, sValue.c_str());
    }
    /// read a string values
    std::string GetASCII(const char* Name, const char* pPreset = nullptr) const;
    /// remove a string value from this group
    void RemoveASCII(const char* Name);
    /** Return all string elements in this group as a vector of strings
     *  Its also possible to set a filter criteria.
     *  @param sFilter only strings which name includes sFilter are put in the vector
     *  @return std::vector of std::strings
     */
    std::vector<std::string> GetASCIIs(const char* sFilter = nullptr) const;
    /// Same as GetASCIIs() but with key,value map
    std::vector<std::pair<std::string, std::string>> GetASCIIMap(const char* sFilter = nullptr) const;
    //@}

    friend class ParameterManager;

    /// returns the name
    const char* GetGroupName() const
    {
        return _cName.c_str();
    }

    /// return the full path of this group
    std::string GetPath() const;
    void GetPath(std::string&) const;

    /** Notifies all observers for all entries except of sub-groups.
     */
    void NotifyAll();

    ParameterGrp* Parent() const
    {
        return _Parent;
    }
    ParameterManager* Manager() const
    {
        return _Manager;
    }

protected:
    /// constructor is protected (handle concept)
    ParameterGrp(
        XERCES_CPP_NAMESPACE::DOMElement* GroupNode = nullptr,
        const char* sName = nullptr,
        ParameterGrp* Parent = nullptr
    );
    /// destructor is protected (handle concept)
    ~ParameterGrp() override;
    /// helper function for GetGroup
    Base::Reference<ParameterGrp> _GetGroup(const char* Name);
    bool ShouldRemove() const;

    void _Reset();

    void _SetAttribute(ParamType Type, const char* Name, const char* Value);
    void _Notify(ParamType Type, const char* Name, const char* Value);

    XERCES_CPP_NAMESPACE::DOMElement* FindNextElement(
        XERCES_CPP_NAMESPACE::DOMNode* Prev,
        const char* Type
    ) const;

    /** Find an element specified by Type and Name
     *  Search in the parent element Start for the first occurrence of an
     *  element of Type and with the attribute Name=Name. On success it returns
     *  the pointer to that element, otherwise NULL
     *  If the names not given it returns the first occurrence of Type.
     */
    XERCES_CPP_NAMESPACE::DOMElement* FindElement(
        XERCES_CPP_NAMESPACE::DOMElement* Start,
        const char* Type,
        const char* Name = nullptr
    ) const;

    /** Find an element specified by Type and Name or create it if not found
     *  Search in the parent element Start for the first occurrence of an
     *  element of Type and with the attribute Name=Name. On success it returns
     *  the pointer to that element, otherwise it creates the element and returns the pointer.
     */
    XERCES_CPP_NAMESPACE::DOMElement* FindOrCreateElement(
        XERCES_CPP_NAMESPACE::DOMElement* Start,
        const char* Type,
        const char* Name
    );

    XERCES_CPP_NAMESPACE::DOMElement* CreateElement(
        XERCES_CPP_NAMESPACE::DOMElement* Start,
        const char* Type,
        const char* Name
    );

    /** Find an attribute specified by Name
     */
    XERCES_CPP_NAMESPACE::DOMNode* FindAttribute(
        XERCES_CPP_NAMESPACE::DOMNode* Node,
        const char* Name
    ) const;

    /// DOM Node of the Base node of this group
    XERCES_CPP_NAMESPACE::DOMElement* _pGroupNode;
    /// the own name
    std::string _cName;
    /// map of already exported groups
    std::map<std::string, Base::Reference<ParameterGrp>> _GroupMap;
    ParameterGrp* _Parent = nullptr;
    ParameterManager* _Manager = nullptr;
    /// Means this group xml element has not been added to its parent yet.
    bool _Detached = false;
    /** Indicate this group is currently being cleared
     *
     * This is used to prevent anynew value/sub-group to be added in observer
     */
    bool _Clearing = false;
};

/** The parameter serializer class
 *  This is a helper class to serialize a parameter XML document.
 *  Does loading and saving the DOM document from and to files.
 *  In sub-classes the load and saving of XML documents can be
 *  customized.
 *  @see ParameterManager
 */
class BaseExport ParameterSerializer
{
public:
    explicit ParameterSerializer(std::string fn);
    ParameterSerializer(const ParameterSerializer&) = delete;
    ParameterSerializer(ParameterSerializer&&) = delete;
    virtual ~ParameterSerializer();

    virtual void SaveDocument(const ParameterManager&);
    virtual int LoadDocument(ParameterManager&);
    virtual bool LoadOrCreateDocument(ParameterManager&);
    const std::string& GetFileName() const
    {
        return filename;
    }

    ParameterSerializer& operator=(const ParameterSerializer&) = delete;
    ParameterSerializer& operator=(ParameterSerializer&&) = delete;

private:
    std::string filename;
};

/** The parameter manager class
 *  This class manages a parameter XML document.
 *  Does loading, saving and handling the DOM document.
 *  @see ParameterGrp
 */
class BaseExport ParameterManager: public ParameterGrp
{
public:
    /// Create a reference counted ParameterManager
    static Base::Reference<ParameterManager> Create();
    static void Init();
    static void Terminate();

    /** Signal on parameter changes
     *
     * The signal is triggered on adding, removing, renaming or modifying on
     * all individual parameters and group. The signature of the signal is
     * \code
     *      void (ParameterGrp *param, ParamType type, const char *name, const char *value)
     * \endcode
     * where 'param' is the parameter group causing the change, 'type' is the
     * type of the parameter, 'name' is the name of the parameter, and 'value'
     * is the current value.
     *
     * The possible values of 'type' are, 'FCBool', 'FCInt', 'FCUint',
     * 'FCFloat', 'FCText', and 'FCParamGroup'. The notification is triggered
     * when value is changed, in which case 'value' contains the new value in
     * text form, or, when the parameter is removed, in which case 'value' is
     * empty.
     *
     * For 'FCParamGroup' type, the observer will be notified in the following events.
     *  - Group creation: both 'name' and 'value' contain the name of the new group
     *  - Group removal: both 'name' and 'value' are empty
     *  - Group rename: 'name' is the new name, and 'value' is the old name
     */
    fastsignals::signal<
        void(ParameterGrp* /*param*/, ParamType /*type*/, const char* /*name*/, const char* /*value*/)>
        signalParamChanged;

    int LoadDocument(const char* sFileName);
    int LoadDocument(const XERCES_CPP_NAMESPACE::InputSource&);
    bool LoadOrCreateDocument(const char* sFileName);
    void SaveDocument(const char* sFileName) const;
    void SaveDocument(XERCES_CPP_NAMESPACE::XMLFormatTarget* pFormatTarget) const;
    void CreateDocument();
    void CheckDocument() const;

    /** @name Parameter serialization */
    //@{
    /// Sets a serializer. The ParameterManager takes ownership of the serializer.
    void SetSerializer(ParameterSerializer*);
    /// Returns true if a serializer is set, otherwise false is returned.
    bool HasSerializer() const;
    /// Returns the filename of the serialize.
    const std::string& GetSerializeFileName() const;
    /// Loads an XML document by calling the serializer's load method.
    int LoadDocument();
    /// Loads or creates an XML document by calling the serializer's load method.
    bool LoadOrCreateDocument();
    /// Saves an XML document by calling the serializer's save method.
    void SaveDocument() const;
    void SetIgnoreSave(bool value);
    bool IgnoreSave() const;
    //@}

private:
    XERCES_CPP_NAMESPACE::DOMDocument* _pDocument {nullptr};
    ParameterSerializer* paramSerializer {nullptr};

    bool gIgnoreSave;
    bool gDoNamespaces;
    bool gDoSchema;
    bool gSchemaFullChecking;
    bool gDoCreate;


    const XMLCh* gOutputEncoding;
    const XMLCh* gMyEOLSequence;

    bool gSplitCdataSections;
    bool gDiscardDefaultContent;
    bool gUseFilter;
    bool gFormatPrettyPrint;

private:
    ParameterManager();

public:
    ~ParameterManager() override;
    ParameterManager(const ParameterManager&) = delete;
    ParameterManager(ParameterManager&&) = delete;

    ParameterManager& operator=(const ParameterManager&) = delete;
    ParameterManager& operator=(ParameterManager&&) = delete;
};

/** python wrapper function
 */
BaseExport PyObject* GetPyObject(const Base::Reference<ParameterGrp>& hcParamGrp);
