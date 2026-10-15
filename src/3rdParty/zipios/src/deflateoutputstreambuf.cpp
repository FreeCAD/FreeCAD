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
 * \brief Implementation of zipios::DeflateOutputStreambuf.
 *
 * This is the counterpart of the zipios::InflateInputStreambuf.
 */

#include "deflateoutputstreambuf.hpp"

#include "zipios/zipiosexceptions.hpp"

#include "zipios_common.hpp"


namespace zipios
{

/** \class DeflateOutputStreambuf
 * \brief A class to handle stream deflate on the fly.
 *
 * DeflateOutputStreambuf is an output stream filter, that deflates
 * the data that is written to it before it passes it on to the
 * output stream it is attached to. Deflation/Inflation is a
 * compression/decompression method used in gzip and zip. The zlib
 * library is used to perform the actual deflation, this class only
 * wraps the functionality in an output stream filter.
 */


/** \brief Initialize a DeflateOutputStreambuf object.
 *
 * This function initializes the DeflateOutputStreambuf object to make it
 * ready for compressing data using the zlib library.
 *
 * \param[in,out] outbuf  The streambuf to use for output.
 */
DeflateOutputStreambuf::DeflateOutputStreambuf(std::streambuf * outbuf)
    : FilterOutputStreambuf(outbuf)
    , m_invec(getBufferSize())
    , m_outvec(getBufferSize())
{
    // NOTICE: It is important that this constructor and the methods it
    //         calls do not do anything with the output streambuf m_outbuf.
    //         The reason is that this class can be sub-classed, and the
    //         sub-class should get a chance to write to the buffer first.
}


/** \brief Clean up any resources used by this object.
 *
 * The destructor makes sure that the zlib library is done with all
 * the input and output data by calling various flush functions. It
 * then makes sure that the remaining data from zlib is printed in
 * the output file.
 *
 * This is similar to calling closeStream() explicitly.
 */
DeflateOutputStreambuf::~DeflateOutputStreambuf()
{
    closeStream();
}


/** \brief Initialize the zlib library.
 *
 * This method is called in the constructor, so it must not write
 * anything to the output streambuf m_outbuf (see notice in
 * constructor.)
 *
 * It will initialize the output stream as required to accept data
 * to be compressed using the zlib library. The compression level
 * is expected to come from the FileEntry which is about to be
 * saved in the file.
 *
 * \param[in] compression_level  The level of compression. A number from 1 to
 * 100 or a special number representing the best, minimum, maximum compression
 * available.
 *
 * \return true if the initialization succeeded, false otherwise.
 */
bool DeflateOutputStreambuf::init(FileEntry::CompressionLevel compression_level)
{
    if(m_zs_initialized)
    {
        // This is excluded from the coverage since if we reach this
        // line there is an internal error that needs to be fixed.
        throw std::logic_error("DeflateOutputStreambuf::init(): initialization function called when the class is already initialized. This is not supported."); // LCOV_EXCL_LINE
    }
    m_zs_initialized = true;

    int const default_mem_level(8);

    int zlevel(Z_NO_COMPRESSION);
    switch(compression_level)
    {
    case FileEntry::COMPRESSION_LEVEL_DEFAULT:
        zlevel = Z_DEFAULT_COMPRESSION;
        break;

    case FileEntry::COMPRESSION_LEVEL_SMALLEST:
        zlevel = Z_BEST_COMPRESSION;
        break;

    case FileEntry::COMPRESSION_LEVEL_FASTEST:
        zlevel = Z_BEST_SPEED;
        break;

    case FileEntry::COMPRESSION_LEVEL_NONE:
        throw std::logic_error("the compression level NONE is not supported in DeflateOutputStreambuf::init()"); // LCOV_EXCL_LINE

    default:
        if(compression_level < FileEntry::COMPRESSION_LEVEL_MINIMUM
        || compression_level > FileEntry::COMPRESSION_LEVEL_MAXIMUM)
        {
            // This is excluded from the coverage since if we reach this
            // line there is an internal error that needs to be fixed.
            throw std::logic_error("the compression level must be defined between -3 and 100, see the zipios/fileentry.hpp for a list of valid levels."); // LCOV_EXCL_LINE
        }
        // The zlevel is calculated linearly from the user specified value
        // of 1 to 100
        //
        // The calculation goes as follow:
        //
        //    x = user specified value - 1    (0 to 99)
        //    x = x * 8                       (0 to 792)
        //    x = x + 11 / 2                  (5 to 797, i.e. +5 with integers)
        //    x = x / 99                      (0 to 8)
        //    x = x + 1                       (1 to 9)
        //
        zlevel = ((compression_level - 1) * 8 + 11 / 2) / 99 + 1;
        break;

    }

    // m_zs.next_in and avail_in must be set according to
    // zlib.h (inline doc).
    m_zs.next_in  = reinterpret_cast<unsigned char *>(&m_invec[0]);
    m_zs.avail_in = 0;

    m_zs.next_out  = reinterpret_cast<unsigned char *>(&m_outvec[0]);
    m_zs.avail_out = getBufferSize();

    //
    // windowBits is passed -MAX_WBITS to tell that no zlib
    // header should be written.
    //
    int const err = deflateInit2(&m_zs, zlevel, Z_DEFLATED, -MAX_WBITS, default_mem_level, Z_DEFAULT_STRATEGY);
    if(err != Z_OK)
    {
        // Not too sure how we could generate an error here, the deflateInit2()
        // would fail if (1) there is not enough memory and (2) if a parameter
        // is out of wack which neither can be generated from the outside
        // (well... not easily)
        std::ostringstream msgs; // LCOV_EXCL_LINE
        msgs << "DeflateOutputStreambuf::init(): error while initializing zlib, " << zError(err) << std::endl; // LCOV_EXCL_LINE
        throw IOException(msgs.str()); // LCOV_EXCL_LINE
    }

    // streambuf init:
    setp(&m_invec[0], &m_invec[0] + getBufferSize());

    m_crc32 = crc32(0, Z_NULL, 0);

    return err == Z_OK;
}


/** \brief Closing the stream.
 *
 * This function is expected to be called once the stream is getting
 * closed (the buffer is destroyed.)
 *
 * It ensures that the zlib library last few bytes get flushed and
 * then mark the class as closed.
 *
 * Note that this function can be called to close the current zlib
 * library stream and start a new one. It is actually called from
 * the putNextEntry() function (via the closeEntry() function.)
 */
void DeflateOutputStreambuf::closeStream()
{
    if(m_zs_initialized)
    {
        m_zs_initialized = false;

        // flush any remaining data
        endDeflation();

        int const err(deflateEnd(&m_zs));
        if(err != Z_OK) // when we close a directory, we get the Z_DATA_ERROR!
        {
            // There are not too many cases which break the deflateEnd()
            // function call...
            std::ostringstream msgs; // LCOV_EXCL_LINE
            msgs << "DeflateOutputStreambuf::closeStream(): deflateEnd failed: " << zError(err) << std::endl; // LCOV_EXCL_LINE
            throw IOException(msgs.str()); // LCOV_EXCL_LINE
        }
    }
}


/** \brief Get the CRC32 of the file.
 *
 * This function returns the CRC32 for the current file.
 *
 * The returned value is the CRC for the data that has been compressed
 * already (due to calls to overflow()). As DeflateOutputStreambuf may
 * buffer an arbitrary amount of bytes until closeStream() has been
 * invoked, the returned value is not very useful before closeStream()
 * has been called.
 *
 * \return The CRC32 of the last file that was passed through.
 */
uint32_t DeflateOutputStreambuf::getCrc32() const
{
    return m_crc32;
}


/** \brief Retrieve the size of the file deflated.
 *
 * This function returns the number of bytes written to the
 * streambuf object and that were processed from the input
 * buffer by the compressor. After closeStream() has been
 * called this number is the total number of bytes written
 * to the stream. In other words, the size of the uncompressed
 * data.
 *
 * \return The uncompressed size of the file that got written here.
 */
size_t DeflateOutputStreambuf::getSize() const
{
    return m_overflown_bytes;
}


/** \brief Handle an overflow.
 *
 * This function is called by the streambuf implementation whenever
 * "too many bytes" are in the output buffer, ready to be compressed.
 *
 * \exception IOException
 * This exception is raised whenever the overflow() function calls
 * a zlib library function which returns an error.
 *
 * \param[in] c  The character (byte) that overflowed the buffer.
 *
 * \return Always zero (0).
 */
int DeflateOutputStreambuf::overflow(int c)
{
    int err(Z_OK);

    m_zs.avail_in = pptr() - pbase();
    m_zs.next_in = reinterpret_cast<unsigned char *>(&m_invec[0]);

    if(m_zs.avail_in > 0)
    {
        m_crc32 = crc32(m_crc32, m_zs.next_in, m_zs.avail_in); // update crc32

        m_zs.next_out = reinterpret_cast<unsigned char *>(&m_outvec[0]);
        m_zs.avail_out = getBufferSize();

        // Deflate until m_invec is empty.
        while((m_zs.avail_in > 0 || m_zs.avail_out == 0) && err == Z_OK)
        {
            if(m_zs.avail_out == 0)
            {
                flushOutvec();
            }

            err = deflate(&m_zs, Z_NO_FLUSH);
        }
    }

    // somehow we need this flush here or it fails
    flushOutvec();

    // Update 'put' pointers
    setp(&m_invec[0], &m_invec[0] + getBufferSize());

    if(err != Z_OK && err != Z_STREAM_END)
    {
        // Throw an exception to make istream set badbit
        //
        // This is marked as not cover-able by tests because the calls
        // that access this function only happen in an internal loop and
        // even if we were to write a direct test, I do not see how
        // we could end up with an error here
        OutputStringStream msgs; // LCOV_EXCL_LINE
        msgs << "Deflation failed:" << zError(err); // LCOV_EXCL_LINE
        throw IOException(msgs.str()); // LCOV_EXCL_LINE
    }

    if(c != EOF)
    {
        *pptr() = c;
        pbump(1);
    }

    return 0;
}


/** \brief Synchronize the buffer.
 *
 * The sync() function is expected to clear the input buffer so that
 * any new data read from the input (i.e. a file) are re-read from
 * disk. However, a call to sync() could break the filtering
 * functionality so we do not implement it at all.
 *
 * This means you are stuck with the existing buffer. But to make
 * sure the system understands that, we always returns -1.
 */
int DeflateOutputStreambuf::sync() // LCOV_EXCL_LINE
{
    return -1; // LCOV_EXCL_LINE
}


/** \brief Flush the cached output data.
 *
 * This function flushes m_outvec and updates the output pointer
 * and size m_zs.next_out and m_zs.avail_out.
 */
void DeflateOutputStreambuf::flushOutvec()
{
    /** \TODO
     * We need to redesign the class to allow for STORED files to
     * flow through without the need to have this crap of bytes to
     * skip...
     */
    std::size_t const deflated_bytes(getBufferSize() - m_zs.avail_out);
    if(deflated_bytes > 0)
    {
        std::size_t const bc(m_outbuf->sputn(&m_outvec[0], deflated_bytes));
        if(deflated_bytes != bc)
        {
            // Without implementing our own stream in our test, this
            // cannot really be reached because it is all happening
            // inside the same loop in ZipFile::saveCollectionToArchive()
            throw IOException("DeflateOutputStreambuf::flushOutvec(): write to buffer failed."); // LCOV_EXCL_LINE
        }
    }

    m_zs.next_out = reinterpret_cast<unsigned char *>(&m_outvec[0]);
    m_zs.avail_out = getBufferSize();
}


/** \brief End deflation of current file.
 *
 * This function flushes the remaining data in the zlib buffers,
 * after which the only possible operations are deflateEnd() or
 * deflateReset().
 */
void DeflateOutputStreambuf::endDeflation()
{
    overflow();

    m_zs.next_out = reinterpret_cast<unsigned char *>(&m_outvec[0]);
    m_zs.avail_out = getBufferSize();

    // Deflate until _invec is empty.
    int err(Z_OK);

    // make sure to NOT call deflate() if nothing was written to the
    // deflate output stream, otherwise we get a "spurious" (as far
    // Zip archives are concerned) 0x03 0x00 marker from the zlib
    // library
    //
    if(m_overflown_bytes > 0)
    {
        while(err == Z_OK)
        {
            if(m_zs.avail_out == 0)
            {
                flushOutvec();
            }

            err = deflate(&m_zs, Z_FINISH);
        }
    }
    else
    {
        // this is not expected to happen, but it can
        err = Z_STREAM_END; // LCOV_EXCL_LINE
    }

    flushOutvec();

    if(err != Z_STREAM_END)
    {
        // This is marked as not cover-able because the calls that
        // access this function only happen in an internal loop and
        // even if we were to write a direct test, I do not see how
        // we could end up with an error here
        std::ostringstream msgs; // LCOV_EXCL_LINE
        msgs << "DeflateOutputStreambuf::endDeflation(): deflate() failed: " // LCOV_EXCL_LINE
             << zError(err) << std::endl; // LCOV_EXCL_LINE
        throw IOException(msgs.str()); // LCOV_EXCL_LINE
    }
}


} // namespace

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
