/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2002     *
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


/*! \namespace App \class App::Document
This is besides the Application class the most important class in FreeCAD
It contains all the data of the opened, saved or newly created FreeCAD Document.
The Document manage the Undo and Redo mechanism and the linking of documents.

Note: the documents are not free objects. They are completly handled by the
App::Application. Only the Application can Open or destroy a document.

\section Exception Exception handling
As the document is the main data structure of FreeCAD we have to take a close
look on how Exceptions affect the integrity of the App::Document.

\section UndoRedo Undo Redo an Transactions
Undo Redo handling is one of the major mechanism of an document in terms of
user friendliness and speed (no one will wait for Undo too long).

\section Dependency Graph and dependency handling
The FreeCAD document handles the dependencies of its DocumentObjects with
an adjacence list. This gives the opportunity to calculate the shortest
recompute path. Also enables more complicated dependencies beyond trees.


@see App::Application
@see App::DocumentObject
*/



#include "PreCompiled.h"

#ifndef _PreComp_
# include <algorithm>
# include <sstream>
# include <climits>
#endif

#include <boost/graph/topological_sort.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/visitors.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/bind.hpp>
#include <boost/regex.hpp>
#include <boost/unordered_set.hpp>

#include <QCoreApplication>
#include <QCryptographicHash>


#include "Document.h"
#include "DocumentPy.h"
#include "Application.h"
#include "DocumentObject.h"
#include "PropertyLinks.h"

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/TimeInfo.h>
#include <Base/Interpreter.h>
#include <Base/Reader.h>
#include <Base/Writer.h>
#include <Base/Stream.h>
#include <Base/FileInfo.h>
#include <Base/Tools.h>
#include <Base/Uuid.h>

#include <zipios++/zipios-config.h>
#include <zipios++/zipfile.h>
#include <zipios++/zipinputstream.h>
#include <zipios++/zipoutputstream.h>
#include <zipios++/meta-iostreams.h>

#include "Application.h"
#include "Transactions.h"

using Base::Console;
using Base::streq;
using Base::Writer;
using namespace App;
using namespace std;
using namespace boost;
using namespace zipios;

#ifdef MemDebugOn
# define new DEBUG_CLIENTBLOCK
#endif

#if FC_DEBUG
#  define FC_LOGFEATUREUPDATE
#endif 

// typedef boost::property<boost::vertex_root_t, DocumentObject* > VertexProperty;
typedef boost::adjacency_list <
boost::vecS,           // class OutEdgeListS  : a Sequence or an AssociativeContainer
boost::vecS,           // class VertexListS   : a Sequence or a RandomAccessContainer
boost::directedS,      // class DirectedS     : This is a directed graph
boost::no_property,    // class VertexProperty:
boost::no_property,    // class EdgeProperty:
boost::no_property,    // class GraphProperty:
boost::listS           // class EdgeListS:
> DependencyList;
typedef boost::graph_traits<DependencyList> Traits;
typedef Traits::vertex_descriptor Vertex;
typedef Traits::edge_descriptor Edge;

namespace App {

// Pimpl class
struct DocumentP
{
    // Array to preserve the creation order of created objects
    std::vector<DocumentObject*> objectArray;
    std::map<std::string,DocumentObject*> objectMap;
    DocumentObject* activeObject;
    Transaction *activeUndoTransaction;
    Transaction *activeTransaction;
    int iTransactionMode;
    int iTransactionCount;
    std::map<int,Transaction*> mTransactions;
    std::map<Vertex,DocumentObject*> vertexMap;
    bool rollback;
    bool closable;
    int iUndoMode;
    unsigned int UndoMemSize;
    unsigned int UndoMaxStackSize;
    DependencyList DepList;
    std::map<DocumentObject*,Vertex> VertexObjectList;

    DocumentP() {
        activeObject = 0;
        activeUndoTransaction = 0;
        activeTransaction = 0;
        iTransactionMode = 0;
        iTransactionCount = 0;
        rollback = false;
        closable = true;
        iUndoMode = 0;
        UndoMemSize = 0;
        UndoMaxStackSize = 20;
    }
};

} // namespace App

PROPERTY_SOURCE(App::Document, App::PropertyContainer)

void Document::writeDependencyGraphViz(std::ostream &out)
{
    //  // caching vertex to DocObject
    //std::map<Vertex,DocumentObject*> VertexMap;
    //for(std::map<DocumentObject*,Vertex>::const_iterator It1= _DepConMap.begin();It1 != _DepConMap.end(); ++It1)
    //  VertexMap[It1->second] = It1->first;

    out << "digraph G {" << endl;
    out << "\tordering=out;" << endl;
    out << "\tnode [shape = box];" << endl;

    for (std::map<std::string,DocumentObject*>::const_iterator It = d->objectMap.begin(); It != d->objectMap.end();++It) {
        out << "\t" << It->first << ";" <<endl;
        std::vector<DocumentObject*> OutList = It->second->getOutList();
        for (std::vector<DocumentObject*>::const_iterator It2=OutList.begin();It2!=OutList.end();++It2)
            if (*It2)
                out << "\t" << It->first << "->" << (*It2)->getNameInDocument() << ";" <<endl;
    }

    /*
    graph_traits<DependencyList>::edge_iterator ei, ei_end;
    for (tie(ei,ei_end) = edges(_DepList); ei != ei_end; ++ei)
      out << "\t"
          << VertexMap[source(*ei, _DepList)]->getNameInDocument()
          << " -> "
          << VertexMap[target(*ei, _DepList)]->getNameInDocument()
          << ";" << endl;
    */
    out << "}" << endl;
}

//bool _has_cycle_dfs(const DependencyList & g, vertex_t u, default_color_type * color)
//{
//  color[u] = gray_color;
//  graph_traits < DependencyList >::adjacency_iterator vi, vi_end;
//  for (tie(vi, vi_end) = adjacent_vertices(u, g); vi != vi_end; ++vi)
//    if (color[*vi] == white_color)
//      if (has_cycle_dfs(g, *vi, color))
//        return true;            // cycle detected, return immediately
//      else if (color[*vi] == gray_color)        // *vi is an ancestor!
//        return true;
//  color[u] = black_color;
//  return false;
//}

bool Document::checkOnCycle(void)
{/*
  std::vector < default_color_type > color(num_vertices(_DepList), white_color);
  graph_traits < DependencyList >::vertex_iterator vi, vi_end;
  for (tie(vi, vi_end) = vertices(_DepList); vi != vi_end; ++vi)
    if (color[*vi] == white_color)
      if (_has_cycle_dfs(_DepList, *vi, &color[0]))
        return true; */
    return false;
}

bool Document::undo(void)
{
    if (d->iUndoMode) {
        if (d->activeUndoTransaction)
            commitTransaction();
        else if (mUndoTransactions.empty())
            return false;

        // redo
        d->activeUndoTransaction = new Transaction();
        d->activeUndoTransaction->Name = mUndoTransactions.back()->Name;

        // applying the undo
        mUndoTransactions.back()->apply(*this,false);

        // save the redo
        mRedoTransactions.push_back(d->activeUndoTransaction);
        d->activeUndoTransaction = 0;

        delete mUndoTransactions.back();
        mUndoTransactions.pop_back();

        signalUndo(*this);
        return true;
    }

    return false;
}

bool Document::redo(void)
{
    if (d->iUndoMode) {
        if (d->activeUndoTransaction)
            commitTransaction();

        assert(mRedoTransactions.size()!=0);

        // undo
        d->activeUndoTransaction = new Transaction();
        d->activeUndoTransaction->Name = mRedoTransactions.back()->Name;

        // do the redo
        mRedoTransactions.back()->apply(*this,true);
        mUndoTransactions.push_back(d->activeUndoTransaction);
        d->activeUndoTransaction = 0;

        delete mRedoTransactions.back();
        mRedoTransactions.pop_back();

        signalRedo(*this);
        return true;
    }

    return false;
}

