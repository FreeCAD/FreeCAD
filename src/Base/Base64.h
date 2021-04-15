/* 
   base64.cpp and base64.h

   Copyright (C) 2004-2008 René Nyffenegger

   This source code is provided 'as-is', without any express or implied
   warranty. In no event will the author be held liable for any damages
   arising from the use of this software.

   Permission is granted to anyone to use this software for any purpose,
   including commercial applications, and to alter it and redistribute it
   freely, subject to the following restrictions:

   1. The origin of this source code must not be misrepresented; you must not
      claim that you wrote the original source code. If you use this source code
      in a product, an acknowledgment in the product documentation would be
      appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
      misrepresented as being the original source code.

   3. This notice may not be removed or altered from any source distribution.

   René Nyffenegger rene.nyffenegger@adp-gmbh.ch

   Copyright (C) 2019 Zheng Lei (realthunder.dev@gmail.com)

   * Added support of in-line transformation using boost iostream for memory
     efficiency
*/
#ifndef BASE_BASE64_H
#define BASE_BASE64_H

#include <string>
#include <memory>
#include <vector>
#include <boost/iostreams/concepts.hpp>
#include <boost/iostreams/operations.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/device/file.hpp>

namespace Base
{

    /// Returns the max bytes of a encoded base64 string
    inline std::size_t base64_encode_size(std::size_t len) {
        return 4 * ((len + 2) / 3);
    }

    /// Returns the max bytes of a decoded base64 binary string
    inline std::size_t base64_decode_size(std::size_t len) {
        return len / 4 * 3;
    }

    /** Encode input binary with base64
     * @param out: output buffer with minimum size of base64_encode(len)
     * appending new data.
     * @param in: input binary data
     * @param len: input length
     * @return The character count written to output.
     */
	BaseExport std::size_t base64_encode(char *out, void const *in , std::size_t len);

    /** Return the internal base64 decoding table
     * 
     * The table maps from any 8-bit character to the decoded binary bits.
     * Valid base64 characters are mapped to the corresponding 6-bit binary
     * data. White space (space, tab, vtab, CR and LF) characters are mapped
     * to -2. Other invalid characters are mapped to -1.
     */
    BaseExport const signed char *base64_decode_table();

    /** Decode the input base64 string into binary data
     * @param out: output buffer with minimum size of base64_encode(len)
     * appending new data.
     * @param in: input binary data
     * @param len: input length
     * @return Return a pair of output size and input read size. Compare the
     * read size to input size to check for error.
     */
	BaseExport std::pair<std::size_t, std::size_t> base64_decode(
            void *out, char const *, std::size_t len);

    /** Encode input binary into base64 string
     * @param out: output string. Note that the string is not cleared before
     *             adding new content.
     * @param in: input binary data
     * @param len: input length
     */
	inline void base64_encode(std::string &out, void const* in , std::size_t len) {
        std::size_t size = out.size();
        out.resize(size + base64_encode_size(len));
        len = base64_encode(&out[size],in,len);
        out.resize(size+len);
    }

    /** Encode input binary into base64 string
     * @param in: input binary data
     * @param len: input length
     * @return Return the base64 string.
     */
	inline std::string base64_encode(void const* in , std::size_t len) {
        std::string out;
        base64_encode(out,in,len);
        return out;
    }

    /** Decode base64 string into binary data
     * @param out: output binary data. Note that the data is not cleared before
     *             adding new content.
     * @param in: input base64 string
     * @param len: input length
     * @return Return the processed input length. Compare this with the
     * argument \c len to check for error.
     */
    template<typename T>
	inline std::size_t base64_decode(T &out, char const *in, std::size_t len) {
        std::size_t size = out.size();
        out.resize(size + base64_decode_size(len));
        std::pair<std::size_t,std::size_t> res = base64_decode(&out[size],in,len);
        out.resize(size + res.first);
        return res.second;
    }

    /** Decode base64 string into binary data
     * @param out: output binary data. Note that the data is not cleared before
     *             adding new content.
     * @param s: input base64 string
     * @return Return the processed input length. Compare this with the
     * argument \c len to check for error.
     */
    template<typename T>
	inline std::size_t base64_decode(T &out, std::string const &s) {
        return base64_decode(out,s.c_str(),s.size());
    }

    /** Decode base64 string into binary data
     * @param out  adding new content.
     * @param s: input base64 string
     * @return Return the decoded binary data.
     */
    inline std::string base64_decode(std::string const& s) {
        std::string out;
        base64_decode(out,s.c_str(),s.size());
        return out;
    }

    namespace bio = boost::iostreams;

    /** A base64 encoder that can be used as a boost iostream filter
     *
     * @sa See create_base64_encoder() for example usage
     */
    struct base64_encoder {

        typedef char char_type;
        struct category : bio::multichar_output_filter_tag
                        , bio::closable_tag
                        , bio::optimally_buffered_tag
        {};

        /** Constructor
         * @param line_size: line size for the output base64 string, 0 to
         * disable segmentation.
         */
        base64_encoder(std::size_t line_size)
            :line_size(line_size)
        {
        }

        std::streamsize optimal_buffer_size() const {
            return base64_decode_size(line_size);
        }

