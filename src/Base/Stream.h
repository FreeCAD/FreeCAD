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
#include <iostream>
#include <string>
#include <vector>
#include "FileInfo.h"


class QByteArray;
class QIODevice;
class QBuffer;
using PyObject = struct _object;

namespace Base {

class BaseExport Stream
{
public:
    enum ByteOrder { BigEndian, LittleEndian };

    ByteOrder byteOrder() const;
    void setByteOrder(ByteOrder);
    virtual ~Stream();

protected:
    Stream();
    Stream(const Stream&) = default;
    Stream(Stream&&) = default;
    Stream& operator=(const Stream&) = default;
    Stream& operator=(Stream&&) = default;

    bool isSwapped() const { return _swap; };

private:
    bool _swap{false};
};

/**
 * The OutputStream class provides writing of binary data to an ostream.
 * @author Werner Mayer
 */
class BaseExport OutputStream : public Stream
{
public:
    OutputStream(std::ostream &rout);
    ~OutputStream() override;

    OutputStream& operator << (bool b);
    OutputStream& operator << (int8_t ch);
    OutputStream& operator << (uint8_t uch);
    OutputStream& operator << (int16_t s);
    OutputStream& operator << (uint16_t us);
    OutputStream& operator << (int32_t i);
    OutputStream& operator << (uint32_t ui);
    OutputStream& operator << (int64_t l);
    OutputStream& operator << (uint64_t ul);
    OutputStream& operator << (float f);
    OutputStream& operator << (double d);

    OutputStream (const OutputStream&) = delete;
    OutputStream (OutputStream&&) = delete;
    void operator = (const OutputStream&) = delete;
    void operator = (OutputStream&&) = delete;

private:
    std::ostream& _out;
};

/**
 * The InputStream class provides reading of binary data from an istream.
 * @author Werner Mayer
 */
class BaseExport InputStream : public Stream
{
public:
    InputStream(std::istream &rin);
    ~InputStream() override;

    InputStream& operator >> (bool& b);
    InputStream& operator >> (int8_t& ch);
    InputStream& operator >> (uint8_t& uch);
    InputStream& operator >> (int16_t& s);
    InputStream& operator >> (uint16_t& us);
    InputStream& operator >> (int32_t& i);
    InputStream& operator >> (uint32_t& ui);
    InputStream& operator >> (int64_t& l);
    InputStream& operator >> (uint64_t& ul);
    InputStream& operator >> (float& f);
    InputStream& operator >> (double& d);

    operator bool() const
    {
        // test if _Ipfx succeeded
        return !_in.eof();
    }

    InputStream (const InputStream&) = delete;
    InputStream (InputStream&&) = delete;
    void operator = (const InputStream&) = delete;
    void operator = (InputStream&&) = delete;

private:
    std::istream& _in;
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
    ~ByteArrayOStreambuf() override;

protected:
    int_type overflow(std::streambuf::int_type v) override;
    std::streamsize xsputn (const char* s, std::streamsize num) override;
    pos_type seekoff(std::streambuf::off_type off,
        std::ios_base::seekdir way,
        std::ios_base::openmode which =
            std::ios::in | std::ios::out) override;
    pos_type seekpos(std::streambuf::pos_type sp,
        std::ios_base::openmode which =
            std::ios::in | std::ios::out) override;

public:
    ByteArrayOStreambuf(const ByteArrayOStreambuf&) = delete;
    ByteArrayOStreambuf(ByteArrayOStreambuf&&) = delete;
    ByteArrayOStreambuf& operator=(const ByteArrayOStreambuf&) = delete;
    ByteArrayOStreambuf& operator=(ByteArrayOStreambuf&&) = delete;

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
    ~ByteArrayIStreambuf() override;

protected:
    int_type uflow() override;
    int_type underflow() override;
    int_type pbackfail(int_type ch) override;
    std::streamsize showmanyc() override;
    pos_type seekoff(std::streambuf::off_type off,
        std::ios_base::seekdir way,
        std::ios_base::openmode which =
            std::ios::in | std::ios::out) override;
    pos_type seekpos(std::streambuf::pos_type pos,
        std::ios_base::openmode which =
            std::ios::in | std::ios::out) override;
public:
    ByteArrayIStreambuf(const ByteArrayIStreambuf&) = delete;
    ByteArrayIStreambuf(ByteArrayIStreambuf&&) = delete;
    ByteArrayIStreambuf& operator=(const ByteArrayIStreambuf&) = delete;
    ByteArrayIStreambuf& operator=(ByteArrayIStreambuf&&) = delete;

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
    ~IODeviceOStreambuf() override;

protected:
    int_type overflow(std::streambuf::int_type v) override;
    std::streamsize xsputn (const char* s, std::streamsize num) override;
    pos_type seekoff(std::streambuf::off_type off,
        std::ios_base::seekdir way,
        std::ios_base::openmode which =
            std::ios::in | std::ios::out) override;
    pos_type seekpos(std::streambuf::pos_type sp,
        std::ios_base::openmode which =
            std::ios::in | std::ios::out) override;
public:
    IODeviceOStreambuf(const IODeviceOStreambuf&) = delete;
    IODeviceOStreambuf(IODeviceOStreambuf&&) = delete;
    IODeviceOStreambuf& operator=(const IODeviceOStreambuf&) = delete;
    IODeviceOStreambuf& operator=(IODeviceOStreambuf&&) = delete;

private:
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
    ~IODeviceIStreambuf() override;

protected:
    int_type underflow() override;
    pos_type seekoff(std::streambuf::off_type off,
        std::ios_base::seekdir way,
        std::ios_base::openmode which =
            std::ios::in | std::ios::out) override;
    pos_type seekpos(std::streambuf::pos_type sp,
        std::ios_base::openmode which =
            std::ios::in | std::ios::out) override;
public:
    IODeviceIStreambuf(const IODeviceIStreambuf&) = delete;
    IODeviceIStreambuf(IODeviceIStreambuf&&) = delete;
    IODeviceIStreambuf& operator=(const IODeviceIStreambuf&) = delete;
    IODeviceIStreambuf& operator=(IODeviceIStreambuf&&) = delete;

private:
    QIODevice* device;
    /* data buffer:
     * - at most, pbSize characters in putback area plus
     * - at most, bufSize characters in ordinary read buffer
     */
    static const int pbSize = 4;        // size of putback area
    static const int bufSize = 1024;    // size of the data buffer
    char buffer[bufSize+pbSize]{};      // data buffer
};

class BaseExport PyStreambuf : public std::streambuf
{
    using int_type = std::streambuf::int_type;
    using pos_type = std::streambuf::pos_type;
    using off_type = std::streambuf::off_type;
    using seekdir  = std::ios::seekdir;
    using openmode = std::ios::openmode;

public:
    enum Type {
        StringIO,
        BytesIO,
        Unknown
    };