std::vector<std::string> Document::getAvailableUndoNames() const
{
    std::vector<std::string> vList;
    if (d->activeUndoTransaction)
        vList.push_back(d->activeUndoTransaction->Name);
    for (std::list<Transaction*>::const_reverse_iterator It=mUndoTransactions.rbegin();It!=mUndoTransactions.rend();++It)
        vList.push_back((**It).Name);
    return vList;
}

std::vector<std::string> Document::getAvailableRedoNames() const
{
    std::vector<std::string> vList;
    for (std::list<Transaction*>::const_reverse_iterator It=mRedoTransactions.rbegin();It!=mRedoTransactions.rend();++It)
        vList.push_back((**It).Name);
    return vList;
}

void Document::openTransaction(const char* name)
{
    if (d->iUndoMode) {
        if (d->activeUndoTransaction)
            commitTransaction();
        _clearRedos();

        d->activeUndoTransaction = new Transaction();
        if (name)
            d->activeUndoTransaction->Name = name;
        else
            d->activeUndoTransaction->Name = "<empty>";
    }
}

void Document::_checkTransaction(DocumentObject* pcObject)
{
    // if the undo is active but no transaction open, open one!
    if (d->iUndoMode) {
        if (!d->activeUndoTransaction) {
            // When the object is going to be deleted we have to check if it has already been added to
            // the undo transactions
            std::list<Transaction*>::iterator it;
            for (it = mUndoTransactions.begin(); it != mUndoTransactions.end(); ++it) {
                if ((*it)->hasObject(pcObject)) {
                    openTransaction();
                    break;
                }
            }
        }
    }
}

void Document::_clearRedos()
{
    while (!mRedoTransactions.empty()) {
        delete mRedoTransactions.back();
        mRedoTransactions.pop_back();
    }
}

void Document::commitTransaction()
{
    if (d->activeUndoTransaction) {
        mUndoTransactions.push_back(d->activeUndoTransaction);
        d->activeUndoTransaction = 0;
        // check the stack for the limits
        if(mUndoTransactions.size() > d->UndoMaxStackSize){
            delete mUndoTransactions.front();
            mUndoTransactions.pop_front();
        }
    }
}

void Document::abortTransaction()
{
    if (d->activeUndoTransaction) {
        d->rollback = true;
        // applying the so far made changes
        d->activeUndoTransaction->apply(*this,false);
        d->rollback = false;

        // destroy the undo
        delete d->activeUndoTransaction;
        d->activeUndoTransaction = 0;
    }
}

bool Document::hasPendingTransaction() const
{
    if (d->activeUndoTransaction)
        return true;
    else
        return false;
}

void Document::clearUndos()
{
    if (d->activeUndoTransaction)
        commitTransaction();

    // When cleaning up the undo stack we must delete the transactions from front
    // to back because a document object can appear in several transactions but
    // once removed from the document the object can never ever appear in any later
    // transaction. Since the document object may be also deleted when the transaction
    // is deleted we must make sure not access an object once it's destroyed. Thus, we
    // go from front to back and not the other way round.
    while (!mUndoTransactions.empty()) {
        delete mUndoTransactions.front();
        mUndoTransactions.pop_front();
    }
    //while (!mUndoTransactions.empty()) {
    //    delete mUndoTransactions.back();
    //    mUndoTransactions.pop_back();
    //}

    _clearRedos();
}

int Document::getAvailableUndos() const
{
    if (d->activeUndoTransaction)
        return static_cast<int>(mUndoTransactions.size() + 1);
    else
        return static_cast<int>(mUndoTransactions.size());
}

int Document::getAvailableRedos() const
{
    return static_cast<int>(mRedoTransactions.size());
}

void Document::setUndoMode(int iMode)
{
    if (d->iUndoMode && !iMode)
        clearUndos();

    d->iUndoMode = iMode;
}

int Document::getUndoMode(void) const
{
    return d->iUndoMode;
}

unsigned int Document::getUndoMemSize (void) const
{
    return d->UndoMemSize;
}

void Document::setUndoLimit(unsigned int UndoMemSize)
{
    d->UndoMemSize = UndoMemSize;
}

void Document::setMaxUndoStackSize(unsigned int UndoMaxStackSize)
{
     d->UndoMaxStackSize = UndoMaxStackSize;
}

unsigned int Document::getMaxUndoStackSize(void)const
{
    return d->UndoMaxStackSize;
}

void Document::onChanged(const Property* prop)
{
    // the Name property is a label for display purposes
    if (prop == &Label) {
        App::GetApplication().signalRelabelDocument(*this);
    }
    else if (prop == &Uid) {
        std::string new_dir = getTransientDirectoryName(this->Uid.getValueStr(),this->FileName.getStrValue());
        std::string old_dir = this->TransientDir.getStrValue();
        Base::FileInfo TransDirNew(new_dir);
        Base::FileInfo TransDirOld(old_dir);
        // this directory should not exist
        if (!TransDirNew.exists()) {
            if (TransDirOld.exists()) {
                if (!TransDirOld.renameFile(new_dir.c_str()))
                    Base::Console().Warning("Failed to rename '%s' to '%s'\n", old_dir.c_str(), new_dir.c_str());
                else
                    this->TransientDir.setValue(new_dir);
            }
            else {
                if (!TransDirNew.createDirectory())
                    Base::Console().Warning("Failed to create '%s'\n", new_dir.c_str());
                else
                    this->TransientDir.setValue(new_dir);
            }
        }
        // when reloading an existing document the transient directory doesn't change
        // so we must avoid to generate a new uuid
        else if (TransDirNew.filePath() != TransDirOld.filePath()) {
            // make sure that the uuid is unique
            std::string uuid = this->Uid.getValueStr();
            Base::Uuid id;
            Base::Console().Warning("Document with the UUID '%s' already exists, change to '%s'\n",
                                    uuid.c_str(), id.getValue().c_str());
            // recursive call of onChanged()
            this->Uid.setValue(id);
        }
    }
}

void Document::onBeforeChangeProperty(const DocumentObject *Who, const Property *What)
{
    if (d->activeUndoTransaction && !d->rollback)
        d->activeUndoTransaction->addObjectChange(Who,What);
}

void Document::onChangedProperty(const DocumentObject *Who, const Property *What)
{
    if (d->activeTransaction && !d->rollback)
        d->activeTransaction->addObjectChange(Who,What);
    signalChangedObject(*Who, *What);
}

void Document::setTransactionMode(int iMode)
{
    /*  if(_iTransactionMode == 0 && iMode == 1)
        beginTransaction();

      if(activTransaction && iMode == 0)
        endTransaction();
      */
    d->iTransactionMode = iMode;
}

#if 0
/// starts a new transaction
int  Document::beginTransaction(void)
{
    if (activTransaction)
        endTransaction();

    iTransactionCount++;

    activTransaction = new Transaction(iTransactionCount);
    d->mTransactions[iTransactionCount] = activTransaction;

    return iTransactionCount;
}

/// revert all changes to the document since beginTransaction()
void Document::rollbackTransaction(void)
{
    // ToDo
    assert(0);
    endTransaction();
}

/// ends the open Transaction
int  Document::endTransaction(void)
{
    activTransaction = 0;
    return iTransactionCount;
}

/// returns the named Transaction
const Transaction *Document::getTransaction(int pos) const
{
    if (pos == -1)
        return activTransaction;
    else {
        std::map<int,Transaction*>::const_iterator Pos(d->mTransactions.find(pos));
        if (Pos != d->mTransactions.end())
            return Pos->second;
        else
            return 0;
    }
}
#endif

