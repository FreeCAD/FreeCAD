/***************************************************************************
 *   Copyright (c) 2011 Jürgen Riegel <juergen.riegel@web.de>              *
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


#ifndef APP_PERSISTENCE_H
#define APP_PERSISTENCE_H

#include "BaseClass.h"

#include <xercesc/util/XercesDefs.hpp>

XERCES_CPP_NAMESPACE_BEGIN
	class DOMNode;
	class DOMElement;
//    class DefaultHandler;
//    class SAX2XMLReader;
XERCES_CPP_NAMESPACE_END

namespace Base
{
class Reader;
class Writer;
class XMLReader;
class DocumentReader;

/// Persistence class and root of the type system
class BaseExport Persistence : public BaseClass
{

  TYPESYSTEM_HEADER();

public:
    /** This method is used to get the size of objects
     * It is not meant to have the exact size, it is more or less an estimation
     * which runs fast! Is it two bytes or a GB?
     */
    virtual unsigned int getMemSize () const = 0;
    /** This method is used to save properties to an XML document.
     * A good example you'll find in PropertyStandard.cpp, e.g. the vector:
     * \code
     *  void PropertyVector::Save (Writer &writer) const
     *  {
     *     writer << writer.ind() << "<PropertyVector valueX=\"" <<  _cVec.x <<
     *                                            "\" valueY=\"" <<  _cVec.y <<
     *                                            "\" valueZ=\"" <<  _cVec.z <<"\"/>" << endl;
     *  }
     * \endcode
     * The writer.ind() expression writes the indentation, just for pretty printing of the XML.
     * As you see, the writing of the XML document is not done with a DOM implementation because
     * of performance reasons. Therefore the programmer has to take care that a valid XML document
     * is written. This means closing tags and writing UTF-8.
     * @see Base::Writer
     */
    virtual void Save (Writer &/*writer*/) const = 0;
    /** This method is used to restore properties from an XML document.
     * It uses the XMLReader class, which bases on SAX, to read the in Save()
     * written information. Again the Vector as an example:
     * \code
     * void PropertyVector::Restore(Base::XMLReader &reader)
     * {
     *   // read my Element
     *   reader.readElement("PropertyVector");
     *   // get the value of my Attribute
     *   _cVec.x = reader.getAttributeAsFloat("valueX");
     *   _cVec.y = reader.getAttributeAsFloat("valueY");
     *   _cVec.z = reader.getAttributeAsFloat("valueZ");
     * }
     * \endcode
     */
    virtual void Restore(XMLReader &/*reader*/) = 0;
    virtual void Restore(DocumentReader &/*reader*/);
    virtual void Restore(DocumentReader &/*reader*/,XERCES_CPP_NAMESPACE_QUALIFIER DOMElement */*containerEl*/);
    
    /** This method is used to save large amounts of data to a binary file.
     * Sometimes it makes no sense to write property data as XML. In case the
     * amount of data is too big or the data type has a more effective way to
     * save itself. In this cases it is possible to write the data in a separate file
     * inside the document archive. In case you want do so you have to re-implement
     * SaveDocFile(). First, you have to inform the framework in Save() that you want do so.
     * Here an example from the Mesh module which can save a (pontetionaly big) triangle mesh:
     * \code
     * void PropertyMeshKernel::Save (Base::Writer &writer) const
     * {
     *   if (writer.isForceXML())
     *   {
     *     writer << writer.ind() << "<Mesh>" << std::endl;
     *     MeshCore::MeshDocXML saver(*_pcMesh);
     *     saver.Save(writer);
     *   }else{
     *    writer << writer.ind() << "<Mesh file=\"" << writer.addFile("MeshKernel.bms", this) << "\"/>" << std::endl;
     * }
     * \endcode
     * The writer.isForceXML() is an indication to force you to write XML. Regardless of size and effectiveness.
     * The second part informs the Base::writer through writer.addFile("MeshKernel.bms", this) that this object
     * wants to write a file with the given name. The method addFile() returns a unique name that then is written
     * in the XML stream.
     * This allows your RestoreDocFile() method to identify and read the file again.
     * Later your SaveDocFile() method is called as many times as you issued the addFile() call:
     * \code
     * void PropertyMeshKernel::SaveDocFile (Base::Writer &writer) const
     * {
     *     _pcMesh->Write( writer );
     * }
     * \endcode
     * In this method you can simply stream your content to the file (Base::Writer inheriting from ostream).
     */
    virtual void SaveDocFile (Writer &/*writer*/) const;
    /** This method is used to restore large amounts of data from a file
     * In this method you simply stream in your SaveDocFile() saved data.
     * Again you have to apply for the call of this method in the Restore() call:
     * \code
     * void PropertyMeshKernel::Restore(Base::XMLReader &reader)
     * {
     *   reader.readElement("Mesh");
     *   std::string file (reader.getAttribute("file") );
     *
     *   if(file == "")
     *   {
     *     // read XML
     *     MeshCore::MeshDocXML restorer(*_pcMesh);
     *     restorer.Restore(reader);
     *   }else{
     *     // initiate a file read
     *     reader.addFile(file.c_str(),this);
     *  }
     * }
     * \endcode
     * After you issued the reader.addFile() your RestoreDocFile() is called:
     * \code
     * void PropertyMeshKernel::RestoreDocFile(Base::Reader &reader)
     * {
     *     _pcMesh->Read( reader );
     * }
     * \endcode
     * @see Base::Reader,Base::XMLReader
     */
    virtual void RestoreDocFile(Reader &/*reader*/);
    
    
    /// Encodes an attribute upon saving.
    static std::string encodeAttribute(const std::string&);

    //dump the binary persistence data into into the stream
    void dumpToStream(std::ostream& stream, int compression);

    //restore the binary persistence data from a stream. Must have the format used by dumpToStream
    void restoreFromStream(std::istream& stream);

private:
    /** This method is used at the end of restoreFromStream()
     * after all data files have been read in.
     * A subclass can set up some internals. The default
     * implementation does nothing.
     */
    virtual void restoreFinished() {}
};

} //namespace Base


#endif // APP_PERSISTENCE_H
