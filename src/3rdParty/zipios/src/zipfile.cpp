/*
  Zipios -- a small C++ library that provides easy access to .zip files.

  Copyright (C) 2000-2007  Thomas Sondergaard
  Copyright (c) 2015-2022  Made to Order Software Corp.  All Rights Reserved

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

/** \file
 * \brief The implementation of zipios::ZipFile.
 *
 * This file contains the high level functions used to read or write
 * a Zip archive file.
 */

#include "zipios/zipfile.hpp"

#include "zipios/streamentry.hpp"
#include "zipios/zipiosexceptions.hpp"

#include "backbuffer.hpp"
#include "zipendofcentraldirectory.hpp"
#include "zipcentraldirectoryentry.hpp"
#include "zipinputstream.hpp"
#include "zipoutputstream.hpp"

#include <fstream>


/** \brief The zipios namespace includes the Zipios library definitions.
 *
 * This namespace is used to clearly separate all the Zipios definitions.
 * Note that a very few definitions are found outside of the namespace.
 * Some of those are hidden in the source of the library, a very few
 * appear in the zipios-config.hpp file as they are used to support
 * Zipios on any platform.
 *
 * Note that to ensure secure development, we do not make use of the
 * C++ "using ..." keyword. That way we can guarantee what's what.
 */
