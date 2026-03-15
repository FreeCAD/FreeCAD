// SPDX-License-Identifier: LGPL-2.1-or-later

/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2019 Zheng Lei (realthunder.dev@gmail.com)               *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#pragma once


#include "Base64.h"
#include "FCGlobal.h"

#include <boost/iostreams/concepts.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/operations.hpp>


// NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic,
// cppcoreguidelines-pro-bounds-constant-array-index, cppcoreguidelines-avoid-magic-numbers,
// readability-magic-numbers)

namespace Base
{

namespace bio = boost::iostreams;

enum class Base64ErrorHandling
{
    throws,
    silent
};
static constexpr int base64DefaultBufferSize {80};

/** A base64 encoder that can be used as a boost iostream filter
 *
 * @sa See create_base64_encoder() for example usage
 */
struct base64_encoder
{

    using char_type = char;
    struct category: bio::multichar_output_filter_tag, bio::closable_tag, bio::optimally_buffered_tag
    {
    };

    /** Constructor
     * @param line_size: line size for the output base64 string, 0 to
     * disable segmentation.
     */
    explicit base64_encoder(std::size_t line_size)
        : line_size(line_size)
    {}

    std::streamsize optimal_buffer_size() const
    {
        static constexpr int defaultBufferSize {1024};
        return static_cast<std::streamsize>(
            base64_encode_size(line_size != 0U ? line_size : defaultBufferSize)
        );
    }

    template<typename Device>
    void close(Device& dev)
    {
        if (pending_size) {
            base64_encode(buffer, pending.data(), pending_size);
        }
        if (!buffer.empty()) {
            bio::write(dev, buffer.c_str(), buffer.size());
            if (line_size) {
                bio::put(dev, '\n');
            }
            buffer.clear();
        }
        else if (pos && line_size) {
            bio::put(dev, '\n');
        }
    }

    template<typename Device>
    std::streamsize write(Device& dev, const char_type* str, std::streamsize n)
    {
        std::streamsize res = n;

        if (pending_size > 0) {
            while (n && pending_size < 3) {
                pending[pending_size] = *str++;
                ++pending_size;
                --n;
            }
            if (pending_size != 3) {
                return res;
            }

            base64_encode(buffer, pending.data(), 3);
        }
        pending_size = n % 3;
        n = n / 3 * 3;
        base64_encode(buffer, str, n);
        str += n;
        for (unsigned i = 0; i < pending_size; ++i) {
            pending[i] = str[i];
        }

        const char* buf = buffer.c_str();
        const char* end = buf + buffer.size();
        if (line_size && buffer.size() >= line_size - pos) {
            bio::write(dev, buf, line_size - pos);
            bio::put(dev, '\n');
            buf += line_size - pos;
            pos = 0;
            for (; end - buf >= (int)line_size; buf += line_size) {
                bio::write(dev, buf, line_size);
                bio::put(dev, '\n');
            }
        }
        pos += end - buf;
        bio::write(dev, buf, end - buf);
        buffer.clear();
        return n;
    }

    std::size_t line_size;
    std::size_t pos = 0;
    std::size_t pending_size = 0;
    std::array<unsigned char, 3> pending {};
    std::string buffer;
};

/** A base64 decoder that can be used as a boost iostream filter
 *
 * @sa See create_base64_decoder() for example usage
 */
struct base64_decoder
{

    using char_type = char;
    struct category: bio::multichar_input_filter_tag, bio::optimally_buffered_tag
    {
    };

    /** Constructor
     * @param line_size: line size of the encoded base64 string. This is
     *                   used just as a suggestion for better buffering.
     * @param silent: whether to throw on invalid non white space character.
     */
    base64_decoder(std::size_t line_size, Base64ErrorHandling errHandling)
        : line_size(line_size)
        , errHandling(errHandling)
    {}

    std::streamsize optimal_buffer_size() const
    {
        static constexpr int defaultBufferSize {1024};
        return static_cast<std::streamsize>(
            base64_encode_size(line_size != 0U ? line_size : defaultBufferSize)
        );
    }