//--------------------------------------------------------------------------
// constructor
//--------------------------------------------------------------------------
Document::Document(void)
{
    // Remark: In a constructor we should never increment a Python object as we cannot be sure
    // if the Python interpreter gets a reference of it. E.g. if we increment but Python don't
    // get a reference then the object wouldn't get deleted in the destructor.
    // So, we must increment only if the interpreter gets a reference.
    // Remark: We force the document Python object to own the DocumentPy instance, thus we don't
    // have to care about ref counting any more.
    DocumentPythonObject = Py::Object(new DocumentPy(this), true);
    d = new DocumentP;

#ifdef FC_LOGUPDATECHAIN
    Console().Log("+App::Document: %p\n",this);
#endif

    ADD_PROPERTY_TYPE(Label,("Unnamed"),0,Prop_None,"The name of the document");
    ADD_PROPERTY_TYPE(FileName,(""),0,Prop_ReadOnly,"The path to the file where the document is saved to");
    ADD_PROPERTY_TYPE(CreatedBy,(""),0,Prop_None,"The creator of the document");
    ADD_PROPERTY_TYPE(CreationDate,(Base::TimeInfo::currentDateTimeString()),0,Prop_ReadOnly,"Date of creation");
    ADD_PROPERTY_TYPE(LastModifiedBy,(""),0,Prop_None,0);
    ADD_PROPERTY_TYPE(LastModifiedDate,("Unknown"),0,Prop_ReadOnly,"Date of last modification");
    ADD_PROPERTY_TYPE(Company,(""),0,Prop_None,"Additional tag to save the the name of the company");
    ADD_PROPERTY_TYPE(Comment,(""),0,Prop_None,"Additional tag to save a comment");
    ADD_PROPERTY_TYPE(Meta,(),0,Prop_None,"Map with additional meta information");
    ADD_PROPERTY_TYPE(Material,(),0,Prop_None,"Map with material properties");
    // create the uuid for the document
    Base::Uuid id;
    ADD_PROPERTY_TYPE(Id,(""),0,Prop_None,"ID of the document");
    ADD_PROPERTY_TYPE(Uid,(id),0,Prop_ReadOnly,"UUID of the document");

    // license stuff
    ADD_PROPERTY_TYPE(License,("CC-BY 3.0"),0,Prop_None,"License string of the Item");
    ADD_PROPERTY_TYPE(LicenseURL,("http://creativecommons.org/licenses/by/3.0/"),0,Prop_None,"URL to the license text/contract");

    // this creates and sets 'TransientDir' in onChanged()
    ADD_PROPERTY_TYPE(TransientDir,(""),0,PropertyType(Prop_Transient|Prop_ReadOnly),
        "Transient directory, where the files live while the document is open");
    Uid.touch();
}

Document::~Document()
{
#ifdef FC_LOGUPDATECHAIN
    Console().Log("-App::Document: %s %p\n",getName(), this);
#endif

    clearUndos();

    std::map<std::string,DocumentObject*>::iterator it;

#ifdef FC_LOGUPDATECHAIN
    Console().Log("-Delete Features of %s \n",getName());
#endif

    d->objectArray.clear();
    for (it = d->objectMap.begin(); it != d->objectMap.end(); ++it) {
        delete(it->second);
    }

    // Remark: The API of Py::Object has been changed to set whether the wrapper owns the passed
    // Python object or not. In the constructor we forced the wrapper to own the object so we need
    // not to dec'ref the Python object any more.
    // But we must still invalidate the Python object because it doesn't need to be
    // destructed right now because the interpreter can own several references to it.
    Base::PyObjectBase* doc = (Base::PyObjectBase*)DocumentPythonObject.ptr();
    // Call before decrementing the reference counter, otherwise a heap error can occur
    doc->setInvalid();

    // remove Transient directory
    Base::FileInfo TransDir(TransientDir.getValue());
    TransDir.deleteDirectoryRecursive();
    delete d;
}

std::string Document::getTransientDirectoryName(const std::string& uuid, const std::string& filename) const
{
    // Create a directory name of the form: {ExeName}_Doc_{UUID}_{HASH}_{PID}
    std::stringstream s;
    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(filename.c_str(), filename.size());
    s << Base::FileInfo::getTempPath() << GetApplication().getExecutableName()
      << "_Doc_" << uuid
      << "_" << hash.result().toHex().left(6).constData()
      << "_" << QCoreApplication::applicationPid();
    return s.str();
}

//--------------------------------------------------------------------------
// Exported functions
//--------------------------------------------------------------------------

void Document::Save (Base::Writer &writer) const
{
    writer.Stream() << "<?xml version='1.0' encoding='utf-8'?>" << endl
    << "<!--" << endl
    << " FreeCAD Document, see http://www.freecadweb.org for more information..." << endl
    << "-->" << endl;

    writer.Stream() << "<Document SchemaVersion=\"4\" ProgramVersion=\""
                    << App::Application::Config()["BuildVersionMajor"] << "."
                    << App::Application::Config()["BuildVersionMinor"] << "R"
                    << App::Application::Config()["BuildRevision"]
                    << "\" FileVersion=\"" << writer.getFileVersion() << "\">" << endl;

    PropertyContainer::Save(writer);

    // writing the features types
    writeObjects(d->objectArray, writer);
    writer.Stream() << "</Document>" << endl;
}

void Document::Restore(Base::XMLReader &reader)
{
    int i,Cnt;
    reader.readElement("Document");
    long scheme = reader.getAttributeAsInteger("SchemaVersion");
    reader.DocumentSchema = scheme;
    if (reader.hasAttribute("ProgramVersion")) {
        reader.ProgramVersion = reader.getAttribute("ProgramVersion");
    } else {
        reader.ProgramVersion = "pre-0.14";
    }
    if (reader.hasAttribute("FileVersion")) {
        reader.FileVersion = reader.getAttributeAsUnsigned("FileVersion");
    } else {
        reader.FileVersion = 0;
    }

    // When this document was created the FileName and Label properties
    // were set to the absolute path or file name, respectively. To save
    // the document to the file it was loaded from or to show the file name
    // in the tree view we must restore them after loading the file because
    // they will be overridden.
    // Note: This does not affect the internal name of the document in any way
    // that is kept in Application.
    std::string FilePath = FileName.getValue();
    std::string DocLabel = Label.getValue();

    // read the Document Properties, when reading in Uid the transient directory gets renamed automatically
    PropertyContainer::Restore(reader);

    // We must restore the correct 'FileName' property again because the stored
    // value could be invalid.
    FileName.setValue(FilePath.c_str());
    Label.setValue(DocLabel.c_str());

    // SchemeVersion "2"
    if ( scheme == 2 ) {
        // read the feature types
        reader.readElement("Features");
        Cnt = reader.getAttributeAsInteger("Count");
        for (i=0 ;i<Cnt ;i++) {
            reader.readElement("Feature");
            string type = reader.getAttribute("type");
            string name = reader.getAttribute("name");

            try {
                addObject(type.c_str(),name.c_str());
            }
            catch ( Base::Exception& ) {
                Base::Console().Message("Cannot create object '%s'\n", name.c_str());
            }
        }
        reader.readEndElement("Features");

        // read the features itself
        reader.readElement("FeatureData");
        Cnt = reader.getAttributeAsInteger("Count");
        for (i=0 ;i<Cnt ;i++) {
            reader.readElement("Feature");
            string name = reader.getAttribute("name");
            DocumentObject* pObj = getObject(name.c_str());
            if (pObj) { // check if this feature has been registered
                pObj->StatusBits.set(4);
                pObj->Restore(reader);
                pObj->StatusBits.reset(4);
            }
            reader.readEndElement("Feature");
        }
        reader.readEndElement("FeatureData");
    } // SchemeVersion "3" or higher
    else if ( scheme >= 3 ) {
        // read the feature types
        readObjects(reader);
    }

    reader.readEndElement("Document");
}

