/***************************************************************************
 *   Copyright (c) 2007 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef BASE_STREAM_H
#define BASE_STREAM_H

#ifdef __GNUC__
# include <cstdint>
#endif

#include <fstream>
#include <ios>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "Swap.h"
#include "FileInfo.h"


class QByteArray;
class QIODevice;
class QBuffer;
typedef struct _object PyObject;

namespace Base {

class BaseExport Stream
{
public:
    enum ByteOrder { BigEndian, LittleEndian };

    ByteOrder byteOrder() const;
    void setByteOrder(ByteOrder);

protected:
    Stream();
    Stream(const Stream&) = default;
    Stream& operator=(const Stream&) = default;
    virtual ~Stream();

    bool _swap;
};

/**
 * The OutputStream class provides writing of binary data to an ostream.
 * @author Werner Mayer
 */
class BaseExport OutputStream : public Stream
{
public:
    /** Constructor
     * @param rout: output downstream
     * @param binary: whether to output as text or not
     */
    OutputStream(std::ostream &rout, bool binary=true);

    OutputStream& operator << (bool b) {
        if(_binary)
            _out.write((const char*)&b, sizeof(bool));
        else
            _out << b << '\n';
        return *this;
    }

    OutputStream& operator << (int8_t ch) {
        if(_binary)
            _out.write((const char*)&ch, sizeof(int8_t));
        else 
            _out << ch << '\n';
        return *this;
    }

    OutputStream& operator << (uint8_t uch) {
        if(_binary)
            _out.write((const char*)&uch, sizeof(uint8_t));
        else
            _out << uch << '\n';
        return *this;
    }

    OutputStream& operator << (int16_t s) {
        if(_binary) {
            if (_swap) SwapEndian<int16_t>(s);
            _out.write((const char*)&s, sizeof(int16_t));
        }else 
            _out << s << '\n';
        return *this;
    }

    OutputStream& operator << (uint16_t us) {
        if(_binary) {
            if (_swap) SwapEndian<uint16_t>(us);
            _out.write((const char*)&us, sizeof(uint16_t));
        }else 
            _out << us << '\n';
        return *this;
    }

    OutputStream& operator << (int32_t i) {
        if(_binary) {
            if (_swap) SwapEndian<int32_t>(i);
            _out.write((const char*)&i, sizeof(int32_t));
        }else 
            _out << i << '\n';
        return *this;
    }

    OutputStream& operator << (uint32_t ui) {
        if(_binary) {
            if (_swap) SwapEndian<uint32_t>(ui);
            _out.write((const char*)&ui, sizeof(uint32_t));
        }else 
            _out << ui << '\n';
        return *this;
    }

    OutputStream& operator << (int64_t l) {
        if(_binary) {
            if (_swap) SwapEndian<int64_t>(l);
            _out.write((const char*)&l, sizeof(int64_t));
        }else 
            _out << l << '\n';
        return *this;
    }

    OutputStream& operator << (uint64_t ul) {
        if(_binary) {
            if (_swap) SwapEndian<uint64_t>(ul);
            _out.write((const char*)&ul, sizeof(uint64_t));
        }else 
            _out << ul << '\n';
        return *this;
    }

    OutputStream& operator << (float f) {
        if(_binary) {
            if (_swap) SwapEndian<float>(f);
            _out.write((const char*)&f, sizeof(float));
        }else 
            _out << f << '\n';
        return *this;
    }

    OutputStream& operator << (double d) {
        if(_binary) {
            if (_swap) SwapEndian<double>(d);
            _out.write((const char*)&d, sizeof(double));
        }else 
            _out << d << '\n';
        return *this;
    }

    OutputStream& operator << (const char *s);

    OutputStream& operator << (const std::string &s) {
        return (*this) << s.c_str();
    }

    OutputStream& operator << (char c) {
        _out.put(c);
        return *this;
    }


    bool isBinary() const {return _binary;}

private:
    OutputStream (const OutputStream&);
    void operator = (const OutputStream&);

private:
    std::ostream& _out;
    bool _binary;
};

