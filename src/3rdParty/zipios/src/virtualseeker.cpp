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
 * \brief Implementation file that defines zipios::VirtualSeeker.
 *
 * The zipios::VirtualSeeker is used to handle offsets of a Zip archive
 * defined within a larger file. See the
 * zipios::ZipFile::openEmbeddedZipFile() function and the appendzip
 * tool for additional information.
 *
 * \sa appendzip.cpp
 * \sa zipios::ZipFile::openEmbeddedZipFile()
 */

#include "zipios/virtualseeker.hpp"

#include "zipios/zipiosexceptions.hpp"


namespace zipios
{


/** \class VirtualSeeker
 * \brief A virtual class used to see in a file embedded in another.
 *
 * The virtual seeker class is a simple definition of an object
 * that keeps track of a set of specified (virtual) file pointers
 * that mark start and end of a file inside another.
 *
 * An example of its use (and its reason for existence) is to
 * keep track of the file endings of a Zip file embedded in another
 * file (see the \ref appendzip_anchor "appendzip tool" and
 * the ZipFile::openEmbeddedZipFile() function).
 *
 * \bug
 * The class is not linked to an input stream when created or
 * the offsets get modified. This means the seek and tell functions
 * cannot be sure that the offsets are valid of the specified
 * input buffer.
 */


/** \brief Create a virtual seeker.
 *
 * This constructor defines a virtual seeker start and end offsets
 * on initialization. By default it is initialized to a transparent
 * seeker since the start and end are set to zero.
 *
 * \note
 * If the offsets are left undefined (both set to zero) then the virtual
 * seeker is viewed as a transparent seeker, meaning that it seeks in
 * the input streams as if it did not exist.
 *
 * \warning
 * The virtual seek end offset is quite peculiar in that it is defined
 * as a POSITIVE number from the end of the file, going backward. The
 * normal seekg() command expects a negative number of an offset to be
 * applied from the end of the file.
 *
 * \warning
 * The class is not attached to one specific input stream so there is no
 * way to verify that the offsets are valid (i.e. not representing an
 * empty virtual file or having offsets completely outside of the available
 * range.)
 *
 * \exception InvalidException
 * The two offsets must be positive.
 *
 * \param[in] start_offset  The start offset of the embedded file.
 * \param[in] end_offset  The end offset of the embedded file.
 */
VirtualSeeker::VirtualSeeker(offset_t start_offset, offset_t end_offset)
    : m_start_offset(start_offset)
    , m_end_offset(end_offset)
{
    if(m_start_offset < 0
    || m_end_offset < 0)
    {
        throw InvalidException("VirtualSeeker::VirtualSeeker(): the start and end offsets cannot be negative.");
    }
}


/** \brief Set the offsets of the virtual seeker.
 *
 * This function can be used to change the virtual seeker offsets.
 *
 * \exception InvalidException
 * The start offset must be before or equal to the end offset or
 * this exception is raised.
 *
 * \param[in] start_offset  The new start offset.
 * \param[in] end_offset  The new end offset.
 */
void VirtualSeeker::setOffsets(offset_t start_offset, offset_t end_offset)
{
    if(start_offset < 0
    || end_offset < 0)
    {
        throw InvalidException("VirtualSeeker::VirtualSeeker(): the start and end offsets cannot be negative.");
    }

    m_start_offset = start_offset;
    m_end_offset = end_offset;
}


/** \brief Retrieve the current offsets.
 *
 * This function retrieves the start and end offsets from the virtual
 * seeker object.
 *
 * \param[out] start_offset  Returns the start offset.
 * \param[out] end_offset  Returns the end offset.
 */
void VirtualSeeker::getOffsets(offset_t & start_offset, offset_t & end_offset) const
{
    start_offset = m_start_offset;
    end_offset = m_end_offset;
}


/** \brief Return the start offset.
 *
 * This function returns a copy of the start offset.
 *
 * \return The start offset.
 */
offset_t VirtualSeeker::startOffset() const
{
    return m_start_offset;
}


/** \brief Return the end offset.
 *
 * This function returns a copy of the end offset.
 *
 * \return The end offset.
 */
offset_t VirtualSeeker::endOffset() const
{
    return m_end_offset;
}


/** \brief Seek within the embedded file.
 *
 * This function changes the file pointer in \p is to the position
 * specified in offset.
 *
 * The direction can be indicated by \p sd.
 *
 * \param[in,out] is  The stream which pointer is to be changed.
 * \param[in] offset  Relative position to set the input pointer to.
 * \param[in] sd  The stream direction to use to apply offset.
 */
void VirtualSeeker::vseekg(std::istream &is, offset_t offset, std::ios::seekdir sd) const
{
    switch(sd)
    {
    case std::ios::cur:
        break;

    case std::ios::beg:
        offset += m_start_offset;
        break;

    case std::ios::end:
        // This definitively looks weird because this class makes use
        // of a POSITIVE offset from the end of the file as the end
        // offset. The parameter 'offset' is expected to be negative
        // or zero in this case.
        offset -= m_end_offset;
        break;

    default:
        throw std::logic_error("VirtualSeekManager::vseekg(): error - unknown seekdir");

    }

    is.seekg(offset, sd);
}


/** \brief Current position within the sub-file.
 *
 * This function calculates the position (file current pointer) within
 * the embedded file in the specified stream.
 *
 * If the position in the existing file is too large or too small, then
 * the function returns -1.
 *
 * \param[in] is  The stream to get the position from.
 *
 * \return The stream offset within the embedded file.
 */
std::streampos VirtualSeeker::vtellg(std::istream & is) const
{
    /** \TODO
     * We may want to get the size of the file and verify that the
     * resulting position is valid. The m_end_offset does not really
     * mean anything at this point that we could use to verify the
     * position boundaries (since it is a positive size from the
     * end of the file.)
     */
    return is.tellg() - m_start_offset;
}


} // zipios namespace

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
