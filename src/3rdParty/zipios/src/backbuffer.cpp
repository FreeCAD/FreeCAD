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
 * \brief The implementation file of zipios::BackBuffer.
 *
 * This class implements the functions necessary to read a file
 * backward.
 */

#include "backbuffer.hpp"

#include "zipios/zipiosexceptions.hpp"

#include <algorithm>

namespace zipios
{

/** \class BackBuffer
 * \brief To read a file by chunk from the end.
 *
 * A BackBuffer instance is useful for reading the last part of a
 * file in an efficient manner, when it is not known exactly how far
 * back (towards the front!) to go, to find the start of the desired
 * data block.
 *
 * BackBuffer is an std::vector<unsigned char> that fills
 * itself with data from a file by reading chunks from the end of the
 * file progressing towards the start. Upon construction the
 * BackBuffer instance is associated with a file and a chunksize can
 * be specified. To read a chunk of the file into the BackBuffer call
 * readChunk().
 *
 * Note that this is not a good option for really large files as the
 * buffer is enlarged using an insert() on each call to readChunk().
 */



/** BackBuffer constructor.
 *
 * Initialize a buffer that reads data backward.
 *
 * The content of the buffer is defined as the content of the \p is
 * stream. The stream must be open and seekable. The file pointer
 * of the stream is modified by this constructor and on each
 * call to readChunk().
 *
 * \exception IOException
 * This exception is raised when the VirtualSeeker (\p vs) returns
 * an invalid answer determining the size of the stream.
 *
 * \param[in,out] is  The istream to read the data from. The stream must
 *                    be seekable, as BackBuffer will reposition the
 *                    file pointer to read chunks from the back of the
 *                    file.
 * \param[in] vs  The virtual seeker used to change the file pointer.
 * \param[in] chunk_size   Specifies the size of the chunks to read the
 *                         file into the BackBuffer in.
 */
BackBuffer::BackBuffer(std::istream & is, VirtualSeeker const & vs, ssize_t const chunk_size)
    : m_vs(vs)
    , m_chunk_size(chunk_size)
    , m_is(is)
    , m_file_pos(0)
{
    if(!m_is)
    {
        throw InvalidException("BackBuffer() initialized with an invalid input stream.");
    }
    if(m_chunk_size <= 0)
    {
        throw InvalidException("Invalid chunk_size parameter, it has to be larger than zero.");
    }

    m_vs.vseekg(m_is, 0, std::ios::end);
    m_file_pos = m_vs.vtellg(m_is);

    // The following should only happens when m_vs.startOffset() is a
    // position in the file that lies after m_vs.endOffset(), which
    // is clearly not a valid situation. However, vtellg() may just
    // fail too.
    if(m_file_pos < 0)
    {
        throw IOException("Invalid virtual file endings.");
    }
}


/** \brief Read a chunk of data.
 *
 * Reads another chunk and returns the size of the chunk that has
 * been read. Returns 0 on I/O failure.
 *
 * When a new chunk is read in the already stored bytes change
 * position in the BackBuffer. \p read_pointer is assumed by
 * readChunk() to be a pointer into a position in the BackBuffer,
 * and is updated to point to the same position in the file
 * as it pointed to before the new chunk was prepended.
 *
 * When first calling readChunk(), \p read_pointer is expected
 * to be zero and represent the position of EOF.
 *
 * \warning
 * The file is read backward.
 *
 * \warning
 * The function may change m_chunk_size. In the end will be
 * zero and no more data can be read.
 *
 * \warning
 * The function changes the file pointer of the attached stream.
 *
 * \bug
 * We may want to throw an error if m_is.is_good() returns false.
 *
 * \param[in,out] read_pointer  The buffer pointer.
 *
 * \return The number of bytes read if any, otherwise zero. Note that if
 *         an error occurs, the function also returns zero.
 */
ssize_t BackBuffer::readChunk(ssize_t & read_pointer)
{
    // Update m_chunk_size and file position
    m_chunk_size = std::min<ssize_t>(static_cast<ssize_t>(m_file_pos), m_chunk_size);
    m_file_pos -= m_chunk_size;
    m_vs.vseekg(m_is, m_file_pos, std::ios::beg);

    // Make space for m_chunk_size new bytes
    insert(begin(), m_chunk_size, static_cast<char>(0));

    // Read in the next m_chunk_size bytes
    zipRead(m_is, *this, m_chunk_size);
    read_pointer += m_chunk_size;

    return m_is.good() ? m_chunk_size : 0;
}


} // zipios namespace
// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
