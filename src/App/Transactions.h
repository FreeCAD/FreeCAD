/***************************************************************************
 *   Copyright (c) Juergen Riegel         <juergen.riegel@web.de>          *
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


#ifndef APP_TRANSACTION_H
#define APP_TRANSACTION_H

#include <Base/Persistence.h>

namespace App
{

class Document;
class DocumentObject;
class Property;
class Transaction;


/** Represents an entry for an object in a Transaction
 */
class AppExport TransactionObject: public Base::Persistence
{
    TYPESYSTEM_HEADER();

public:
    /// Construction
    TransactionObject(const DocumentObject *pcObj,const char *NameInDocument=0);
    /// Destruction
    virtual ~TransactionObject();

    void applyNew(Document &Doc, DocumentObject *pcObj);
    void applyDel(Document &Doc, DocumentObject *pcObj);
    void applyChn(Document &Doc, DocumentObject *pcObj,bool Forward);

    void setProperty(const Property* pcProp);

    virtual unsigned int getMemSize (void) const;
    virtual void Save (Base::Writer &writer) const;
    /// This method is used to restore properties from an XML document.
    virtual void Restore(Base::XMLReader &reader);

    friend class Transaction;

protected:
    enum Status {New,Del,Chn} status;
    std::map<const Property*,Property*> _PropChangeMap;
    std::string _NameInDocument;
};

/** Represents a atomic transaction of the document
 */
class AppExport Transaction : public Base::Persistence
{
    TYPESYSTEM_HEADER();

public:
    /// Construction
    Transaction();
    /// Construction
    Transaction(int pos);
    /// Destruction
    virtual ~Transaction();

    /// apply the content to the document
    void apply(Document &Doc,bool forward);

    // the utf-8 name of the transaction
    std::string Name; 

    virtual unsigned int getMemSize (void) const;
    virtual void Save (Base::Writer &writer) const;
    /// This method is used to restore properties from an XML document.
    virtual void Restore(Base::XMLReader &reader);

    /// get the position in the transaction history
    int getPos(void) const;
    /// check if this object is used in a transaction
    bool hasObject(DocumentObject *Obj) const;

    friend class Document;

protected:
    void addObjectNew(DocumentObject *Obj);
    void addObjectDel(const DocumentObject *Obj);
    void addObjectChange(const DocumentObject *Obj,const Property *Prop);

private:
    int iPos;
    std::map<const DocumentObject*,TransactionObject*> _Objects;
};


} //namespace App

#endif // APP_TRANSACTION_H