void Document::exportObjects(const std::vector<App::DocumentObject*>& obj,
                             std::ostream& out)
{
    Base::ZipWriter writer(out);
    writer.putNextEntry("Document.xml");
    writer.Stream() << "<?xml version='1.0' encoding='utf-8'?>" << endl;
    writer.Stream() << "<Document SchemaVersion=\"4\" ProgramVersion=\""
                        << App::Application::Config()["BuildVersionMajor"] << "."
                        << App::Application::Config()["BuildVersionMinor"] << "R"
                        << App::Application::Config()["BuildRevision"]
                        << "\" FileVersion=\"1\">" << endl;
    // Add this block to have the same layout as for normal documents
    writer.Stream() << "<Properties Count=\"0\">" << endl;
    writer.Stream() << "</Properties>" << endl;

    // writing the object types
    writeObjects(obj, writer);
    writer.Stream() << "</Document>" << endl;

    // Hook for others to add further data.
    signalExportObjects(obj, writer);

    // write additional files
    writer.writeFiles();
}

void Document::writeObjects(const std::vector<App::DocumentObject*>& obj,
                            Base::Writer &writer) const
{
    // writing the features types
    writer.incInd(); // indention for 'Objects count'
    writer.Stream() << writer.ind() << "<Objects Count=\"" << obj.size() <<"\">" << endl;

    writer.incInd(); // indention for 'Object type'
    std::vector<DocumentObject*>::const_iterator it;
    for (it = obj.begin(); it != obj.end(); ++it) {
        writer.Stream() << writer.ind() << "<Object "
        << "type=\"" << (*it)->getTypeId().getName() << "\" "
        << "name=\"" << (*it)->getNameInDocument()       << "\" "
        << "/>" << endl;
    }

    writer.decInd();  // indention for 'Object type'
    writer.Stream() << writer.ind() << "</Objects>" << endl;

    // writing the features itself
    writer.Stream() << writer.ind() << "<ObjectData Count=\"" << obj.size() <<"\">" << endl;

    writer.incInd(); // indention for 'Object name'
    for (it = obj.begin(); it != obj.end(); ++it) {
        writer.Stream() << writer.ind() << "<Object name=\"" << (*it)->getNameInDocument() << "\">" << endl;
        (*it)->Save(writer);
        writer.Stream() << writer.ind() << "</Object>" << endl;
    }

    writer.decInd(); // indention for 'Object name'
    writer.Stream() << writer.ind() << "</ObjectData>" << endl;
    writer.decInd();  // indention for 'Objects count'
}

std::vector<App::DocumentObject*>
Document::readObjects(Base::XMLReader& reader)
{
    std::vector<App::DocumentObject*> objs;

    // read the object types
    reader.readElement("Objects");
    int Cnt = reader.getAttributeAsInteger("Count");
    for (int i=0 ;i<Cnt ;i++) {
        reader.readElement("Object");
        std::string type = reader.getAttribute("type");
        std::string name = reader.getAttribute("name");

        try {
            // Use name from XML as is and do NOT remove trailing digits because
            // otherwise we may cause a dependency to itself
            // Example: Object 'Cut001' references object 'Cut' and removing the
            // digits we make an object 'Cut' referencing itself.
            App::DocumentObject* obj = addObject(type.c_str(),name.c_str());
            if (obj) {
                objs.push_back(obj);
                // use this name for the later access because an object with
                // the given name may already exist
                reader.addName(name.c_str(), obj->getNameInDocument());
            }
        }
        catch (const Base::Exception& e) {
            Base::Console().Error("Cannot create object '%s': (%s)\n", name.c_str(), e.what());
        }
    }
    reader.readEndElement("Objects");

    // read the features itself
    reader.readElement("ObjectData");
    Cnt = reader.getAttributeAsInteger("Count");
    for (int i=0 ;i<Cnt ;i++) {
        reader.readElement("Object");
        std::string name = reader.getName(reader.getAttribute("name"));
        DocumentObject* pObj = getObject(name.c_str());
        if (pObj) { // check if this feature has been registered
            pObj->StatusBits.set(4);
            pObj->Restore(reader);
            pObj->StatusBits.reset(4);
        }
        reader.readEndElement("Object");
    }
    reader.readEndElement("ObjectData");

    return objs;
}

std::vector<App::DocumentObject*>
Document::importObjects(Base::XMLReader& reader)
{
    reader.readElement("Document");
    long scheme = reader.getAttributeAsInteger("SchemaVersion");
    reader.DocumentSchema = scheme;
    if (reader.hasAttribute("ProgramVersion")) {
        reader.ProgramVersion = reader.getAttribute("ProgramVersion");
    } else {
        reader.ProgramVersion = "pre-0.14";
    }
    if (reader.hasAttribute("FileVersion")) {
        reader.FileVersion = reader.getAttributeAsUnsigned("FileVersion");
    } else {
        reader.FileVersion = 0;
    }

    std::vector<App::DocumentObject*> objs = readObjects(reader);

    reader.readEndElement("Document");
    signalImportObjects(objs, reader);

    // reset all touched
    for (std::vector<DocumentObject*>::iterator it= objs.begin();it!=objs.end();++it) {
        (*it)->onDocumentRestored();
        (*it)->purgeTouched();
    }
    return objs;
}

unsigned int Document::getMemSize (void) const
{
    unsigned int size = 0;

    // size of the DocObjects in the document
    std::vector<DocumentObject*>::const_iterator it;
    for (it = d->objectArray.begin(); it != d->objectArray.end(); ++it)
        size += (*it)->getMemSize();

    // size of the document properties...
    size += PropertyContainer::getMemSize();

    // Undo Redo size
    size += getUndoMemSize();

    return size;
}

void Document::exportGraphviz(std::ostream& out)
{
    std::vector<std::string> names;
    names.reserve(d->objectMap.size());
    DependencyList DepList;
    std::map<DocumentObject*,Vertex> VertexObjectList;

    // Filling up the adjacency List
    for (std::map<std::string,DocumentObject*>::const_iterator It = d->objectMap.begin(); It != d->objectMap.end();++It) {
        // add the object as Vertex and remember the index
        VertexObjectList[It->second] = add_vertex(DepList);
        names.push_back(It->second->Label.getValue());
    }
    // add the edges
    for (std::map<std::string,DocumentObject*>::const_iterator It = d->objectMap.begin(); It != d->objectMap.end();++It) {
        std::vector<DocumentObject*> OutList = It->second->getOutList();
        for (std::vector<DocumentObject*>::const_iterator It2=OutList.begin();It2!=OutList.end();++It2) {
            if (*It2)
                add_edge(VertexObjectList[It->second],VertexObjectList[*It2],DepList);
        }
    }

    boost::write_graphviz(out, DepList, boost::make_label_writer(&(names[0])));
}

bool Document::saveAs(const char* file)
{
    Base::FileInfo fi(file);
    if (this->FileName.getStrValue() != file) {
        this->FileName.setValue(file);
        this->Label.setValue(fi.fileNamePure());
        this->Uid.touch(); // this forces a rename of the transient directory
    }

    return save();
}