    template<typename Device>
    std::streamsize read(Device& dev, char_type* str, std::streamsize n)
    {
        static auto table = base64_decode_table();

        if (!n) {
            return 0;
        }

        std::streamsize count = 0;

        for (;;) {
            while (pending_out < out_count) {
                *str++ = char_array_3[pending_out++];
                ++count;
                if (--n == 0) {
                    return count;
                }
            }

            if (eof) {
                return count ? count : -1;
            }

            for (;;) {
                int newChar = bio::get(dev);
                if (newChar < 0) {
                    eof = true;
                    if (pending_in <= 1) {
                        if (pending_in == 1 && errHandling == Base64ErrorHandling::throws) {
                            throw BOOST_IOSTREAMS_FAILURE("Unexpected ending of base64 string");
                        }
                        return count ? count : -1;
                    }
                    out_count = pending_in - 1;
                    pending_in = 4;
                }
                else {
                    signed char decodedChar = table[newChar];
                    if (decodedChar < 0) {
                        if (decodedChar == -2 || errHandling == Base64ErrorHandling::silent) {
                            continue;
                        }
                        throw BOOST_IOSTREAMS_FAILURE("Invalid character in base64 string");
                    }
                    char_array_4[pending_in++] = (char)decodedChar;
                }
                if (pending_in == 4) {
                    pending_out = pending_in = 0;
                    char_array_3[0] = static_cast<char>(
                        (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4)
                    );
                    char_array_3[1] = static_cast<char>(
                        ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2)
                    );
                    char_array_3[2] = static_cast<char>(
                        ((char_array_4[2] & 0x3) << 6) + char_array_4[3]
                    );
                    break;
                }
            }
        }
    }

    std::size_t line_size;
    std::uint8_t pending_in = 0;
    std::array<char, 4> char_array_4 {};
    std::uint8_t pending_out = 3;
    std::uint8_t out_count = 3;
    std::array<char, 3> char_array_3 {};
    Base64ErrorHandling errHandling;
    bool eof = false;
};

/** Create an output stream that transforms the input binary data to base64 strings
 *
 * @param out: the downstream output stream that will be fed with base64 string
 * @param line_size: line size of the base64 string. Zero to disable segmenting.
 *
 * @return A unique pointer to an output stream that can transforms the
 * input binary data to base64 strings.
 */
inline std::unique_ptr<std::ostream> create_base64_encoder(
    std::ostream& out,
    std::size_t line_size = base64DefaultBufferSize
)
{
    std::unique_ptr<std::ostream> res(new bio::filtering_ostream);
    auto* filteringStream = dynamic_cast<bio::filtering_ostream*>(res.get());
    filteringStream->push(base64_encoder(line_size));
    filteringStream->push(out);
    return res;
}

/** Create an output stream that stores the input binary data to file as base64 strings
 *
 * @param filename: the output file path
 * @param line_size: line size of the base64 string. Zero to disable segmenting.
 *
 * @return A unique pointer to an output stream that can transforms the
 * input binary data to base64 strings.
 */
inline std::unique_ptr<std::ostream> create_base64_encoder(
    const std::string& filepath,
    std::size_t line_size = base64DefaultBufferSize
)
{
    std::unique_ptr<std::ostream> res(new bio::filtering_ostream);
    auto* filteringStream = dynamic_cast<bio::filtering_ostream*>(res.get());
    filteringStream->push(base64_encoder(line_size));
    filteringStream->push(bio::file_sink(filepath));
    return res;
}

/** Create an input stream that can transform base64 into binary
 *
 * @param in: input upstream.
 * @param line_size: line size of the encoded base64 string. This is
 *                   used just as a suggestion for better buffering.
 * @param silent: whether to throw on invalid non white space character.
 *
 * @return A unique pointer to an input stream that read from the given
 * upstream and transform the read base64 strings into binary data.
 */
inline std::unique_ptr<std::istream> create_base64_decoder(
    std::istream& in,
    std::size_t line_size = base64DefaultBufferSize,
    Base64ErrorHandling errHandling = Base64ErrorHandling::silent
)
{
    std::unique_ptr<std::istream> res(new bio::filtering_istream);
    auto* filteringStream = dynamic_cast<bio::filtering_istream*>(res.get());
    filteringStream->push(base64_decoder(line_size, errHandling));
    filteringStream->push(in);
    return res;
}

/** Create an input stream that can transform base64 into binary
 *
 * @param filepath: input file.
 * @param ending: optional ending character. If non zero, the filter
 *                will signal EOF when encounter this character.
 * @param putback: if true and the filter read the ending character
 *                 it will put it back into upstream
 * @param line_size: line size of the encoded base64 string. This is
 *                   used just as a suggestion for better buffering.
 * @param silent: whether to throw on invalid non white space character.
 *
 * @return A unique pointer to an input stream that read from the given
 * file and transform the read base64 strings into binary data.
 */
inline std::unique_ptr<std::istream> create_base64_decoder(
    const std::string& filepath,
    std::size_t line_size = base64DefaultBufferSize,
    Base64ErrorHandling errHandling = Base64ErrorHandling::silent
)
{
    std::unique_ptr<std::istream> res(new bio::filtering_istream);
    auto* filteringStream = dynamic_cast<bio::filtering_istream*>(res.get());
    filteringStream->push(base64_decoder(line_size, errHandling));
    filteringStream->push(bio::file_source(filepath));
    return res;
}

}  // namespace Base

// NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic,
// cppcoreguidelines-pro-bounds-constant-array-index, cppcoreguidelines-avoid-magic-numbers,
// readability-magic-numbers)
