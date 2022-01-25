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

#ifndef BASE__PARAMETER_H
#define BASE__PARAMETER_H

// Include files
#include <Python.h>

#ifdef FC_OS_MACOSX
#undef toupper
#undef tolower
#undef isupper
#undef islower
#undef isspace
#undef isalpha
#undef isalnum
#endif

#ifdef FC_OS_LINUX
#include <sstream>
#endif

#include <map>
#include <vector>
#include <xercesc/util/XercesDefs.hpp>

// Std. configurations
#include "Handle.h"
#include "Observer.h"

#ifdef _MSC_VER
#	pragma warning( disable : 4251 )
#	pragma warning( disable : 4503 )
#	pragma warning( disable : 4786 )  // specifier longer then 255 chars
#	pragma warning( disable : 4290 )  // not implemented throw specification
#	pragma warning( disable : 4275 )
#endif


XERCES_CPP_NAMESPACE_BEGIN
class DOMNode;
class DOMElement;
class DOMDocument;
class XMLFormatTarget;
class InputSource;
XERCES_CPP_NAMESPACE_END

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
 *  Its main task is making user parameter persitent, saving
 *  last used values in dialog boxes, setting and retrieving all
 *  kind of preferences and so on.
 *  @see ParameterManager
 */
class  BaseExport ParameterGrp : public Base::Handled,public Base::Subject <const char*>
{
public:
    /** @name copy and insertation */
    //@{
    /// make a deep copy to the other group
    void copyTo(Base::Reference<ParameterGrp>);
    /// overwrite everything similar, leave the others alone
    void insertTo(Base::Reference<ParameterGrp>);
    /// export this group to a file
    void exportTo(const char* FileName);
    /// import from a file to this group
    void importFrom(const char* FileName);
    /// insert from a file to this group, overwrite only the similar
    void insert(const char* FileName);
    //@}

    /** @name methods for group handling */
    //@{
    /// get a handle to a sub group or create one
    Base::Reference<ParameterGrp> GetGroup(const char* Name);
    /// get a vector of all sub groups in this group
    std::vector<Base::Reference<ParameterGrp> > GetGroups();
    /// test if this group is empty
    bool IsEmpty() const;
    /// test if a special sub group is in this group
    bool HasGroup(const char* Name) const;
    /// type of the handle
    typedef Base::Reference<ParameterGrp> handle;
    /// remove a sub group from this group
    void RemoveGrp(const char* Name);
    /// rename a sub group from this group
    bool RenameGrp(const char* OldName, const char* NewName);
    /// clears everything in this group (all types)
    void Clear();
    //@}

    /** @name methods for bool handling */
    //@{
    /// read bool values or give default
    bool GetBool(const char* Name, bool bPreset=false) const;
    /// set a bool value
    void SetBool(const char* Name, bool bValue);
    /// get a vector of all bool values in this group
    std::vector<bool> GetBools(const char * sFilter = nullptr) const;
    /// get a map with all bool values and the keys of this group
    std::vector<std::pair<std::string,bool> > GetBoolMap(const char * sFilter = nullptr) const;
    /// remove a bool value from this group
    void RemoveBool(const char* Name);
    //@}

    /** @name methods for Int handling */
    //@{
    /// read bool values or give default
    long GetInt(const char* Name, long lPreset=0) const;
    /// set a int value
    void SetInt(const char* Name, long lValue);
    /// get a vector of all int values in this group
    std::vector<long> GetInts(const char * sFilter = nullptr) const;
    /// get a map with all int values and the keys of this group
    std::vector<std::pair<std::string,long> > GetIntMap(const char * sFilter = nullptr) const;
    /// remove a int value from this group
    void RemoveInt(const char* Name);
    //@}

    /** @name methods for Unsigned Int handling */
    //@{
    /// read uint values or give default
    unsigned long GetUnsigned(const char* Name, unsigned long lPreset=0) const;
    /// set a uint value
    void SetUnsigned(const char* Name, unsigned long lValue);
    /// get a vector of all uint values in this group
    std::vector<unsigned long> GetUnsigneds(const char * sFilter = nullptr) const;
    /// get a map with all uint values and the keys of this group
    std::vector<std::pair<std::string,unsigned long> > GetUnsignedMap(const char * sFilter = nullptr) const;
    /// remove a uint value from this group
    void RemoveUnsigned(const char* Name);
    //@}


    /** @name methods for Float handling */
    //@{
    /// set a float value
    double GetFloat(const char* Name, double dPreset=0.0) const;
    /// read float values or give default
    void SetFloat(const char* Name, double dValue);
    /// get a vector of all float values in this group
    std::vector<double> GetFloats(const char * sFilter = nullptr) const;
    /// get a map with all float values and the keys of this group
    std::vector<std::pair<std::string,double> > GetFloatMap(const char * sFilter = nullptr) const;
    /// remove a float value from this group
    void RemoveFloat(const char* Name);
    //@}