// Save the document under the name it has been opened
bool Document::save (void)
{
    int compression = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/Document")->GetInt("CompressionLevel",3);

    if (*(FileName.getValue()) != '\0') {
        LastModifiedDate.setValue(Base::TimeInfo::currentDateTimeString());
        // make a tmp. file where to save the project data first and then rename to
        // the actual file name. This may be useful if overwriting an existing file
        // fails so that the data of the work up to now isn't lost.
        std::string uuid = Base::Uuid::createUuid();
        std::string fn = FileName.getValue();
        fn += "."; fn += uuid;
        Base::FileInfo tmp(fn);

        // open extra scope to close ZipWriter properly
        {
            Base::ofstream file(tmp, std::ios::out | std::ios::binary);
            Base::ZipWriter writer(file);

            writer.setComment("FreeCAD Document");
            writer.setLevel(compression);
            writer.putNextEntry("Document.xml");

            Document::Save(writer);

            // Special handling for Gui document.
            signalSaveDocument(writer);

            // write additional files
            writer.writeFiles();

            GetApplication().signalSaveDocument(*this);
        }

        // if saving the project data succeeded rename to the actual file name
        Base::FileInfo fi(FileName.getValue());
        if (fi.exists()) {
            bool backup = App::GetApplication().GetParameterGroupByPath
                ("User parameter:BaseApp/Preferences/Document")->GetBool("CreateBackupFiles",true);
            int count_bak = App::GetApplication().GetParameterGroupByPath
                ("User parameter:BaseApp/Preferences/Document")->GetInt("CountBackupFiles",1);
            if (backup) {
                int nSuff = 0;
                std::string fn = fi.fileName();
                Base::FileInfo di(fi.dirPath());
                std::vector<Base::FileInfo> backup;
                std::vector<Base::FileInfo> files = di.getDirectoryContent();
                for (std::vector<Base::FileInfo>::iterator it = files.begin(); it != files.end(); ++it) {
                    std::string file = it->fileName();
                    if (file.substr(0,fn.length()) == fn) {
                        // starts with the same file name
                        std::string suf(file.substr(fn.length()));
                        if (suf.size() > 0) {
                            std::string::size_type nPos = suf.find_first_not_of("0123456789");
                            if (nPos==std::string::npos) {
                                // store all backup files
                                backup.push_back(*it);
                                nSuff = std::max<int>(nSuff, std::atol(suf.c_str()));
                            }
                        }
                    }
                }

                if (!backup.empty() && (int)backup.size() >= count_bak) {
                    // delete the oldest backup file we found
                    Base::FileInfo del = backup.front();
                    for (std::vector<Base::FileInfo>::iterator it = backup.begin(); it != backup.end(); ++it) {
                        if (it->lastModified() < del.lastModified())
                            del = *it;
                    }

                    del.deleteFile();
                    fn = del.filePath();
                }
                else {
                    // create a new backup file
                    std::stringstream str;
                    str << fi.filePath() << (nSuff + 1);
                    fn = str.str();
                }

                fi.renameFile(fn.c_str());
            }
            else {
                fi.deleteFile();
            }
        }
        if (tmp.renameFile(FileName.getValue()) == false)
            Base::Console().Warning("Cannot rename file from '%s' to '%s'\n",
            fn.c_str(), FileName.getValue());

        return true;
    }

    return false;
}

// Open the document
void Document::restore (void)
{
    // clean up if the document is not empty
    // !TODO mind exeptions while restoring!
    clearUndos();
    for (std::vector<DocumentObject*>::iterator obj = d->objectArray.begin(); obj != d->objectArray.end(); ++obj) {
        signalDeletedObject(*(*obj));
        delete *obj;
    }
    d->objectArray.clear();
    d->objectMap.clear();
    d->activeObject = 0;

    Base::FileInfo fi(FileName.getValue());
    Base::ifstream file(fi, std::ios::in | std::ios::binary);
    std::streambuf* buf = file.rdbuf();
    std::streamoff size = buf->pubseekoff(0, std::ios::end, std::ios::in);
    buf->pubseekoff(0, std::ios::beg, std::ios::in);
    if (size < 22) // an empty zip archive has 22 bytes
        throw Base::FileException("Invalid project file",FileName.getValue());

    zipios::ZipInputStream zipstream(file);
    Base::XMLReader reader(FileName.getValue(), zipstream);

    if (!reader.isValid())
        throw Base::FileException("Error reading compression file",FileName.getValue());

    GetApplication().signalStartRestoreDocument(*this);

    try {
        Document::Restore(reader);
    }
    catch (const Base::Exception& e) {
        Base::Console().Error("Invalid Document.xml: %s\n", e.what());
    }

    // Special handling for Gui document, the view representations must already
    // exist, what is done in Restore().
    // Note: This file doesn't need to be available if the document has been created
    // without GUI. But if available then follow after all data files of the App document.
    signalRestoreDocument(reader);
    reader.readFiles(zipstream);
    
    // reset all touched
    for (std::map<std::string,DocumentObject*>::iterator It= d->objectMap.begin();It!=d->objectMap.end();++It) {
        It->second->onDocumentRestored();
        It->second->purgeTouched();
    }

    GetApplication().signalFinishRestoreDocument(*this);
}

bool Document::isSaved() const
{
    std::string name = FileName.getValue();
    return !name.empty();
}

/** Label is the visible name of a document shown e.g. in the windows title
 * or in the tree view. The label almost (but not always e.g. if you manually change it)
 * matches with the file name where the document is stored to.
 * In contrast to Label the method getName() returns the internal name of the document that only
 * matches with Label when loading or creating a document because then both are set to the same value.
 * Since the internal name cannot be changed during runtime it must differ from the Label after saving 
 * the document the first time or saving it under a new file name.
 * @ note More than one document can have the same label name.
 * @ note The internal is always guaranteed to be unique because @ref Application::newDocument() checks
 * for a document with the same name and makes it unique if needed. Hence you cannot rely on that the
 * internal name matches with the name you passed to Application::newDoument(). You should use the
 * method getName() instead.
 */
const char* Document::getName() const
{
    return GetApplication().getDocumentName(this);
}

/// Remove all modifications. After this call The document becomes valid again.
void Document::purgeTouched()
{
    for (std::vector<DocumentObject*>::iterator It = d->objectArray.begin();It != d->objectArray.end();++It)
        (*It)->purgeTouched();
}

bool Document::isTouched() const
{
    for (std::vector<DocumentObject*>::const_iterator It = d->objectArray.begin();It != d->objectArray.end();++It)
        if ((*It)->isTouched())
            return true;
    return false;
}

vector<DocumentObject*> Document::getTouched(void) const
{
    vector<DocumentObject*> result;

    for (std::vector<DocumentObject*>::const_iterator It = d->objectArray.begin();It != d->objectArray.end();++It)
        if ((*It)->isTouched())
            result.push_back(*It);

    return result;
}

void Document::setClosable(bool c)
{
    d->closable = c;
}

bool Document::isClosable() const
{
    return d->closable;
}

int Document::countObjects(void) const
{
   return static_cast<int>(d->objectArray.size());
}

std::vector<App::DocumentObject*> Document::getInList(const DocumentObject* me) const
{
    // result list
    std::vector<App::DocumentObject*> result;
    // go through all objects
    for (std::map<std::string,DocumentObject*>::const_iterator It = d->objectMap.begin(); It != d->objectMap.end();++It) {
        // get the outList and search if me is in that list
        std::vector<DocumentObject*> OutList = It->second->getOutList();
        for (std::vector<DocumentObject*>::const_iterator It2=OutList.begin();It2!=OutList.end();++It2)
            if (*It2 && *It2 == me)
                // add the parent object
                result.push_back(It->second);
    }
    return result;
}

std::vector<App::DocumentObject*>
Document::getDependencyList(const std::vector<App::DocumentObject*>& objs) const
{
    DependencyList DepList;
    std::map<DocumentObject*,Vertex> ObjectMap;
    std::map<Vertex,DocumentObject*> VertexMap;

    // Filling up the adjacency List
    for (std::map<std::string,DocumentObject*>::const_iterator It = d->objectMap.begin(); It != d->objectMap.end();++It) {
        // add the object as Vertex and remember the index
        Vertex v = add_vertex(DepList);
        ObjectMap[It->second] = v;
        VertexMap[v] = It->second;
    }
    // add the edges
    for (std::map<std::string,DocumentObject*>::const_iterator It = d->objectMap.begin(); It != d->objectMap.end();++It) {
        std::vector<DocumentObject*> OutList = It->second->getOutList();
        for (std::vector<DocumentObject*>::const_iterator It2=OutList.begin();It2!=OutList.end();++It2) {
            if (*It2)
                add_edge(ObjectMap[It->second],ObjectMap[*It2],DepList);
        }
    }

    std::list<Vertex> make_order;
    DependencyList::out_edge_iterator j, jend;

    try {
        // this sort gives the execute
        boost::topological_sort(DepList, std::front_inserter(make_order));
    }
    catch (const std::exception&) {
        return std::vector<App::DocumentObject*>();
    }

    //std::vector<App::DocumentObject*> out;
    boost::unordered_set<App::DocumentObject*> out;
    for (std::vector<App::DocumentObject*>::const_iterator it = objs.begin(); it != objs.end(); ++it) {
        std::map<DocumentObject*,Vertex>::iterator jt = ObjectMap.find(*it);
        // ok, object is part of this graph
        if (jt != ObjectMap.end()) {
            for (boost::tie(j, jend) = boost::out_edges(jt->second, DepList); j != jend; ++j) {
                out.insert(VertexMap[boost::target(*j, DepList)]);
            }
            out.insert(*it);
        }
    }

    std::vector<App::DocumentObject*> ary;
    ary.insert(ary.end(), out.begin(), out.end());
    return ary;
}

