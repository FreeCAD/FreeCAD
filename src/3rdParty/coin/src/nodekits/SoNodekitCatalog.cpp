/**************************************************************************\
 * Copyright (c) Kongsberg Oil & Gas Technologies AS
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\**************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef HAVE_NODEKITS

/*!
  \class SoNodekitCatalog SoNodekitCatalog.h Inventor/nodekits/SoNodekitCatalog.h
  \brief The SoNodekitCatalog class is a container for nodekit layouts.

  \ingroup coin_nodekits

  Nodekits store all their hierarchical layout information and part
  information in instances of this class.

  \sa SoNodeKit, SoBaseKit
*/

#include <Inventor/nodekits/SoBaseKit.h>

#include <cassert>
#include <cstdio> // fprintf()

#include <Inventor/lists/SoTypeList.h>
#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

#include "threads/threadsutilp.h"

// Private container class.
class CatalogItem {
public:
  CatalogItem(void) { }
  ~CatalogItem() { }

  SbName name, parentname, siblingname;
  SoType type, defaulttype, containertype;
  SbBool isdefaultnull, islist, ispublic;
  SoTypeList itemtypeslist;
};

/*!
  Initialization of static variables.
*/
void
SoNodekitCatalog::initClass(void)
{
}

/*!
  Constructor.
*/
SoNodekitCatalog::SoNodekitCatalog(void)
{
}

/*!
  Destructor.
*/
SoNodekitCatalog::~SoNodekitCatalog()
{
  int i;
  for (i=0; i < this->items.getLength(); i++)
    delete this->items[i];
  for (i=0; i < this->delayeditems.getLength(); i++)
    delete this->delayeditems[i];
}

/*!
  Returns total number of entries in the catalog.
*/
int
SoNodekitCatalog::getNumEntries(void) const
{
  assert(this->delayeditems.getLength() == 0);
  return this->items.getLength();
}

/*!
  Returns part number in catalog with \a name. If no part exists with
  \a name, returns \c SO_CATALOG_NAME_NOT_FOUND.
*/
int
SoNodekitCatalog::getPartNumber(const SbName & name) const
{
  assert(this->delayeditems.getLength() == 0);
  return this->getPartNumber(this->items, name);
}

/*!
  Given the \a part number, return name of that part.
*/
const SbName &
SoNodekitCatalog::getName(int part) const
{
  assert( this->delayeditems.getLength() == 0 );
  assert( part >= 0 && part < this->getNumEntries() &&
          "invalid part" );

  return this->items[part]->name;
}

/*!
  Given the \a part number, return type.
*/
SoType
SoNodekitCatalog::getType(int part) const
{
  assert(this->delayeditems.getLength() == 0);
  assert( part >= 0 && part < this->getNumEntries() &&
          "invalid part" );
  return this->items[part]->type;
}

/*!
  Given the part \a name, return type.
*/
SoType
SoNodekitCatalog::getType(const SbName & name) const
{
  assert(this->delayeditems.getLength() == 0);
  return this->getType( this->getPartNumber( name ) );
}

/*!
  Given \a part number, return default type of part.
*/
SoType
SoNodekitCatalog::getDefaultType(int part) const
{
  assert(this->delayeditems.getLength() == 0);
  assert( part >= 0 && part < this->getNumEntries() &&
          "invalid part" );

  return this->items[part]->defaulttype;
}

/*!
  Given part \a name, return default type of part.
*/
SoType
SoNodekitCatalog::getDefaultType(const SbName & name) const
{
  assert( this->delayeditems.getLength() == 0 );
  return this->getDefaultType( this->getPartNumber( name ) );
}

/*!
  Returns \c TRUE if the \a part is empty by default, otherwise \c FALSE.
*/
SbBool
SoNodekitCatalog::isNullByDefault(int part) const
{
  assert( this->delayeditems.getLength() == 0 );
  assert( part != SO_CATALOG_NAME_NOT_FOUND &&
          "invalid part");

  return this->items[part]->isdefaultnull;
}

// Seems like there's a Doxygen bug (version 1.2.1, at least) as it
// reports isNullByDefault(const SbName & name) as undocumented
// without the explicit \fn.

