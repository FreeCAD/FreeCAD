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
 * \brief Implementation of zipios::GZIPOutputStream.
 *
 * The zipios::GZipOutputStream defines functions which handle the
 * saving of Zip archive content file in compressed form using
 * the zlib library.
 */

#include "gzipoutputstream.hpp"

#include <fstream>


namespace zipios
{

/** \class GZIPOutputStream
 * \brief A stream implementation that outputs data to a ZIP file.
 *
 * GZIPOutputStream is an ostream that writes the output to a zip file.
 * The interface approximates the interface of the Java GZIPOutputStream.
 *
 * It can be used with either an existing std::ostream object, or
 * a filename.
 */



/** \brief Create a ZIP output stream object.
 *
 * This constructor creates a zip stream from an existing standard
 * output stream.
 *
 * \warning
 * You must keep the output stream valid for as long as this object
 * exists (although this object close() function can be used to close
 * the \p os stream.)
 *
 * \param[in,out] os  ostream to which the compressed zip archive is written.
 * \param[in] compression_level  The compression level to use to compress.
 */
GZIPOutputStream::GZIPOutputStream(std::ostream & os, FileEntry::CompressionLevel compression_level)
    : m_ozf(std::make_unique<GZIPOutputStreambuf>(os.rdbuf(), compression_level))
{
    init(m_ozf.get());
}


/** \brief Create a named ZIP stream for output.
 *
 * \note
 * The filename is not automatically saved as part of the stream.
 * To do so, call the setFilename() function.
 *
 * \param[in] filename  Name of the file where the zip archive is to
 *                      be written.
 * \param[in] compression_level  The compression level to use to compress.
 */
GZIPOutputStream::GZIPOutputStream(std::string const & filename, FileEntry::CompressionLevel compression_level)
    : std::ostream(0)
    , m_ofs(std::make_unique<std::ofstream>(filename.c_str(), std::ios::out | std::ios::binary))
    , m_ozf(std::make_unique<GZIPOutputStreambuf>(m_ofs->rdbuf(), compression_level))
{
    init(m_ozf.get());
}


/** \brief Destroy the output stream.
 *
 * The destructor ensures that all allocated resources get destroyed.
 */
GZIPOutputStream::~GZIPOutputStream()
{
}


/** \brief Set the filename of a stream.
 *
 * This function can be used to set the name of the file being
 * added to this stream.
 *
 * The filename is optional.
 *
 * \param[in] filename  The filename to attach to this stream.
 */
void GZIPOutputStream::setFilename(std::string const & filename)
{
    m_ozf->setFilename(filename);
}


/** \brief Set a comment in the stream.
 *
 * This function can be used to add a comment to the zip file.
 *
 * A comment is optional.
 *
 * \param[in] comment  The comment to attach to this stream.
 */
void GZIPOutputStream::setComment(std::string const & comment)
{
    m_ozf->setComment(comment);
}


/** \brief Close the streams.
 *
 * This function closes the streams making sure that all data gets
 * saved in the output file.
 *
 * It is not required since destroying the object will also force
 * a close.
 */
void GZIPOutputStream::close()
{
    m_ozf->close();
    if(m_ofs)
    {
        m_ofs->close();
    }
}


/** \brief Finishes the stream.
 *
 */
void GZIPOutputStream::finish()
{
    m_ozf->finish();
}


} // zipios namespace

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