void Document::_rebuildDependencyList(void)
{
    d->VertexObjectList.clear();
    d->DepList.clear();
    // Filling up the adjacency List
    for (std::map<std::string,DocumentObject*>::const_iterator It = d->objectMap.begin(); It != d->objectMap.end();++It) {
        // add the object as Vertex and remember the index
        d->VertexObjectList[It->second] = add_vertex(d->DepList);
    }
    // add the edges
    for (std::map<std::string,DocumentObject*>::const_iterator It = d->objectMap.begin(); It != d->objectMap.end();++It) {
        std::vector<DocumentObject*> OutList = It->second->getOutList();
        for (std::vector<DocumentObject*>::const_iterator It2=OutList.begin();It2!=OutList.end();++It2) {
            if (*It2)
                add_edge(d->VertexObjectList[It->second],d->VertexObjectList[*It2],d->DepList);
        }
    }
}

void Document::recompute()
{
    // delete recompute log
    for( std::vector<App::DocumentObjectExecReturn*>::iterator it=_RecomputeLog.begin();it!=_RecomputeLog.end();++it)
        delete *it;
    _RecomputeLog.clear();

    // updates the dependency graph
    _rebuildDependencyList();

    std::list<Vertex> make_order;
    DependencyList::out_edge_iterator j, jend;

    try {
        // this sort gives the execute
        boost::topological_sort(d->DepList, std::front_inserter(make_order));
    }
    catch (const std::exception& e) {
        std::cerr << "Document::recompute: " << e.what() << std::endl;
        return;
    }

    // caching vertex to DocObject
    for (std::map<DocumentObject*,Vertex>::const_iterator It1= d->VertexObjectList.begin();It1 != d->VertexObjectList.end(); ++It1)
        d->vertexMap[It1->second] = It1->first;

#ifdef FC_LOGFEATUREUPDATE
    std::clog << "make ordering: " << std::endl;
#endif

    for (std::list<Vertex>::reverse_iterator i = make_order.rbegin();i != make_order.rend(); ++i) {
        DocumentObject* Cur = d->vertexMap[*i];
        if (!Cur) continue;
#ifdef FC_LOGFEATUREUPDATE
        std::clog << Cur->getNameInDocument() << " dep on: " ;
#endif
        bool NeedUpdate = false;

        // ask the object if it should be recomputed
        if (Cur->mustExecute() == 1)
            NeedUpdate = true;
        else {// if (Cur->mustExecute() == -1)
            // update if one of the dependencies is touched
            for (boost::tie(j, jend) = out_edges(*i, d->DepList); j != jend; ++j) {
                DocumentObject* Test = d->vertexMap[target(*j, d->DepList)];
                if (!Test) continue;
#ifdef FC_LOGFEATUREUPDATE
                std::clog << Test->getNameInDocument() << ", " ;
#endif
                if (Test->isTouched()) {
                    NeedUpdate = true;
                    break;
                }
            }
#ifdef FC_LOGFEATUREUPDATE
            std::clog << std::endl;
#endif
        }
        // if one touched recompute
        if (NeedUpdate) {
#ifdef FC_LOGFEATUREUPDATE
            std::clog << "Recompute" << std::endl;
#endif
            if (_recomputeFeature(Cur)) {
                // if somthing happen break execution of recompute
                d->vertexMap.clear();
                return;
            }
        }
    }

    // reset all touched
    for (std::map<Vertex,DocumentObject*>::iterator it = d->vertexMap.begin(); it != d->vertexMap.end(); ++it) {
        if (it->second)
            it->second->purgeTouched();
    }
    d->vertexMap.clear();
}

const char * Document::getErrorDescription(const App::DocumentObject*Obj) const
{
    for (std::vector<App::DocumentObjectExecReturn*>::const_iterator it=_RecomputeLog.begin();it!=_RecomputeLog.end();++it)
        if ((*it)->Which == Obj)
            return (*it)->Why.c_str();
    return 0;
}

// call the recompute of the Feature and handle the exceptions and errors.
bool Document::_recomputeFeature(DocumentObject* Feat)
{
#ifdef FC_LOGFEATUREUPDATE
    std::clog << "Solv: Executing Feature: " << Feat->getNameInDocument() << std::endl;;
#endif

    DocumentObjectExecReturn  *returnCode = 0;
    try {
        returnCode = Feat->recompute();
    }
    catch(Base::AbortException &e){
        e.ReportException();
        _RecomputeLog.push_back(new DocumentObjectExecReturn("User abort",Feat));
        Feat->setError();
        return true;
    }
    catch (const Base::MemoryException& e) {
        Base::Console().Error("Memory exception in feature '%s' thrown: %s\n",Feat->getNameInDocument(),e.what());
        _RecomputeLog.push_back(new DocumentObjectExecReturn("Out of memory exception",Feat));
        Feat->setError();
        return true;
    }
    catch (Base::Exception &e) {
        e.ReportException();
        _RecomputeLog.push_back(new DocumentObjectExecReturn(e.what(),Feat));
        Feat->setError();
        return false;
    }
    catch (std::exception &e) {
        Base::Console().Warning("exception in Feature \"%s\" thrown: %s\n",Feat->getNameInDocument(),e.what());
        _RecomputeLog.push_back(new DocumentObjectExecReturn(e.what(),Feat));
        Feat->setError();
        return false;
    }
#ifndef FC_DEBUG
    catch (...) {
        Base::Console().Error("App::Document::_RecomputeFeature(): Unknown exception in Feature \"%s\" thrown\n",Feat->getNameInDocument());
        _RecomputeLog.push_back(new DocumentObjectExecReturn("Unknown exeption!"));
        Feat->setError();
        return true;
    }
#endif

    // error code
    if (returnCode == DocumentObject::StdReturn) {
        Feat->resetError();
    }
    else {
        returnCode->Which = Feat;
        _RecomputeLog.push_back(returnCode);
        Base::Console().Error("%s\n",returnCode->Why.c_str());
        Feat->setError();
    }
    return false;
}

void Document::recomputeFeature(DocumentObject* Feat)
{
     // delete recompute log
    for( std::vector<App::DocumentObjectExecReturn*>::iterator it=_RecomputeLog.begin();it!=_RecomputeLog.end();++it)
        delete *it;
    _RecomputeLog.clear();

    _recomputeFeature(Feat);
}

DocumentObject * Document::addObject(const char* sType, const char* pObjectName)
{
    Base::BaseClass* base = static_cast<Base::BaseClass*>(Base::Type::createInstanceByName(sType,true));

    string ObjectName;
    if (!base)
        return 0;
    if (!base->getTypeId().isDerivedFrom(App::DocumentObject::getClassTypeId())) {
        delete base;
        std::stringstream str;
        str << "'" << sType << "' is not a document object type";
        throw Base::Exception(str.str());
    }

    App::DocumentObject* pcObject = static_cast<App::DocumentObject*>(base);
    pcObject->setDocument(this);

    // do no transactions if we do a rollback!
    if(!d->rollback){
        // Transaction stuff
        if (d->activeTransaction)
            d->activeTransaction->addObjectNew(pcObject);
        // Undo stuff
        if (d->activeUndoTransaction)
            d->activeUndoTransaction->addObjectDel(pcObject);
    }

    // get Unique name
    if (pObjectName && pObjectName[0] != '\0')
        ObjectName = getUniqueObjectName(pObjectName);
    else
        ObjectName = getUniqueObjectName(sType);


    d->activeObject = pcObject;

    // insert in the name map
    d->objectMap[ObjectName] = pcObject;
    // cache the pointer to the name string in the Object (for performance of DocumentObject::getNameInDocument())
    pcObject->pcNameInDocument = &(d->objectMap.find(ObjectName)->first);
    // insert in the vector
    d->objectArray.push_back(pcObject);
    // insert in the adjacence list and referenc through the ConectionMap
    //_DepConMap[pcObject] = add_vertex(_DepList);

    pcObject->Label.setValue( ObjectName );

    // mark the object as new (i.e. set status bit 2) and send the signal
    pcObject->StatusBits.set(2);
    signalNewObject(*pcObject);
    signalActivatedObject(*pcObject);

    // return the Object
    return pcObject;
}