/*!
  \fn SbBool SoNodekitCatalog::isNullByDefault(const SbName & name) const

  Returns \c TRUE if part \a name is empty by default, otherwise \c
  FALSE.
*/
SbBool
SoNodekitCatalog::isNullByDefault(const SbName & name) const
{
  assert(this->delayeditems.getLength() == 0);
  return this->isNullByDefault( this->getPartNumber(name) );
}

/*!
  Returns \c TRUE if the \a part is \e not a parent for any
  other parts in the nodekit catalog.
*/
SbBool
SoNodekitCatalog::isLeaf(int part) const
{
  assert( this->delayeditems.getLength() == 0);
  assert( part >= 0 && part < this->getNumEntries() &&
          "invalid part" );

  for (int i=0; i < this->items.getLength(); i++) {
    if ((i != part) && (this->items[part]->name == this->items[i]->parentname))
      return FALSE;
  }
  return TRUE;
}

/*!
  Returns \c TRUE if the part \a name is \e not a parent for any
  other parts in the nodekit catalog.
*/
SbBool
SoNodekitCatalog::isLeaf(const SbName & name) const
{
  assert(this->delayeditems.getLength() == 0);
  return this->isLeaf( this->getPartNumber( name ));
}

/*!
  Returns name of parent of \a part. If \a part doesn't have a parent,
  the empty string is returned.
*/
const SbName &
SoNodekitCatalog::getParentName(int part) const
{
  assert( this->delayeditems.getLength() == 0);
  assert( part >= 0 && part < this->getNumEntries() &&
          "invalid part" );
  return this->items[part]->parentname;
}

/*!
  Returns name of parent of the part. If \a name doesn't have a parent,
  the empty string is returned.
*/
const SbName &
SoNodekitCatalog::getParentName(const SbName & name) const
{
  assert(this->delayeditems.getLength() == 0);
  return this->getParentName( this->getPartNumber(name) );
}

/*!
  Returns part number of given part's parent. If \a part doesn't have a parent,
  SO_CATALOG_NAME_NOT_FOUND is returned.
*/
int
SoNodekitCatalog::getParentPartNumber(int part) const
{
  assert( this->delayeditems.getLength() == 0 );
  assert( part >= 0 && part < this->getNumEntries() &&
          "invalid part" );

  return this->getPartNumber(this->items[part]->parentname);
}

/*!
  Returns part number of given part's parent. If \a name doesn't have a parent,
  SO_CATALOG_NAME_NOT_FOUND is returned.
*/
int
SoNodekitCatalog::getParentPartNumber(const SbName & name) const
{
  assert(this->delayeditems.getLength() == 0);
  return this->getParentPartNumber( this->getPartNumber(name) );  
}

/*!
  Returns name of right sibling of \a part. Returns the empty string if
  \a part doesn't have a right sibling.
*/
const SbName &
SoNodekitCatalog::getRightSiblingName(int part) const
{
  assert(this->delayeditems.getLength() == 0);
  assert( part >= 0 && part < this->getNumEntries() &&
          "invalid part" );

  return this->items[part]->siblingname;
}


/*!
  Returns name of sibling of the part. Returns the empty string if
  \a name doesn't have a right sibling.
*/
const SbName &
SoNodekitCatalog::getRightSiblingName(const SbName & name) const
{
  assert(this->delayeditems.getLength() == 0);
  return this->getRightSiblingName( this->getPartNumber(name) );
}

/*!
  Returns part number of given part's sibling. Returns
  SO_CATALOG_NAME_NOT_FOUND if \a part doesn't have a right sibling.
*/
int
SoNodekitCatalog::getRightSiblingPartNumber(int part) const
{
  assert(this->delayeditems.getLength() == 0);
  assert( part >= 0 && part < this->getNumEntries() && 
          "invalid part" );

  return this->getPartNumber(this->items[part]->siblingname);
}

/*!
  Returns part number of given part's right sibling. Returns
  SO_CATALOG_NAME_NOT_FOUND if part doesn't have a right sibling.
*/
int
SoNodekitCatalog::getRightSiblingPartNumber(const SbName & name) const
{
  assert(this->delayeditems.getLength() == 0);

  return this->getRightSiblingPartNumber( this->getPartNumber(name) );
}

/*!
  Returns \c TRUE if the given \a part is a list container.
*/
SbBool
SoNodekitCatalog::isList(int part) const
{
  assert(this->delayeditems.getLength() == 0);
  assert( part >= 0 && part < this->getNumEntries() &&
          "invalid part" );

  return this->items[part]->islist;
}

