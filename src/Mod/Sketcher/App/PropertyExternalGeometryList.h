/***************************************************************************
 *   Copyright (c) 2018 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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


#ifndef APP_PropertyExternalGeometryList_H
#define APP_PropertyExternalGeometryList_H

#include <App/PropertyLinks.h>

namespace Base {
class Writer;
}

namespace App
{
    class DocumentObject;
}

namespace Sketcher
{

    class AppExport PropertyExternalGeometryList: public App::PropertyLinkSubList
    {
        TYPESYSTEM_HEADER();

    public:
        typedef std::tuple<App::DocumentObject*, std::vector<std::string>, std::vector<bool> > SubSet;
        /**
         * A constructor.
         * A more elaborate description of the constructor.
         */
        PropertyExternalGeometryList();

        /**
         * A destructor.
         * A more elaborate description of the destructor.
         */
        virtual ~PropertyExternalGeometryList();

        virtual void setSize(int newSize);

        /** Sets the property.
         * setValue(0, whatever) clears the property
         * 
         * The default value for boolean makes it compatible with the interface of PropertyLinkSubList
         */
        void setValue(App::DocumentObject*,const char*, bool vbool);
        void setValues(const std::vector<App::DocumentObject*>&,const std::vector<const char*>&,const std::vector<bool> &vbool);
        void setValues(const std::vector<App::DocumentObject*>&,const std::vector<std::string>&,const std::vector<bool> &vbool);
        virtual void setValue(App::DocumentObject*,const char*);
        virtual void setValues(const std::vector<App::DocumentObject*>&,const std::vector<const char*>&);
        virtual void setValues(const std::vector<App::DocumentObject*>&,const std::vector<std::string>&);	

        /**
         * @brief setValue: PropertyLinkSub-compatible overload
         * @param SubList
         */
	virtual void setValue(App::DocumentObject *lValue, const std::vector<std::string> &SubList=std::vector<std::string>());

        virtual const std::string getPyReprString() const;

        const std::vector<bool> &getBoolValues(void) const {
            return _lboolList;
        }

        /**
         * @brief Removes all occurrences of \a lValue in the property
         * together with its sub-elements and returns the number of entries removed.
         */
        virtual int removeValue(App::DocumentObject *lValue);

        void setSubListValues(const std::vector<PropertyExternalGeometryList::SubSet>&);
	virtual void setSubListValues(const std::vector<PropertyLinkSubList::SubSet>&);
	
        std::vector<PropertyExternalGeometryList::SubSet> getSubListValues() const;

        virtual PyObject *getPyObject(void);
        virtual void setPyObject(PyObject *);

        virtual void Save (Base::Writer &writer) const;
        virtual void Restore(Base::XMLReader &reader);

        virtual Property *Copy(void) const;
        virtual void Paste(const Property &from);

        virtual unsigned int getMemSize (void) const;

    protected:
        std::vector<bool>     _lboolList;
};

} // namespace Sketcher


#endif // APP_PropertyExternalGeometryList_H