void Document::_addObject(DocumentObject* pcObject, const char* pObjectName)
{
    d->objectMap[pObjectName] = pcObject;
    d->objectArray.push_back(pcObject);
    // cache the pointer to the name string in the Object (for performance of DocumentObject::getNameInDocument())
    pcObject->pcNameInDocument = &(d->objectMap.find(pObjectName)->first);

    // do no transactions if we do a rollback!
    if(!d->rollback){
        // Transaction stuff
        if (d->activeTransaction)
            d->activeTransaction->addObjectNew(pcObject);
        // Undo stuff
        if (d->activeUndoTransaction)
            d->activeUndoTransaction->addObjectDel(pcObject);
    }
    // send the signal
    signalNewObject(*pcObject);
}

/// Remove an object out of the document
void Document::remObject(const char* sName)
{
    std::map<std::string,DocumentObject*>::iterator pos = d->objectMap.find(sName);

    // name not found?
    if (pos == d->objectMap.end())
        return;

    _checkTransaction(pos->second);

    if (d->activeObject == pos->second)
        d->activeObject = 0;

    signalDeletedObject(*(pos->second));
    if (!d->vertexMap.empty()) {
        // recompute of document is running
        for (std::map<Vertex,DocumentObject*>::iterator it = d->vertexMap.begin(); it != d->vertexMap.end(); ++it) {
            if (it->second == pos->second) {
                it->second = 0; // just nullify the pointer
                break;
            }
        }
    }

    // Before deleting we must nullify all dependant objects
    breakDependency(pos->second, true);

    // do no transactions if we do a rollback!
    if(!d->rollback){

        // Transaction stuff
        if (d->activeTransaction)
            d->activeTransaction->addObjectDel(pos->second);

        // Undo stuff
        if (d->activeUndoTransaction) {
            // in this case transaction delete or save the object
            d->activeUndoTransaction->addObjectNew(pos->second);
            // set name cache false
            //pos->second->pcNameInDocument = 0;
        }
        else
            // if not saved in undo -> delete object
            delete pos->second;
    }

    for (std::vector<DocumentObject*>::iterator obj = d->objectArray.begin(); obj != d->objectArray.end(); ++obj) {
        if (*obj == pos->second) {
            d->objectArray.erase(obj);
            break;
        }
    }
    // remove from adjancy list
    //remove_vertex(_DepConMap[pos->second],_DepList);
    //_DepConMap.erase(pos->second);
    d->objectMap.erase(pos);
}

/// Remove an object out of the document (internal)
void Document::_remObject(DocumentObject* pcObject)
{
    _checkTransaction(pcObject);

    std::map<std::string,DocumentObject*>::iterator pos = d->objectMap.find(pcObject->getNameInDocument());

    if (d->activeObject == pcObject)
        d->activeObject = 0;

    signalDeletedObject(*pcObject);

    // do no transactions if we do a rollback!
    if(!d->rollback){
        // Transaction stuff
        if (d->activeTransaction)
            d->activeTransaction->addObjectDel(pcObject);

        // Undo stuff
        if (d->activeUndoTransaction)
            d->activeUndoTransaction->addObjectNew(pcObject);
    }
    // remove from map
    d->objectMap.erase(pos);
    //// set name cache false
    //pcObject->pcNameInDocument = 0;

    for (std::vector<DocumentObject*>::iterator it = d->objectArray.begin(); it != d->objectArray.end(); ++it) {
        if (*it == pcObject) {
            d->objectArray.erase(it);
            break;
        }
    }
}

void Document::breakDependency(DocumentObject* pcObject, bool clear)
{
    // Nullify all dependant objects
    for (std::map<std::string,DocumentObject*>::iterator it = d->objectMap.begin(); it != d->objectMap.end(); ++it) {
        std::map<std::string,App::Property*> Map;
        it->second->getPropertyMap(Map);
        // search for all properties that could have a link to the object
        for (std::map<std::string,App::Property*>::iterator pt = Map.begin(); pt != Map.end(); ++pt) {
            if (pt->second->getTypeId().isDerivedFrom(PropertyLink::getClassTypeId())) {
                PropertyLink* link = static_cast<PropertyLink*>(pt->second);
                if (link->getValue() == pcObject)
                    link->setValue(0);
                else if (link->getContainer() == pcObject && clear)
                    link->setValue(0);
            }
            else if (pt->second->getTypeId().isDerivedFrom(PropertyLinkSub::getClassTypeId())) {
                PropertyLinkSub* link = static_cast<PropertyLinkSub*>(pt->second);
                if (link->getValue() == pcObject)
                    link->setValue(0);
                else if (link->getContainer() == pcObject && clear)
                    link->setValue(0);
            }
            else if (pt->second->getTypeId().isDerivedFrom(PropertyLinkList::getClassTypeId())) {
                PropertyLinkList* link = static_cast<PropertyLinkList*>(pt->second);
                if (link->getContainer() == pcObject && clear) {
                    link->setValues(std::vector<DocumentObject*>());
                }
                else {
                    // copy the list (not the objects)
                    std::vector<DocumentObject*> linked = link->getValues();
                    for (std::vector<DocumentObject*>::iterator fIt = linked.begin(); fIt != linked.end(); ++fIt) {
                        if ((*fIt) == pcObject) {
                            // reassign the the list without the object to be deleted
                            linked.erase(fIt);
                            link->setValues(linked);
                            break;
                        }
                    }
                }
            }
            else if (pt->second->getTypeId().isDerivedFrom(PropertyLinkSubList::getClassTypeId())) {
                PropertyLinkSubList* link = static_cast<PropertyLinkSubList*>(pt->second);
                if (link->getContainer() == pcObject && clear) {
                    link->setValues(std::vector<DocumentObject*>(), std::vector<std::string>());
                }
                else {
                    const std::vector<DocumentObject*>& links = link->getValues();
                    const std::vector<std::string>& sub = link->getSubValues();
                    std::vector<DocumentObject*> newLinks;
                    std::vector<std::string> newSub;

                    if (std::find(links.begin(), links.end(), pcObject) != links.end()) {
                        std::vector<DocumentObject*>::const_iterator jt;
                        std::vector<std::string>::const_iterator kt;
                        for (jt = links.begin(),kt = sub.begin(); jt != links.end() && kt != sub.end(); ++jt, ++kt) {
                            if (*jt != pcObject) {
                                newLinks.push_back(*jt);
                                newSub.push_back(*kt);
                            }
                        }

                        link->setValues(newLinks, newSub);
                    }
                }
            }
        }
    }
}