        template<typename Device>
        void close(Device &dev)
        {
            if(pending_size)
                base64_encode(buffer,pending,pending_size);
            if(buffer.size()) {
                bio::write(dev,buffer.c_str(),buffer.size());
                if(line_size)
                    bio::put(dev,'\n');
                buffer.clear();
            }else if(pos && line_size)
                bio::put(dev,'\n');
        }

        template<typename Device>
        std::streamsize write(Device& dev, const char_type* s, std::streamsize n) {
            std::streamsize res = n;

            if(pending_size) {
                while(n && pending_size < 3) {
                    pending[pending_size++] = *s++;
                    --n;
                }
                if(pending_size!=3)
                    return res;

                base64_encode(buffer,pending,3);
            }
            pending_size = n%3;
            n = n/3*3;
            base64_encode(buffer,s,n);
            s += n;
            for(unsigned i=0;i<pending_size;++i)
                pending[i] = s[i];

            const char *buf = buffer.c_str();
            const char *end = buf + buffer.size();
            if(line_size && buffer.size()>=line_size-pos) {
                bio::write(dev,buf,line_size-pos);
                bio::put(dev,'\n');
                buf += line_size-pos;
                pos = 0;
                for(; end-buf>=(int)line_size; buf+=line_size) {
                    bio::write(dev,buf,line_size);
                    bio::put(dev,'\n');
                }
            }
            pos += end-buf;
            bio::write(dev,buf,end-buf);
            buffer.clear();
            return	n;
        }

        std::size_t line_size;
        std::size_t pos = 0;
        std::size_t pending_size = 0;
        unsigned char pending[3];
        std::string buffer;
    };

    /** A base64 decoder that can be used as a boost iostream filter
     *
     * @sa See create_base64_decoder() for example usage
     */
    struct base64_decoder {

        typedef char	char_type;
        struct category : bio::multichar_input_filter_tag
                        , bio::optimally_buffered_tag
        {};

        /** Constructor
         * @param line_size: line size of the encoded base64 string. This is
         *                   used just as a suggestion for better buffering.
         * @param silent: whether to throw on invalid non white space character.
         */
        base64_decoder(std::size_t line_size, bool silent)
            :line_size(line_size), silent(silent)
        {
        }

        std::streamsize optimal_buffer_size() const {
            return base64_encode_size(line_size?line_size:1024);
        }

        template<typename Device>
        std::streamsize read(Device& dev, char_type* s, std::streamsize n) {
            static const signed char *table = base64_decode_table();

            if(!n)
                return 0;

            std::streamsize count = 0;

            for(;;) {
                while(pending_out<out_count) {
                    *s++ = char_array_3[pending_out++];
                    ++count;
                    if(--n == 0) 
                        return count;
                }

                if(eof)
                    return count?count:-1;

                for(;;) {
                    int d = bio::get(dev);
                    if(d < 0) {
                        eof = true;
                        if(pending_in<=1) {
                            if(pending_in == 1 && !silent)
                                throw BOOST_IOSTREAMS_FAILURE("Unexpected ending of base64 string");
                            return count?count:-1;
                        }
                        out_count = pending_in-1;
                        pending_in = 4;
                    } else {
                        signed char c = table[d];
                        if(c < 0) {
                            if(c==-2 || silent)
                                continue;
                            throw BOOST_IOSTREAMS_FAILURE("Invalid character in base64 string");
                        }
                        char_array_4[pending_in++] = (char)c;
                    }
                    if(pending_in == 4) {
                        pending_out = pending_in = 0;
                        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
                        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
                        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
                        break;
                    }
                }
            }
        }

        std::size_t line_size;
        std::uint8_t pending_in = 0;
        char char_array_4[4];
        std::uint8_t pending_out = 3;
        std::uint8_t out_count = 3;
        char char_array_3[3];
        bool silent;
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
                    std::ostream &out, std::size_t line_size=80)
    {
        std::unique_ptr<std::ostream> res(new bio::filtering_ostream);
        bio::filtering_ostream *f = static_cast<bio::filtering_ostream*>(res.get());
        f->push(base64_encoder(line_size));
        f->push(out);
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
                    const std::string &filepath, std::size_t line_size=80) 
    {
        std::unique_ptr<std::ostream> res(new bio::filtering_ostream);
        bio::filtering_ostream *f = static_cast<bio::filtering_ostream*>(res.get());
        f->push(base64_encoder(line_size));
        f->push(bio::file_sink(filepath));
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
            std::istream &in, std::size_t line_size=80, bool silent=true)
    {
        std::unique_ptr<std::istream> res(new bio::filtering_istream);
        bio::filtering_istream *f = static_cast<bio::filtering_istream*>(res.get());
        f->push(base64_decoder(line_size,silent));
        f->push(in);
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
            const std::string &filepath, std::size_t line_size=80, bool silent=true)
    {
        std::unique_ptr<std::istream> res(new bio::filtering_istream);
        bio::filtering_istream *f = static_cast<bio::filtering_istream*>(res.get());
        f->push(base64_decoder(line_size,silent));
        f->push(bio::file_source(filepath));
        return res;
    }

} // namespace Base

#endif 