/*!
  Returns \c TRUE if the given part is a list container.
*/
SbBool
SoNodekitCatalog::isList(const SbName & name) const
{
  assert(this->delayeditems.getLength() == 0);
  return this->isList( this->getPartNumber(name) );
}

/*!
  Returns type of list container (SoGroup, SoSeparator, SoSwitch, etc)
  which \a part is.
*/
SoType
SoNodekitCatalog::getListContainerType(int part) const
{
  assert( this->delayeditems.getLength() == 0 );
  assert( part >= 0 && part < this->getNumEntries() && 
          "invalid part" );
  assert( this->items[part]->islist && 
          "not a list container" );

  return this->items[part]->containertype;
}

/*!
  Returns type of list container (SoGroup, SoSeparator, SoSwitch, etc)
  which the named part is.
*/
SoType
SoNodekitCatalog::getListContainerType(const SbName & name) const
{
  assert(this->delayeditems.getLength() == 0);
  return this->getListContainerType( this->getPartNumber(name) );
}

/*!
  Returns list of node types which are allowed to be children of the
  list container \a part.
*/
const SoTypeList &
SoNodekitCatalog::getListItemTypes(int part) const
{
  assert( this->delayeditems.getLength() == 0);
  assert( (part >= 0 && part < this->getNumEntries()) &&
          "invalid part");
  assert( (this->items[part]->islist) && 
          "part is not a list container" );

  return this->items[part]->itemtypeslist;
}

/*!
  Returns list of node types which are allowed to be children of the
  named list container part.
*/
const SoTypeList &
SoNodekitCatalog::getListItemTypes(const SbName & name) const
{
  assert(this->delayeditems.getLength() == 0);
  return this->getListItemTypes( this->getPartNumber(name) );
}

/*!
  Returns \c TRUE if \a part is visible and publicly available for
  queries and modifications, \c FALSE if \a part is hidden.
*/
SbBool
SoNodekitCatalog::isPublic(int part) const
{
  assert( this->delayeditems.getLength() == 0 );
  assert( (part >= 0 && part < this->getNumEntries()) && "invalid part" );
  
  return this->items[part]->ispublic;
}

/*!
  Returns \c TRUE if the part is visible and publicly available for
  queries and modifications, \c FALSE if it is hidden.
*/
SbBool
SoNodekitCatalog::isPublic(const SbName & name) const
{
  assert(this->delayeditems.getLength() == 0);
  return this->isPublic( this->getPartNumber(name) );
}

/*!
  Return a clone of this catalog. \a type will be used to set the type
  and default type values of the top level \c this entry.
*/
SoNodekitCatalog *
SoNodekitCatalog::clone(SoType type) const
{
  assert(this->delayeditems.getLength() == 0);

  SoNodekitCatalog * newcat = new SoNodekitCatalog;
  for (int i=0; i < this->items.getLength(); i++) {
    CatalogItem * olditem = this->items[i];
    CatalogItem * newitem = new CatalogItem(*olditem);
    if (i == 0) {
      newitem->type = type;
      newitem->defaulttype = type;
    }
    // This is the only element in CatalogItem which can't be bitwise
    // copied.
    newitem->itemtypeslist = olditem->itemtypeslist;
    newcat->items.append(newitem);
  }
  return newcat;
}

static void SoNodekitCatalogPropagateDefaultInit( SoNodekitCatalog * pthis )
{

#if COIN_DEBUG && 0
  SoDebugError::postInfo("SoNodekitCatalogPropagateDefaultInit",
                         "sanitizing catalog" );
#endif

  for( int i = pthis->getNumEntries()-1; i > 0; --i ){
    if( !pthis->isNullByDefault( i ) ){
      SbName parent = pthis->getParentName( i );
      if( pthis->isNullByDefault( parent ) )
        pthis->setNullByDefault( parent, FALSE );
    }
  }
}
                                               