namespace zipios
{


/** \mainpage Zipios
 *
 * \image html zipios.jpg
 *
 * \section intro Introduction
 *
 * Zipios is a java.util.zip-like C++ library for reading and
 * writing Zip files (ZipFile). Access to individual entries is
 * provided through a Zipios class (FileEntry) for the meta data
 * of the and a standard C++ iostreams for the contents of the file.
 *
 * A simple virtual file system that mounts regular directories and
 * zip files is also provided (FileCollection).
 *
 * The library is fully re-entrant. It is not otherwise thread safe.
 *
 * The source code is released under the <a
 * href="http://www.gnu.org/copyleft/lesser.html">GNU Lesser General Public
 * License</a>.
 *
 * \section status Status
 *
 * This was the status of version 1.x. At this point, 2.x has a brand new
 * version out and we are waiting for good news about the current status.
 * That being said, version 2.x comes a test suite which produces a
 * 100% coverage of the library (except gzip which is not yet publicly
 * available.)
 *
 * \warning
 * There is a bug in the catch.hpp header file that generates a never
 * ending loop (see https://github.com/philsquared/Catch/issues/271 for
 * more information) when running the test suite under FreeBSD and an error
 * occurs (although you should not have an error, if it happens, then
 * the loop never ends.) I have noticed that problem with the following
 * scenario, and it does not seem to be fixed yet (Apr 4, 2015):
 *
 * \li "use Zipios to create zip archives with 1 or 3 files each"
 *
 * Spanned archives are not supported, and support is not planned.
 *
 * The library v1.x has been tested and appears to be working with:
 *
 * \li <a href="http://www.freebsd.org/ports/archivers.html#zipios++-0.1.5">FreeBSD stable and current / gcc 2.95.3</a>
 * \li Red Hat Linux release 7.0  / gcc 2.96
 * \li Red Hat Linux release 6.2 (Zoot) / egcs-2.91.66
 * \li Linux Mandrake release 7.0 (Air) / gcc 2.95.2
 * \li SGI IRIX64 6.5 / gcc 2.95.2
 * \li SGI IRIX64 6.5 / MIPSpro Compilers: Version 7.30
 *
 * The library v2.x has been compiled and appears to be working with:
 *
 * \li Ubuntu (starting with 14.04) -- full test suite working
 * \li Debian (starting with Stretch)
 * \li Fedora (starting with F25)
 * \li FreeBSD (starting with 10.01)
 * \li SunOS (starting with Open SunOS 11.2)
 * \li Cygwin (starting with 6.1)
 *
 * If you make Zipios work on other platforms, let us know by posting
 * an issue on GitHub:
 *
 *    https://github.com/Zipios/Zipios/issues
 *
 *
 * \section documentation Documentation
 *
 * This web page is the front page to the library documentation which
 * is generated from the source files using <a
 * href="http://www.stack.nl/~dimitri/doxygen/index.html">Doxygen</a>.
 * Use the links at the top of the page to browse the API
 * documentation. Your Doxygen installation may also be capable
 * of generating other formats (Latex, PDF, etc.) if you would
 * prefer such (we only offer the HTML documentation.)
 *
 * \subsection zipfiles Zip file access
 *
 * The two most important classes are DirectoryCollection and ZipFile.
 *
 * A ZipFile is also a FileCollection, only the collection is loaded
 * from a Zip archive instead of a directory. A ZipFile is composed of
 * ZipCentralDirectoryEntry objects. As far as you are concerned though,
 * you can only use it as FileEntry objects.
 *
 * Note that the ZipFile constructor immediately scans the Central
 * Directory of the Zip archive so the entries are immediately accessible.
 *
 * The DirectoryCollection can be created one file at a time, so it is
 * possible to create a collection without having to include all the
 * files from a directory. However, the files still have to exist on
 * disk. The DirectoryCollection is composed of DirectoryEntry objects.
 *
 * To access the entries in a collection, use the entries() function
 * which returns a vector of FileEntry objects. If you know the exact
 * filename of an entry, you may also use the getEntry() with that name.
 * This is particularly useful if you want to use Zipios as a way to
 * handle the resources of your executable (see the openEmbeddedZipFile()
 * function and the appendzip.cpp tool). Finally, you want to use
 * the getInputStream() function to read the data of a file defined in
 * a collection.
 *
 * \code
 *      // Resources global pointer
 *      zipios::ZipFile::pointer_t g_resources;
 *
 *      // Initialization of resources
 *      g_resources = zipios::ZipFile::openEmbeddedZipFile("executable_filename");
 *
 *      // Anywhere else in your application
 *
 *      // 1. get the entry (to access meta data)
 *      zipios::FileEntry::pointer_t entry(g_resources->getEntry("my/resource/file.xml"));
 *
 *      // 2. get the istream (to access the actual file data)
 *      zipios::FileCollection::stream_pointer_t in_stream(g_resources->getInputStream("my/resource/file.xml"));
 * \endcode
 *
 * zipios_example.cpp demonstrates the central elements of Zipios when used
 * in read mode.
 *
 * \subsection filecollection FileCollection
 *
 * A ZipFile is actually just a special kind of FileCollection that
 * obtains its entries from a Zip archive. Zipios also implements
 * a DirectoryCollection that obtains its entries from an on disk
 * directory and a CollectionCollection that obtains its entries from
 * other collections.
 *
 * Using a single CollectionCollection, any number of other FileCollection's
 * can be placed under its control and accessed through the same single
 * interface that is used to access a ZipFile or a DirectoryCollection.
 *
 * \warning
 * The CollectionCollection singleton in version 1.x was removed to make
 * the entire library 100% re-entrant without the need to link against
 * a thread library.
 *
 * \section download Download
 *
 * The <a href="https://github.com/Zipios/Zipios">Zipios project</a> is now
 * on github. You can find the source files, tarballs and changelog files
 * there.
 *
 * \section development Development
 *
 * The Zipios project makes use of a few development tools and the
 * tests require the zip utility, used to verify that an external tool
 * can generate a zip file that Zipios can read.
 *
 * Under a Debian or Ubuntu system, you can run apt-get install with
 * the following list of packages:
 *
 * \code
 *   # For source management (or download the .tar.gz file)
 *   apt-get install git
 *
 *   # For development
 *   apt-get install g++ cmake zlib1g-dev
 *
 *   # For documentation (or download the -doc.tar.gz file)
 *   apt-get install doxygen graphviz
 *
 *   # For testing (optional, albeit recommended)
 *   apt-get install catch zip
 * \endcode
 *
 * Under Fedora, you can use dnf (or yum for early versions):
 *
 * \code
 *   # For source management (or download the .tar.gz file)
 *   dnf install git
 *
 *   # For development
 *   dnf install gcc-c++ cmake zlib-devel
 *
 *   # For documentation (or download the -doc.tar.gz file)
 *   dnf install doxygen graphviz
 *
 *   # For testing (optional, albeit recommended)
 *   dnf install catch-devel zip
 * \endcode
 *
 *
 * Other systems use tools with pretty much the same naming convention
 * so you should be able to make the correlation.
 *
 * \section history History
 *
 * <a href="http://www.zlib.net/">zlib</a>.
 * The compression library that Zipios uses to perform the actual
 * compression and decompression.
 *
 * Zipios version 1.0 is heavily inspired by the java.util.zip package.
 * Version 2.0 is following the same philosophy without (1) attempting
 * to follow the interface one to one and (2) without updating to the
 * newer version, if there were changes...
 *
 * You will find a text file in the doc directory named zip-format.txt
 * with a complete description of the zip file format as of October 1, 2014.
 *
 * \section bugs Bugs
 *
 * <a href="https://github.com/Zipios/Zipios/issues">Submit bug reports
 * and patches on github.</a>
 *
 * \section source Source
 *
 * The <a href="https://github.com/Zipios/Zipios">source code is available
 * on github</a>.
 */


/** \class ZipFile
 * \brief The ZipFile class represents a collection of files.
 *
 * ZipFile is a FileCollection, where the files are stored
 * in a .zip file.
 */



/** \brief Open a zip archive that was previously appended to another file.
 *
 * Opens a Zip archive embedded in another file, by writing the zip
 * archive to the end of the file followed by the start offset of
 * the zip file on 4 bytes. The offset must be written in zip-file
 * byte-order (little endian).
 *
 * The program appendzip, which is part of the Zipios distribution can
 * be used to append a Zip archive to a file, e.g. a binary program.
 *
 * The function may throw various exception if the named file does not
 * seem to include a valid zip archive attached.
 *
 * \note
 * Only one file can be appended and opened in this way. Although
 * the appendzip tool can be used to append any number of files,
 * only the last one is accessible.
 *
 * \exception FileCollectionException
 * This exception is raised if the initialization fails. The function verifies
 * that the input stream represents what is considered a valid zip file.
 *
 * \param[in] filename  The filename of your executable (generally, argv[0]).
 *
 * \return A ZipFile that one can use to read compressed data.
 */
ZipFile::pointer_t ZipFile::openEmbeddedZipFile(std::string const & filename)
{
    // open zipfile, read 4 last bytes close file
    uint32_t start_offset;
    {
        std::ifstream ifs(filename, std::ios::in | std::ios::binary);
        ifs.seekg(-4, std::ios::end);
        zipRead(ifs, start_offset);
        // todo: add support for 64 bit (files of more than 4Gb)
    }

    // create ZipFile object from embedded data
    return std::make_shared<ZipFile>(filename, start_offset, 4);
}


/** \brief Initialize a ZipFile object.
 *
 * This is the default constructor of the ZipFile object.
 *
 * Note that an empty ZipFile is marked as invalid. More or less, such
 * an object is useless although it is useful to have this constructor
 * if you want to work with maps or vectors of ZipFile objects.
 */
ZipFile::ZipFile()
{
}


/** \brief Initialize a ZipFile object from an existing file.
 *
 * This constructor opens the named zip file. If the zip "file" is
 * embedded in a file that contains other data, e.g. a binary
 * program, the offset of the zip file start and end must be
 * specified.
 *
 * If the file cannot be opened or the Zip directory cannot
 * be read, then the constructor throws an exception.
 *
 * \exception FileCollectionException
 * This exception is raised if the initialization fails. The function verifies
 * that the input stream represents what is considered a valid zip file.
 *
 * \param[in] filename  The filename of the zip file to open.
 * \param[in] s_off  Offset relative to the start of the file, that
 *                   indicates the beginning of the zip data in the file.
 * \param[in] e_off  Offset relative to the end of the file, that
 *                   indicates the end of the zip data in the file.
 *                   The offset is a positive number, even though the
 *                   offset goes toward the beginning of the file.
 */
ZipFile::ZipFile(std::string const & filename, offset_t s_off, offset_t e_off)
    : FileCollection(filename)
    , m_vs(s_off, e_off)
{
    std::ifstream zipfile(m_filename, std::ios::in | std::ios::binary);
    if(!zipfile)
    {
        throw IOException("Error opening Zip archive file for reading in binary mode.");
    }

    init(zipfile);
}


/** \brief Initialize a ZipFile object from an istream.
 *
 * This constructor opens the ZipFile from the specified istream. The
 * istream can be in memory or even an Internet stream. However, the
 * ZipFile algorithm requires a stream with proper back and forth seek
 * capabilities.
 *
 * If the Zip directory cannot be read, then the constructor throws an
 * exception.
 *
 * \exception FileCollectionException
 * This exception is raised if the initialization fails. The function verifies
 * that the input stream represents what is considered a valid zip file.
 *
 * \param[in] is  The input stream with the zip file data.
 * \param[in] s_off  Offset relative to the start of the file, that
 *                   indicates the beginning of the zip data in the file.
 * \param[in] e_off  Offset relative to the end of the file, that
 *                   indicates the end of the zip data in the file.
 *                   The offset is a positive number, even though the
 *                   offset goes toward the beginning of the file.
 */
ZipFile::ZipFile(std::istream & is, offset_t s_off, offset_t e_off)
    : m_vs(s_off, e_off)
{
    init(is);
}


/** \brief Initialize the ZipFile from the specified input stream.
 *
 * This function finishes the initialization of the ZipFile from the
 * constructor. It is 100% private.
 *
 * \exception FileCollectionException
 * This exception is raised if the initialization fails. The function verifies
 * that the input stream represents what is considered a valid zip file.
 *
 * \param[in] is  The input stream used to read the ZipFile.
 */
void ZipFile::init(std::istream & is)
{
    // Find and read the End of Central Directory.
    ZipEndOfCentralDirectory eocd;
    {
        BackBuffer bb(is, m_vs);
        ssize_t read_p(-1);
        for(;;)
        {
            if(read_p < 0)
            {
                if(!bb.readChunk(read_p))
                {
                    throw FileCollectionException("Unable to find zip structure: End-of-central-directory");
                }
            }
            // Note: this is pretty fast since it reads from 'bb' which
            //       caches the buffer the readChunk() function just read.
            //
            if(eocd.read(bb, read_p))
            {
                // found it!
                break;
            }
            --read_p;
        }
    }

    // Position read pointer to start of first entry in central dir.
    m_vs.vseekg(is, eocd.getOffset(), std::ios::beg);

    // TBD -- is that ", 0" still necessary? (With VC2012 and better)
    // Give the second argument in the next line to keep Visual C++ quiet
    //m_entries.resize(eocd.getCount(), 0);
    m_entries.resize(eocd.getCount());

    size_t const max_entry(eocd.getCount());
    for(size_t entry_num(0); entry_num < max_entry; ++entry_num)
    {
        m_entries[entry_num] = std::make_shared<ZipCentralDirectoryEntry>();
        m_entries[entry_num].get()->read(is);
    }

    // Consistency check #1:
    // The virtual seeker position is exactly the start offset of the
    // Central Directory plus the Central Directory size
    //
    offset_t const pos(m_vs.vtellg(is));
    if(static_cast<offset_t>(eocd.getOffset() + eocd.getCentralDirectorySize()) != pos)
    {
        throw FileCollectionException("Zip file consistency problem. Zip file data fields are inconsistent with zip file layout.");
    }

    // Consistency check #2:
    // Are local headers consistent with CD headers?
    //
    for(auto it = m_entries.begin(); it != m_entries.end(); ++it)
    {
        /** \TODO
         * Make sure the entry offset is properly defined by
         * ZipCentralDirectoryEntry.
         *
         * Also the isEqual() is a quite advanced (slow) test here!
         */
        m_vs.vseekg(is, (*it)->getEntryOffset(), std::ios::beg);
        ZipLocalEntry zlh;
        zlh.read(is);
        if(!is || !zlh.isEqual(**it))
        {
            throw FileCollectionException("Zip file consistency problem. Zip file data fields are inconsistent with zip file layout.");
        }
    }

    // we are all good!
    m_valid = true;
}


/** \brief Create a clone of this ZipFile.
 *
 * This function creates a heap allocated clone of the ZipFile object.
 *
 * \return A shared pointer to a copy of this ZipFile object.
 */
FileCollection::pointer_t ZipFile::clone() const
{
    return FileCollection::pointer_t(std::make_shared<ZipFile>(*this));
}


/** \brief Clean up the ZipFile object.
 *
 * The destructor ensures that any ZipFile data gets flushed
 * out before returning.
 */
ZipFile::~ZipFile()
{
    close();
}


/** \brief Retrieve a pointer to a file in the Zip archive.
 *
 * This function returns a shared pointer to an istream defined from the
 * named entry, which gives you access to the corresponding file defined
 * in the Zip archive.
 *
 * The function returns nullptr if there is no entry with the
 * specified name in this ZipFile.
 *
 * Note that the function returns a smart pointer to an istream. The
 * ZipFile class does not hold that pointer meaning that
 * if you call getInputStream() multiple times with the same
 * \p entry_name parameter, you get different istream instance each
 * time.
 *
 * By default the \p entry_name parameter is expected to match the full
 * path and filename (MatchPath::MATCH). If you are looking for a file
 * and want to ignore the path, set the matchpath parameter
 * to MatchPath::IGNORE.
 *
 * \note
 * If the file is compressed inside the Zip archive, this input stream
 * returns the uncompressed data transparently to you (outside of the
 * time it takes to decompress the data, of course.)
 *
 * \param[in] entry_name  The name of the file to search in the collection.
 * \param[in] matchpath  Whether the full path or just the filename is matched.
 *
 * \return A shared pointer to an open istream for the specified entry.
 *
 * \sa CollectionCollection
 * \sa DirectoryCollection
 * \sa FileCollection
 */
ZipFile::stream_pointer_t ZipFile::getInputStream(std::string const & entry_name, MatchPath matchpath)
{
    mustBeValid();

    // TODO: see whether we could make the handling of the StreamEntry
    //       non-special
    //
    FileEntry::pointer_t entry(getEntry(entry_name, matchpath));
    StreamEntry::pointer_t stream(std::dynamic_pointer_cast<StreamEntry>(entry));
    if(stream != nullptr)
    {
        stream_pointer_t zis(std::make_shared<ZipInputStream>(stream->getStream()));
        return zis;
    }
    else if(entry != nullptr)
    {
        stream_pointer_t zis(std::make_shared<ZipInputStream>(m_filename, entry->getEntryOffset() + m_vs.startOffset()));
        return zis;
    }

    // no entry with that name (and match) available
    return nullptr;
}


/** \brief Create a Zip archive from the specified FileCollection.
 *
 * This function is expected to be used with a DirectoryCollection
 * that you created to save the collection in an archive.
 *
 * \param[in,out] os  The output stream where the Zip archive is saved.
 * \param[in] collection  The collection to save in this output stream.
 * \param[in] zip_comment  The global comment of the Zip archive.
 */
void ZipFile::saveCollectionToArchive(
      std::ostream & os
    , FileCollection & collection
    , std::string const & zip_comment)
{
    try
    {
        ZipOutputStream output_stream(os);

        output_stream.setComment(zip_comment);

        FileEntry::vector_t entries(collection.entries());
        for(auto it(entries.begin()); it != entries.end(); ++it)
        {
            output_stream.putNextEntry(*it);

            // next we need to include the data of that file in the
            // output buffer if it is not a directory and the file is
            // not an empty file
            //
            if(!(*it)->isDirectory()
            && (*it)->getSize() > 0)
            {
                // get an InputStream
                //
                FileCollection::stream_pointer_t is(collection.getInputStream((*it)->getName()));
                if(is != nullptr
                && is->good())
                {
                    // copy the file content to the output
                    //
                    output_stream << is->rdbuf();
                }
            }
        }

        // clean up manually so we can get any exception
        // (so we avoid having exceptions gobbled by the destructor)
        output_stream.closeEntry();
        output_stream.finish();
        output_stream.close();
    }
    catch(...)
    {
        os.setstate(std::ios::failbit);
        throw;
    }
}


} // zipios namespace

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