    /** @name methods for Blob handling (not implemented yet) */
    //@{
    /// set a blob value
    void  SetBlob(const char* Name, void *pValue, long lLength);
    /// read blob values or give default
    void GetBlob(const char* Name, void * pBuf, long lMaxLength, void* pPreset=nullptr) const;
    /// remove a blob value from this group
    void RemoveBlob(const char* Name);
    //@}



    /** @name methods for String handling */
    //@{
    /// set a string value
    void  SetASCII(const char* Name, const char *sValue);
    /// read a string values
    std::string GetASCII(const char* Name, const char * pPreset=nullptr) const;
    /// remove a string value from this group
    void RemoveASCII(const char* Name);
    /** Return all string elements in this group as a vector of strings
     *  Its also possible to set a filter criteria.
     *  @param sFilter only strings which name includes sFilter are put in the vector
     *  @return std::vector of std::strings
     */
    std::vector<std::string> GetASCIIs(const char * sFilter = nullptr) const;
    /// Same as GetASCIIs() but with key,value map
    std::vector<std::pair<std::string,std::string> > GetASCIIMap(const char * sFilter = nullptr) const;
    //@}

    friend class ParameterManager;

    /// returns the name
    const char* GetGroupName() const {
        return _cName.c_str();
    }

    /** Notifies all observers for all entries except of sub-groups.
     */
    void NotifyAll();

protected:
    /// constructor is protected (handle concept)
    ParameterGrp(XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *GroupNode=nullptr,const char* sName=nullptr);
    /// destructor is protected (handle concept)
    ~ParameterGrp();
    /// helper function for GetGroup
    Base::Reference<ParameterGrp> _GetGroup(const char* Name);
    bool ShouldRemove() const;

    XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *FindNextElement(XERCES_CPP_NAMESPACE_QUALIFIER DOMNode *Prev, const char* Type) const;

    /** Find an element specified by Type and Name
     *  Search in the parent element Start for the first occurrence of an
     *  element of Type and with the attribute Name=Name. On success it returns
     *  the pointer to that element, otherwise NULL
     *  If the names not given it returns the first occurrence of Type.
     */
    XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *FindElement(XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *Start, const char* Type, const char* Name=nullptr) const;

    /** Find an element specified by Type and Name or create it if not found
     *  Search in the parent element Start for the first occurrence of an
     *  element of Type and with the attribute Name=Name. On success it returns
     *  the pointer to that element, otherwise it creates the element and returns the pointer.
     */
    XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *FindOrCreateElement(XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *Start, const char* Type, const char* Name) const;


    /// DOM Node of the Base node of this group
    XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *_pGroupNode;
    /// the own name
    std::string _cName;
    /// map of already exported groups
    std::map <std::string ,Base::Reference<ParameterGrp> > _GroupMap;

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
    ParameterSerializer(const std::string& fn);
    virtual ~ParameterSerializer();

    virtual void SaveDocument(const ParameterManager&);
    virtual int LoadDocument(ParameterManager&);
    virtual bool LoadOrCreateDocument(ParameterManager&);

protected:
    std::string filename;
};

/** The parameter manager class
 *  This class manages a parameter XML document.
 *  Does loading, saving and handling the DOM document.
 *  @see ParameterGrp
 */
class BaseExport ParameterManager : public ParameterGrp
{
public:
    ParameterManager();
    ~ParameterManager();
    static void Init();
    static void Terminate();

    int   LoadDocument(const char* sFileName);
    int   LoadDocument(const XERCES_CPP_NAMESPACE_QUALIFIER InputSource&);
    bool  LoadOrCreateDocument(const char* sFileName);
    void  SaveDocument(const char* sFileName) const;
    void  SaveDocument(XERCES_CPP_NAMESPACE_QUALIFIER XMLFormatTarget* pFormatTarget) const;
    void  CreateDocument();
    void  CheckDocument() const;

    /** @name Parameter serialization */
    //@{
    /// Sets a serializer. The ParameterManager takes ownership of the serializer.
    void  SetSerializer(ParameterSerializer*);
    /// Returns true if a serializer is set, otherwise false is returned.
    bool  HasSerializer() const;
    /// Loads an XML document by calling the serializer's load method.
    int   LoadDocument();
    /// Loads or creates an XML document by calling the serializer's load method.
    bool  LoadOrCreateDocument();
    /// Saves an XML document by calling the serializer's save method.
    void  SaveDocument() const;
    //@}

private:

    XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument   *_pDocument;
    ParameterSerializer * paramSerializer;

    bool          gDoNamespaces         ;
    bool          gDoSchema             ;
    bool          gSchemaFullChecking   ;
    bool          gDoCreate             ;


    const XMLCh*  gOutputEncoding       ;
    const XMLCh*  gMyEOLSequence        ;

    bool          gSplitCdataSections   ;
    bool          gDiscardDefaultContent;
    bool          gUseFilter            ;
    bool          gFormatPrettyPrint    ;

};

/** python wrapper function
*/
BaseExport PyObject* GetPyObject( const Base::Reference<ParameterGrp> &hcParamGrp);


#endif // BASE__PARAMETER_H