/*!
  Add a new entry to the catalog. Returns \c TRUE if add was OK.
*/
SbBool
SoNodekitCatalog::addEntry(const SbName & name, SoType type,
                           SoType defaulttype, SbBool isdefaultnull,
                           const SbName & parentname,
                           const SbName & rightsiblingname,
                           SbBool islist, SoType listcontainertype,
                           SoType listitemtype, SbBool ispublic)
{
  // The elements of a nodekit catalog is conceptually ordered like a
  // tree, but implementation-wise we stuff them inside a list. This
  // will make it speedier to access the elements through preset part
  // number indices.
  //
  // The list is in the same order which we would get by continually
  // appending elements through a prefix traversal of the tree.

  // Note: this is a fix to make it possible to compile the
  // SO_KIT_ADD_CATALOG_ENTRY() etc macros under MS VisualC++ with ""
  // entries where you can specify blank , , entries under gcc.
  SbName parent = parentname;
  SbName rightsibling = rightsiblingname;
  if (parent[0] == '\"' && parent[1] == '\"') parent = "";
  if (rightsibling[0] == '\"' && rightsibling[1] == '\"') rightsibling = "";


#if COIN_DEBUG && 0
  SoDebugError::postInfo("SoNodekitCatalog::addEntry",
                         "new entry: \"%s\"", name.getString());
#endif

  CC_GLOBAL_LOCK;
  if (!this->hasEntry(name)) {
    assert((name != "") && "Empty name not allowed");
    assert((this->getPartNumber( this->items, name ) == SO_CATALOG_NAME_NOT_FOUND ) && "partname already in use" );
    assert(this->getPartNumber( this->delayeditems, name ) == SO_CATALOG_NAME_NOT_FOUND && "partname already in use" );
    assert( (parent!="" || this->getNumEntries() == 0) && "need a parent name" );
    assert( (type != SoType::badType()) && "bad type" );
    assert( (defaulttype != SoType::badType()) && "bad default type" );
    
    // NOTE: !(A ^ B) <=> !A v !B
    assert( (!islist || (listcontainertype != SoType::badType())) && 
            "bad list container type");
    assert( (!islist || (listitemtype != SoType::badType())) &&
            "bad list item type" );
    
    CatalogItem * newitem = new CatalogItem;
    newitem->name = name;
    newitem->type = type;
    newitem->defaulttype = defaulttype;
    newitem->isdefaultnull = isdefaultnull;
    newitem->parentname = parent;
    newitem->siblingname = rightsibling;
    newitem->islist = islist;
    newitem->containertype = listcontainertype;
    newitem->itemtypeslist.append(listitemtype);
    newitem->ispublic = ispublic;
    
    SbBool delay = FALSE;
    if (rightsibling != "" &&
        this->getPartNumber(this->items, rightsibling) == SO_CATALOG_NAME_NOT_FOUND) {
      delay = TRUE;
    }
    else if (parent != "" &&
             this->getPartNumber(this->items, parent) == SO_CATALOG_NAME_NOT_FOUND) {
      delay = TRUE;
    }  
    
    if (delay)
      this->delayeditems.append(newitem);
    else 
      this->reallyAddEntry(newitem);
    
    // Move elements from list of delayed inserts to "real" catalog
    // list, if possible.
    for (int i = 0; i < this->delayeditems.getLength(); i++) {
      const SbName & p = this->delayeditems[i]->parentname;
      const SbName & r = this->delayeditems[i]->siblingname;
      
      if (this->getPartNumber(this->items, p) != SO_CATALOG_NAME_NOT_FOUND &&
          ((r == "") || 
           (this->getPartNumber(this->items, r) != SO_CATALOG_NAME_NOT_FOUND))){
        
        this->reallyAddEntry(this->delayeditems[i]);
        this->delayeditems.remove(i);
        i = -1; // restart scan
        
#if COIN_DEBUG && 0
        SoDebugError::postInfo("SoNodekitCatalog::addEntry",
                               "fixed delayed item, %d item%s left",
                               this->delayeditems.getLength(),
                               this->delayeditems.getLength()==1 ? "" : "s");
#endif
      }
    }
    
    if( this->delayeditems.getLength() == 0 )
      SoNodekitCatalogPropagateDefaultInit( this );
  }
  CC_GLOBAL_UNLOCK;
  return TRUE;
}

