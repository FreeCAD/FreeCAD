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

#include <array>
#include <cstdint>
#include <ios>
#include <istream>
#include <memory>
#include <ostream>
#include <streambuf>
#include <string>
#include <vector>


// NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic,
// cppcoreguidelines-pro-bounds-constant-array-index, cppcoreguidelines-avoid-magic-numbers,
// readability-magic-numbers)

namespace Base
{

enum class Base64ErrorHandling
{
    throws,
    silent
};
static constexpr int base64DefaultBufferSize {80};

/** A stream buffer that base64 encodes everything written to it
 *
 * The encoded text is written to the given downstream buffer. The last
 * (possibly partial) group is encoded and flushed by finish(), which is
 * also called by the destructor.
 *
 * @sa See create_base64_encoder() for example usage
 */
class Base64EncoderBuf: public std::streambuf
{
public:
    /** Constructor
     * @param sink: the downstream buffer that receives the base64 text.
     * @param line_size: line size for the output base64 string, 0 to
     * disable segmentation.
     */
    Base64EncoderBuf(std::streambuf& sink, std::size_t line_size)
        : sink(sink)
        , line_size(line_size)
    {}

    ~Base64EncoderBuf() override
    {
        try {
            finish();
        }
        catch (...) {  // NOLINT(bugprone-empty-catch)
        }
    }

    Base64EncoderBuf(const Base64EncoderBuf&) = delete;
    Base64EncoderBuf(Base64EncoderBuf&&) = delete;
    Base64EncoderBuf& operator=(const Base64EncoderBuf&) = delete;
    Base64EncoderBuf& operator=(Base64EncoderBuf&&) = delete;

    /// Encode any pending input bytes and terminate the last line
    void finish()
    {
        if (finished) {
            return;
        }
        finished = true;
        if (pending_size) {
            base64_encode(buffer, pending.data(), pending_size);
            pending_size = 0;
        }
        if (!buffer.empty()) {
            sink.sputn(buffer.c_str(), static_cast<std::streamsize>(buffer.size()));
            if (line_size) {
                sink.sputc('\n');
            }
            buffer.clear();
        }
        else if (pos && line_size) {
            sink.sputc('\n');
        }
    }

protected:
    int_type overflow(int_type ch) override
    {
        if (traits_type::eq_int_type(ch, traits_type::eof())) {
            return traits_type::not_eof(ch);
        }
        const char c = traits_type::to_char_type(ch);
        return xsputn(&c, 1) == 1 ? ch : traits_type::eof();
    }

    std::streamsize xsputn(const char* str, std::streamsize n) override
    {
        const std::streamsize res = n;

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
            sink.sputn(buf, static_cast<std::streamsize>(line_size - pos));
            sink.sputc('\n');
            buf += line_size - pos;
            pos = 0;
            for (; end - buf >= static_cast<std::ptrdiff_t>(line_size); buf += line_size) {
                sink.sputn(buf, static_cast<std::streamsize>(line_size));
                sink.sputc('\n');
            }
        }
        pos += end - buf;
        sink.sputn(buf, end - buf);
        buffer.clear();
        return res;
    }

    int sync() override
    {
        return sink.pubsync();
    }

private:
    std::streambuf& sink;
    std::size_t line_size;
    std::size_t pos = 0;
    std::size_t pending_size = 0;
    std::array<unsigned char, 3> pending {};
    std::string buffer;
    bool finished = false;
};

/** A stream buffer that base64 decodes the content of an upstream buffer
 *
 * @sa See create_base64_decoder() for example usage
 */
class Base64DecoderBuf: public std::streambuf
{
public:
    /** Constructor
     * @param source: the upstream buffer holding the base64 text.
     * @param line_size: line size of the encoded base64 string. This is
     *                   used just as a suggestion for better buffering.
     * @param errHandling: whether to throw on invalid non white space character.
     */
    Base64DecoderBuf(std::streambuf& source, std::size_t line_size, Base64ErrorHandling errHandling)
        : source(source)
        , errHandling(errHandling)
        , out_buffer(base64_encode_size(line_size != 0U ? line_size : 1024))
    {}

    ~Base64DecoderBuf() override = default;

    Base64DecoderBuf(const Base64DecoderBuf&) = delete;
    Base64DecoderBuf(Base64DecoderBuf&&) = delete;
    Base64DecoderBuf& operator=(const Base64DecoderBuf&) = delete;
    Base64DecoderBuf& operator=(Base64DecoderBuf&&) = delete;

protected:
    int_type underflow() override
    {
        if (gptr() < egptr()) {
            return traits_type::to_int_type(*gptr());
        }
        const std::streamsize count
            = decode(out_buffer.data(), static_cast<std::streamsize>(out_buffer.size()));
        if (count <= 0) {
            return traits_type::eof();
        }
        setg(out_buffer.data(), out_buffer.data(), out_buffer.data() + count);
        return traits_type::to_int_type(*gptr());
    }

private:
    std::streamsize decode(char* str, std::streamsize n)
    {
        static const auto table = base64_decode_table();

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
                const int_type newChar = source.sbumpc();
                if (traits_type::eq_int_type(newChar, traits_type::eof())) {
                    eof = true;
                    if (pending_in <= 1) {
                        if (pending_in == 1 && errHandling == Base64ErrorHandling::throws) {
                            throw std::ios_base::failure("Unexpected ending of base64 string");
                        }
                        return count ? count : -1;
                    }
                    out_count = pending_in - 1;
                    pending_in = 4;
                }
                else {
                    const signed char decodedChar
                        = table[static_cast<unsigned char>(traits_type::to_char_type(newChar))];
                    if (decodedChar < 0) {
                        if (decodedChar == -2 || errHandling == Base64ErrorHandling::silent) {
                            continue;
                        }
                        throw std::ios_base::failure("Invalid character in base64 string");
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

    std::streambuf& source;
    Base64ErrorHandling errHandling;
    std::vector<char> out_buffer;
    std::uint8_t pending_in = 0;
    std::array<char, 4> char_array_4 {};
    std::uint8_t pending_out = 3;
    std::uint8_t out_count = 3;
    std::array<char, 3> char_array_3 {};
    bool eof = false;
};

namespace detail
{

/// An output stream owning a base64 encoding buffer
class Base64OStream: public std::ostream
{
public:
    Base64OStream(std::ostream& out, std::size_t line_size)
        : std::ostream(nullptr)
        , buf(*out.rdbuf(), line_size)
    {
        rdbuf(&buf);
    }

private:
    Base64EncoderBuf buf;
};

/// An input stream owning a base64 decoding buffer
class Base64IStream: public std::istream
{
public:
    Base64IStream(std::istream& in, std::size_t line_size, Base64ErrorHandling errHandling)
        : std::istream(nullptr)
        , buf(*in.rdbuf(), line_size, errHandling)
    {
        rdbuf(&buf);
    }

private:
    Base64DecoderBuf buf;
};

}  // namespace detail

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
    return std::make_unique<detail::Base64OStream>(out, line_size);
}

/** Create an input stream that can transform base64 into binary
 *
 * @param in: input upstream.
 * @param line_size: line size of the encoded base64 string. This is
 *                   used just as a suggestion for better buffering.
 * @param errHandling: whether to throw on invalid non white space character.
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
    return std::make_unique<detail::Base64IStream>(in, line_size, errHandling);
}

}  // namespace Base

// NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic,
// cppcoreguidelines-pro-bounds-constant-array-index, cppcoreguidelines-avoid-magic-numbers,
// readability-magic-numbers)