/**
 * The InputStream class provides reading of binary data from an istream.
 * @author Werner Mayer
 */
class BaseExport InputStream : public Stream
{
public:
    /** Constructor
     * @param rin: upstream input
     * @param binary: whether read the stream as text or not
     */
    InputStream(std::istream &rin, bool binary=true);

    InputStream& operator >> (bool& b) {
        if(_binary)
            _in.read((char*)&b, sizeof(bool));
        else
            _in >> b;
        return *this;
    }

    InputStream& operator >> (int8_t& ch) {
        if(_binary)
            _in.read((char*)&ch, sizeof(int8_t));
        else {
            int i;
            _in >> i;
            ch = (int8_t)i;
        }
        return *this;
    }

    InputStream& operator >> (uint8_t& uch) {
        if(_binary)
            _in.read((char*)&uch, sizeof(uint8_t));
        else {
            unsigned u;
            _in >> u;
            uch = (uint8_t)u;
        }
        return *this;
    }

    InputStream& operator >> (int16_t& s) {
        if(_binary) {
            _in.read((char*)&s, sizeof(int16_t));
            if (_swap) SwapEndian<int16_t>(s);
        } else 
            _in >> s;
        return *this;
    }

    InputStream& operator >> (uint16_t& us) {
        if(_binary) {
            _in.read((char*)&us, sizeof(uint16_t));
            if (_swap) SwapEndian<uint16_t>(us);
        } else 
            _in >> us;
        return *this;
    }

    InputStream& operator >> (int32_t& i) {
        if(_binary) {
            _in.read((char*)&i, sizeof(int32_t));
            if (_swap) SwapEndian<int32_t>(i);
        } else 
            _in >> i;
        return *this;
    }

    InputStream& operator >> (uint32_t& ui) {
        if(_binary) {
            _in.read((char*)&ui, sizeof(uint32_t));
            if (_swap) SwapEndian<uint32_t>(ui);
        } else 
            _in >> ui;
        return *this;
    }

    InputStream& operator >> (int64_t& l) {
        if(_binary) {
            _in.read((char*)&l, sizeof(int64_t));
            if (_swap) SwapEndian<int64_t>(l);
        } else 
            _in >> l;
        return *this;
    }

    InputStream& operator >> (uint64_t& ul) {
        if(_binary) {
            _in.read((char*)&ul, sizeof(uint64_t));
            if (_swap) SwapEndian<uint64_t>(ul);
        } else 
            _in >> ul;
        return *this;
    }

    InputStream& operator >> (float& f) {
        if(_binary) {
            _in.read((char*)&f, sizeof(float));
            if (_swap) SwapEndian<float>(f);
        } else 
            _in >> f;
        return *this;
    }

    InputStream& operator >> (double& d) {
        if(_binary) {
            _in.read((char*)&d, sizeof(double));
            if (_swap) SwapEndian<double>(d);
        } else 
            _in >> d;
        return *this;
    }

    InputStream& operator >> (std::string &s);

    InputStream& operator >> (char &c) {
        c = (char)_in.get();
        return *this;
    }

    operator bool() const
    {
        // test if _Ipfx succeeded
        return !_in.eof();
    }

    bool isBinary() const {return _binary;}

private:
    InputStream (const InputStream&);
    void operator = (const InputStream&);

private:
    std::istream& _in;
    std::ostringstream _ss;
    bool _binary;
};

// ----------------------------------------------------------------------------

/**
 * This class implements the streambuf interface to write data to a QByteArray.
 * This class can only be used for writing but not for reading purposes.
 * @author Werner Mayer
 */