    PyStreambuf(PyObject* o, std::size_t buf_size = 256, std::size_t put_back = 8);
    ~PyStreambuf() override;
    void setType(Type t) {
        type = t;
    }

protected:
    int_type underflow() override;
    int_type overflow(int_type c = EOF) override;
    std::streamsize xsputn (const char* s, std::streamsize num) override;
    int sync() override;
    pos_type seekoff(off_type offset, seekdir dir, openmode) override;
    pos_type seekpos(pos_type offset, openmode mode) override;

private:
    bool flushBuffer();
    bool writeStr(const char* s, std::streamsize num);

public:
    PyStreambuf(const PyStreambuf&) = delete;
    PyStreambuf(PyStreambuf&&) = delete;
    PyStreambuf& operator=(const PyStreambuf&) = delete;
    PyStreambuf& operator=(PyStreambuf&&) = delete;

private:
    PyObject* inp;
    Type type{Unknown};
    const std::size_t put_back;
    std::vector<char> buffer;
};

class BaseExport Streambuf : public std::streambuf
{
public:
    explicit Streambuf(const std::string& data);
    ~Streambuf() override;

protected:
    int_type uflow() override;
    int_type underflow() override;
    int_type pbackfail(int_type ch) override;
    std::streamsize showmanyc() override;
    pos_type seekoff(std::streambuf::off_type off,
        std::ios_base::seekdir way,
        std::ios_base::openmode which =
            std::ios::in | std::ios::out) override;
    pos_type seekpos(std::streambuf::pos_type pos,
        std::ios_base::openmode which =
            std::ios::in | std::ios::out) override;

public:
    Streambuf(const Streambuf&) = delete;
    Streambuf(Streambuf&&) = delete;
    Streambuf& operator=(const Streambuf&) = delete;
    Streambuf& operator=(Streambuf&&) = delete;

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
    ofstream() = default;
    ofstream(const ofstream&) = delete;
    ofstream(ofstream&&) = delete;
    ofstream(const FileInfo& fi, ios_base::openmode mode =
                                 std::ios::out | std::ios::trunc)
#ifdef _MSC_VER
    : std::ofstream(fi.toStdWString().c_str(), mode) {}
#else
    : std::ofstream(fi.filePath().c_str(), mode) {}
#endif
    ~ofstream() override = default;
    void open(const FileInfo& fi, ios_base::openmode mode =
                                  std::ios::out | std::ios::trunc) {
#ifdef _MSC_VER
        std::ofstream::open(fi.toStdWString().c_str(), mode);
#else
        std::ofstream::open(fi.filePath().c_str(), mode);
#endif
    }

    ofstream& operator = (const ofstream&) = delete;
    ofstream& operator = (ofstream&&) = delete;
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
    ifstream() = default;
    ifstream(const ifstream&) = delete;
    ifstream(ifstream&&) = delete;
    ifstream(const FileInfo& fi, ios_base::openmode mode =
                                 std::ios::in)
#ifdef _MSC_VER
    : std::ifstream(fi.toStdWString().c_str(), mode) {}
#else
    : std::ifstream(fi.filePath().c_str(), mode) {}
#endif
    ~ifstream() override = default;
    void open(const FileInfo& fi, ios_base::openmode mode =
                                  std::ios::in) {
#ifdef _MSC_VER
        std::ifstream::open(fi.toStdWString().c_str(), mode);
#else
        std::ifstream::open(fi.filePath().c_str(), mode);
#endif
    }

    ifstream& operator = (const ifstream&) = delete;
    ifstream& operator = (ifstream&&) = delete;
};

} // namespace Base

#endif // BASE_STREAM_H