// Add the item at the correct position in the entry list, where the
// argument "newitem" is guaranteed to have both parent and right
// sibling (if any) present in the catalog.
SbBool
SoNodekitCatalog::reallyAddEntry(CatalogItem * newitem)
{
  const int n = this->items.getLength();
  
  if (n == 0) {
    this->items.append(newitem);
    return TRUE;
  }

  int position = -1;
  for (int i = 0; i < n; i++) {
    if ((this->items[i]->parentname == newitem->parentname) &&
        (this->items[i]->siblingname == newitem->siblingname)) {
      // this might happen when extending the catalog of another nodekit
      this->items[i]->siblingname = newitem->name;
      position = i+1;
      break;
    }
  }
  if (position < 0) {
    position = 0;
    while (position < n &&
           (this->items[position]->name != newitem->siblingname ||
            this->items[position]->parentname != newitem->parentname)) position++;
    if (position == n) {
      // parent and sibling not found, insert item after the parent
      position = this->getPartNumber(this->items, newitem->parentname) + 1;
    }
  }
  if (position == this->items.getLength())
    this->items.append(newitem);
  else 
    this->items.insert(newitem, position);
  return TRUE;
}

/*!
  Add another allowable type for the given \a part. \a part must of course
  be a list container item.
*/
void
SoNodekitCatalog::addListItemType(int part, SoType type)
{
  assert(this->delayeditems.getLength() == 0);
  this->addListItemType(this->items, part, type);
}

/*!
  Add another allowable type for the \a name part. The part must of course
  be a list container.
*/
void
SoNodekitCatalog::addListItemType(const SbName & name, SoType type)
{
  CC_GLOBAL_LOCK;
  if (!this->hasListItemType(name, type)) {
    // FIXME: If a part name is invalid, this procedure bails out
    // elsewhere on an assert. The check and debug comment should be
    // superfluous? If it isn't, it should be possible to find a way to
    // write this that expresses the code intentions better. 
    // 20021029 rolvs
    
    if (!this->addListItemType(this->items, name, type) &&
        !this->addListItemType(this->delayeditems, name, type)) {
#if COIN_DEBUG
      SoDebugError::post("SoNodekitCatalog::addListItemType",
                         "invalid part name, \"%s\"", name.getString());
#endif
    }
  }
  CC_GLOBAL_UNLOCK;
}

/*!
  Set the type and default type of a part to be subtypes of the old
  types. Useful for "narrowing" the specification of a nodekit which
  inherits the catalog of a more generic nodekit superclass.
*/
void
SoNodekitCatalog::narrowTypes(const SbName & name,
                              SoType newtype, SoType newdefaulttype)
{
  CC_GLOBAL_LOCK;

  assert(this->delayeditems.getLength() == 0);

  int part = this->getPartNumber(name);

  assert( part != SO_CATALOG_NAME_NOT_FOUND &&
          "invalid part name" );
  assert( newtype.isDerivedFrom( this->items[part]->type ) &&
          "new type should be derived of old type" );
  assert( newdefaulttype.isDerivedFrom( this->items[part]->type ) &&
          "new type should be derived of old type" );

  this->items[part]->type = newtype;
  this->items[part]->defaulttype = newdefaulttype;

  CC_GLOBAL_UNLOCK;
}

/*!
  Change whether or not the part with the given \a name is created by
  default.
*/
void
SoNodekitCatalog::setNullByDefault(const SbName & name, SbBool nullbydefault)
{
  assert(this->delayeditems.getLength() == 0);

  int part = this->getPartNumber(name);
  assert( part != SO_CATALOG_NAME_NOT_FOUND &&
          "invalid part name" );

  this->items[part]->isdefaultnull = nullbydefault;
}

/*!
  Recursively search \a part number in catalog for the \a name part.

  The \a checked SoTypeList is just used as a placeholder to remember which
  nodekit class catalogs have already been scanned (or are being scanned)
  during the recursion. You should normally just pass in an empty list.
*/
SbBool
SoNodekitCatalog::recursiveSearch(int part, const SbName & name,
                                  SoTypeList * checked) const
{
  assert( this->delayeditems.getLength() == 0 );
  assert( part >= 0 && part < this->getNumEntries() &&
          "part index out of bounds");

  if ((part == 0) && (checked->find(this->getType(0)) == -1))
    checked->append(this->getType(0));

  int start = (part == 0) ? 1 : part;
  int end = (part == 0) ? this->getNumEntries()-1 : part;

  for (int i = start; i <= end; i++) {
    if (name == this->getName(i)) return TRUE;

    SoType parttype = this->getType(i);
    if (parttype.isDerivedFrom(SoBaseKit::getClassTypeId())) {
      if (checked->find(parttype) == -1) {
        checked->append(parttype);
        SoBaseKit * kit = (SoBaseKit *)parttype.createInstance();
        kit->ref();
        const SoNodekitCatalog * cat = kit->getNodekitCatalog();
        SbBool result = cat->recursiveSearch(0, name, checked);
        kit->unref();
        if (result) return TRUE;
      }
    }
  }

  return FALSE;
}