class BaseExport ByteArrayOStreambuf : public std::streambuf
{
public:
    explicit ByteArrayOStreambuf(QByteArray& ba);
    ~ByteArrayOStreambuf();

protected:
    virtual int_type overflow(std::streambuf::int_type v);
    virtual std::streamsize xsputn (const char* s, std::streamsize num);
    virtual pos_type seekoff(std::streambuf::off_type off,
        std::ios_base::seekdir way,
        std::ios_base::openmode which =
            std::ios::in | std::ios::out);
    virtual pos_type seekpos(std::streambuf::pos_type sp,
        std::ios_base::openmode which =
            std::ios::in | std::ios::out);

private:
    ByteArrayOStreambuf(const ByteArrayOStreambuf&);
    ByteArrayOStreambuf& operator=(const ByteArrayOStreambuf&);

private:
    QBuffer* _buffer;
};

/**
 * This class implements the streambuf interface to read data from a QByteArray.
 * This class can only be used for reading but not for writing purposes.
 * @author Werner Mayer
 */
class BaseExport ByteArrayIStreambuf : public std::streambuf
{
public:
    explicit ByteArrayIStreambuf(const QByteArray& buf);
    ~ByteArrayIStreambuf();

protected:
    virtual int_type uflow();
    virtual int_type underflow();
    virtual int_type pbackfail(int_type ch);
    virtual std::streamsize showmanyc();
    virtual pos_type seekoff(std::streambuf::off_type off,
        std::ios_base::seekdir way,
        std::ios_base::openmode which =
            std::ios::in | std::ios::out);
    virtual pos_type seekpos(std::streambuf::pos_type pos,
        std::ios_base::openmode which =
            std::ios::in | std::ios::out);
private:
    ByteArrayIStreambuf(const ByteArrayIStreambuf&);
    ByteArrayIStreambuf& operator=(const ByteArrayIStreambuf&);

private:
    const QByteArray& _buffer;
    int _beg, _end, _cur;
};

/**
 * Simple class to write data directly into Qt's QIODevice.
 * This class can only be used for writing but not reading purposes.
 * @author Werner Mayer
 */
class BaseExport IODeviceOStreambuf : public std::streambuf
{
public:
    IODeviceOStreambuf(QIODevice* dev);
    ~IODeviceOStreambuf();

protected:
    virtual int_type overflow(std::streambuf::int_type v);
    virtual std::streamsize xsputn (const char* s, std::streamsize num);
    virtual pos_type seekoff(std::streambuf::off_type off,
        std::ios_base::seekdir way,
        std::ios_base::openmode which =
            std::ios::in | std::ios::out);
    virtual pos_type seekpos(std::streambuf::pos_type sp,
        std::ios_base::openmode which =
            std::ios::in | std::ios::out);
private:
    IODeviceOStreambuf(const IODeviceOStreambuf&);
    IODeviceOStreambuf& operator=(const IODeviceOStreambuf&);

protected:
    QIODevice* device;
};

/**
 * Simple class to read data directly from Qt's QIODevice.
 * This class can only be used for readihg but not writing purposes.
 * @author Werner Mayer
 */
class BaseExport IODeviceIStreambuf : public std::streambuf
{
public:
    IODeviceIStreambuf(QIODevice* dev);
    ~IODeviceIStreambuf();

protected:
    virtual int_type underflow();
    virtual pos_type seekoff(std::streambuf::off_type off,
        std::ios_base::seekdir way,
        std::ios_base::openmode which =
            std::ios::in | std::ios::out);
    virtual pos_type seekpos(std::streambuf::pos_type sp,
        std::ios_base::openmode which =
            std::ios::in | std::ios::out);
private:
    IODeviceIStreambuf(const IODeviceIStreambuf&);
    IODeviceIStreambuf& operator=(const IODeviceIStreambuf&);

protected:
    QIODevice* device;
    /* data buffer:
     * - at most, pbSize characters in putback area plus
     * - at most, bufSize characters in ordinary read buffer
     */
    static const int pbSize = 4;        // size of putback area
    static const int bufSize = 1024;    // size of the data buffer
    char buffer[bufSize+pbSize];        // data buffer
};

