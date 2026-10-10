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
 * \brief Implementation of zipios::InflateInputStreambuf.
 *
 * This file defines the various functions found in the
 * zipios::InflateInputStreambuf class. It filters a stream buffer
 * to decompress data that was compressed using the zlib library.
 */

#include "inflateinputstreambuf.hpp"

#include "zipios/zipiosexceptions.hpp"

#include "zipios_common.hpp"


namespace zipios
{

/** \class InflateInputStreambuf
 * \brief A stream buffer to inflate data previous compressed with zlib.
 *
 * The InflateInputStreambuf class is an input stream filter, that
 * inflates the input from the attached input stream.
 *
 * Deflation/Inflation is a compression/decompression method used
 * in gzip and zip. The zlib library is used to perform the actual
 * inflation, this class only wraps the functionality in an input
 * stream filter.
 *
 * \todo
 * Add support for bzip2, lzma compressions.
 */



/** \brief Initialize a InflateInputStreambuf.
 *
 * The constructor initializes the various stream buffers and
 * setup the stream start position using the \p start_pos
 * parameter.
 *
 * Data will be inflated (decompressed using zlib) before being
 * returned.
 *
 * \param[in,out] inbuf  The streambuf to use for input.
 * \param[in] start_pos  A position to reset the inbuf to before reading. Specify
 *                       -1 to not change the position.
 */
InflateInputStreambuf::InflateInputStreambuf(std::streambuf * inbuf, offset_t start_pos)
    : FilterInputStreambuf(inbuf)
    , m_outvec(getBufferSize())
    , m_invec(getBufferSize())
{
    // NOTICE: It is important that this constructor and the methods it
    // calls doesn't do anything with the input streambuf inbuf, other
    // than repositioning it to the specified \p start_pos. The reason is
    // that this class can be sub-classed, and the sub-class should get a
    // chance to read from the buffer first

    reset(start_pos);
    // We are not checking the return value of reset() and throwing
    // an exception in case of an error, because we cannot catch the
    // exception in the constructors of sub-classes with all compilers.
}


/** \brief Clean up the InflateInputStreambuf object.
 *
 * The destructor makes sure all allocated resources get cleaned up.
 */
InflateInputStreambuf::~InflateInputStreambuf()
{
    // Dealloc z_stream
    int const err(inflateEnd(&m_zs));
    if(err != Z_OK)
    {
        // in a destructor we cannot throw...
        OutputStringStream msgs; // LCOV_EXCL_LINE
        msgs << "InflateInputStreambuf::~InflateInputStreambuf(): inflateEnd() failed" // LCOV_EXCL_LINE
             << ": " << zError(err); // LCOV_EXCL_LINE
        /** \TODO
         * Write an error callback interface and call that instead of
         * using std::cerr...
         */
        std::cerr << msgs.str() << std::endl; // LCOV_EXCL_LINE
    }
}


/** \brief Called when more data is required.
 *
 * The function ensures that at least one byte is available
 * in the input area by updating the pointers to the input area
 * and reading more data in from the input sequence if required.
 *
 * This function actually passes the data through the zlib library
 * to decompress it.
 *
 * \return The value of that character on success or
 *         std::streambuf::traits_type::eof() on failure.
 */
std::streambuf::int_type InflateInputStreambuf::underflow()
{
    // If not really underflow do not fill buffer
    // (is that really possible?!)
    if(gptr() < egptr())
    {
        return traits_type::to_int_type(*gptr()); // LCOV_EXCL_LINE
    }

    // Prepare _outvec and get array pointers
    m_zs.avail_out = getBufferSize();
    m_zs.next_out = reinterpret_cast<unsigned char *>(&m_outvec[0]);

    // Inflate until _outvec is full
    // eof (or I/O prob) on _inbuf will break out of loop too.
    int err(Z_OK);
    while(m_zs.avail_out > 0 && err == Z_OK)
    {
        if(m_zs.avail_in == 0)
        {
            // fill m_invec
            std::streamsize const bc(m_inbuf->sgetn(&m_invec[0], getBufferSize()));
            /** \FIXME
             * Add I/O error handling while inflating data from a file.
             */
            m_zs.next_in = reinterpret_cast<unsigned char *>(&m_invec[0]);
            m_zs.avail_in = bc;
            // If we could not read any new data (bc == 0) and inflate is not
            // done it will return Z_BUF_ERROR and thus breaks out of the
            // loop. This means we do not have to respond to the situation
            // where we cannot read more bytes here.
        }

        err = inflate(&m_zs, Z_NO_FLUSH);
    }

    // Normally the number of inflated bytes will be the
    // full length of the output buffer, but if we can't read
    // more input from the _inbuf streambuf, we end up with
    // less.
    offset_t const inflated_bytes = getBufferSize() - m_zs.avail_out;
    setg(&m_outvec[0], &m_outvec[0], &m_outvec[0] + inflated_bytes);

    /** \FIXME
     * Look at the error returned from inflate here, if there is
     * some way to report it to the InflateInputStreambuf user.
     * For now we throw. We probably want to create a log
     * mechanism that the end user can connect to with a callback.
     */
    if(err != Z_OK && err != Z_STREAM_END)
    {
        OutputStringStream msgs;
        msgs << "InflateInputStreambuf::underflow(): inflate failed"
             << ": " << zError(err);
        // Throw an exception to immediately exit to the read() or similar
        // function and make istream set badbit
        throw IOException(msgs.str());
    }

    if(inflated_bytes > 0)
    {
        return traits_type::to_int_type(*gptr());
    }

    return traits_type::eof();
}



/** \brief Initializes the stream buffer.
 *
 * This function resets the zlib stream and purges input and output buffers.
 * It also repositions the input streambuf at stream_position.
 *
 * \warning
 * This method is called in the constructor, so it must not read anything
 * from the input streambuf m_inbuf (see notice in constructor.)
 *
 * \param[in] stream_position  A position to reset the inbuf to before
 *                             reading. Specify -1 to read from the
 *                             current position.
 *
 * \sa InflateInputStreambuf()
 */
bool InflateInputStreambuf::reset(offset_t stream_position)
{
    if(stream_position >= 0)
    {
        // reposition m_inbuf
        m_inbuf->pubseekpos(stream_position);
    }

    // m_zs.next_in and avail_in must be set according to
    // zlib.h (inline doc).
    m_zs.next_in = reinterpret_cast<Bytef *>(&m_invec[0]);
    m_zs.avail_in = 0;

    int err(Z_OK);
    if(m_zs_initialized)
    {
        // just reset it
        err = inflateReset(&m_zs);
    }
    else
    {
        // initialize it
        err = inflateInit2(&m_zs, -MAX_WBITS);
        /* windowBits is passed < 0 to tell that there is no zlib header.
           Note that in this case inflate *requires* an extra "dummy" byte
           after the compressed stream in order to complete decompression
           and return Z_STREAM_END.  We always have an extra "dummy" byte,
           because there is always some trailing data after the compressed
           data (either the next entry or the central directory).  */
        m_zs_initialized = true;
    }

    // streambuf init:
    // The important thing here, is that
    // - the pointers are not NULL (which would mean unbuffered)
    // - and that gptr() is not less than egptr() (so we trigger underflow
    //   the first time data is read).
    setg(&m_outvec[0], &m_outvec[0] + getBufferSize(), &m_outvec[0] + getBufferSize());

    return err == Z_OK;
}


} // zipios namespace

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