DocumentObject* Document::_copyObject(DocumentObject* obj, std::map<DocumentObject*, 
                                      DocumentObject*>& copy_map, bool recursive,
                                      bool keepdigitsatend)
{
    if (!obj) return 0;
    // remove number from end to avoid lengthy names
    std::string objname = obj->getNameInDocument();
    if (!keepdigitsatend) {
        size_t lastpos = objname.length()-1;
        while (objname[lastpos] >= 48 && objname[lastpos] <= 57)
            lastpos--;
        objname = objname.substr(0, lastpos+1);
    }
    DocumentObject* copy = addObject(obj->getTypeId().getName(),objname.c_str());
    if (!copy) return 0;
    copy->addDynamicProperties(obj);

    copy_map[obj] = copy;

    std::map<std::string,App::Property*> props;
    copy->getPropertyMap(props);
    for (std::map<std::string,App::Property*>::iterator it = props.begin(); it != props.end(); ++it) {
        App::Property* prop = obj->getPropertyByName(it->first.c_str());
        if (prop && prop->getTypeId() == it->second->getTypeId()) {
            if (prop->getTypeId() == PropertyLink::getClassTypeId()) {
                DocumentObject* link = static_cast<PropertyLink*>(prop)->getValue();
                std::map<DocumentObject*, DocumentObject*>::iterator pt = copy_map.find(link);
                if (pt != copy_map.end()) {
                    // the object has already been copied
                    static_cast<PropertyLink*>(it->second)->setValue(pt->second);
                }
                else if (recursive) {
                    DocumentObject* link_copy = _copyObject(link, copy_map, recursive, keepdigitsatend);
                    copy_map[link] = link_copy;
                    static_cast<PropertyLink*>(it->second)->setValue(link_copy);
                }
                else if (link && link->getDocument() == this) {
                    //static_cast<PropertyLink*>(it->second)->setValue(link);
                }
            }
            else if (prop->getTypeId() == PropertyLinkList::getClassTypeId()) {
                std::vector<DocumentObject*> links = static_cast<PropertyLinkList*>(prop)->getValues();
                if (recursive) {
                    std::vector<DocumentObject*> links_copy;
                    for (std::vector<DocumentObject*>::iterator jt = links.begin(); jt != links.end(); ++jt) {
                        std::map<DocumentObject*, DocumentObject*>::iterator pt = copy_map.find(*jt);
                        if (pt != copy_map.end()) {
                            // the object has already been copied
                            links_copy.push_back(pt->second);
                        }
                        else {
                            links_copy.push_back(_copyObject(*jt, copy_map, recursive, keepdigitsatend));
                            copy_map[*jt] = links_copy.back();
                        }
                    }
                    static_cast<PropertyLinkList*>(it->second)->setValues(links_copy);
                }
                else {
                    std::vector<DocumentObject*> links_ref;
                    //for (std::vector<DocumentObject*>::iterator jt = links.begin(); jt != links.end(); ++jt) {
                    //    if ((*jt)->getDocument() == this)
                    //        links_ref.push_back(*jt);
                    //}
                    static_cast<PropertyLinkList*>(it->second)->setValues(links_ref);
                }
            }
            else {
                std::auto_ptr<Property> data(prop->Copy());
                if (data.get()) {
                    it->second->Paste(*data);
                }
            }
        }
    }

     // unmark to be not re-computed later
    copy->onFinishDuplicating();
    copy->purgeTouched();
    return copy;
}

DocumentObject* Document::copyObject(DocumentObject* obj, bool recursive,  bool keepdigitsatend)
{
    std::map<DocumentObject*, DocumentObject*> copy_map;
    DocumentObject* copy = _copyObject(obj, copy_map, recursive, keepdigitsatend);
    return copy;
}

DocumentObject* Document::moveObject(DocumentObject* obj, bool recursive)
{
    Document* that = obj->getDocument();
    if (that == this)
        return 0; // nothing todo

    // all object of the other document that refer to this object must be nullified
    that->breakDependency(obj, false);
    std::string objname = getUniqueObjectName(obj->getNameInDocument());
    that->_remObject(obj);
    this->_addObject(obj, objname.c_str());
    obj->setDocument(this);

    std::map<std::string,App::Property*> props;
    obj->getPropertyMap(props);
    for (std::map<std::string,App::Property*>::iterator it = props.begin(); it != props.end(); ++it) {
        if (it->second->getTypeId() == PropertyLink::getClassTypeId()) {
            DocumentObject* link = static_cast<PropertyLink*>(it->second)->getValue();
            if (recursive) {
                moveObject(link, recursive);
                static_cast<PropertyLink*>(it->second)->setValue(link);
            }
            else {
                static_cast<PropertyLink*>(it->second)->setValue(0);
            }
        }
        else if (it->second->getTypeId() == PropertyLinkList::getClassTypeId()) {
            std::vector<DocumentObject*> links = static_cast<PropertyLinkList*>(it->second)->getValues();
            if (recursive) {
                for (std::vector<DocumentObject*>::iterator jt = links.begin(); jt != links.end(); ++jt)
                    moveObject(*jt, recursive);
                static_cast<PropertyLinkList*>(it->second)->setValues(links);
            }
            else {
                static_cast<PropertyLinkList*>(it->second)->setValues(std::vector<DocumentObject*>());
            }
        }
    }

    return obj;
}

DocumentObject * Document::getActiveObject(void) const
{
    return d->activeObject;
}

DocumentObject * Document::getObject(const char *Name) const
{
    std::map<std::string,DocumentObject*>::const_iterator pos;

    pos = d->objectMap.find(Name);

    if (pos != d->objectMap.end())
        return pos->second;
    else
        return 0;
}

const char * Document::getObjectName(DocumentObject *pFeat) const
{
    std::map<std::string,DocumentObject*>::const_iterator pos;

    for (pos = d->objectMap.begin();pos != d->objectMap.end();++pos)
        if (pos->second == pFeat)
            return pos->first.c_str();

    return 0;
}

std::string Document::getUniqueObjectName(const char *Name) const
{
    if (!Name || *Name == '\0')
        return std::string();
    std::string CleanName = Base::Tools::getIdentifier(Name);

    // name in use?
    std::map<std::string,DocumentObject*>::const_iterator pos;
    pos = d->objectMap.find(CleanName);

    if (pos == d->objectMap.end()) {
        // if not, name is OK
        return CleanName;
    }
    else {
        std::vector<std::string> names;
        names.reserve(d->objectMap.size());
        for (pos = d->objectMap.begin();pos != d->objectMap.end();++pos) {
            names.push_back(pos->first);
        }
        return Base::Tools::getUniqueName(CleanName, names, 3);
    }
}

std::string Document::getStandardObjectName(const char *Name, int d) const
{
    std::vector<App::DocumentObject*> mm = getObjects();
    std::vector<std::string> labels;
    labels.reserve(mm.size());

    for (std::vector<App::DocumentObject*>::const_iterator it = mm.begin(); it != mm.end(); ++it) {
        std::string label = (*it)->Label.getValue();
        labels.push_back(label);
    }
    return Base::Tools::getUniqueName(Name, labels, d);
}

std::vector<DocumentObject*> Document::getObjects() const
{
    return d->objectArray;
}

std::vector<DocumentObject*> Document::getObjectsOfType(const Base::Type& typeId) const
{
    std::vector<DocumentObject*> Objects;
    for (std::vector<DocumentObject*>::const_iterator it = d->objectArray.begin(); it != d->objectArray.end(); ++it) {
        if ((*it)->getTypeId().isDerivedFrom(typeId))
            Objects.push_back(*it);
    }
    return Objects;
}

std::vector<DocumentObject*> Document::findObjects(const Base::Type& typeId, const char* objname) const
{
    boost::regex rx(objname);
    boost::cmatch what;
    std::vector<DocumentObject*> Objects;
    for (std::vector<DocumentObject*>::const_iterator it = d->objectArray.begin(); it != d->objectArray.end(); ++it) {
        if ((*it)->getTypeId().isDerivedFrom(typeId)) {
            if (boost::regex_match((*it)->getNameInDocument(), what, rx))
                Objects.push_back(*it);
        }
    }
    return Objects;
}

int Document::countObjectsOfType(const Base::Type& typeId) const
{
    int ct=0;
    for (std::map<std::string,DocumentObject*>::const_iterator it = d->objectMap.begin(); it != d->objectMap.end(); ++it) {
        if (it->second->getTypeId().isDerivedFrom(typeId))
            ct++;
    }

    return ct;
}

PyObject * Document::getPyObject(void)
{
    return Py::new_reference_to(DocumentPythonObject);
}