class BaseExport PyStreambuf : public std::streambuf
{
    typedef std::streambuf::int_type int_type;
    typedef std::streambuf::pos_type pos_type;
    typedef std::streambuf::off_type off_type;
    typedef std::ios::seekdir        seekdir;
    typedef std::ios::openmode       openmode;

public:
    enum Type {
        StringIO,
        BytesIO,
        Unknown
    };

    PyStreambuf(PyObject* o, std::size_t buf_size = 256, std::size_t put_back = 8);
    virtual ~PyStreambuf();
    void setType(Type t) {
        type = t;
    }

protected:
    int_type underflow();
    int_type overflow(int_type c = EOF);
    std::streamsize xsputn (const char* s, std::streamsize num);
    int sync();
    pos_type seekoff(off_type offset, seekdir dir, openmode);
    pos_type seekpos(pos_type offset, openmode mode);

private:
    bool flushBuffer();
    bool writeStr(const char* s, std::streamsize num);

private:
    PyStreambuf(const PyStreambuf&);
    PyStreambuf& operator=(const PyStreambuf&);

private:
    PyObject* inp;
    Type type;
    const std::size_t put_back;
    std::vector<char> buffer;
};

class BaseExport Streambuf : public std::streambuf
{
public:
    explicit Streambuf(const std::string& data);
    ~Streambuf();

protected:
    virtual int_type uflow();
    virtual int_type underflow();
    virtual int_type pbackfail(int_type ch);
    virtual std::streamsize showmanyc();
    virtual pos_type seekoff(std::streambuf::off_type off,
        std::ios_base::seekdir way,
        std::ios_base::openmode which =
            std::ios::in | std::ios::out);
    virtual pos_type seekpos(std::streambuf::pos_type pos,
        std::ios_base::openmode which =
            std::ios::in | std::ios::out);

private:
    Streambuf(const Streambuf&);
    Streambuf& operator=(const Streambuf&);

private:
    std::string::const_iterator _beg;
    std::string::const_iterator _end;
    std::string::const_iterator _cur;
};

// ----------------------------------------------------------------------------

class FileInfo;

/**
 * The ofstream class is provided for convenience. On Windows
 * platforms it opens a stream with UCS-2 encoded file name
 * while on Linux platforms the file name is UTF-8 encoded.
 * @author Werner Mayer
 */
class ofstream : public std::ofstream
{
public:
    ofstream()
    {}

    ofstream(const FileInfo& fi, ios_base::openmode mode =
                                 std::ios::out | std::ios::trunc)
#ifdef _MSC_VER
    : std::ofstream(fi.toStdWString().c_str(), mode)
#else
    : std::ofstream(fi.filePath().c_str(), mode)
#endif
    {
    }

    void open(const FileInfo &fi, ios_base::openmode mode =
                                 std::ios::out | std::ios::trunc)
    {
#ifdef _MSC_VER
        std::ofstream::open(fi.toStdWString().c_str(), mode);
#else
        std::ofstream::open(fi.filePath().c_str(), mode);
#endif
    }

    virtual ~ofstream()
    {
    }
};

/**
 * The ofstream class is provided for convenience. On Windows
 * platforms it opens a stream with UCS-2 encoded file name
 * while on Linux platforms the file name is UTF-8 encoded.
 * @author Werner Mayer
 */
class ifstream : public std::ifstream
{
public:
    ifstream() {}

    ifstream(const FileInfo& fi, ios_base::openmode mode =
                                 std::ios::in)
#ifdef _MSC_VER
    : std::ifstream(fi.toStdWString().c_str(), mode)
#else
    : std::ifstream(fi.filePath().c_str(), mode)
#endif
    {
    }

    void open(const FileInfo &fi, ios_base::openmode mode =
                                 std::ios::in)
    {
#ifdef _MSC_VER
        std::ifstream::open(fi.toStdWString().c_str(), mode);
#else
        std::ifstream::open(fi.filePath().c_str(), mode);
#endif
    }

    virtual ~ifstream()
    {
    }
};

} // namespace Base

#endif // BASE_STREAM_H