/*!
  Lists all catalog parts, which is useful for debugging.
*/
void
SoNodekitCatalog::printCheck(void) const
{
  int nritems = this->getNumEntries();
  fprintf(stdout, "catalog printout: number of entries = %d\n", nritems);
  for (int i=0; i < nritems; i++) {
    CatalogItem * item = this->items[i];

    fprintf(stdout,
            "#%d\n"
            "    name = %s, type = %s, defaultType = %s\n"
            "    nullByDefault = %d\n"
            "    parentName = %s\n"
            "    sibling = %s, listPart = %d\n",
            i, item->name.getString(),
            item->type == SoType::badType() ? "*bad*" : item->type.getName().getString(),
            item->defaulttype == SoType::badType() ? "*bad*" : item->defaulttype.getName().getString(),
            item->isdefaultnull, item->parentname.getString(),
            item->siblingname.getString(), item->islist);

    if (item->islist) {
      fprintf(stdout, "listItemTypes =");
      for (int j=0; j < item->itemtypeslist.getLength(); j++) {
        fprintf(stdout, " %s", item->itemtypeslist[j].getName().getString());
      }
      fprintf(stdout, "\n");
    }

    fprintf(stdout, "    publicPart = %d\n", item->ispublic);
  }
}

// Overloaded to work with both delayed and "real" list of entries.
int
SoNodekitCatalog::getPartNumber(const SbList<class CatalogItem *> & l,
                                const SbName & name) const
{
  int nritems = l.getLength();
  for (int i= 0; i < nritems; i++) {
    if (name == l[i]->name) return i;
  }
  return SO_CATALOG_NAME_NOT_FOUND;
}

// Overloaded to work with both delayed and "real" list of entries.
void
SoNodekitCatalog::addListItemType(const SbList<class CatalogItem *> & l,
                                  int part, SoType type)
{
  assert( part >= 0 && part < l.getLength() && 
          "invalid part number" );
  assert( type != SoType::badType() &&
          "dont add SoType::badType(), you stupid!");
  assert( l[part]->islist &&
          "type must be a list-item type" );
  assert( l[part]->itemtypeslist.find( type ) == -1 &&
          "trying to add item that already exists" );

  l[part]->itemtypeslist.append(type);
}

// Overloaded to work with both delayed and "real" list of entries.
SbBool
SoNodekitCatalog::addListItemType(const SbList<class CatalogItem *> & l,
                                  const SbName & name, SoType type)
{
  int part = this->getPartNumber(l, name);
  if (part == SO_CATALOG_NAME_NOT_FOUND) return FALSE;
  this->addListItemType(l, part, type);
  return TRUE;
}

/*!
  \internal
  \since Coin 2.3
*/
SbBool 
SoNodekitCatalog::hasEntry(const SbName & name) const
{ 
  if (this->getPartNumber(this->items, name) != SO_CATALOG_NAME_NOT_FOUND) return TRUE;
  if (this->getPartNumber(this->delayeditems, name) != SO_CATALOG_NAME_NOT_FOUND) return TRUE;
  return FALSE;
}

/*!
  \internal
  \since Coin 2.3
*/
SbBool 
SoNodekitCatalog::hasListItemType(const SbName & name, SoType type) const
{
  const SbList <class CatalogItem*> * lptr = &this->items;
  int part = this->getPartNumber(this->items, name);
  if (part == SO_CATALOG_NAME_NOT_FOUND) {
    lptr = &this->delayeditems;
    part = this->getPartNumber(this->delayeditems, name);
  }
  const SbList <class CatalogItem*> & l = *lptr;
  assert(part >= 0 && part < l.getLength() && 
         "invalid part number");
  assert(type != SoType::badType() &&
         "dont add SoType::badType(), you stupid!");
  assert(l[part]->islist &&
         "type must be a list-item type" );

  return l[part]->itemtypeslist.find( type ) != -1;
}

#endif // HAVE_NODEKITS
